#include "SolidGeometry.h"

#include <algorithm>  // for max
#include <cmath>      // for abs
#include <optional>   // for optional
#include <string>     // for string

namespace {
// Box corners: the front face first, in the order the outline is drawn, then each back corner,
// which is the matching front corner pushed along the depth vector.
constexpr std::size_t BA = 0, BB = 1, BCC = 2, BDD = 3;      // box front
constexpr std::size_t BA2 = 4, BB2 = 5, BCC2 = 6, BDD2 = 7;  // box back
// Box faces
constexpr std::size_t BOX_FRONT = 0, BOX_RIGHT = 1, BOX_BOTTOM = 2, BOX_LEFT = 3, BOX_TOP = 4, BOX_BACK = 5;

// Pyramid corners
constexpr std::size_t P1 = 0, P2 = 1, P3 = 2, P4 = 3, APEX = 4;
// Pyramid faces
constexpr std::size_t P_BASE = 0, P_FRONT = 1, P_RIGHT = 2, P_BACK = 3, P_LEFT = 4;
}  // namespace

auto SolidGeometry::boxWireframe(const Point& f1, const Point& f2, double depthFraction, double dx, double dy)
        -> Wireframe {
    const double size = std::max(std::abs(f2.x - f1.x), std::abs(f2.y - f1.y));
    const double depth = depthFraction * size;
    const Point d{dx * depth, dy * depth};

    Wireframe w;
    w.vertices = {
            Point{f1.x, f1.y},              // BA
            Point{f2.x, f1.y},              // BB
            Point{f2.x, f2.y},              // BCC
            Point{f1.x, f2.y},              // BDD
            Point{f1.x + d.x, f1.y + d.y},  // BA2
            Point{f2.x + d.x, f1.y + d.y},  // BB2
            Point{f2.x + d.x, f2.y + d.y},  // BCC2
            Point{f1.x + d.x, f2.y + d.y},  // BDD2
    };

    // Every face is listed in one rotational order, so the sign of its projected area says which
    // way it points; every edge names the two faces that meet at it.
    w.faces = {
            {BA, BB, BCC, BDD},    // front
            {BB, BB2, BCC2, BCC},   // right
            {BCC, BCC2, BDD2, BDD},  // bottom
            {BDD, BDD2, BA2, BA},   // left
            {BA, BA2, BB2, BB},    // top
            {BB2, BA2, BDD2, BCC2},  // back
    };
    w.edges = {
            {BA, BB, BOX_FRONT, BOX_TOP},     {BB, BCC, BOX_FRONT, BOX_RIGHT},  {BCC, BDD, BOX_FRONT, BOX_BOTTOM},
            {BDD, BA, BOX_FRONT, BOX_LEFT},   {BA, BA2, BOX_TOP, BOX_LEFT},     {BB, BB2, BOX_TOP, BOX_RIGHT},
            {BCC, BCC2, BOX_RIGHT, BOX_BOTTOM}, {BDD, BDD2, BOX_LEFT, BOX_BOTTOM}, {BA2, BB2, BOX_BACK, BOX_TOP},
            {BB2, BCC2, BOX_BACK, BOX_RIGHT},  {BCC2, BDD2, BOX_BACK, BOX_BOTTOM}, {BDD2, BA2, BOX_BACK, BOX_LEFT},
    };
    w.visibleFace = BOX_FRONT;  // visible whatever the depth direction
    return w;
}

auto SolidGeometry::pyramidWireframe(const Point& baseCentre, double baseWidth, double apexHeight, double depthFraction,
                                     double dx, double dy) -> Wireframe {
    const double halfW = baseWidth / 2.0;
    const Point d{dx * depthFraction * baseWidth, dy * depthFraction * baseWidth};

    Wireframe w;
    w.vertices = {
            Point{baseCentre.x - halfW, baseCentre.y},                  // b1 front-left
            Point{baseCentre.x + halfW, baseCentre.y},                  // b2 front-right
            Point{baseCentre.x + halfW + d.x, baseCentre.y + d.y},      // b3 back-right
            Point{baseCentre.x - halfW + d.x, baseCentre.y + d.y},      // b4 back-left
            Point{baseCentre.x + d.x / 2.0, baseCentre.y - apexHeight},  // apex
    };

    w.faces = {
            // The base must be listed in the SAME rotational order as the lateral faces, otherwise
            // the sign of its projected area is the sign of a face pointing the other way and the
            // classification inverts. The base as {P1, P2, P3, P4} runs the opposite way round from
            // the lateral faces and made every figure come out with the wrong edges dashed; as
            // {P4, P3, P2, P1} it agrees with them.
            {P4, P3, P2, P1},  // base
            {P1, P2, APEX},    // lateral face standing on the front edge
            {P2, P3, APEX},    // lateral face standing on the right edge
            {P3, P4, APEX},    // lateral face standing on the back edge
            {P4, P1, APEX},    // lateral face standing on the left edge
    };
    w.edges = {
            {P1, P2, P_BASE, P_FRONT}, {P2, P3, P_BASE, P_RIGHT}, {P3, P4, P_BASE, P_BACK}, {P4, P1, P_BASE, P_LEFT},
            {P1, APEX, P_FRONT, P_LEFT}, {P2, APEX, P_FRONT, P_RIGHT}, {P3, APEX, P_RIGHT, P_BACK},
            {P4, APEX, P_BACK, P_LEFT},
    };
    // The front lateral face is the reference: it is the face the viewer looks at when the figure
    // is drawn upright, and with the base wound the same way round as the lateral faces (see the
    // note on w.faces) its sign is the right convention for both an upright and an upside-down
    // figure. Choosing the base instead for an upside-down figure gives the same answer, so there
    // is nothing to gain from a special case.
    w.visibleFace = P_FRONT;
    return w;
}

