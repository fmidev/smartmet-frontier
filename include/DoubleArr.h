// ======================================================================
/*!
 * \brief frontier::doubleArr
 */
// ======================================================================

#ifndef FRONTIER_DOUBLEARR_H
#define FRONTIER_DOUBLEARR_H

namespace frontier
{
enum Orientation
{
  POS,
  NEG
};
typedef bool boolean;

class DoubleArr
{
 public:
  DoubleArr(double d1 = 0.0, double d2 = 0.0)
  {
    itsArr[0] = d1;
    itsArr[1] = d2;
  }
  DoubleArr(const DoubleArr& d) = default;
  DoubleArr& operator=(const DoubleArr& d) = default;

  bool operator==(const DoubleArr& d) const
  {
    return ((itsArr[0] == d.itsArr[0]) && (itsArr[1] == d.itsArr[1]));
  }
  double& operator[](const int i) { return itsArr[i ? 1 : 0]; }
  const double& getX() const { return itsArr[0]; }
  const double& getY() const { return itsArr[1]; }

 private:
  double itsArr[2];
};

}  // namespace frontier

#endif  // FRONTIER_DOUBLEARR_H
