#include "MagnetSnapView.h"

#include <cmath>  // for M_PI

#include "view/Repaintable.h"

using namespace xoj::view;

MagnetSnapView::MagnetSnapView(Repaintable* parent, Point anchor): OverlayView(parent), anchor(anchor) {}

MagnetSnapView::~MagnetSnapView() noexcept = default;

void MagnetSnapView::draw(cairo_t* cr) const {
    cairo_save(cr);

    // A thin ring: readable on white paper and on a PDF page, without hiding what is under it
    cairo_set_line_width(cr, 1.5);
    cairo_set_source_rgba(cr, 0.10, 0.55, 0.95, 0.95);
    cairo_arc(cr, this->anchor.x, this->anchor.y, RADIUS, 0.0, 2.0 * M_PI);
    cairo_stroke(cr);

    // A dot in the middle: shows the exact point the stroke will be attached to
    cairo_set_source_rgba(cr, 0.10, 0.55, 0.95, 0.95);
    cairo_arc(cr, this->anchor.x, this->anchor.y, 1.8, 0.0, 2.0 * M_PI);
    cairo_fill(cr);

    cairo_restore(cr);
}

bool MagnetSnapView::isViewOf(const OverlayBase* overlay) const {
    // The marker belongs to no element of the document: it is deleted by its owning input handler
    return false;
}
