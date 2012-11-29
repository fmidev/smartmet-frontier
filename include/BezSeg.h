// ======================================================================
/*!
 * \brief frontier::BezSeg
 *
 *		  Converted from Esa's java code
 */
// ======================================================================

#ifndef FRONTIER_BEZSEG_H
#define FRONTIER_BEZSEG_H

#define ArrayList List
#define List std::vector

#include <vector>

namespace frontier
{

  typedef bool boolean;

  class doubleArr {

  public:
	doubleArr(double d1 = 0.0, double d2 = 0.0) { itsArr[0] = d1; itsArr[1] = d2; }
	doubleArr & operator = (const doubleArr & d) { itsArr[0] = d.itsArr[0]; itsArr[1] = d.itsArr[1]; return *this; }
	double & operator [] (const int i) { return itsArr[i ? i : 0]; }

  private:
	double itsArr[2];
  };

  class DirectPosition;
  typedef DirectPosition GeneralDirectPosition;

  class DirectPosition {

  public:
	DirectPosition(double x = 0.0, double y = 0.0) : itsArr(x,y) { }
	DirectPosition(const doubleArr & da) : itsArr(da) { }
	doubleArr & getCoordinates() { return itsArr; }
	double & getX() { return itsArr[0]; }
	double & getY() { return itsArr[1]; }

  private:
	doubleArr itsArr;
  };

  class BezSeg {

  public:
	BezSeg(List<DirectPosition> curvePositions, int segmentIndex, double tightness = -1.0);

	int getSegmentIndex();
	double getSegmentLength();
	DirectPosition getControlPoint1();
	DirectPosition getControlPoint2();
	DirectPosition getStartPoint();
	DirectPosition getEndPoint();
	doubleArr getPosition(double t);
	doubleArr getTangent(double t);
	double getSegmentLengthInPixels();

  private:
	double dt; 	// Bezier segment evalation step [0..1]
	int segmentIndex;
	double tightness;
	double segmentLength;
	List<DirectPosition> curvePositions;

	doubleArr start;
	doubleArr end;
	doubleArr c1;
	doubleArr c2;

	void computeControlPointPositions();
	doubleArr createControlPoint(doubleArr & a, doubleArr & b, doubleArr & c, int direction);
	void computeSegmentLengthInPixels();
  };

} // namespace frontier

#endif // FRONTIER_BEZSEG_H