auto SolidGeometry::signedArea(const std::vector<Point>& vertices, const std::vector<std::size_t>& face) -> double {
    double twiceArea = 0.0;
    const std::size_t n = face.size();
    for (std::size_t i = 0; i < n; ++i) {
        const Point& p = vertices[face[i]];
        const Point& q = vertices[face[(i + 1) % n]];
        twiceArea += p.x * q.y - q.x * p.y;
    }
    return 0.5 * twiceArea;
}

auto SolidGeometry::hiddenEdges(const Wireframe& wireframe) -> std::vector<std::size_t> {
    // A face is visible when the sign of its area agrees with that of a face we know to be visible.
    // This is robust to the input's vertex order: the reference face supplies the convention.
    const double reference = signedArea(wireframe.vertices, wireframe.faces[wireframe.visibleFace]);

    auto isHiddenFace = [&](std::size_t face) {
        for (std::size_t h: wireframe.alwaysHidden) {
            if (h == face) {
                return true;
            }
        }
        // A face seen exactly edge-on projects to a segment: its area is zero and its sign carries
        // no information, so it is never reported as hidden here.
        return signedArea(wireframe.vertices, wireframe.faces[face]) * reference < 0.0;
    };

    std::vector<std::size_t> result;
    for (std::size_t i = 0; i < wireframe.edges.size(); ++i) {
        if (isHiddenFace(wireframe.edges[i].faceA) && isHiddenFace(wireframe.edges[i].faceB)) {
            result.push_back(i);
        }
    }
    return result;
}

auto SolidGeometry::hiddenEdgePolylines(const Wireframe& wireframe) -> std::vector<std::vector<Point>> {
    std::vector<std::vector<Point>> polylines;
    for (std::size_t index: hiddenEdges(wireframe)) {
        const Edge& e = wireframe.edges[index];
        polylines.push_back({wireframe.vertices[e.from], wireframe.vertices[e.to]});
    }
    return polylines;
}

namespace {
/**
 * Two points closer than this are the same corner. The shape tools place corners exactly, but a figure
 * copied around or drawn on top of another can put two of them a fraction of a point apart.
 */
constexpr double SAME_CORNER = 1.0;

auto samePoint(const Point& a, const Point& b) -> bool {
    return std::abs(a.x - b.x) <= SAME_CORNER && std::abs(a.y - b.y) <= SAME_CORNER;
}

/// The corners, in the order they first appear in the given points.
auto collectCorners(const std::vector<Point>& points) -> std::vector<Point> {
    std::vector<Point> corners;
    for (const Point& p: points) {
        bool known = false;
        for (const Point& c: corners) {
            if (samePoint(p, c)) {
                known = true;
                break;
            }
        }
        if (!known) {
            corners.push_back(p);
        }
    }
    return corners;
}

auto contains(const std::vector<Point>& corners, const Point& p) -> bool {
    for (const Point& c: corners) {
        if (samePoint(c, p)) {
            return true;
        }
    }
    return false;
}
}  // namespace

