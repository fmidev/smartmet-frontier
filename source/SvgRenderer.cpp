// ======================================================================
/*!
 * \brief frontier::SvgRenderer
 */
// ======================================================================

#include "SvgRenderer.h"
#include "Path.h"
#include "PathFactory.h"
#include "PathTransformation.h"

#include <smartmet/woml/CloudAreaBorder.h>
#include <smartmet/woml/ColdFront.h>
#include <smartmet/woml/CubicSplineSurface.h>
#include <smartmet/woml/FontSymbol.h>
#include <smartmet/woml/GeophysicalParameterValueSet.h>
#include <smartmet/woml/GraphicSymbol.h>
#include <smartmet/woml/Jet.h>
#include <smartmet/woml/OccludedFront.h>
#include <smartmet/woml/PointGeophysicalParameterValueSet.h>
#include <smartmet/woml/PointMeteorologicalSymbol.h>
#include <smartmet/woml/SurfacePrecipitationArea.h>
#include <smartmet/woml/Trough.h>
#include <smartmet/woml/UpperTrough.h>
#include <smartmet/woml/WarmFront.h>

#include <smartmet/newbase/NFmiArea.h>
#include <smartmet/newbase/NFmiEnumConverter.h>
#include <smartmet/newbase/NFmiQueryData.h>
#include <smartmet/newbase/NFmiFastQueryInfo.h>

#include <smartmet/macgyver/Cast.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include <iomanip>
#include <iostream>
#include <limits>

namespace frontier
{

  // ----------------------------------------------------------------------
  /*!
   * \brief Read a configuration value with proper error messages
   */
  // ----------------------------------------------------------------------

  template <typename T>
  T lookup(const libconfig::Config & config,
		   const std::string & name)
  {
	try
	  {
		T value = config.lookup(name);
		return value;
	  }
	catch(libconfig::ConfigException & e)
	  {
		if(!config.exists(name))
		  throw std::runtime_error("Setting for "+name+" is missing");
		throw std::runtime_error("Failed to parse value of '"
								 + name
								 + "' as type "
								 + Fmi::number_name<T>());
	  }
  }

