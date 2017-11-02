// ======================================================================
/*!
 * \brief frontier::CubicBezier
 */
// ======================================================================

#include "CubicBezier.h"
#include "clipper.hpp"

namespace frontier
{
// ----------------------------------------------------------------------
/*!
 * \brief Construct from points
 */
// ----------------------------------------------------------------------

CubicBezier::CubicBezier(const Point& p1, const Point& p2, const Point& p3, const Point& p4)
    : P1(p1), P2(p2), P3(p3), P4(p4)
{
}

// ----------------------------------------------------------------------
/*!
 * \brief Construct from coordinates
 */
// ----------------------------------------------------------------------

CubicBezier::CubicBezier(
    double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4)
    : P1(Point(x1, y1)), P2(Point(x2, y2)), P3(Point(x3, y3)), P4(Point(x4, y4))
{
}

// ----------------------------------------------------------------------
/*!
 * \brief Split the segment in half
 */
// ----------------------------------------------------------------------

std::pair<CubicBezier, CubicBezier> CubicBezier::split() const
{
  Point b1 = (P1 + P2) / 2;
  Point b2 = (P2 + P3) / 2;
  Point b3 = (P3 + P4) / 2;

  Point c1 = (b1 + b2) / 2;
  Point c2 = (b2 + b3) / 2;

  Point d1 = (c1 + c2) / 2;

  CubicBezier part1(P1, b1, c1, d1);
  CubicBezier part2(d1, c2, b3, P4);

  return std::make_pair(part1, part2);
}

// ----------------------------------------------------------------------
/*!
 * \brief Calculate the length of the Bezier
 */
// ----------------------------------------------------------------------

double CubicBezier::length(
    double eps, NFmiFillMap* fmap, double* lastx, double* lasty, ClipperLib::Path* path) const
{
  // Length along control points and the end points
  double chord = distance(P1, P4);
  double arc = distance(P1, P2) + distance(P2, P3) + distance(P3, P4);

  // Safety check which also guarantees arc>0 and termination
  // even if eps is way too small

  if (chord == 0) return 0;

  // Safety check which guarantees termination if chord is way too small

  if ((chord / arc) < eps) return chord;

  // If the relative difference is small, return good length estimate
  if ((arc - chord) / arc < eps)
  {
    if (fmap && lastx && lasty)
    {
      fmap->Add(*lastx, *lasty, P1.x, P1.y);
      fmap->Add(P1.x, P1.y, P2.x, P2.y);
      fmap->Add(P2.x, P2.y, P3.x, P3.y);
      fmap->Add(P3.x, P3.y, P4.x, P4.y);

      *lastx = P4.x;
      *lasty = P4.y;
    }

    if (path)
    {
      // Storing the bezier curve as segmentized lines/points. The segmentized path will be used
      // to scale up the surface to get enough fill positions.
      //
      *path << ClipperLib::IntPoint(P1.x, P1.y);
      *path << ClipperLib::IntPoint(P2.x, P2.y);
      *path << ClipperLib::IntPoint(P3.x, P3.y);
      *path << ClipperLib::IntPoint(P4.x, P4.y);
    }

    return 0.5 * (arc + chord);
  }

  // Otherwise subdivide and recurse

  std::pair<CubicBezier, CubicBezier> parts = split();

  double l1 = parts.first.length(eps, fmap, lastx, lasty, path);
  double l2 = parts.second.length(eps, fmap, lastx, lasty, path);

  return l1 + l2;
}

}  // namespace frontier
