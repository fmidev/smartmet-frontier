// ======================================================================
/*!
 * \brief frontier::Path
 */
// ======================================================================

#include "Path.h"
#include "PathTransformation.h"
#include "CubicBezier.h"
#include <cmath>
#include <iomanip>
#include <sstream>

#include "clipper.hpp"

#define sqr(x) ((x) * (x))

namespace frontier
{
  // ----------------------------------------------------------------------
  /*!
   * \brief Supported absolute SVG path elements
   *
   * Note: We store the enums as doubles, which forces us
   * to make explicit conversions to avoid warnings. This
   * is an intentional design choice to keep the representation
   * as simple as possible. In particular we want to avoid
   * a class structure of path elements
   */
  // ----------------------------------------------------------------------

  enum PathElement
	{
	  ClosePath,
	  MoveTo,
	  LineTo,
	  CurveTo
	};

  // ----------------------------------------------------------------------
  /*!
   * \brief ClosePath
   */
  // ----------------------------------------------------------------------

  void Path::closepath()
  {
	pathdata.push_back(ClosePath);
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief MoveTo
   */
  // ----------------------------------------------------------------------

  void Path::moveto(double x, double y)
  {
	pathdata.push_back(MoveTo);
	pathdata.push_back(x);
	pathdata.push_back(y);
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief LineTo
   */
  // ----------------------------------------------------------------------

  void Path::lineto(double x, double y)
  {
	pathdata.push_back(LineTo);
	pathdata.push_back(x);
	pathdata.push_back(y);
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief CurveTo
   */
  // ----------------------------------------------------------------------

  void Path::curveto(double x1, double y1, double x2, double y2, double x, double y)
  {
	pathdata.push_back(CurveTo);
	pathdata.push_back(x1);
	pathdata.push_back(y1);
	pathdata.push_back(x2);
	pathdata.push_back(y2);
	pathdata.push_back(x);
	pathdata.push_back(y);
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Append another path
   */
  // ----------------------------------------------------------------------

  Path & Path::operator+=(const Path & path)
  {
	std::copy(path.pathdata.begin(),
			  path.pathdata.end(),
			  std::back_inserter(pathdata));
	return *this;
  }
  
  // ----------------------------------------------------------------------
  /*!
   * \brief SVG representation of the curve
   */
  // ----------------------------------------------------------------------

  std::string Path::svg() const
  {
	std::ostringstream out;

	char lastelement = ' ';
	char element = ' ';

	for(PathData::size_type i=0; i<pathdata.size(); )
	  {
		if(i!=0) out << ' ';
		out << std::fixed << std::setprecision(1);

		PathElement cmd = static_cast<PathElement>(pathdata[i++]);

		switch(cmd)
		  {
		  case ClosePath:
			element = 'Z';
			if(element != lastelement) out << element;
			lastelement = element;
			break;
		  case MoveTo:
			element = 'M';
			if(element != lastelement) out << element;
			lastelement = element;
			out << pathdata[i] << ',' << pathdata[i+1];
			i += 2;
			break;
		  case LineTo:
			element = 'L';
			if(element != lastelement) out << element;
			lastelement = element;
			out << pathdata[i] << ',' << pathdata[i+1];
			i += 2;
			break;
		  case CurveTo:
			element = 'C';
			if(element != lastelement) out << element;
			lastelement = element;
			out << pathdata[i] << ',' << pathdata[i+1] << ' '
				<< pathdata[i+2] << ',' << pathdata[i+3] << ' '
				<< pathdata[i+4] << ',' << pathdata[i+5];
			i += 6;
			break;
		  }
	  }
	return out.str();
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Test if the path is empty
   */
  // ----------------------------------------------------------------------

  bool Path::empty() const
  {
	return pathdata.empty();
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Length of the curve. Adds the curve points to a fillmap to be used
   * 	    when determing positions for symbol/pattern fill.
   */
  // ----------------------------------------------------------------------

  double Path::length(NFmiFillMap * fmap) const
  {
	double len = 0;
	double lastx = 0;
	double lasty = 0;
	double startx = 0;
	double starty = 0;

	for(PathData::size_type i=0; i<pathdata.size(); )
	  {
		PathElement cmd = static_cast<PathElement>(pathdata[i++]);

		switch(cmd)
		  {
		  case ClosePath:
			// Note: startx, starty remain the same according to SVG spec
			len += sqrt(sqr(lastx-startx)+sqr(lasty-starty));
			lastx = startx;
			lasty = starty;
			break;
		  case MoveTo:
			lastx = pathdata[i];
			lasty = pathdata[i+1];
			// start new subpath
			startx = lastx;
			starty = lasty;
			i += 2;
			break;
		  case LineTo:
			len += sqrt(sqr(lastx-pathdata[i])+sqr(lasty-pathdata[i+1]));
			if (fmap)
				fmap->Add(lastx,lasty,pathdata[i],pathdata[i+1]);
			lastx = pathdata[i];
			lasty = pathdata[i+1];
			i += 2;
			break;
		  case CurveTo:
			{
			  CubicBezier bez(lastx,lasty,
							  pathdata[i],pathdata[i+1],
							  pathdata[i+2],pathdata[i+3],
							  pathdata[i+4],pathdata[i+5]);
			  double eps = 0.001;
			  len += bez.length(eps,fmap,&lastx,&lasty);
			  lastx = pathdata[i+4];
			  lasty = pathdata[i+5];
			  i += 6;
			  break;
			}
		  }
	  }
	return len;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Path transformation
   */
  // ----------------------------------------------------------------------

  void Path::transform(const PathTransformation & transformation)
  {
	for(PathData::size_type i=0; i<pathdata.size(); )
	  {
		PathElement cmd = static_cast<PathElement>(pathdata[i++]);

		switch(cmd)
		  {
		  case ClosePath:
			break;
		  case MoveTo:
		  case LineTo:
			transformation(pathdata[i],pathdata[i+1]);
			i += 2;
			break;
		  case CurveTo:
			transformation(pathdata[i],pathdata[i+1]);
			transformation(pathdata[i+2],pathdata[i+3]);
			transformation(pathdata[i+4],pathdata[i+5]);
			i += 6;
			break;
		  }
	  }
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Path scaling. Uses 'clipper' module
   * 		(http://sourceforge.net/projects/polyclipping/).
   */
  // ----------------------------------------------------------------------

  Path Path::scale(double offset) const
  {
	// Load 'clipper' path

	using namespace ClipperLib;

	ClipperLib::Path srcPath;
	double lastx = 0;
	double lasty = 0;

	for(PathData::size_type i=0; i<pathdata.size(); )
	  {
		PathElement cmd = static_cast<PathElement>(pathdata[i++]);

		switch(cmd)
		  {
		  case ClosePath:
			break;
		  case MoveTo:
		  case LineTo:
			srcPath << IntPoint(lastx = pathdata[i],lasty = pathdata[i+1]);
			i += 2;
			break;
		  case CurveTo:
			CubicBezier bez(lastx,lasty,
							pathdata[i],pathdata[i+1],
							pathdata[i+2],pathdata[i+3],
							pathdata[i+4],pathdata[i+5]);
			double eps = 0.001;
			bez.length(eps,NULL,NULL,NULL,&srcPath);
			lastx = pathdata[i+4];
			lasty = pathdata[i+5];
			i += 6;
			break;
		  }
	  }

	// Scale the 'clipper' path

	ClipperOffset co;
	ClipperLib::Paths solution;
	co.AddPath(srcPath,jtRound,etClosedPolygon);
	co.Execute(solution,offset);

	// Load and return scaled path

	frontier::Path scaledPath;

	for (ClipperLib::Path::const_iterator it = solution.front().begin(); (it != solution.front().end()); it++)
		if (it == solution.front().begin())
			scaledPath.moveto(it->X,it->Y);
		else
			scaledPath.lineto(it->X,it->Y);

	scaledPath.closepath();

	return scaledPath;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Get path's bounding box.
   */
  // ----------------------------------------------------------------------

  void Path::walk(size_t i,BBox & bbox,double x,double y) const
  {
	if (i == 1) {
		bbox.blX = bbox.trX = x;
		bbox.blY = bbox.trY = y;
	}
	else {
		if (x < bbox.blX) bbox.blX = x;
		else if (x > bbox.trX) bbox.trX = x;

		if (y < bbox.blY) bbox.blY = y;
		else if (y > bbox.trY) bbox.trY = y;
	}
  }

  Path::BBox Path::getBBox() const
  {
	BBox bbox;

	for(PathData::size_type i=0; i<pathdata.size(); )
	  {
		PathElement cmd = static_cast<PathElement>(pathdata[i++]);

		switch(cmd)
		  {
		  case ClosePath:
			break;
		  case MoveTo:
		  case LineTo:
			walk((size_t) i,bbox,pathdata[i],pathdata[i+1]);
			i += 2;
			break;
		  case CurveTo:
			walk((size_t) i,bbox,pathdata[i],pathdata[i+1]);
			walk((size_t) i,bbox,pathdata[i+2],pathdata[i+3]);
			walk((size_t) i,bbox,pathdata[i+4],pathdata[i+5]);
			i += 6;
			break;
		  }
	  }

	return bbox;
  }

} // namespace frontier
