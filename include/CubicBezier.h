// ======================================================================
/*!
 * \brief frontier::CubicBezier
 */
// ======================================================================

#ifndef FRONTIER_CUBICBEZIER_H
#define FRONTIER_CUBICBEZIER_H

#include "NFmiFillMap.h"
#include "Point.h"
#include "clipper.hpp"
#include <utility>

namespace frontier
{
class CubicBezier
{
 public:
  CubicBezier(const Point& p1, const Point& p2, const Point& p3, const Point& p4);
  CubicBezier(
      double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4);

  double length(double eps = 0.001,
                NFmiFillMap* fmap = NULL,
                double* lastx = NULL,
                double* lasty = NULL,
                ClipperLib::Path* path = NULL) const;
  std::pair<CubicBezier, CubicBezier> split() const;

 private:
  Point P1;
  Point P2;
  Point P3;
  Point P4;

};  // class CubicBezier
}  // namespace frontier

#endif  // FRONTIER_CUBICBEZIER_H
