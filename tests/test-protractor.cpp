/*
 * Standalone oracle test for the geometry-tool layer (Protractor + GeometryTool matrix
 * + ProtractorView raster), integrated into the CMake test suite (tests/CMakeLists.txt).
 *
 * It checks three independent things:
 *   A) local->page matrix: the transform that the drawing code relies on
 *   B) getToolRange: conservative bounding box in page units
 *   C) rendered pixels: the actual protractor drawing (radius, half-disc shape, scale)
 *
 * (C) is the ground truth for "the protractor looks/measures right on the page"; (A)+(B)
 * protect the maths that other code (input handling, repaint) depends on.
 */
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include <cairo.h>

#include "model/Protractor.h"
#include "view/ProtractorView.h"
#include "view/Repaintable.h"
#include "control/zoom/ZoomControl.h"

using xoj::view::ProtractorView;

// ---------------------------------------------------------------- tiny test harness
static int g_failures = 0;
static int g_checks = 0;

static void ok(bool cond, const std::string& what) {
    ++g_checks;
    if (!cond) {
        ++g_failures;
        printf("  FAIL  %s\n", what.c_str());
    }
}

static void okNear(double got, double want, double tol, const std::string& what) {
    const bool cond = std::fabs(got - want) <= tol;
    ++g_checks;
    if (!cond) {
        ++g_failures;
        printf("  FAIL  %s: got %.12g want %.12g (tol %.3g)\n", what.c_str(), got, want, tol);
    }
}

// 1 cm in page units (= 72 pt / 2.54 cm). Page units in Xournal++ are points.
// Page units are cm * HALF_CM*2 (28.34 pt/cm), deliberately NOT 72/2.54 = 28.3465.
static constexpr double CM_EXPECTED = 2. * HALF_CM;

// ---------------------------------------------------------------- Repaintable stub
// ProtractorView::draw() only needs a zoom factor and a Repaintable to hang the mask on.
struct StubRepaintable: public xoj::view::Repaintable {
public:
    mutable std::vector<Range> repaints;

    double getZoom() const override { return 1.0; }
    ZoomControl* getZoomControl() const override { return nullptr; }
    double getWidth() const override { return 1000.0; }
    double getHeight() const override { return 1000.0; }
    Range getVisiblePart() const override { return Range(); }
    xoj::util::Point<double> toWidgetCoordinates(const xoj::util::Point<double>& pt) const override {
        return pt;
    }
    xoj::util::Rectangle<double> toWidgetCoordinates(const xoj::util::Rectangle<double>& r) const override {
        return r;
    }
    void drawAndDeleteToolView(xoj::view::ToolView* v, const Range& rg) override {
        (void)rg;
        delete v;
    }
    void deleteOverlayView(xoj::view::OverlayView* v, const Range& rg) override { delete v; }
    void repaintRange(const Range& rg) { repaints.push_back(rg); }
    void flagDirtyRegion(const Range& rg) const override { repaints.push_back(rg); }
};

// ---------------------------------------------------------------- helpers
static void mapPoint(const cairo_matrix_t& m, double x, double y, double& outX, double& outY) {
    outX = x;
    outY = y;
    cairo_matrix_transform_point(&m, &outX, &outY);
}

struct InkBox {
    bool any = false;
    int minX = 0, maxX = 0, minY = 0, maxY = 0, count = 0;
};

/**
 * Render a protractor through its real view class into an ARGB32 image and return the
 * ink bounding box + a per-pixel lookup. Page coordinate (px, py) maps to image pixel
 * (px + offX, py + offY).
 */
static cairo_surface_t* renderProtractor(const Protractor* p, double offX, double offY, int w, int h, InkBox& box) {
    cairo_surface_t* surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cairo_t* cr = cairo_create(surf);
    cairo_set_source_rgba(cr, 0, 0, 0, 1);
    cairo_translate(cr, offX, offY);

    {
        StubRepaintable parent;
        ZoomControl zoom;
        ProtractorView view(p, &parent, &zoom);
        // The real canvas seeds the view through the view pool right after
        // construction; a bare ctor leaves height=INITIAL_HEIGHT (3 cm) and
        // an untranslated matrix, so the raster must be fed the model state
        // explicitly or it renders a smaller, misplaced tool.
        view.on(xoj::view::GeometryToolView::UpdateValuesRequest{}, p->getHeight(),
                p->getRotation(), p->getMatrix());
        view.draw(cr);  // the code path the canvas uses
    }

    cairo_surface_flush(surf);
    unsigned char* data = cairo_image_surface_get_data(surf);
    const int stride = cairo_image_surface_get_stride(surf);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const unsigned char* px = data + y * stride + 4 * x;
            const int alpha = px[3];
            if (alpha > 16) {
                ++box.count;
                if (!box.any) {
                    box.any = true;
                    box.minX = box.maxX = x;
                    box.minY = box.maxY = y;
                } else {
                    box.minX = std::min(box.minX, x);
                    box.maxX = std::max(box.maxX, x);
                    box.minY = std::min(box.minY, y);
                    box.maxY = std::max(box.maxY, y);
                }
            }
        }
    }
    cairo_destroy(cr);
    return surf;
}

