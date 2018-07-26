// ======================================================================
/*!
 * \brief frontier::PathTransformation
 */
// ======================================================================

#ifndef FRONTIER_PATHTRANSFORMATION_H
#define FRONTIER_PATHTRANSFORMATION_H

class NFmiAngle;

namespace frontier
{
class PathTransformation
{
 public:
  virtual ~PathTransformation() {}
  virtual void operator()(double& x, double& y, NFmiAngle* trueNorthAzimuth = nullptr) const = 0;

};  // class PathTransformation
}  // namespace frontier

#endif  // FRONTIER_PATHTRANSFORMATION_H
