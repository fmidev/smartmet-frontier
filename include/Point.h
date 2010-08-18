// ======================================================================
/*!
 * \brief frontier::Point
 */
// ======================================================================

#ifndef FRONTIER_POINT_H
#define FRONTIER_POINT_H

#include <cmath>

namespace frontier
{
  class Point
  {
  public:
	Point(double theX, double theY) : x(theX), y(theY) { }
	double x;
	double y;

	Point & operator+=(const Point & thePoint);
	Point & operator-=(const Point & thePoint);

	Point & operator+=(double theValue);
	Point & operator-=(double theValue);
	Point & operator*=(double theFactor);
	Point & operator/=(double theFactor);

  private:
	Point();

  };

  inline Point & Point::operator+=(const Point & thePoint)
  {
	x += thePoint.x;
	y += thePoint.y;
	return *this;
  }

  inline Point & Point::operator-=(const Point & thePoint)
  {
	x -= thePoint.x;
	y -= thePoint.y;
	return *this;
  }
  
  inline Point & Point::operator+=(double theValue)
  {
	x += theValue;
	y += theValue;
	return *this;
  }

  inline Point & Point::operator-=(double theValue)
  {
	x -= theValue;
	y -= theValue;
	return *this;
  }

  inline Point & Point::operator*=(double theValue)
  {
	x *= theValue;
	y *= theValue;
	return *this;
  }

  inline Point & Point::operator/=(double theValue)
  {
	x /= theValue;
	y /= theValue;
	return *this;
  }

  inline Point operator-(const Point & p)
  { return Point(-p.x,-p.y); }

  inline Point operator+(const Point & p1, const Point & p2)
  { return Point(p1.x+p2.x, p1.y+p2.y); }

  inline Point operator+(const Point & p1, double v2)
  { return Point(p1.x+v2, p1.y+v2); }

  inline Point operator+(double v1, const Point & p2)
  { return Point(v1+p2.x,v1+p2.y); }

  inline Point operator-(const Point & p1, const Point & p2)
  { return Point(p1.x-p2.x, p1.y-p2.y); }

  inline Point operator-(const Point & p1, double v2)
  { return Point(p1.x-v2, p1.y-v2); }

  inline Point operator-(double v1, const Point & p2)
  { return Point(v1-p2.x,v1-p2.y); }

  inline Point operator*(const Point & p1, double v2)
  { return Point(p1.x*v2, p1.y*v2); }

  inline Point operator*(double v1, const Point & p2)
  { return Point(v1*p2.x,v1*p2.y); }

  inline Point operator/(const Point & p1, double v2)
  { return Point(p1.x/v2, p1.y/v2); }

  // Dot product
  inline double operator*(const Point & p1, const Point & p2)
  {
	return p1.x*p2.x + p1.y+p2.y;
  }

  inline double distance(const Point & p1, const Point & p2)
  {
	return std::sqrt((p1.x-p2.x)*(p1.x-p2.x)+(p1.y-p2.y)*(p1.y-p2.y));
  }

} // namespace frontier

#endif // FRONTIER_POINT_H