auto SolidGeometry::vertexLabels(const std::vector<std::vector<Point>>& strokes) -> std::vector<VertexLabel> {
    // The outline: the shape tools draw the whole figure as one long stroke and the hidden edges as short
    // two-point strokes, so the longest stroke is the one that walks the figure.
    const std::vector<Point>* outline = nullptr;
    for (const std::vector<Point>& s: strokes) {
        if (s.size() > (outline == nullptr ? 0 : outline->size())) {
            outline = &s;
        }
    }
    if (outline == nullptr || outline->empty()) {
        return {};
    }

    std::vector<Point> allPoints;
    for (const std::vector<Point>& s: strokes) {
        allPoints.insert(allPoints.end(), s.begin(), s.end());
    }
    // Corners are ordered by the OUTLINE first, so that A, B, C, D follow the way the figure was drawn
    // even when a hidden-edge stroke happens to be listed before it.
    std::vector<Point> corners = collectCorners(*outline);
    for (const Point& p: collectCorners(allPoints)) {
        if (!contains(corners, p)) {
            corners.push_back(p);
        }
    }

    std::vector<VertexLabel> labels;

    if (corners.size() == 3 || corners.size() == 4) {
        // Triangle / quadrilateral: name the corners in the order they were drawn.
        for (std::size_t i = 0; i < corners.size(); ++i) {
            labels.push_back({corners[i], std::string(1, static_cast<char>('A' + static_cast<int>(i)))});
        }
        return labels;
    }

    if (corners.size() == 5) {
        // Quadrilateral pyramid: the base is walked first and the walk finishes at the apex, so the last
        // point OF THE OUTLINE is the apex and the other four corners are the base.
        //
        // Taking the last point of ALL the strokes instead is wrong: a hidden-edge stroke listed after the
        // outline would hand back one of ITS endpoints, and the apex would then be labelled as a base
        // corner while a base corner got the apex's name. That mistake was made and caught in the app.
        const Point apex = outline->back();
        for (const Point& c: corners) {
            if (!samePoint(c, apex)) {
                labels.push_back({c, std::string(1, static_cast<char>('A' + static_cast<int>(labels.size())))});
            }
        }
        if (labels.size() != 4) {
            return {};  // not four base corners plus one apex
        }
        labels.push_back({apex, "S"});
        return labels;
    }

    if (corners.size() == 8) {
        // Box: find the depth vector as the shift that maps exactly four corners onto four others. Both v
        // and -v do that, so the direction is fixed by the walk's starting corner, which is a front corner.
        const Point& start = outline->front();
        std::optional<Point> depth;
        for (const Point& from: corners) {
            for (const Point& to: corners) {
                if (samePoint(from, to)) {
                    continue;
                }
                const Point v{to.x - from.x, to.y - from.y};
                std::size_t pairs = 0;
                for (const Point& p: corners) {
                    if (contains(corners, Point{p.x + v.x, p.y + v.y})) {
                        ++pairs;
                    }
                }
                if (pairs == corners.size() / 2 && contains(corners, Point{start.x + v.x, start.y + v.y})) {
                    depth = v;
                    break;
                }
            }
            if (depth.has_value()) {
                break;
            }
        }
        if (!depth.has_value()) {
            return {};
        }

        std::size_t letter = 0;
        for (const Point& front: corners) {
            const Point back{front.x + depth->x, front.y + depth->y};
            if (!contains(corners, back)) {
                continue;  // this corner belongs to the back face
            }
            const std::string name(1, static_cast<char>('A' + static_cast<int>(letter)));
            labels.push_back({front, name});
            labels.push_back({back, name + "'"});
            ++letter;
        }
        if (labels.size() != corners.size()) {
            return {};  // the corners do not split into four front / four back pairs
        }
        return labels;
    }

    return {};  // not a figure this knows: say nothing rather than something wrong
}

auto SolidGeometry::boxOutline(const Wireframe& wireframe) -> std::vector<Point> {
    if (wireframe.vertices.size() != 8) {
        return {};
    }
    const auto& v = wireframe.vertices;
    // A single closed walk that covers all twelve edges without ever crossing a face: around the
    // back face, then across the four joining edges (each twice, i.e. simply overdrawn and
    // invisible), then around the front face. See the header for why a single stroke cannot cover
    // a box's edges exactly once.
    const std::vector<std::size_t> walk = {
            BA,   BA2,  BB2, BCC2, BDD2,  // back face
            BA2,  BA,                     // top-left joining edge, out and back
            BB,   BB2, BB,                // top-right joining edge
            BCC,  BCC2, BCC,              // bottom-right joining edge
            BDD,  BDD2, BDD,              // bottom-left joining edge
            BA,                           // close the front face
    };
    std::vector<Point> outline;
    outline.reserve(walk.size());
    for (std::size_t i: walk) {
        outline.push_back(v[i]);
    }
    return outline;
}

auto SolidGeometry::pyramidOutline(const Wireframe& wireframe) -> std::vector<Point> {
    if (wireframe.vertices.size() != 5) {
        return {};
    }
    const auto& v = wireframe.vertices;
    return {
            v[P1], v[P2], v[P3], v[P4], v[P1],  // base (closed)
            v[P4], v[APEX],                     // lateral edge to the back-left corner
            v[P3], v[APEX],                     // lateral edge to the back-right corner
            v[P2], v[APEX],                     // lateral edge to the front-right corner
            v[P1], v[APEX],                     // lateral edge to the front-left corner
    };
}
