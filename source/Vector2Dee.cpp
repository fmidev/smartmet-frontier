// ======================================================================
/*!
 * \brief frontier::Vector2Dee
 *
 *		  Converted from Esa's java code
 */
// ======================================================================

#include "Vector2Dee.h"
#include <math.h>

static const double PI = 3.14159265;

namespace frontier
{
DoubleArr Vector2Dee::getUnitVectorFromScalars(double x, double y)
{
  double len = sqrt(x * x + y * y);
  DoubleArr norm(0., 0.);

  if (len == 0) return norm;

  norm[X] = x / len;
  norm[Y] = y / len;

  return norm;
}

DoubleArr Vector2Dee::getUnitVector(DoubleArr vector)
{
  return getUnitVectorFromScalars(vector[X], vector[Y]);
}

DoubleArr Vector2Dee::getUnitVector(DoubleArr vector1, DoubleArr vector2)
{
  return getUnitVectorFromScalars(vector1[X] - vector2[X], vector1[Y] - vector2[Y]);
}

DoubleArr Vector2Dee::getScaledNormal(DoubleArr leftPosition,
                                      DoubleArr basePosition,
                                      DoubleArr rightPosition,
                                      boolean isLeftOriented,
                                      double scaler)
{
  DoubleArr unitNormal = getUnitNormal(leftPosition, basePosition, rightPosition, isLeftOriented);
  return getScaledVector(unitNormal, scaler);
}

DoubleArr Vector2Dee::getUnitNormal(DoubleArr leftPosition,
                                    DoubleArr basePosition,
                                    DoubleArr rightPosition,
                                    boolean isLeftOriented)
{
  DoubleArr normalizedLeftVec = Vector2Dee::getUnitVector(leftPosition, basePosition);
  DoubleArr normalizedRightVec = Vector2Dee::getUnitVector(rightPosition, basePosition);
  return getUnitNormal(normalizedLeftVec, normalizedRightVec, isLeftOriented);
}

DoubleArr Vector2Dee::getUnitNormal(DoubleArr normalizedVector1,
                                    DoubleArr normalizedVector2,
                                    boolean isLeftOriented)
{
  //////////////////////////////////////////////////////////////////////////
  // NB! Vectors 'normalizedVector1' and 'normalizedVector2' *must* be normalized (unit) vectors
  //////////////////////////////////////////////////////////////////////////

  if (normalizedVector1 == getNegatedVector(normalizedVector2))
  {
    // Given vectors were equal in magnitude but opposite in direction resulting in a null vector.
    // Set up a (rather clumsy) epsilon displacement just to make vectors point slightly different
    // directions
    normalizedVector1[0] += 0.00001;  // Only displace X of 'vector1'
    normalizedVector2[1] += 0.00001;  // Only displace Y of 'vector2'
  }

  DoubleArr normal =
      getUnitVector(Vector2Dee::getAddedVector(normalizedVector1, normalizedVector2));
  double crossProd = getCrossProd(normalizedVector1, normalizedVector2);

  if (isLeftOriented)
  {
    if (crossProd >= 0.)
    {
      normal[X] = -normal[X];
      normal[Y] = -normal[Y];
    }
  }
  else
  {
    if (crossProd < 0.)
    {
      normal[X] = -normal[X];
      normal[Y] = -normal[Y];
    }
  }

  return normal;
}

double Vector2Dee::getCrossProd(DoubleArr vector1, DoubleArr vector2)
{
  return vector1[X] * vector2[Y] - vector1[Y] * vector2[X];
}

DoubleArr Vector2Dee::getSubtractedVector(DoubleArr vector1, DoubleArr vector2)
{
  return DoubleArr(vector1[X] - vector2[X], vector1[Y] - vector2[Y]);
}

DoubleArr Vector2Dee::getAddedVector(DoubleArr vector1, DoubleArr vector2)
{
  return DoubleArr(vector1[X] + vector2[X], vector1[Y] + vector2[Y]);
}

DoubleArr Vector2Dee::getNegatedVector(DoubleArr vector)
{
  return DoubleArr(-vector[X], -vector[Y]);
}

boolean Vector2Dee::equals(DoubleArr vector1, DoubleArr vector2)
{
  return ((vector1[X] == vector2[X]) && (vector1[Y] == vector2[Y]));
}

DoubleArr Vector2Dee::getScaledVector(DoubleArr vector, double scaler)
{
  return DoubleArr(scaler * vector[X], scaler * vector[Y]);
}

double Vector2Dee::getLength(DoubleArr vector1, DoubleArr vector2)
{
  double dx = vector1[X] - vector2[X];
  double dy = vector1[Y] - vector2[Y];
  return sqrt(dx * dx + dy * dy);
}

double Vector2Dee::getLength(DoubleArr vector)
{
  double x = vector[X];
  double y = vector[Y];
  return sqrt(x * x + y * y);
}

double Vector2Dee::getRotationAngleInRadians(DoubleArr p1, DoubleArr p2)
{
  // Returns angle between given points in RADIANS.
  double dx = p2[X] - p1[X];
  double dy = p2[Y] - p1[Y];
  return (float)atan2(dy, dx);
}

double Vector2Dee::getRotationAngleInDegrees(DoubleArr p1, DoubleArr p2)
{
  // Returns RADIAN angle between given points.
  return (getRotationAngleInRadians(p1, p2) * (180 / PI));
}

DoubleArr Vector2Dee::getRotatedUnitVector(DoubleArr p1,
                                           DoubleArr p2,
                                           double rotationAngleInDegrees)
{
  // Returns rotated *unit* vector by rotating vector ['p1'-'p2'] by the given rotation angle

  double initialAngleInRadians = getRotationAngleInRadians(p1, p2);
  double radAngle = initialAngleInRadians + (rotationAngleInDegrees / (180 / PI));
  double lineSegmentLength = getLength(getSubtractedVector(p1, p2));
  DoubleArr vector;
  vector[X] = lineSegmentLength * cos(radAngle);
  vector[Y] = lineSegmentLength * sin(radAngle);
  return getUnitVector(vector);
}

}  // namespace frontier