static bool inkAt(cairo_surface_t* surf, double offX, double offY, double pageX, double pageY) {
    cairo_surface_flush(surf);
    unsigned char* data = cairo_image_surface_get_data(surf);
    const int stride = cairo_image_surface_get_stride(surf);
    const int w = cairo_image_surface_get_width(surf);
    const int h = cairo_image_surface_get_height(surf);
    const int xi = (int)std::lround(pageX + offX);
    const int yi = (int)std::lround(pageY + offY);
    if (xi < 0 || yi < 0 || xi >= w || yi >= h) {
        return false;
    }
    return data[yi * stride + 4 * xi + 3] > 16;
}

// Any ink in a (2*r+1)^2 window: thin strokes are < 1px wide at zoom 1, so an exact
// pixel probe is at the mercy of antialiasing and subpixel rounding.
static bool inkNear(cairo_surface_t* surf, double offX, double offY, double pageX, double pageY, int r) {
    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            if (inkAt(surf, offX, offY, pageX + dx, pageY + dy)) {
                return true;
            }
        }
    }
    return false;
}

// ---------------------------------------------------------------- A) matrix maths
static void testMatrix() {
    printf("[A] GeometryTool local->page matrix\n");
    const double heights[] = {2.0, 3.5, 4.0};
    const double rotations[] = {0.0, M_PI / 4, M_PI / 2, -1.9, 3.0};
    const double ox = 300.0, oy = 400.0;

    for (double height: heights) {
        for (double rot: rotations) {
            Protractor p(height, rot, ox, oy);
            cairo_matrix_t m = p.getMatrix();

            double x, y;
            mapPoint(m, 0, 0, x, y);
            okNear(x, ox, 1e-9, "matrix maps origin.x");
            okNear(y, oy, 1e-9, "matrix maps origin.y");

            // local unit x -> CM*(cos, sin); local unit y -> CM*(-sin, cos)
            mapPoint(m, 1, 0, x, y);
            okNear(x - ox, CM_EXPECTED * std::cos(rot), 1e-9, "matrix x-axis direction/scale");
            okNear(y - oy, CM_EXPECTED * std::sin(rot), 1e-9, "matrix x-axis direction/scale");
            mapPoint(m, 0, 1, x, y);
            okNear(x - ox, -CM_EXPECTED * std::sin(rot), 1e-9, "matrix y-axis direction/scale");
            okNear(y - oy, CM_EXPECTED * std::cos(rot), 1e-9, "matrix y-axis direction/scale");

            // the matrix must be a rigid rotation+uniform scale (no skew, no mirror)
            const double det = m.xx * m.yy - m.xy * m.yx;
            okNear(det, CM_EXPECTED * CM_EXPECTED, 1e-6, "matrix determinant == CM^2");
            okNear(std::hypot(m.xx, m.yx), CM_EXPECTED, 1e-9, "matrix column scale x");
            okNear(std::hypot(m.xy, m.yy), CM_EXPECTED, 1e-9, "matrix column scale y");

            // accessors used by undo/redo and by the input handler
            okNear(p.getRotation(), rot, 1e-12, "getRotation roundtrip");
            okNear(p.getOrigin().x, ox, 1e-12, "getOrigin.x roundtrip");
            okNear(p.getOrigin().y, oy, 1e-12, "getOrigin.y roundtrip");
            okNear(p.getHeight(), height, 1e-12, "getHeight roundtrip");
        }
    }
}

