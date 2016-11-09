// ======================================================================
/*!
 * \brief frontier::BezierModel
 *
 *		  Converted from Esa's java code
 */
// ======================================================================

#include "BezierModel.h"
#include "Vector2Dee.h"
#include <boost/foreach.hpp>
#include <math.h>

namespace frontier
{
BezierModel::BezierModel(const List<DirectPosition>& theCurvePositions,
                         boolean isClosedCurve,
                         double theTightness)
    : tightness(theTightness)
{
  init(theCurvePositions, isClosedCurve);
}

void BezierModel::init(const List<DirectPosition>& theCurvePositions, boolean isClosedCurve)
{
  // Creates list of Bezier segments *from the scratch*.

  curvePositions = theCurvePositions;
  //	this.curvePositions = null;
  //	this.curvePositions = new ArrayList<DirectPosition>();
  //	for(DirectPosition pos : curvePositions)
  //	{
  //		this.curvePositions.add(pos);
  //	}

  bIsClosedCurve = isClosedCurve;
  orientation = POS;
  totalCurveLength = 0.;

  int lastIndex = curvePositions.size() - 1;
  int secondLastIndex = curvePositions.size() - 2;

  for (int i = 0; i < (int)curvePositions.size(); i++)
  {
    BezSeg bs(curvePositions, i, isClosedCurve, tightness);
    bs.isSecondLastSegment(false);  // Just an initialization
    bs.isLastSegment(false);        // Just an initialization

    if (i == lastIndex)
    {
      bs.isLastSegment(true);
    }
    else if (i == secondLastIndex)
    {
      bs.isSecondLastSegment(true);
    }

    bezierSegments.push_back(bs);
    totalCurveLength += bs.getSegmentLength();
    cumulatedCurveLength.push_back(totalCurveLength);
  }

  //	this.curveDecorator = new BezierCurve(this);
}

void BezierModel::addCurvePosition(DirectPosition curvePosition, boolean isClosedCurve)
{
  curvePositions.push_back(curvePosition);

  init(curvePositions, isClosedCurve);
}

double BezierModel::getCumulatedCurveLength(int i)
{
  std::list<double>::iterator iter = cumulatedCurveLength.begin();
  advance(iter, i);

  return *iter;
}

const List<DirectPosition>& BezierModel::getCurvePositions()
{
  return curvePositions;
}

const std::list<BezSeg>& BezierModel::getBezierSegments()
{
  return bezierSegments;
}

int BezierModel::getBezierSegmentCount()
{
  return bezierSegments.size();
}

DirectPosition BezierModel::getStartPointOfLastBezierSegment()
{
  if (curvePositions.size() <= 2)
    return curvePositions[0];

  return bezierSegments.back().getStartPoint();
}

DirectPosition BezierModel::getEndPointOfLastBezierSegment()
{
  return bezierSegments.back().getEndPoint();
}

DirectPosition BezierModel::getStartPointOfFirstBezierSegment()
{
  return bezierSegments.front().getStartPoint();
}

DirectPosition BezierModel::getEndPointOfFirstBezierSegment()
{
  return bezierSegments.front().getEndPoint();
}

DirectPosition BezierModel::getFirstControlPointOfLastBezierSegment()
{
  return bezierSegments.back().getFirstControlPoint();
}

DirectPosition BezierModel::getSecondControlPointOfFirstBezierSegment()
{
  return bezierSegments.front().getSecondControlPoint();
}

DirectPosition BezierModel::getFirstControlPointOfFirstBezierSegment()
{
  return bezierSegments.front().getFirstControlPoint();
}

DirectPosition BezierModel::getSecondControlPointOfLastBezierSegment()
{
  return bezierSegments.back().getSecondControlPoint();
}

double BezierModel::getTotalLengthOfAllSegments()
{
  return totalCurveLength;
}

void BezierModel::setOrientation(Orientation theOrientation)
{
  orientation = theOrientation;
}

Orientation BezierModel::getOrientation()
{
  return orientation;
}

// public BezierCurve getBezierCurve() {
//	return this.curveDecorator;
//}

// public Shape getCurveLinePath() {
//	return this.curveDecorator.getCurveLinePath();
//}

// public Rectangle getBoundingCurveLineRect() {
//	return this.getCurveLinePath().getBounds();
//}

// public Shape getDecorationLinePath() {
//	return this.curveDecorator.getDecorationLinePath();
//}

// public void setCurveDecorator(BezierCurve curveDecorator) {
//	this.curveDecorator = curveDecorator;
//}

BezSeg BezierModel::getLastBezierSegment()
{
  return bezierSegments.back();
}

boolean BezierModel::isEmpty()
{
  return bezierSegments.empty();
}

// public List<Integer> getEvaluatedCurvePositionSegmentIndices()
//{
//    return this.curveDecorator.getEvaluatedCurvePositionSegmentIndices();
//}

// public DirectPosition getEvaluatedCurvePosition(double cumulatedPathLength) {
//    return this.getBezierCurve().getEvaluatedCurvePosition(cumulatedPathLength);
//}

// public List<DirectPosition> getEvaluatedCurvePositions(double pathLengthIncrement)
//{
//    List<DirectPosition> evaluatedPositions = new ArrayList<DirectPosition>();
//    for(double cumulatedPathLength = 0; cumulatedPathLength < this.getTotalLengthOfAllSegments();
//    cumulatedPathLength += pathLengthIncrement)
//    {
//        DirectPosition evaluatedPos = this.getEvaluatedCurvePosition(cumulatedPathLength);
//        evaluatedPositions.add(evaluatedPos);
//    }
//    return evaluatedPositions;
//}

boolean BezierModel::isClosedCurve()
{
  return bIsClosedCurve;
}

void BezierModel::setTightness(double theTightness)
{
  // All Bezier segments in model are assumed to be of same tightness value
  for (std::list<BezSeg>::iterator bezSeg = bezierSegments.begin();
       (bezSeg != bezierSegments.end());
       bezSeg++)
  {
    bezSeg->setTightness(theTightness);
  }
}

double BezierModel::getTightness()
{
  // All Bezier segments in model are assumed to be of same tightness value.
  // (At least one segment should be found)
  return bezierSegments.front().getTightness();
}

// ======================================================================
/*!
 * \brief getSteppedCurvePoints
 *
 * 		  Extracts bezier curve points with given step
 */
// ======================================================================

int BezierModel::getSteppedCurvePoints(
    unsigned int baseStep,   // Base distance between curve points along the line
    unsigned int maxRand,    // Max random distance added to the base
    unsigned int maxRepeat,  // Max number of subsequent points having equal distance
    std::list<DirectPosition>& curvePoints)  // Output points
{
  const unsigned int minBaseStep = 3;

  if (baseStep <= minBaseStep)
    baseStep = minBaseStep;

  int curveLength = (int)(floor(getTotalLengthOfAllSegments()) + 0.1);  // Total curve length
  double lss = 0.0;  // Cumulative curve length at the start of current segment
  double t = 0.0;    // Relative position within current segment

  unsigned int step;          // Next step
  unsigned int prevStep = 0;  // Previous step
  unsigned int repStep = 0;   // Counter for subsequent points having equal distance/step
  int curPos = 0;             // Current curve position

  // Segment iterator
  std::list<BezSeg>::iterator itb = bezierSegments.begin();

  // Cumulative segment length iterator
  std::list<double>::iterator itl = cumulatedCurveLength.begin(),
                              itlEnd = cumulatedCurveLength.end();

  // Number of points
  int nPoints = 0;

  srand(time(NULL));

  curvePoints.clear();

  while (true)
  {
    if (nPoints > 0)
    {
      // Step
      step = baseStep;

      if (maxRand > 0)
      {
        step += (int)((maxRand + 1) * (rand() / (RAND_MAX + 1.0)));

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

      while ((itl != itlEnd) && (*itl < curPos))
      {
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

void BezierModel::decorateCurve(
    std::list<DirectPosition>& curvePoints,  // Bezier curve points
    bool negative,                           // Decorator point direction
    int scaleHeightMin,     // Base curve distance (normal length) for the decorator points
    int scaleHeightRandom,  // Max random distance added to the base
    int controlMin,         // Base offset for the decorator points
    int controlRandom,      // Max random offset added to the base
    std::list<DoubleArr>& decoratorPoints)  // Output; decorator points
{
  std::list<DirectPosition>::iterator litcp = curvePoints.begin(), cpend = curvePoints.end(), ritcp;

  ritcp = litcp;
  if (ritcp != cpend)
    ritcp++;

  srand(time(NULL));

  decoratorPoints.clear();

  for (; (ritcp != cpend); litcp++, ritcp++)
  {
    DoubleArr leftPosition(litcp->getX(), litcp->getY());
    DoubleArr rightPosition(ritcp->getX(), ritcp->getY());

    DoubleArr basePosition(leftPosition[0] + ((rightPosition[0] - leftPosition[0]) / 2.0) + 0.001,
                           leftPosition[1] + ((rightPosition[1] - leftPosition[1]) / 2.0) + 0.001);

    DoubleArr normalScale = Vector2Dee::getScaledNormal(
        leftPosition,
        basePosition,
        rightPosition,
        negative ? NEG : POS,
        scaleHeightMin + ((scaleHeightRandom + 1) * (rand() / (RAND_MAX + 1.0))));

    DoubleArr control(basePosition[0] + normalScale[0] + controlMin +
                          ((controlRandom + 1) * (rand() / (RAND_MAX + 1.0))),
                      basePosition[1] + normalScale[1] + controlMin +
                          ((controlRandom + 1) * (rand() / (RAND_MAX + 1.0))));

    decoratorPoints.push_back(control);
  }
}

}  // namespace frontier
