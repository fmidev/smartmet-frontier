/*
 * PathAdapter.h
 */

#pragma once

#include "Path.h"

#include <geos/geom/Geometry.h>

namespace frontier {

class PathAdapter {
  public:
	void moveto( const geos::geom::Coordinate & coordinate ) { path.moveto(coordinate.x,coordinate.y); }
	void lineto( const geos::geom::Coordinate & coordinate ) { path.lineto(coordinate.x,coordinate.y); }

	const Path & getPath() { return path; }

  private:
	Path path;
};

namespace GeosTools {
	const Path & getContours(const geos::geom::Geometry * geom,PathAdapter & pathAdapter);
}

}
