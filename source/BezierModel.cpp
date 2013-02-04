// ======================================================================
/*!
 * \brief frontier::BezierModel
 *
 *		  Converted from Esa's java code
 */
 // ======================================================================

#include "BezierModel.h"
#include <boost/foreach.hpp>
#include <math.h>

namespace frontier
{

BezierModel::BezierModel(List<DirectPosition> _curvePositions, boolean _isClosedCurve, double _tightness)
  : curvePositions(_curvePositions)
  , bezierSegments()
  , totalCurveLengthInPixels(0.)
  , cumulatedCurveLengthInPixels()
  , isClosedCurve(_isClosedCurve)
  , tightness(_tightness)
{
	if(isClosedCurve)
	{
		// Add last point making it the same as first point due to curve being closed
		curvePositions.push_back(curvePositions[0]);
	}

	for(int i = 0; i < (int) curvePositions.size(); i++)
	{
		BezSeg bs(curvePositions, i, tightness);

		bezierSegments.push_back(bs);
		totalCurveLengthInPixels += bs.getSegmentLengthInPixels();
		cumulatedCurveLengthInPixels.push_back(totalCurveLengthInPixels);
	}
}

const List<DirectPosition> & BezierModel::getCurvePositions() {
	return curvePositions;
}
	
const std::list<BezSeg> & BezierModel::getBezierSegments() {
	return bezierSegments;
}

double BezierModel::getTotalLengthOfAllSegments()
{
	return totalCurveLengthInPixels;
}

// ======================================================================
/*!
 * \brief getSteppedCurvePoints
 *
 * 		  Extracts bezier curve points with given step
 */
 // ======================================================================

int BezierModel::getSteppedCurvePoints(unsigned int baseStep,					// Base distance between curve points along the line
									   unsigned int maxRand,					// Max random distance added to the base
									   unsigned int maxRepeat,					// Max number of subsequent points having equal distance
									   std::list<DirectPosition> & curvePoints)	// Output points
{
	const unsigned int minBaseStep = 3;

	if (baseStep <= minBaseStep)
		baseStep = minBaseStep;

	int curveLength = (int) (floor(getTotalLengthOfAllSegments()) + 0.1);	// Total curve length
	double lss = 0.0;														// Cumulative curve length at the start of current segment
	double t = 0.0;															// Relative position within current segment

	unsigned int step;														// Next step
	unsigned int prevStep = 0;												// Previous step
	unsigned int repStep = 0;												// Counter for subsequent points having equal distance/step
	int curPos = 0;															// Current curve position

	// Segment iterator
	std::list<BezSeg>::iterator itb = bezierSegments.begin();

	// Cumulative segment length iterator
	std::list<double>::iterator itl = cumulatedCurveLengthInPixels.begin(),itlEnd = cumulatedCurveLengthInPixels.end();

	// Number of points
	int nPoints = 0;

	srand(time(NULL));

	while (true) {
		if (nPoints > 0) {
			// Step
			step = baseStep;
	
			if (maxRand > 0) {
				step += (int) ((maxRand + 1) * (rand() / (RAND_MAX + 1.0)));
	
				if (step == prevStep)
				  {
					if (repStep >= maxRepeat)
					  {
						step += ((step == (baseStep + maxRand)) ? -1 : 1);
						repStep = 0;
					  }
					else
					  repStep++;
				  }
	
				prevStep = step;
			}

			curPos += step;
			if ((curPos + (0.5 * baseStep)) >= curveLength)
				curPos = curveLength;

			while ((itl != itlEnd) && (*itl < curPos)) {
				lss = *itl;
	
				itl++;
				itb++;
			}

			if (itl == itlEnd)
				//
				// Never
				break;

			t = ((curPos < curveLength) ? ((curPos - lss) / (*itl - lss)) : 1.0);
		}

		curvePoints.push_back(DirectPosition(itb->getPosition(t)));

		nPoints++;

		if (curPos >= curveLength)
			break;
	}

	return nPoints;
}

// ======================================================================
/*!
 * \brief decorateCurve
 *
 * 		  Generates curve decorator points (for cloud border).
 * 		  Converted from Esa's java code.
 */
 // ======================================================================

void BezierModel::decorateCurve(std::list<DirectPosition> & curvePoints,	// Bezier curve points
								int scaleHeightMin,							// Base curve distance (normal length) for the decorator points
								int scaleHeightRandom,						// Max random distance added to the base
								int controlMin,								// Base offset for the decorator points
								int controlRandom,							// Max random offset added to the base
								std::list<doubleArr> & decoratorPoints)		// Output; decorator points
{
	std::list<DirectPosition>::iterator litcp = curvePoints.begin(),cpend = curvePoints.end(),ritcp;

	ritcp = litcp;
	if (ritcp != cpend)
		ritcp++;

	srand(time(NULL));

	for ( ; (ritcp != cpend); litcp++, ritcp++) {
		doubleArr leftPosition(litcp->getX(), litcp->getY());
		doubleArr rightPosition(ritcp->getX(), ritcp->getY());

		doubleArr basePosition(leftPosition[0] + ((rightPosition[0] - leftPosition[0]) / 2.0) + 0.001,
							   leftPosition[1] + ((rightPosition[1] - leftPosition[1]) / 2.0) + 0.001);

		doubleArr normalScale = Vector2Dee::getScaledNormal(leftPosition,
															basePosition,
															rightPosition,
															NEG,
															scaleHeightMin + ((scaleHeightRandom + 1) * (rand() / (RAND_MAX + 1.0))));

		doubleArr control(basePosition[0] + normalScale[0] + controlMin + ((controlRandom + 1) * (rand() / (RAND_MAX + 1.0))),
						  basePosition[1] + normalScale[1] + controlMin + ((controlRandom + 1) * (rand() / (RAND_MAX + 1.0))));

		decoratorPoints.push_back(control);
	}
}

// Helper class

const int Vector2Dee::X = 0;
const int Vector2Dee::Y = 1;

doubleArr Vector2Dee::normalize(double x, double y)
{
	double len = sqrt(x*x + y*y);
	doubleArr norm(0., 0.);

	if(len == 0)
		return norm;

	norm[X] = x/len;
	norm[Y] = y/len;

	return norm;
}

doubleArr Vector2Dee::normalize(doubleArr vector)
{
	return normalize(vector[X], vector[Y]);
}

doubleArr Vector2Dee::normalize(doubleArr vector1, doubleArr vector2)
{
	return normalize(vector1[X] - vector2[X], vector1[Y] - vector2[Y]);
}

double Vector2Dee::crossProd(doubleArr vector1, doubleArr vector2)
{
	return vector1[X]*vector2[Y] - vector1[Y]*vector2[X];
}

doubleArr Vector2Dee::sub(doubleArr vector1, doubleArr vector2)
{
	return doubleArr(vector1[X] - vector2[X], vector1[Y] - vector2[Y]);
}

doubleArr Vector2Dee::add(doubleArr vector1, doubleArr vector2)
{
	return doubleArr(vector1[X] + vector2[X], vector1[Y] + vector2[Y]);
}

doubleArr Vector2Dee::negate(doubleArr vector)
{
	return doubleArr(-vector[X], -vector[Y]);
}

doubleArr Vector2Dee::scale(doubleArr vector, double scaler)
{
	return doubleArr(scaler*vector[X], scaler*vector[Y]);
}

double Vector2Dee::length(doubleArr vector1, doubleArr vector2)
{
	double dx = vector1[X]-vector2[X];
	double dy = vector1[Y]-vector2[Y];
	return sqrt(dx*dx + dy*dy);
}

double Vector2Dee::length(doubleArr vector)
{
	double x = vector[X];
	double y = vector[Y];
	return sqrt(x*x + y*y);
}

doubleArr Vector2Dee::getScaledNormal(doubleArr & leftPosition,
									  doubleArr & basePosition,
									  doubleArr & rightPosition,
									  Orientation orientation,
									  double scaler)
{
	doubleArr unitNormal = getUnitNormal(leftPosition, basePosition, rightPosition, orientation);
	return scale(unitNormal, scaler);
}

doubleArr Vector2Dee::getUnitNormal(doubleArr & leftPosition,
									doubleArr & basePosition,
									doubleArr & rightPosition,
									Orientation orientation)
{
	doubleArr normalizedLeftVec = normalize(leftPosition,basePosition);
	doubleArr normalizedRightVec = normalize(rightPosition,basePosition);
	return getUnitNormal(normalizedLeftVec, normalizedRightVec, orientation);
}

doubleArr Vector2Dee::getUnitNormal(doubleArr & vector1,
									doubleArr & vector2,
									Orientation orientation)
{
	// NB! Vectors 'vector1' and 'vector2' *must* be normalized (unit)
	// vectors
	doubleArr normal = normalize(add(vector1, vector2));
	double _crossProd = crossProd(vector1, vector2);

	if (orientation == POS) {
		if (_crossProd >= 0.) {
			normal[0] = -normal[0];
			normal[1] = -normal[1];
		}
	} else {
		if (_crossProd < 0.) {
			normal[0] = -normal[0];
			normal[1] = -normal[1];
		}
	}
	return normal;
}

} // namespace frontier
