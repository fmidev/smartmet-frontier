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
#include <stdexcept>

namespace frontier { namespace PathFactory {

  typedef std::vector<Point> Vector;

  // ----------------------------------------------------------------------
  /*!
   * \brief Utility subroutine for solving tridiagonal systems
   */
  // ----------------------------------------------------------------------

  Vector trisolve(const Vector & a,
				  const Vector & b,
				  const Vector & c,
				  const Vector & rhs)
  {
	if(b[0].x==0 || b[0].y==0)
	  throw std::runtime_error("Singular matrix encountered in PathFactory when converting spline representations");

	// If this happens then you should rewrite your equations as a set of 
	// order N - 1, with u2 trivially eliminated.

	size_t n = rhs.size();
	Vector u(n,Point(0,0));
	Vector gam(n,Point(0,0));
		
	Point bet = b[0];
	u[0] = rhs[0]/bet;

	// Decomposition and forward substitution.
	for(size_t j=1; j<n; j++)
	  {
		gam[j] = c[j-1]/bet;
		bet = b[j]-a[j]*gam[j];
		if (bet.x==0 || bet.y==0)
		  throw std::runtime_error("Singular matrix encountered in PathFactory when converting spline representations");
		u[j] = (rhs[j]-a[j]*u[j-1])/bet;
	  }

	for(size_t j=1; j<n; j++) 
	  u[n-j-1] -= gam[n-j]*u[n-j]; // Backsubstitution.

	return u;
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

	// The rest is based on
	//
	// http://www.codeproject.com/KB/graphics/ClosedBezierSpline.aspx

	// Convert woml points to math points
	
	Vector knots(spline.size(), Point(0,0));
	for(size_t i=0; i<spline.size(); ++i)
	  knots[i] = Point(spline[i].lon(), spline[i].lat());

	size_t n = knots.size();

	// The tridiagonal matrix
	Vector a(n,Point(1,1));
	Vector b(n,Point(4,4));
	Vector c(n,Point(1,1));

	// Right hand side vector

	Vector rhs(n,Point(0,0));
	for(size_t i=0; i<n; ++i)
	  {
		size_t j = (i==n-1) ? 0 : i+1;
		rhs[i] = 4*knots[i]+2*knots[j];
	  }

	// Solve the system

	Point alpha(1,1);
	Point beta(1,1);
	Point gamma = -b[0]; // Avoid subtraction error in forming bb[0].

	// Set up the diagonal of the modified tridiagonal system.
	
	Vector bb = b;
	bb[0] = b[0]-gamma;
	bb[n-1] = b[n-1]-alpha*beta/gamma;

	// Solve A · x = rhs.

	Vector x = trisolve(a,bb,c,rhs);

	// Set up the vector u.

	Vector u(n,Point(0,0));
	u[0] = gamma;
	u[n-1] = alpha;

	// Solve A · z = u.

	Vector z = trisolve(a,bb,c,u);

	// Form v · x/(1 + v · z).

	Point fact = (x[0]+beta*x[n-1]/gamma) /
	             (1+z[0]+beta*z[n-1]/gamma);

	// Now get the solution vector x.

	for(size_t i=0; i<n; ++i) 
	  x[i] -= fact*z[i];

	// Build the path

	for(size_t i=0; i<n; ++i)
	  {
		// Note: According to the original article we should use
		//       i instead of j for the second control point.
		//       I checked all the code, and only this change
		//       produced the correct result. - Mika Heiskanen

		size_t j = (i==n-1) ? 0 : i+1;
#if 1
		// Working version
		path.curveto(x[i].x, x[i].y,
					 2*knots[j].x - x[j].x, 2*knots[j].y - x[j].y,
					 knots[j].x, knots[j].y);
#else
		// Net version
		path.curveto(x[i].x, x[i].y,
					 2*knots[i].x - x[i].x, 2*knots[i].y - x[i].y,
					 knots[j].x, knots[j].y);
#endif
	  }

	path.closepath();
	return path;
  }

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
	
	Vector knots(spline.size(), Point(0,0));
	for(size_t i=0; i<spline.size(); ++i)
	  knots[i] = Point(spline[i].lon(), spline[i].lat());

	// Build rhs of equations

	size_t n = knots.size() - 1;
	Vector rhs(n, Point(0,0));
	for(size_t i=1; i<n-1; ++i)
	  rhs[i] = 4*knots[i] + 2*knots[i+1];
	rhs[0]   = knots[0] + 2*knots[1];
	rhs[n-1] = (8*knots[n-1]+knots[n])/2;

	// Solve tridiagonal system of equations

	Vector ctrl(n,Point(0,0));
	Vector tmp(n,Point(0,0));

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
