// ======================================================================
/*!
 * \brief frontier::Path
 *
 * A path represents an arbitrary curve which may be represented
 * as an SVG object. A path contains only a subset of possible
 * SVG path elements. In particular, quadratic curveto and elliptic
 * arcs are omitted, as are all relative path segments. Note that
 * quadratic segments are convertible to cubic segments (Wikipedia).
 *
 * The path object is not designed for editing, only for storing
 * a path, calculating its length and producing the respective
 * SVG path cdata.
 */
// ======================================================================

#ifndef FRONTIER_PATH_H
#define FRONTIER_PATH_H

#include <string>
#include <vector>

namespace frontier
{
  class Path
  {
  public:
	void closepath();
	void moveto(double x, double y);
	void lineto(double x, double y);
	void curveto(double x1, double y1, double x2, double y2, double x, double y);
	double length() const;
	std::string svg() const;

  private:
	typedef std::vector<double> PathData;
	PathData pathdata;

  }; // class Path

} // namespace frontier

#endif // FRONTIER_PATH_H
