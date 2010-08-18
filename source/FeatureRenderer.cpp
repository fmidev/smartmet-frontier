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
#include <smartmet/woml/Jet.h>
#include <smartmet/woml/OccludedFront.h>
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
  // TODO
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
  // TODO
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
  // TODO
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
  // TODO
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

  std::string frontname = "warmfront" + boost::lexical_cast<std::string>(nwarmfronts);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();
  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  paths << "<path id=\"" << frontname << "\" d=\"" << path.svg() << "\"/>\n";

  warmfronts << "<use class=\"warmfront\" xlink:href=\"#"
			 << frontname
			 << "\"/>\n";

  BOOST_FOREACH(const woml::SimpleCubicSpline & spline, splines)
	{
	  if(!spline.empty())
		{
		  warmfronts << "<g class=\"warmfrontglyph\">\n"
					 << " <text><textPath xlink:href=\"#"
					 << frontname
					 << "\" startOffset=\"30\">W</textPath></text>\n"
					 << " <text><textPath xlink:href=\"#"
					 << frontname
					 << "\" startOffset=\"90\">W</textPath></text>\n"
					 << " <text><textPath xlink:href=\"#"
					 << frontname
					 << "\" startOffset=\"150\">W</textPath></text>\n"
					 << " <text><textPath xlink:href=\"#"
					 << frontname
					 << "\" startOffset=\"210\">W</textPath></text>\n"
					 << " <text><textPath xlink:href=\"#"
					 << frontname
					 << "\" startOffset=\"270\">W</textPath></text>\n"
					 << " <text><textPath xlink:href=\"#"
					 << frontname
					 << "\" startOffset=\"330\">W</textPath></text>\n"
					 << "</g>\n";
		}
	}
}

} // namespace frontier