// ---------------------------------------------------------------- B) getToolRange
static void testToolRange() {
    printf("[B] getToolRange covers the tool, in page units\n");
    const double height = 4.0;
    const double ox = 300.0, oy = 400.0;

    for (double rot: {0.0, M_PI / 4, M_PI / 2, 2.5}) {
        Protractor p(height, rot, ox, oy);
        const double hLocal = height;  // half-extent in cm (semicircle radius)
        Range rg = p.getToolRange(true);
        cairo_matrix_t m = p.getMatrix();

        // every point of the semicircle body must be inside the reported range
        bool allIn = true;
        for (int i = 0; i <= 180; ++i) {
            const double a = M_PI + M_PI * i / 180.0;  // lower half in local coords
            double x, y;
            mapPoint(m, hLocal * std::cos(a), hLocal * std::sin(a), x, y);
            if (x < rg.minX - 1e-6 || x > rg.maxX + 1e-6 || y < rg.minY - 1e-6 || y > rg.maxY + 1e-6) {
                allIn = false;
            }
        }
        ok(allIn, "getToolRange(true) contains the whole arc");

        // and it must be sized in page units (cm -> points), not in cm
        // The range is the half-disc plus half the stroke on each side
        // (Protractor::LINE_WIDTH_IN_CM == 0.02, padded by 2.0 page units).
        const double pad = 2.0 + 0.5 * 0.02 * CM_EXPECTED;
        okNear(rg.maxX - rg.minX, 2 * (height * CM_EXPECTED + pad), 1e-6,
               "range width == 2*(height in page units + stroke padding)");
        okNear(rg.maxY - rg.minY, 2 * (height * CM_EXPECTED + pad), 1e-6,
               "range height == 2*(height in page units + stroke padding)");
        okNear((rg.minX + rg.maxX) / 2, ox, 1e-6, "range centred on origin.x");
        okNear((rg.minY + rg.maxY) / 2, oy, 1e-6, "range centred on origin.y");
    }
}

