// ======================================================================
/*!
 * \brief frontier::PathFactory
 */
// ======================================================================

#include <iostream>

#include "Path.h"
#include "PathFactory.h"
#include "Point.h"
#include <smartmet/woml/SimpleCubicSpline.h>
#include <smartmet/woml/CubicSplineRing.h>
#include <smartmet/woml/CubicSplineCurve.h>
#include <smartmet/woml/CubicSplineSurface.h>
#include <boost/foreach.hpp>

namespace frontier { namespace PathFactory {

  // ----------------------------------------------------------------------
  /*!
   * \brief Convert a SimpleCubicSpline into a path
   */
  // ----------------------------------------------------------------------

  Path create(const woml::SimpleCubicSpline & spline)
  {
	Path path;

	// Handle special cases of 0, 1 or 2 points

	if(spline.empty())
	  return path;

	path.moveto(spline[0].lon(), spline[0].lat());

	if(spline.size() == 1)
	  return path;

	if(spline.size() == 2)
	  {
		path.lineto(spline[1].lon(), spline[1].lat());
		return path;
	  }

	// Ref: http://www.codeproject.com/KB/graphics/BezierSpline.aspx

	// Convert woml points to math points
	
	std::vector<Point> knots(spline.size(), Point(0,0));
	for(size_t i=0; i<spline.size(); ++i)
	  knots[i] = Point(spline[i].lon(), spline[i].lat());

	// Build rhs of equations

	size_t n = knots.size() - 1;
	std::vector<Point> rhs(n, Point(0,0));
	for(size_t i=1; i<n-1; ++i)
	  rhs[i] = 4*knots[i] + 2*knots[i+1];
	rhs[0]   = knots[0] + 2*knots[1];
	rhs[n-1] = (8*knots[n-1]+knots[n])/2;

	// Solve tridiagonal system of equations

	std::vector<Point> ctrl(n,Point(0,0));
	std::vector<Point> tmp(n,Point(0,0));

	Point b(2,2);
	ctrl[0] = rhs[0]/b;
	for(size_t i=1; i<ctrl.size(); ++i)
	  {
		tmp[i] = 1/b;
		b = (i<ctrl.size()-1 ? 4.0 : 3.5) - tmp[i];
		ctrl[i] = (rhs[i] - ctrl[i - 1]) / b;
	  }

	for(size_t i=1; i<ctrl.size(); i++)
	  ctrl[ctrl.size()-i-1] -= tmp[ctrl.size()-i] * ctrl[ctrl.size()-i]; // Backsubstitution.

	double x, y, x1, y1, x2, y2;
	for(size_t i=0; i<n; ++i)
	  {
		x1 = ctrl[i].x;
		y1 = ctrl[i].y;

		if (i<n-1)
		  {
			x2 = 2*knots[i+1].x - ctrl[i+1].x;
			y2 = 2*knots[i+1].y - ctrl[i+1].y;
		  }
		else
		  {
			x2 = (knots[i].x + ctrl[i-1].x)/2;
			y2 = (knots[i].y + ctrl[i-1].y)/2;
		  }

		x = knots[i+1].x;
		y = knots[i+1].y;

		path.curveto(x1,y1,x2,y2,x,y);
	  }

	return path;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Convert a CubicSplineRing into a path
   */
  // ----------------------------------------------------------------------

  Path create(const woml::CubicSplineRing & spline)
  {
	Path path;

	// Handle special cases of 0, 1 or 2 points

	if(spline.empty())
	  return path;

	path.moveto(spline[0].lon(), spline[0].lat());

	if(spline.size() == 1)
	  return path;

	if(spline.size() == 2)
	  {
		path.lineto(spline[1].lon(), spline[1].lat());
		path.closepath();
		return path;
	  }

	// The rest is modeled after SimpleCubicSpline conversion,
	// but the equations found from
	//
	//   http://www.codeproject.com/KB/graphics/BezierSpline.aspx
	//
	// were modified to reflect periodic boundary conditions. 

	// Convert woml points to math points
	
	int extras = 2;

	std::vector<Point> knots(spline.size()+2*extras, Point(0,0));
	for(size_t i=0; i<spline.size(); ++i)
	  knots[i+extras] = Point(spline[i].lon(), spline[i].lat());

	// Make it dummy-periodic

	knots[1] = knots[knots.size()-extras-1];
	knots[0] = knots[knots.size()-extras-2];
	knots[knots.size()-1] = knots[extras+1];
	knots[knots.size()-2] = knots[extras+0];

	// Build rhs of equations

	size_t n = knots.size() - 1;
	std::vector<Point> rhs(n, Point(0,0));
	for(size_t i=1; i<n-1; ++i)
	  rhs[i] = 4*knots[i] + 2*knots[i+1];

	// Natural spline has second derivatives set to zero
	rhs[0]   = knots[0] + 2*knots[1];
	rhs[n-1] = (8*knots[n-1]+knots[n])/2;

	// Solve tridiagonal system of equations

	std::vector<Point> ctrl(n,Point(0,0));
	std::vector<Point> tmp(n,Point(0,0));

	Point b(2,2);
	ctrl[0] = rhs[0]/b;
	for(size_t i=1; i<ctrl.size(); ++i)
	  {
		tmp[i] = 1/b;
		b = (i<ctrl.size()-1 ? 4.0 : 3.5) - tmp[i];
		ctrl[i] = (rhs[i] - ctrl[i - 1]) / b;
	  }

	for(size_t i=1; i<ctrl.size(); i++)
	  ctrl[ctrl.size()-i-1] -= tmp[ctrl.size()-i] * ctrl[ctrl.size()-i]; // Backsubstitution.

	double x, y, x1, y1, x2, y2;
	for(size_t i=extras; i<n-extras+1; ++i)
	  {
		x1 = ctrl[i].x;
		y1 = ctrl[i].y;

		if (i<n-1+extras)
		  {
			x2 = 2*knots[i+1].x - ctrl[i+1].x;
			y2 = 2*knots[i+1].y - ctrl[i+1].y;
		  }
		else
		  {
			x2 = (knots[i].x + ctrl[i-1].x)/2;
			y2 = (knots[i].y + ctrl[i-1].y)/2;
		  }

		x = knots[i+1].x;
		y = knots[i+1].y;

		path.curveto(x1,y1,x2,y2,x,y);
	  }

	path.closepath();
	return path;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Convert a CubicSplineCurve into a path
   */
  // ----------------------------------------------------------------------

  Path create(const woml::CubicSplineCurve & spline)
  {
	Path path;

	BOOST_FOREACH(const woml::SimpleCubicSpline & simplespline, spline)
	  path += create(simplespline);

	return path;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Convert a CubicSplineSurface into a path
   */
  // ----------------------------------------------------------------------

  Path create(const woml::CubicSplineSurface & spline)
  {
	Path path;

	// Exterior part
	path += create(spline.exterior());

	// Interior part
	BOOST_FOREACH(const woml::CubicSplineRing & ring, spline)
	  path += create(ring);

	return path;
  }

} } // namespace frontier::PathFactory