// ======================================================================
/*!
 * \brief frontier::PathTransformation
 */
// ======================================================================

#ifndef FRONTIER_PATHTRANSFORMATION_H
#define FRONTIER_PATHTRANSFORMATION_H

namespace frontier
{
  class PathTransformation
  {
  public:
	virtual ~PathTransformation() { }
	virtual void operator()(double & x, double & y) const = 0;

  }; // class PathTransformation
} // namespace frontier

#endif // FRONTIER_PATHTRANSFORMATION_H