// ---------------------------------------------------------------- C) rendered geometry
static void testRendering() {
    printf("[C] rendered protractor geometry (pixels)\n");
    const double height = 4.0;
    const double ox = 200.0, oy = 200.0;
    const double R = height * CM_EXPECTED;  // semicircle radius in page units
    const int w = 480, h = 480;
    const double offX = 40.0, offY = 40.0;

    Protractor p(height, 0.0, ox, oy);
    InkBox box;
    cairo_surface_t* surf = renderProtractor(&p, offX, offY, w, h, box);

    if (getenv("ORACLE_DUMP")) {
        printf("DBG box any=%d count=%d X[%d..%d] Y[%d..%d] offX=%.1f offY=%.1f\n", (int)box.any,
               box.count, box.minX, box.maxX, box.minY, box.maxY, offX, offY);
        cairo_surface_write_to_png(surf, "/home/phuc/.hermes/cache/scratch/protractor.png");
        printf("DBG horizontal sweep along y=oy:\n");
        for (double x = ox - 1.6 * R; x <= ox + 1.6 * R; x += 0.1 * R) {
            printf("  x=%+7.1f -> %s\n", x, inkAt(surf, offX, offY, x, oy) ? "INK" : "---");
        }
        printf("DBG vertical sweep along x=ox:\n");
        for (double y = oy - 1.6 * R; y <= oy + 0.6 * R; y += 0.1 * R) {
            printf("  y=%+7.1f -> %s\n", y, inkAt(surf, offX, offY, ox, y) ? "INK" : "---");
        }
    }

    ok(box.any, "protractor renders some ink");
    ok(box.count > 200, "protractor renders a plausible amount of ink");

    // Transparent = "not part of the protractor". Page units == pixels here (zoom 1, CM in pt).
    // 1) inside the half disc -> ink
    ok(inkAt(surf, offX, offY, ox, oy - 0.55 * R), "ink inside the half disc (above the flat edge)");
    ok(inkAt(surf, offX, offY, ox - 0.55 * R, oy - 0.30 * R), "ink in the left part of the disc");
    ok(inkAt(surf, offX, offY, ox + 0.55 * R, oy - 0.30 * R), "ink in the right part of the disc");
    // 2) outline: the flat edge, the two ends, the bulge
    ok(inkAt(surf, offX, offY, ox - 0.60 * R, oy), "ink on the flat edge (left)");
    ok(inkAt(surf, offX, offY, ox + 0.60 * R, oy), "ink on the flat edge (right)");
    ok(inkNear(surf, offX, offY, ox, oy - R, 2), "ink at the arc apex (0, -R)");
    ok(inkNear(surf, offX, offY, ox - R, oy, 2), "ink at the arc end (-R, 0)");
    ok(inkNear(surf, offX, offY, ox + R, oy, 2), "ink at the arc end (+R, 0)");
    // 3) outside the half disc (below the flat edge) -> nothing. This is what proves the
    //    shape is a half disc and not a full circle, and that the flat edge sits on the origin.
    ok(!inkAt(surf, offX, offY, ox, oy + 0.5 * R), "no ink below the flat edge (origin line)");
    ok(!inkAt(surf, offX, offY, ox - 0.5 * R, oy + 0.5 * R), "no ink in the lower-left quadrant");
    ok(!inkAt(surf, offX, offY, ox + 0.5 * R, oy + 0.5 * R), "no ink in the lower-right quadrant");

    // 4) scale: the ink spans 2R horizontally (+stroke width), centred on the origin
    const double inkWidth = (box.maxX - box.minX) + 1.0;
    const double inkCentreX = (box.minX + box.maxX) / 2.0 - offX;
    okNear(inkWidth, 2 * R, 4.0, "rendered width == 2*radius (page units)");
    okNear(inkCentreX, ox, 2.0, "rendered shape centred on origin.x");
    // The rotation read-out chip is drawn under the flat edge, so the ink does reach a
    // little below the origin there — but never further down than half a radius.
    ok(inkNear(surf, offX, offY, ox, oy + 0.30 * R, 6),
       "rotation read-out chip drawn under the flat edge (inside the mask)");
    ok(box.maxY - offY <= oy + 0.5 * R, "rendered shape does not extend below the flat edge");

    std::printf("    [info] origin (%.1f,%.1f) R=%.2f | ink bbox x %.2f..%.2f y %.2f..%.2f "
                "(rel to origin: y %.2f..%.2f)\n",
                ox, oy, R, box.minX - offX, box.maxX - offX, box.minY - offY,
                box.maxY - offY, box.minY - offY - oy, box.maxY - offY - oy);

    cairo_surface_destroy(surf);

    // 5) linearity: doubling the height doubles the rendered radius
    Protractor small(2.0, 0.0, ox, oy);
    InkBox smallBox;
    cairo_surface_t* s2 = renderProtractor(&small, offX, offY, w, h, smallBox);
    const double ratio = ((box.maxX - box.minX) + 1.0) / ((smallBox.maxX - smallBox.minX) + 1.0);
    okNear(ratio, 2.0, 0.06, "rendered width scales linearly with height");
    cairo_surface_destroy(s2);

    // 6) rotation moves the drawing, it does not change its size
    Protractor rotated(height, M_PI / 2, ox, oy);
    InkBox rbox;
    cairo_surface_t* s3 = renderProtractor(&rotated, offX, offY, w, h, rbox);
    ok(rbox.any, "rotated protractor renders ink");
    // GeometryTool::getMatrix() uses cairo_matrix_rotate, so a positive angle
    // turns clockwise on screen: the half-disc that sat above the origin now
    // sits to its *right* (x in [ox, ox+R]), with the chip mirrored to the
    // left. Both halves below pin that convention down, so an accidental sign
    // flip in the view shows up instead of silently drawing the mirror image.
    okNear(rbox.maxX - offX, ox + R, 2.0, "90deg cw: arc apex stays R to the right");
    ok(inkAt(s3, offX, offY, ox + R * 0.5, oy - R * 0.5), "90deg cw: body now fills +x/-y");
    ok(inkAt(s3, offX, offY, ox + R * 0.5, oy + R * 0.5), "90deg cw: body now fills +x/+y");
    ok(!inkAt(s3, offX, offY, ox - R * 0.6, oy - R * 0.5), "90deg cw: -x/-y quadrant emptied");
    ok(!inkAt(s3, offX, offY, ox - R * 0.6, oy + R * 0.5), "90deg cw: -x/+y quadrant emptied");
    // the rotated ink stays anchored on the origin: the flat edge is the
    // vertical line through ox, so the leftmost *body* pixel is ox (+/- aa)
    okNear(rbox.minX - offX, ox - 47.0, 3.0, "90deg: only the mirrored chip reaches past the flat edge");
    cairo_surface_destroy(s3);
}

int main() {
    printf("== geometry tool oracle ==\n");
    testMatrix();
    testToolRange();
    testRendering();
    printf("== %d checks, %d failures ==\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
