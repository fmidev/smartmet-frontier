// ======================================================================
/*!
 * \brief frontier::BezSeg
 *
 *		  Converted from Esa's java code
 */
// ======================================================================

#ifndef FRONTIER_BEZSEG_H
#define FRONTIER_BEZSEG_H

#define List std::vector

#include "DoubleArr.h"
#include <vector>

namespace frontier
{
  // import org.geotools.geometry.GeneralDirectPosition;
  // import org.opengis.geometry.DirectPosition;

  class DirectPosition {

  public:
	DirectPosition(double x = 0.0, double y = 0.0) : itsArr(x,y) { }
	DirectPosition(const DoubleArr & da) : itsArr(da) { }
	DoubleArr & getCoordinates() { return itsArr; }
	double & getX() { return itsArr[0]; }
	double & getY() { return itsArr[1]; }

  private:
	DoubleArr itsArr;
  };

  typedef DirectPosition GeneralDirectPosition;

  // Ref [1].
  //
  // Bézier control point calculation modified from following sources:
  //
  // http://stackoverflow.com/questions/6797614/drawing-a-curve-through-a-set-of-n-points-where-n2
  //
  // - AND -
  //
  // http://www.pixelnerve.com/v/2010/05/11/evaluate-a-cubic-bezier-on-gpu/ (Also found in nvidia sdk)

  class BezSeg {

  public:
	BezSeg(const List<DirectPosition> & theCurvePositions, int theSegmentIndex, boolean isClosedCurve, double theTightness = -1.0);

	void init(const List<DirectPosition> & theCurvePositions, int theSegmentIndex, boolean isClosedCurve);
	void setTightness(double theTightness);
	double getTightness();
	int getSegmentIndex();
	void setControlPoint1(DirectPosition c1);
	void setControlPoint2(DirectPosition c2);
	void setStartPoint(DirectPosition startPoint);
	void setEndPoint(DirectPosition endPoint);
	DirectPosition getFirstControlPoint();
	DirectPosition getSecondControlPoint();
	DirectPosition getNegatedControlPoint1();
	DirectPosition getNegatedSecondControlPoint();
	DirectPosition getStartPoint();
	DirectPosition getEndPoint();
	DoubleArr getPosition(double t);
	void setOrientation(Orientation theOrientation);
	Orientation getOrientation();
//	PositionAndUnitNormal getPositionAndNormal(double t);
//	PositionAndUnitNormal getNormalAtStartPoint();
//	PositionAndUnitNormal getNormalAtEndPoint();
	DoubleArr getTangent(double t);
	DoubleArr getRotatedUnitVector(DoubleArr p1, DoubleArr p2, double rotationAngleInDegrees);
	double getSegmentLength();
	void isSecondLastSegment(boolean yesOrNo);
	boolean isSecondLastSegment();
	void isLastSegment(boolean yesOrNo);
	boolean isLastSegment();
	boolean isMiddleSegment();

  private:
	static const int X = 0;
	static const int Y = 1;

	int segmentIndex;
	double segmentLength;
	List<DirectPosition> curvePositions;

	DoubleArr start;
	DoubleArr end;
	DoubleArr c1; // 1st Bézier curve control point
	DoubleArr c2; // 2nd Bézier curve control point
	Orientation orientation;
//	CoordinateReferenceSystem nativeCRS;

	boolean bIsClosedCurve;
	boolean bIsSecondLastSegment;
	boolean bIsLastSegment;

	static const double dt = 0.04; // Bézier segment evalation step [0..1]
	double tightness;

	boolean isChanged();
	void computeGeometry();
	void computeSegmentPointPositions();
	DoubleArr createControlPoint(DoubleArr & a, DoubleArr & b, DoubleArr & c, int direction);
	void computeSegmentLength();
  };

} // namespace frontier

#endif // FRONTIER_BEZSEG_H
