// ======================================================================
/*!
 * \brief frontier::BezierModel
 * 
 *		  Converted from Esa's java code
 */
// ======================================================================

#ifndef FRONTIER_BEZIERMODEL_H
#define FRONTIER_BEZIERMODEL_H

#include <list>
#include "BezSeg.h"

namespace frontier
{

  class BezierModel {

	// Ref [1].
	//
	// Bezier control point calculation modified from:
	//
	// http://stackoverflow.com/questions/6797614/drawing-a-curve-through-a-set-of-n-points-where-n2
	//
	// - AND -
	//
	// http://www.pixelnerve.com/v/2010/05/11/evaluate-a-cubic-bezier-on-gpu/ (Also found in nvidia sdk)

  public:
	BezierModel(const List<DirectPosition> & theCurvePositions, boolean isClosedCurve, double theTightness = -1.0);

	void init(const List<DirectPosition> & theCurvePositions, boolean isClosedCurve);

	void addCurvePosition(DirectPosition curvePosition, boolean isClosedCurve);
	double getCumulatedCurveLength(int i);
	const List<DirectPosition> & getCurvePositions();
	const std::list<BezSeg> & getBezierSegments();
	int getBezierSegmentCount();
	DirectPosition getStartPointOfLastBezierSegment();
	DirectPosition getEndPointOfLastBezierSegment();
	DirectPosition getStartPointOfFirstBezierSegment();
	DirectPosition getEndPointOfFirstBezierSegment();
	DirectPosition getFirstControlPointOfLastBezierSegment();
	DirectPosition getSecondControlPointOfFirstBezierSegment();
	DirectPosition getFirstControlPointOfFirstBezierSegment();
	DirectPosition getSecondControlPointOfLastBezierSegment();
	double getTotalLengthOfAllSegments();
	void setOrientation(Orientation theOrientation);
	Orientation getOrientation();
//	BezierCurve getBezierCurve();
//	Shape getCurveLinePath();
//	Rectangle getBoundingCurveLineRect();
//	Shape getDecorationLinePath();
//	void setCurveDecorator(BezierCurve curveDecorator);
	BezSeg getLastBezierSegment();
	boolean isEmpty();
//	List<Integer> getEvaluatedCurvePositionSegmentIndices();
//	DirectPosition getEvaluatedCurvePosition(double cumulatedPathLength);
//	List<DirectPosition> getEvaluatedCurvePositions(double pathLengthIncrement);
	boolean isClosedCurve();
	void setTightness(double theTightness);
	double getTightness();

	int getSteppedCurvePoints(unsigned int baseStep,
							  unsigned int maxRand,
							  unsigned int maxRepeat,
							  std::list<DirectPosition> & curvePoints);
	void decorateCurve(std::list<DirectPosition> & curvePoints,
					   bool negative,
					   int scaleHeightMin,
					   int scaleHeightRandom,
					   int controlMin,
					   int controlRandom,
					   std::list<DoubleArr> & decoratorPoints);

  private:
	List<DirectPosition> curvePositions;
	std::list<BezSeg> bezierSegments;
	Orientation orientation;
	double totalCurveLength;
	std::list<double> cumulatedCurveLength;
	boolean bIsClosedCurve;
	double tightness;

  };

} // namespace frontier

#endif // FRONTIER_BEZIERMODEL_H
