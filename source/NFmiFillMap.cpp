// ======================================================================
//
// Definition of a map of x-coordinates indexed by an y-coordinate.
//
// See NFmiFillMap.h for documentation.
//
// History:
//
// 13.08.2001 Mika Heiskanen
//
//	Implemented
//
// 24.04.2012 PKi
//
//	Modified to get surface fill symbol positions.
//
// ======================================================================

#include "NFmiFillMap.h"

#include <algorithm>
#include <cmath>
#include <list>

using namespace std;

// A help utility to avoid eps-size errors in filling due to
// for example small rounding errors in projections

namespace
{
float myround(float theValue)
{
  if (abs(theValue - round(theValue)) < 0.001)  // 0.001 pixels is meaningless
    return round(theValue);
  return theValue;
}
}  // namespace

namespace frontier
{
namespace
{
// ----------------------------------------------------------------------
// Find the first x1-x2 range of the row overlapping the xMin-xMax range
// wide enough for the fill symbol. Narrows xMin-xMax to the overlap and
// returns the iterator to the range's x2, or the end of the row if
// there is no such range.
// ----------------------------------------------------------------------

NFmiFillMapElement::iterator findOverlappingRange(
    NFmiFillMapElement& row, int imageWidth, int symbolWidth, int& xMin, int& xMax)
{
  // We have no active x-coordinate yet

  float x1 = kFloatMissing;

  for (auto dataiter = row.begin(); dataiter != row.end(); ++dataiter)
  {
    float x2 = *dataiter;

    // If last x was invalid, set new beginning of line

    if (x1 == kFloatMissing)
    {
      x1 = x2;
      continue;
    }

    int i1 = static_cast<int>(ceil(x1));
    int i2 = static_cast<int>(floor(x2));

    x1 = kFloatMissing;

    // If intersection has integer X coordinate, x1 would be
    // interior, x2 exterior

    if (x2 == i2)
      i2--;

    if ((i1 < imageWidth) && (i2 > 0) && (i2 > i1))
    {
      // Adjust the x1-x2 range with image dimensions
      //

      i1 = std::max(i1, 0);
      i2 = std::min(i2, imageWidth - 1);

      if (i1 >= xMax)
        return row.end();

      if (i2 > xMin)
      {
        // Adjust the x1-x2 range with current max(x1) and min(x2) values
        //
        i1 = std::max(i1, xMin);
        i2 = std::min(i2, xMax);

        if ((i2 - i1) >= symbolWidth)
        {
          xMin = i1;
          xMax = i2;
          return dataiter;
        }
      }
    }
  }

  return row.end();
}

// ----------------------------------------------------------------------
// Get all map areas, regardless of their size
// ----------------------------------------------------------------------

void collectMapAreas(const NFmiFillMapData& theData, NFmiFillAreas& fillAreas)
{
  auto iter = theData.begin();
  auto enditer = theData.end();

  for (; (iter != enditer); ++iter)
  {
    auto dataiter = iter->second.begin();
    auto enddata = iter->second.end();

    for (; (dataiter != enddata);)
    {
      double x1 = *dataiter;
      dataiter++;

      if (dataiter != enddata)
      {
        double x2 = *dataiter;
        fillAreas.emplace_back(Point(std::min(x1, x2), iter->first),
                               Point(std::max(x1, x2), iter->first + 1));

        dataiter++;
      }
    }
  }
}

// ----------------------------------------------------------------------
// Sort the x-coordinates of each row
// ----------------------------------------------------------------------

void sortRows(NFmiFillMapData& data)
{
  for (auto iter = data.begin(); iter != data.end(); ++iter)
    if (iter->second.size() == 2)
    {
      if (iter->second[0] > iter->second[1])
        swap(iter->second[0], iter->second[1]);
    }
    else
      sort(iter->second.begin(), iter->second.end());
}

// ----------------------------------------------------------------------
// Test whether the fill symbol would exceed the image or fill area boundary,
// or whether the iterators have reached each other
// ----------------------------------------------------------------------

bool rowLimitReached(bool downwards, int y, int symbolHeight, int yPrev, int yLast, int imageHeight)
{
  if (downwards)
    return ((y < symbolHeight) || (y <= yPrev));
  return ((y + symbolHeight) > std::min(yLast, imageHeight));
}

}  // namespace

const size_t fillAreaWidthMin = 3;
const size_t fillAreaHeightMin = 3;

// ----------------------------------------------------------------------
// Scan for continuous rows overlapping the given xMin-xMax range to check
// if the "column" is wide/tall enough for fill symbol.
//
// Stores the maximum horizontal or vertical rectangle and returns true
// if the area is big enough.
// ----------------------------------------------------------------------

template <typename T>
bool NFmiFillMap::scanColumn(T iter,
                             const T& endIter,
                             int imageWidth,
                             int imageHeight,
                             int symbolWidth,
                             int symbolHeight,
                             int yFirst,
                             int xMin,
                             int xMax,
                             bool verticalRects,
                             NFmiFillAreas& fillAreas)
{
  // Collector for 'x2' iterators

  std::list<NFmiFillMapElement::iterator> x2iters;

  // Number of continuous rows needed

  int h = symbolHeight;

  // Top or bottom y -coord of the rectangle

  int yLast = 0;

  T fiter = ++iter;

  for (; iter != endIter; ++iter)
  {
    float y = iter->first;

    if (y < 0 || y >= imageHeight)
      break;

    yLast = static_cast<int>(y);

    // Set to true if the row is valid for fill

    auto x2iter = findOverlappingRange(iter->second, imageWidth, symbolWidth, xMin, xMax);
    bool validRow = (x2iter != iter->second.end());

    if (validRow)
    {
      x2iters.push_back(x2iter);
      h--;
    }

    if ((!validRow) || ((!verticalRects) && (h == 0)))
      break;
  }  // for iter

  if (h <= 0)
  {
    // The "column" is wide/tall enough for fill symbol.
    //
    // Reserve the area of the column/rectangle by splitting the x1-x2
    // ranges to x1-rectx1 and rectx2-x2. First adjust the rectangle
    // width (rectx1 and rectx2) down to multiple of symbolWidth.
    //
    std::list<NFmiFillMapElement::iterator>::iterator it;

    float x2 = NAN;
    int width = xMax - xMin;
    int n = static_cast<int>(floor(width / symbolWidth));
    width -= (n * symbolWidth);

    if (width > 0)
    {
      xMin += static_cast<int>(floor(width / 2));
      xMax -= static_cast<int>(floor(width / 2));
    }

    int nextx2 = xMax + symbolWidth;

    for (it = x2iters.begin(); it != x2iters.end(); ++it, fiter++)
    {
      x2 = *(*it);
      *(*it) = xMin;

      if (x2 >= nextx2)
      {
        fiter->second.push_back(xMax);
        fiter->second.push_back(x2);
        sort(fiter->second.begin(), fiter->second.end());
      }
    }

    // Store the rectangle

    fillAreas.emplace_back(Point(xMin, std::min(yFirst, yLast)),
                           Point(xMax, std::max(yFirst, yLast)));

    return true;
  }

  return false;
}

// ----------------------------------------------------------------------
// Scan the x1-x2 ranges of the current row. For each range wide enough
// for the fill symbol, scan the rows upwards/downwards to check if there
// is space for the fill symbol.
//
// Returns true if any areas were found.
// ----------------------------------------------------------------------

template <typename T>
bool NFmiFillMap::scanRow(T iter,
                          const T& endIter,
                          int imageWidth,
                          int imageHeight,
                          int symbolWidth,
                          int symbolHeight,
                          int y,
                          bool verticalRects,
                          NFmiFillAreas& fillAreas)
{
  bool found = false;

  // We have no active x-coordinate yet

  float x1 = kFloatMissing;

  auto dataiter = iter->second.begin();
  auto dataend = iter->second.end();

  for (; dataiter != dataend; ++dataiter)
  {
    float x2 = *dataiter;

    // If last x was invalid, set new beginning of line

    if (x1 == kFloatMissing)
    {
      x1 = x2;
      continue;
    }

    int i1 = static_cast<int>(ceil(x1));
    int i2 = static_cast<int>(floor(x2));

    // And invalidate x1

    x1 = kFloatMissing;

    // If intersection has integer X coordinate, x1 would be
    // interior, x2 exterior

    if (x2 == i2)
      i2--;

    if ((i1 < imageWidth) && (i2 > 0) && (i2 > i1))
    {
      // Adjust the x1-x2 range with image dimensions
      //

      i1 = std::max(i1, 0);
      i2 = std::min(i2, imageWidth - 1);

      // If the width is enough, scan the rows upwards/downwards to check if
      // there is space for the fill symbol

      if ((i2 - i1) >= symbolWidth && scanColumn(iter,
                                                 endIter,
                                                 imageWidth,
                                                 imageHeight,
                                                 symbolWidth,
                                                 symbolHeight,
                                                 y,
                                                 i1,
                                                 i2,
                                                 verticalRects,
                                                 fillAreas))
        found = true;
    }
  }

  return found;
}

// ----------------------------------------------------------------------
// Get symbol fill areas. The maximum horizontal or vertical rectangle
// is provided for each suitable position.
//
// Returns true if any areas were found.
// ----------------------------------------------------------------------

bool NFmiFillMap::getFillAreas(int imageWidth,
                               int imageHeight,
                               int symbolWidth,
                               int symbolHeight,
                               float scale,
                               bool verticalRects,
                               NFmiFillAreas& fillAreas,
                               bool retainMap,
                               bool scanUpDown,
                               bool getMapAreas)
{
  if (itsData.empty())
    return false;

  if (getMapAreas)
  {
    // Get all areas, regardless of their size
    //
    collectMapAreas(itsData, fillAreas);
    return true;
  }

  symbolWidth = static_cast<int>(floor(symbolWidth * scale));
  symbolHeight = static_cast<int>(floor(symbolHeight * scale));

  if ((imageWidth <= symbolWidth) || (imageHeight <= symbolHeight) ||
      (symbolWidth < (int)fillAreaWidthMin) || (symbolHeight < (int)fillAreaHeightMin))
    return false;

  bool found = false;

  // The iterator for traversing the data is not const, because we
  // sort the x-coordinates.
  //
  // Using copy of the map data if it must be retained; x -coords
  // will be changed in processing.

  NFmiFillMapData _data;

  if (retainMap)
    _data = itsData;

  NFmiFillMapData& data = (retainMap ? _data : itsData);
  auto iter = data.begin();
  auto enditer = data.end();
  NFmiFillMapData::reverse_iterator riter(data.end());
  NFmiFillMapData::reverse_iterator renditer(data.begin());

  sortRows(data);

  // Iterate over y-coordinates in the map from both directions
  // if upDown is true; otherwise from bottom up

  int i = 0;

  int yLast = static_cast<int>((--enditer)->first);
  int yPrev = 0;
  enditer++;

  for (iter = data.begin(); iter != enditer; i += (scanUpDown ? 1 : 2))
  {
    if (i)
    {
      if ((!scanUpDown) || i % 2)
        iter++;
      else
        riter++;
    }

    // Skip the row if the y-coordinate is below 0.
    //
    // Stop iteration if the fill symbol would exceed the image or fill area boundary,
    // or when the iterators have reached each other

    int y = ((i % 2) ? static_cast<int>(riter->first) : static_cast<int>(iter->first));

    if (y < 0)
      continue;
    if (rowLimitReached((i % 2) != 0, y, symbolHeight, yPrev, yLast, imageHeight))
      break;

    yPrev = y;

    // Scan the row's x1-x2 ranges upwards/downwards to check if there is space for the fill symbol

    bool rowFound = ((i % 2) ? scanRow(riter,
                                       renditer,
                                       imageWidth,
                                       imageHeight,
                                       symbolWidth,
                                       symbolHeight,
                                       y,
                                       verticalRects,
                                       fillAreas)
                             : scanRow(iter,
                                       enditer,
                                       imageWidth,
                                       imageHeight,
                                       symbolWidth,
                                       symbolHeight,
                                       y,
                                       verticalRects,
                                       fillAreas));
    found |= rowFound;
  }

  return found;
}

// ----------------------------------------------------------------------
// A method to add an edge into the map
// ----------------------------------------------------------------------

void NFmiFillMap::Add(float theX1, float theY1, float theX2, float theY2)
{
  // Ignore invalid coordinates

  if (theX1 == kFloatMissing || theY1 == kFloatMissing || theX2 == kFloatMissing ||
      theY2 == kFloatMissing)
    return;

  // Ignore lines completely outside the area

  if (itsLoLimit != kFloatMissing && std::max(theY1, theY2) < itsLoLimit)
    return;

  if (itsHiLimit != kFloatMissing && std::min(theY1, theY2) > itsHiLimit)
    return;

  // The parametric equation of the line is:
  //
  // x = x1 + s*(x2-x1)
  // y = y1 + s*(y2-y1)
  //
  // where s=[0,1]
  //
  // Solving for s from the latter we get
  //
  //          y-y1
  // x = x1 + ----- * (x2-x1) = x1 + k * (y-y1) = (x1 - k*y1) + k*y
  //          y2-y1
  //
  // The equation is always valid, since the case y1=y2 has already been
  // handled.

  float x1 = myround(theX1);
  float y1 = myround(theY1);
  float x2 = myround(theX2);
  float y2 = myround(theY2);

  // First, we ignore horizontal lines, they are meaningless
  // when filling with horizontal lines.

  if (y1 == y2)
    return;

  if (y1 > y2)
  {
    swap(x1, x2);
    swap(y1, y2);
  }

  int lo = static_cast<int>(ceil(y1));
  int hi = static_cast<int>(floor(y2));

  // If limits are active, we can speed things up by clipping the limits

  if (itsLoLimit != kFloatMissing && lo < itsLoLimit)
    lo = static_cast<int>(ceil(itsLoLimit));

  if (itsHiLimit != kFloatMissing && hi > itsHiLimit)
    hi = static_cast<int>(floor(itsHiLimit));

  // We don't want to intersect ymin, it is handled
  // by the line connected to this one, except at
  // the bottom!

  if (y2 <= 0)
    return;

  if (static_cast<float>(lo) == y1 && y1 > 0)
    lo++;

  // We precalculate k and x1+k*y1 for speed
  // Should in principle remove the multiplication too,
  // but realistically speaking this cost quite small,
  // and a decent compiler will make fast code anyway.

  float k = (x2 - x1) / (y2 - y1);
  float tmp = x1 - k * y1;

  for (int j = lo; j <= hi; ++j)
    itsData[j].push_back(tmp + k * j);
}

}  // namespace frontier
