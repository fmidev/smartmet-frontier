// ======================================================================
/*!
 * \brief frontier::FeatureRenderer
 */
// ======================================================================

#include "FeatureRenderer.h"
#include "Path.h"
#include "PathFactory.h"
#include "PathTransformation.h"
#include <smartmet/woml/ColdFront.h>
#include <smartmet/woml/CubicSplineSurface.h>
#include <smartmet/woml/Jet.h>
#include <smartmet/woml/OccludedFront.h>
#include <smartmet/woml/SurfacePrecipitationArea.h>
#include <smartmet/woml/Trough.h>
#include <smartmet/woml/UpperTrough.h>
#include <smartmet/woml/WarmFront.h>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>
#include <iomanip>

namespace frontier
{

  // ----------------------------------------------------------------------
  /*!
   * \brief Functor for projecting a path
   */
  // ----------------------------------------------------------------------

  class PathProjector : public PathTransformation
  {
  public:
	PathProjector(const boost::shared_ptr<NFmiArea> & theArea)
	  : area(theArea)
	{
	}

	void operator()(double & x, double & y) const
	{
	  NFmiPoint ll(x,y);
	  NFmiPoint xy = area->ToXY(ll);
	  x = xy.X();
	  y = xy.Y();
	}

  private:
	boost::shared_ptr<NFmiArea> area;
	PathProjector();

  };

  // ----------------------------------------------------------------------
  /*!
   * \brief Utility function for front rendering
   */
  // ----------------------------------------------------------------------

  void render_front(const Path & path,
					std::ostringstream & paths,
					std::ostringstream & fronts,
					const std::string & id,
					const std::string & lineclass,
					const std::string & glyphclass,
					const std::string & glyphs1,
					const std::string & glyphs2,
					double fontsize,
					double spacing)
  {
	paths << "<path id=\"" << id << "\" d=\"" << path.svg() << "\"/>\n";
	
	fronts << "<use class=\""
		   << lineclass
		   << "\" xlink:href=\"#"
		   << id
		   << "\"/>\n";
	
	double len = path.length();
	double textlength = 0.5*(glyphs1.size() + glyphs2.size());
	int intervals = static_cast<int>(std::floor(len/(fontsize*textlength+spacing)+0.5));
	double interval = len/intervals;
	double startpoint = interval/2;
	
	fronts << "<g class=\"" << glyphclass << "\">\n";
	
	for(int j=0; j<intervals; ++j)
	  {
		double offset = startpoint + j * interval;
		fronts << " <text><textPath xlink:href=\"#"
			   << id
			   << "\" startOffset=\""
			   << std::fixed << std::setprecision(1) << offset
			   << "\">"
			   << (j%2 == 0 ? glyphs1 : glyphs2)
			   << "</textPath></text>\n";
	  }
	fronts << "</g>\n";
  }
  
  // ----------------------------------------------------------------------
  /*!
   * \brief Utility function for surface rendering
   */
  // ----------------------------------------------------------------------

