// ======================================================================
/*!
 * \brief frontier::BezSeg
 *
 *		  Converted from Esa's java code
 */
 // ======================================================================

#include "BezSeg.h"
#include "BezierModel.h"
#include "Vector2Dee.h"

namespace frontier
{

BezSeg::BezSeg(const List<DirectPosition> & theCurvePositions, int theSegmentIndex, boolean isClosedCurve, double theTightness)
  : tightness((theTightness <= 0.) ? 3.0 : theTightness)
{
	init(theCurvePositions, theSegmentIndex, isClosedCurve);
}

void BezSeg::init(const List<DirectPosition> & theCurvePositions, int theSegmentIndex, boolean isClosedCurve)
{
	segmentIndex = theSegmentIndex;
	bIsClosedCurve = isClosedCurve;
	segmentLength = 0.;

	orientation = POS;
//	this.nativeCRS = FeatureGeometryHelper.getDefaultCRS();

	bIsSecondLastSegment = false;
	bIsLastSegment = false;

	curvePositions = theCurvePositions;
//	this.curvePositions = new ArrayList<DirectPosition>();
//	for(DirectPosition pos : curvePositions)
//		this.curvePositions.add(pos);

	computeGeometry();
}

void BezSeg::setTightness(double theTightness)
{
    tightness = theTightness;
}

double BezSeg::getTightness()
{
    return tightness;
}

boolean BezSeg::isChanged()
{
	// TODO: Check if c1 , c2 , start, end are changed .. ?

	return false;
}

void BezSeg::computeGeometry() {
	computeSegmentPointPositions();
	computeSegmentLength();
}

void BezSeg::computeSegmentPointPositions() {

	// Compute four positions a Bézier segment (with index ' segmentIndex') is made up of, namely
	//
	// * a starting curve point
	// * an ending curve point
	// * first control point (also known as first Bézier curve control point 'c1')
	// * second control point (also known as second Bézier curve control point 'c2')

	int numberOfCurvePoints = curvePositions.size();

	if(numberOfCurvePoints == 0)
		return;

	if(numberOfCurvePoints == 1)
	{
		start = DoubleArr(curvePositions[0].getCoordinates()[X], curvePositions[0].getCoordinates()[Y]);
		end = DoubleArr(start[X], start[Y]);
		c1 = DoubleArr(end[X], end[Y]);
		c2 = DoubleArr(start[X], start[Y]);
		return; // Cannot create Bézier segment from a single point anyways ...
	}

	if(numberOfCurvePoints == 2)
	{
		// Create a special "single-control-point" Bézier segment between two first curve points
		start = DoubleArr(curvePositions[0].getCoordinates()[X], curvePositions[0].getCoordinates()[Y]);
		end =   DoubleArr(curvePositions[1].getCoordinates()[X], curvePositions[1].getCoordinates()[Y]);
		c1 = DoubleArr(end[X], end[Y]);
		c2 = DoubleArr(start[X], start[Y]);
		return;
	}

	DoubleArr a;
	DoubleArr b;
	DoubleArr c;

	int startIndex = segmentIndex;
	int endIndex = 0;

	int aStartIndex = 0;
	int bStartIndex = 0;
	int cStartIndex = 0;
	int aEndIndex = 0;
	int bEndIndex = 0;
	int cEndIndex = 0;

	if(bIsClosedCurve)
	{
		// To create a closed loop of a curve, connect starting curve point into ending curve point.
		endIndex = (startIndex + 1) % numberOfCurvePoints;

		aStartIndex = startIndex - 1;
		if(aStartIndex < 0)
			aStartIndex += numberOfCurvePoints;

		aStartIndex = aStartIndex % numberOfCurvePoints;
		bStartIndex = startIndex % numberOfCurvePoints;
		cStartIndex = (startIndex + 1) % numberOfCurvePoints;

		aEndIndex = endIndex - 1;
		if(aEndIndex < 0)
			aEndIndex += numberOfCurvePoints;

		aEndIndex = aEndIndex % numberOfCurvePoints;
		bEndIndex = endIndex % numberOfCurvePoints;
		cEndIndex = (endIndex + 1) % numberOfCurvePoints;
	}
	else
	{
		endIndex = startIndex + 1;

		if(endIndex >= numberOfCurvePoints)
		{
			// "Guess" curve tangent at the end of curve by assuming curve to run ahead as a straight line.
			// This is implemented by the means of creating an extraneous (virtual) curve point.

			start = DoubleArr(curvePositions[startIndex].getCoordinates()[X], curvePositions[startIndex].getCoordinates()[Y]);
			DoubleArr extraneousCurvePoint = Vector2Dee::getUnitVector(Vector2Dee::getSubtractedVector(start, curvePositions[startIndex-1].getCoordinates()));
			end = DoubleArr(start[X] + extraneousCurvePoint[X], start[Y] + extraneousCurvePoint[Y]);

			c1 = DoubleArr(end[X], end[Y]);
			c2 = DoubleArr(start[X], start[Y]);
			return;
		}

		aStartIndex = startIndex - 1;
		bStartIndex = startIndex;
		cStartIndex = startIndex + 1;

		aEndIndex = endIndex - 1;
		bEndIndex = endIndex;
		cEndIndex = endIndex + 1;
	}

	start = DoubleArr(curvePositions[startIndex].getCoordinates()[X], curvePositions[startIndex].getCoordinates()[Y]);
	end = DoubleArr(curvePositions[endIndex].getCoordinates()[X], curvePositions[endIndex].getCoordinates()[Y]);

	// Create 1st control point

	b = DoubleArr(curvePositions[bStartIndex].getCoordinates()[X], curvePositions[bStartIndex].getCoordinates()[Y]);
	c = DoubleArr(curvePositions[cStartIndex].getCoordinates()[X], curvePositions[cStartIndex].getCoordinates()[Y]);

	if(aStartIndex < 0)
	{
		// Approximate missing point 'a' at the beginning of curve
		DoubleArr extraneousCurvePoint = DoubleArr(b[X] - c[X], b[Y] - c[Y]);
		a = DoubleArr(b[X] + extraneousCurvePoint[X], b[Y]  + extraneousCurvePoint[Y]);
	}
	else
	{
		a = DoubleArr(curvePositions[aStartIndex].getCoordinates()[X], curvePositions[aStartIndex].getCoordinates()[Y]);
	}

	int direction = -1;
	c1 = createControlPoint(a, b, c, direction);

	// Create 2nd control point
	a = DoubleArr(curvePositions[aEndIndex].getCoordinates()[X], curvePositions[aEndIndex].getCoordinates()[Y]);
	b = DoubleArr(curvePositions[bEndIndex].getCoordinates()[X], curvePositions[bEndIndex].getCoordinates()[Y]);

	if(cEndIndex >= (int) curvePositions.size())
	{
		// Approximate missing point 'c' at the end of curve
		DoubleArr extraneousCurvePoint = Vector2Dee::getUnitVector(Vector2Dee::getSubtractedVector(b, a));
		c = DoubleArr(b[X] + extraneousCurvePoint[X], b[Y]  + extraneousCurvePoint[Y]);
	}
	else
	{
		c = DoubleArr(curvePositions[cEndIndex].getCoordinates()[X], curvePositions[cEndIndex].getCoordinates()[Y]);
	}

	direction = 1;
	c2 = createControlPoint(a, b, c, direction);
}

DoubleArr BezSeg::createControlPoint(DoubleArr & a, DoubleArr & b, DoubleArr & c, int direction)
{
	// Computes and returns a Bézier control point based on three given curve points and a tangent direction value.
	// Direction value -1 is assumed to be given when creating first Bézier control point c1, whereas
	// direction value 1 is assumed to be given when creating second control point c2.
	// This part is modified from Ref [1].

	DoubleArr delta_a = Vector2Dee::getSubtractedVector(b, a);
	DoubleArr delta_c = Vector2Dee::getSubtractedVector(c, b);
	DoubleArr delta;

	// Get an orthogonal to the angle bisector
	DoubleArr m = Vector2Dee::getUnitVector(Vector2Dee::getAddedVector(Vector2Dee::getUnitVector(delta_a),Vector2Dee::getUnitVector(delta_c)));
	DoubleArr ma = Vector2Dee::getScaledVector(m, direction);

	if(direction == -1)
	{
		delta = delta_c;
		//this.derivativeAtStart = Vector2Dee.getUnitVector(delta);
	}
	else
	{
		delta = delta_a;
		//this.derivativeAtEnd = Vector2Dee.getUnitVector(delta);
	}

	double dist = -Vector2Dee::getLength(delta)/tightness;
	return DoubleArr(b[X] + dist * ma[X],
			 		 b[Y] + dist * ma[Y]);
}

int BezSeg::getSegmentIndex() {
	return segmentIndex;
}

void BezSeg::setControlPoint1(DirectPosition _c1)
{
	c1[X] = _c1.getCoordinates()[X];
	c1[Y] = _c1.getCoordinates()[Y];
	computeSegmentLength(); // Do NOT call computeGeometry() here
}

void BezSeg::setControlPoint2(DirectPosition _c2)
{
	c2[X] = _c2.getCoordinates()[X];
	c2[Y] = _c2.getCoordinates()[Y];
	computeSegmentLength(); // Do NOT call computeGeometry() here
}

void BezSeg::setStartPoint(DirectPosition startPoint) {
	start[X] = startPoint.getCoordinates()[X];
	start[Y] = startPoint.getCoordinates()[Y];
	computeSegmentLength(); // Do NOT call computeGeometry() here
}

void BezSeg::setEndPoint(DirectPosition endPoint) {
	end[X] = endPoint.getCoordinates()[X];
	end[Y] = endPoint.getCoordinates()[Y];
	computeSegmentLength(); // Do NOT call computeGeometry() here
}

DirectPosition BezSeg::getFirstControlPoint()
{
	GeneralDirectPosition pos = GeneralDirectPosition(c1[X], c1[Y]);
//	pos.setCoordinateReferenceSystem(this.nativeCRS);
	return pos;
}

DirectPosition BezSeg::getSecondControlPoint()
{
	GeneralDirectPosition pos = GeneralDirectPosition(c2[X], c2[Y]);
//	pos.setCoordinateReferenceSystem(this.nativeCRS);
	return pos;
}

DirectPosition BezSeg::getNegatedControlPoint1()
{
	DoubleArr c1MinusStart(c1[X], c1[Y]);
	c1MinusStart[X] -= start[X];
	c1MinusStart[Y] -= start[Y];
	DoubleArr c1Negated(start[X] - c1MinusStart[X],  start[Y] - c1MinusStart[Y]);

	GeneralDirectPosition pos = GeneralDirectPosition(c1Negated[X], c1Negated[Y]);
//	pos.setCoordinateReferenceSystem(this.nativeCRS);

	return pos;
}

DirectPosition BezSeg::getNegatedSecondControlPoint()
{
	DoubleArr c2MinusEnd(c2[X], c2[Y]);
	c2MinusEnd[X] -= end[X];
	c2MinusEnd[Y] -= end[Y];
	DoubleArr c2Negated(end[X] - c2MinusEnd[X],  end[Y] - c2MinusEnd[Y]);

	GeneralDirectPosition pos = GeneralDirectPosition(c2Negated[X], c2Negated[Y]);
//	pos.setCoordinateReferenceSystem(this.nativeCRS);

	return pos;
}

DirectPosition BezSeg::getStartPoint()
{
	GeneralDirectPosition pos = GeneralDirectPosition(start[X], start[Y]);
//	pos.setCoordinateReferenceSystem(this.nativeCRS);
	return pos;
}

DirectPosition BezSeg::getEndPoint()
{
	GeneralDirectPosition pos = GeneralDirectPosition(end[X], end[Y]);
//	pos.setCoordinateReferenceSystem(this.nativeCRS);
	return pos;
}

DoubleArr BezSeg::getPosition(double t) {

	if(t == 0.)
	{
		return start;
	}
	else
	if(t == 1.)
	{
		return end;
	}

	double oneMinusT = 1.0 - t;
	DoubleArr pos;

	if(curvePositions.size() == 2)
	{
		// Use simple linear interpolation
		pos[X] = oneMinusT*start[X] + t*end[X];
		pos[Y] = oneMinusT*start[Y] + t*end[Y];
		return pos;
	}

	/* May be applied to *quadric* curve evaluation
	if((this.segmentIndex == 0) || (this.segmentIndex == this.curvePositions.size()-1)) //ALKUP
	{
		// First and last segments are assumed to be a quadratic segments
		// so let's evaluate quadratic Bézier segment.

		double b0 = oneMinusT*oneMinusT;
		double b1 = 2.*t*oneMinusT;
		double b2 = t*t;

		return new GeneralDirectPosition(b0*this.start[X] + b1*this.c1.getCoordinates()[X] + b2*this.end[X],
				                         b0*this.start[Y] + b1*this.c1.getCoordinates()[Y] + b2*this.end[Y]);


		// TESTII
		//return new GeneralDirectPosition(b0*curvePositions.get(this.segmentIndex).getCoordinates()[X] + b1*this.c1.getCoordinates()[X] + b2*curvePositions.get(this.segmentIndex+1).getCoordinates()[X],
                						   b0*curvePositions.get(this.segmentIndex).getCoordinates()[Y] + b1*this.c1.getCoordinates()[Y] + b2*curvePositions.get(this.segmentIndex+1).getCoordinates()[Y]);

	}
	*/

	// Evaluate *cubic* Bézier segment.
	// Returns evaluated t-point [0..1] on given Bézier segment:  P(t) = (1 - t)^3 * P0 + 3t(1-t)^2 * P1 + 3t^2 (1-t) * P2 + t^3 * P3

	double b0 = oneMinusT*oneMinusT*oneMinusT;
	double b1 = 3.0*t*oneMinusT*oneMinusT;
	double b2 = 3.0*t*t*oneMinusT;
	double b3 = t*t*t;

	pos[X] = b0*start[X] + b1*c1[X] + b2*c2[X] + b3*end[X];
	pos[Y] = b0*start[Y] + b1*c1[Y] + b2*c2[Y] + b3*end[Y];
	return pos;
}

void BezSeg::setOrientation(Orientation theOrientation)
{
	orientation = theOrientation;
}

Orientation BezSeg::getOrientation()
{
	return orientation;
}

//PositionAndUnitNormal BezSeg::getPositionAndNormal(double t)
//{
//	double tEps = dt/2.;
//
//	// When necessary, make an artificial position displacement on base point just to make it possible to compute normal in a simplest way
//
//	double tDispaced = t;
//	DoubleArr basePosition;
//
//	// Base point of normal vector point on curve
//	if(t <=  0.000001)
//	{
//		tDispaced = 0.000001;
//		basePosition = getPosition(tDispaced);
//	}
//	else
//	if(t >= 0.999999)
//	{
//		tDispaced = 0.999999;
//		basePosition = getPosition(tDispaced);
//	}
//	else
//	{
//		// Use original given 't'
//		basePosition = getPosition(t);
//	}
//
//	// Left sample point on curve
//	double tLeft = tDispaced - tEps;
//	if(tLeft < 0.) tLeft = 0.;
//	DoubleArr leftPosition = getPosition(tLeft);
//
//	// Right sample point on curve
//	double tRight = tDispaced + tEps;
//	if(tRight > 1.) tRight = 1.;
//	DoubleArr rightPosition = getPosition(tRight);
//
//	boolean isLeftOriented = (orientation == Orientation.POS);
//	DoubleArr normal = Vector2Dee.getUnitNormal(leftPosition, basePosition, rightPosition, isLeftOriented);
//
//	// Restore true base point position corresponding given 't'
//	if(t == 0.0)
//		basePosition = start;
//	else
//		if(t == 1.0)
//			basePosition = end;
//
//	return PositionAndUnitNormal(basePosition, normal);
//}
//
//PositionAndUnitNormal BezSeg::getNormalAtStartPoint()
//{
//	return getPositionAndNormal(0.0);
//}
//
//PositionAndUnitNormal BezSeg::getNormalAtEndPoint()
//{
//	return getPositionAndNormal(1.0);
//}

DoubleArr BezSeg::getTangent(double t)
{
	// Tangent at point t: ]0,1[

	// VAIHTOEHTO 1:

	double t1 = 1. - t;
//	double[] c1 = this.c1;
//	double[] c2 = this.c2;

	DoubleArr tang(
		-3.*t1*t1*start[X]+
		3.*t1*(1-3*t)*c1[X]+
		3.*t*(2-3*t) *c2[X]+
		3.*t*t*end[X],

		-3.*t1*t1*start[Y]+
		3.*t1*(1-3*t)*c1[Y]+
		3.*t*(2-3*t)*c2[Y]+
		3.*t*t*end[Y]);

	return Vector2Dee::getUnitVector(tang);


/*********** OK
	// VAIHTOEHTO 2:

	var p0 = evaluatedPosition(start, c1, c2, end, t - 0.001);
	var p2 = evaluatedPosition(start, c1, c2, end, t + 0.001);
	var tang = [p2[X]-p0[X], p2[Y]-p0[Y]];
***********/


/*********** OK	(ehkä paras?)

	// VAIHTOEHTO 3:

	// http://stackoverflow.com/questions/4089443/find-the-tangent-of-a-point-on-a-cubic-bezier-curve-on-an-iphone

	//	dP(t)/dt = -3(1-t)^2 * P0 + 3(1-t)^2 * P1 - 6t(1-t) * P1 - 3t^2 * P2 + 6t(1-t) * P2 + 3t^2 * P3

	double oneMinusT = 1.0 - t;
	//double a0 = oneMinusT*oneMinusT*oneMinusT;
	double a1 = 3.0*oneMinusT*oneMinusT;
	double a2 = 6.0*t*oneMinusT;
	double a3 = 3.0*t*t;

	double[] start = curvePositions.get(i-1).getCoordinates();
	double[] end = curvePositions.get(i).getCoordinates();
	double[] c1 = bPoints.get(2*i-3).getCoordinates();
	double[] c2 = bPoints.get(2*i-2).getCoordinates();

	DirectPosition tang = new GeneralDirectPosition(-a1*start[X] + a1*c1[X] - a2*c1[X] - a3*c2[X] + a2*c2[X] + a3*end[X],
	                       						    -a1*start[Y] + a1*c1[Y] - a2*c1[Y] - a3*c2[Y] + a2*c2[Y] + a3*end[Y]);

	return Vector2D.normalize(tang);
*/

}

DoubleArr BezSeg::getRotatedUnitVector(DoubleArr p1, DoubleArr p2, double rotationAngleInDegrees)
{
	return Vector2Dee::getRotatedUnitVector(p1, p2, rotationAngleInDegrees);
}

void BezSeg::computeSegmentLength()
{
	if(curvePositions.size() == 1)
		return;

	if(curvePositions.size() == 2)
	{
		// Use simple linear distance length
		segmentLength = Vector2Dee::getLength(end, start);
		return;
	}

	double len = 0.;
	DoubleArr prevPos = start;
	DoubleArr evalPos;

	for(double t = 0.; t <= 1.0; t += dt/2.)
	{
		evalPos = getPosition(t);
		len += Vector2Dee::getLength(evalPos, prevPos);
		prevPos = evalPos;
	}

	segmentLength = len;
}

double BezSeg::getSegmentLength()
{
	return segmentLength;
}

void BezSeg::isSecondLastSegment(boolean yesOrNo) {
	bIsSecondLastSegment = yesOrNo;
}

boolean BezSeg::isSecondLastSegment() {
	return bIsSecondLastSegment;
}

void BezSeg::isLastSegment(boolean yesOrNo) {
	bIsLastSegment = yesOrNo;
}

boolean BezSeg::isLastSegment() {
	return bIsLastSegment;
}

boolean BezSeg::isMiddleSegment() {
	// NB! Assumption: number of Bézier segments equals number of given curve points !

	int maxSegmentIndex = curvePositions.size()-1;
	if((segmentIndex <= 1) && (maxSegmentIndex <= 1))
		return true;

	if(segmentIndex == maxSegmentIndex/2)
		return true;

	return false;
}

} // namespace frontier
