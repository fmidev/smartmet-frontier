// ======================================================================
/*!
 * \brief frontier::SvgRenderer
 */
// ======================================================================

#ifndef FRONTIER_SVGRENDERER_H
#define FRONTIER_SVGRENDERER_H

#include "Options.h"
#include "Path.h"

#include "smartmet/woml/FeatureVisitor.h"
#include <smartmet/woml/Point.h>
#include <smartmet/woml/MeasureValue.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/shared_ptr.hpp>

#include <libconfig.h++>

#include <sstream>
#include <string>

class NFmiArea;
class NFmiQueryData;

namespace frontier
{
  class SvgRenderer : public woml::FeatureVisitor
  {
  public:

	virtual ~SvgRenderer() {}
	SvgRenderer(const Options & theOptions,
				const libconfig::Config & theConfig,
				const std::string & theTemplate,
				const boost::shared_ptr<NFmiArea> & theArea,
				const boost::posix_time::ptime & theValidTime,
				std::ostringstream * theDebugOutput = NULL);

    virtual void visit(const woml::CloudArea & theFeature);
    virtual void visit(const woml::ColdAdvection & theFeature);
    virtual void visit(const woml::ColdFront & theFeature);
    virtual void visit(const woml::JetStream & theFeature);
    virtual void visit(const woml::OccludedFront & theFeature);
    virtual void visit(const woml::PointMeteorologicalSymbol & theFeature);
    virtual void visit(const woml::Ridge & theFeature);
    virtual void visit(const woml::SurfacePrecipitationArea & theFeature);
    virtual void visit(const woml::TimeGeophysicalParameterValueSet & theFeature);
    virtual void visit(const woml::Trough & theFeature);
    virtual void visit(const woml::UpperTrough & theFeature);
    virtual void visit(const woml::WarmAdvection & theFeature);
    virtual void visit(const woml::WarmFront & theFeature);

	virtual void visit(const woml::AntiCyclone & theFeature);
	virtual void visit(const woml::Antimesocyclone & theFeature);
	virtual void visit(const woml::Cyclone & theFeature);
	virtual void visit(const woml::HighPressureCenter & theFeature);
	virtual void visit(const woml::LowPressureCenter & theFeature);
	virtual void visit(const woml::Mesocyclone & theFeature);
	virtual void visit(const woml::Mesolow & theFeature);
	virtual void visit(const woml::ParameterValueSetPoint & theFeature);
	virtual void visit(const woml::PolarCyclone & theFeature);
	virtual void visit(const woml::PolarLow & theFeature);
	virtual void visit(const woml::TropicalCyclone & theFeature);
	virtual void visit(const woml::ConvectiveStorm & theFeature);
	virtual void visit(const woml::Storm & theFeature);

	virtual void visit(const woml::InfoText & theFeature);

//	void contour(const boost::shared_ptr<NFmiQueryData> & theQD,
//				 const boost::posix_time::ptime & theTime);

	std::string svg() const;

  private:

	SvgRenderer();

	bool hasCssClass(const std::string & theCssClass) const;

	double getCssSize(const std::string & theCssClass,
					  const std::string & theAttribute,
					  double defaultValue = 30);

	void render_header(const std::string & confPath,
					   const std::string & hdrClass);
	void render_text(const std::string & confPath,
					 const std::string & textName,
					 const std::string & text);
	void render_surface(const Path & path,
						std::ostringstream & surfaces,
						const std::string & id,
						const std::string & surfaceName);
	void render_symbol(const std::string & path,
					   const std::string & symClass,
					   const std::string & symCode,
					   double lon,double lat,
					   NFmiFillPositions * fpos = NULL);
	void render_value(const std::string & path,
					  const std::string & symClass,
					  const woml::NumericalSingleValueMeasure * lowerLimit,
					  const woml::NumericalSingleValueMeasure * upperLimit,
					  double lon,
					  double lat);

	const Options & options;
	const libconfig::Config & config;
	std::string svgbase;
	boost::shared_ptr<NFmiArea> area;
	const boost::posix_time::ptime validtime;
	std::ostringstream _debugoutput;

	// defs
	std::ostringstream masks;
	std::ostringstream paths;

	// body
	std::ostringstream cloudareas;
	std::ostringstream coldadvections;
	std::ostringstream coldfronts;
	std::ostringstream jets;
	std::ostringstream longinfotext;
	std::ostringstream occludedfronts;
	std::ostringstream pointnotes;
	std::ostringstream pointsymbols;
	std::ostringstream pointvalues;
	std::ostringstream precipitationareas;
	std::ostringstream ridges;
	std::ostringstream shortinfotext;
	std::ostringstream troughs;
	std::ostringstream uppertroughs;
	std::ostringstream warmadvections;
	std::ostringstream warmfronts;
	std::ostringstream & debugoutput;
	typedef boost::ptr_map<std::string,std::ostringstream> Texts;
	Texts texts;

	typedef boost::ptr_map<std::string,std::ostringstream> Contours;
	Contours contours;

	int ncloudareas;
	int ncoldadvections;
	int ncoldfronts;
	int njets;
	int noccludedfronts;
	int npointnotes;
	int npointsymbols;
	int npointvalues;
	int nprecipitationareas;
	int nridges;
	int ntroughs;
	int nuppertroughs;
	int nwarmadvections;
	int nwarmfronts;

  }; // class SvgRenderer


} // namespace frontier

#endif // FRONTIER_SVGRENDERER_H
