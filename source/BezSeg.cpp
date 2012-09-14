// ======================================================================
/*!
 * \brief frontier::BezSeg
 *
 *		  Converted from Esa's java code
 */
 // ======================================================================

#include "BezSeg.h"
#include "BezierModel.h"

namespace frontier
{

BezSeg::BezSeg(List<DirectPosition> _curvePositions, int _segmentIndex)
  : dt(0.04)
  , segmentIndex(_segmentIndex)
  , segmentLength(0.)
  , curvePositions(_curvePositions)
{
	computeControlPointPositions();
	computeSegmentLengthInPixels();
}

void BezSeg::computeControlPointPositions() {

	if(curvePositions.size() == 1)
		return; // Cannot create bezier segment from a single point anyways ...

	if(curvePositions.size() == 2)
	{
		// Create a special "single-control-point" bezier segment between from first curve points
		start = doubleArr(curvePositions[0].getCoordinates()[0], curvePositions[0].getCoordinates()[1]);
		end =  doubleArr(curvePositions[1].getCoordinates()[0], curvePositions[1].getCoordinates()[1]);
		c1 = end;
		c2 = start;
		return;
	}

	// Starting index:  size-1, endingIndex:  Starting index + 1

	start = doubleArr(curvePositions[segmentIndex].getCoordinates()[0], curvePositions[segmentIndex].getCoordinates()[1]);

	int ind = std::min((int) curvePositions.size()-1, segmentIndex+1);
	end = doubleArr(curvePositions[ind].getCoordinates()[0], curvePositions[ind].getCoordinates()[1]);

	// Create 1st control point for bezier segment 'i'
	ind = std::max(0, segmentIndex-1);
	doubleArr a(curvePositions[ind].getCoordinates()[0], curvePositions[ind].getCoordinates()[1]);

	ind = segmentIndex;
	doubleArr b(curvePositions[ind].getCoordinates()[0], curvePositions[ind].getCoordinates()[1]);

	ind = std::min((int) curvePositions.size()-1, segmentIndex+1);
	doubleArr c(curvePositions[ind].getCoordinates()[0], curvePositions[ind].getCoordinates()[1]);

	int direction = -1;
	c1 = createControlPoint(a, b, c, direction);

	// Create 2nd control point for bezier segment 'i'
	ind = segmentIndex;
	a = doubleArr(curvePositions[ind].getCoordinates()[0], curvePositions[ind].getCoordinates()[1]);

	ind = std::min((int) curvePositions.size()-1, segmentIndex+1);
	b = doubleArr(curvePositions[ind].getCoordinates()[0], curvePositions[ind].getCoordinates()[1]);

	ind = std::min((int) curvePositions.size()-1, segmentIndex+2);
	c = doubleArr(curvePositions[ind].getCoordinates()[0], curvePositions[ind].getCoordinates()[1]);

	direction = 1;
	c2 = createControlPoint(a, b, c, direction);
}

doubleArr BezSeg::createControlPoint(doubleArr & a, doubleArr & b, doubleArr & c, int direction)
{
	// Computes and returns a bezier control point based on three given curve points and a tangent direction value.
	// Direction value -1 is assumed to be given when creating first bezier control point c1, whereas
	// direction value 1 is assumed to be given when creating second control point c2.
	// This part is modified from Ref [1].

	double tightness = 3.0;
	doubleArr delta_a = Vector2Dee::sub(b, a);
	doubleArr delta_c = Vector2Dee::sub(c, b);
	doubleArr delta;

	// Get an orthogonal to the angle bisector
	doubleArr m = Vector2Dee::normalize(Vector2Dee::add(Vector2Dee::normalize(delta_a),Vector2Dee::normalize(delta_c)));
	doubleArr ma = Vector2Dee::scale(m, direction);

	if(direction == -1)
	{
		delta = delta_c;
		//this.derivativeAtStart = Vector2D.normalize(delta);
	}
	else
	{
		delta = delta_a;
		//this.derivativeAtEnd = Vector2D.normalize(delta);
	}

	double dist = -Vector2Dee::length(delta)/tightness;
	return doubleArr(b[0] + dist * ma[0],
					 b[1] + dist * ma[1]);
}

int BezSeg::getSegmentIndex() {
	return segmentIndex;
}

DirectPosition BezSeg::getControlPoint1()
{
	GeneralDirectPosition pos(c1[0], c1[1]);
	return pos;
}

DirectPosition BezSeg::getControlPoint2()
{
	GeneralDirectPosition pos(c2[0], c2[1]);
	return pos;
}

DirectPosition BezSeg::getStartPoint()
{
	GeneralDirectPosition pos(start[0], start[1]);
	return pos;
}

DirectPosition BezSeg::getEndPoint()
{
	GeneralDirectPosition pos(end[0], end[1]);
	return pos;
}

doubleArr BezSeg::getPosition(double t) {

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
	doubleArr pos;

	if(curvePositions.size() == 2)
	{
		// Use simple linear interpolation
		pos[0] = oneMinusT*start[0] + t*end[0];
		pos[1] = oneMinusT*start[1] + t*end[1];
		return pos;
	}

	/* May be applied to *quadric* curve evaluation
	if((this.segmentIndex == 0) || (this.segmentIndex == this.curvePositions.size()-1)) //ALKUP
	{
		// First and last segments are assumed to be a quadratic segments
		// so let's evaluate quadratic bezier segment.

		double b0 = oneMinusT*oneMinusT;
		double b1 = 2.*t*oneMinusT;
		double b2 = t*t;

		return new GeneralDirectPosition(b0*this.start[0] + b1*this.c1.getCoordinates()[0] + b2*this.end[0],
										 b0*this.start[1] + b1*this.c1.getCoordinates()[1] + b2*this.end[1]);


		// TESTII
		//return new GeneralDirectPosition(b0*curvePositions.get(this.segmentIndex).getCoordinates()[0] + b1*this.c1.getCoordinates()[0] + b2*curvePositions.get(this.segmentIndex+1).getCoordinates()[0],
										   b0*curvePositions.get(this.segmentIndex).getCoordinates()[1] + b1*this.c1.getCoordinates()[1] + b2*curvePositions.get(this.segmentIndex+1).getCoordinates()[1]);

	}
	*/

	// Evaluate *cubic* bezier segment.
	// Returns evaluated t-point [0..1] on given bezier segment:  P(t) = (1 - t)^3 * P0 + 3t(1-t)^2 * P1 + 3t^2 (1-t) * P2 + t^3 * P3

	double b0 = oneMinusT*oneMinusT*oneMinusT;
	double b1 = 3.0*t*oneMinusT*oneMinusT;
	double b2 = 3.0*t*t*oneMinusT;
	double b3 = t*t*t;

	pos[0] = b0*start[0] + b1*c1[0] + b2*c2[0] + b3*end[0];
	pos[1] = b0*start[1] + b1*c1[1] + b2*c2[1] + b3*end[1];
	return pos;
}

doubleArr BezSeg::getTangent(double t)
{
	// Tangent at point t: ]0,1[

	// VAIHTOEHTO 1:

	double t1 = 1. - t;

	doubleArr tang(
		-3.*t1*t1*start[0]+
		3.*t1*(1-3*t)*c1[0]+
		3.*t*(2-3*t) *c2[0]+
		3.*t*t*end[0],

		-3.*t1*t1*start[1]+
		3.*t1*(1-3*t)*c1[1]+
		3.*t*(2-3*t)*c2[1]+
		3.*t*t*end[1]);

	return Vector2Dee::normalize(tang);


/*********** OK
	// VAIHTOEHTO 2:

	var p0 = evaluatedPosition(start, c1, c2, end, t - 0.001);
	var p2 = evaluatedPosition(start, c1, c2, end, t + 0.001);
	var tang = [p2[0]-p0[0], p2[1]-p0[1]];
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

	DirectPosition tang = new GeneralDirectPosition(-a1*start[0] + a1*c1[0] - a2*c1[0] - a3*c2[0] + a2*c2[0] + a3*end[0],
													-a1*start[1] + a1*c1[1] - a2*c1[1] - a3*c2[1] + a2*c2[1] + a3*end[1]);

	return Vector2D.normalize(tang);
*/
}

void BezSeg::computeSegmentLengthInPixels()
{
	if(curvePositions.size() == 1)
		return;

	if(curvePositions.size() == 2)
	{
		// Use simple linear distance length
		segmentLength = Vector2Dee::length(end, start);
		return;
	}

	double len = 0.;
	doubleArr prevPos = start;
	doubleArr evalPos;

	for(double t = 0.; t <= 1.0; t += dt/2.)
	{
		evalPos = getPosition(t);
		len += Vector2Dee::length(evalPos, prevPos);
		prevPos = evalPos;
	}

	segmentLength = len;
}

double BezSeg::getSegmentLengthInPixels()
{
	return segmentLength;
}

} // namespace frontier
