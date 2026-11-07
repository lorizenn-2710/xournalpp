/*
 * Xournal++
 *
 * Geometry of the solid figures the shape tools draw, and which of their edges are hidden
 *
 * @author Xournal++ Team
 * https://github.com/xournalpp/xournalpp
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <cstddef>   // for size_t
#include <optional>  // for optional
#include <string>    // for string
#include <vector>    // for vector

#include "model/Point.h"  // for Point

/**
 * @brief The wireframe of a solid figure, and which of its edges the viewer cannot see.
 *
 * WHY THIS EXISTS: a solid figure (a box, a pyramid) is a closed wireframe, but it is unreadable if
 * every edge is drawn the same way. Every textbook draws the edges the viewer cannot see as dashed
 * lines, and doing that by hand is exactly where the figure comes out wrong.
 *
 * HOW THE HIDDEN EDGES ARE FOUND: the figure is a closed convex polyhedron, so on screen its faces
 * are convex polygons and an edge is hidden exactly when BOTH faces meeting at it are hidden. A face
 * is hidden when it turns away from the viewer, which shows up in the sign of the signed area of
 * its projected outline: with the vertices of every face listed in the same rotational order, the
 * sign of the area says which way the face points, and a face pointing the other way from a face
 * known to be visible is a face the viewer cannot see.
 *
 * The obvious alternative — an edge is hidden if it runs inside the outline of the figure — is
 * WRONG here: in oblique projection a vertex can sit inside the outline and still be perfectly
 * visible, so that test invents hidden edges that do not exist. It was tried and abandoned.
 *
 * This file holds both the wireframes and the classification so the whole thing can be tested on
 * its own (tests/test-solid-geometry.cpp) instead of only through a running application.
 */
