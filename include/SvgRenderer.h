// ======================================================================
/*!
 * \brief frontier::SvgRenderer
 */
// ======================================================================

#ifndef FRONTIER_SVGRENDERER_H
#define FRONTIER_SVGRENDERER_H

#include "Options.h"
#include "Path.h"
#include "BezSeg.h"

#include "smartmet/woml/FeatureVisitor.h"
#include <smartmet/woml/Point.h>
#include <smartmet/woml/MeasureValue.h>
#include <smartmet/woml/GeophysicalParameterValueSet.h>
#include <smartmet/woml/TimeSeriesSlot.h>

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

  class Elevation
  {
  public:
	Elevation(double theElevation,double theScale,int theScaledElevation,const std::string & theLLabel,const std::string & theRLabel);
	Elevation(double theElevation);

	bool operator < (const Elevation & theOther) const;

	double elevation() const { return itsElevation; }
	double scale() const { return itsScale; }
	int scaledElevation() const { return itsScaledElevation; }
	const std::string & lLabel() const { return itsLLabel; }
	const std::string & rLabel() const { return itsRLabel; }
	void factor(double theFactor) { itsFactor = theFactor; }
	double factor() const { return itsFactor; }

  private:
	double itsElevation;
	double itsScale;
	int itsScaledElevation;
	std::string itsLLabel;
	std::string itsRLabel;
	double itsFactor;		// Factor for linear interpolation between this and the next elevation
  };

  class AxisManager
  {
  public:
	AxisManager(const libconfig::Config & config);

	// Y -axis height
	double axisHeight() const { return itsAxisHeight; }

	// X -axis width
	void axisWidth(double theWidth) { itsAxisWidth = theWidth; }
	double axisWidth() const { return itsAxisWidth; }

	// Time period
	void timePeriod(const boost::posix_time::time_period & theTimePeriod) { itsTimePeriod = theTimePeriod; }
	const boost::posix_time::time_period & timePeriod() { return itsTimePeriod; }

	// X -axis offset
	double xOffset(const boost::posix_time::ptime & validTime) const;

	// X -axis step
	double xStep() const;

	// utc
	void utc(bool theUtc) { itsUtc = theUtc; }
	bool utc() const { return itsUtc; }

	// Elevations
	const std::list<Elevation> & elevations() { return itsElevations; }

	// Scaled elevation
	double scaledElevation(double elevation,bool * above = NULL,double belowZero = 50.0,double aboveTop = 100.0);

	// Min elevation value to be taken as nonzero
	double nonZeroElevation() { return 10.0; }

  private:
	double itsAxisHeight;
	double itsMinElevation;
	double itsMaxElevation;
	double itsAxisWidth;
	bool itsUtc;
	boost::posix_time::time_period itsTimePeriod;

	std::list<Elevation> itsElevations;
  };

  class CloudGroup
  {
  public:
	CloudGroup(const std::string & theClassDef,
			   const std::string & theCloudTypes,
			   const std::string & theSymbolType,
			   const std::string * theLabel,
			   bool bbCenterLabel,
			   const std::string & thePlaceHolder,
			   const std::string & theTextPlaceHolder,
			   bool combined,
			   unsigned int theBaseStep,
			   unsigned int theMaxRand,
			   unsigned int theMaxRepeat,
			   int theScaleHeightMin,
			   int theScaleHeightRandom,
			   int theControlMin,
			   int theControlRandom,
			   std::set<size_t> & theCloudSet,
			   const libconfig::Setting * localScope,
			   const libconfig::Setting * globalScope = NULL
			   );

	const std::string & classDef() const { return itsClass; }
	const std::string & textClassDef() const { return itsTextClass; }
	const std::string & symbolType() const { return itsSymbolType; }
	std::string label() const { return (hasLabel ? itsLabel : cloudTypes()); }
	bool bbCenterLabel() const { return bbCenterLabelPos; }
	const std::string & placeHolder() const { return itsPlaceHolder; }
	const std::string & labelPlaceHolder() const { return itsLabelPlaceHolder; }
	bool standalone() const { return itsStandalone; }
	bool contains(const std::string & theCloudType) const;
	void addType(const std::string & type) const;
	std::string cloudTypes() const;

	unsigned int baseStep() const { return itsBaseStep; }
	unsigned int maxRand() const { return itsMaxRand; }
	unsigned int maxRepeat() const { return itsMaxRepeat; }

	int scaleHeightMin() const { return itsScaleHeightMin; }
	int scaleHeightRandom() const { return itsScaleHeightRandom; }
	int controlMin() const { return itsControlMin; }
	int controlRandom() const { return itsControlRandom; }

	std::set<size_t> & cloudSet() const { return itsCloudSet; }

	const libconfig::Setting * localScope() const { return itsLocalScope; }
	const libconfig::Setting * globalScope() const { return itsGlobalScope; }

  private:
	std::string itsClass;
	std::string itsTextClass;
	std::string itsCloudTypes;
	std::string itsSymbolType;
	std::string itsLabel;
	bool hasLabel;
	bool bbCenterLabelPos;
	std::string itsPlaceHolder;
	std::string itsLabelPlaceHolder;
	bool itsStandalone;

	unsigned int itsBaseStep;
	unsigned int itsMaxRand;
	unsigned int itsMaxRepeat;

	int itsScaleHeightMin;
	int itsScaleHeightRandom;
	int itsControlMin;
	int itsControlRandom;

	std::set<size_t> & itsCloudSet;				// Cloud types contained in the current cloud

	const libconfig::Setting * itsLocalScope;	// Group's configuration block
	const libconfig::Setting * itsGlobalScope;	// Group's global configuration block (if any)
  };

  struct CloudType : public std::binary_function< CloudGroup, std::string, bool > {
    bool operator () ( const CloudGroup & cloudGroup, const std::string & cloudType) const;
  };

  class CategoryValueMeasureGroup
  {
  public:
	virtual bool groupMember(bool first,const woml::CategoryValueMeasure * cvm) = 0;
	virtual bool standalone() = 0;
  };

  class CloudGroupCategory : public CategoryValueMeasureGroup
  {
  public:
	CloudGroupCategory();

	std::list<CloudGroup> & cloudGroups() { return itsCloudGroups; }
	std::list<CloudGroup>::const_iterator currentGroup() { return itcg; }

	bool groupMember(bool first,const woml::CategoryValueMeasure * cvm);
	bool standalone() { return ((itsCloudGroups.size() > 0) && (*itcg).standalone()); }

  private:
	std::list<CloudGroup> itsCloudGroups;
	std::list<CloudGroup>::const_iterator itcg;
  };

  class IcingCategory : public CategoryValueMeasureGroup
  {
  public:
	IcingCategory();

	const std::string & category() { return itsCategory; }

	bool groupMember(bool first,const woml::CategoryValueMeasure * cvm);
	bool standalone() { return false; }

  private:
	std::string itsCategory;
  };

  class ElevationGroupItem
  {
  public:
	ElevationGroupItem(boost::posix_time::ptime theValidTime,
					   const std::list<boost::shared_ptr<woml::GeophysicalParameterValueSet> >::const_iterator & thePvs,
					   const std::list<woml::GeophysicalParameterValue>::iterator & thePv);

	const boost::posix_time::ptime & validTime() const { return itsValidTime; }
	const std::list<boost::shared_ptr<woml::GeophysicalParameterValueSet> >::const_iterator & Pvs() const { return itsPvs; }
	const std::list<woml::GeophysicalParameterValue>::iterator & Pv() const { return itsPv; }
	void topConnected(bool connected) { itsTopConnected = connected; }
	bool topConnected() const { return itsTopConnected; }
	void bottomConnected(bool connected) { itsBottomConnected = connected; }
	bool bottomConnected() const { return itsBottomConnected; }
	void leftOpen(bool open) { itsLeftOpen = open; }
	bool leftOpen() const { return itsLeftOpen; }
	void bottomConnection(woml::Elevation theBottomConnection) { itsBottomConnection = theBottomConnection; }
	const boost::optional<woml::Elevation> & bottomConnection() const { return itsBottomConnection; }
	void groundConnected(bool connected);
	bool groundConnected() const;
	void negativeTemp(bool negative);
	bool negativeTemp() const;
	void generated(bool _generated) { itsGenerated = _generated; }
	bool generated() const { return itsGenerated; }
	void elevation(boost::optional<woml::Elevation> & theElevation) { itsElevation = theElevation; }
	const woml::Elevation & elevation() const { return (itsElevation ? *itsElevation : itsPv->elevation()); }

	enum elevationFlags { b_groundConnected = 1, b_negative = (1 << 1) };

  private:
	boost::posix_time::ptime itsValidTime;
	std::list<boost::shared_ptr<woml::GeophysicalParameterValueSet> >::const_iterator itsPvs;
	std::list<woml::GeophysicalParameterValue>::iterator itsPv;
	bool itsTopConnected;
	bool itsBottomConnected;
	bool itsLeftOpen;
	boost::optional<woml::Elevation> itsBottomConnection;	// Rigth side elevation
	bool itsGenerated;										// Set for generated (below 0) elevation
	boost::optional<woml::Elevation> itsElevation;			// ZeroTolerance; part of the original elevation cut into 2 pieces or generated ground elevation
  };

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

	virtual void visit(const woml::CloudLayers & theFeature);
	virtual void visit(const woml::Contrails & theFeature);
	virtual void visit(const woml::Icing & theFeature);
	virtual void visit(const woml::MigratoryBirds & theFeature);
	virtual void visit(const woml::SurfaceVisibility & theFeature);
	virtual void visit(const woml::SurfaceWeather & theFeature);
	virtual void visit(const woml::Winds & theFeature);
	virtual void visit(const woml::ZeroTolerance & theFeature);

