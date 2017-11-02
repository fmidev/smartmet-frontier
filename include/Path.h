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

#include "NFmiFillMap.h"

#include <string>
#include <utility>
#include <vector>

namespace frontier
{
class PathTransformation;

class Path
{
 public:
  void closepath();
  void moveto(double x, double y);
  void lineto(double x, double y);
  void curveto(double x1, double y1, double x2, double y2, double x, double y);

  Path& operator+=(const Path& path);

  bool empty() const;
  double length(NFmiFillMap* fmap = NULL) const;
  std::string svg() const;
  void transform(const PathTransformation& transformation);
  bool scale(double offset, Path& scaledPath) const;

  std::pair<double, double> nearestVertex(double x, double y) const;

  typedef struct bbox
  {
    bbox(double _blX = 0.0, double _blY = 0.0, double _trX = 0.0, double _trY = 0.0)
        : blX(_blX), blY(_blY), trX(_trX), trY(_trY)
    {
    }
    double blX;
    double blY;
    double trX;
    double trY;
  } BBox;
  BBox getBBox() const;

 private:
  typedef std::vector<double> PathData;
  PathData pathdata;

  void clear() { pathdata.clear(); }
  void walk(size_t i, BBox& bbox, double x, double y) const;

};  // class Path

}  // namespace frontier

inline frontier::Path operator+(const frontier::Path& path1, const frontier::Path& path2)
{
  frontier::Path path(path1);
  return (path += path2);
}

#endif  // FRONTIER_PATH_H