namespace SolidGeometry {

/**
 * @brief An edge of the wireframe, and the two faces it separates.
 */
struct Edge {
    std::size_t from;   ///< index into the vertex list
    std::size_t to;     ///< index into the vertex list
    std::size_t faceA;  ///< index of one of the two faces meeting at this edge
    std::size_t faceB;  ///< index of the other face
};

/**
 * @brief A solid figure, ready to be asked which edges are hidden.
 */
struct Wireframe {
    std::vector<Point> vertices;
    /// Each face, as its vertex indices in rotational order (the same order for every face)
    std::vector<std::vector<std::size_t>> faces;
    std::vector<Edge> edges;
    /// Index of a face that is visible whatever the figure's orientation (its front face)
    std::size_t visibleFace = 0;
    /**
     * Faces to treat as hidden whatever their projected area. A face seen exactly edge-on projects
     * to a straight line: its area is zero and its sign says nothing about which way it points, so
     * it is declared here instead.
     */
    std::vector<std::size_t> alwaysHidden;
};

/**
 * @brief A corner of a figure, and the name a textbook would give it.
 */
struct VertexLabel {
    Point position;
    std::string name;
};

/**
 * @brief Names the corners of a figure, given the strokes that drew it.
 *
 * WHY THIS EXISTS: a figure is only readable when its corners are named — a teacher writing on the board
 * labels A, B, C, D at the corners of a box or a pyramid, and putting those letters at the right corners by
 * hand (four or eight of them, moving them apart when they collide) is slow and comes out crooked. The
 * geometry already knows exactly where the corners are, because the shape tools place them.
 *
 * HOW IT RECOGNISES THE FIGURE: only the number and the order of the corners are used, never a guess about
 * the drawing gesture:
 *   - 3 corners  -> triangle:            A, B, C
 *   - 4 corners  -> quadrilateral A,B,C,D, or triangular pyramid A,B,C,S when its edge topology matches
 *   - 6 corners  -> right triangular prism: lower A,B,C and vertically paired upper A',B',C'
 *   - 5 corners  -> quadrilateral pyramid: base A, B, C, D and the apex S (the walk ends at the apex)
 *   - 8 corners  -> box: the four corners of the FRONT face are A, B, C, D and their twins behind are
 *                   A', B', C', D'. The depth vector is found as the vector that maps exactly four
 *                   corners onto four other corners, and its direction is fixed by the fact that the
 *                   outline starts on a front corner (`boxOutline()` walks from a front corner to its
 *                   twin, around the back face, and back to the front face).
 * Anything else returns an empty list rather than a wrong naming.
 *
 * @param strokes the strokes of the figure, each as its points in drawing order. The stroke with the most
 *        points is taken as the outline; the shape tools draw the figure as one such stroke and the hidden
 *        edges as short two-point strokes.
 * @return one label per corner (empty when the figure is not one this knows)
 */
auto vertexLabels(const std::vector<std::vector<Point>>& strokes) -> std::vector<VertexLabel>;

/**
 * @brief The wireframe of a rectangular box whose front face has f1 and f2 as opposite corners.
 *
 * The front face is drawn exactly like the rectangle tool; a second parallelogram is placed behind
 * it, offset by the depth vector, and the two are joined by four edges.
 *
 * @param depthFraction depth as a fraction of the size of the front face
 * @param dx, dy direction of the depth, as a fraction of its length
 */
auto boxWireframe(const Point& f1, const Point& f2, double depthFraction, double dx, double dy) -> Wireframe;

/**
 * @brief The wireframe of a quadrilateral pyramid standing on a parallelogram base.
 *
 * The base is a parallelogram: its front edge is horizontal and its two side edges run along the
 * oblique depth, so the far edge sits behind and slightly above the front edge. That is what makes
 * the base read as a quadrilateral seen in perspective; drawing the far edge level with the front
 * one would make it a trapezium.
 *
 * @param baseCentre centre of the base
 * @param baseWidth width of the base (its front edge)
 * @param apexHeight how far the apex stands above the centre of the base. Negative means the apex
 *        hangs BELOW the base, i.e. the figure is drawn upside down, and the hidden faces are then
 *        the ones on the other side of the figure.
 * @param depthFraction depth of the base as a fraction of its width
 * @param dx, dy direction of the base depth, as a fraction of its length
 */
auto pyramidWireframe(const Point& baseCentre, double baseWidth, double apexHeight, double depthFraction, double dx,
                      double dy) -> Wireframe;

/// S directly above A, with the same quadrilateral base and face topology.
auto rightPyramidWireframe(const Point& baseCentre, double baseWidth, double apexHeight, double depthFraction,
                           double dx, double dy) -> Wireframe;

/// Vertices A (back-left), B (front), C (back-right), S (apex).
auto triangularPyramidWireframe(const Point& baseCentre, double baseWidth, double apexHeight) -> Wireframe;

/// Vertices A, B, C on the lower base, then their vertically translated A', B', C'.
auto triangularPrismWireframe(const Point& first, const Point& second) -> Wireframe;

/// Walk actual edges only; retracing edges joins branches without spurious diagonals.
auto triangularOutline(const Wireframe& wireframe, bool excludeHidden = false) -> std::vector<Point>;

/**
 * @brief The signed area of a polygon.
 *
 * Only the sign is meaningful here, and only ever compared between faces of the same figure.
 */
double signedArea(const std::vector<Point>& vertices, const std::vector<std::size_t>& face);

/**
 * @brief Which of `edges` are hidden, i.e. have both of their faces hidden.
 *
 * @return the indices into `wireframe.edges` that must be drawn dashed
 */
std::vector<std::size_t> hiddenEdges(const Wireframe& wireframe);

/**
 * @brief The hidden edges as ready-to-draw polylines (one per edge).
 *
 * This is what a shape tool hands to the view and to the undo stack, so the preview under the pen
 * and the committed figure use one single computation.
 */
std::vector<std::vector<Point>> hiddenEdgePolylines(const Wireframe& wireframe);

/**
 * @brief The outline of a box, ready to be drawn as one path.
 *
 * A box CANNOT be drawn as a single stroke without drawing across a face: every one of its eight
 * corners has three edges, so the wireframe is not an Eulerian graph and no walk covers all twelve
 * edges exactly once. Concatenating the obvious polylines is worse still — it adds a diagonal at
 * every junction where one polyline ends somewhere other than where the next one starts.
 *
 * The walk below is the one a person draws by hand, and it draws every edge with no diagonal:
 * around the back face, across the joining edges (twice, so they are simply overdrawn and
 * invisible), and around the front face. It returns to its starting point, so the path stays closed
 * and a filled pen still fills the figure correctly.
 *
 * @param wireframe a box wireframe (see boxWireframe)
 */
std::vector<Point> boxOutline(const Wireframe& wireframe, bool excludeHidden = false);

/**
 * @brief The outline of a pyramid: the base (closed) followed by the four edges from the apex down
 * to each base corner. No edge of a pyramid is drawn twice and no diagonal is needed.
 * When `excludeHidden` is true, only visible edges are walked so hidden edges stay dashed.
 */
std::vector<Point> pyramidOutline(const Wireframe& wireframe, bool excludeHidden = false);

}  // namespace SolidGeometry
