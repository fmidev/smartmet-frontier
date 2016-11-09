// ======================================================================
/*!
 * \brief frontier::Vector2Dee
 *
 *		  Converted from Esa's java code
 */
// ======================================================================

#ifndef FRONTIER_VECTOR2DEE_H
#define FRONTIER_VECTOR2DEE_H

#include "DoubleArr.h"

namespace frontier
{
class Vector2Dee
{
 public:
  static DoubleArr getUnitVector(DoubleArr vector);
  static DoubleArr getUnitVector(DoubleArr vector1, DoubleArr vector2);
  static DoubleArr getScaledNormal(DoubleArr leftPosition,
                                   DoubleArr basePosition,
                                   DoubleArr rightPosition,
                                   boolean isLeftOriented,
                                   double scaler);
  static DoubleArr getUnitNormal(DoubleArr leftPosition,
                                 DoubleArr basePosition,
                                 DoubleArr rightPosition,
                                 boolean isLeftOriented);
  static DoubleArr getUnitNormal(DoubleArr normalizedVector1,
                                 DoubleArr normalizedVector2,
                                 boolean isLeftOriented);
  static double getCrossProd(DoubleArr vector1, DoubleArr vector2);
  static DoubleArr getSubtractedVector(DoubleArr vector1, DoubleArr vector2);
  static DoubleArr getAddedVector(DoubleArr vector1, DoubleArr vector2);
  static DoubleArr getNegatedVector(DoubleArr vector);
  static boolean equals(DoubleArr vector1, DoubleArr vector2);
  static DoubleArr getScaledVector(DoubleArr vector, double scaler);
  static double getLength(DoubleArr vector1, DoubleArr vector2);
  static double getLength(DoubleArr vector);
  static double getRotationAngleInRadians(DoubleArr p1, DoubleArr p2);
  static double getRotationAngleInDegrees(DoubleArr p1, DoubleArr p2);
  static DoubleArr getRotatedUnitVector(DoubleArr p1, DoubleArr p2, double rotationAngleInDegrees);

 private:
  static DoubleArr getUnitVectorFromScalars(double x, double y);

  static const int X = 0;
  static const int Y = 1;
};

}  // namespace frontier

#endif  // FRONTIER_VECTOR2DEE_H
