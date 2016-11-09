// ======================================================================
/*!
 * \brief frontier::PathFactory
 */
// ======================================================================

#ifndef FRONTIER_PATHFACTORY_H
#define FRONTIER_PATHFACTORY_H

namespace woml
{
class SimpleCubicSpline;
class CubicSplineRing;
class CubicSplineCurve;
class CubicSplineSurface;
}

namespace frontier
{
class Path;

namespace PathFactory
{
Path create(const woml::SimpleCubicSpline& spline);
Path create(const woml::CubicSplineRing& spline);
Path create(const woml::CubicSplineCurve& spline);
Path create(const woml::CubicSplineSurface& spline);

}  // namespace PathFactory
}  // namespace frontier

#endif  // FRONTIER_PATHFACTORY_H
