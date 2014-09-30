// ======================================================================
//
// Definition of a map of x-coordinates indexed by an y-coordinate.
// The container is used for holding the points where a collection
// of edges intersects integral Y-coordinates. Typical use involves
// intersecting all edges of a contour, and then filling the contour
// based on the x- and y-coordinates of the this container.
//
// This class is intended to be used only by NFmiImage Fill methods
// as a temporary data-holder.
//
// Typical usage:
//
//	NFmiImage img(400,400);
//	int color = img.Color(255,0,0);	// red
//	NFmiFillMap theMap(theTree);	// build from NFmiContourTree
//	img.Fill(theMap,color);		// fill image based on the map
//
// The map needed by Fill can also be built from a NFmiPath, or just
// common vectors of floats (or a vector of a pair of floats).
//
// For speed reasons the constructor also accepts as input
// limiting values for the Y coordinates, outside which
// nothing should be saved. The default values of the limits
// is kFloatMissing, which implies no limit.
//
// This is meaningful when we are rendering only a small part
// of the polygon, for example when zooming into the data.
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

#ifndef IMAGINE_NFMIFILLMAP_H
#define IMAGINE_NFMIFILLMAP_H

#include "Point.h"
#include <smartmet/newbase/NFmiGlobals.h>

#include <algorithm>	// vector sorting
#include <map>			// maps
#include <set>			// sets
#include <vector>		// vectors
#include <utility>		// pairs
#include <cmath>		// abs,min,max
#include <list>			// lists

namespace frontier
{

  // Typedefs to ease things

  typedef std::vector<float>					NFmiFillMapElement;
  typedef std::map<float,NFmiFillMapElement>	NFmiFillMapData;
  typedef std::pair<Point,Point>				NFmiFillRect;			// Bottom left and top right corner
  typedef std::list<NFmiFillRect>				NFmiFillAreas;
  typedef std::list<Point>						NFmiFillPositions;

  class NFmiFillMap
  {

  public:

	// Constructor

	NFmiFillMap(float theLoLimit=kFloatMissing, float theHiLimit=kFloatMissing)
	  : itsData()
	  , itsLoLimit(theLoLimit)
	  , itsHiLimit(theHiLimit)
	{ }

	// Data access

	const NFmiFillMapData & MapData(void) const		{ return itsData; }

	// Adding a line

	void Add(float theX1, float theY1,
			 float theX2, float theY2);

	// Areas wide/tall enough for symbol filling

	bool getFillAreas(int imageWidth, int imageHeigth,
					  int symbolWidth, int symbolHeigth,
					  float scale,
					  bool verticalRects,
					  NFmiFillAreas & fillAreas,
					  bool retainMap = true,
					  bool scanUpDown = false,
					  bool getMapAreas = false);

  private:

	template <typename T>
	bool scanColumn(T iter,T endIter,
					int imageWidth,int imageHeigth,
					int symbolWidth,int symbolHeigth,
					int yMin,int xMin,int xMax,
					bool verticalRects,
					NFmiFillAreas & fillAreas);

	// Data-elements

	NFmiFillMapData	itsData;
	float			itsLoLimit;
	float			itsHiLimit;

  };

} // namespace frontier

#endif // IMAGINE_NFMIFILLMAP_H

// ======================================================================
