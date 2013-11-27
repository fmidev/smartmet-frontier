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

  typedef enum { POS, NEG } Orientation;

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
	BezierModel(List<DirectPosition> curvePositions, boolean isClosedCurve, double tightness = -1.0);

	void init(List<DirectPosition> curvePositions, boolean isClosedCurve);

	const List<DirectPosition> & getCurvePositions();
	const std::list<BezSeg> & getBezierSegments();
	double getTotalLengthOfAllSegments();

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
					   std::list<doubleArr> & decoratorPoints);

  private:
	List<DirectPosition> curvePositions;
	std::list<BezSeg> bezierSegments;
	double totalCurveLengthInPixels;
	std::list<double> cumulatedCurveLengthInPixels;
	boolean isClosedCurve;
	double tightness;
  };

  class Vector2Dee {
  public:
	static doubleArr normalize(double x, double y);
	static doubleArr normalize(doubleArr vector);
	static doubleArr normalize(doubleArr vector1, doubleArr vector2);
	static double crossProd(doubleArr vector1, doubleArr vector2);
	static doubleArr sub(doubleArr vector1, doubleArr vector2);
	static doubleArr add(doubleArr vector1, doubleArr vector2);
	static doubleArr negate(doubleArr vector);
	static doubleArr scale(doubleArr vector, double scaler);
	static double length(doubleArr vector1, doubleArr vector2);
	static double length(doubleArr vector);
	static doubleArr getScaledNormal(doubleArr & leftPosition,
									 doubleArr & basePosition,
									 doubleArr & rightPosition,
									 Orientation orientation,
									 double scaler);
	static doubleArr getUnitNormal(doubleArr & leftPosition,
								   doubleArr & basePosition,
								   doubleArr & rightPosition,
								   Orientation orientation);
	static doubleArr getUnitNormal(doubleArr & vector1,
								   doubleArr & vector2,
								   Orientation orientation);

  private:
	static const int X;
	static const int Y;

  };

} // namespace frontier

#endif // FRONTIER_BEZIERMODEL_H