#ifdef __contouring__
	void contour(const boost::shared_ptr<NFmiQueryData> & theQD,
				 const boost::posix_time::ptime & theTime);
#endif

	void render_header(boost::posix_time::ptime & validTime,
					   const boost::posix_time::time_period & timePeriod,
					   const boost::posix_time::ptime & forecastTime,
					   const boost::posix_time::ptime & creationTime,
					   const boost::optional<boost::posix_time::ptime> & modificationTime,
					   const std::string & regionName,
					   const std::string & regionId
					  );

	std::string svg() const;

  private:

	SvgRenderer();

	bool hasCssClass(const std::string & theCssClass) const;

	double getCssSize(const std::string & theCssClass,
					  const std::string & theAttribute,
					  double defaultValue = 30);

	void render_header(const std::string & hdrClass,
					   const boost::posix_time::ptime & datetime,
					   const std::string & text = "",
					   const std::string & confPath = "Header");
	void render_text(const std::string & confPath,
					 const std::string & textName,
					 const std::string & text);
	void render_surface(const Path & path,
						std::ostringstream & surfaces,
						const std::string & id,
						const std::string & surfaceName);
	void render_aerodromeSymbol(const std::string & confPath,
								const std::string & symClass,
								const std::string & classNameExt,
								const std::string & value,
								double x,double y,
								bool codeValue = true);
	template <typename T> void render_aerodromeSymbols(const T & theFeature,
												       const std::string & confPath);
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

	typedef enum { fst, fwd, vup, eup, bwd, vdn, edn, lst } Phase;
	typedef std::list<ElevationGroupItem> ElevGrp;
	typedef enum { t_ground, t_nonground, t_mixed } GroupType;
	GroupType elevationGroup(const std::list<woml::TimeSeriesSlot> & ts,
							 const boost::posix_time::ptime & bt,const boost::posix_time::ptime & et,
							 ElevGrp & eGrp,
							 bool all = false,
							 bool mixed = true,
							 CategoryValueMeasureGroup * groupCategory = NULL);
	Phase uprightdown(ElevGrp & eGrp,ElevGrp::iterator & iteg,double lo,double hi,bool nonGndFwd2Gnd = true);
	Phase downleftup(ElevGrp & eGrp,ElevGrp::iterator & iteg,double lo,double hi,bool nonGndVdn2Gnd = false);

	template <typename T> void render_aerodrome(const T & theFeature);
	void render_aerodromeFrame(const boost::posix_time::time_period & theTimePeriod);
	void render_elevationAxis();
	void render_timeAxis(const boost::posix_time::time_period & theTimePeriod);
	const libconfig::Setting & cloudLayerConfig(const std::string & confPath,
	  	  	  	  	  	  	  	  	  	  	  	double & tightness,
												std::list<CloudGroup> & cloudGroups,
												std::set<size_t> & cloudSet);
	void render_cloudSymbol(const std::string & confPath,
							const CloudGroup & cg,
							int nGroups,
							double x,
							double lopx,double hipx);
	void render_cloudSymbols(const std::string confPath,
							 const ElevGrp & eGrp,
							 std::list<CloudGroup>::const_iterator itcg,
							 int nGroups);
	bool scaledCurvePositions(ElevGrp & eGrp,
							  List<DirectPosition> & curvePositions,
							  std::vector<double> & scaledLo,std::vector<double> & scaledHi,
							  std::ostringstream & path,
							  bool * isVisible = NULL,
							  bool checkGround = false,
							  bool nonGndFwd2Gnd = true,
							  bool nonGndVdn2Gnd = false
							 );
	void render_timeserie(const woml::CloudLayers & cloudlayers);
	void render_timeserie(const woml::Contrails & contrails);
	void render_timeserie(const woml::Icing & icing);
	void render_timeserie(const woml::MigratoryBirds & migratorybirds);
	void render_timeserie(const woml::SurfaceVisibility & surfacevisibility);
	void render_timeserie(const woml::SurfaceWeather & surfaceweather);
	void render_timeserie(const woml::Winds & winds);
	void checkZeroToleranceGroup(ElevGrp & eGrpIn,ElevGrp & eGrpOut,bool mixed = true);
	void render_timeserie(const woml::ZeroTolerance & zerotolerance);

	const Options & options;
	const libconfig::Config & config;
	std::string svgbase;
	boost::shared_ptr<NFmiArea> area;
	const boost::posix_time::ptime validtime;
	std::ostringstream _debugoutput;
	boost::shared_ptr<AxisManager> axisManager;
	bool initAerodrome;

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

	// For text and various stuff
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