  template <typename T>
  T lookup(const libconfig::Setting & setting,
		   const std::string & prefix,
		   const std::string & name)
  {
	T ret;

	if(setting.lookupValue(name,ret))
	  return ret;

	if(!setting.exists(name))
	  throw std::runtime_error("Setting for "+name+" is missing");

	if(!prefix.empty())
	  throw std::runtime_error("Failed to parse value of '"
							   + (prefix + "." + name)
							   + "' as type "
							   + Fmi::number_name<T>());
	else
	  throw std::runtime_error("Failed to parse value of '"
							   + name
							   + "' as type "
							   + Fmi::number_name<T>());
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Convert ptime NFmiMetTime
   */
  // ----------------------------------------------------------------------
  
  NFmiMetTime to_mettime(const boost::posix_time::ptime  & theTime)
  {
	return NFmiMetTime(theTime.date().year(),
					   theTime.date().month(),
					   theTime.date().day(),
					   theTime.time_of_day().hours(),
					   theTime.time_of_day().minutes(),
					   theTime.time_of_day().seconds(),
					   1);
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Make URI valid for SVG
   */
  // ----------------------------------------------------------------------

  std::string svgescape(const std::string & theURI)
  {
	using boost::algorithm::replace_all;
	using boost::algorithm::replace_all_copy;

	std::string ret = replace_all_copy(theURI,"&","&amp;");

	replace_all(ret,
				"http://kopla.fmi.fi/serve-meteogram.php?set=2&amp;symbol=",
				"file:///home/mheiskan/mirwa/2/");

	replace_all(ret,
				"http://kopla.fmi.fi/serve-meteogram.php?set=1&amp;symbol=",
				"file:///home/mheiskan/mirwa/1/");
								  
	replace_all(ret,"&amp;width=xxx&amp;height=xxx",".svg");

	return ret;

  }

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

	if(textlength > 0)
	  {
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

SvgRenderer::SvgRenderer(const Options & theOptions,
						 const libconfig::Config & theConfig,
						 const std::string & theTemplate,
						 const boost::shared_ptr<NFmiArea> & theArea)
  : options(theOptions)
  , config(theConfig)
  , svgbase(theTemplate)
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

std::string SvgRenderer::svg() const
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

  // BOOST_FOREACH does not work nicely with ptr_map

  for(Contours::const_iterator it=contours.begin(); it!=contours.end(); ++it)
	replace_all(ret,it->first, it->second->str());

  return ret;

}

// ----------------------------------------------------------------------
/*!
 * \brief Render a CloudAreaBorder
 */
// ----------------------------------------------------------------------

void
SvgRenderer::visit(const woml::CloudAreaBorder & theFeature)
{
  if(options.debug)	std::cerr << "Visiting CloudAreaBorder" << std::endl;

  ++ncloudborders;
  std::string id = "cloudborder" + boost::lexical_cast<std::string>(ncloudborders);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = getCssSize("cloudborderglyph","font-size");

  paths << "<path id=\"" << id << "\" d=\"" << path.svg() << "\"/>\n";
	
  cloudborders << "<use class=\"cloudborder\" xlink:href=\"#"
			   << id
			   << "\"/>\n";
	
  double factor = 2; // the required math here is unclear
  double len = path.length();
  int chars = static_cast<int>(std::ceil(factor*len/fontsize));

  cloudborders << "<text><textPath class=\"cloudborderglyph\" xlink:href=\"#"
			   << id
			   << "\">\n";

  for(int i=0; i<chars; ++i)
	cloudborders << "U";

  cloudborders << "</textPath>\n</text>\n";
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a ColdFront
 */
// ----------------------------------------------------------------------

void
SvgRenderer::visit(const woml::ColdFront & theFeature)
{
  if(options.debug)	std::cerr << "Visiting ColdFront" << std::endl;

  ++ncoldfronts;
  std::string id = "coldfront" + boost::lexical_cast<std::string>(ncoldfronts);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = getCssSize("coldfrontglyph","font-size");
  double spacing = lookup<double>(config,"coldfront.letter-spacing");

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
SvgRenderer::visit(const woml::Jet & theFeature)
{
  if(options.debug)	std::cerr << "Visiting Jet" << std::endl;

  ++njets;

  std::string id = "jet" + boost::lexical_cast<std::string>(njets);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = 0;
  double spacing = 0;

  render_front(path,paths,jets,id,
			   "jet","",
			   "","",
			   fontsize,spacing);
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a OccludedFront
 */
// ----------------------------------------------------------------------

void
SvgRenderer::visit(const woml::OccludedFront & theFeature)
{
  if(options.debug)	std::cerr << "Visiting OccludedFront" << std::endl;

  ++noccludedfronts;
  std::string id = "occludedfront" + boost::lexical_cast<std::string>(noccludedfronts);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = getCssSize("occludedfrontglyph","font-size");
  double spacing = lookup<double>(config,"occludedfront.letter-spacing");

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
SvgRenderer::visit(const woml::PointGeophysicalParameterValueSet & theFeature)
{
  if(options.debug)	std::cerr << "Visiting PointgeophysicalParameterValueSet" << std::endl;

  ++npointvalues;

  // Find the pixel coordinate
  PathProjector proj(area);
  double lon = theFeature.point().lon();
  double lat = theFeature.point().lat();
  proj(lon,lat);

  boost::shared_ptr<woml::GeophysicalParameterValueSet> params = theFeature.parameters();

  BOOST_FOREACH(const woml::GeophysicalParameterValue & value, params->values())
	{
	  pointvalues << "<text class=\""
				  << value.parameter().name() << "value"
				  << "\" x=\""
				  << std::fixed << std::setprecision(1) << lon
				  << "\" y=\""
				  << std::fixed << std::setprecision(1) << lat
				  << "\">"
				  << std::fixed << std::setprecision(0) << value.value()
				  << "</text>\n";
	}
  BOOST_FOREACH(const woml::GeophysicalParameterValueRange & range, params->ranges())
	{
	  pointvalues << "<text class=\""
				  << range.parameter().name() << "range"
				  << "\" x=\""
				  << std::fixed << std::setprecision(1) << lon
				  << "\" y=\""
				  << std::fixed << std::setprecision(1) << lat
				  << "\">"
				  << std::fixed << std::setprecision(0) << range.lowerLimit()
				  << "..."
				  << std::fixed << std::setprecision(0) << range.upperLimit()
				  << "</text>\n";
	}
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a PointMeteorologicalSymbol
 */
// ----------------------------------------------------------------------

void
SvgRenderer::visit(const woml::PointMeteorologicalSymbol & theFeature)
{
  if(options.debug)	std::cerr << "Visiting PointMeteorologicalSymbol" << std::endl;

  ++npointsymbols;

  // Find the pixel coordinate
  PathProjector proj(area);
  double lon = theFeature.point().lon();
  double lat = theFeature.point().lat();
  proj(lon,lat);

  // Draw the symbol

  boost::shared_ptr<woml::MeteorologicalSymbol> symb = theFeature.symbol();

  // dynamic_cast to pointer reuturns NULL on failure

  woml::FontSymbol * fs = dynamic_cast<woml::FontSymbol *>(symb.get());
  if(fs != NULL)
	{
	  std::string class1 = fs->fontName() + "symbol";
	  std::string class2 = class1 + boost::lexical_cast<std::string>(fs->symbolIndex());

	  std::string id = "symbol" + boost::lexical_cast<std::string>(npointsymbols);

	  pointsymbols << "<text class=\""
				   << class1
				   << ' '
				   << class2
				   << "\" id=\""
				   << id
				   << "\" x=\""
				   << std::fixed << std::setprecision(1) << lon
				   << "\" y=\""
				   << std::fixed << std::setprecision(1) << lat
				   << "\">&#"
				   << fs->symbolIndex()
				   << ";</text>\n";
	  return;
	}

  woml::GraphicSymbol * gs = dynamic_cast<woml::GraphicSymbol *>(symb.get());
  if(gs != NULL)
	{
	  const woml::URIList & uris = gs->URIs();

	  double sz = getCssSize("graphicsymbol","fmi-size") * gs->scaleFactor();

	  BOOST_FOREACH(const std::string & uri, uris)
		{
		  pointsymbols << "<image xlink:href=\""
					   << svgescape(uri)
					   << "\" x=\""
					   << std::fixed << std::setprecision(1) << (lon-sz/2)
					   << "\" y=\""
					   << std::fixed << std::setprecision(1) << (lat-sz/2)
					   << "\" width=\""
					   << std::fixed << std::setprecision(1) << sz
					   << "px\" height=\""
					   << std::fixed << std::setprecision(1) << sz
					   << "\"/>\n";
		}
	  return;
	}

  throw std::runtime_error("Unknown MeteorologicalSymbol encountered, only FontSymbol and GraphicSymbol are currently known");

}

// ----------------------------------------------------------------------
/*!
 * \brief Render a PointNote
 */
// ----------------------------------------------------------------------

void
SvgRenderer::visit(const woml::PointNote & theFeature)
{
  if(options.debug)	std::cerr << "Visiting PointNote" << std::endl;

  ++npointnotes;
  if(options.verbose)
	std::cerr << "Skipping PointNote " << npointnotes << std::endl;
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a SurfacePrecipitationArea
 */
// ----------------------------------------------------------------------

void
SvgRenderer::visit(const woml::SurfacePrecipitationArea & theFeature)
{
  if(options.debug)	std::cerr << "Visiting SurfacePrecipitationArea" << std::endl;

  ++nprecipitationareas;
  std::string id = "precipitation" + boost::lexical_cast<std::string>(nprecipitationareas);
  woml::RainPhase phase = theFeature.rainPhase();

  const woml::CubicSplineSurface surface = theFeature.controlSurface();

  Path path = PathFactory::create(surface);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = 0;
  double spacing = 0;

  switch(phase)
	{
	case woml::unknown:
	  render_surface(path,paths,masks,precipitationareas,id,
				 "precipitation","","","",fontsize,spacing);
	  break;
	case woml::rain:
	  render_surface(path,paths,masks,precipitationareas,id,
				 "rain","","","",fontsize,spacing);
	  break;
	case woml::snow:
	  render_surface(path,paths,masks,precipitationareas,id,
				 "snow","","","",fontsize,spacing);
	  break;
	case woml::fog:
	  render_surface(path,paths,masks,precipitationareas,id,
				 "fog","","","",fontsize,spacing);
	  break;
	case woml::sleet:
	  render_surface(path,paths,masks,precipitationareas,id,
				 "sleet","","","",fontsize,spacing);
	  break;
	case woml::hail:
	  render_surface(path,paths,masks,precipitationareas,id,
				 "hail","","","",fontsize,spacing);
	  break;
	case woml::freezing_precipitation:
	  render_surface(path,paths,masks,precipitationareas,id,
				 "freezing_precipitation","","","",fontsize,spacing);
	  break;
	case woml::drizzle:
	  render_surface(path,paths,masks,precipitationareas,id,
				 "drizzle","","","",fontsize,spacing);
	  break;
	case woml::mixed:
	  render_surface(path,paths,masks,precipitationareas,id,
				 "mixed","","","",fontsize,spacing);
	  break;
	}
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a Trough
 */
// ----------------------------------------------------------------------

void
SvgRenderer::visit(const woml::Trough & theFeature)
{
  if(options.debug)	std::cerr << "Visiting Trough" << std::endl;

  ++ntroughs;

  std::string id = "trough" + boost::lexical_cast<std::string>(ntroughs);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = getCssSize("troughglyph","font-size");
  double spacing = lookup<double>(config,"trough.letter-spacing");

  render_front(path,paths,troughs,id,
			   "trough","troughglyph",
			   "t","T",
			   fontsize,spacing);

}

// ----------------------------------------------------------------------
/*!
 * \brief Render a UpperTrough
 */
// ----------------------------------------------------------------------

void
SvgRenderer::visit(const woml::UpperTrough & theFeature)
{
  if(options.debug)	std::cerr << "Visiting UpperTrough" << std::endl;

  ++nuppertroughs;

  std::string id = "uppertrough" + boost::lexical_cast<std::string>(nuppertroughs);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = getCssSize("uppertroughglyph","font-size");
  double spacing = lookup<double>(config,"uppertrough.letter-spacing");

  render_front(path,paths,uppertroughs,id,
			   "uppertrough","uppertroughglyph",
			   "t","T",
			   fontsize,spacing);

}

// ----------------------------------------------------------------------
/*!
 * \brief Render a WarmFront
 */
// ----------------------------------------------------------------------

void
SvgRenderer::visit(const woml::WarmFront & theFeature)
{
  if(options.debug)	std::cerr << "Visiting WarmFront" << std::endl;

  ++nwarmfronts;
  std::string id = "warmfront" + boost::lexical_cast<std::string>(nwarmfronts);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = getCssSize("warmfrontglyph","font-size");
  double spacing = lookup<double>(config,"warmfront.letter-spacing");

  render_front(path,paths,warmfronts,id,
			   "warmfront","warmfrontglyph",
			   "W","W",
			   fontsize,spacing);
}

// ----------------------------------------------------------------------
/*!
 * \brief Test if the given CSS class exists
 */
// ----------------------------------------------------------------------

bool SvgRenderer::hasCssClass(const std::string & theCssClass) const
{
  return (svgbase.find("."+theCssClass) != std::string::npos);
}

// ----------------------------------------------------------------------
/*!
 * \brief Find the given CSS size setting
 */
// ----------------------------------------------------------------------

double SvgRenderer::getCssSize(const std::string & theCssClass,
							   const std::string & theAttribute)
{
  std::string msg = "CSS setting ." + theCssClass + " { " + theAttribute + " } is missing";

  std::string::size_type pos = svgbase.find("." + theCssClass);
  if(pos == std::string::npos)
	throw std::runtime_error(msg);

  // Require attribute to come before next "}"

  std::string::size_type pos2 = svgbase.find("}", pos);
  if(pos2 == std::string::npos)
	throw std::runtime_error(msg);

  std::string::size_type pos1 = svgbase.find(theAttribute+":", pos);
  if(pos1 == std::string::npos || pos1 > pos2)
	throw std::runtime_error(msg);

  // Extract the next word after "attribute:"
  
  pos1 += theAttribute.size()+1;
  pos2 = svgbase.find(";",pos1);

  std::string word = svgbase.substr(pos1,pos2-pos1);
  boost::algorithm::trim(word);

  if(word.substr(word.size()-2,2) != "px")
	throw std::runtime_error("CSS value for "+theCssClass+"."+theAttribute+" must be in pixels");

  std::string num = word.substr(0,word.size()-2);

  return Fmi::number_cast<double>(num);
}

// ----------------------------------------------------------------------
/*
 * TODO: This part should be refactored when finished
 */
// ----------------------------------------------------------------------

#include <smartmet/tron/Tron.h>
#include <smartmet/newbase/NFmiDataMatrix.h>
#include <smartmet/tron/MirrorMatrix.h>
#include <smartmet/tron/SavitzkyGolay2D.h>

class DataMatrixAdapter
{
public:
  typedef float value_type;
  typedef double coord_type;
  typedef NFmiDataMatrix<value_type>::size_type size_type;

  DataMatrixAdapter(const NFmiDataMatrix<value_type> & theMatrix,
					const NFmiDataMatrix<NFmiPoint> & theCoordinates)
	: itsMatrix(theMatrix)
	, itsCoordinates(theCoordinates)
  { }

  const value_type & operator()(size_type i, size_type j) const
  { return itsMatrix[i][j]; }

  value_type & operator()(size_type i, size_type j)
  { return itsMatrix[i][j]; }

  coord_type x(size_type i, size_type j) const
  { return itsCoordinates[i][j].X(); }

  coord_type y(size_type i, size_type j) const
  { return itsCoordinates[i][j].Y(); }

  size_type width()  const { return itsMatrix.NX(); }
  size_type height() const { return itsMatrix.NY(); }

  void swap(DataMatrixAdapter & other)
  {
	itsMatrix.swap(other.itsMatrix);
	itsCoordinates.swap(other.itsCoordinates);
  }

  void make_nan_missing()
  {
	for(size_type j=0; j<height(); ++j)
	  for(size_type i=0; i<width(); ++i)
		if(itsMatrix[i][j] == kFloatMissing)
		  itsMatrix[i][j] = std::numeric_limits<float>::quiet_NaN();
  }

private:

  DataMatrixAdapter();
  NFmiDataMatrix<float> itsMatrix;
  NFmiDataMatrix<NFmiPoint> itsCoordinates;

};

typedef Tron::Traits<float,float,Tron::NanMissing> MyTraits;

typedef Tron::Contourer<DataMatrixAdapter,
                        Path,
                        MyTraits,
                        Tron::LinearInterpolation> MyContourer;

typedef Tron::Hints<DataMatrixAdapter,MyTraits> MyHints;

// ----------------------------------------------------------------------
/*!
 * \brief Render the contours
 */
// ----------------------------------------------------------------------

void
SvgRenderer::contour(const boost::shared_ptr<NFmiQueryData> & theQD,
					 const boost::posix_time::ptime & theTime)
{

  // Fast access iterator

  boost::shared_ptr<NFmiFastQueryInfo> qi(new NFmiFastQueryInfo(theQD.get()));

  // newbase time

  NFmiMetTime validtime = to_mettime(theTime);

  // Coordinates

  NFmiDataMatrix<NFmiPoint> coordinates;
  qi->LocationsXY(coordinates,*area);

  // Parameter identification

  NFmiEnumConverter enumconverter;

  // Contour lines

  if(config.exists("contourlines"))
	{
	  // Except list of groups

	  const char * typemsg = "contourlines must contain a list of groups in parenthesis";

	  const libconfig::Setting & contourspecs = config.lookup("contourlines");

	  if(!contourspecs.isList())
		throw std::runtime_error(typemsg);

	  std::size_t linenumber = 0;

	  for(int i=0; i<contourspecs.getLength(); ++i)
		{
		  const libconfig::Setting & specs = contourspecs[i];

		  if(!specs.isGroup())
			throw std::runtime_error(typemsg);

		  // Now process this individual contour

		  std::string paramname = lookup<std::string>(specs,"contourlines","parameter");
		  std::string classname = lookup<std::string>(specs,"contourlines","class");
		  std::string outputname = lookup<std::string>(specs,"contourlines","output");
		  std::string smoother = lookup<std::string>(specs,"contourlines","smoother");

		  double start = lookup<double>(specs,"contourlines","start");
		  double stop  = lookup<double>(specs,"contourlines","stop");
		  double step  = lookup<double>(specs,"contourlines","step");

		  if(options.verbose)
			{
			  std::cerr << "Contourline "
						<< paramname
						<< " class="
						<< classname
						<< " to "
						<< outputname
						<< " range "
						<< start
						<< "..."
						<< stop
						<< " in step "
						<< step
						<< std::endl;
			}

		  FmiParameterName param = FmiParameterName(enumconverter.ToEnum(paramname));
		  if(param == kFmiBadParameter)
			throw std::runtime_error("Unknown parameter name '"
									 + paramname
									 + "' requested for contouring");
		  
		  if(!qi->Param(param))
			throw std::runtime_error("Parameter '"
									 + paramname
									 + "' is not available in the referenced numerical model");


		  // Get data values and replace kFmiMissing with NaN

		  NFmiDataMatrix<float> matrix;
		  qi->Values(matrix,validtime);

		  // Adapt for contouring

		  DataMatrixAdapter grid(matrix,coordinates);
		  grid.make_nan_missing();

		  if(smoother == "none")
			;
		  else if(smoother == "Savitzky-Golay")
			{
			  int window = lookup<int>(specs,"contourlines","smoother-size");
			  int degree = lookup<int>(specs,"contourlines","smoother-degree");
			  if(options.verbose)
				std::cerr << "Savitzky-Golay smoothing of size "
						  << window
						  << " of degree "
						  << degree
						  << std::endl;
			  Tron::SavitzkyGolay2D::smooth(grid,window,degree);
			}
		  else
			throw std::runtime_error("Unknown smoother name '"+smoother+"'");

		  MyHints hints(grid);

		  for(double value=start; value<=stop; value+=step)
			{
			  Path path;
			  MyContourer::line(path,grid,value,hints);

			  if(!path.empty())
				{
				  ++linenumber;
				  std::string id = ("contourline"
									+ boost::lexical_cast<std::string>(linenumber));
				  paths << "<path id=\""
						<< id
						<< "\" d=\""
						<< path.svg()
						<< "\"/>\n";


				  contours[outputname] << "<use class=\""
									   << classname
									   << "\" xlink:href=\"#"
									   << id
									   << "\"/>\n";
				}
			}

		}

	}

}

} // namespace frontier