  void render_surface(const Path & path,
					  std::ostringstream & paths,
					  std::ostringstream & masks,
					  std::ostringstream & surfaces,
					  const std::string & id,
					  const std::string & lineclass,
					  const std::string & glyphclass,
					  const std::string & glyphs1,
					  const std::string & glyphs2,
					  double fontsize,
					  double spacing)
  {
	paths << "<path id=\"" << id << "\" d=\"" << path.svg() << "\"/>\n";
	
	// If you give width=100% and height=100% Batik may screw up
	// the image. Not giving the dimensions seems to work.
	
	masks << "<mask id=\""
		  << id
		  << "mask\" maskUnits=\"userSpaceOnUse\">\n"
		  << " <use xlink:href=\"#"
		  << id
		  << "\" class=\"precipitationmask\"/>\n"
		  << "</mask>\n";

	surfaces << "<use class=\""
			 << lineclass
			 << "\" xlink:href=\"#"
			 << id
			 << "\" mask=\"url(#"
			 << id
			 << "mask)\"/>\n";

	// TODO: THE REST IS UNTESTED - FITTING CHARS TO PATH LENGTH NEEDS FIXING

	double textlength = 0.5*(glyphs1.size() + glyphs2.size());

	if(textlength > 0)
	  {
		double len = path.length();
		int intervals = static_cast<int>(std::floor(len/(fontsize*textlength+spacing)+0.5));
		double interval = len/intervals;
		double startpoint = interval/2;
	
		surfaces << "<g class=\"" << glyphclass << "\">\n";
	
		for(int j=0; j<intervals; ++j)
		  {
			double offset = startpoint + j * interval;
			surfaces << " <text><textPath xlink:href=\"#"
					 << id
					 << "\" startOffset=\""
					 << std::fixed << std::setprecision(1) << offset
					 << "\">"
					 << (j%2 == 0 ? glyphs1 : glyphs2)
					 << "</textPath></text>\n";
		  }
		surfaces << "</g>\n";
	  }
  }
  
// ----------------------------------------------------------------------
/*!
 * \brief Constructor
 */
// ----------------------------------------------------------------------

FeatureRenderer::FeatureRenderer(const std::string & theTemplate,
								 const boost::shared_ptr<NFmiArea> & theArea)
  : svgbase(theTemplate)
  , area(theArea)
  , ncloudborders(0)
  , ncoldfronts(0)
  , njets(0)
  , noccludedfronts(0)
  , npointnotes(0)
  , npointsymbols(0)
  , npointvalues(0)
  , nprecipitationareas(0)
  , ntroughs(0)
  , nuppertroughs(0)
  , nwarmfronts(0)
{
}

// ----------------------------------------------------------------------
/*!
 * \brief Return the final SVG
 */
// ----------------------------------------------------------------------

std::string FeatureRenderer::svg() const
{
  std::string ret = svgbase;

  int h = static_cast<int>(std::floor(0.5+area->Height()));
  int w = static_cast<int>(std::floor(0.5+area->Width()));

  std::string width = boost::lexical_cast<std::string>(w);
  std::string height = boost::lexical_cast<std::string>(h);
  
  using boost::algorithm::replace_all;

  replace_all(ret,"--WIDTH--",width);
  replace_all(ret,"--HEIGHT--",height);

  replace_all(ret,"--PATHS--",paths.str());
  replace_all(ret,"--MASKS--",masks.str());

  replace_all(ret,"--PRECIPITATIONAREAS--",precipitationareas.str());;
  replace_all(ret,"--CLOUDBORDERS--"      ,cloudborders.str());;
  replace_all(ret,"--OCCLUDEDFRONTS--"    ,occludedfronts.str());;
  replace_all(ret,"--WARMFRONTS--"        ,warmfronts.str());;
  replace_all(ret,"--COLDFRONTS--"        ,coldfronts.str());;
  replace_all(ret,"--TROUGHS--"           ,troughs.str());;
  replace_all(ret,"--UPPERTROUGHS--"      ,uppertroughs.str());;
  replace_all(ret,"--JETS--"              ,jets.str());;
  replace_all(ret,"--POINTNOTES--"        ,pointnotes.str());;
  replace_all(ret,"--POINTSYMBOLS--"      ,pointsymbols.str());;
  replace_all(ret,"--POINTVALUES--"       ,pointvalues.str());;

  return ret;

}

// ----------------------------------------------------------------------
/*!
 * \brief Render a CloudAreaBorder
 */
// ----------------------------------------------------------------------

void
FeatureRenderer::visit(const woml::CloudAreaBorder & theFeature)
{
  // TODO
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a ColdFront
 */
// ----------------------------------------------------------------------

void
FeatureRenderer::visit(const woml::ColdFront & theFeature)
{
  ++ncoldfronts;
  std::string id = "coldfront" + boost::lexical_cast<std::string>(ncoldfronts);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = 20;
  double spacing = 30;

  render_front(path,paths,coldfronts,id,
			   "coldfront","coldfrontglyph",
			   "C","C",
			   fontsize,spacing);
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a Jet
 */
// ----------------------------------------------------------------------

void
FeatureRenderer::visit(const woml::Jet & theFeature)
{
  // TODO
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a OccludedFront
 */
// ----------------------------------------------------------------------

void
FeatureRenderer::visit(const woml::OccludedFront & theFeature)
{
  ++noccludedfronts;
  std::string id = "occludedfront" + boost::lexical_cast<std::string>(noccludedfronts);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = 20;
  double spacing = 30;

  render_front(path,paths,occludedfronts,id,
			   "occludedfront","occludedfrontglyph",
			   "WC","WC",
			   fontsize,spacing);
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a PointGeophysicalParameterValueSet
 */
// ----------------------------------------------------------------------

void
FeatureRenderer::visit(const woml::PointGeophysicalParameterValueSet & theFeature)
{
  // TODO
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a PointMeteorologicalSymbol
 */
// ----------------------------------------------------------------------

void
FeatureRenderer::visit(const woml::PointMeteorologicalSymbol & theFeature)
{
  // TODO
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a PointNote
 */
// ----------------------------------------------------------------------

void
FeatureRenderer::visit(const woml::PointNote & theFeature)
{
  // TODO
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a SurfacePrecipitationArea
 */
// ----------------------------------------------------------------------

void
FeatureRenderer::visit(const woml::SurfacePrecipitationArea & theFeature)
{
  ++nprecipitationareas;
  std::string id = "precipitation" + boost::lexical_cast<std::string>(nprecipitationareas);

  const woml::CubicSplineSurface surface = theFeature.controlSurface();

  Path path = PathFactory::create(surface);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = 20;
  double spacing = 30;

  render_surface(path,paths,masks,precipitationareas,id,
				 "precipitation",
				 "","","",fontsize,spacing);
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a Trough
 */
// ----------------------------------------------------------------------

void
FeatureRenderer::visit(const woml::Trough & theFeature)
{
  // TODO
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a UpperTrough
 */
// ----------------------------------------------------------------------

void
FeatureRenderer::visit(const woml::UpperTrough & theFeature)
{
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a WarmFront
 */
// ----------------------------------------------------------------------

void
FeatureRenderer::visit(const woml::WarmFront & theFeature)
{
  ++nwarmfronts;
  std::string id = "warmfront" + boost::lexical_cast<std::string>(nwarmfronts);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = 20;
  double spacing = 30;

  render_front(path,paths,warmfronts,id,
			   "warmfront","warmfrontglyph",
			   "W","W",
			   fontsize,spacing);
}

} // namespace frontier
