// ======================================================================
/*!
 * \brief frontier::SvgRenderer
 */
// ======================================================================

#include "SvgRenderer.h"
#include "ConfigTools.h"
#include "PathFactory.h"
#include "PathTransformation.h"

#include "BezierModel.h"

#include <smartmet/woml/CubicSplineSurface.h>
#include <smartmet/woml/GeophysicalParameterValueSet.h>
#include <smartmet/woml/JetStream.h>
#include <smartmet/woml/OccludedFront.h>
#include <smartmet/woml/ParameterTimeSeriesPoint.h>

#include <smartmet/woml/ParameterValueSetPoint.h>
#include <smartmet/woml/PointMeteorologicalSymbol.h>
#include <smartmet/woml/PressureCenterType.h>
#include <smartmet/woml/StormType.h>

#include <smartmet/woml/CloudArea.h>
#include <smartmet/woml/SurfacePrecipitationArea.h>
#include <smartmet/woml/ParameterValueSetArea.h>

#include <smartmet/woml/InfoText.h>

#include <smartmet/newbase/NFmiArea.h>
#include <smartmet/newbase/NFmiStringTools.h>

#ifdef CONTOURING
#include <smartmet/tron/Tron.h>
#include <smartmet/newbase/NFmiDataMatrix.h>
#include <smartmet/tron/MirrorMatrix.h>
#include <smartmet/tron/SavitzkyGolay2D.h>
#include <smartmet/newbase/NFmiEnumConverter.h>
#include <smartmet/newbase/NFmiQueryData.h>
#include <smartmet/newbase/NFmiFastQueryInfo.h>
#endif

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/regex.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <list>

#include <cairo.h>

#include <smartmet/newbase/NFmiMetTime.h>
#include <smartmet/newbase/NFmiString.h>

namespace frontier
{
  const char * areaLabels = "AreaLabels";
  const size_t defaultSymbolWidth = 50;
  const size_t defaultSymbolHeight = 50;
  const double defaultSymbolPosHeightFactor = 1.0;
  const double defaultLabelPosHeightFactor = 1.0;
  const size_t labelPosHeightMin = 5;
  const double labelPosHeightFactorMin = 0.1;
  const double symbolPosHeightFactorMin = 0.1;
  const size_t fillAreaOverlapMax = 3;
  const size_t markerScaleFactorMin = 0.75;				// Minimum marker size 3/4'th of the original
  const size_t symbolBBoxFactorMin = 0.5;				// Minimum symbol bbox 0.5 * symbol width/height; symbols can overlap
  const size_t pathScalingSymbolHeightFactorMax = 5;	// Surface scaling max offset 5 * (symbol height / 2)

  // ----------------------------------------------------------------------
  /*!
   * \brief Convert ptime NFmiMetTime
   */
  // ----------------------------------------------------------------------

#ifdef CONTOURING

  NFmiMetTime to_mettime(const boost::posix_time::ptime  & theTime)
  {
	return NFmiMetTime(theTime.date().year(),
					   theTime.date().month(),
					   theTime.date().day(),
					   static_cast<short>(theTime.time_of_day().hours()),
					   static_cast<short>(theTime.time_of_day().minutes()),
					   static_cast<short>(theTime.time_of_day().seconds()),
					   1);
  }

#endif

  // ----------------------------------------------------------------------
  /*!
   * \brief Make URI valid for SVG
   */
  // ----------------------------------------------------------------------

  std::string svgescape(const std::string & theURI)
  {
	using boost::algorithm::replace_all_copy;

	std::string ret = replace_all_copy(theURI,"&","&amp;");

	return ret;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Make text valid for SVG
   */
  // ----------------------------------------------------------------------

  std::string svgescapetext(const std::string & theText,bool wrapScands = false)
  {
	using boost::algorithm::replace_all_copy;
	using boost::algorithm::replace_all;

	std::string ret = replace_all_copy(theText,"&","&amp;");
	replace_all(ret,"\"","&quot;");
	replace_all(ret,"'","&apos;");
	replace_all(ret,"<","&lt;");
	replace_all(ret,">","&gt;");

	if (wrapScands) {
		// ÄäÖöÅå
		//
		replace_all(ret,"\303\204","\304");
		replace_all(ret,"%C3%84","\304");
		replace_all(ret,"\304","\303\204");
		replace_all(ret,"\303\244","\344");
		replace_all(ret,"%C3%A4","\344");
		replace_all(ret,"\344","\303\244");
		replace_all(ret,"\303\226","\326");
		replace_all(ret,"%C3%96","\326");
		replace_all(ret,"\326","\303\226");
		replace_all(ret,"\303\266","\366");
		replace_all(ret,"%C3%B6","\366");
		replace_all(ret,"\366","\303\266");
		replace_all(ret,"\303\205","\305");
		replace_all(ret,"%C3%85","\305");
		replace_all(ret,"\305","\303\205");
		replace_all(ret,"\303\245","\345");
		replace_all(ret,"%C3%A5","\345");
		replace_all(ret,"\345","\303\245");
	}

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
	double textlength = 0.5*static_cast<double>((glyphs1.size() + glyphs2.size()));

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
   * \brief Utility function for reading configuration value from local or global scope
   */
  // ----------------------------------------------------------------------

  template <typename T>
  T _lookup(const libconfig::Setting & scope,
		  	const std::string & scopeName,
		  	const std::string & param,
		  	settings settingId = s_required,
		  	bool * isSet = NULL)
  {
	T value = lookup<T>(scope,scopeName,param,settingId,isSet);

	if (isSet && (*isSet)) {
		// Setting was found, check if it is overridden
		//
		bool _isSet;
		T _value = lookup<T>(scope,scopeName,param + "_",s_optional,&_isSet);

		if (_isSet)
			return _value;
	}

	return value;
  }

  template <typename T,typename T2>
  T configValue(const libconfig::Setting & localScope,
		  	    const std::string & localScopeName,
		  	    const std::string & param,
		  	    const libconfig::Setting * globalScope = NULL,
		  	    settings settingId = s_required,
		  	    bool *isSet = NULL,
		  	    const libconfig::Setting ** matchingScope = NULL)
  {
	// If 's_required' setting is not found, std::runtime_error is thrown.
	//
	// If 's_optional' setting is not found, unset (default constructor) value is returned and
	// 'isSet' flag (if available) is set to false.
	//
	// If specific ('s_base' + n) setting is not found, SettingIdNotFoundException is thrown.
	//
	// matchingScope (if available) is set to point the matching scope if setting is found.

    // Read the setting as optional. If the reading fails, try to read with secondary type

	const libconfig::Setting * scope = &localScope;
    bool _isSet;

	T value = _lookup<T>(localScope,localScopeName,param,s_optional,&_isSet);
    if ((!_isSet) && (typeid(T) != typeid(T2))) {
    	T2 value2 = _lookup<T2>(localScope,localScopeName,param,s_optional,&_isSet);

        if (_isSet)
        	value = boost::lexical_cast<T>(value2);
    }

    // Try global scope if available

    if ((!_isSet) && (scope = globalScope)) {
  	  value = _lookup<T>(*globalScope,localScopeName,param,s_optional,&_isSet);

      if ((!_isSet) && (typeid(T) != typeid(T2))) {
          T2 value2 = _lookup<T2>(*globalScope,localScopeName,param,s_optional,&_isSet);

          if (_isSet)
          	value = boost::lexical_cast<T>(value2);
      }
    }

    if (isSet)
  	  *isSet = _isSet;

    if (matchingScope)
        *matchingScope = (_isSet ? scope : NULL);

    if (_isSet || (settingId == s_optional))
    	return value;

    // Throw SettingIdNotFoundException for specific settings

    if (settingId != s_required)
      throw SettingIdNotFoundException(settingId,"Setting for " + localScopeName + "." + param + " is missing");

    throw std::runtime_error("Setting for " + localScopeName + "." + param + " is missing");
  }

  template <typename T>
  T configValue(const libconfig::Setting & localScope,
		  	    const std::string & localScopeName,
		  	    const std::string & param,
		  	    const libconfig::Setting * globalScope = NULL,
		  	    settings settingId = s_required,
		  	    bool *isSet = NULL,
		  	    const libconfig::Setting ** matchingScope = NULL)
  {
	return configValue<T,T>(localScope,localScopeName,param,globalScope,settingId,isSet,matchingScope);
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Utility function for reading configuration value from active scope
   */
  // ----------------------------------------------------------------------

  template <typename T,typename T2>
  T configValue(std::list<const libconfig::Setting *> & scope,
		  	    const std::string & localScopeName,
		  	    const std::string & param,
		  	    settings settingId = s_required,
		  	    bool *isSet = NULL,
		  	    bool truncate = false)
  {
	T value = T();
	const libconfig::Setting *matchingScope = NULL;
    bool _isSet = false;

	std::list<const libconfig::Setting *>::reverse_iterator rbegiter(scope.end()),renditer(scope.begin()),riter,it,git;

	for(riter=rbegiter; ((riter!=renditer) && (!_isSet)); ) {
		it = riter;
		riter++;
    	git = riter;

	    if (riter!=renditer)
			riter++;

	    // For the last block(s) use the given settingId; the call throws if the setting is required but not found
	    //
	    value = configValue<T,T2>(**it,localScopeName,param,((git!=renditer) ? *git : NULL),(riter!=renditer) ? s_optional : settingId,&_isSet,&matchingScope);
	}

    if (isSet)
      *isSet = _isSet;

    if (truncate && _isSet) {
    	// Truncate the scope to have the matching block as the last block
    	//
    	std::list<const libconfig::Setting *>::iterator fenditer = scope.end(),fiter = ((matchingScope == *it) ? it : git).base();
		scope.erase(fiter,fenditer);
    }

    return value;
  }

  template <typename T>
  T configValue(std::list<const libconfig::Setting *> & scope,
		  	    const std::string & localScopeName,
		  	    const std::string & param,
		  	    settings settingId = s_required,
		  	    bool *isSet = NULL,
		  	    bool truncate = false)
  {
	return configValue<T,T>(scope,localScopeName,param,settingId,isSet,truncate);
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Utility function for converting locale to FmiLanguage
   */
  // ----------------------------------------------------------------------

  FmiLanguage locale2FmiLanguage(const std::string & confPath,
  	  	  	  	  	  	  	  	 const std::string & theLocale)
  {
	const char * langMsg = ": language must be 'fi','en' or 'sv'";

	if (theLocale.empty() || (theLocale.find("fi") == 0))
		return kFinnish;
	else if (theLocale.find("en") == 0)
		return kEnglish;
	else if (theLocale.find("sv") == 0)
		return kSwedish;
	else
		throw std::runtime_error(confPath + ": '" + theLocale + "'" + langMsg);
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Utility function for converting string to lower case
   */
  // ----------------------------------------------------------------------

  std::string toLower(std::string theString)
  {
	  boost::to_lower(theString);

	  return theString;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Utility function for converting boost::ptime to utc or localized
   *		formatted datum string
   */
  // ----------------------------------------------------------------------

  NFmiString toFormattedString(const boost::posix_time::ptime & pt,
		  	  	  	  	  	   const std::string & pref,
		  	  	  	  	  	   bool utc,
		  	  	  	  	  	   FmiLanguage language = kFinnish)

  {
	// Convert from ptime (to_iso_string() format YYYYMMDDTHHMMSS,fffffffff
	// where T is the date-time separator) to NFmiMetTime (utc) and further
	// to local time if (utc not) requested

	NFmiMetTime utcTime(1);
	std::string isoString = to_iso_string(pt);
	boost::replace_first(isoString,"T","");
	utcTime.FromStr(isoString);

	return (utc ? utcTime.ToStr(pref,language) : utcTime.CorrectLocalTime().ToStr(pref,language));
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Render aerodrome timeserie
   */
  // ----------------------------------------------------------------------

  template <typename T>
  void
  SvgRenderer::render_aerodrome(const T & theFeature)
  {
    // First initialize axis manager and render axis labels and elevation lines

	if (initAerodrome) {
		render_aerodromeFrame(theFeature.timePeriod());
		initAerodrome = false;
	}

    render_timeserie(theFeature);
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Initialization for aerodrome rendering.
   */
  // ----------------------------------------------------------------------

  void
  SvgRenderer::render_aerodromeFrame(const boost::posix_time::time_period & theTimePeriod)
  {
    if(options.debug)	std::cerr << "Rendering AerodromeFrame" << std::endl;

    boost::posix_time::ptime bt = theTimePeriod.begin();
    boost::posix_time::ptime et = theTimePeriod.end();

    if (
  	    (bt.is_not_a_date_time() || et.is_not_a_date_time()) ||
  	    (et < (bt + boost::posix_time::hours(1))) ||
  	    (et > (bt + boost::posix_time::hours(240)))
  	   )
  	  throw std::runtime_error("render_aerodromeFrame: invalid time period");

    // Initialize axis manager and render y -axis labels

	render_elevationAxis();

    // Render x -axis labels and elevation lines

	render_timeAxis(theTimePeriod);
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Aerodrome y -axis label rendering.
   *
   * 		Initializes axis manager.
   */
  // ----------------------------------------------------------------------

  void
  SvgRenderer::render_elevationAxis()
  {
	// Initialize axis manager

	axisManager.reset(new AxisManager(config));

	// Output y -axis labels
	//
	// svg y -axis grows downwards; use axis height to transfrom the y -coordinates

	double axisHeight = axisManager->axisHeight();

	const std::list<Elevation> & elevations = axisManager->elevations();
	std::list<Elevation>::const_iterator it = elevations.begin();

	std::string ELEVATIONLABELS1("ELEVATIONLABELS1");
	std::string ELEVATIONLABELS2("ELEVATIONLABELS2");

	for ( ; (it != elevations.end()); it++) {
		const std::string & lLabel = it->lLabel();
		if (!lLabel.empty())
			texts[ELEVATIONLABELS1] << "<text x=\"0\" y=\""
									<< std::fixed << std::setprecision(1) << (axisHeight - it->scaledElevation())
									<< "\">"
									<< lLabel
									<< "</text>\n";

		const std::string & rLabel = it->rLabel();
		if (!rLabel.empty())
			texts[ELEVATIONLABELS2] << "<text x=\"0\" y=\""
									<< std::fixed << std::setprecision(1) << (axisHeight - it->scaledElevation())
									<< "\">"
									<< rLabel
									<< "</text>\n";
	}
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Aerodrome x -axis label and elevation line rendering
   */
  // ----------------------------------------------------------------------

  void
  SvgRenderer::render_timeAxis(const boost::posix_time::time_period & theTimePeriod)
  {
  	const std::string confPath("TimeAxis");

  	try {
  		const char * typeMsg = " must contain a group in curly brackets";
  		const char * stepMsg = ": value >= 1 expected";

  		const libconfig::Setting & timeSpecs = config.lookup(confPath);
  		if(!timeSpecs.isGroup())
  			throw std::runtime_error(confPath + typeMsg);

		// Axis width (px)

		int width = configValue<int>(timeSpecs,confPath,"width");
		axisManager->axisWidth(width);

		// Step for label/hour output

		const char *l;
		unsigned int step = configValue<unsigned int>(timeSpecs,confPath,l = "step");

		if (step < 1)
			throw std::runtime_error(confPath + ": " + l + ": '" + boost::lexical_cast<std::string>(step) + "'" + stepMsg);

		// utc (default: false)

		bool isSet;
		bool utc = configValue<bool>(timeSpecs,confPath,"utc",NULL,s_optional,&isSet);
		if (!isSet)
			utc = false;
		axisManager->utc(utc);

		// Time period

		axisManager->timePeriod(theTimePeriod);

		// Output the labels

		boost::posix_time::ptime bt = theTimePeriod.begin(),et = theTimePeriod.end();
		boost::posix_time::time_iterator it(bt,boost::posix_time::hours(step));

		double x = 0.0,xStep = (step * axisManager->xStep());

		for ( ; (it <= et); ++it, x += xStep)
			texts["TIMELABELS"] << "<text x=\""
								<< std::fixed << std::setprecision(1) << x
								<< "\" y=\"0\">"
								<< toFormattedString(*it,"HH",utc).CharPtr()
								<< "</text>\n";

		// Output elevation lines
		//
		// svg y -axis grows downwards; use axis height to transfrom the y -coordinates

		double axisHeight = axisManager->axisHeight();

		const std::list<Elevation> & elevations = axisManager->elevations();
		std::list<Elevation>::const_iterator eit = elevations.begin();

		for ( ; (eit != elevations.end()); eit++)
			if (eit->line()) {
				double y = eit->scaledElevation();

				if (y > 0.1)
					texts["ELEVATIONLINES"] << "<line x1=\"0\" y1=\""
											<< std::fixed << std::setprecision(1) << (axisHeight - y)
											<< "\" x2=\""
											<< std::fixed << width
											<< "\" y2=\""
											<< std::fixed << std::setprecision(1) << (axisHeight - y)
											<< "\"/>\n";
			}
  	}
  	catch(libconfig::ConfigException & ex)
  	{
		if(!config.exists(confPath))
	  		// No settings for confPath
	  		//
		    throw std::runtime_error("Settings for " + confPath + " are missing");

	    throw ex;
  	}
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Picture header rendering
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::render_header(const std::string & hdrClass,
		  	  	  	  	  	  	  const boost::posix_time::ptime & datetime,
		  	  	  	  	  	  	  bool useDate,
		  	  	  	  	  	  	  const std::string & text,
		  	  	  	  	  	  	  const std::string & confPath)
  {
	try {
		std::string HEADERhdrClass("HEADER" + hdrClass);

		if (!text.empty()) {
			// Just store the text (target region's name or id)
			//
			texts[HEADERhdrClass] << svgescapetext(text,true);
			return;
		}

		const char * typeMsg = " must contain a list of groups in parenthesis";
		const char * hdrtypeMsg = ": type must be 'datetime'";

		const libconfig::Setting & hdrSpecs = config.lookup(confPath);
		if(!hdrSpecs.isList())
			throw std::runtime_error(confPath + typeMsg);

		settings s_name((settings) (s_base + 0));

		// Configuration blocks to search values for the settings; global blocks (blocks with no name and no locale),
		// locale global blocks (blocks with matching name but having no locale and blocks having no name
		// but with matching locale) and the current block (block with matching name and locale).
		//
		// The blocks are stored to the list in the order of appearance (and having current block
		// as the last element). The list is processed in reverse order when searching.
		//
		// Current block or locale global block is needed for rendering.
		//
		// Note: Currently no locale settings are required.

		std::list<const libconfig::Setting *> scope;
		bool nameMatch,noName;
		bool hasLocaleGlobals = false;

		int hdrIdx = -1;
		int lastIdx = hdrSpecs.getLength() - 1;

		for(int i=0; i<=lastIdx; ++i)
		{
			const libconfig::Setting & specs = hdrSpecs[i];
			if(!specs.isGroup())
				throw std::runtime_error(confPath + typeMsg);

			try {
				noName = false;
				nameMatch = (lookup<std::string>(specs,confPath,"name",s_name) == hdrClass);
			}
			catch (SettingIdNotFoundException & ex) {
				// Global settings have no name; go on to check locale
				//
				noName = true;
				nameMatch = false;
			}

			// Enter the block on matching name, and for globals and last config entry
			// to load/use the globals
			//
			if (nameMatch || noName || ((i == lastIdx) && hasLocaleGlobals)) {
				if (nameMatch || noName) {
					if (nameMatch)
						hdrIdx = i;

					// Locale
					//
					std::string locale(toLower(configValue<std::string>(specs,hdrClass,"locale",NULL,s_optional)));
					bool localeMatch = (locale == options.locale);

					if (localeMatch || (locale == "")) {
						scope.push_back(&specs);

						// Currently no locale settings are required
						//
//						if ((nameMatch && (locale == "")) || (noName && localeMatch))
						if ((nameMatch && (locale == "")) || (noName /* && localeMatch */))
							hasLocaleGlobals = true;
					}

					// Currently no locale settings are required
					//
//					if ((noName || (!localeMatch)) && ((i < lastIdx) || (!hasLocaleGlobals)))
					if ((noName || ((!localeMatch) && (locale != ""))) && ((i < lastIdx) || (!hasLocaleGlobals)))
						continue;
				}

				// Type (currently only datetime supported)
				std::string type = configValue<std::string>(scope,hdrClass,"type",s_optional);

				if (type.empty() || (type == "datetime")) {
					// NFmiTime language and format
					//
					FmiLanguage language = locale2FmiLanguage(confPath,options.locale);
					std::string pref(configValue<std::string>(scope,hdrClass,"pref")),prefDate,& format = pref;

					if (useDate) {
						prefDate = configValue<std::string>(scope,hdrClass,"prefdate",s_optional);

						if (!prefDate.empty())
							format = prefDate;
					}

					// utc
					bool isSet;
					bool utc(configValue<bool>(scope,hdrClass,"utc",s_optional,&isSet));

					// Store formatted datum
					texts[HEADERhdrClass] << (datetime.is_not_a_date_time()
											  ? ""
											  : svgescapetext(toFormattedString(datetime,format,isSet && utc,language).CharPtr(),true));

					return;
				}
				else
					throw std::runtime_error(confPath + ": '" + type + "'" + hdrtypeMsg);
			}  // if
		}	// for

		if (options.debug) {
			// No (locale) settings for the header
			//
			const char * p = ((hdrIdx < 0) ? "Settings for " : "Locale specific settings for " );

			debugoutput << p
					    << hdrClass
					    << " not found\n";
		}
	}
	catch (libconfig::SettingNotFoundException & ex) {
		// No settings for confPath
		//
		if (options.debug)
			debugoutput << "Settings for "
						<< confPath
						<< " ("
						<< hdrClass
						<< ") not found\n";
	}
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Get fill symbol position(s) within given fill area rectangle.
   */
  // ----------------------------------------------------------------------

  bool getFillPositions(NFmiFillAreas::const_iterator iter,
		  	  	  	  	int symbolWidth,int symbolHeight,
			  			float scale,
			  			const NFmiFillRect & reservedArea,
			  			NFmiFillPositions & fpos,
			  			size_t & symCnt)

  {
	// Check if the area is reserved (overlaps reserved area)

	if (
		(iter->first.x < reservedArea.second.x) && (iter->second.x > reservedArea.first.x) &&
		(iter->first.y < reservedArea.second.y) && (iter->second.y > reservedArea.first.y)
	   )
		return false;

	// Fillrect and symbol bounding box dimensions

	float width = (iter->second.x - iter->first.x);
	float height = (iter->second.y - iter->first.y);
	float bw = symbolWidth * scale;
	float bh = symbolHeight * scale;

	// Number of symbols horizontally and vertically

	int h,nh = static_cast<int>(width / floor(bw));
	int v,nv = static_cast<int>(height / floor(bh));

	// Horizontal and vertical spacing between symbols

	float hoffset = (width - (nh * bw)) / (nh + 1);
	float voffset = (height - (nv * bh)) / (nv + 1);

	// Bounding box mid point (symbol center point) offsets

	float xoffset = bw / 2;
	float yoffset = bh / 2;

	// NW, SE, NE and SW offsets for varying the symbol positions

	float shoffset = ((scale - 1) * symbolWidth) / 2;
	float svoffset = ((scale - 1) * symbolHeight) / 2;

	float symhoff[] = { (xoffset + shoffset), (xoffset - shoffset), (xoffset - shoffset), (xoffset + shoffset) };
	float symvoff[] = { (yoffset + svoffset), (yoffset - svoffset), (yoffset + svoffset), (yoffset - svoffset) };
	int   shoCnt = (sizeof(symhoff) / sizeof(float)),svoCnt = (sizeof(symvoff) / sizeof(float));

	float x,y;

	for (h = 0, x = hoffset; (h < nh); h++, x += (bw + hoffset))
		for (v = 0, y = voffset; (v < nv); v++, y += (bh + voffset), symCnt++)
			fpos.push_back(Point(iter->first.x + x + symhoff[symCnt % shoCnt],iter->first.y + y + symvoff[symCnt % svoCnt]));

	return true;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Sort fill areas by top left corner
   */
  // ----------------------------------------------------------------------

  bool sortFillAreas(const NFmiFillRect & area1,const NFmiFillRect & area2)
  {
	if (area1.first.x < area2.first.x)
		return true;

	return ((area1.first.x == area2.first.x) && (area1.first.y < area2.first.y));
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Split fill areas wider than twice the symbol width
   */
  // ----------------------------------------------------------------------

  void splitFillAreas(NFmiFillAreas & areas,int symbolWidth,float scale)
  {
	NFmiFillAreas::iterator iter = areas.begin();
	float scaledWidth = scale * symbolWidth;

	for ( ; (iter != areas.end()); iter++) {
		if ((iter->second.x - iter->first.x) > (2 * scaledWidth)) {
			float xEnd = iter->second.x;
			float x1 = iter->second.x = iter->first.x + floor(scaledWidth + 0.5);

			do {
				float x2 = x1 + floor(scaledWidth + 0.5);

				if ((x2 + floor(scaledWidth + 0.5)) > xEnd)
					x2 = xEnd;

				areas.push_back(std::make_pair(Point(x1,iter->first.y),Point(x2,iter->second.y)));

				x1 = x2;
			}
			while (x1 < xEnd);
		}
	}
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Get column or row step for selecting fill areas.
   *
   *		Returns first step and sets looping step to 'stepN'.
   */
  // ----------------------------------------------------------------------

  size_t getStep(size_t symbolCnt,size_t nColsOrRows,size_t & stepN)
  {
	size_t step1;

	if (symbolCnt > 1) {
		stepN = (nColsOrRows / symbolCnt);

		if ((symbolCnt != 2) && (nColsOrRows % 2) && (((symbolCnt * (stepN + 1)) - 1) == nColsOrRows)) {
			// Even spaced from end to end
			//
			step1 = 1;
			stepN++;
		}
		else {
			// Adjust first step by the starting/ending offsets
			//
			size_t sOff = (stepN - 1),eOff = (nColsOrRows - (symbolCnt * stepN));

			step1 = ((eOff <= sOff) ? (stepN - ((sOff - eOff) / 2)) : (stepN + ((eOff - sOff) / 2)));

			if ((symbolCnt == 2) && (nColsOrRows % 2))
				// Same offset for both ends for 2 symbols
				//
				stepN = (nColsOrRows - sOff) - step1;
		}
	}
	else
		step1 = stepN = ((nColsOrRows / 2) + (nColsOrRows % 2));

	return step1;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Fill area column handling.
   */
  // ----------------------------------------------------------------------

  bool sortFillAreaColumnAsc(const NFmiFillRect & r1,const NFmiFillRect & r2) { return (r1.first.y < r2.first.y); }
  bool sortFillAreaColumnDesc(const NFmiFillRect & r1,const NFmiFillRect & r2) { return (r1.first.y > r2.first.y); }

  struct Column
  {
	Column(NFmiFillAreas::const_iterator fstArea,size_t n,bool sortAsc = true)
	{
		NFmiFillAreas::const_iterator lstArea = fstArea;
		advance(lstArea,n);
		fillAreas.insert(fillAreas.end(),fstArea,lstArea);
		if (n > 1)
			fillAreas.sort(sortAsc ? sortFillAreaColumnAsc : sortFillAreaColumnDesc);
	}
	NFmiFillAreas::const_iterator areas() const { return fillAreas.begin(); }
	size_t nRows() const { return fillAreas.size(); }

  private:
	NFmiFillAreas fillAreas;
  };

  typedef std::vector<Column> FillAreaColumns;

  FillAreaColumns getFillAreaColumns(const NFmiFillAreas & areas)
  {
	// Extract fill area "columns", groups of subsequent fill areas having starting x -coordinate less than or
	// equal to the center x -coordinate of group's first fill area.

	NFmiFillAreas::const_iterator iArea = areas.begin(),iFirstColArea;
	FillAreaColumns columns;
	double colX = 0.0;
	size_t n = 0;
	bool sortAsc = true;

	for (iFirstColArea = iArea; (iArea != areas.end()); iArea++)
		if ((iFirstColArea == iArea) || (iArea->first.x > colX)) {
			if (iFirstColArea != iArea) {
				columns.push_back(Column(iFirstColArea,n,sortAsc));
				sortAsc = (!sortAsc);
			}

			colX = iArea->first.x + ((iArea->second.x - iArea->first.x) / 2);
			iFirstColArea = iArea;
			n = 1;
		}
		else
			n++;

	columns.push_back(Column(iFirstColArea,n));

	return columns;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Get area symbol positions within fill areas.
   */
  // ----------------------------------------------------------------------

  void getFillPositions(NFmiFillAreas & areas,size_t symbolCnt,NFmiFillPositions & fpos,const std::string & fillMode = "")
  {
	// Extract fill area "columns", groups of fill areas having starting x -coordinate greater than or equal to
	// the starting x -coordinate and less than or equal to the center x -coordinate of group's first fill area.

	FillAreaColumns columns = getFillAreaColumns(areas);

	// Get fill positions within the columns

	if (fillMode.empty() || (fillMode == "symbol")) {
		// By default get one fill position for each symbol
		//
		;
	}
	else if (fillMode == "fill") {
		// Get all available positions
		//
		symbolCnt = areas.size();
	}
	else if (fillMode == "column") {
		// Get one fill position for each column if there are enough columns for the symbols,
		// otherwise get one fill position for each symbol
		//
		if (columns.size() >= symbolCnt)
			symbolCnt = columns.size();
	}
	else if (
			 (strlen(fillMode.c_str()) >= 2) && (strlen(fillMode.c_str()) <= 4) &&
			 (strspn(fillMode.c_str(),"1234567890") == (strlen(fillMode.c_str()) - 1)) &&
			 strrchr(fillMode.c_str(),'%') &&
			 (atoi(fillMode.c_str()) <= 100)
			) {
		// Get given percentage of available positions
		//
		size_t nPos = 0;

		for (FillAreaColumns::const_iterator iCol = columns.begin(); (iCol != columns.end()); iCol++)
			nPos += iCol->nRows();

		size_t nSym = (size_t) floor(((atoi(fillMode.c_str()) / 100.0) * nPos) + 0.5);

		if (nSym > symbolCnt)
			symbolCnt = std::min(nSym,nPos);
	}
	else
		throw std::runtime_error("getFillPositions: invalid fill mode: '" + fillMode + "'");

	size_t nAreasLeft = areas.size(),nSymbolsLeft = symbolCnt,nColsLeft = columns.size(),colStepN = 1,colStep = 1,col,n;

	if (symbolCnt < nColsLeft)
		// There are more columns than symbols, stepping over some columns
		//
		colStep = getStep(symbolCnt,nColsLeft,colStepN);

	for (FillAreaColumns::const_iterator iCol = columns.begin(); ((iCol != columns.end()) && (nSymbolsLeft > 0)); iCol++, nColsLeft--) {
		// Step over columns
		//
		for (n = 1; ((iCol != columns.end()) && (n < colStep)); iCol++, nColsLeft--, n++)
			nAreasLeft -= iCol->nRows();

		colStep = colStepN;

		if (iCol == columns.end())
			// Should not get here
			//
			break;

		size_t nColSymbols = (nSymbolsLeft / nColsLeft),rowStepN = 1,rowStep = 1,row;

		if ((nColSymbols * nColsLeft) < nSymbolsLeft)
			nColSymbols++;

		nAreasLeft -= iCol->nRows();

		if (nAreasLeft < (nSymbolsLeft - nColSymbols))
			// There would not be enough fill areas in the rest of the columns; place more symbols into
			// this column
			//
			nColSymbols = nSymbolsLeft - nAreasLeft;

		if (nColSymbols < iCol->nRows())
			// There are more fill areas than symbols; stepping over some fill areas
			//
			rowStep = getStep(nColSymbols,iCol->nRows(),rowStepN);

		NFmiFillAreas::const_iterator iArea = iCol->areas();

		for (col = 0,row = 0,n = 1; ((col < iCol->nRows()) && (nColSymbols > 0) && (nSymbolsLeft > 0)); col++, iArea++) {
			if (n >= rowStep) {
				double x = iArea->first.x + ((iArea->second.x - iArea->first.x) / 2.0);
				double y = iArea->first.y + ((iArea->second.y - iArea->first.y) / 2.0);

				fpos.push_back(Point(x,y));

				nColSymbols--;
				nSymbolsLeft--;
				n = 0;

				rowStep = rowStepN;
			}

			n++;
		}
	}

	if (nSymbolsLeft > 0)
		throw std::runtime_error("getFillPositions: unexpected end of columns");
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Get area symbol positions within fill areas.
   */
  // ----------------------------------------------------------------------

  void getFillPositions(NFmiFillAreas & areas,int symbolWidth,int symbolHeight,size_t symbolCnt,float scale,const NFmiFillRect & reservedArea,NFmiFillPositions & fpos,const std::string & fillMode = "")
  {
	// Fill areas might have room for multiple symbols; split them to hold a single symbol

	splitFillAreas(areas,symbolWidth,scale);

	// Sort the fill areas by top left corner

	areas.sort(sortFillAreas);

	// Erase reserved areas (areas which overlap reserved area)

	if (reservedArea.second.x > reservedArea.first.x)
		for (NFmiFillAreas::iterator iter = areas.begin(); (iter != areas.end()); )
			if (
				(iter->first.x < reservedArea.second.x) && (iter->second.x > reservedArea.first.x) &&
				(iter->first.y < reservedArea.second.y) && (iter->second.y > reservedArea.first.y)
			   )
				iter = areas.erase(iter);
			else
				iter++;

	if (symbolCnt < areas.size()) {
		// There are more fill areas than symbols, stepping over some fill areas
		//
		getFillPositions(areas,symbolCnt,fpos,fillMode);
		return;
	}

	// Populate every fill area with at most 4 fill symbols

	if (symbolCnt > (4 * areas.size()))
		throw std::runtime_error("getFillPositions: too many symbols");

	size_t nAreasLeft = areas.size(),nSymbolsLeft = symbolCnt,symIndex = 4,maxAreaSymbols;

	for (NFmiFillAreas::const_iterator iter = areas.begin(); ((iter != areas.end()) && (nSymbolsLeft > 0)); nSymbolsLeft--) {
		double xOff,yOff;

		maxAreaSymbols = ((nSymbolsLeft - 1 + (4 - symIndex)) / nAreasLeft) + 1;

		if ((maxAreaSymbols == 1) && (symIndex == 4)) {
			xOff = yOff = 2.0;				// Center of the area
		}
		else {
			if (symIndex == 4)
				xOff = yOff = 4.0;			// Upper left quarter of the area
			else if (symIndex == 3)
				xOff = yOff = (4.0 / 3.0);	// Lower right quarter of the area
			else if (symIndex == 2) {
				xOff = (4.0 / 3.0);			// Upper right quarter of the area
				yOff = 4;
			}
			else {
				xOff = 4;					// Lower left quarter of the area
				yOff = (4.0 / 3.0);
			}

			symIndex--;

			if (symIndex <= (4 - maxAreaSymbols))
				symIndex = 4;
		}

		double x = iter->first.x + ((iter->second.x - iter->first.x) / xOff);
		double y = iter->first.y + ((iter->second.y - iter->first.y) / yOff);

		fpos.push_back(Point(x,y));

		if ((maxAreaSymbols == 1) || (symIndex == 4)) {
			iter++;
			nAreasLeft--;
		}
	}

	if (nSymbolsLeft > 0)
		throw std::runtime_error("getFillPositions: unexpected end of fill areas");
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Get center fill area rect.
   */
  // ----------------------------------------------------------------------

  NFmiFillRect getCenterFillAreaRect(NFmiFillAreas & areas,double width,double scale)
  {
	if (areas.size() < 1)
		throw std::runtime_error("getCenterFillAreaRect: no fill areas");

	// Get the center column

	splitFillAreas(areas,width,scale);

	areas.sort(sortFillAreas);

	FillAreaColumns columns = getFillAreaColumns(areas);
	FillAreaColumns::const_iterator iCol = columns.begin();
	size_t colStepN;
	size_t colStep = getStep(1,columns.size(),colStepN);

	if (colStep > 1)
		advance(iCol,colStep - 1);

	// Get the center row

	NFmiFillAreas::const_iterator iArea = iCol->areas();
	size_t rowStepN;
	size_t rowStep = getStep(1,iCol->nRows(),rowStepN);

	if (rowStep > 1)
		advance(iArea,rowStep - 1);

	return *iArea;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Utility function for splitting text into lines using given
   * 		max. linewidth.
   *
   * 		Note: Only a small set (serif, ...) of fonts currently supported
   * 			  (limitation of cairo 'toy' api).
   *
   * 		Note: Max text height is not taken into account currently.
   *
   *		Returns true if the text could be fitted using the given w/h limits.
   */
  // ----------------------------------------------------------------------

  bool getTextLines(const std::string & text,				// Text to split
		  	  	    std::string font,						// Fonts
		  	  	    unsigned int fontsize,					// Font size
		  	  	  	cairo_font_slant_t slant,				// Font slant
		  	  	  	cairo_font_weight_t weight,				// Font weight
		  	  	  	unsigned int maxWidth,					// Max text width
		  	  	  	unsigned int maxHeight,					// Max text height
					std::list<std::string> & outputLines,	// Text lines
					unsigned int & textWidth,				// Text width
					unsigned int & textHeight,				// Text height
					unsigned int & maxLineHeight)			// Max line height
  {
	// cairo surface to get font mectrics

	cairo_surface_t *cs = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,1,1);
	cairo_t * cr = cairo_create(cs);
	cairo_surface_destroy(cs);

	cairo_select_font_face(cr,font.c_str(),slant,weight);
	cairo_set_font_size(cr,fontsize);

	// Split the text to lines shorter than given max. width.
	// Keep track of max. line width and height

	std::vector<std::string> inputLines;
	boost::split(inputLines,text,boost::is_any_of("\n"));
	size_t l,textlc = inputLines.size();

	cairo_text_extents_t extents;
	std::string line,word;

// for regenerate with smaller fontsize {
//

	unsigned int maxLineWidth = 0;
	maxLineHeight = 0;

	for (l = 0; (l < textlc); l++) {
		// Next line
		//
		cairo_text_extents(cr,inputLines[l].c_str(),&extents);

		unsigned int lineHeight = static_cast<unsigned int>(ceil(extents.height));

		if (lineHeight > maxLineHeight)
			maxLineHeight = lineHeight;

		unsigned int lineWidth = static_cast<unsigned int>(ceil(extents.width));

		if (lineWidth < maxWidth) {
			outputLines.push_back(inputLines[l]);

			if (lineWidth > maxLineWidth)
				maxLineWidth = lineWidth;

			continue;
		}

		std::vector<std::string> words;
		boost::split(words,inputLines[l],boost::is_any_of(" \t"));

		size_t w,linewc = 0,textwc = words.size(),pos = std::string::npos;
		bool fits;

		for (w = 0; (w < textwc); ) {
			// Next word.
			// Add a space in front of it if not the first word of the line
			//
			word = std::string((linewc > 0) ? " " : "") + words[w];

			cairo_text_extents(cr,(line + word).c_str(),&extents);

			if (! (fits = (ceil(extents.width) < maxWidth))) {
				// Goes too wide, try cutting to first comma or period
				//
				pos = word.find_first_of(",.");

				if (pos != std::string::npos) {
					std::string cutted = word.substr(0,pos + 1);
					cairo_text_extents(cr,(line + cutted).c_str(),&extents);

					if ((fits = (ceil(extents.width) < maxWidth)))
						word = cutted;
				}
			}

			if (fits) {
				line += word;
				linewc++;
				lineWidth = static_cast<unsigned int>(ceil(extents.width));

				if (pos == std::string::npos) {
					// Word was not cutted; next word
					//
					w++;

					if (w < textwc)
						continue;
				}
				else
					// Cutted; the rest of the word goes to the next line
					//
					words[w].erase(0,pos + 1);
			}
			else if (linewc == 0) {
				// A single word exceeding the max line width
				//
				lineWidth = static_cast<unsigned int>(ceil(extents.width));
				w++;
			}

			// Maximum line width or end of line reached

			outputLines.push_back((linewc == 0) ? word : line);

			if (lineWidth > maxLineWidth)
				maxLineWidth = lineWidth;

			line.clear();
			linewc = 0;

			pos = std::string::npos;

//			if ((maxHeight > 0) && (maxHeight < ((outputLines.size() * (maxLineHeight + 2)) - 2)))
//				// Max height exceeded; decrement font size (downto half of the given size) and regenerate the text
//				;

		}  // for word
	}  // for line

	textHeight = ((outputLines.size() * (maxLineHeight + 2)) - 2);
	textWidth = ((maxLineWidth == 0) ? maxWidth : maxLineWidth);

//	if ((maxHeight > 0) && (maxHeight < textHeight))
//		// Max height exceeded; decrement font size (downto half of the given size) and regenerate the text
//		;
//
//  }  // for regenerate with smaller fontsize

	cairo_destroy(cr);
//	cairo_debug_reset_static_data();

	return true;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Text rendering
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::render_text(Texts & texts,
		  	  	  	  	  	    const std::string & confPath,
		  	  	  	  	  	    const std::string & textName,
		  	  	  	  	  	    const std::string & text,
		  	  	  	  	  	    int & xPosW,int & yPosH,		// I: text's starting x/y pos O: text's width/height
		  	  	  	  	  	    bool ignoreErr,
		  	  	  	  	  	    bool setTextArea,
		  	  	  	  	  	    bool centerToStartX,
		  	  	  	  	  	    const std::string & TEXTPOSid,
		  	  	  	  	  	    bool useTextName,
		  	  	  	  	  	    int * maxTextWidth,				// I
		  	  	  	  	  	    int * fontSize,					// I
		  	  	  	  	  	    int * tXOffset,					// I/O: text's x -offset
		  	  	  	  	  	    int * tYOffset)					// I/O: text's y -offset
  {
	try {
		const char * typeMsg = " must contain a list of groups in parenthesis";
		const char * styleMsg = ": slant must be 'normal', 'italic' or 'oblique'";
		const char * weightMsg = ": weight must be 'normal' or 'bold'";

		// Note: 'textName' contains area/symbol type for area/symbol infotexts; using confPath to generate fixed placeholder names.
		//		 'useTextName' is set when rendering cloudlayer, icing/turbulence and zerotolerance labels; using the text name
		//		  as text output placeholder as is.

		std::string TEXTCLASStextName("TEXTCLASS" + ((TEXTPOSid.empty() || useTextName) ? textName : confPath));
		std::string TEXTtextName(useTextName ? textName : ("TEXT" + (TEXTPOSid.empty() ? textName : confPath)));
		std::string TEXTAREAtextName;

		if (setTextArea) {
			// Set initial textarea rect in case no output is generated
			//
			TEXTAREAtextName = "TEXTAREA" + textName;
			texts[TEXTAREAtextName] << " x=\"0\" y=\"0\" width=\"0\" height=\"0\"";
		}

		int startX = xPosW,startY = yPosH;
		xPosW = yPosH = 0;

		if (text.empty())
			return;

		const libconfig::Setting & textSpecs = config.lookup(confPath);
		if(!textSpecs.isList())
			throw std::runtime_error(confPath + typeMsg);

		settings s_name((settings) (s_base + 0));

		// Configuration blocks to search values for the settings; global blocks (blocks with no name and no locale),
		// locale global blocks (blocks with matching name but having no locale and blocks having no name
		// but with matching locale) and the current block (block with matching name and locale).
		//
		// The blocks are stored to the list in the order of appearance (and having current block
		// as the last element). The list is processed in reverse order when searching.
		//
		// Current block or locale global block is needed for rendering.
		//
		// Note: Currently locale settings are not used.

		std::list<const libconfig::Setting *> scope;
		bool nameMatch,noName;
		bool hasLocaleGlobals = false;

		int textIdx = -1;
		int lastIdx = textSpecs.getLength() - 1;

		for(int i=0; i<=lastIdx; ++i)
		{
			const libconfig::Setting & specs = textSpecs[i];
			if(!specs.isGroup())
				throw std::runtime_error(confPath + typeMsg);

			try {
				noName = false;
				nameMatch = (lookup<std::string>(specs,confPath,"name",s_name) == textName);
			}
			catch (SettingIdNotFoundException & ex) {
				// Global settings have no name; go on to check locale
				//
				noName = true;
				nameMatch = false;
			}

			// Enter the block on matching name, and for globals and last config entry
			// to load/use the globals
			//
			try {
				if (nameMatch || noName || ((i == lastIdx) && hasLocaleGlobals)) {
					if (nameMatch || noName) {
						if (nameMatch)
							textIdx = i;

						// Locale
						//
						// Currently locale settings are not used
						//
						std::string locale(options.locale /*toLower(configValue<std::string>(specs,textName,"locale",NULL,s_optional))*/);
						bool localeMatch = (locale == options.locale);

						if (localeMatch || (locale == "")) {
							scope.push_back(&specs);

							if ((nameMatch && (locale == "")) || (noName && localeMatch))
								hasLocaleGlobals = true;
						}

						if ((noName || (!localeMatch)) && ((i < lastIdx) || (!hasLocaleGlobals)))
							continue;
					}

					// Font name
					std::string font = configValue<std::string>(scope,textName,"font-family");

					// Font size
					int fSize = (fontSize && (*fontSize > 0)) ? *fontSize : configValue<unsigned int>(scope,textName,"font-size");

					// Font style
					cairo_font_slant_t slant = CAIRO_FONT_SLANT_NORMAL;
					std::string style = configValue<std::string>(scope,textName,"font-style",s_optional);

					if (style == "italic")
						slant = CAIRO_FONT_SLANT_ITALIC;
					else if (style == "oblique")
						slant = CAIRO_FONT_SLANT_OBLIQUE;
					else if (style != "")
						throw std::runtime_error(confPath + styleMsg);
					else
						style = "normal";

					// Font weight
					cairo_font_weight_t _weight = CAIRO_FONT_WEIGHT_NORMAL;
					std::string weight = configValue<std::string>(scope,textName,"font-weight",s_optional);

					if (weight == "bold")
						_weight = CAIRO_FONT_WEIGHT_BOLD;
					else if (weight != "")
						throw std::runtime_error(confPath + weightMsg);
					else
						weight = "normal";

					// Stroke and fill color
					std::string stroke = configValue<std::string>(scope,textName,"stroke",s_optional);
					std::string fill = configValue<std::string>(scope,textName,"fill",s_optional);

					// Max width, height, x/y margin and x/y offsets
					//
					// For area/symbol text (when tXOffset and tYOffset are passed in as nonnull) the offsets are used later
					// when setting the final text position.
					//
					// Note: If the passed offsets (or font size) are nonzero, they were set in the text and won't be overridden by config settings.
					unsigned int maxWidth = ((maxTextWidth && (*maxTextWidth > 0)) ? *maxTextWidth : configValue<unsigned int>(scope,textName,"textwidth"));

					bool isSet;
					unsigned int maxHeight = configValue<unsigned int>(scope,textName,"textheight",s_optional,&isSet);
					if (! isSet)
						maxHeight = 0;

					unsigned int margin = configValue<unsigned int>(scope,textName,"margin",s_optional,&isSet);
					if (! isSet)
						margin = 2;

					if (maxWidth <= 20)
						maxWidth = 20;
					if ((maxHeight != 0) && (maxHeight <= 20))
						maxHeight = 20;

					int xOffset = configValue<int>(scope,textName,"txoffset",s_optional,&isSet);
					if ((! isSet) || tXOffset) {
						if (isSet && (*tXOffset == 0))
							*tXOffset = xOffset;

						xOffset = 0;
					}

					int yOffset = configValue<int>(scope,textName,"tyoffset",s_optional,&isSet);
					if ((! isSet) || tYOffset) {
						if (isSet && (*tYOffset == 0))
							*tYOffset = yOffset;

						yOffset = 0;
					}

					// Settings for background; style and x/y offsets for top left corner

					std::string bStyle = configValue<std::string>(scope,textName,"bstyle",s_optional);

					int bXOffset = configValue<int>(scope,textName,"bxoffset",s_optional,&isSet);
					if (!isSet)
						bXOffset = 0;

					int bYOffset = configValue<int>(scope,textName,"byoffset",s_optional,&isSet);
					if (!isSet)
						bYOffset = 0;

					// Split the text into lines using given max. width and height

					std::list<std::string> textLines;
					unsigned int textWidth,textHeight,maxLineHeight;

					getTextLines(text,
								 font,fSize,slant,_weight,
								 maxWidth,maxHeight,
								 textLines,
								 textWidth,textHeight,maxLineHeight);

					xPosW = (2 * margin) + textWidth;

					// Store the css class definition
					//
					// Note: Generating css class for each area infotext; the settings can differ

					std::string textClass("text" + ((TEXTPOSid.empty() || useTextName) ? textName : TEXTPOSid.substr(TEXTPOSid.find("_") + 1)));

					if (texts[TEXTCLASStextName].str().empty()) {
						texts[TEXTCLASStextName] << "." << textClass
												 << " {\nfont-family: " << font
												 << ";\nfont-size: " << fSize
												 << "px;\nfont-weight : " << weight
												 << ";\nfont-style : " << style;

						if (!stroke.empty()) texts[TEXTCLASStextName] << ";\nstroke: " << stroke;
						if (!fill.empty()) texts[TEXTCLASStextName] << ";\nfill: " << fill;

						texts[TEXTCLASStextName] << ";\n}\n";
					}

					int x = startX - xOffset - (centerToStartX ? ((textWidth / 2) + margin) : 0);
					int y = startY + yOffset;

					if (setTextArea) {
						// Store geometry for text border
						//
						texts[TEXTAREAtextName].str("");
						texts[TEXTAREAtextName].clear();

						texts[TEXTAREAtextName] << " x=\"" << x << "\" y=\"" << y << "\" width=\"" << xPosW;
					}

					// Store the text

					std::list<std::string>::const_iterator it = textLines.begin();

					for (x += margin,y += margin; (it != textLines.end()); it++) {
						y += maxLineHeight;

						if ((!TEXTPOSid.empty()) && (it == textLines.begin()))
							// Set transformation data.
							//
							// Note: Scaling placeholder/value might not get set; unset placehoders will be removed prior output
							//
							texts[TEXTtextName] << "<g transform=\"translate(--" << TEXTPOSid << "--) --" << TEXTPOSid << "SCALE--\">\n";

						if (!bStyle.empty())
							// Set background
							//
							texts[TEXTtextName] << "<rect width=\"" << textWidth - (2 * bXOffset)
												<< "\" height=\"" << textHeight - (2 * bYOffset)
												<< "\" x=\"" << bXOffset << "\" y=\"" << bYOffset << "\" style=\"" << bStyle << "\"" << "/>";

						texts[TEXTtextName] << "<text class=\"" << textClass
											<< "\" x=\"" << x
											<< "\" y=\"" << y
											<< "\">" << svgescapetext(*it) << "</text>\n";
					}

					yPosH = y;

					if (!TEXTPOSid.empty())
						texts[TEXTtextName] << "</g>\n";
					else if (setTextArea)
						texts[TEXTAREAtextName] << "\" height=\"" << yPosH << "\" ";

					return;
				}  // if
			}
			catch (...) {
				if (ignoreErr)
					return;

				throw;
			}
		}	// for

		if (options.debug) {
			// No (locale) settings for the text
			//
			const char * p = ((textIdx < 0) ? "Settings for " : "Locale specific settings for " );

			debugoutput << p
						<< textName
						<< " not found\n";
		}
	}
	catch (libconfig::SettingNotFoundException & ex) {
		// No settings for confPath
		//
		if (options.debug)
			debugoutput << "Settings for "
						<< confPath
						<< " ("
						<< textName
						<< ") not found\n";
	}
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Symbol mapping. Sets the mapped code into 'mappedCode' and
   *		returns true if nonempty mapping exists.
   */
  // ----------------------------------------------------------------------

  bool symbolMapping(const libconfig::Config & config,
		  	  	  	 const std::string & confPath,
		  	  	  	 const std::string & symCode,
		  	  	  	 settings s_code,
		  	  	  	 std::string & mappedCode,
		  	  	  	 std::list<const libconfig::Setting *> * scope = NULL)
  {
	// Symbol's <code>; code_<symCode> = <code>
	//
	// Note: libconfig supports only some non alphanumeric characters in configuration keys; the codes
	//		 have their '+' and '@' characters replaced with 'P_' and '_' respectively (e.g. '+RA' is converted to 'P_RA')

	mappedCode = boost::algorithm::trim_copy(symCode);
	bool isSet = false;

	if (!mappedCode.empty()) {
		const char * typeMsg = " must contain a group in curly brackets";

		const libconfig::Setting & symbolSpecs = (scope ? *(scope->front()) : config.lookup(confPath));

		if(!symbolSpecs.isGroup())
			throw std::runtime_error(confPath + typeMsg);

		boost::algorithm::replace_all(mappedCode,"+","P_");
		boost::algorithm::replace_all(mappedCode,"@","_");

		mappedCode = scope
			? configValue<std::string>(*scope,confPath,mappedCode,s_code,&isSet)
			: configValue<std::string>(symbolSpecs,confPath,mappedCode,NULL,s_code,&isSet);
	}

	return (isSet && (!mappedCode.empty()));
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Get infotext settings.
   */
  // ----------------------------------------------------------------------

  void textSettings(std::string & infoText,std::string & textPosition,int & maxTextWidth,int & fontSize,int & tXOffset,int & tYOffset)
  {
	// The infotext can start with text position (<area> (or shortly <a>), <top> etc.), max text width (<width=n>),
	// font size (<fontsize=n>) and x/y offset (<x/yoffset=n>) settings.

	const char * settings[] = {
		// long name		short name
		"<area>",			"<a>",
		"<center>",			"<c>",
		"<top>",			"<t>",
		"<topleft>",		"<tl>",
		"<topright>",		"<tr>",
		"<left>",			"<l>",
		"<right>",			"<r>",
		"<bottom>",			"<b>",
		"<bottomleft>",		"<bl>",
		"<bottomright>",	"<br>",
		"<width=",			"<w=",		// unsigned
		"<fontsize=",		"<f=",		// unsigned
		"<xoffset=",		"<x=",
		"<yoffset=",		"<y=",
		NULL
	};

	size_t n = 0;

	for (const char ** s = settings, ** ps = NULL; *s; )
		if (infoText.find(*s) == 0) {
			// Get the setting and erase it from the text. Just give up on invalid setting (e.g. <width=z20>, <width=> etc.)
			//
			size_t pos = infoText.find(">");

			if (pos == std::string::npos)
				return;

			if (*(*s + strlen(*s) - 1) == '=')
			{
				size_t pos2 = strlen(*s);
				const char * p = infoText.c_str() + pos2;
				const char * numSet = "-0123456789";
				const char * p2 = p + strspn(p,numSet + (((*(*s + 1) == 'w') || (*(*s + 1) == 'f')) ? 1 : 0));

				if ((*p2 != '>') || (*(p2 - 1) == '=') || (*(p2 - 1) == '-'))
					return;

				int & setting = ((*(*s + 1) == 'w') ? maxTextWidth : ((*(*s + 1) == 'f') ? fontSize : ((*(*s + 1) == 'x') ? tXOffset : tYOffset)));

				setting = atoi(infoText.substr(pos2,pos - pos2).c_str());
			}
			else {
				const char * p = (n % 2 ? *ps : *s);					// "<a>" --> "<area>" etc.
				textPosition = std::string(p).substr(1,strlen(p) - 2);	// "<area>" --> "area" etc.
			}

			infoText.erase(0,pos + 1);
			boost::algorithm::trim(infoText);

			s = settings;
			ps = NULL;
			n = 0;
		}
		else {
			ps = s;
			s++;
			n++;
		}
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Set infotext position.
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::setTextPosition(const Path & path,
		  	  	  	  	  	  	  	const std::string & TEXTPOSid,
		  	  	  	  	  	  	  	std::string & textPosition,
		  	  	  	  	  	  	  	const NFmiFillRect & infoTextRect,
									int areaWidth,int areaHeight,
		  	  	  	  	  	  	  	int textWidth,int textHeight,
		  	  	  	  	  	  	  	int tXOffset,int tYOffset,
		  	  	  	  	  	  	  	Path::BBox * boundingBox)
  {
	if ((textPosition == "area") || (textPosition == "center")) {
		texts[TEXTPOSid] << infoTextRect.first.x << "," << infoTextRect.first.y;
		return;
	}

	Path::BBox bbox = (boundingBox ? *boundingBox : path.getBBox());
	double bboxWidth = bbox.trX - bbox.blX,bboxHeight = bbox.trY - bbox.blY,x,y;
	y = bbox.blY; bbox.blY = bbox.trY; bbox.trY = y;

	if (textPosition.empty())
		// Default is top
		//
		textPosition = "top";

	if (
		(textPosition == "top") || (textPosition == "bottom") ||
		(textPosition == "topleft") || (textPosition == "topright") ||
		(textPosition == "bottomleft") || (textPosition == "bottomright")
	   )
	{
		if ((textPosition == "top") || (textPosition == "bottom"))
			x = bbox.blX + (bboxWidth / 2) - (textWidth / 2) + tXOffset;
		else
			x = ((((textPosition == "topleft") || (textPosition == "bottomleft")) ? bbox.blX : bbox.trX) - (textWidth / 2)) + tXOffset;

		y = ((textPosition.find("top") == 0) ? (bbox.trY - textHeight - tYOffset) : (bbox.blY + tYOffset));
	}
	else if ((textPosition == "left") || (textPosition == "right")) {
		x = ((textPosition == "left") ? (bbox.blX - textWidth - tXOffset) : (bbox.trX + tXOffset));
		y = bbox.trY + (bboxHeight / 2) - (textHeight / 2);
	}
	else
		throw std::runtime_error("invalid text position: '" + textPosition + "'; use top[left|right], bottom[left|right], left or right");

	if (x < tXOffset)
		x = tXOffset;
	else if ((x + textWidth + tXOffset) > areaWidth)
		x = areaWidth - textWidth - tXOffset;

	if (y < tYOffset)
		y = tYOffset;
	else if ((y + textHeight + tYOffset) > areaHeight)
		y = areaHeight - textHeight - tYOffset;

	texts[TEXTPOSid] << std::fixed << std::setprecision(1) << x << "," << y;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Surface rendering
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::render_surface(Path & path,
		  	  	  	  	  	  	   std::ostringstream & surfaceOutput,
		  	  	  	  	  	  	   const std::string & id,
		  	  	  	  	  	  	   const std::string & surfaceName,				// cloudType (for CloudArea), rainPhase (for SurfacePrecipitationArea or area type for ParameterValueSetArea)
		  	  	  	  	  	  	   const woml::Feature * feature,				// For area's infotext rendering
		  	  	  	  	  	  	   const std::list<std::string> * areaSymbols)	// Symbols to be placed into ParameterValueSetArea (warning area's symbols)
  {
	const std::string confPath("Surface");

	try {
		const char * typeMsg = " must contain a list of groups in parenthesis";
		const char * surfTypeMsg = ": surface type must be 'pattern', 'mask', 'glyph' or 'svg'";

		const libconfig::Setting & surfSpecs = config.lookup(confPath);
		if(!surfSpecs.isList())
			throw std::runtime_error(confPath + typeMsg);

		settings s_name((settings) (s_base + 0));

		// Configuration blocks to search values for the settings; global blocks (blocks with no name)
		// and the current block with matching name.
		//
		// The blocks are stored to the list in the order of appearance (and having current block
		// as the last element). The list is processed in reverse order when searching.
		//
		// Current block or global block is needed for rendering.

		std::list<const libconfig::Setting *> scope;
		bool nameMatch,noName;

		int surfIdx = -1;
		int lastIdx = surfSpecs.getLength() - 1;

		for(int i=0; i<=lastIdx; ++i)
		{
			const libconfig::Setting & specs = surfSpecs[i];
			if(!specs.isGroup())
				throw std::runtime_error(confPath + typeMsg);

			try {
				noName = false;
				nameMatch = (lookup<std::string>(specs,confPath,"name",s_name) == surfaceName);
			}
			catch (SettingIdNotFoundException & ex) {
				// Global settings have no name
				//
				noName = true;
				nameMatch = false;
			}

			// Enter the block on matching name, and for globals and last config entry
			// to load/use the globals
			//
			if (nameMatch || noName || ((i == lastIdx) && (!scope.empty()))) {
				if (nameMatch || noName) {
					if (nameMatch)
						surfIdx = i;

					scope.push_back(&specs);

					if (noName && (i < lastIdx))
						continue;
				}

				// Surface type; pattern, mask, glyph or svg

				std::string type = configValue<std::string>(scope,surfaceName,"type");
				bool filled = ((type == "fill") || (type == "fill+mask"));
				bool masked = ((type == "mask") || (type == "fill+mask"));

				if ((type == "pattern") || (type == "glyph") || filled || masked) {
					// Class and style
					//
					std::string classDef((type != "pattern") ? configValue<std::string>(scope,surfaceName,"class") : "");
					std::string style((type != "fill") ? configValue<std::string>(scope,surfaceName,"style") : "");

					// Output placeholder; by default output to passed stream

					std::string placeHolder(boost::algorithm::trim_copy(configValue<std::string>(scope,surfaceName,"output",s_optional)));
					std::ostringstream & surfaces = (placeHolder.empty() ? surfaceOutput : texts[placeHolder]);

					if (type == "pattern")
						surfaces << "<g id=\""
								 << id
								 << "\">\n"
								 << "<path style=\""
								 << style
								 << "\" d=\"" << path.svg() << "\"/>\n"
								 << "</g>\n";
					else {
						std::string pathId(id + type);

						paths << "<path id=\""
							  << pathId
							  << "\" d=\"" << path.svg() << "\"/>\n";

						surfaces << "<use class=\""
								 << classDef
								 << "\" xlink:href=\"#"
								 << pathId << ((filled && (!masked)) ? "\"/>\n" : "");

						if (masked) {
							std::string maskId(pathId + "mask");

							masks << "<mask id=\""
								  << maskId
								  << "\" maskUnits=\"userSpaceOnUse\">\n"
								  << "<use xlink:href=\"#"
								  << pathId
								  << "\" class=\""
								  << style
								  << "\"/>\n</mask>\n";

							surfaces << "\" mask=\"url(#"
									 << maskId
									 << ")\"/>\n";
						}

						if (filled) {
							// Fill symbol width and height and scale factor for bounding box for
							// calculating suitable symbol positions
							//
							int _width = configValue<int>(scope,surfaceName,"width");
							int _height = configValue<int>(scope,surfaceName,"height");
							float _scale = configValue<float>(scope,surfaceName,"scale");

							if (_scale < symbolBBoxFactorMin)
								throw std::runtime_error(confPath + ": minimum scale is " + boost::lexical_cast<std::string>(symbolBBoxFactorMin));

							// If autoscale is true (default: false) symbol is shrinken until
							// at least one symbol or all area symbols can be placed on the surface
							bool isSet;
							bool autoScale = configValue<bool>(scope,surfaceName,"autoscale",s_optional,&isSet);
							if (!isSet)
								autoScale = false;

							// If mode is 'vertical' (default: horizontal) the symbols are placed using y -axis as the
							// primary axis (using maximum vertical fillrects instead of maximum horizontal fillrects)
							std::string mode = configValue<std::string>(scope,surfaceName,"mode",s_optional);
							bool verticalRects = (mode == "vertical");

							// If direction is 'both' (default: up) iterating vertically over the surface y-coordinates
							// from both directions; otherwise from bottom up
							std::string direction = configValue<std::string>(scope,surfaceName,"direction",s_optional);
							bool scanUpDown = (direction == "both");

							// If showareas is true (default: false) output the fill area bounding boxes too
							bool showAreas = configValue<bool>(scope,surfaceName,"showareas",s_optional,&isSet);
							if (!isSet)
								showAreas = false;

							// Get mapping for ParameterValueSetArea's symbols; code_<symCode> = <code>

							std::list<std::string> fillSymbols;

							if (areaSymbols) {
								std::list<std::string>::const_iterator sym = areaSymbols->begin();
								std::string code;

								for ( ; (sym != areaSymbols->end()); sym++)
									if (symbolMapping(config,confPath,"code_" + *sym,s_optional,code,&scope))
										fillSymbols.push_back(code);

								if (fillSymbols.size() < areaSymbols->size())
									;
							}

							NFmiFillRect infoTextRect(std::make_pair(Point(1,1),Point(0,0)));
							const std::string & infoText = (feature ? feature->text(options.locale) : "");
							std::string TEXTPOSid("Z0TEXTPOS_" + id),textPosition;
							int areaWidth = static_cast<int>(std::floor(0.5+area->Width()));
							int areaHeight = static_cast<int>(std::floor(0.5+area->Height()));
							int textWidth = 0,textHeight = 0,maxTextWidth = 0,fontSize = 0,tXOffset = 0,tYOffset = 0;

							if ((!infoText.empty()) && (infoText != options.locale)) {
								// Render feature's infotext. The text is rendered starting from coordinate (0,0) and
								// final position is selected afterwards. The final position is set as transformation
								// offsets to the selected position.
								//
								// Note: The key for position must sort after the key for the text (the key/text
								//		 containing the position key must be handled/outputted prior the position key/text);
								//		 therefore the position key starts with "Z0"
								//
								std::string textOut(boost::algorithm::trim_copy(infoText));

								textPosition = configValue<std::string>(scope,surfaceName,"textposition",s_optional);

								// The text can start with position, fontsize and x/y offset settings overriding the configured values.

								textSettings(textOut,textPosition,maxTextWidth,fontSize,tXOffset,tYOffset);

								render_text(texts,confPath,surfaceName,NFmiStringTools::UrlDecode(textOut),textWidth,textHeight,true,false,false,TEXTPOSid,false,&maxTextWidth,&fontSize,&tXOffset,&tYOffset);

								if (textPosition == "area") {
									// Get text position within the area
									//
									NFmiFillMap fmap;
									NFmiFillAreas areas;
									path.length(&fmap);

									if (fmap.getFillAreas(areaWidth,areaHeight,textWidth,textHeight,1.0,verticalRects,areas,false,scanUpDown))
										infoTextRect = getCenterFillAreaRect(areas,textWidth,1.0);
									else
										// No room within the area, using the defaut location
										//
										textPosition.clear();
								}
							}

							// Get symbol fill areas.
							//
							// If none available or not enough room for all area symbols, and autoscale is set,
							// retry with smaller symbol size. If still not enough room, retry without placing the
							// infotext into the area, by using smaller symbol bbox and by scaling the surface bigger
							//
							NFmiFillMap fmap;
							NFmiFillAreas areas;
							NFmiFillPositions fpos;
							path.length(&fmap);

							int width = _width;
							int height = _height;
							double scale = _scale;
							size_t pathScalingOffset = 0;
							bool noTextRetry = false,sizeOk,ok;

							// If symbol bounding box (spacing) is increased, first scale up the surface to get
							// the outermost symbols positioned near enough the surface border.
							//
							// The scaled surface will not be rendered, it is used only to get fill areas.

							if ((scale >= 1) && autoScale) {
								NFmiFillMap fmapScaled;

								pathScalingOffset = (((scale - 1.0) * 30) + 12);

								path = path.scale(std::min(pathScalingOffset,(pathScalingSymbolHeightFactorMax * height)));
								path.length(&fmapScaled);
								fmap = fmapScaled;
							}

							do {
								do {
									areas.clear();

									ok = sizeOk = fmap.getFillAreas(areaWidth,areaHeight,width,height,scale,verticalRects,areas,true,scanUpDown);

									if (ok && (fillSymbols.size() > 0)) {
										// Check there is room for all area symbols. Erase fill areas overlapping the area
										// reserved for info text.
										//
										// Note: To get fillareas with width near or equal to twice (etc) the symbol width splitted, decreasing scale
										//		 slightly (this needs some checking/thinking later through)
										//
										splitFillAreas(areas,width,scale - 0.1);

										NFmiFillAreas::iterator iter;
										size_t symCnt = 0;

										for (iter = areas.begin(), fpos.clear(); ((iter != areas.end()) && (fpos.size() < fillSymbols.size())); )
											if (getFillPositions(iter,width,height,scale,infoTextRect,fpos,symCnt))
												iter++;
											else
												iter = areas.erase(iter);

										ok = (fpos.size() >= fillSymbols.size());
									}

									if (!ok) {
										// Render infotext within the area only when all symbols fit in.
										//
										int nw = width - 2,nh = height - 2;

										sizeOk = ((nw >= floor(_width * markerScaleFactorMin)) && (nh >= floor(_height * markerScaleFactorMin)));
										noTextRetry = false;

										if (sizeOk || (textPosition != "area")) {
											if (sizeOk) {
												// First trying with smaller symbol size
												//
												width = nw;
												height = nh;
											}
											else if ((fillSymbols.size() > 0) && (scale >= (symbolBBoxFactorMin + 0.1)))
												// Thirdly trying with smaller symbol bounding box, allowing (more) overlap
												//
												scale -= 0.1;
											else
												// Not enough room. Break the inner loop and scale up the surface
												//
												break;
										}
										else {
											// Secondly trying without the infotext within the area
											//
											textPosition.clear();
											infoTextRect = std::make_pair(Point(0,0),Point(0,0));

											width = _width;
											height = _height;

											noTextRetry = true;
										}
									}
								}
								while ((autoScale || noTextRetry) && (! ok) && sizeOk);

								if ((!ok) && autoScale) {
									// Scale up the surface
									//
									pathScalingOffset += 2;

									if (pathScalingOffset <= (pathScalingSymbolHeightFactorMax * height)) {
										NFmiFillMap fmapScaled;

										path = path.scale(2);
										path.length(&fmapScaled);
										fmap = fmapScaled;
									}
								}
							}
							while (autoScale && (! ok) && (pathScalingOffset <= (pathScalingSymbolHeightFactorMax * height)));

							if (!ok) {
								// For warning areas fit max 4 symbols per fill area
								//
								if (fillSymbols.size() == 0)
									areas.clear();
								else if ((!autoScale) || ((4 * fpos.size()) < fillSymbols.size()))
									throw std::runtime_error("render_surface: too many symbols");
							}
//if (pathScalingOffset > 0) {
//paths << "<path id=\""
//	  << pathId + "C"
//	  << "\" d=\"" << path.svg() << "\"/>\n";
//
//surfaces << "<use class=\""
//		 << classDef
//		 << "\" xlink:href=\"#"
//		 << pathId  + "C" << ((filled && (!masked)) ? "\"/>\n" : "");
//
//if (masked) {
//	std::string maskId(pathId + "Cmask");
//
//	masks << "<mask id=\""
//		  << maskId
//		  << "\" maskUnits=\"userSpaceOnUse\">\n"
//		  << "<use xlink:href=\"#"
//		  << pathId + "C"
//		  << "\" class=\""
//		  << style
//		  << "\"/>\n</mask>\n";
//
//	surfaces << "\" mask=\"url(#"
//			 << maskId
//			 << ")\"/>\n";
//}
//}

							fpos.clear();

							// Set final infotext position

							if (!infoText.empty())
								setTextPosition(path,TEXTPOSid,textPosition,infoTextRect,areaWidth,areaHeight,textWidth,textHeight,tXOffset,tYOffset);

							// Get symbol positions withing the fill areas

							const char *clrs[] =
							{ "red",
							  "blue",
							  "green",
							  "orange",
							  "brown",
							  "yellow"
							};
							size_t symCnt = 0,clrCnt = (sizeof(clrs) / sizeof(char *)),clrIdx = 0;

							if (!areaSymbols || (areas.size() < 2)) {
								for (NFmiFillAreas::const_iterator iter = areas.begin(); (iter != areas.end()); iter++) {
									getFillPositions(iter,width,height,scale,infoTextRect,fpos,symCnt);

									if (showAreas) {
										// Draw fill area rects
										//
										surfaces << "<rect x=\""
												 << iter->first.x
												 << "\" y=\""
												 << iter->first.y
												 << "\" width=\""
												 << iter->second.x - iter->first.x
												 << "\" height=\""
												 << iter->second.y - iter->first.y
												 << "\" fill=\"" << clrs[clrIdx % clrCnt] << "\"/>\n";
										clrIdx++;
									}
								}
							}
							else if (fillSymbols.size() > 0) {
								// fillMode controls whether to get one position for each fill symbol (the default) or
								// for each column, or to get all or given percentage of the available positions
								//
								std::string fillMode = configValue<std::string>(scope,surfaceName,"fillmode",s_optional);

								getFillPositions(areas,width,height,fillSymbols.size(),scale,infoTextRect,fpos,fillMode);

								if (showAreas)
									// Draw fill area rects
									//
									for (NFmiFillAreas::const_iterator iter = areas.begin(); (iter != areas.end()); iter++) {
										surfaces << "<rect x=\""
												 << iter->first.x
												 << "\" y=\""
												 << iter->first.y
												 << "\" width=\""
												 << iter->second.x - iter->first.x
												 << "\" height=\""
												 << iter->second.y - iter->first.y
												 << "\" fill=\"" << clrs[clrIdx % clrCnt] << "\"/>\n";
										clrIdx++;
								}
							}

							// Render the symbols
							render_symbol(confPath,pointsymbols,surfaceName,"",0,0,NULL,NULL,&fpos,(fillSymbols.size() > 0) ? &fillSymbols : NULL,width,height);
						}
						else if (!masked) {
							// glyph
							std::string _glyph("U");
							double textlength = static_cast<double>(_glyph.size());
							double len = path.length();
							double fontsize = getCssSize(".cloudborderglyph","font-size");
							double spacing = 0.0;

							const int CosmologicalConstant = 2;
							int nglyphs = CosmologicalConstant * static_cast<int>(std::floor(len/(fontsize*textlength+spacing)+0.5));

							if((textlength > 0) && (nglyphs > 0))
							{
								std::string glyph(_glyph.size() * nglyphs,' ');
								std::string spaces(_glyph.size(),' ');
								boost::replace_all(glyph,spaces,_glyph);

								surfaces << "\"/>\n<text>\n"
										 << "<textPath class=\""
										 << style
										 << "\" xlink:href=\"#"
										 << pathId
										 << "\">\n"
										 << glyph
										 << "\n</textPath>"
//											 << "<!-- len=" << len << " textlength=" << textlength << " fontsize=" << fontsize << " -->"
										 << "\n</text>\n";
							}
							else
								surfaces << "\"/>\n";
						}
					}

					return;
				}
				else if (type == "svg") {
					throw std::runtime_error(confPath + " type 'svg' not implemented yet");
				}
				else
					throw std::runtime_error(confPath + ": '" + type + "'" + surfTypeMsg);
			}
		}	// for

		if (options.debug) {
			// No (locale) settings for the surface
			//
			const char * p = ((surfIdx < 0) ? "Settings for " : "Locale specific settings for " );

			debugoutput << p
						<< surfaceName
						<< " not found\n";
		}
	}
	catch (libconfig::SettingNotFoundException & ex) {
		// No settings for confPath
		//
		if (options.debug)
			debugoutput << "Settings for "
						<< confPath
						<< " ("
						<< surfaceName
						<< ") not found\n";
	}
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Utility function for getting conditional settings based on numerical value.
   * 		Returns the configuration group/block on success or NULL on not found.
   */
  // ----------------------------------------------------------------------

  const libconfig::Setting * matchingCondition(const libconfig::Config & config,
		  	  	  	  	  	  	  	  	  	   const libconfig::Setting & condSpecs,
		  	  	  	  	  	  	  	  	  	   const std::string & confPath,
		  	  	  	  	  	  	  	  	  	   const std::string & className,
		  	  	  	  	  	  	  	  	  	   int specsIdx,
		  	  	  	  	  	  	  	  	  	   const std::string & parameter,
		  	  	  	  	  	  	  	  	  	   const double numericValue)
  {
	const char * typeMsg = " must contain a list of groups in parenthesis";
	const char * condMsg = ": refix must be 'eq', 'le', 'lt', 'ge' or 'gt'";
	const char * valMsg = ": value must be numeric or 'NaN'";

	double ltCondValue = 0.0,gtCondValue = 0.0,eps = 0.00001;
	int ltCondIdx = -1,gtCondIdx = -1,idx = -1;

	for(int i=0; i<condSpecs.getLength(); i++)
	{
		const libconfig::Setting & conds = condSpecs[i];
		if(!conds.isGroup())
			throw std::runtime_error(confPath + ".conditions" + typeMsg);

		std::string refix(configValue<std::string>(conds,className,"refix"));
		double condValue = 0.0;
		bool eqNan = false,badNan = false;

		// Check for equal to NaN condition.

		try {
			badNan = (boost::algorithm::to_lower_copy(lookup<std::string>(conds,confPath,"value")) != "nan");

			if (badNan)
				throw std::runtime_error(confPath + ".conditions" + valMsg);

			if (!isnan(numericValue))
				continue;

			eqNan = true;
		}
		catch (std::runtime_error ex) {
			if (badNan)
				throw;

			condValue = configValue<double>(conds,className,"value");
		}
		catch (...) {
			throw;
		}

		if (refix == "eq") {
			if (eqNan || (fabs(condValue - numericValue) <= eps)) {
				ltCondIdx = gtCondIdx = i;
				break;
			}
		}
		else if (eqNan)
			// Only eq match for NaN.
			;
		else if (refix == "le") {
			if (
				(
				 (condValue >= numericValue)
				 || (fabs(condValue - numericValue) <= eps)
				) &&
				((ltCondIdx < 0) || (condValue < ltCondValue))
			   ) {
				ltCondValue = condValue;
				ltCondIdx = i;
			}
		}
		else if (refix == "lt") {
			if ((condValue > numericValue) && ((ltCondIdx < 0) || (condValue < ltCondValue))) {
				ltCondValue = condValue;
				ltCondIdx = i;
			}
		}
		else if (refix == "ge") {
			if (
				(
				 (condValue <= numericValue)
				 || (fabs(condValue - numericValue) <= eps)
				) &&
				((gtCondIdx < 0) || (condValue > gtCondValue))
			   ) {
				gtCondValue = condValue;
				gtCondIdx = i;
			}
		}
		else if (refix == "gt") {
			if ((condValue < numericValue) && ((gtCondIdx < 0) || (condValue > gtCondValue))) {
				gtCondValue = condValue;
				gtCondIdx = i;
			}
		}
		else
			throw std::runtime_error(confPath + ".conditions: '" + refix + "'" + condMsg);
	}

	if ((ltCondIdx >= 0) || (gtCondIdx >= 0)) {
		idx = ((ltCondIdx < 0) || ((gtCondIdx >= 0) && ((numericValue - ltCondValue) > (gtCondValue - numericValue))))
			  ? gtCondIdx
			  : ltCondIdx;

		return &(condSpecs[idx]);
	}

	return NULL;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Utility function for getting conditional settings based on datetime value.
   * 		Returns the configuration group/block on success or NULL on not found.
   */
  // ----------------------------------------------------------------------

  const libconfig::Setting * matchingCondition(const libconfig::Config & config,
		  	  	  	  	  	  	  	  	  	   const libconfig::Setting & condSpecs,
		  	  	  	  	  	  	  	  	  	   const std::string & confPath,
		  	  	  	  	  	  	  	  	  	   const std::string & className,
		  	  	  	  	  	  	  	  	  	   int specsIdx,
		  	  	  	  	  	  	  	  	  	   const std::string & parameter,
		  	  	  	  	  	  	  	  	  	   const boost::posix_time::ptime & dateTimeValue)
  {
	const char * typeMsg = " must contain a list of groups in parenthesis";
	const char * condMsg = ": refix must be 'range'";
	const char * rngMsg = ": hh[:mm]-hh[:mm] range expected";

	for(int i=0; i<condSpecs.getLength(); i++)
	{
		const libconfig::Setting & conds = condSpecs[i];
		if(!conds.isGroup())
			throw std::runtime_error(confPath + ".conditions" + typeMsg);

		std::string refix(configValue<std::string>(conds,className,"refix",NULL,s_optional));
		std::string condValue(configValue<std::string>(conds,className,"value"));
		std::string rngErr;

		// Currently only hh[:mm]-hh[:mm] range comparison supported
		//
		if ((refix == "") || (refix == "range")) {
			// Get hh[:mm]-hh[:mm] time range
			//
			std::vector<std::string> defs;
			boost::split(defs,condValue,boost::is_any_of("-"));

			if (defs.size() == 2) {
				std::vector<std::string> loRng;
				boost::trim(defs[0]);
				boost::split(loRng,defs[0],boost::is_any_of(":"));

				std::vector<std::string> hiRng;
				boost::trim(defs[1]);
				boost::split(hiRng,defs[1],boost::is_any_of(":"));

				int lh,lm,hh,hm;

				if ((loRng.size() <= 2) || (hiRng.size() <= 2)) try {
					lh = boost::lexical_cast<unsigned int>(loRng[0]);
					lm = ((loRng.size() == 2) ? boost::lexical_cast<unsigned int>(loRng[1]) : 0);
					hh = boost::lexical_cast<unsigned int>(hiRng[0]);
					hm = ((hiRng.size() == 2) ? boost::lexical_cast<unsigned int>(hiRng[1]) : 0);

					if ((lh <= 23) && (lm <= 59) && (hh <= 23) && (hm <= 59)) {
						// Build lo and hi range timestamps using validtime's date and given time range
						//
						boost::posix_time::ptime ldt(dateTimeValue.date(),boost::posix_time::hours(lh) + boost::posix_time::minutes(lm));
						boost::posix_time::ptime hdt(dateTimeValue.date(),boost::posix_time::hours(hh) + boost::posix_time::minutes(hm));

						// Advance hi range to next day if less than or equal to lo range (eg. range 18-06)

						if (hdt <= ldt)
							hdt += boost::gregorian::days(1);

						// Advance validtime to next day if less than lo range

						boost::posix_time::ptime vdt(dateTimeValue);
						if (vdt < ldt)
							vdt += boost::gregorian::days(1);

						if ((vdt >= ldt) && (vdt <= hdt)) {
							// Validtime is between the range; check if the parameter is available
							//
							std::string condPath(confPath +
											     ((specsIdx >= 0) ? (".[" + boost::lexical_cast<std::string>(specsIdx) + "]") : "") + ".conditions" +
												 ".[" + boost::lexical_cast<std::string>(i) + "]." + parameter);

							if (parameter.empty() || config.exists(condPath))
								return &(condSpecs[i]);
						}

						continue;
					}
				}
				catch(std::exception & ex) {
					rngErr = std::string(": ") + ex.what();
				}
			}

			throw std::runtime_error(confPath + ".conditions: '" + condValue + "'" + rngMsg + rngErr);
		}
		else
			throw std::runtime_error(confPath + ".conditions: '" + refix + "'" + condMsg);
	}

	return NULL;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Utility function for getting conditional settings based on string value.
   * 		Returns the configuration group/block on success or NULL on not found.
   */
  // ----------------------------------------------------------------------

  const libconfig::Setting * matchingCondition(const libconfig::Config & config,
		  	  	  	  	  	  	  	  	  	   const libconfig::Setting & condSpecs,
		  	  	  	  	  	  	  	  	  	   const std::string & confPath,
		  	  	  	  	  	  	  	  	  	   const std::string & className,
		  	  	  	  	  	  	  	  	  	   int specsIdx,
		  	  	  	  	  	  	  	  	  	   const std::string & parameter,
		  	  	  	  	  	  	  	  	  	   const std::string & stringValue)
  {
	const char * typeMsg = " must contain a list of groups in parenthesis";
	const char * condMsg = ": refix must be 'eq'";
	const char * valMsg = ": string value expected";

	std::string s(boost::algorithm::to_lower_copy(stringValue));

	int i;

	for(i=0; i<condSpecs.getLength(); i++)
	{
		const libconfig::Setting & conds = condSpecs[i];
		if(!conds.isGroup())
			throw std::runtime_error(confPath + ".conditions" + typeMsg);

		std::string refix("eq"/*configValue<std::string>(conds,className,"refix")*/),condValue;

		if (refix == "eq") {
			try {
				condValue = boost::algorithm::to_lower_copy(lookup<std::string>(conds,confPath,"value"));
			}
			catch (...) {
				throw std::runtime_error(confPath + ".conditions" + valMsg);
			}

			if (condValue == s) {
				break;
			}
		}
		else
			throw std::runtime_error(confPath + ".conditions: '" + refix + "'" + condMsg);
	}

	if (i<condSpecs.getLength())
		return &(condSpecs[i]);

	return NULL;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Utility function for getting conditional settings
   */
  // ----------------------------------------------------------------------

  template <typename T>
  const libconfig::Setting * matchingCondition(const libconfig::Config & config,
		  	  	  	  	  	  	  	  	  	   const std::string & confPath,
		  	  	  	  	  	  	  	  	  	   const std::string & className,
		  	  	  	  	  	  	  	  	  	   const std::string & parameter,
		  	  	  	  	  	  	  	  	  	   int specsIdx,
		  	  	  	  	  	  	  	  	  	   const T & condValue)
  {
	const char * typeMsg = " must contain a list of groups in parenthesis";

	std::string condPath(confPath + ((specsIdx >= 0) ? (".[" + boost::lexical_cast<std::string>(specsIdx) + "]") : "") + ".conditions");

	if (!config.exists(condPath))
		return NULL;

	const libconfig::Setting & condSpecs = config.lookup(condPath);
	if(!condSpecs.isList())
		throw std::runtime_error(confPath + ".conditions" + typeMsg);

	return matchingCondition(config,condSpecs,confPath,className,specsIdx,parameter,condValue);
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Utility function for obtaining symbol code and optional folder; [<folder>/]<code>.
   */
  // ----------------------------------------------------------------------

  std::string getFolderAndSymbol(const libconfig::Config & config,
		  	  	  	  	  	  	 std::list<const libconfig::Setting *> * scope,
		  	  	  	  	  	  	 const std::string & confPath,
		  	  	  	  	  	  	 const std::string & symClass,
		  	  	  	  	  	  	 const std::string & symCode,
		  	  	  	  	  	  	 settings s_code,
		  	  	  	  	  	  	 const boost::posix_time::ptime & dateTimeValue,
		  	  	  	  	  	  	 std::string & folder,
		  	  	  	  	  	  	 bool commonCode = false)
  {
	const char * codeMsg = ": symbol code: [<folder>/]<code> expected";

	// Search for the code truncating the scope to have the matching block as the last block
	//
	// If commonCode is set, using primarily plain 'code' key instead given symCode (containing code_<symCode>);
	// used currently to conditionally map all wind symbols to 'calm' if wind speed is nan

	std::string code;

	if (scope && commonCode)
		code = configValue<std::string,int>(*scope,symClass,"code",s_optional,NULL,true);

	if (code.empty())
		code = (scope ? configValue<std::string,int>(*scope,symClass,symCode,s_code,NULL,true) : symCode);

	std::vector<std::string> cols;
	boost::split(cols,code,boost::is_any_of("/"));

	int codeIdx;
	boost::trim(cols[codeIdx = 0]);

	if (cols.size() == 2)
		boost::trim(cols[codeIdx = 1]);

	if ((cols.size() > 2) || (cols[0] == "") || (cols[codeIdx] == ""))
		throw std::runtime_error(confPath + ": '" + code + "'" + codeMsg);

	folder = ((codeIdx == 1) ? cols[0] : (scope ? "" : folder));

	return cols[codeIdx];
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Utility function for formatting
   */
  // ----------------------------------------------------------------------

  std::string formattedValue(const woml::NumericalSingleValueMeasure * lowerLimit,
		  	  	  	  	     const woml::NumericalSingleValueMeasure * upperLimit,
		  	  	  	  	     const std::string & confPath,
		  	  	  	  	     const std::string & pref,
		  	  	  	  	     double * scale = NULL)
  {
	if (pref.empty())
		return upperLimit ? (lowerLimit->value() + ".." + upperLimit->value()) : (isnan(lowerLimit->numericValue()) ? "" : lowerLimit->value());
	else if (pref.find('%') == std::string::npos)
		// Static/literal format
		//
		return svgescapetext(pref,true);

	std::ostringstream os;

	if (scale && (*scale == 0.0))
		scale = NULL;

	try {
		if (upperLimit)
			if (scale)
				os << boost::format(pref) % (lowerLimit->numericValue() / *scale) % (upperLimit->numericValue() / *scale);
			else
				os << boost::format(pref) % lowerLimit->numericValue() % upperLimit->numericValue();
		else
			os << boost::format(pref) % (scale ? (lowerLimit->numericValue() / *scale) : lowerLimit->numericValue());

		return svgescapetext(os.str(),true);
	}
	catch(std::exception & ex) {
		throw std::runtime_error(confPath + ": '" + pref + "': " + ex.what());
	}
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Aerodrome symbol rendering
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::render_aerodromeSymbol(const std::string & confPath,
		  	  	  	  	  	  	  	  	   const std::string & symClass,
		  	  	  	  	  	  	  	  	   const std::string & classNameExt,
		  	  	  	  	  	  	  	  	   const std::string & value,
		  	  	  	  	  	  	  	  	   double x,double y,
		  	  	  	  	  	  	  	  	   bool codeValue,bool mappedCode,
		  	  	  	  	  	  	  	  	   double xScale,double yScale,
		  	  	  	  	  	  	  	  	   const std::string symbolPosIdN)
  {
	// Nothing to do with empty value

	std::string code = (mappedCode ? value : boost::algorithm::trim_copy(value));

	if (code.empty())
		return;

	try {
		const char * typeMsg = " must contain a group in curly brackets";

		const libconfig::Setting & symbolSpecs = config.lookup(confPath);
		if(!symbolSpecs.isGroup())
			throw std::runtime_error(confPath + typeMsg);

		// Class

		std::string class1 = configValue<std::string>(symbolSpecs,confPath,"class");
		boost::trim(class1);

		std::string uri;
		std::string class2;

		if (codeValue) {
			// Get symbol's code if not already mapped; code_<symCode> = <code>
			//
			if (!mappedCode) {
				settings s_code((settings) (s_base + 0));
				symbolMapping(config,confPath,"code_" + code,s_code,code);
			}

			// Url

			uri = configValue<std::string>(symbolSpecs,confPath,"url");
			boost::algorithm::replace_all(uri,"%symbol%",code + classNameExt);

			// Use last of the given classes for symbol specific class reference

			std::vector<std::string> classes;
			boost::split(classes,class1,boost::is_any_of(" "));

			class2 = (class1.empty() ? "" : " ") + classes[classes.size() - 1] + code;
		}

		// Scale

		bool hasScale;
		double scale = (double) configValue<double,int>(symbolSpecs,confPath,"scale",NULL,s_optional,&hasScale),scaleX,scaleY;

		if (xScale > 0.0) {
			scaleX = xScale;
			scaleY = yScale;

			if (hasScale) {
				scaleX *= scale;
				scaleY *= scale;
			}

			hasScale = true;
		}
		else
			scaleX = scaleY = scale;

		std::ostringstream transf;
		std::string scaleIdN(symbolPosIdN + "SCALE");

		if (symbolPosIdN.empty())
			transf << "translate(" << std::fixed << std::setprecision(1) << x << "," << y << ")";
		else
			transf << "translate(--" << symbolPosIdN << "--)";

		if (hasScale) {
			if (symbolPosIdN.empty())
				transf << " scale(" << std::fixed << std::setprecision(1) << scale << ")";
			else
				transf << " --" << scaleIdN << "--";
		}

		std::string id = confPath + boost::lexical_cast<std::string>(npointsymbols);
		npointsymbols++;

		if (codeValue)
			texts[symClass] << "<g id=\"" << id
							<< "\" class=\"" << class1 << class2
							<< "\" transform=\"" << transf.str() << "\">\n"
							<< "<use xlink:href=\"" << uri << "\"/>\n"
							<< "</g>\n";
		else
			texts[symClass] << "<g id=\"" << id
							<< "\" class=\"" << class1
							<< "\" transform=\"" << transf.str() << "\">\n"
							<< "<text>" << value << "</text>\n"
							<< "</g>\n";

		if (!symbolPosIdN.empty()) {
			texts[symbolPosIdN] << x << "," << y;
			texts[scaleIdN] << "scale(" << scaleX << "," << scaleY << ")";
		}

		return;
	}
	catch (SettingIdNotFoundException & ex) {
		// Symbol code not found
		//
//			if (options.debug)
//				debugoutput << "Setting for "
//							<< confPath
//							<< ".code_"
//							<< symCode
//							<< " not found\n";

    	throw std::runtime_error("Setting for " + confPath + ".code_" + value + " not found");
	}
	catch (libconfig::ConfigException & ex) {
		if (!config.exists(confPath)) {
			// No settings for confPath
			//
//			if (options.debug)s
//				debugoutput << "Settings for "
//							<< confPath
//							<< " not found\n";

			throw std::runtime_error("Settings for " + confPath + " are missing");
		}

	    throw ex;
	}
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Aerodrome symbol rendering
   */
  // ----------------------------------------------------------------------

  template <typename T>
  void SvgRenderer::render_aerodromeSymbols(const T & theFeature,
		  	  	  	  	  	  	  	  	    const std::string & confPath,
		  	  	  	  	  	  	  	  	    bool setVisibility)
  {
	// Document's time period

	const boost::posix_time::time_period & tp = axisManager->timePeriod();

	// Get the elevations from the time serie

	ElevGrp eGrp;
	boost::posix_time::ptime bt,et;

	bool ground = (elevationGroup(theFeature.timeseries(),bt,et,eGrp) == t_ground);

	// Do not return on empty group if rendering visibility information too (MigratoryBirds)

	if (eGrp.size() == 0) {
		if (setVisibility)
			ground = false;
		else
			return;
	}

	// Render the symbols

	try {
		const char * grouptypeMsg = " must contain a group in curly brackets";

		const libconfig::Setting & specs = config.lookup(confPath);
		if(!specs.isGroup())
			throw std::runtime_error(confPath + grouptypeMsg);

		ElevGrp::const_iterator egend = eGrp.end(),iteg;
		const std::string FEATURE(boost::algorithm::to_upper_copy(confPath));
		const std::string classDef = (ground ? "" : configValue<std::string>(specs,confPath,"class"));
		const std::string symClass(boost::algorithm::to_upper_copy(confPath + theFeature.classNameExt()));
		bool visible = false,aboveTop = false;

		for (iteg = eGrp.begin(); (iteg != egend); iteg++) {
			// Note: Using the lo-hi range average value for "nonground" elevations (MigratoryBirds) and
			// 		 0 for "ground" elevation (SurfaceVisibility, SurfaceWeather)
			//
			boost::posix_time::ptime vt = iteg->validTime();

			if (vt < tp.begin())
				continue;
			else if (vt > tp.end())
				break;

			double x = axisManager->xOffset(vt),y = 0.0;

			// Category/magnitude or visibility (meters)

			const woml::CategoryValueMeasure * cvm = dynamic_cast<const woml::CategoryValueMeasure *>(iteg->Pv()->value());
			const woml::NumericalSingleValueMeasure * nsv = (cvm ? NULL : dynamic_cast<const woml::NumericalSingleValueMeasure *>(iteg->Pv()->value()));

			if ((!cvm) && (!nsv))
				throw std::runtime_error("render_aerodromeSymbol: " + confPath + ": CategoryValueMeasure or NumericalSingleValueMeasure values expected");

			if (!ground) {
				// Note: For "nonground" elevations (MigratoryBirds) checking for conditional y -position based on the value (category);
				//		 used to position migration type (text 'morning', 'arctic' etc. as symbol) relatively to the bird symbol or to
				//		 fixed y -position (above or below the x -axis).
				//
				//		 Conditions are expected to exist only for the texts, not for the actual bird symbols;
				//		 visibility related flags are not set if condition exists.
				//
				const libconfig::Setting * condSpecs = NULL;
				int yPos = 0;

				if (cvm) {
					condSpecs = matchingCondition(config,confPath,confPath,"",-1,cvm->value());

					if (condSpecs) {
						yPos = configValue<int>(*condSpecs,confPath,"ypos");

						bool isSet;
						bool absolute = configValue<bool>(*condSpecs,confPath,"absolute",NULL,s_optional,&isSet);

						if ((!isSet) || absolute) {
							// svg y -axis grows downwards; use axis height to transform the y -coordinate
							//
							yPos = axisManager->axisHeight() - yPos;
						}
						else
							// Position is relative; clear condSpecs to use the elevation
							//
							condSpecs = NULL;
					}
				}

				if (!condSpecs) {
					const woml::Elevation & e = iteg->Pv()->elevation();
					boost::optional<woml::NumericalSingleValueMeasure> itsBoundedLo = (e.bounded() ? e.lowerLimit() : woml::NumericalSingleValueMeasure());
					const boost::optional<woml::NumericalSingleValueMeasure> & itsLoLimit = (e.bounded() ? itsBoundedLo : e.value());
					boost::optional<woml::NumericalSingleValueMeasure> itsBoundedHi = (e.bounded() ? e.upperLimit() : woml::NumericalSingleValueMeasure());
					const boost::optional<woml::NumericalSingleValueMeasure> & itsHiLimit = (e.bounded() ? itsBoundedHi : e.value());

					double lo = ((!itsLoLimit) ? 0.0 : itsLoLimit->numericValue());
					double hi = ((!itsHiLimit) ? 0.0 : itsHiLimit->numericValue());

					bool above = false;
					y = axisManager->scaledElevation(lo + ((hi - lo) / 2),&above);

					if (y < 0) {
						if (above)
							aboveTop = true;

						continue;
					}
				}

				y += yPos;

				if ((!condSpecs) && ((y > 0) && (y < axisManager->axisHeight())))
					visible = true;

				// Move first and last time instant's symbols to keep them better withing the area

				if (vt == tp.begin()) {
					// Offset in px for moving first time instant's symbols to the right
					//
					bool isSet;
					int sOffset = configValue<int>(specs,confPath,"soffset",NULL,s_optional,&isSet);

					if (isSet)
						x += sOffset;
				}
				else if (vt == tp.end()) {
					// Offset in px for moving last time instant's symbols to the left
					//
					bool isSet;
					int eOffset = configValue<int>(specs,confPath,"eoffset",NULL,s_optional,&isSet);

					if (isSet)
						x -= eOffset;
				}
			}

			// Render the symbol or visibility value

			if (cvm)
				// Symbol
				//
				render_aerodromeSymbol(confPath,symClass,theFeature.classNameExt(),cvm->value(),x,y);
			else {
				// Format visibility based on it's value
				//
				// The default format is "%.1f KM" upto visibilityPrec100m and "%.0f KM" above it
				//
				double scale = 1000.0,value = nsv->numericValue();
				std::string pref;

				const libconfig::Setting * condSpecs = matchingCondition(config,confPath,confPath,"",-1,value);

				if (condSpecs)
					pref = configValue<std::string>(*condSpecs,confPath,"pref",&specs);
				else
					pref = configValue<std::string>(specs,confPath,"pref",NULL,s_optional);

				if (pref.empty()) {
					const double visibilityPrec100m = 5000.0;
					pref = ((value <= visibilityPrec100m) ? "%.1f KM" : "%.0f KM");
				}

				render_aerodromeSymbol(confPath,symClass,theFeature.classNameExt(),formattedValue(nsv,NULL,confPath,pref,&scale),x,y,false);
			}
		}

		// Visibility information for "nonground" data (MigratoryBirds)

		if (!ground) {
			if (!visible) {
				if (!aboveTop) {
					// Show the fixed template text "No xxx"
					//
					const std::string NOFEATURE(FEATURE + "NO" + FEATURE);
					texts[NOFEATURE] << classDef << "Visible";
				}
				else {
					// Show the fixed template text "xxx are located in the upper"
					//
					const std::string FEATUREUPPER(FEATURE + "INTHEUPPER");
					texts[FEATUREUPPER] << classDef << "Visible";
				}
			}

			// Show the xxx legend (even if no xxx exists or are visible)

			const std::string FEATUREVISIBLE(FEATURE + "VISIBLE");
			texts[FEATUREVISIBLE] << classDef << "Visible";
		}
	}
	catch (libconfig::ConfigException & ex) {
		if (!config.exists(confPath)) {
			// No settings for confPath
			//
//			if (options.debug)s
//				debugoutput << "Settings for "
//							<< confPath
//							<< " not found\n";

			throw std::runtime_error("Settings for " + confPath + " are missing");
		}

	    throw ex;
	}
 }

  // ----------------------------------------------------------------------
  /*!
   * \brief Conseptual model symbol rendering
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::render_symbol(const std::string & confPath,
		  	  	  	  	  	  	  std::ostringstream & symOutput,
		  	  	  	  	  	  	  const std::string & symClass,
		  	  	  	  	  	  	  const std::string & symCode,
		  	  	  	  	  	  	  double lon,double lat,
		  	  	  	  	  	  	  const woml::Feature * feature,
		  	  	  	  	  	  	  const woml::NumericalSingleValueMeasure * svm,
		  						  NFmiFillPositions * fpos,
		  						  const std::list<std::string> * areaSymbols,
		  						  int width,int height)
  {
	// symCode is empty when called for PressureCenterType and StormType derived objects
	// (AntiCyclone, Antimesocyclone, ..., ConvectiveStorm, Storm) or for surface filling
	//
	// Symbol's <code> is taken from
	// 		PointMeteorologicalSymbol:	code_<symCode> = <code>
	// 		AntiCyclone etc.			code = <code>
	//
	std::string _symCode("code" + (symCode.empty() ? "" : ("_" + symCode)));

	// Find the pixel coordinate. For surface fill the positions are already projected
	if (!fpos) {
		PathProjector proj(area);
		proj(lon,lat);
	}

	++npointsymbols;

	try {
		const char * typeMsg = " must contain a list of groups in parenthesis";
		const char * symTypeMsg = ": symbol type must be 'svg', 'img' or 'font'";

		const libconfig::Setting & symbolSpecs = config.lookup(confPath);
		if(!symbolSpecs.isList())
			throw std::runtime_error(confPath + typeMsg);

		settings s_name((settings) (s_base + 0));
		settings s_code((settings) (s_base + 1));

		// Configuration blocks to search values for the settings; global blocks (blocks with no name),
		// locale global blocks (blocks with matching name but having no locale) and the current block
		// (block with matching name and locale).
		//
		// The blocks are stored to the list in the order of appearance (and having current block
		// as the last element). The list is processed in reverse order when searching.
		//
		// Current block or locale global block is needed for rendering.
		//
		std::list<const libconfig::Setting *> scope;
		bool nameMatch;
		bool hasLocaleGlobals = false,_hasLocaleGlobals = false;

		int symbolIdx = -1,localeIdx = -1;
		int lastIdx = symbolSpecs.getLength() - 1;

		for(int i=0; i<=lastIdx; ++i)
		{
			const libconfig::Setting & specs = symbolSpecs[i];
			if(!specs.isGroup())
				throw std::runtime_error(confPath + typeMsg);

			try {
				nameMatch = (lookup<std::string>(specs,confPath,"name",s_name) == symClass);
			}
			catch (SettingIdNotFoundException & ex) {
				// Global settings have no name
				//
				scope.push_back(&specs);
				nameMatch = false;
			}

			// Enter the block on matching name and for last config entry to load/use the globals
			//
			if (nameMatch || ((i == lastIdx) && _hasLocaleGlobals)) {
				if (nameMatch) {
					symbolIdx = i;

					// Locale
					//
					std::string locale(toLower(configValue<std::string>(specs,symClass,"locale",NULL,s_optional)));
					bool localeMatch = (locale == options.locale);

					if (localeMatch || (locale == "")) {
						scope.push_back(&specs);

						if (localeMatch)
							localeIdx = i;
						else
							hasLocaleGlobals = _hasLocaleGlobals = true;

						// Search for conditional settings based on the documents validTime (it's time value) or given measurement value

						const libconfig::Setting * condSpecs = svm
							? matchingCondition(config,confPath,symClass,"",symbolIdx,svm->numericValue())
							: matchingCondition(config,confPath,symClass,_symCode,symbolIdx,validtime);

						if (condSpecs)
							scope.push_back(condSpecs);
					}

					// Continue searching unless got (new) current block or reached the end of configuration
					// and there is (new) locale global blocks to use

					if ((!localeMatch) && ((i < lastIdx) || (!_hasLocaleGlobals)))
						continue;
				}

				// Symbol code. May contain folder too; [<folder>/]<code>

				std::string folder;
				std::string code;

				if (!areaSymbols) {
					try {
						// Note: When rendering wind symbol with wind speed, using primarily 'code' key for
						//		 symbol mapping (used conditionally to map all wind symbols to 'calm' if speed is nan)
						//
						code = getFolderAndSymbol(config,&scope,confPath,symClass,_symCode,s_code,validtime,folder,svm ? true : false);
					}
					catch (SettingIdNotFoundException & ex) {
						// Symbol code not found
						//
						// Clear _hasLocaleGlobals to indicate the settings have been scanned upto current block and
						// there's no use to enter here again unless new locale global block(s) or current block is met
						//
						_hasLocaleGlobals = false;
						continue;
					}
				}

				// Symbol type; (fill, fill+mask ==>) svg, img or font
				std::string type = configValue<std::string>(scope,symClass,"type");

				// x- and y- position offsets (mainly to center font symbol glyphs)
				//
				int xoffset = 0,yoffset = 0;

				if ((type == "fill") || (type == "fill+mask"))
					type = "svg";
				else {
					bool isSet;

					xoffset = static_cast<int>(floor(configValue<float,int>(scope,symClass,"xoffset",s_optional,&isSet)));
					if (!isSet)
						xoffset = 0;
					yoffset = static_cast<int>(floor(configValue<float,int>(scope,symClass,"yoffset",s_optional,&isSet)));
					if (!isSet)
						yoffset = 0;
				}

				// Output placeholder; by default output to passed stream. The setting is ignored for ParameterValueSetArea.

				std::string placeHolder(areaSymbols ? "" : boost::algorithm::trim_copy(configValue<std::string>(scope,symClass,"output",s_optional)));
				std::ostringstream & symbols = (placeHolder.empty() ? symOutput : texts[placeHolder]);

				bool hasWidth,hasHeight;

				if ((type == "svg") || (type == "img")) {
					// Some settings are required for svg but optional for img
					//
					settings reqORopt = ((type == "svg") ? s_required : s_optional);
					std::ostringstream wh;

					// If symbol url is not given, 'use' reference is generated

					std::string uri = configValue<std::string>(scope,symClass,"url",fpos ? s_required : s_optional);

					if (uri.empty()) {
						std::string classDef = configValue<std::string>(scope,symClass,"class",s_optional);
						if (classDef.empty())
							classDef = code;

						bool hasScale;
						double scale;
						scale = configValue<double>(scope,symClass,"scale",s_optional,&hasScale);

						wh << "<use class=\"" << classDef << "\""
						   << " xlink:href=\"#" << code << "\""
						   << " x=\"" << lon << "\" y=\"" << lat << "\"";

						if (hasScale)
							wh << std::setprecision(4) << " transform=\"translate(" << -lon * (scale - 1) << "," << -lat * (scale - 1)
							   << ") scale(" << scale << ")\"";

						wh << "/>\n";
					}
					else {
						if (folder.empty())
							folder = configValue<std::string,int>(scope,symClass,"folder",reqORopt);

						if ((width <= 0) || (height <= 0)) {
							width = configValue<int>(scope,symClass,"width",reqORopt,&hasWidth);
							height = configValue<int>(scope,symClass,"height",reqORopt,&hasHeight);
						}
						else
							hasWidth = hasHeight = true;

						if (!areaSymbols) {
							boost::algorithm::replace_all(uri,"%folder%",folder);
							boost::algorithm::replace_all(uri,"%symbol%",code + "." + type);
						}

						if (hasWidth)
							wh << " width=\"" << std::fixed << std::setprecision(0) << width << "px\"";
						else
							width = 0;
						if (hasHeight)
							wh << " height=\"" << std::fixed << std::setprecision(0) << height << "px\"";
						else
							height = 0;
					}

					if (!fpos) {
						if (uri.empty())
							symbols << wh.str();
						else
							symbols << "<image xlink:href=\""
									<< svgescape(uri)
									<< "\" x=\""
									<< std::fixed << std::setprecision(1) << ((lon-width/2) - xoffset)
									<< "\" y=\""
									<< std::fixed << std::setprecision(1) << ((lat-height/2) + yoffset)
									<< "\"" << wh.str() << "/>\n";
					}
					else {
						NFmiFillPositions::const_iterator piter;
						std::list<std::string>::const_iterator siter;

						if (areaSymbols)
							siter = areaSymbols->begin();

						for (piter = fpos->begin(); (piter != fpos->end()); piter++)
							if (areaSymbols) {
								if (siter == areaSymbols->end())
									siter = areaSymbols->begin();

								if (siter != areaSymbols->end()) {
									std::string u(uri);

									// Symbol code may contain folder too; [<folder>/]<code>

									std::string symFolder(folder);
									code = getFolderAndSymbol(config,NULL,symClass,symClass,*siter,s_code,validtime,symFolder);

									boost::algorithm::replace_all(u,"%folder%",symFolder);
									boost::algorithm::replace_all(u,"%symbol%",code + "." + type);

									symbols << "<image xlink:href=\""
											<< svgescape(u)
											<< "\" x=\""
											<< std::fixed << std::setprecision(1) << (piter->x - (width/2))
											<< "\" y=\""
											<< std::fixed << std::setprecision(1) << (piter->y - (height/2))
											<< "\"" << wh.str() << "/>\n";

									siter++;
								}
							}
							else
								symbols << "<image xlink:href=\""
										<< svgescape(uri)
										<< "\" x=\""
										<< std::fixed << std::setprecision(1) << (piter->x - (width/2))
										<< "\" y=\""
										<< std::fixed << std::setprecision(1) << (piter->y - (height/2))
										<< "\"" << wh.str() << "/>\n";
					}
				}
				else if (type == "font") {
					// Use last of the given classes for symbol specific class reference
					//
					std::string class1 = configValue<std::string>(scope,symClass,"class");
					boost::trim(class1);

					std::vector<std::string> classes;
					boost::split(classes,class1,boost::is_any_of(" "));

					std::string class2((class1.empty() ? "" : " ") + classes[classes.size() - 1] + (symCode.empty() ? symClass : symCode));
					std::string id = "symbol" + boost::lexical_cast<std::string>(npointsymbols);

					if (!fpos)
						symbols << "<text class=\""
								<< class1
								<< class2
								<< "\" id=\""
								<< id
								<< "\" x=\""
								<< std::fixed << std::setprecision(1) << (lon - xoffset)
								<< "\" y=\""
								<< std::fixed << std::setprecision(1) << (lat + yoffset)
								<< "\">&#" << code << ";</text>\n";
					else {
						NFmiFillPositions::const_iterator piter;

						for (piter = fpos->begin(); (piter != fpos->end()); piter++)
							symbols << "<text class=\""
									<< class1
									<< class2
									<< "\" id=\""
									<< id
									<< "\" x=\""
									<< std::fixed << std::setprecision(1) << piter->x
									<< "\" y=\""
									<< std::fixed << std::setprecision(1) << piter->y
									<< "\">&#" << code << ";</text>\n";
					}
				}
				else
					throw std::runtime_error(confPath + ": '" + type + "'" + symTypeMsg);

				if (feature) {
					std::string textOut(feature->text(options.locale));

					if ((!textOut.empty()) && (textOut != options.locale)) {
						// Render feature's infotext. The text is rendered starting from coordinate (0,0) and
						// final position is selected afterwards. The final position is set as transformation
						// offsets to the selected position.
						//
						// Note: The key for position must sort after the key for the text (the key/text
						//		 containing the position key must be handled/outputted prior the position key/text);
						//		 therefore the position key starts with "Z0"
						//
						std::string TEXTPOSid("Z0TEXTPOS_" + symClass + boost::lexical_cast<std::string>(npointsymbols));
						std::string textPosition = configValue<std::string>(scope,symClass,"textposition",s_optional);
						int textWidth = 0,textHeight = 0,maxTextWidth = 0,fontSize = 0,tXOffset = 0,tYOffset = 0;
						int areaWidth = static_cast<int>(std::floor(0.5+area->Width())),areaHeight = static_cast<int>(std::floor(0.5+area->Height()));

						// The text can start with position, fontsize and x/y offset settings overriding the configured values.
						//
						// Using default symbol/image width and height if not given.

						textSettings(textOut,textPosition,maxTextWidth,fontSize,tXOffset,tYOffset);

						render_text(texts,confPath,symClass,NFmiStringTools::UrlDecode(textOut),textWidth,textHeight,true,false,false,TEXTPOSid,false,&maxTextWidth,&fontSize,&tXOffset,&tYOffset);

						if (width == 0) width = defaultSymbolWidth;
						if (height == 0) height = defaultSymbolHeight;

						Path::BBox bbox(lon - (width  / 2.0),lat - (height  / 2.0),lon + (width  / 2.0),lat + (height  / 2.0));

						// To position the text centered to the symbol should one wish so

						NFmiFillRect infoTextRect(std::make_pair(Point(lon - (textWidth / 2.0),lat - (textHeight / 2.0)),Point(0,0)));

						setTextPosition(Path(),TEXTPOSid,textPosition,infoTextRect,areaWidth,areaHeight,textWidth,textHeight,tXOffset,tYOffset,&bbox);
					}
				}

				return;
			}  // if
		}	// for

		if (options.debug) {
			// No (locale) settings for the symbol or code
			//
			const char * p = (((symbolIdx < 0) || hasLocaleGlobals || (localeIdx >= 0)) ? "Settings for " : "Locale specific settings for " );

			debugoutput << p
						<< symClass
						<< " ("
						<< _symCode
						<< ") not found\n";
		}
	}
	catch (libconfig::SettingNotFoundException & ex) {
		// No settings for confPath
		//
		if (options.debug)
			debugoutput << "Settings for "
						<< confPath
						<< " ("
						<< symClass
						<< symCode
						<< ") not found\n";
	}
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Determine next path point for given hi range point
   */
  // ----------------------------------------------------------------------

  SvgRenderer::Phase SvgRenderer::uprightdown(ElevGrp & eGrp,ElevGrp::iterator & iteg,double lo,double hi,bool nonGndFwd2Gnd)
  {
	ElevGrp::iterator uiteg = iteg,riteg = iteg,triteg = iteg;
	const boost::posix_time::ptime & vt = iteg->validTime();
	double ulo = 0.0;
	double uhi = 0.0;
	double nonZ = axisManager->nonZeroElevation();
	bool upopen = false;

	if (iteg != eGrp.begin()) {
		uiteg--;

		if ((uiteg->validTime() == vt) && (!(uiteg->bottomConnected()))) {
			const woml::Elevation & e = uiteg->elevation();
			boost::optional<woml::NumericalSingleValueMeasure> itsBoundedLo = (e.bounded() ? e.lowerLimit() : woml::NumericalSingleValueMeasure());
			const boost::optional<woml::NumericalSingleValueMeasure> & itsLoLimit = (e.bounded() ? itsBoundedLo : e.value());
			boost::optional<woml::NumericalSingleValueMeasure> itsBoundedHi = (e.bounded() ? e.upperLimit() : woml::NumericalSingleValueMeasure());
			const boost::optional<woml::NumericalSingleValueMeasure> & itsHiLimit = (e.bounded() ? itsBoundedHi : e.value());

			ulo = ((!itsLoLimit) ? 0.0 : itsLoLimit->numericValue());
			uhi = ((!itsHiLimit) ? 0.0 : itsHiLimit->numericValue());
		}
		else
			uiteg = iteg;
	}

	for (riteg++; (riteg != eGrp.end()); riteg++) {
		int dh = (riteg->validTime() - vt).hours();

		if (dh == 0) {
			if (riteg->bottomConnected() && (!(riteg->topConnected())))
				// Lower elevation has it's bottom connected, close the loop by going down
				//
				return edn;

			continue;
		}
		else if (dh == 1) {
			const woml::Elevation & e = riteg->elevation();
			boost::optional<woml::NumericalSingleValueMeasure> itsBoundedLo = (e.bounded() ? e.lowerLimit() : woml::NumericalSingleValueMeasure());
			const boost::optional<woml::NumericalSingleValueMeasure> & itsLoLimit = (e.bounded() ? itsBoundedLo : e.value());
			boost::optional<woml::NumericalSingleValueMeasure> itsBoundedHi = (e.bounded() ? e.upperLimit() : woml::NumericalSingleValueMeasure());
			const boost::optional<woml::NumericalSingleValueMeasure> & itsHiLimit = (e.bounded() ? itsBoundedHi : e.value());

			double rlo = ((!itsLoLimit) ? 0.0 : itsLoLimit->numericValue());
			double rhi = ((!itsHiLimit) ? 0.0 : itsHiLimit->numericValue());

			// If nonGndFwd2Gnd is false (ZeroTolerance), can always go forward from below 0 to below 0 or ground elevation
			// (even if the elevations do not overlap)

			bool FWD = ((!nonGndFwd2Gnd) && (hi < nonZ) && (rlo < nonZ));

			if (((rlo <= hi) && (rhi >= lo)) || FWD) {
				if ((uiteg != iteg) && ((rlo <= uhi) && (rhi >= ulo))) {
					// Can go up
					//
					upopen = true;
				}

				if (((triteg == iteg) || riteg->bottomConnected()) && (!(riteg->topConnected())))
					// Can go right
					//
					triteg = riteg;
			}
		}
		else
			break;
	}

	if (upopen && ((!(uiteg->topConnected())) || (triteg == iteg))) {
		// Up
		//
		// Inform the right side elevation that the path has travelled by it's left side
		// (the right side elevation can not be travelled through upwards)

		if (triteg != iteg)
			triteg->leftOpen(false);
		iteg = uiteg;

		return vup;
	}
	else if (triteg != iteg) {
		// Right
		//
		iteg = triteg;
		return fwd;
	}

	// Down (or last)

	return (iteg->bottomConnected() ? lst : edn);
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Determine next path point for given lo range point
   */
  // ----------------------------------------------------------------------

  SvgRenderer::Phase SvgRenderer::downleftup(ElevGrp & eGrp,ElevGrp::iterator & iteg,double lo,double hi,bool nonGndVdn2Gnd)
  {
	ElevGrp::iterator diteg = iteg,liteg = iteg,bliteg = iteg;
	const boost::posix_time::ptime & vt = iteg->validTime();
	double dlo = 0.0;
	double dhi = 0.0;
	double nonZ = axisManager->nonZeroElevation();
	bool downopen = false,leftopen = false,upopen = false;

	if (iteg != eGrp.begin()) {
		diteg++;

		if ((diteg != eGrp.end()) && (diteg->validTime() == vt) && (!(diteg->topConnected()))) {
			const woml::Elevation & e = diteg->elevation();
			boost::optional<woml::NumericalSingleValueMeasure> itsBoundedLo = (e.bounded() ? e.lowerLimit() : woml::NumericalSingleValueMeasure());
			const boost::optional<woml::NumericalSingleValueMeasure> & itsLoLimit = (e.bounded() ? itsBoundedLo : e.value());
			boost::optional<woml::NumericalSingleValueMeasure> itsBoundedHi = (e.bounded() ? e.upperLimit() : woml::NumericalSingleValueMeasure());
			const boost::optional<woml::NumericalSingleValueMeasure> & itsHiLimit = (e.bounded() ? itsBoundedHi : e.value());

			dlo = ((!itsLoLimit) ? 0.0 : itsLoLimit->numericValue());
			dhi = ((!itsHiLimit) ? 0.0 : itsHiLimit->numericValue());

			if (iteg->bottomConnected()) {
				if (iteg->bottomConnection()) {
					// Check if the elevation below overlaps the elevation on the right side of the current elevation
					//
					const woml::Elevation & e = *(iteg->bottomConnection());
					boost::optional<woml::NumericalSingleValueMeasure> itsBoundedLo = (e.bounded() ? e.lowerLimit() : woml::NumericalSingleValueMeasure());
					const boost::optional<woml::NumericalSingleValueMeasure> & itsLoLimit = (e.bounded() ? itsBoundedLo : e.value());
					boost::optional<woml::NumericalSingleValueMeasure> itsBoundedHi = (e.bounded() ? e.upperLimit() : woml::NumericalSingleValueMeasure());
					const boost::optional<woml::NumericalSingleValueMeasure> & itsHiLimit = (e.bounded() ? itsBoundedHi : e.value());

					double bclo = ((!itsLoLimit) ? 0.0 : itsLoLimit->numericValue());
					double bchi = ((!itsHiLimit) ? 0.0 : itsHiLimit->numericValue());

					downopen = ((bclo <= dhi) && (bchi >= dlo));
				}
			}
		}
		else
			diteg = iteg;

		for ( ; ; ) {
			if (liteg == eGrp.begin())
				break;
			liteg--;

			int dh = (liteg->validTime() - vt).hours();

			if (dh == 0) {
				if (liteg->topConnected() && (!(liteg->bottomConnected())))
					// Upper elevation having only it's top connected
					//
					upopen = true;

				continue;
			}
			else if (dh == -1) {
				const woml::Elevation & e = liteg->elevation();
				boost::optional<woml::NumericalSingleValueMeasure> itsBoundedLo = (e.bounded() ? e.lowerLimit() : woml::NumericalSingleValueMeasure());
				const boost::optional<woml::NumericalSingleValueMeasure> & itsLoLimit = (e.bounded() ? itsBoundedLo : e.value());
				boost::optional<woml::NumericalSingleValueMeasure> itsBoundedHi = (e.bounded() ? e.upperLimit() : woml::NumericalSingleValueMeasure());
				const boost::optional<woml::NumericalSingleValueMeasure> & itsHiLimit = (e.bounded() ? itsBoundedHi : e.value());

				double llo = ((!itsLoLimit) ? 0.0 : itsLoLimit->numericValue());
				double lhi = ((!itsHiLimit) ? 0.0 : itsHiLimit->numericValue());

				// If nonGndVdn2Gnd is true (ZeroTolerance), can always go backward between below 0 and/or ground elevations
				// (even if the elevations do not overlap)

				bool BWD = (nonGndVdn2Gnd && (lo < nonZ) && (llo < nonZ));

				if (((llo <= hi) && (lhi >= lo)) || BWD) {
					if ((diteg != iteg) && ((llo <= dhi) && (lhi >= dlo))) {
						// Can go down
						//
						downopen = true;
					}

					// Select the lowest overlapping left side elevation for which the path has not travelled by left side
					// (otherwise the elevation can not be travelled through upwards)

					if (
						((bliteg == iteg) || (!(bliteg->leftOpen()))) &&
						(!(liteg->bottomConnected()))
					   ) {
						// Can go left
						//
						leftopen = (!(liteg->topConnected()));
						bliteg = liteg;
					}
				}
			}
			else
				break;
		}
	}

	if (downopen && ((!upopen) || leftopen)) {
		// Go down; there is no upper elevation having only it's top connected or it can reached
		// through overlapping left side elevation
		//
		iteg = diteg;
		return vdn;
	}
	else if ((bliteg != iteg) && ((!upopen) || leftopen)) {
		// Go left; there is no upper elevation having only it's top connected or it can reached
		// through the overlapping left side elevation
		//
		bliteg->bottomConnection(iteg->elevation());
		iteg = bliteg;

		return bwd;
	}

	// Up (or last)

	return (iteg->topConnected() ? lst : eup);
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Loads cloud layer configuration
   */
  // ----------------------------------------------------------------------

  const libconfig::Setting & SvgRenderer::cloudLayerConfig(const std::string & _confPath,
		  	  	  	  	  	  	  	  	  	  	  	  	   double & tightness,bool & borderCompensation,
		  	  	  	  	  	  	  	  	  	  	  	  	   int & minLabelPosHeight,double & labelPosHeightFactor,
		  	  	  	  	  	  	  	  	  	  	  	  	   std::list<CloudGroup> & cloudGroups,
		  	  	  	  	  	  	  	  	  	  	  	  	   std::set<size_t> & cloudSet)
  {
	std::string confPath(_confPath);

	try {
		const char * grouptypeMsg = " must contain a group in curly brackets";
		const char * listtypeMsg = " must contain a list of groups in parenthesis";

		std::string CLOUDLAYERS(boost::algorithm::to_upper_copy(confPath));

		// Class

		const libconfig::Setting & specs = config.lookup(confPath);
		if(!specs.isGroup())
			throw std::runtime_error(confPath + grouptypeMsg);

		std::string classDef = configValue<std::string>(specs,confPath,"class");

		// Bezier curve tightness

		bool isSet;
		tightness = (double) configValue<double,int>(specs,confPath,"tightness",NULL,s_optional,&isSet);

		if (!isSet)
			// Using the default by setting a negative value
			//
			tightness = -1.0;

		// If bordercompensation is true (the default), cloud baseline is adjusted to keep decorated baseline at the right level

		borderCompensation = configValue<bool>(specs,confPath,"bordercompensation",NULL,s_optional,&isSet);

		if (!isSet)
			borderCompensation = true;

		// Minimum absolute/relative height for the label position

		minLabelPosHeight = configValue<int>(specs,confPath,"minlabelposheight",NULL,s_optional,&isSet);

		if ((!isSet) || (minLabelPosHeight < (int) labelPosHeightMin)) {
			labelPosHeightFactor = configValue<double>(specs,confPath,"labelposheightfactor",NULL,s_optional,&isSet);

			if ((!isSet) || (labelPosHeightFactor < labelPosHeightFactorMin))
				labelPosHeightFactor = defaultLabelPosHeightFactor;

			minLabelPosHeight = 0;
		}

		// If bbcenterlabel is true (default: false), cloud label is positioned to the center of the cloud's bounding box;
		// otherwise using 1 or 2 elevations in the middle of the cloud in x -direction

		bool bbCenterLabel = configValue<bool>(specs,confPath,"bbcenterlabel",NULL,s_optional,&isSet);
		if (!isSet)
			bbCenterLabel = false;

		// Cloud groups

		confPath += ".groups";

		const libconfig::Setting & groupSpecs = config.lookup(confPath);
		if(!groupSpecs.isList())
			throw std::runtime_error(confPath + listtypeMsg);

		for(int i=0, globalsIdx = -1; i<groupSpecs.getLength(); i++)
		{
			const libconfig::Setting & group = groupSpecs[i];
			if(!group.isGroup())
				throw std::runtime_error(confPath + listtypeMsg);

			// Cloud types

			std::string cloudTypes(boost::algorithm::to_upper_copy(boost::algorithm::trim_copy(configValue<std::string>(group,confPath,"types",NULL,s_optional))));

			if (!cloudTypes.empty()) {
				std::vector<std::string> ct;
				boost::algorithm::split(ct,cloudTypes,boost::is_any_of(","));

				if (ct.size() > 1) {
					std::ostringstream cts;

					for (unsigned int i = 0,n = 0; (i < ct.size()); i++) {
						boost::algorithm::trim(ct[i]);

						if (!ct[i].empty()) {
							cts << (n ? "," : "") << ct[i];
							n++;
						}
					}

					cloudTypes = cts.str();
				}
			}

			if (cloudTypes.empty()) {
				// Global settings have no type(s)
				//
				globalsIdx = i;
				continue;
			}

			// Missing settings from globals when available

			libconfig::Setting * globalScope = ((globalsIdx >= 0) ? &groupSpecs[globalsIdx] : NULL);

			// Class

			std::string cloudClass = configValue<std::string>(group,confPath,"class",globalScope,s_optional);

			// Output label; default label is the cloud types concatenated with ',' as separator.
			// If empty label is given, no label.

			std::string label(boost::algorithm::trim_copy(configValue<std::string>(group,confPath,"label",NULL,s_optional,&isSet)));
			bool hasLabel = isSet;

			// Output placeholders; default label placeholder is the cloud placeholder + "TEXT"

			std::string placeHolder(boost::algorithm::trim_copy(configValue<std::string>(group,confPath,"output",globalScope,s_optional)));
			if (placeHolder.empty())
				placeHolder = CLOUDLAYERS;

			std::string labelPlaceHolder(boost::algorithm::trim_copy(configValue<std::string>(group,confPath,"labeloutput",globalScope,s_optional)));
			if (labelPlaceHolder.empty())
				labelPlaceHolder = placeHolder + "TEXT";

			// Standalone cloud types (only one type in the group) have flag controlling whether the
			// group/cloud is combined or not

			bool combined = configValue<bool>(group,confPath,"combined",globalScope,s_optional,&isSet);
			if (!isSet)
				combined = false;

			// Render as symbol ?

			std::string symbolType(boost::algorithm::trim_copy(configValue<std::string>(group,confPath,"symboltype",NULL,s_optional)));

			if (symbolType.empty()) {
				bool symbol = configValue<bool>(group,confPath,"symbol",globalScope,s_optional,&isSet);

				if (isSet && symbol)
					symbolType = boost::algorithm::trim_copy(configValue<std::string>(group,confPath,"symboltype",globalScope));
			}

			// Controls for extracting Bezier curve points; base distance between curve points along the line,
			// max random distance added to the base and max number of subsequent points having equal distance

			unsigned int baseStep = configValue<unsigned int>(group,confPath,"baseStep",globalScope,s_optional,&isSet);
			if (!isSet)
				baseStep = 10;
			unsigned int maxRand = configValue<unsigned int>(group,confPath,"maxRand",globalScope,s_optional,&isSet);
			if (!isSet)
				maxRand = 4;
			unsigned int maxRepeat = configValue<unsigned int>(group,confPath,"maxRepeat",globalScope,s_optional,&isSet);
			if (!isSet)
				maxRepeat = 3;

			// Controls for decorating the Bezier curve; base curve distance (normal length) for the decorator points,
			// max random distance added to the base, base offset for the decorator points and max random offset added to the base

			int scaleHeightMin = configValue<int>(group,confPath,"scaleHeightMin",globalScope,s_optional,&isSet);
			if (!isSet)
				scaleHeightMin = 5;
			int scaleHeightRandom = configValue<int>(group,confPath,"scaleHeightRandom",globalScope,s_optional,&isSet);
			if (!isSet)
				scaleHeightRandom = 3;
			int controlMin = configValue<int>(group,confPath,"controlMin",globalScope,s_optional,&isSet);
			if (!isSet)
				controlMin = -2;
			int controlRandom = configValue<int>(group,confPath,"controlRandom",globalScope,s_optional,&isSet);
			if (!isSet)
			   controlRandom = 4;

			// Offsets for intermediate curve points on both sides of the elevation's hi/lo range point

			double xStep = axisManager->xStep();

			double xOffset = configValue<double>(group,confPath,"xoffset",globalScope,s_optional,&isSet);
			if (!isSet)
				xOffset = 0.0;
			else
				xOffset *= xStep;

			double yOffset = (double) configValue<int,double>(group,confPath,"yoffset",globalScope,s_optional,&isSet);
			if (!isSet)
				yOffset = 0.0;

			// Relative offset for intermediate curve points (controls how much the ends of the area
			// are extended horizontally)

			double vOffset = configValue<double>(group,confPath,"voffset",globalScope,s_optional,&isSet);
			if (!isSet)
				vOffset = 0.0;
			else
				vOffset *= xStep;

			// Relative top/bottom and side x -offsets for intermediate curve points for single elevation (controls how much the
			// ends of the area are extended horizontally)

			double vSOffset = configValue<double>(group,confPath,"vsoffset",globalScope,s_optional,&isSet);
			if (!isSet)
				vSOffset = xStep / 3;
			else
				vSOffset *= xStep;

			double vSSOffset = configValue<double>(group,confPath,"vssoffset",globalScope,s_optional,&isSet);
			if (!isSet)
				vSSOffset = vSOffset;
			else
				vSSOffset *= xStep;

			// Max offset in px for extending first time instant's elevations to the left

			int sOffset = configValue<int>(group,confPath,"soffset",globalScope,s_optional,&isSet);
			if (!isSet)
				sOffset = 0;

			// Max offset in px for extending last time instant's elevations to the right

			int eOffset = configValue<int>(group,confPath,"eoffset",globalScope,s_optional,&isSet);
			if (!isSet)
				eOffset = 0;

			cloudGroups.push_back(CloudGroup(cloudClass.empty() ? classDef : cloudClass,
											 cloudTypes,
											 symbolType,
											 hasLabel ? &label : NULL,
											 bbCenterLabel,
											 placeHolder,
											 labelPlaceHolder,
											 combined,
											 baseStep,
											 maxRand,
											 maxRepeat,
											 scaleHeightMin,
											 scaleHeightRandom,
											 controlMin,
											 controlRandom,
											 xOffset,yOffset,
											 vOffset,vSOffset,vSSOffset,
											 sOffset,eOffset,
											 cloudSet,
											 &group,
											 globalScope
											));
		}

		return specs;
	}
	catch(libconfig::ConfigException & ex)
	{
		if(!config.exists(confPath))
			// No settings for confPath
			//
			throw std::runtime_error("Settings for " + confPath + " are missing");

		throw ex;
	}
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Cloud symbol rendering
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::render_cloudSymbol(const std::string & confPath,
		  	  	  	  	  	  	  	   const CloudGroup & cg,
		  	  	  	  	  	  	  	   int nGroups,
		  	  	  	  	  	  	  	   double x,
		  	  	  	  	  	  	  	   double lopx,double hipx)
  {
	// Search for conditional settings based on cloud height in px

	double hpx = lopx - hipx;
	const libconfig::Setting * condSpecs = matchingCondition(config,confPath,confPath,"",-1,hpx);

	// Set active configuration scope

	std::list<const libconfig::Setting *> scope;

	if (cg.globalScope())
		scope.push_back(cg.globalScope());

	scope.push_back(cg.localScope());

	if (condSpecs)
		scope.push_back(condSpecs);

	// Symbol url, type, height and width

	std::string uri = configValue<std::string>(scope,confPath,"url");
	std::string symbolType = configValue<std::string>(scope,confPath,"symboltype");
	int height = configValue<int>(scope,confPath,"height");
	int width = configValue<int>(scope,confPath,"width");

	boost::algorithm::replace_all(uri,"%symboltype%",symbolType);
	boost::algorithm::replace_all(uri,"%height%",boost::lexical_cast<std::string>(height));
	boost::algorithm::replace_all(uri,"%width%",boost::lexical_cast<std::string>(width));

	// Vertical and horizontal scale

	double vs = (hpx / height);
	double hs = (axisManager->xStep() / width);

	texts[cg.placeHolder()] << "<g id=\"CloudLayers" << nGroups << "\" transform=\"translate("
							<< std::fixed << std::setprecision(3)
							<< x << "," << (lopx - (hpx / 2)) << ")\">\n"
							<< "<use transform=\"scale("
							<< hs << "," << vs << ")\" "
							<< "xlink:href=\"" << uri << "\"/>\n"
							<< "</g>\n";

	std::string label = cg.label();

	if (!label.empty())
		texts[cg.markerPlaceHolder()] << "<text class=\"" << cg.textClassDef()
									  << "\" id=\"" << "CloudLayersText" << nGroups
									  << "\" text-anchor=\"middle"
									  << "\" x=\"" << x
									  << "\" y=\"" << (lopx - ((lopx - hipx) / 2))
									  << "\">" << label << "</text>\n";
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Utility function for area's label position tracking.
   *
   *
   */
  // ----------------------------------------------------------------------

  void trackAreaLabelPos(ElevGrp::iterator iteg,
		  	  	  	  	 double x,double & maxx,
		  	  	  	  	 double lopx,std::vector<double> & _lopx,
		  	  	  	  	 double hipx,std::vector<double> & _hipx,
		  	  	  	  	 std::vector<bool> & hasHole)
  {
	// Keep track of elevations (when travelling forwards)

	int nX = _lopx.size();

	if ((nX == 0) || (x > (maxx + 0.01))) {
		if (iteg->hasHole()) {
			// Elevation has hole(s); use the tallest part.
			//
			const boost::optional<woml::Elevation> & labelElevation = iteg->Pv()->labelElevation();

			lopx = labelElevation->lowerLimit()->numericValue();
			hipx = labelElevation->upperLimit()->numericValue();
		}

		_lopx.push_back(lopx);
		_hipx.push_back(hipx);
		hasHole.push_back(iteg->hasHole());

		maxx = x;
	}
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Move used/reserved 'fillAreas' container's fill areas into
   *		to 'candidateAreas' to avoid overlapping labels/symbols etc.
   */
  // ----------------------------------------------------------------------

  void eraseReservedAreas(const std::string & markerId,const std::string & reserver,const NFmiFillAreas & reservedAreas,FillAreas & candidateAreas,NFmiFillAreas & fillAreas,bool isHole = false,bool storeCandidates = true,bool eraseReserved = true)
  {
	// Allow some overlap for nonholes

	int overlap = (isHole ? -fillAreaOverlapMax : fillAreaOverlapMax);

	for (NFmiFillAreas::const_iterator riter = reservedAreas.begin(); (riter != reservedAreas.end()); riter++)
		for (NFmiFillAreas::iterator fiter = fillAreas.begin(); (fiter != fillAreas.end()); )
			if (
				(markerId != reserver) &&
				(fiter->first.x < riter->second.x) && (fiter->second.x >= (riter->first.x + overlap)) &&
				(fiter->first.y < riter->second.y) && (fiter->second.y >= (riter->first.y + overlap))
			   ) {
				if ((!isHole) && storeCandidates) {
					candidateAreas[markerId].fillAreas.push_back(*fiter);
					candidateAreas[markerId].markers.push_back(reserver);
					candidateAreas[markerId].scales.push_back(std::make_pair(1.0,1.0));
				}

//fprintf(stderr,">> %s %.0f,%.0f - %.0f,%.0f [%.0f,%.0f - %.0f,%.0f]\n",eraseReserved ? "erase" : "keep",fiter->first.x,fiter->first.y,fiter->second.x,fiter->second.y,riter->first.x,riter->first.y,riter->second.x,riter->second.y);
				if (eraseReserved)
					fiter = fillAreas.erase(fiter);
				else
					fiter++;
			}
			else
				fiter++;
  }

  void eraseReservedAreas(const std::string & markerId,const FillAreas & reservedAreas,FillAreas & candidateAreas,NFmiFillAreas & fillAreas,bool isHole = false,bool storeCandidates = true)
  {
	// If storing candidates, loop the check twice; first get the candidates and remove the reserved areas on second round

	if (storeCandidates) {
		for (FillAreas::const_iterator riter = reservedAreas.begin(); (riter != reservedAreas.end()); riter++)
			// Note: Temporary "CURRENTCANDIDATE" reserved marker is flagged with nonempty 'markers' field
			//
			eraseReservedAreas(markerId,riter->first,riter->second.fillAreas,candidateAreas,fillAreas,isHole,riter->second.markers.empty(),false);

		storeCandidates = false;
	}

	for (FillAreas::const_iterator riter = reservedAreas.begin(); (riter != reservedAreas.end()); riter++)
		eraseReservedAreas(markerId,riter->first,riter->second.fillAreas,candidateAreas,fillAreas,isHole,storeCandidates);
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Check if given marker (label or symbol) position is free.
   *		Return the bbox if the position is free, otherwise an empty list
   *		is returned.
   */
  // ----------------------------------------------------------------------

  NFmiFillAreas getMarkerArea(const std::string & markerId,const NFmiFillAreas & holeAreas,const FillAreas & reservedAreas,FillAreas & candidateAreas,double mx,double my,int markerWidth,int markerHeight,bool storeCandidates = true)
  {
	NFmiFillAreas markerArea;

	markerArea.push_back(std::make_pair(Point(mx - (markerWidth / 2.0),my - (markerHeight / 2.0)),Point(mx + (markerWidth / 2.0),my + (markerHeight / 2.0))));

	eraseReservedAreas("hole","hole",holeAreas,candidateAreas,markerArea,true);
	eraseReservedAreas(markerId,reservedAreas,candidateAreas,markerArea,false,storeCandidates);

	return markerArea;
  }

  NFmiFillAreas getMarkerArea(const std::string & markerId,const NFmiFillAreas & holeAreas,const FillAreas & reservedAreas,FillAreas & candidateAreas,double & mx,double my,int & markerWidth,int & markerHeight,bool checkBwd,bool checkFwd,double xStep)
  {
	NFmiFillAreas markerArea;
	double x = mx;
	bool storeCandidates = true;
	int w = markerWidth,h = markerHeight;

	do {
		// Check the given position

		markerArea = getMarkerArea(markerId,holeAreas,reservedAreas,candidateAreas,mx,my,markerWidth,markerHeight,storeCandidates);

		if (markerArea.empty()) {
			if (checkBwd) {
				// Check half timestep backwards
				//
				mx = x - (xStep / 2);
				markerArea = getMarkerArea(markerId,holeAreas,reservedAreas,candidateAreas,mx,my,markerWidth,markerHeight,storeCandidates);

				// Check 1/4 timestep backwards

				if (markerArea.empty()) {
					mx = x - (xStep / 4);
					markerArea = getMarkerArea(markerId,holeAreas,reservedAreas,candidateAreas,mx,my,markerWidth,markerHeight,storeCandidates);
				}
			}

			if (markerArea.empty() && checkFwd) {
				// Check half timestep forwards
				//
				mx = x + (xStep / 2);
				markerArea = getMarkerArea(markerId,holeAreas,reservedAreas,candidateAreas,mx,my,markerWidth,markerHeight,storeCandidates);

				// Check 1/4 timestep forwards

				if (markerArea.empty()) {
					mx = x + (xStep / 4);
					markerArea = getMarkerArea(markerId,holeAreas,reservedAreas,candidateAreas,mx,my,markerWidth,markerHeight,storeCandidates);
				}
			}

			if (markerArea.empty()) {
				markerWidth -= 2;
				markerHeight -= 2;

				storeCandidates = false;
			}
		}
	}
	while (markerArea.empty() && (markerWidth >= floor(w * markerScaleFactorMin)) && (markerHeight >= floor(h * markerScaleFactorMin)));

	return markerArea;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief	Select/return free marker position which is nearest to given position,
   *		and if 'storeFreeAreas' is true, store the rest free positions into
   *		free fill area container; otherwise (called with free fill area container
   *		as input container) remove the selected area from free fill area container
   */
  // ----------------------------------------------------------------------

  NFmiFillRect selectMarkerPos(const std::string & markerId,NFmiFillAreas & areas,std::list<std::pair<double,double> > & scales,FillAreas & freeAreas,double x,double y,std::list<std::pair<double,double> >::iterator & sits,bool storeFreeAreas = true)
  {
	NFmiFillAreas::iterator fits = areas.begin(),fit;
	std::list<std::pair<double,double> >::iterator sit;
	double dist2 = 0.0;

	for (fit = fits,sit = sits = scales.begin(); (fit != areas.end()); fit++, sit++) {
		double cx = fit->first.x + ((fit->second.x - fit->first.x) / 2),cy = fit->first.y + ((fit->second.y - fit->first.y) / 2);
		double d2 = ((x - cx) * (x - cx)) + ((y - cy) * (y - cy));

		if ((fit == areas.begin()) || (d2 < dist2)) {
			if ((fit != areas.begin()) && storeFreeAreas) {
				freeAreas[markerId].fillAreas.push_back(*fits);
				freeAreas[markerId].scales.push_back(*sits);
			}

			fits = fit;
			sits = sit;
			dist2 = d2;
		}
		else if (storeFreeAreas) {
			freeAreas[markerId].fillAreas.push_back(*fit);
			freeAreas[markerId].scales.push_back(*sit);
		}
	}

	NFmiFillRect r(*fits);

	if (!storeFreeAreas)
		areas.erase(fits);

	return r;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief	Move reserved marker to another position
   */
  // ----------------------------------------------------------------------

  void moveMarker(Texts & texts,FillAreas freeAreas,FillAreas::iterator & rit,std::list<std::pair<double,double> >::iterator & sit,const NFmiFillRect & r)
  {
	// Release old and reserve new position

//NFmiFillRect rr(rit->second.fillAreas.front());
//fprintf(stderr,">> %s Release Old %.0f,%.0f - %.0f,%.0f\n",rit->first.c_str(),rr.first.x,rr.first.y,rr.second.x,rr.second.y);
//fprintf(stderr,">> %s %s Reserve New %.0f,%.0f - %.0f,%.0f\n",rit->first.c_str(),texts[rit->first].str().c_str(),r.first.x,r.first.y,r.second.x,r.second.y);
	freeAreas[rit->first].fillAreas.push_back(rit->second.fillAreas.front());
	freeAreas[rit->first].scales.push_back(rit->second.scales.front());
	rit->second.fillAreas.clear();

	rit->second.fillAreas.push_back(r);

	// Set/change marker position and scale in svg output collection

	double x = r.first.x + (rit->second.centered ? ((r.second.x - r.first.x) / 2) : 0);
	double y = r.first.y + (rit->second.centered ? ((r.second.y - r.first.y) / 2) : 0);

	texts[rit->first].str("");
	texts[rit->first] << std::fixed << std::setprecision(1) << x << "," << y;
	texts[rit->first + "SCALE"].str("");
	texts[rit->first + "SCALE"] << std::fixed << std::setprecision(1) << "scale(" << rit->second.scale * sit->first << "," << rit->second.scale * sit->second << ")";
//fprintf(stderr,">> --> %s %s\n",rit->first.c_str(),texts[rit->first].str().c_str());
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Arrange reserved markers to find a free position for next marker.
   *
   * 		Search candidate fill areas recursively until the reserving
   * 		marker can be released and moved to a free position; then release
   * 		the rest of the reserved areas by replacing them with now free
   *		candidate areas and return the "bottommost"/free candidate area.
   *
   *		R5	F5  Marker 5 has reserved area R5 and it has free area F5
   *		R4	F4  Marker 4 has reserved area R4 and it has free area F4
   *		R3	C3  Marker 3 has reserved area R3 and it's candidate area C3 is reserved by R4 and by R5
   *		R2	C2	Marker 2 has reserved area R2 and it's candidate area C2 is reserved by R3
   *			C1	Marker 1 has no free areas and it's candidate area C1 is reserved by R2
   *
   *		==>
   *		R5 --> F5  Release R5, use F5
   *		R4 --> F4  Release R4, use F4
   *		R3 --> C3  Release R3, use C3
   *		R2 --> C2  Release R2, use C2
   *
   *		C1 is stored to 'fillAreas' and can be used for next marker.
   *
   *		Returns true if a free position was found.
   */
  // ----------------------------------------------------------------------

  bool arrangeMarkers(Texts & texts,FillAreas & reservedAreas,FillAreas & freeAreas,FillAreas & candidateAreas,FillAreas::iterator cit,NFmiFillAreas & fillAreas,int RC = 1,std::list<std::string>::iterator * mit2 = NULL,NFmiFillAreas::iterator * it2 = NULL,std::list<std::pair<double,double> >::iterator * sit2 = NULL)
  {
//fprintf(stderr,"\n>> [%d] MARKER: %s\n",RC,(cit == candidateAreas.end()) ? "END" : cit->first.c_str());
	if (cit == candidateAreas.end())
		return false;

	// If the iterators are passed in, candidate has multiple reservations; trying recursively to release/move other overlapping markers

	std::list<std::string>::iterator mit = (mit2 ? *mit2 : cit->second.markers.begin());
	std::list<std::string>::iterator mend = (mit2 ? *mit2 : cit->second.markers.end());
	NFmiFillAreas::iterator it = (mit2 ? *it2 : cit->second.fillAreas.begin());
	std::list<std::pair<double,double> >::iterator sit = (mit2 ? *sit2 : cit->second.scales.begin());

	if (mit2)
		mend++;
//{
//FillAreas::iterator rit = reservedAreas.begin();
//for ( ; (rit != reservedAreas.end()); rit++) {
//if ((rit->first.find("ZZTURB") != 0) || (rit->first == "ZZTURBULENCESymbol121")) continue;
//fprintf(stderr,">> [%d] RESE: %s\n",RC,rit->first.c_str());
//NFmiFillAreas::iterator it = rit->second.fillAreas.begin();
//for ( ; (it != rit->second.fillAreas.end()); it++)
//fprintf(stderr,">>\t%.0f,%.0f - %.0f,%.0f\n",it->first.x,it->first.y,it->second.x,it->second.y);
//}
//FillAreas::iterator fit = freeAreas.begin();
//for ( ; (fit != freeAreas.end()); fit++) {
//if ((fit->first.find("ZZTURB") != 0) || (fit->first == "ZZTURBULENCESymbol121")) continue;
//fprintf(stderr,"\n>> [%d] FREE: %s\n",RC,fit->first.c_str());
//NFmiFillAreas::iterator it = fit->second.fillAreas.begin();
//for ( ; (it != fit->second.fillAreas.end()); it++)
//fprintf(stderr,">>\t%.0f,%.0f - %.0f,%.0f\n",it->first.x,it->first.y,it->second.x,it->second.y);
//}
//FillAreas::iterator cit = candidateAreas.begin();
//for ( ; (cit != candidateAreas.end()); cit++) {
//if ((cit->first.find("ZZTURB") != 0) || (cit->first == "ZZTURBULENCESymbol121")) continue;
//fprintf(stderr,"\n>> [%d] CAND: %s\n",RC,cit->first.c_str());
//NFmiFillAreas::iterator it = cit->second.fillAreas.begin();
//std::list<std::string>::iterator mit = cit->second.markers.begin();
//for ( ; (it != cit->second.fillAreas.end()); it++, mit++)
//fprintf(stderr,">>\t%s %.0f,%.0f - %.0f,%.0f\n",mit->c_str(),it->first.x,it->first.y,it->second.x,it->second.y);
//}
//fprintf(stderr,"\n");
//}

	for ( ; (mit != mend); mit++, it++, sit++) {
		// Check if the marker can be moved to a free position
		//
		FillAreas::iterator rit = reservedAreas.find(*mit);

		if (rit == reservedAreas.end())
			throw std::runtime_error("arrangeMarkers: internal: reserved marker not found: '" + *mit + "'");

		FillAreas::iterator fit = freeAreas.find(*mit);

		if (fit != freeAreas.end()) {
			// Reserve the area of the current candidate temporarily, and check for (and move into candidate container)
			// fill areas which have been reserved after the marker was previously positioned
			//
			// Note: Temporary "CURRENTCANDIDATE" reserved marker is flagged with nonempty 'markers' field
			//
			reservedAreas["CURRENTCANDIDATE"].fillAreas.push_back(*it);
			reservedAreas["CURRENTCANDIDATE"].markers.push_back("CURRENTCANDIDATE");
			eraseReservedAreas(fit->first,reservedAreas,candidateAreas,fit->second.fillAreas);
			reservedAreas["CURRENTCANDIDATE"].fillAreas.clear();
			reservedAreas["CURRENTCANDIDATE"].markers.clear();

			if (!(fit->second.fillAreas.empty())) {
				// Free position exists for the reserving marker.
				//
				// Get new position near the originally selected marker position.
				// Release old and reserve new marker position and set/change marker position and scale in svg output container
				//
				std::list<std::pair<double,double> >::iterator csit;
				moveMarker(texts,freeAreas,rit,csit,selectMarkerPos(rit->first,fit->second.fillAreas,fit->second.scales,freeAreas,rit->second.x,rit->second.y,csit,false));
			}
			else
				fit = freeAreas.end();
		}

		// Use the candidate if released above or recurse with reserving marker's candidates

		if ((fit != freeAreas.end()) || arrangeMarkers(texts,reservedAreas,freeAreas,candidateAreas,candidateAreas.find(*mit),fillAreas,RC + 1)) {
			// Store the candidate
			//
			fillAreas.clear();
			fillAreas.push_back(*it);

			FillAreas::iterator rit = reservedAreas.find(cit->first);

			if (rit != reservedAreas.end())
				// Release the old and reserve new position and change the position in output container
				//
				moveMarker(texts,freeAreas,rit,sit,fillAreas.front());

			// Remove candidate from container
			//
			cit->second.markers.erase(mit);
			cit->second.fillAreas.erase(it);
			cit->second.scales.erase(sit);
//else {
//NFmiFillRect r(fillAreas.front());
//fprintf(stderr,">> [%d] %s NO reserved areas, RET candidate %.0f,%.0f - %.0f,%.0f\n",RC,cit->first.c_str(),r.first.x,r.first.y,r.second.x,r.second.y);
//}

			return true;
		}
	}

	return false;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief	Scale down reserved markers which overlap current marker.
   */
  // ----------------------------------------------------------------------

  void scaleMarkers(Texts & texts,FillAreas & reservedAreas,double markerWidth,double markerHeight,double mx,double my,double xScale,double yScale)
  {
	// Currently simply scaling with current marker's scale instead of searching for scales down from 1 until markers do not overlap,
	// and without moving the scaled markers withing their original bbox

	NFmiFillRect r(std::make_pair(Point(mx - (markerWidth / 2.0),my - (markerHeight / 2.0)),Point(mx + (markerWidth / 2.0),my + (markerHeight / 2.0))));
	int overlap = fillAreaOverlapMax;

	for (FillAreas::const_iterator rrit = reservedAreas.begin(); (rrit != reservedAreas.end()); rrit++)
		for (NFmiFillAreas::const_iterator rit = rrit->second.fillAreas.begin(); (rit != rrit->second.fillAreas.end()); rit++) {
			if (
				(r.first.x < rit->second.x) && (r.second.x >= (rit->first.x + overlap)) &&
				(r.first.y < rit->second.y) && (r.second.y >= (rit->first.y + overlap))
			   ) {
				texts[rrit->first + "SCALE"].str("");
				texts[rrit->first + "SCALE"] << std::fixed << std::setprecision(1) << "scale(" << rrit->second.scale * xScale << "," << rrit->second.scale * yScale << ")";
			}
		}
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief	Adjust marker position depending on it's location on the area.
   */
  // ----------------------------------------------------------------------

  void adjustMarkerPosition(double minx,double axisWidth,double xStep,double sOffset,double eOffset,int n0,int nN,int nS,double & mx,double & my)
  {
	// To better keep the marker within the area adjust the x -position slightly if using area's
	// first or last elevation.
	//
	// For single elevation areas make adjustment only for document's first and last time instant's elevations.
	// Take the extension offsets (how much the elevation exceeds the begin or end of the x -axis) into
	// account when calculating the marker position.

	if ((nS == n0) || (nS == nN)) {
		if (nN != n0)
			mx -= (((nS == n0) ? -1 : 1) * (xStep / 5));
		else if (minx < (xStep / 2))
			mx += ((xStep / 5) - sOffset);
		else if (fabs(axisWidth - minx) < (xStep / 2))
			mx -= ((xStep / 5) - eOffset);
	}
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Get area's marker (label or symbol) position(s); return one
   *		position (markerX and markerY) for each separate visible part
   *		of the area.
   *
   *		The available positions are checked against 'holeAreas' and 'reservedAreas'.
   *		If the area is a hole, all available positions are added to 'holeAreas'.
   *		The selected positions are added to 'reservedAreas'.
   *
   *		Note: Separate 'holeAreas' is used by each feature to ignore feature's
   *			  holes. 'reservedAreas' is used across features.
   */
  // ----------------------------------------------------------------------

  void getAreaMarkerPos(Texts & texts,const std::string & markerId,const std::list<DirectPosition> & curvePoints,NFmiFillAreas & holeAreas,FillAreas & reservedAreas,FillAreas & freeAreas,FillAreas & candidateAreas,bool isHole,bool bbCenterMarker,bool textOutput,
		  	  	  	    int windowWidth,int windowHeight,
		  	  	  	    double minx,double axisHeight,double axisWidth,double xStep,double minh,double sOffset,double eOffset,
		  	  	  	    int markerWidth,int markerHeight,double markerScale,
		  	  	  	    std::vector<double> & _lopx,std::vector<double> & _hipx,std::vector<bool> & hasHole,
		  	  	  	    std::list<double> & markerX,std::list<double> & markerY,
		  	  	  	    std::list<double> & scaleX,std::list<double> & scaleY,
		  	  	  	    boost::ptr_map<std::string,std::ostringstream> * areasOut = NULL,const std::string & areaPlaceHolder = "")
  {
	// Get fill areas. axisWidth (instead of windowWidth) is passed to getFillAreas() to ignore possible fill areas outside
	// (data may contain an extra time step before and after) the document time range

	NFmiFillMap fillMap;
	NFmiFillAreas fillAreas;
	std::list<DirectPosition>::const_iterator cpend = curvePoints.end(),itcp = curvePoints.begin(),pitcp = curvePoints.begin();
	std::list<std::pair<double,double> > scales;
	double mw = markerWidth,mh = markerHeight,lopx,mx,my,xScale = 1.0,yScale = 1.0;
	size_t nMarkers = 0;

	for (itcp++; (itcp != cpend); itcp++, pitcp++)
		fillMap.Add(pitcp->getX(),pitcp->getY(),itcp->getX(),itcp->getY());

	fillMap.getFillAreas(axisWidth,windowHeight,markerWidth,markerHeight,1.0,false,fillAreas,isHole && textOutput,false,isHole && (!textOutput));

	if (isHole && (!textOutput)) {
		// Just reserve the area of the hole
		//
		holeAreas.insert(holeAreas.end(),fillAreas.begin(),fillAreas.end());

		return;
	}

//int TRAP = 0;
	if (!fillAreas.empty()) {
		// Move reserved fill areas into candidate container
		//
		splitFillAreas(fillAreas,markerWidth,1.0);
		eraseReservedAreas("hole","hole",holeAreas,candidateAreas,fillAreas,true);
		fillAreas.sort(sortFillAreas);
//if (TRAP != 0)
//fillAreas.clear();
	}

	if (isHole) {
		// Reserve the area of the hole ('fillAreas' contain fill areas suitable for the label, load/save the area of the hole as fill areas too)
		//
		NFmiFillAreas holeFillAreas;

		fillMap.getFillAreas(windowWidth,windowHeight,0,0,0.0,false,holeFillAreas,false,true);
		holeAreas.insert(holeAreas.end(),holeFillAreas.begin(),holeFillAreas.end());
	}

	NFmiFillAreas::iterator area = fillAreas.begin();

	for (int npx = 0,nX = 0,n0 = 0; (npx < ((int) _lopx.size())); ) {
		// Step until end of data or area is no longer visible
		//
		lopx = _lopx[npx];

		if (lopx > 0) {
			if (nX == 0)
				// First visible time instant
				//
				n0 = npx;

			if (bbCenterMarker && (npx > 0)) {
				// Keep track of bounding box
				//
				double hipx = std::max(_hipx[npx],0.0);

				if ((nX == 0) || (lopx > _lopx[0]))
					_lopx[0] = lopx;
				if ((nX == 0) || (hipx < _hipx[0]))
					_hipx[0] = hipx;
			}

			nX++;
		}

		npx++;

		if (((lopx > 0) && (npx < ((int) _lopx.size()))) || (nX == 0))
			continue;

		nMarkers++;
		std::string mId(markerId + boost::lexical_cast<std::string>(nMarkers));

		if (!bbCenterMarker) {
			// Use a fill area if available
			//
			NFmiFillAreas areas;

			if (area != fillAreas.end()) {
				int timeSteps = nX;

				if (npx < ((int) _lopx.size()))
					for (int idx = npx; ((idx < ((int) _lopx.size())) && (_lopx[idx] <= 0)); idx++)
						timeSteps++;

				for ( ; ((area != fillAreas.end()) && (area->first.x < (minx + ((n0 + timeSteps) * xStep)))); area++)
				{
					// Ignore areas going below 0 over 1/3'rd

					if (area->second.y <= (axisHeight + ((area->second.y - area->first.y) / 3))) {
						areas.push_back(*area);
						scales.push_back(std::make_pair(1.0,1.0));
					}
//fprintf(stderr,"\t%s n0=%d nN=%d xLim=%.0f bl=%.0f,%.0f tr=%.0f,%.0f\n",area->second.y <= (axisHeight + ((area->second.y - area->first.y) / 3)) ? "Pick" : "Skip",n0,n0 + timeSteps,(minx + ((n0 + timeSteps) * xStep)),area->first.x,area->first.y,area->second.x,area->second.y);
				}

				if (!areas.empty()) {
					eraseReservedAreas(mId,reservedAreas,candidateAreas,areas);

					if (!areas.empty()) {
						if (areasOut && (!areaPlaceHolder.empty())) {
							boost::ptr_map<std::string,std::ostringstream> & texts = *areasOut;

							texts[areaPlaceHolder] << "<line x1=\"" << (minx + ((n0 + timeSteps) * xStep)) << "\" y1=\"0\""
												   <<      " x2=\"" << (minx + ((n0 + timeSteps) * xStep)) << "\" y2=\"" << axisHeight
												   <<      "\" style=\"stroke:rgb(255,0,0);stroke-width:2\"/>";

							const char *clrs[] =
							{ "red",
							  "blue",
							  "green",
							  "orange",
							  "brown",
							  "yellow"
							};

							size_t clrCnt = (sizeof(clrs) / sizeof(char *)),clrIdx = 0;

							for (NFmiFillAreas::const_iterator iter = areas.begin(); (iter != areas.end()); iter++) {
								texts[areaPlaceHolder] << "<rect x=\"" << iter->first.x
													   << "\" y=\"" << iter->first.y
													   << "\" width=\"" << iter->second.x - iter->first.x
													   << "\" height=\"" << iter->second.y - iter->first.y
													   << "\" fill=\"" << clrs[clrIdx % clrCnt]
													   << "\" stroke=\"" << clrs[clrIdx % clrCnt] << "\"/>\n";
								clrIdx++;
							}
						}
//fprintf(stderr,"\n");

//						// Note: Symbols are centered to the selected position, labels have their height as the rendered y -coordinate (x is 0);
//						//		 adjust the position (used as tranformation offsets) for a label to center it to the selected position
//
//						NFmiFillRect markerRect = getCenterFillAreaRect(areas,markerWidth,1.0);
//						markerX.push_back(markerRect.first.x + (textOutput ? 0 : ((markerRect.second.x - markerRect.first.x) / 2)));
//						markerY.push_back(markerRect.first.y + (textOutput ? 0 : ((markerRect.second.y - markerRect.first.y) / 2)));
//						scaleX.push_back(1.0);
//						scaleY.push_back(1.0);
//
//						reservedAreas.push_back(std::make_pair(Point(markerRect.first.x,markerRect.first.y),Point(markerRect.second.x,markerRect.second.y)));
////fprintf(stderr,"\tFR Reserve bl=%.0f,%.0f tr=%.0f,%.0f\n",markerRect.first.x,markerRect.first.y,markerRect.second.x,markerRect.second.y);
//
//						nX = 0;
//						continue;
					}
				}
			}

			// Use 1 or 2 elevations in the middle or a elevation as near as possible to the middle of
			// the area in x -direction and having enough height for the marker.
			//
			// To favour the center of the area, use the nearest middle position having at least 70% of
			// the selected position's height.
			//
			// Note: The holes are now checked with fill areas ('hasHole' not used anymore):
			//
			// // If the 2 middle elevations have holes, use the taller one; by using the center of the
			// // elevations without additional checks the marker might end up in a hole.
			//

			double selmx = 0.0,selmy = 0.0;			// Selected marker position
			double selmfvx = 0.0,selmfvy = 0.0;		// Favoured marker position
			double selh = 0.0,selfvh = 0.0,h;		// Height of marker position
			double selfrh = 0.0;					// Height of free marker position
			double fvlim = 0.7;						// Using 70% as the favouring limit
			bool nearest = false;					// Set if the primary position (middle elevation) did not have enough room for the marker
			bool multiple = false,fvmul = false;	// Set if using center of 2 middle elevations as marker position

			int n = n0 + ((nX - 1) / 2);		// Middle elevation
			int nS = n;							// Selected elevation
			int nN = n0 + nX - 1;				// Last elevation
			int n2 = n + ((nX > 1) ? 1 : 0);	// Right side elevation
			int n1 = n2 - ((nX > 1) ? 1 : 0);	// Left side elevation
			int selhoff = -1;					// Index of selected marker position
			int selfroff = -1;					// Index of heighest free marker position
			int selfvoff = -1;					// Index of favoured free marker position

			for ( ; (n2 <= nN); ) {
				if (nearest || (nX % 2) /* The holes are now checked with fill areas: || hasHole[n1] || hasHole[n2] */) {
					if (nearest || ((nX % 2) == 0)) {
						// Select the taller elevation
						//
						// Note: The holes are now checked with fill areas ...
						//
//						n = (((_lopx[n2] - _hipx[n2]) > (_lopx[n1] - _hipx[n1])) ? n2 : n1);
//
//						n2++;
//						n1--;

						// ... scanning the left side first

						if (n1 >= 0) {
							n = n1;
							n1--;
						}
						else {
							n = n2;
							n2++;
						}
					}
					else
						n1--;

					double lo = std::min(_lopx[n],axisHeight),hi = std::max(_hipx[n],0.0);

					mx = minx + ((nS = n) * xStep);
					my = lo - ((lo - hi) / 2);

					// Adjust x -position if using areas first or last elevation

					adjustMarkerPosition(minx,axisWidth,xStep,sOffset,eOffset,n0,nN,nS,mx,my);

					h = lo - hi;

					multiple = false;
//fprintf(stderr,"\tCheck (%d,%d) %d mx=%.0f my=%.0f h=%.0f\n",n2,n1,n,mx,my,h);
				}
				else {
					double lo2 = std::min(_lopx[n2],axisHeight),hi2 = std::max(_hipx[n2],0.0);
					double lo1 = std::min(_lopx[n1],axisHeight),hi1 = std::max(_hipx[n1],0.0);

					mx = minx + (n1 * xStep) + (xStep / 2);

					double y2 = lo2 - ((lo2 - hi2) / 2);
					double y1 = lo1 - ((lo1 - hi1) / 2);

					if (y2 > y1) {
						my = lo1 - ((lo1 - hi2) / 2);
						h = lo2 - hi2;
					}
					else {
						my = lo2 - ((lo2 - hi1) / 2);
						h = lo1 - hi1;
					}

					n2++;
					n1--;
					n = nN;

					multiple = true;
//fprintf(stderr,"\tCheck %d,%d mx=%.0f my=%.0f h=%.0f\n",n2,n1,mx,my,h);
				}

				// Check the current x -position and 1/4 and 1/2 timesteps backwards/forwards if there's free space for the marker.
				//
				// Scale the marker down to half width/height if needed

				double x = mx;
				NFmiFillAreas markerArea = getMarkerArea(mId,holeAreas,reservedAreas,candidateAreas,x,my,markerWidth,markerHeight,(n > n0),(n < nN),xStep);

				if (!markerArea.empty()) {
					// Free, store the center point and scale
					//
					areas.push_back(std::make_pair(Point(x - (markerWidth / 2),my - (markerHeight / 2)),Point(x + (markerWidth / 2),my + (markerHeight / 2))));
					scales.push_back(std::make_pair(markerWidth / mw,markerHeight / mh));

					if (selfrh < h) {
						// Highest free position
						//
						selfrh = h;
						selfroff = areas.size();
					}
				}

				markerWidth = mw;
				markerHeight = mh;

				if ((h > minh) || (h > selh)) {
					selh = h;
					selmx = mx;
					selmy = my;

					if (!markerArea.empty()) {
						// Select the last fill area
						//
						selhoff = areas.size();

						if (selh >= minh)
							break;
					}

					if (selfvh < (fvlim * selh)) {
						selfvh = selh;
						selmfvx = selmx;
						selmfvy = selmy;
						fvmul = multiple;

						if (!markerArea.empty())
							// Favour the last fill area
							//
							selfvoff = areas.size();
					}
				}

				nearest = true;
			}

//if (TRAP != 0) {
////areas.clear(); reservedAreas.clear(); freeAreas.clear(); candidateAreas.clear();
////
////reservedAreas["R4"].fillAreas.push_back(std::make_pair(Point(44,44),Point(55,55)));
////reservedAreas["R4"].scales.push_back(std::make_pair(1,1));
////freeAreas["R4"].fillAreas.push_back(std::make_pair(Point(444,444),Point(555,555)));
////freeAreas["R4"].scales.push_back(std::make_pair(1,1));
////
////reservedAreas["R3"].fillAreas.push_back(std::make_pair(Point(33,33),Point(44,44)));
////reservedAreas["R3"].scales.push_back(std::make_pair(1,1));
////freeAreas["R3"].fillAreas.push_back(std::make_pair(Point(333,333),Point(444,444)));
////freeAreas["R3"].scales.push_back(std::make_pair(1,1));
////
////reservedAreas["R2"].fillAreas.push_back(std::make_pair(Point(22,22),Point(33,33)));
////reservedAreas["R2"].scales.push_back(std::make_pair(1,1));
////candidateAreas["R2"].markers.push_back("R3");
////candidateAreas["R2"].fillAreas.push_back(std::make_pair(Point(34,34),Point(45,45)));
////candidateAreas["R2"].areaIds.push_back(1);
////candidateAreas["R2"].scales.push_back(std::make_pair(1,1));
////candidateAreas["R2"].markers.push_back("R4");
////candidateAreas["R2"].fillAreas.push_back(std::make_pair(Point(34,34),Point(45,45)));
////candidateAreas["R2"].areaIds.push_back(1);
////candidateAreas["R2"].scales.push_back(std::make_pair(1,1));
////
////reservedAreas["R1"].fillAreas.push_back(std::make_pair(Point(11,11),Point(22,22)));
////reservedAreas["R1"].scales.push_back(std::make_pair(1,1));
////candidateAreas["R1"].markers.push_back("R2");
////candidateAreas["R1"].fillAreas.push_back(std::make_pair(Point(23,23),Point(34,34)));
////candidateAreas["R1"].areaIds.push_back(1);
////candidateAreas["R1"].scales.push_back(std::make_pair(1,1));
////
////candidateAreas[mId].markers.push_back("R1");
////candidateAreas[mId].fillAreas.push_back(std::make_pair(Point(12,12),Point(23,23)));
////candidateAreas[mId].areaIds.push_back(1);
////candidateAreas[mId].scales.push_back(std::make_pair(1,1));
//FillAreas::const_iterator rit = reservedAreas.begin();
//areas.clear(); candidateAreas[mId].fillAreas.push_back(rit->second.fillAreas.front());
//}
			if ((areas.size() > 0) || arrangeMarkers(texts,reservedAreas,freeAreas,candidateAreas,candidateAreas.find(mId),areas)) {
				// Select free marker position near the selected position
				//
				std::list<std::pair<double,double> >::iterator csit;
				NFmiFillRect r = selectMarkerPos(mId,areas,scales,freeAreas,selmx,selmy,csit);

				mx = r.first.x + ((r.second.x - r.first.x) / 2);
				my = r.first.y + ((r.second.y - r.first.y) / 2);
				xScale = csit->first;
				yScale = csit->second;
//if (areasOut && (!areaPlaceHolder.empty())) {
//boost::ptr_map<std::string,std::ostringstream> & texts = *areasOut;
//texts[areaPlaceHolder] << "<circle cx=\"" << selmx << "\" cy=\"" << selmy << "\" r=\"5\" stroke=\"black\" stroke-width=\"1\" fill=\"blue\"/>"; }
			}
//			else if ((selhoff >= 0) || (selfroff >= 0)) {
//				// Using the last (selh; selh > minh), favoured (selfvh) or the heighest free (selfrh) fill area
//				//
//				NFmiFillAreas::const_iterator it = areas.begin();
//				std::list<std::pair<double,double> >::const_iterator sit = scale.begin();
//
//				if (selhoff < 0)
//					selhoff = selfvoff = selfroff;
//
//				advance(it,(selh >= minh) ? selhoff - 1 : selfvoff - 1);
//				advance(sit,(selh >= minh) ? selhoff - 1 : selfvoff - 1);
//
//				mx = it->first.x;
//				my = it->first.y;
//				xScale = sit->first;
//				yScale = sit->second;
//			}
			else {
				// No free fill areas, using the highest/favoured position
				//
				if ((selh < minh) || (selfvh >= (fvlim * selh))) {
					mx = selmfvx;
					my = selmfvy;
					multiple = fvmul;
				}
				else {
					mx = selmx;
					my = selmy;
				}

				// Scale marker down to half size and adjust x -position if using areas first or last elevation

				xScale = yScale = 0.5;

				if (!multiple)
					adjustMarkerPosition(minx,axisWidth,xStep,sOffset,eOffset,n0,nN,nS,mx,my);
			}
		}
		else {
			double xmin = minx + (n0 * xStep);
			double xmax = xmin + ((nX - 1) * xStep);

			mx = (xmax - ((xmax - xmin) / 2));
			my = (_lopx[0] - ((_lopx[0] - std::max(_hipx[0],0.0)) / 2));
		}

		// Scale down reserved markers which overlap current marker

		markerWidth = xScale * mw;
		markerHeight = yScale * mh;

		scaleMarkers(texts,reservedAreas,markerWidth,markerHeight,mx,my,xScale,yScale);

		// Note: Symbols are centered to the selected position, labels have their height as the rendered y -coordinate (x is 0);
		//		 adjust the position (used as tranformation offset) for a label to center it to the selected position

		markerX.push_back(mx - (textOutput ? (markerWidth / 2.0) : 0));
		markerY.push_back(my - (textOutput ? (markerHeight / 2.0) : 0));
		scaleX.push_back(xScale);
		scaleY.push_back(yScale);

		reservedAreas[mId].x = mx;
		reservedAreas[mId].y = my;
		reservedAreas[mId].centered = (!textOutput);
		reservedAreas[mId].fillAreas.push_back(std::make_pair(Point(mx - (markerWidth / 2.0),my - (markerHeight / 2.0)),Point(mx + (markerWidth / 2.0),my + (markerHeight / 2.0))));
		reservedAreas[mId].scale = markerScale;
		reservedAreas[mId].scales.push_back(std::make_pair(xScale,yScale));
//fprintf(stderr,"\tNF Reserve bl=%.0f,%.0f tr=%.0f,%.0f\n",mx - (markerWidth / 2.0),my - (markerHeight / 2.0),mx + (markerWidth / 2.0),my + (markerHeight / 2.0));

		nX = 0;
	}
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Render a label to each separate visible part of a area.
   *		For a hole just store the area as reserved.
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::renderAreaLabels(const std::list<DirectPosition> & curvePoints,NFmiFillAreas & holeAreas,const std::string & confPath,const std::string & textName,const std::string & labelPlaceHolder,const std::string & textPosId,bool isHole,const std::string & label,bool bbCenterLabel,
		  	  	  	  	  	  	  	 double minX,int minH,double hFactor,double sOffset,double eOffset,
		  	  	  	  	  	  	  	 std::vector<double> & loPx,std::vector<double> & hiPx,std::vector<bool> & hasHole,
		  	  	  	  	  	  	  	 const std::string & areaPlaceHolder)
  {
	using boost::algorithm::replace_all_copy;

	double minLabelPosHeight = 0.0;
	int textWidth = 0,textHeight = 0;

	if (!label.empty()) {
		// Render the label to fixed position (0,0). The final positions are set afterwards by using transformation offsets
		//
		texts[textName].str("");
		render_text(texts,confPath,textName,label,textWidth,textHeight,true,false,false,textPosId,true);

		minLabelPosHeight = ((minH > 0) ? minH : (hFactor * textHeight));
	}

	// Get and reserve label positions for each separate visible part of the area

	std::list<double> labelX,labelY,scaleX,scaleY;
	std::string tId(textPosId + boost::lexical_cast<std::string>(npointsymbols));
	npointsymbols++;
//fprintf(stderr,">> LABEL %s1 %s\n",tId.c_str(),label.c_str());

	getAreaMarkerPos(texts,tId,curvePoints,holeAreas,reservedAreas,freeAreas,candidateAreas,isHole,bbCenterLabel,!label.empty(),
					 static_cast<int>(std::floor(0.5+area->Width())),
					 static_cast<int>(std::floor(0.5+area->Height())),
					 minX,axisManager->axisHeight(),axisManager->axisWidth(),axisManager->xStep(),minLabelPosHeight,sOffset,eOffset,
					 textWidth,textHeight,1.0,
					 loPx,hiPx,hasHole,
					 labelX,labelY,scaleX,scaleY,
					 areaPlaceHolder.empty() ? NULL : &texts,areaPlaceHolder);

	if (label.empty())
		return;

	// Render the label

	using boost::algorithm::replace_first_copy;

	std::string svg = texts[textName].str(),textPosId0("--" + textPosId + "--"),scaleId0("--" + textPosId + "SCALE--");
	std::list<double>::const_iterator itX,itY,sitX,sitY;

	int placeHolderIndex = 1;

//fprintf(stderr,"\nSvg: %s\n",svg.c_str());
	for (itX = labelX.begin(),itY = labelY.begin(),sitX = scaleX.begin(),sitY = scaleY.begin(); (itX != labelX.end()); itX++, itY++, sitX++, sitY++, placeHolderIndex++) {
		std::string textPosIdN(tId + boost::lexical_cast<std::string>(placeHolderIndex));
		std::string scaleIdN(textPosIdN + "SCALE");
		std::string s(replace_first_copy(svg,textPosId0,"--" + textPosIdN + "--"));

		texts[labelPlaceHolder] << replace_first_copy(s,scaleId0,"--" + scaleIdN + "--");
		texts[textPosIdN] << std::fixed << std::setprecision(1) << *itX << "," << *itY;
		texts[scaleIdN] << std::fixed << std::setprecision(1) << "scale(" << *sitX << "," << *sitY << ")";
//fprintf(stderr,"Out %s=%s %s=%s\n",labelPlaceHolder.c_str(),texts[labelPlaceHolder].str().c_str(),textPosIdN.c_str(),texts[textPosIdN].str().c_str());
	}
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Render a symbol to each separate visible part of a area
   */
  // ----------------------------------------------------------------------

  template <typename T>
  void SvgRenderer::renderAreaSymbols(const T & cg,const std::list<DirectPosition> & curvePoints,NFmiFillAreas & holeAreas,const std::string & confPath,const std::string & symbolPosId,
		  	  	  	  	  	  	  	  double minx,std::vector<double> & loPx,std::vector<double> & hiPx,std::vector<bool> & hasHole,
		  	  	  	  	  	  	  	  const std::string & symClass,const std::string & classNameExt,const std::string & symbol,
		  	  	  	  	  	  	  	  bool asSymbol,bool & visible,bool & aboveTop)
  {
	std::list<const libconfig::Setting *> scope;

	const char * typeMsg = " must contain a group in curly brackets";

	const libconfig::Setting & specs = config.lookup(confPath);
	if(!specs.isGroup())
		throw std::runtime_error(confPath + typeMsg);

	scope.push_back(&specs);

	if (cg.globalScope())
		scope.push_back(cg.globalScope());

	scope.push_back(cg.localScope());

	// Symbol height, width, scale and minimum absolute/relative height for the symbol position

	bool isSet;

	int height = configValue<int>(scope,confPath,"height",s_optional,&isSet);
	if (!isSet)
		height = defaultSymbolHeight;

	int width = configValue<int>(scope,confPath,"width",s_optional,&isSet);
	if (!isSet)
		width = defaultSymbolWidth;

	double scale = configValue<double>(scope,confPath,"scale",s_optional,&isSet);
	if (!isSet)
		scale = 1.0;

	int minSymbolPosHeight = configValue<int>(scope,confPath,"minsymbolposheight",s_optional,&isSet);
	if ((!isSet) || (minSymbolPosHeight < (height * symbolPosHeightFactorMin))) {
		double symbolPosHeightFactor = configValue<double>(scope,confPath,"symbolposheightfactor",s_optional,&isSet);

		if ((!isSet) || (symbolPosHeightFactor < symbolPosHeightFactorMin))
			symbolPosHeightFactor = defaultSymbolPosHeightFactor;

		minSymbolPosHeight = height * scale * symbolPosHeightFactor;
	}

	// Get and reserve symbol position for each separate visible part of the area

	std::list<double> symbolX,symbolY,scaleX,scaleY;
	std::list<double>::const_iterator itx,ity,sitx,sity;
	double axisHeight = axisManager->axisHeight(),x,y;
	std::string sId(symbolPosId + boost::lexical_cast<std::string>(npointsymbols));
	npointsymbols++;
//fprintf(stderr,">> SYMBOL %s1 %s\n",sId.c_str(),symbol.c_str());

	getAreaMarkerPos(texts,sId,curvePoints,holeAreas,reservedAreas,freeAreas,candidateAreas,false,cg.bbCenterMarker(),false,
					 static_cast<int>(std::floor(0.5+area->Width())),
					 static_cast<int>(std::floor(0.5+area->Height())),
					 minx,axisManager->axisHeight(),axisManager->axisWidth(),axisManager->xStep(),minSymbolPosHeight,cg.sOffset(),cg.eOffset(),
					 width * scale,height * scale,scale,
					 loPx,hiPx,hasHole,
					 symbolX,symbolY,scaleX,scaleY,
					 options.debug ? &texts : NULL,options.debug ? cg.placeHolder() + "FILLAREAS" : "");

	// Render the symbol

	int placeHolderIndex = 1;

	for (itx = symbolX.begin(),ity = symbolY.begin(),sitx = scaleX.begin(),sity = scaleY.begin(); (itx != symbolX.end()); itx++, ity++, sitx++, sity++, placeHolderIndex++) {
		render_aerodromeSymbol(confPath,symClass,classNameExt,symbol,(x = *itx),(y = *ity),true,true,*sitx,*sity,sId + boost::lexical_cast<std::string>(placeHolderIndex));

		if (asSymbol) {
			// The symbol should be visible, checking anyway
			//
			if ((y > 0) && (y < axisHeight))
				visible = true;
			else if (y < 0)
				aboveTop = true;
		}
	}
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Group membership detection based on group number
   */
  // ----------------------------------------------------------------------

  bool CategoryValueMeasureGroup::groupMember(bool first,const woml::CategoryValueMeasure * cvm)
  {
	// Store first member of the group or check if group matches

	if (!cvm)
		throw std::runtime_error("CategoryValueMeasureGroup::groupMember: CategoryValueMeasure value expected");

	if (first)
		itsFirstMember = cvm;
	else if (cvm->groupNumber() != itsFirstMember->groupNumber())
		return false;

	return true;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Group membership detection
   */
  // ----------------------------------------------------------------------

  template <typename T>
  GroupCategory<T>::GroupCategory() : CategoryValueMeasureGroup()
  {
  }

  template <typename T>
  bool GroupCategory<T>::groupMember(const woml::CategoryValueMeasure * cvm) const
  {
	// Check if the given member type is known

	if (!cvm)
		throw std::runtime_error("GroupCategory::groupMember: CategoryValueMeasure value expected");

	std::string category(boost::algorithm::to_upper_copy(cvm->category()));

	return (std::find_if(itsGroups.begin(),itsGroups.end(),std::bind2nd(MemberType(),category)) != itsGroups.end());
  }

  template <typename T>
  bool GroupCategory<T>::groupMember(bool first,const woml::CategoryValueMeasure * cvm)
  {
	// Add first member to the group or check if the given member type is compatible with the group

	if (!cvm)
		throw std::runtime_error("GroupCategory::groupMember: CategoryValueMeasure value expected");

	std::string category(boost::algorithm::to_upper_copy(cvm->category()));

	if (first) {
		itcg = std::find_if(itsGroups.begin(),itsGroups.end(),std::bind2nd(MemberType(),category));

		if (itcg == itsGroups.end())
			throw std::runtime_error("GroupCategory::groupMember: unknown category '" + category + "'");

		// The member set is common for all groups; clear it

		itcg->memberSet().clear();

		itsFirstMember = cvm;
	}
	else {
		// Check if elevations overlap (have the same group number) and have matching category
		//
		if (
			(!CategoryValueMeasureGroup::groupMember(false,cvm)) ||
			(std::find_if(itsGroups.begin(),itsGroups.end(),std::bind2nd(MemberType(),category)) != itcg)
		   )
			return false;
	}

	// Add contained member type into the member set

	itcg->addType(category);

	return true;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Return group's symbol.
   */
  // ----------------------------------------------------------------------

  template <typename T>
  const std::string & GroupCategory<T>::groupSymbol() const
  {
	// Search for a group containing only the types in current group, and if found, return it's symbol

	typename std::list<T>::const_iterator group = std::find_if(itsGroups.begin(),itsGroups.end(),std::bind2nd(GroupType<T>(),itcg->memberTypes()));

	return ((group != itsGroups.end()) ? group->symbol() : itcg->symbol());
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Return group's label.
   */
  // ----------------------------------------------------------------------

  template <typename T>
  std::string GroupCategory<T>::groupLabel() const
  {
	// Search for a group containing only the types in current group, and if found, return it's label

	typename std::list<T>::const_iterator group = std::find_if(itsGroups.begin(),itsGroups.end(),std::bind2nd(GroupType<T>(),itcg->memberTypes()));

	return ((group != itsGroups.end()) ? group->label(true) : itcg->label());
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Render group's elevations as cloud symbols
   *
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::render_cloudSymbols(const std::string confPath,
		  	  	  	  	  	  	  	    const ElevGrp & eGrp,
		  	  	  	  	  	  	  	    std::list<CloudGroup>::const_iterator itcg,
		  	  	  	  	  	  	  	    int nGroups)
  {
	ElevGrp::const_iterator egbeg = eGrp.begin(),egend = eGrp.end(),iteg;

	for (iteg = egbeg; (iteg != egend); iteg++) {
		// Get elevation's lo and hi range values
		//
		const woml::Elevation & e = iteg->elevation();
		boost::optional<woml::NumericalSingleValueMeasure> itsBoundedLo = (e.bounded() ? e.lowerLimit() : woml::NumericalSingleValueMeasure());
		const boost::optional<woml::NumericalSingleValueMeasure> & itsLoLimit = (e.bounded() ? itsBoundedLo : e.value());
		boost::optional<woml::NumericalSingleValueMeasure> itsBoundedHi = (e.bounded() ? e.upperLimit() : woml::NumericalSingleValueMeasure());
		const boost::optional<woml::NumericalSingleValueMeasure> & itsHiLimit = (e.bounded() ? itsBoundedHi : e.value());

		double lo = ((!itsLoLimit) ? 0.0 : itsLoLimit->numericValue());
		double hi = ((!itsHiLimit) ? 0.0 : itsHiLimit->numericValue());
		double lopx = axisManager->scaledElevation(lo);
		double hipx = axisManager->scaledElevation(hi);

		// x -coord of this time instant

		const boost::posix_time::ptime & vt = iteg->validTime();
		double x = axisManager->xOffset(vt);

		// Render cloud symbol

		render_cloudSymbol(confPath,*itcg,nGroups,x,lopx,hipx);
	}

	// Remove group's elevations from the collection

	for (iteg = egbeg; (iteg != eGrp.end()); iteg++)
		iteg->Pvs()->get()->editableValues().erase(iteg->Pv());
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Get scaled curve points connecting hi range points and
   *		lo range points for subsequent overlapping elevations.
   *
   *		The scaled lo and hi range values of elevations selected as
   *		candidates for labeling are stored to 'scaledLo' and 'scaledHi'
   *		and 'hasHole' is set to indicate whether the elevations have
   *		holes or not.
   *
   *		Returs true if the curve encapsulates a single elevation only.
   *
   *		Note: The elevations included into the curve are deleted from
   *			  the underlying woml object collection.
   */
  // ----------------------------------------------------------------------

  void addCurvePosition(List<DirectPosition> & curvePositions,List<DirectPosition> & curvePositions0,const DirectPosition & pos,const DirectPosition & pos0)
  {
	curvePositions.push_back(pos);

	if (&curvePositions != &curvePositions0)
		curvePositions0.push_back(pos0);
  }

  void addCurvePosition(List<DirectPosition> & curvePositions,List<DirectPosition> & curvePositions0,double x,double y,double y0,double xOffset = 0.0,double yOffset = 0.0,bool leftOnly = false)
  {
	// Add curve point and if x- (and additionally y-) offset is given, add points on both sides too (to control the curve shape on corners)
	// if leftOnly is false; if leftOnly is true, add only the left side point (going up to the left side hi range point
	// when closing the path to the starting top left corner).
	//
	// Note: y -offset is passed in for top left ('fst') and bottom right ('edn' followed by 'bwd') corners; for top right ('fwd' + 'edn') and
	//		 bottom left ('bwd' + 'eup') corner smoothenCurve() is used

	if (fabs(xOffset) >= 1.0) {
		addCurvePosition(curvePositions,curvePositions0,DirectPosition(x - xOffset,y + yOffset),DirectPosition(x - xOffset,y0 + yOffset));

		if (leftOnly)
			return;
	}

	addCurvePosition(curvePositions,curvePositions0,DirectPosition(x,y + (yOffset / 2)),DirectPosition(x,y0 + (yOffset / 2)));

	if (fabs(xOffset) >= 1.0)
		addCurvePosition(curvePositions,curvePositions0,DirectPosition(x + xOffset,y),DirectPosition(x + xOffset,y0));
  }

  void addCurvePosition(List<DirectPosition> & curvePositions,double x,double y,double xOffset = 0.0,double yOffset = 0.0,bool leftOnly = false)
  {
	// Add curve point and if x- (and additionally y-) offset is given, add points on both sides too (to control the curve shape on corners)
	// if leftOnly is false; if leftOnly is true, add only the left side point (going up to the left side hi range point
	// when closing the path to the starting top left corner).
	//
	// Note: y -offset is passed in for top left ('fst') and bottom right ('edn' followed by 'bwd') corners; for top right ('fwd' + 'edn') and
	//		 bottom left ('bwd' + 'eup') corner smoothenCurve() is used

	if (fabs(xOffset) >= 1.0) {
		curvePositions.push_back(DirectPosition(x - xOffset,y + yOffset));

		if (leftOnly)
			return;
	}

	curvePositions.push_back(DirectPosition(x,y + (yOffset / 2)));

	if (fabs(xOffset) >= 1.0)
		curvePositions.push_back(DirectPosition(x + xOffset,y));
  }

  void smoothenCurve(List<DirectPosition> & curvePositions,double x,double y,double height,double xOffset,double yOffset,bool bottomRight = false)
  {
	// Smoothen curve corner by adjusting y -coordinates with given offset. The corner has three points (two additional points around the actual
	// elevation lo/hi range point at x,y), spaced by xOffset in x -direction and having originally the same y -coodinate.

	if ((fabs(xOffset) < 1.0) || (fabs(yOffset) < 1.0) || (curvePositions.size() < 3))
		return;

	if (fabs(yOffset) > (height / 10))
		yOffset = ((yOffset < 0) ? -1 : 1) * (height / 10);

	// Erase the points to be adjusted; the actual/center point and the second/latter additional point

	curvePositions.pop_back();
	curvePositions.pop_back();

	if (!bottomRight) {
		curvePositions.push_back(DirectPosition(x,y + (yOffset / 2)));
		curvePositions.push_back(DirectPosition(x + xOffset,y + yOffset));
	}
	else {
		// Must adjust all three points for bottom right corner
		//
		curvePositions.pop_back();
		addCurvePosition(curvePositions,x,y,xOffset,yOffset);
	}
  }

  bool SvgRenderer::scaledCurvePositions(ElevGrp & eGrp,
		  	  	  	  	  	  	  	     const boost::posix_time::ptime & bt,const boost::posix_time::ptime & et,
		  	  	  	  	  	  	  	     List<DirectPosition> & curvePositions,List<DirectPosition> & curvePositions0,
		  	  	  	  	  	  	  	     std::vector<double> & scaledLo,std::vector<double> & scaledHi,
		  	  	  	  	  	  	  	     std::vector<bool> & hasHole,
		  	  	  	  	  	  	  	     double xOffset,double yOffset,
		  	  	  	  	  	  	  	     double vOffset,double vSOffset,double vSSOffset,
		  	  	  	  	  	  	  	     int sOffset,int eOffset,
		  	  	  	  	  	  	  	     int scaleHeightMin,int scaleHeightRandom,
		  	  	  	  	  	  	  	     std::ostringstream & path,
		  	  	  	  	  	  	  	     bool * isVisible,
		  	  	  	  	  	  	  	     bool checkGround,
		  	  	  	  	  	  	  	     bool nonGndFwd2Gnd,
		  	  	  	  	  	  	  	     bool nonGndVdn2Gnd
		  	  	  	  	  	  	  	    )
  {
	ElevGrp::iterator egbeg = eGrp.begin(),egend = eGrp.end(),iteg;
	Phase phase,pphase;
	double lopx = 0.0,plopx,lopx0 = 0.0,plopx0;
	double hipx = 0.0,phipx,hipx0 = 0.0,phipx0;
	double maxx = 0.0;
	bool single = true,smoothen = false;

	double axisWidth = axisManager->axisWidth(),xStep = axisManager->xStep(),nonZ = axisManager->nonZeroElevation();

	srand(time(NULL));

	// x -offset for additional points generated on both sides of the elevation's lo/hi range points can't exceed
	// half of the timestep width (an intermediate point is always generated halfway of subsequent elevations)

	xOffset = ((xOffset < 0.0) ? 0.0 : std::min(xOffset,(xStep / 2)));

	curvePositions.clear(); curvePositions0.clear();
	addCurvePosition(curvePositions,curvePositions0,0,0,0);

	for (iteg = egbeg, phase = pphase = fst; (iteg != egend); ) {
		// Get elevation's lo and hi range values
		//
		const woml::Elevation & e = iteg->elevation();
		boost::optional<woml::NumericalSingleValueMeasure> itsBoundedLo = (e.bounded() ? e.lowerLimit() : woml::NumericalSingleValueMeasure());
		const boost::optional<woml::NumericalSingleValueMeasure> & itsLoLimit = (e.bounded() ? itsBoundedLo : e.value());
		boost::optional<woml::NumericalSingleValueMeasure> itsBoundedHi = (e.bounded() ? e.upperLimit() : woml::NumericalSingleValueMeasure());
		const boost::optional<woml::NumericalSingleValueMeasure> & itsHiLimit = (e.bounded() ? itsBoundedHi : e.value());

		double lo = ((!itsLoLimit) ? 0.0 : itsLoLimit->numericValue());
		double hi = ((!itsHiLimit) ? 0.0 : itsHiLimit->numericValue());

		if ((lo > 0.0) && (lo < nonZ))
			lo = 0.0;

		bool ground = (checkGround && (lo < nonZ));

		if (isVisible && (hi >= nonZ))
			*isVisible = true;

		plopx = lopx; plopx0 = lopx0;
		phipx = hipx; phipx0 = hipx0;

		// Adjust baseline points to keep decorated curve baseline at the right level.
		// The unadjusted baseline is stored too if requested (it is used for cloud labeling).
		//
		// Note: 1/6'th of the decoration control point offset just seems to do the job well enough.

		double lOffset = scaleHeightMin,hOffset = scaleHeightMin;

		if (scaleHeightRandom > 0) {
			lOffset += ((scaleHeightRandom + 1) * (rand() / (RAND_MAX + 1.0)));
			hOffset += ((scaleHeightRandom + 1) * (rand() / (RAND_MAX + 1.0)));
		}

		lopx0 = lopx = axisManager->scaledElevation(lo);
		hipx0 = hipx = axisManager->scaledElevation(hi);

		double lopxBL = lopx - (lOffset / 6);
		double hipxBL = hipx + (hOffset / 6);

		if (lopxBL > hipxBL) {
			lopx = lopxBL;
			hipx = hipxBL;
		}

		// x -coord of this time instant

		const boost::posix_time::ptime & vt = iteg->validTime();
		double x = axisManager->xOffset(vt);

		// Keep track of elevations to calculate the label position(s)

		if ((x >= 0) && (x < (axisWidth + 1)))
			trackAreaLabelPos(iteg,x,maxx,lopx0,scaledLo,hipx0,scaledHi,hasHole);

#define STEPSS

#ifdef STEPS
std::string cs;
if (iteg->topConnected()) cs = " top";
if (iteg->bottomConnected()) cs += " bot";
cs += (" sz=" + boost::lexical_cast<std::string>(eGrp.size()));
#endif

		if (phase == fst) {
			// First hi range point
			//
			// If time instant of the first "ground" (zeroTolerance) elevation is not forecast's first
			// time instant, connect hi range point to 0 at the previous time instant
			//
#ifdef STEPS
fprintf(stderr,"**** fst hi=%.0f %s\n",hi,cs.c_str());
#endif
			if (ground && (vt > bt)) {
				addCurvePosition(curvePositions,curvePositions0,x - xStep,axisManager->axisHeight(),axisManager->axisHeight());
				single = false;
			}

			addCurvePosition(curvePositions,curvePositions0,x,hipx,hipx0,xOffset,yOffset);
			iteg->topConnected(true);

			if (options.debug) {
				if (ground && (vt > bt))
					path << "M" << (x - xStep) << "," << axisManager->axisHeight()
						 << " L" << x << "," << hipx;
				else
					path << "M" << x << "," << hipx;
			}

			phase = fwd;
		}
		else if (phase == fwd) {
			// Move forward thru intermediate point connecting to next elevation's hi range point
			//
#ifdef STEPS
fprintf(stderr,">>>> fwd hi=%.0f %s\n",hi,cs.c_str());
#endif
			double ipx = ((hipx < phipx) ? hipx : phipx) + (fabs(hipx - phipx) / 2);
			double ipx0 = ((hipx0 < phipx0) ? hipx0 : phipx0) + (fabs(hipx0 - phipx0) / 2);

			addCurvePosition(curvePositions,curvePositions0,x - (xStep / 2),ipx,ipx0);
			addCurvePosition(curvePositions,curvePositions0,x,hipx,hipx0,xOffset);
			iteg->topConnected(true);

			if (options.debug)
				path << " L" << x << "," << hipx;
		}
		else if (phase == vup) {
			// Move up connecting to upper elevation's lo range point.
			//
			// Generate intermediate point half timestep forwards
			//
#ifdef STEPS
fprintf(stderr,">>>> vup lo=%.0f %s\n",lo,cs.c_str());
#endif
			addCurvePosition(curvePositions,curvePositions0,x + (xStep / 2),phipx - ((phipx - lopx) / 2),phipx0 - ((phipx0 - lopx0) / 2));
			addCurvePosition(curvePositions,curvePositions0,x,lopx,lopx0);
			iteg->bottomConnected(true);

			if (options.debug)
				path << " L" << x << "," << lopx;

			phase = bwd;
		}
		else if (phase == vdn) {
			// Move down connecting to lower elevation's hi range point
			//
			// Generate intermediate point half timestep backwards
			//
#ifdef STEPS
fprintf(stderr,">>>> vdn hi=%.0f %s\n",hi,cs.c_str());
#endif
			addCurvePosition(curvePositions,curvePositions0,x - (xStep / 2),hipx - ((hipx - plopx) / 2),hipx0 - ((hipx0 - plopx0) / 2));
			addCurvePosition(curvePositions,curvePositions0,x,hipx,hipx0);
			iteg->topConnected(true);

			if (options.debug)
				path << " L" << x << "," << hipx;

			phase = fwd;
		}
		else if (phase == eup) {
			// Move up connecting "nonground" (or non ZeroTolorence) elevation's lo range point to hi range point
			//
			// Note: ZeroTolorence "ground" elevation's hi range point can be connected to 0 at previous/next time instant
			//
#ifdef STEPS
fprintf(stderr,">>>> eup hi=%.0f %s\n",hi,cs.c_str());
#endif
			// The curve turns up, smoothen the corner
			smoothenCurve(curvePositions,x,lopx,lopx - hipx,-xOffset,-yOffset);

			if (&curvePositions != &curvePositions0)
				smoothenCurve(curvePositions0,x,lopx0,lopx0 - hipx0,-xOffset,-yOffset);

			if ((!ground) || (vt == bt) || (!(iteg->topConnected()))) {
				// Limit the extent of the left side of first time instant
				//
				double offset = (((vt == bt) && (vOffset > sOffset)) ? sOffset : vOffset);

				addCurvePosition(curvePositions,curvePositions0,x - offset,lopx - ((lopx - hipx) / 2),lopx0 - ((lopx0 - hipx0) / 2));
				addCurvePosition(curvePositions,curvePositions0,x,hipx,hipx0,xOffset,0.0,iteg->topConnected());

				if (options.debug)
					path << (((vt != bt) && (vt != et)) ? " L" : " M") << x << "," << hipx;
			}

			if (iteg->topConnected())
				// Path is now closed
				//
				break;

			iteg->topConnected(true);

			phase = fwd;
		}
		else if (phase == edn) {
			// Move down connecting elevation's hi range point to lo range point
			//
			// If time instant of the last "ground" (zeroTolerance) elevation in the group is not forecast's last
			// time instant, use 0 at the next time instant as intermediate point
			//
#ifdef STEPS
fprintf(stderr,">>>> edn lo=%.0f %s\n",lo,cs.c_str());
#endif
			// The curve turns down, smoothen the corner
			smoothenCurve(curvePositions,x,hipx,lopx - hipx,xOffset,yOffset);

			if (&curvePositions != &curvePositions0)
				smoothenCurve(curvePositions0,x,hipx0,lopx0 - hipx0,xOffset,yOffset);

			if (ground && (vt < et)) {
				addCurvePosition(curvePositions,curvePositions0,x + xStep,axisManager->axisHeight(),axisManager->axisHeight());
				single = false;
			}
			else {
				// Limit the extent of the right side of last time instant.
				//
				double offset = (((vt == et) && (vOffset > eOffset)) ? eOffset : vOffset);

				addCurvePosition(curvePositions,curvePositions0,x + offset,lopx - ((lopx - hipx) / 2),lopx0 - ((lopx0 - hipx0) / 2));
			}

			addCurvePosition(curvePositions,curvePositions0,x,lopx,lopx0,-xOffset);
			iteg->bottomConnected(true);

			if (options.debug) {
				if (ground && (vt < et))
					path << " L" << (x + xStep) << "," << axisManager->axisHeight();

				path << (((vt != bt) && (vt != et)) ? " L" : " M") << x << "," << lopx;
			}

			pphase = phase;
			phase = bwd;
		}
		else if (phase == bwd) {
			// Move backward thru intermediate point connecting to previous elevation's lo range point
			//
#ifdef STEPS
fprintf(stderr,">>>> bwd lo=%.0f %s\n",lo,cs.c_str());
#endif
			if (smoothen) {
				// The curve turns backwards, smoothen the corner
				//
				smoothenCurve(curvePositions,x + xStep,plopx,plopx - phipx,-xOffset,-yOffset,true);

				if (&curvePositions != &curvePositions0)
					smoothenCurve(curvePositions0,x + xStep,plopx0,plopx0 - phipx0,-xOffset,-yOffset,true);
			}

			double ipx = ((lopx < plopx) ? lopx : plopx) + (fabs(lopx - plopx) / 2);
			double ipx0 = ((lopx0 < plopx0) ? lopx0 : plopx0) + (fabs(lopx0 - plopx0) / 2);

			addCurvePosition(curvePositions,curvePositions0,x + (xStep / 2),ipx,ipx0);
			addCurvePosition(curvePositions,curvePositions0,x,lopx,lopx0,-xOffset);
			iteg->bottomConnected(true);

			if (options.debug)
				path << " L" << x << "," << lopx;
		}
		else {	// lst
			if (single) {
				// Bounding box for single elevation. Limit the extent of the left side
				// of first time instant and of the right side of last time instant.
				//
				double leftSide = x - (((vt == bt) && (vSOffset > sOffset)) ? sOffset : vSOffset) + 1;
				double rightSide = x + (((vt == et) && (vSOffset > eOffset))? eOffset : vSOffset) - 1;

				if (options.debug) {
					path.clear();
					path.str("");

					path << "M" << leftSide << "," << hipx
						 << " L" << rightSide << "," << hipx
						 << " L" << rightSide << "," << lopx
						 << " L" << leftSide << "," << lopx
						 << " L" << leftSide << "," << hipx;
				}

				curvePositions.clear(); curvePositions0.clear();

				addCurvePosition(curvePositions,curvePositions0,0,0,0);
				addCurvePosition(curvePositions,curvePositions0,leftSide,hipx,hipx0,1);
				addCurvePosition(curvePositions,curvePositions0,rightSide,hipx,hipx0,1);
				addCurvePosition(curvePositions,curvePositions0,rightSide + ((vSSOffset < 0.0) ? 0.0 : vSSOffset),lopx - ((lopx - hipx) / 2),lopx0 - ((lopx0 - hipx0) / 2));
				addCurvePosition(curvePositions,curvePositions0,rightSide,lopx,lopx0,-1);
				addCurvePosition(curvePositions,curvePositions0,leftSide,lopx,lopx0,-1);
				addCurvePosition(curvePositions,curvePositions0,leftSide - ((vSSOffset < 0.0) ? 0.0 : vSSOffset),lopx - ((lopx - hipx) / 2),lopx0 - ((lopx0 - hipx0) / 2));
				addCurvePosition(curvePositions,curvePositions0,leftSide,hipx,hipx0,1,0.0,true);

				break;
			}

			// Close the path
			//
			phase = eup;

			continue;
		}

		smoothen = false;

		if (phase == fwd)
			phase = uprightdown(eGrp,iteg,lo,hi,nonGndFwd2Gnd);
		else
			phase = downleftup(eGrp,iteg,lo,hi,nonGndVdn2Gnd);

		if ((phase == fwd) || (phase == bwd)) {
			// Path covers multiple time instants
			//
			single = false;

			// Check for backwards turn to smoothen the curve edge
			smoothen = ((pphase == edn) && (phase == bwd));

			pphase = phase;
		}
	}	// for iteg

	// Use the second last curve position - in the middle of the left side of the starting / top left elevation - as
	// the starting position to eliminate the "starting n�kk�nen" effect, which appeared especially with single elevations

	curvePositions[0] = curvePositions[curvePositions.size() - 2];
	curvePositions.pop_back();

	if (&curvePositions != &curvePositions0) {
		curvePositions0[0] = curvePositions0[curvePositions0.size() - 2];
		curvePositions0.pop_back();
	}

	// Remove group's elevations from the collection

	for (iteg = egbeg; (iteg != eGrp.end()); iteg++)
		if ((!(iteg->isGenerated())) && iteg->topConnected() && iteg->bottomConnected()) {
			iteg->Pvs()->get()->editableValues().erase(iteg->Pv());
			iteg->isDeleted(true);
		}

	return single;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Cloud layer rendering
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::render_timeserie(const woml::CloudLayers & cloudlayers)
  {
	// 'cloudSet' stores cloud types (their starting positions in the group's 'types' setting) contained in current group;
	// if group's label is not given, label (comma separated list of types) is build using the order
	// the types are defined in 'types' setting
	//
	// The cloud set is cleared and then set by all the groups; cloud types contained are
	// added to the set when collecting a group in elevationGroup()

	const std::string confPath("CloudLayers");
	const std::string CLOUDLAYERS(boost::algorithm::to_upper_copy(confPath));

	bool visible = false,aboveTop = false,borderCompensation;
	double axisWidth = axisManager->axisWidth(),tightness,labelPosHeightFactor;
	int minLabelPosHeight;
	CloudGroupCategory cloudGroupCategory;

	std::set<size_t> cloudSet;

	// Get cloud grouping configuration

	cloudLayerConfig(confPath,tightness,borderCompensation,minLabelPosHeight,labelPosHeightFactor,cloudGroupCategory.groups(),cloudSet);

	// Cloud layer time series

	const std::list<woml::TimeSeriesSlot> & ts = cloudlayers.timeseries();

	// Elevation group and curve point data

	ElevGrp eGrp;
	int nGroups = 0;

	List<DirectPosition> curvePositions,curvePositions0;
	std::list<DirectPosition> curvePoints,curvePoints0;
	std::list<DoubleArr> decoratorPoints;

	// Set unique group number for members of each overlapping group of elevations

	setGroupNumbers(ts);

	// Search and flag elevation holes

	NFmiFillAreas holeAreas;
	bool hasHole = searchHoles(ts,&cloudGroupCategory);

	boost::posix_time::ptime bt,et;

	for ( ; ; ) {
		// Get group of overlapping elevations from the time serie
		//
		// Note: Holes are loaded first; they (fill areas extracted from them) are added to reserved areas
		//		 before loading the clouds and positioning the labels
		//
		elevationGroup(ts,bt,et,eGrp,false,true,hasHole,&cloudGroupCategory);

		if (eGrp.size() == 0) {
			if (hasHole) {
				hasHole = false;
				continue;
			}

			break;
		}

		nGroups++;

		// Cloud group for current elevation group

		std::list<CloudGroup>::const_iterator itcg = cloudGroupCategory.currentGroup();
		bool isHole = eGrp.front().isHole();

		if ((!isHole) && !(itcg->symbolType().empty())) {
			// Rendering the group as cloud symbols
			//
			render_cloudSymbols(confPath,eGrp,itcg,nGroups);
			continue;
		}

		// Rendering the group as bezier curve; get curve positions for bezier creation.
		//
		// The scaled lo/hi values for the elevations are returned in scaledLo/scaledHi;
		// they are used to calculate cloud label positions.
		//
		// Use cloud decoration offsets and get the unmodified curve baseline too (for label positioning)
		// if the decoration is compensated when constructing the base line

		std::vector<double> scaledLo,scaledHi;
		std::vector<bool> hasHole;

		int scaleHeightMin = (borderCompensation ? itcg->scaleHeightMin() : 0);
		int scaleHeightRandom = (borderCompensation ? itcg->scaleHeightRandom() : 0);

		std::ostringstream path;
		path << std::fixed << std::setprecision(3);

		scaledCurvePositions(eGrp,bt,et,curvePositions,borderCompensation ? curvePositions0 : curvePositions,scaledLo,scaledHi,hasHole,
							 itcg->xOffset(),itcg->yOffset(),itcg->vOffset(),itcg->vSOffset(),itcg->vSSOffset(),itcg->sOffset(),itcg->eOffset(),
							 scaleHeightMin,scaleHeightRandom,path);

		if (options.debug)
			texts[itcg->placeHolder() + "PATH"] << "<path class=\"" << itcg->classDef()
												<< "\" id=\"" << "CloudLayersB" << nGroups
												<< "\" d=\""
												<< path.str()
												<< "\"/>\n";

		// Create bezier curve and get curve and decorator points

//std::ostringstream pnts; pnts << std::fixed << std::setprecision(3);
//{
//List<DirectPosition>::iterator cpbeg = curvePositions.begin(),cpend = curvePositions.end(),itcp;
//for (itcp = cpbeg; (itcp != cpend); itcp++) {
//pnts << ((itcp == cpbeg) ? "M" : " L") << itcp->getX() << "," << itcp->getY(); }
//}
//--
//{ std::cerr << std::fixed << std::setprecision(1);
//List<DirectPosition>::iterator cpbeg = curvePositions.begin(),cpend = curvePositions.end(),itcp; size_t n = 0;
//for (itcp = cpbeg; (itcp != cpend); itcp++, n++) {
//if ((n % 10) == 0) std::cerr << std::endl << n << ": "; else std::cerr << " "; std::cerr << itcp->getX() << "," << itcp->getY();
//pnts << "<circle cx=\"" << itcp->getX() << "\" cy=\"" << itcp->getY()+150 << "\" r=\"5\" stroke=\"black\" stroke-width=\"1\" fill=\"blue\"/>"; } std::cerr << std::endl << "<<< PNT"; }
		BezierModel bm(curvePositions,true,tightness);
		bm.getSteppedCurvePoints(itcg->baseStep(),itcg->maxRand(),itcg->maxRepeat(),curvePoints);

		if (borderCompensation) {
			BezierModel bm0(curvePositions0,true,tightness);
			bm0.getSteppedCurvePoints(itcg->baseStep(),0,0,curvePoints0);
		}
//{ std::cerr << std::fixed << std::setprecision(1);
//std::list<DirectPosition>::iterator cpbeg = curvePoints.begin(),cpend = curvePoints.end(),itcp; size_t n = 0;
//for (itcp = cpbeg; (itcp != cpend); itcp++, n++) {
//if ((n % 10) == 0) std::cerr << std::endl << n << ": "; else std::cerr << " "; std::cerr << itcp->getX() << "," << itcp->getY()+150; } std::cerr << std::endl << "<<< STP" << std::endl; }
		bm.decorateCurve(curvePoints,isHole,itcg->scaleHeightMin(),itcg->scaleHeightRandom(),itcg->controlMin(),itcg->controlRandom(),decoratorPoints);

		// Render path

		std::list<DirectPosition>::const_iterator cpbeg = curvePoints.begin(),cpend = curvePoints.end(),itcp;
		std::list<DoubleArr>::const_iterator itdp = decoratorPoints.begin();

		if (options.debug) {
			path.clear();
			path.str("");
		}

		for (itcp = cpbeg; (itcp != cpend); itcp++) {
			if (itcp == cpbeg)
				path << "M";
			else {
				path << " Q" << itdp->getX() << "," << itdp->getY();
//pnts << "<circle cx=\"" << (*itdp)[0] << "\" cy=\"" << (*itdp)[1]+150 << "\" r=\"5\" stroke=\"black\" stroke-width=\"1\" fill=\"yellow\"/>";
				itdp++;
			}

			double x = itcp->getX(),y = itcp->getY();

			if ((!isHole) && (x >= 0) && (x < (axisWidth + 1))) {
				if (y > 0)
					visible = true;
				else
					aboveTop = true;
			}

			path << " " << x << "," << y;
//pnts << "<circle cx=\"" << x << "\" cy=\"" << y/*+150*/ << "\" r=\"3\" stroke=\"black\" stroke-width=\"1\" fill=\"red\"/>";
		}

		texts[itcg->placeHolder()] << "<path class=\"" << itcg->classDef()
								   << "\" id=\"" << "CloudLayers" << nGroups
								   << "\" d=\""
								   << path.str()
								   << "\"/>\n";
//texts[itcg->placeHolder()] << "<path class=\"" << itcg->classDef()
//						   << "\" id=\"" << "CloudLayers" << nGroups
//						   << "\" d=\""
//						   << pnts.str()
//						   << "\"/>\n";
//texts[itcg->placeHolder()] << pnts.str();

		std::string label = cloudGroupCategory.groupLabel();

		if (!label.empty()) {
			// For a hole store it's area as reserved, otherwise render the label to each separate visible part of the cloud
			//
			// x -coord of group's first visible time instant
			//
			const boost::posix_time::ptime & vt = eGrp.front().validTime();
			double minX = std::max(axisManager->xOffset(vt),0.0);

			renderAreaLabels(borderCompensation ? curvePoints0 : curvePoints,holeAreas,areaLabels,"CLOUDLAYERSLABEL",itcg->markerPlaceHolder(),"ZZCloudLayersText",isHole,isHole ? "" : label,itcg->bbCenterMarker(),
							 minX,minLabelPosHeight,labelPosHeightFactor,itcg->sOffset(),itcg->eOffset(),scaledLo,scaledHi,hasHole,
							 options.debug ? itcg->placeHolder() + "FILLAREAS" : "");
		}
	}	// for group

	// Visibility information

	if (!visible) {
		const std::string & classDef = ((cloudGroupCategory.groups().size() > 0) ? cloudGroupCategory.groups().front().classDef() : "");

		if (!aboveTop) {
			// Show the fixed template text "SKC"
			//
			const std::string NOCLOUDLAYERS(CLOUDLAYERS + "NOCLOUDLAYERS");
			texts[NOCLOUDLAYERS] << classDef << "Visible";
		}
		else {
			// Show the fixed template text "Clouds are located in the upper"
			//
			const std::string CLOUDLAYERSUPPER(CLOUDLAYERS + "INTHEUPPER");
			texts[CLOUDLAYERSUPPER] << classDef << "Visible";
		}
	}

	return;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Join vertically overlapping elevations
   */
  // ----------------------------------------------------------------------

  bool joinElevations(ElevGrp & eGrp)
  {
	ElevGrp::iterator egend(eGrp.end()),iteg = eGrp.begin(),piteg;
	double plo = 0.0,phi = 0.0;
	bool joined = false;

	for (piteg = iteg; (iteg != egend); ) {
		if (iteg->isDeleted()) {
			iteg = eGrp.erase(iteg);
			continue;
		}

		const woml::Elevation & e = iteg->Pv()->elevation();
		boost::optional<woml::NumericalSingleValueMeasure> itsBoundedLo = (e.bounded() ? e.lowerLimit() : woml::NumericalSingleValueMeasure());
		const boost::optional<woml::NumericalSingleValueMeasure> & itsLoLimit = (e.bounded() ? itsBoundedLo : e.value());
		boost::optional<woml::NumericalSingleValueMeasure> itsBoundedHi = (e.bounded() ? e.upperLimit() : woml::NumericalSingleValueMeasure());
		const boost::optional<woml::NumericalSingleValueMeasure> & itsHiLimit = (e.bounded() ? itsBoundedHi : e.value());
		double lo = ((!itsLoLimit) ? 0.0 : itsLoLimit->numericValue());
		double hi = ((!itsHiLimit) ? 0.0 : itsHiLimit->numericValue());

		int dh = ((piteg != iteg) ? (piteg->validTime() - iteg->validTime()).hours() : 1);

		if ((dh == 0) && (plo < hi)) {
			// Elevations overlap vertically; store the joined range to the upper and erase the lower elevation
			//
			lo = std::min(plo,lo);
			hi = phi;

			boost::optional<woml::NumericalValueRangeMeasure> joinedRng(woml::NumericalValueRangeMeasure(
							woml::NumericalSingleValueMeasure(lo,"",""),
							woml::NumericalSingleValueMeasure(hi,"","")));
			woml::Elevation jElev(joinedRng);
			boost::optional<woml::Elevation> eJoined(jElev);

			piteg->Pv()->elevation(*eJoined);

			iteg->Pvs()->get()->editableValues().erase(iteg->Pv());
			iteg = eGrp.erase(iteg);

			joined = true;
		}
		else {
			piteg = iteg;
			iteg++;
		}

		plo = lo;
		phi = hi;
	}

	return joined;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Get group of elevations from a time serie.
   * 		Returns group's type.
   */
  // ----------------------------------------------------------------------

  SvgRenderer::ElevationGroupType SvgRenderer::elevationGroup(const std::list<woml::TimeSeriesSlot> & ts,
		  	  	  	  	  	  	  	  	  	  	  	  	  	  boost::posix_time::ptime & bt,boost::posix_time::ptime & et,
		  	  	  	  	  	  	  	  	  	  	  	  	  	  ElevGrp & eGrp,
		  	  	  	  	  	  	  	  	  	  	  	  	  	  bool all,
		  	  	  	  	  	  	  	  	  	  	  	  	  	  bool join,
		  	  	  	  	  	  	  	  	  	  	  	  	  	  bool getHole,
		  	  	  	  	  	  	  	  	  	  	  	  	  	  CategoryValueMeasureGroup * categoryGroup)
  {
	// Collect group of elevations.
	//
	// If 'all' is true, all elevations are included into the group.
	//
	// Otherwise preset group number (set by setGroupNumbers()) and value category
	// (if given; cloud type or magnitude for icing or turbulence) is taken into account.
	//
	// If 'join' is true, vertically overlapping elevations are joined.

	CategoryValueMeasureGroup overlappingGroup;

	const boost::posix_time::time_period & tp = axisManager->timePeriod();
	bt = tp.begin();
	et = tp.end();

	std::list<woml::TimeSeriesSlot>::const_iterator tsbeg = ts.begin();
	std::list<woml::TimeSeriesSlot>::const_iterator tsend = ts.end();
	std::list<woml::TimeSeriesSlot>::const_iterator itts,pitts;

	bool byCategory = (categoryGroup != NULL),groundGrp = false,nonGroundGrp = false,hole = getHole;
	double nonZ = axisManager->nonZeroElevation();

	eGrp.clear();

	for (itts = pitts = tsbeg; (itts != tsend); itts++) {
		// Load the time instants within the document's time range plus one time instant before the starting and after the
		// ending time instant (if available), thus extending the document's time range (by 1 hour) in both directions.
		//
		// Extendind the time range gives some hints how the clouds, icing etc. continue outside the visible time window.
		//
		// All data is rendered, but only the document's original time range is assumed to be visible; only the visible parts of
		// the clouds, icing etc. are used when positioning labels/symbols.

		const boost::posix_time::ptime & vt = itts->validTime();

		if (vt < bt) {
			std::list<woml::TimeSeriesSlot>::const_iterator nitts = itts;
			nitts++;

			if ((nitts == tsend) || (nitts->validTime() != bt) || ((bt - vt).hours() != 1))
				continue;

			// Extending the time range 1h backwards

			bt = vt;
		}
		else {
			if ((!all) && (eGrp.size() > 0)) {
				// Check for missing time instant
				//
				if ((vt - pitts->validTime()).hours() > 1)
					break;
			}

			if ((vt > et) && ((itts == tsbeg) || (pitts->validTime() != et) || ((vt - et).hours() != 1)))
				break;
		}

		// Loop thru the parameter sets and their parameters

		std::list<boost::shared_ptr<woml::GeophysicalParameterValueSet> >::const_iterator pvsend = itts->values().end(),itpvs;

		for (itpvs = itts->values().begin(); (itpvs != pvsend); itpvs++) {
			std::list<woml::GeophysicalParameterValue>::iterator itpv = itpvs->get()->editableValues().begin();
			std::list<woml::GeophysicalParameterValue>::iterator pvend = itpvs->get()->editableValues().end();

			for ( ; (itpv != pvend); itpv++) {
				// Not mixing holes and nonholes
				//
				if (((eGrp.size() > 0) || getHole) && (hole != ((itpv->getFlags() & ElevationGroupItem::b_isHole) ? true : false)))
					continue;

				const woml::Elevation & e = itpv->elevation();
				boost::optional<woml::NumericalSingleValueMeasure> itsBoundedLo = (e.bounded() ? e.lowerLimit() : woml::NumericalSingleValueMeasure());
				const boost::optional<woml::NumericalSingleValueMeasure> & itsLoLimit = (e.bounded() ? itsBoundedLo : e.value());
				double lo = ((!itsLoLimit) ? 0.0 : itsLoLimit->numericValue());

				bool ground = (lo < nonZ);

				if (!all) {
					// Check if group number and value category (cloud type or magnitude for icing or turbulence) matches.
					//
					const woml::CategoryValueMeasure * cvm = dynamic_cast<const woml::CategoryValueMeasure *>(itpv->value());
					CategoryValueMeasureGroup * group = (byCategory ? categoryGroup : &overlappingGroup);

					if (!(group->groupMember((eGrp.size() == 0),cvm))) {
						if ((eGrp.size() == 0) && options.debug)
							debugoutput << "Settings for "
										<< cvm->category()
										<< " not found\n";

						continue;
					}
				}

				pitts = itts;

				if (eGrp.size() == 0) {
					groundGrp = ground;
					hole = ((itpv->getFlags() & ElevationGroupItem::b_isHole) ? true : false);
				}
				else if (!ground)
					nonGroundGrp = true;

				eGrp.push_back(ElevationGroupItem(vt,itpvs,itpv));

				if (byCategory && categoryGroup->standalone())
					// Note: Vertical overlapping is currently not checked for standalone group category; should load
					//		 all elevations for the time instant, join and return the first elevation if required
					//
					return (groundGrp ? t_ground : t_nonground);
			}	// for itpv
		}	// for itpvs

		if (vt > et) {
			// Extending the time range 1h forwards
			//
			et = vt;
			break;
		}
	}	// for itts

	// Join vertically overlapping elevations.
	//
	// Note: If nonground and ground elevations are joined, the group type returned might be logically wrong
	//		 ('mixed' instead of 'ground'); currently this is not an issue.

	if (join)
		joinElevations(eGrp);

	return ((groundGrp && nonGroundGrp) ? t_mixed : (groundGrp ? t_ground : t_nonground));
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Check if the hole (a gap between two vertically located elevations)
   *		is closed or bounded by an elevation on the left side.
   *
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::checkLeftSide(ElevGrp & eGrp,ElevationHole & hole,CategoryValueMeasureGroup * groupCategory)
  {
	std::reverse_iterator<ElevGrp::iterator> egend(eGrp.begin()),liteg(hole.aboveElev);
	boost::posix_time::ptime vt = hole.aboveElev->validTime();

	// Check if the left side of the hole is closed or bounded above.

	for ( ; (liteg != egend); liteg++) {
		int dh = (liteg->validTime() - vt).hours();

		if (dh == -1) {
			if (groupCategory) {
				// Check that elevation belongs to the same category (cloud type or icing/turbulence magnitude) as the hole
				//
				ElevGrp::iterator iteg = liteg.base();
				iteg--;

				if (!checkCategory(groupCategory,hole.aboveElev,iteg))
					continue;
			}

			const woml::Elevation & e = liteg->Pv()->elevation();
			boost::optional<woml::NumericalSingleValueMeasure> itsBoundedLo = (e.bounded() ? e.lowerLimit() : woml::NumericalSingleValueMeasure());
			const boost::optional<woml::NumericalSingleValueMeasure> & itsLoLimit = (e.bounded() ? itsBoundedLo : e.value());
			boost::optional<woml::NumericalSingleValueMeasure> itsBoundedHi = (e.bounded() ? e.upperLimit() : woml::NumericalSingleValueMeasure());
			const boost::optional<woml::NumericalSingleValueMeasure> & itsHiLimit = (e.bounded() ? itsBoundedHi : e.value());

			double lo = ((!itsLoLimit) ? 0.0 : itsLoLimit->numericValue());
			double hi = ((!itsHiLimit) ? 0.0 : itsHiLimit->numericValue());

			if (hi < hole.hi)
				continue;

			if ((lo > 0.0) && (lo < axisManager->nonZeroElevation()))
				lo = 0.0;

			hole.leftAboveBounded = true;
			hole.leftClosed = (lo <= hole.lo);

			return;
		}
		else if (dh != 0)
			return;
	}

	return;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Check if the hole (a gap between two vertically located elevations)
   *		is closed by a (left side) closed hole(s) on the left side.
   *
   */
  // ----------------------------------------------------------------------

  bool SvgRenderer::checkLeftSideHoles(ElevationHoles & holes,ElevationHoles::iterator & iteh,CategoryValueMeasureGroup * groupCategory)
  {
	std::reverse_iterator<ElevationHoles::iterator> ehend(holes.begin()),liteh(iteh);
	boost::posix_time::ptime vt = iteh->aboveElev->validTime();

	// Check if the holes on the left side (if any) are closed.

	bool closed = false;

	for ( ; (liteh != ehend); liteh++) {
		int dh = (liteh->aboveElev->validTime() - vt).hours();

		if (dh == -1) {
			if (groupCategory) {
				// Check that the holes belong to the same category (cloud type or icing/turbulence magnitude)
				//
				if (!checkCategory(groupCategory,iteh->aboveElev,liteh->aboveElev))
					continue;
			}

			if (liteh->hi < iteh->lo)
				continue;
			else if (liteh->lo > iteh->hi)
				return closed;
			else if (!(liteh->leftClosed))
				return false;

			closed = true;
		}
		else if (dh != 0)
			return closed;
	}

	return closed;
  }

  // ----------------------------------------------------------------------
   /*!
    * \brief Check if the hole (a gap between two vertically located elevations)
    *		is closed by an elevation on the right side.
    *
    */
   // ----------------------------------------------------------------------

  void SvgRenderer::checkRightSide(ElevGrp & eGrp,ElevationHoles::iterator & iteh,CategoryValueMeasureGroup * groupCategory)
  {
	// Check if the right side of the hole is closed.

	ElevGrp::iterator egend = eGrp.end(),riteg = iteh->belowElev;
	boost::posix_time::ptime vt = riteg->validTime();

	for (riteg++; (riteg != egend); riteg++) {
		int dh = (riteg->validTime() - vt).hours();

		if (dh == 1) {
			if (groupCategory) {
				// Check that elevation belongs to the same category (cloud type or icing/turbulence magnitude) as the hole
				//
				if (!checkCategory(groupCategory,iteh->aboveElev,riteg))
					continue;
			}

			const woml::Elevation & e = riteg->Pv()->elevation();
			boost::optional<woml::NumericalSingleValueMeasure> itsBoundedLo = (e.bounded() ? e.lowerLimit() : woml::NumericalSingleValueMeasure());
			const boost::optional<woml::NumericalSingleValueMeasure> & itsLoLimit = (e.bounded() ? itsBoundedLo : e.value());
			boost::optional<woml::NumericalSingleValueMeasure> itsBoundedHi = (e.bounded() ? e.upperLimit() : woml::NumericalSingleValueMeasure());
			const boost::optional<woml::NumericalSingleValueMeasure> & itsHiLimit = (e.bounded() ? itsBoundedHi : e.value());

			double lo = ((!itsLoLimit) ? 0.0 : itsLoLimit->numericValue());
			double hi = ((!itsHiLimit) ? 0.0 : itsHiLimit->numericValue());

			if (hi >= iteh->hi) {
				iteh->rightAboveBounded = true;

				if ((lo > 0.0) && (lo < axisManager->nonZeroElevation()))
					lo = 0.0;

				if (lo > iteh->lo)
					continue;

				iteh->rightClosed = true;
			}

			return;
		}
		else if (dh != 0)
			return;
	}

	return;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Check if the hole (a gap between two vertically located elevations)
   *		is closed by a both side closed hole(s) on the right side.
   *
   */
  // ----------------------------------------------------------------------

  bool SvgRenderer::checkRightSideHoles(ElevationHoles & holes,ElevationHoles::reverse_iterator iteh,CategoryValueMeasureGroup * groupCategory)
  {
	ElevationHoles::iterator ehend(holes.end()),riteh = iteh.base();
	boost::posix_time::ptime vt = iteh->aboveElev->validTime();

	// Check if the holes on the right side (if any) are closed.

	bool closed = false;

	for ( ; (riteh != ehend); riteh++) {
		int dh = (riteh->aboveElev->validTime() - vt).hours();

		if (dh == 1) {
			if (groupCategory) {
				// Check that the holes belong to the same category (cloud type or icing/turbulence magnitude)
				//
				if (!checkCategory(groupCategory,iteh->aboveElev,riteh->aboveElev))
					continue;
			}

			if (riteh->lo > iteh->hi)
				continue;
			else if (riteh->hi < iteh->lo)
				return closed;
			else if (!(riteh->leftClosed && riteh->rightClosed))
				return false;

			closed = true;
		}
		else if (dh != 0)
			return closed;
	}

	return closed;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Check if the hole (a gap between two vertically located elevations)
   *		is closed by a both side closed hole(s) (if any) on both sides.
   *
   */
  // ----------------------------------------------------------------------

  bool SvgRenderer::checkBothSideHoles(ElevationHoles & holes,ElevationHoles::iterator & iteh,CategoryValueMeasureGroup * groupCategory)
  {
	std::reverse_iterator<ElevationHoles::iterator> lehend(holes.begin()),liteh(iteh);
	boost::posix_time::ptime vt = iteh->aboveElev->validTime();

	// Check if the holes on the left side (if any) are closed.

	bool closed = true;

	for ( ; ((liteh != lehend) && closed); liteh++) {
		int dh = (liteh->aboveElev->validTime() - vt).hours();

		if (dh == -1) {
			if (groupCategory) {
				// Check that the holes belong to the same category (cloud type or icing/turbulence magnitude)
				//
				if (!checkCategory(groupCategory,iteh->aboveElev,liteh->aboveElev))
					continue;
			}

			if (liteh->hi < iteh->lo)
				continue;
			else if (liteh->lo > iteh->hi)
				break;

			closed = (liteh->leftClosed && liteh->rightClosed);
		}
		else if (dh != 0)
			break;
	}

	// Check if the holes on the right side (if any) are closed.

	ElevationHoles::iterator rehend(holes.end()),riteh = iteh;

	for (riteh++; ((riteh != rehend) && closed); riteh++) {
		int dh = (riteh->aboveElev->validTime() - vt).hours();

		if (dh == 1) {
			if (groupCategory) {
				// Check that the holes belong to the same category (cloud type or icing/turbulence magnitude)
				//
				if (!checkCategory(groupCategory,iteh->aboveElev,riteh->aboveElev))
					continue;
			}

			if (riteh->lo > iteh->hi)
				continue;
			else if (riteh->hi < iteh->lo)
				break;

			closed = (riteh->leftClosed && riteh->rightClosed);
		}
		else if (dh != 0)
			break;
	}

	return closed;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Check which of the possible holes (gaps between two vertically
   *		located elevations) are closed.
   *
   */
  // ----------------------------------------------------------------------

  bool SvgRenderer::checkHoles(ElevGrp & eGrp,ElevationHoles & holes,CategoryValueMeasureGroup * groupCategory,bool setNegative)
  {
	// Check if the holes are closed by left side closed holes on the left side or by elevations on the right side.

	ElevationHoles::iterator iteh = holes.begin(),ehend = holes.end();

	for ( ; (iteh != ehend); iteh++) {
		if ((!(iteh->leftClosed)) && iteh->leftAboveBounded && checkLeftSideHoles(holes,iteh,groupCategory))
			iteh->leftClosed = true;

		if (iteh->leftClosed)
			checkRightSide(eGrp,iteh,groupCategory);
	}

	// Check if the the holes are closed by both side closed holes on the right side.

	ElevationHoles::reverse_iterator riteh(holes.end()),rehend(holes.begin());

	for ( ; (riteh != rehend); riteh++)
		if (riteh->leftClosed && (!(riteh->rightClosed)) && riteh->rightAboveBounded && checkRightSideHoles(holes,riteh,groupCategory))
			riteh->rightClosed = true;

	// Check if all overlapping holes have both sides closed. Repeat until no "leaks" are found (all overlapping holes
	// are set to open if needed), rolling the status changes (set to open) to neighbour holes one hole at a time.

	bool reScan,hasHole = false;

	do {
		reScan = false;

		for (iteh = holes.begin(); (iteh != ehend); iteh++)
			if ((iteh->leftClosed != iteh->rightClosed) || (iteh->leftClosed && (!checkBothSideHoles(holes,iteh,groupCategory)))) {
				iteh->leftClosed = iteh->rightClosed = false;
				reScan = true;
			}
	}
	while (reScan);

	// Combine the bounding elevations to the 'above' and store the hole to the 'below' elevations.
	// Delete generated 'below' elevations for open holes.

	ElevGrp::iterator aboveElev = eGrp.end(),holeElev = eGrp.end();
	ElevationHoles::iterator piteh = holes.end();

	for (iteh = holes.begin(); (iteh != ehend); iteh++) {
		if (!(iteh->leftClosed && iteh->rightClosed)) {
			if (iteh->belowElev->isHole())
				iteh->belowElev->Pvs()->get()->editableValues().erase(iteh->belowElev->Pv());

			continue;
		}

		// Combine

		if (iteh->aboveElev == holeElev) {
			// Multiple holes, extending the topmost elevation to cover this hole too
			//
			iteh->aboveElev = aboveElev;
		}
		else {
			aboveElev = iteh->aboveElev;
			piteh = iteh;

			// Keep track of elevation's tallest part for labeling

			boost::optional<woml::NumericalValueRangeMeasure> labelRng(woml::NumericalValueRangeMeasure(
							woml::NumericalSingleValueMeasure(axisManager->scaledElevation(iteh->hi),"",""),
							woml::NumericalSingleValueMeasure(axisManager->scaledElevation(iteh->hiHi),"","")));
			woml::Elevation lElev(labelRng);
			boost::optional<woml::Elevation> eLabel(lElev);

			iteh->aboveElev->Pv()->labelElevation(*eLabel);
		}

		boost::optional<woml::NumericalValueRangeMeasure> joinedRng(woml::NumericalValueRangeMeasure(
						woml::NumericalSingleValueMeasure(iteh->loLo,"",""),
						woml::NumericalSingleValueMeasure(piteh->hiHi,"","")));
		woml::Elevation jElev(joinedRng);
		boost::optional<woml::Elevation> eJoined(jElev);

		iteh->aboveElev->Pv()->elevation(*eJoined);
		iteh->aboveElev->hasHole(true);

		// Keep track of elevation's tallest part for labeling

		boost::optional<woml::NumericalValueRangeMeasure> labelRng(woml::NumericalValueRangeMeasure(
						woml::NumericalSingleValueMeasure(axisManager->scaledElevation(iteh->loLo),"",""),
						woml::NumericalSingleValueMeasure(axisManager->scaledElevation(iteh->lo),"","")));
		woml::Elevation lElev(labelRng);
		boost::optional<woml::Elevation> eLabel(lElev);

		iteh->aboveElev->Pv()->labelElevation(*eLabel);

		// Store the hole

		boost::optional<woml::NumericalValueRangeMeasure> holeRng(woml::NumericalValueRangeMeasure(
						woml::NumericalSingleValueMeasure(iteh->lo,"",""),
						woml::NumericalSingleValueMeasure(iteh->hi,"","")));
		woml::Elevation hElev(holeRng);
		boost::optional<woml::Elevation> eHole(hElev);

		(holeElev = iteh->belowElev)->Pv()->elevation(*eHole);
		holeElev->isHole(hasHole = true);

		if (setNegative)
			holeElev->isNegative(true);
	}

	return hasHole;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Generate a new elevation for storing a hole touching the ground
   *		(there is no 'below' elevation to use).
   *
   *	    On return 'iteg' points to the elevation before the new elevation.
   *
   */
  // ----------------------------------------------------------------------

  void generateHoleElevation(ElevGrp & eGrp,ElevGrp::iterator & piteg,ElevGrp::iterator & iteg)
  {
	// Add new object to the underlying woml collection

	const woml::CategoryValueMeasure * cvmA = dynamic_cast<const woml::CategoryValueMeasure *>(piteg->Pv()->value());

	if (!cvmA)
		throw std::runtime_error("generateHoleElevation: CategoryValueMeasure value expected");

	woml::GeophysicalParameter p;
	woml::CategoryValueMeasure * cvm = new woml::CategoryValueMeasure(cvmA->category(),cvmA->codebase());
	boost::optional<woml::NumericalValueRangeMeasure> holeR(woml::NumericalValueRangeMeasure(
					woml::NumericalSingleValueMeasure(0,"",""),
					woml::NumericalSingleValueMeasure(0,"","")));
	woml::Elevation holeE(holeR);
	woml::GeophysicalParameterValue pv(p,cvm,holeE);

	piteg->Pvs()->get()->editableValues().push_back(pv);

	std::list<woml::GeophysicalParameterValue>::iterator itpv = piteg->Pvs()->get()->editableValues().end();
	itpv--;

	// Add new object to the elevation group. Flag it as a hole (already) to enable simple detection when finished
	// searching holes (the woml object will be deleted if the hole is not closed)

	iteg = eGrp.insert(iteg,ElevationGroupItem(piteg->validTime(),piteg->Pvs(),itpv));
	iteg->isHole(true);
	iteg->groupNumber(piteg->groupNumber());

	// Set the item iterator to point to the elevation preceeding the new elevation.

	iteg--;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Check if elevation's category (cloud type or magnitude for
   *		icing or turbulence) is known.
   *
   */
  // ----------------------------------------------------------------------

  bool SvgRenderer::checkCategory(CategoryValueMeasureGroup * groupCategory,ElevGrp::iterator & e) const
  {
	const woml::CategoryValueMeasure * cvm = dynamic_cast<const woml::CategoryValueMeasure *>(e->Pv()->value());

	if (!(groupCategory->groupMember(cvm))) {
		if (options.debug)
			debugoutput << "Settings for "
						<< cvm->category()
						<< " not found\n";

		return false;
	}

	return true;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Check if the elevations belong to the same category (cloud type
   *		or magnitude for icing or turbulence).
   *
   */
  // ----------------------------------------------------------------------

  bool SvgRenderer::checkCategory(CategoryValueMeasureGroup * groupCategory,ElevGrp::iterator & e1,ElevGrp::iterator & e2) const
  {
	const woml::CategoryValueMeasure * cvm1 = dynamic_cast<const woml::CategoryValueMeasure *>(e1->Pv()->value());
	const woml::CategoryValueMeasure * cvm2 = dynamic_cast<const woml::CategoryValueMeasure *>(e2->Pv()->value());

	if (!(groupCategory->groupMember(true,cvm1))) {
		if (options.debug)
			debugoutput << "Settings for "
						<< cvm1->category()
						<< " not found\n";

		return false;
	}

	return groupCategory->groupMember(false,cvm2);
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Set group item scanning status and advance item iterator.
   *
   */
  // ----------------------------------------------------------------------

  void nextItem(ElevGrp::iterator & iteg,bool & scanned,bool & reScan)
  {
	if (scanned)
		iteg->isScanned(true);
	else
		reScan = scanned = true;

	iteg++;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Search elevation holes; areas closed by elevations on the left side,
   * 		below (or by ground), above and on the right side.
   *
   *		The bounding above and below elevations are combined/stored to the 'above'
   *		elevations covering/filling the gaps inbetween and the gaps/holes are
   *		stored to the 'below' elevations. For holes touching ground new ('below')
   *		elevations are generated to store the holes.
   *
   *	    Holes will be rendered separately, additionally based on group's type
   *	    (cloud type or magnitude for icing or turbulence).
   */
  // ----------------------------------------------------------------------

  bool SvgRenderer::searchHoles(const std::list<woml::TimeSeriesSlot> & ts,CategoryValueMeasureGroup * groupCategory,bool setNegative)
  {
	// Get all elevations.
	//
	// Note: 'join' controls whether vertically overlapping elevations are joined or not. It must be passed in as false here
	//		 because some features have additional grouping factor (cloud type for CloudLayers and magnitude for Icing and
	//		 Turbulence), and joining can only be made after each overlapping subgroup of elevations having matching/configured
	//		 type/magnitude have first been extracted.


	ElevGrp eGrp;
	boost::posix_time::ptime bt,et;

	elevationGroup(ts,bt,et,eGrp,true,false);

	// Search for holes.

	ElevGrp::iterator egbeg = eGrp.begin(),egend = eGrp.end();
	ElevGrp::iterator iteg,piteg;

	if (groupCategory) {
		// For categorized data the elevations are scanned one category (cloud type or icing/turbulence magnitude) at a time.
		// Elevations with unknown category are first removed from the underlying woml object collection.
		//
		bool reLoad = false;

		for (iteg = egbeg; (iteg != egend); iteg++)
			if (!checkCategory(groupCategory,iteg)) {
				iteg->Pvs()->get()->editableValues().erase(iteg->Pv());
				iteg->isDeleted(true);

				reLoad = true;
			}

		if (reLoad) {
			elevationGroup(ts,bt,et,eGrp,true,false);

			egbeg = eGrp.begin();
			egend = eGrp.end();
		}
	}

	double nonZ = axisManager->nonZeroElevation(),lo = 0.0,plo = 0.0,hi = 0.0,phi = 0.0;
	bool scanned = true,reScan;

	ElevationHoles holes;

	do {
		for (reScan = false, iteg = egbeg, piteg = egend; (iteg != egend); nextItem(iteg,scanned,reScan)) {
			// No need to check the first and last time instants (holes would be open on the left or right side).
			//
			if (iteg->validTime() == bt)
				continue;
			else if (iteg->validTime() == et)
				break;
			else if (!(iteg->isScanned())) {
				// Get elevation's lo and hi range values

				const woml::Elevation & e = iteg->Pv()->elevation();
				boost::optional<woml::NumericalSingleValueMeasure> itsBoundedLo = (e.bounded() ? e.lowerLimit() : woml::NumericalSingleValueMeasure());
				const boost::optional<woml::NumericalSingleValueMeasure> & itsLoLimit = (e.bounded() ? itsBoundedLo : e.value());
				boost::optional<woml::NumericalSingleValueMeasure> itsBoundedHi = (e.bounded() ? e.upperLimit() : woml::NumericalSingleValueMeasure());
				const boost::optional<woml::NumericalSingleValueMeasure> & itsHiLimit = (e.bounded() ? itsBoundedHi : e.value());

				lo = ((!itsLoLimit) ? 0.0 : itsLoLimit->numericValue());
				hi = ((!itsHiLimit) ? 0.0 : itsHiLimit->numericValue());
			}

			if (piteg != egend) {
				if (((iteg->validTime() - piteg->validTime()).hours() > 0) && (plo > nonZ)) {
					// Generate a new elevation for the previous time instant to store the hole touching ground.
					//
					generateHoleElevation(eGrp,piteg,iteg);
					scanned = false;

					continue;
				}
				else if (iteg->isScanned())
					continue;

				if (iteg->validTime() == piteg->validTime()) {
					// Check first that above and below elevation belong to the same category (cloud type or icing/turbulence magnitude).
					//
					if ((!groupCategory) || checkCategory(groupCategory,piteg,iteg)) {
						// For overlapping elevations only the first (having max hi value) is taken into account.
						//
						if (hi >= plo)
							continue;

						// Check if the hole is closed or bounded by an elevation on the left side. Store the hole.
						//
						ElevationHole hole;

						hole.aboveElev = piteg;
						hole.belowElev = iteg;
						hole.lo = hi;
						hole.loLo = lo;
						hole.hi = plo;
						hole.hiHi = phi;

						checkLeftSide(eGrp,hole,groupCategory);

						holes.push_back(hole);
					}
					else if (groupCategory) {
						// Category changes; rescan
						//
						scanned = false;
						continue;
					}
				}
			}

			if (!(iteg->isScanned())) {
				piteg = iteg;
				plo = lo;
				phi = hi;
			}
		}
	}
	while (reScan);

	// Sort the holes order by validtime asc, hirange desc

	if (holes.size() > 1)
		holes.sort();

	// Check which of the holes are closed (if any). For closed holes the bounding elevations are modified
	// in the underlying woml object collection. For nonclosed holes the generated elevations (if any) are
	// deleted.

	return checkHoles(eGrp,holes,groupCategory,setNegative);
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Contrails rendering
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::render_timeserie(const woml::Contrails & contrails)
  {
	const std::string confPath("Contrails");

	try {
		ElevGrp eGrp;
		double axisWidth = axisManager->axisWidth(),xStep = axisManager->xStep();
		const std::string CONTRAILS(boost::algorithm::to_upper_copy(confPath));
		bool visible = false,hiVisible = false,aboveTop = false;
		int nGroups = 0;

		const char * typeMsg = " must contain a group in curly brackets";

		// Contrail time series

		const std::list<woml::TimeSeriesSlot> & ts = contrails.timeseries();

		// Class

		const libconfig::Setting & specs = config.lookup(confPath);
		if(!specs.isGroup())
			throw std::runtime_error(confPath + typeMsg);

		const std::string classDef = configValue<std::string>(specs,confPath,"class");

		// Set unique group number for members of each overlapping group of elevations.
		// Do not mix ground and nonground elevations.

		setGroupNumbers(ts,false);

		boost::posix_time::ptime bt,et;

		for ( ; ; ) {
			// Get group of overlapping elevations from the time serie
			//
			elevationGroup(ts,bt,et,eGrp,false,true);

			if (eGrp.size() == 0)
				break;

			// Loop thru the elevations connecting first the continuous visible lo range points and then the
			// continuous visible hi range points

			ElevGrp::const_iterator egend = eGrp.end(),iteg,prevIteg;

			for (int rng = 0; (rng < 2); rng++) {
				double lo,hi;
				double & eVal = ((rng == 0) ? lo : hi);
				std::string contrailId = ((rng == 0) ? "ContrailsLo" : "ContrailsHi");

				std::ostringstream path;
				path << std::fixed << std::setprecision(1);

				double prevX = -xStep,prevVal = -1;
				size_t pntCnt = 0;
				bool segmentDrawn = false,prevAbove = false;

				for (iteg = prevIteg = eGrp.begin(); (iteg != egend); ) {
					// Get elevation's lo and hi range values
					//
					const woml::Elevation & e = iteg->Pv()->elevation();
					boost::optional<woml::NumericalSingleValueMeasure> itsBoundedLo = (e.bounded() ? e.lowerLimit() : woml::NumericalSingleValueMeasure());
					const boost::optional<woml::NumericalSingleValueMeasure> & itsLoLimit = (e.bounded() ? itsBoundedLo : e.value());
					boost::optional<woml::NumericalSingleValueMeasure> itsBoundedHi = (e.bounded() ? e.upperLimit() : woml::NumericalSingleValueMeasure());
					const boost::optional<woml::NumericalSingleValueMeasure> & itsHiLimit = (e.bounded() ? itsBoundedHi : e.value());

					lo = ((!itsLoLimit) ? 0.0 : itsLoLimit->numericValue());
					hi = ((!itsHiLimit) ? 0.0 : itsHiLimit->numericValue());

					// scaledElevation() returns negative value and 'above' is set if the scaled value exceeds y -axis height.

					bool above = false;
					double val = axisManager->scaledElevation(eVal,&above);

					// x -coord of this time instant

					double x = axisManager->xOffset(iteg->validTime());

					// Check if the elevation for the previous time instant was missing or invisible

					bool prevMissing = ((val > 0) && (x > (xStep / 2)) && ((x > (prevX + xStep + (xStep / 2))) || (prevVal <= 0)));

					if (((val <= 0) || prevMissing) && (pntCnt > 0)) {
						// Continue ending path segment half timestep forwards.
						//
						path << " L" << (prevX + (xStep / 2)) << "," << prevVal;

						if (above)
							// And then to the top of y -axis at this time instant.
							//
							path << " L" << (prevX + xStep) << ",0";
						else if (rng == 1) {
							// Connect the hi range point to the lo range point.
							//
							const woml::Elevation & e = prevIteg->Pv()->elevation();
							boost::optional<woml::NumericalSingleValueMeasure> itsBoundedLo = (e.bounded() ? e.lowerLimit() : woml::NumericalSingleValueMeasure());
							const boost::optional<woml::NumericalSingleValueMeasure> & itsLoLimit = (e.bounded() ? itsBoundedLo : e.value());

							lo = ((!itsLoLimit) ? 0.0 : itsLoLimit->numericValue());

							path << " L" << (prevX + (xStep / 2)) << "," << axisManager->scaledElevation(lo);
						}
					}

					if (val > 0) {
						// Within the visible y -axis�range
						//
						if (prevMissing) {
							// Start new path segment half timestep backwards.
							//
							path << " M" << (x - (xStep / 2)) << "," << val;

							if (prevAbove)
								// Connect to the top of y -axis at previous time instant.
								//
								path << " L" << prevX << ",0"
									 << " M" << (x - (xStep / 2)) << "," << val;
							else if (rng == 0)
								// Connect the lo range point to the hi range point.
								//
								path << " L" << (x - (xStep / 2)) << "," << axisManager->scaledElevation(hi)
									 << " M" << (x - (xStep / 2)) << "," << val;

							pntCnt++;
						}

						path << ((pntCnt == 0) ? "M" : " L") << x << "," << val;
						pntCnt++;

						// Because the data can now exceed the visible x -axis, checking/setting visibility here by checking the x -coordinate too

						if ((x >= 0) && (x < (axisWidth + 1))) {
							visible = true;

							if (rng == 1)
								hiVisible = true;
						}
					}
					else {
						// Missing or above the top
						//
						if (above && (x >= 0) && (x < (axisWidth + 1)))
							aboveTop = true;

						if (pntCnt > 0)
							segmentDrawn = true;

						pntCnt = 0;
					}

					prevX = x;
					prevVal = val;
					prevAbove = above;

					iteg++;

					if ((iteg == egend) && ((pntCnt > 0) || segmentDrawn)) {
						if ((pntCnt > 0) && (eGrp.back().validTime() < et)) {
							// Continue ending path segment half timestep forwards.
							// Connect the lo range point to the hi range point.
							//
							path << " L" << (x + (xStep / 2)) << "," << val;

							if (rng == 0)
								path << " L" << (x + (xStep / 2)) << "," << axisManager->scaledElevation(hi);
						}

						nGroups++;

						texts[CONTRAILS] << "<path class=\"" << classDef << "Visible"
										 << "\" id=\"" << contrailId << nGroups
										 << "\" d=\""
										 << path.str()
										 << "\"/>\n";
					}
				}	// for iteg
			}	// for rng

			// Remove group's elevations from the collection

			for (iteg = eGrp.begin(); (iteg != eGrp.end()); iteg++)
				iteg->Pvs()->get()->editableValues().erase(iteg->Pv());
		}	// for group

		// Visibility information

		if (!visible) {
			if (!aboveTop) {
				// Show the fixed template text "No condensation trails"
				//
				const std::string NOCONTRAILS(CONTRAILS + "NOCONDENSATIONS");
				texts[NOCONTRAILS] << classDef << "Visible";
			}
			else {
				// Show the fixed template text "Condensation trails are located in the upper"
				//
				const std::string CONTRAILSUPPER(CONTRAILS + "INTHEUPPER");
				texts[CONTRAILSUPPER] << classDef << "Visible";
			}
		}
		else if (!hiVisible) {
			// Show the fixed template text "condensation trails" (the hi trail is not visible)
			//
			const std::string NOCONTRAILS(CONTRAILS + "CONDENSATIONS");
			texts[NOCONTRAILS] << classDef << "Visible";
		}

		// Show the "condensation trails" legend (even if no condesation trails exists or are visible)

		const std::string CONTRAILSVISIBLE(CONTRAILS + "VISIBLE");
		texts[CONTRAILSVISIBLE] << classDef << "Visible";

		return;
	}
	catch(libconfig::ConfigException & ex)
	{
		if(!config.exists(confPath))
			// No settings for confPath
			//
			throw std::runtime_error("Settings for " + confPath + " are missing");

		throw ex;
	}

	if (options.debug)
		debugoutput << "Settings for "
					<< confPath
					<< " not found\n";

	return;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Loads group configuration
   */
  // ----------------------------------------------------------------------

  template <typename T>
  const libconfig::Setting & SvgRenderer::groupConfig(const std::string & groupConfPath,
		  	  	  	  	  	  	  	  	  	  	  	  double & tightness,
	  	  	  	  	  	  	  	  	  	  	  	  	  int & minLabelPosHeight,double & labelPosHeightFactor,
		  	  	  	  	  	  	  	  	  	  	  	  std::list<T> & groups,
		  	  	  	  	  	  	  	  	  	  	  	  std::set<size_t> & memberSet)
  {
	std::string confPath(groupConfPath);

	try {
		const char * grouptypeMsg = " must contain a group in curly brackets";
		const char * listtypeMsg = " must contain a list of groups in parenthesis";

		std::string FEATURE(boost::algorithm::to_upper_copy(confPath));

		// Class

		const libconfig::Setting & specs = config.lookup(confPath);
		if(!specs.isGroup())
			throw std::runtime_error(confPath + grouptypeMsg);

		std::string classDef = configValue<std::string>(specs,confPath,"class",NULL,s_optional);

		// Bezier curve tightness

		bool isSet;

		tightness = (double) configValue<double,int>(specs,confPath,"tightness",NULL,s_optional,&isSet);
		if (!isSet)
			// Using the default by setting a negative value
			//
			tightness = -1.0;

		// Minimum absolute/relative height for the label position

		minLabelPosHeight = configValue<int>(specs,confPath,"minlabelposheight",NULL,s_optional,&isSet);

		if ((!isSet) || (minLabelPosHeight < (int) labelPosHeightMin)) {
			labelPosHeightFactor = configValue<double>(specs,confPath,"labelposheightfactor",NULL,s_optional,&isSet);

			if ((!isSet) || (labelPosHeightFactor < labelPosHeightFactorMin))
				labelPosHeightFactor = defaultLabelPosHeightFactor;

			minLabelPosHeight = 0;
		}

		// If bbcenterlabel is true (default: false), group's symbol is positioned to the center of the area's bounding box;
		// otherwise using 1 or 2 elevations in the middle of the area in x -direction

		bool bbCenterLabel = configValue<bool>(specs,confPath,"bbcenterlabel",NULL,s_optional,&isSet);
		if (!isSet)
			bbCenterLabel = false;

		// Grouping

		confPath += ".groups";

		const libconfig::Setting & groupSpecs = config.lookup(confPath);
		if(!groupSpecs.isList())
			throw std::runtime_error(confPath + listtypeMsg);

		for(int i=0, globalsIdx = -1; i<groupSpecs.getLength(); i++)
		{
			const libconfig::Setting & group = groupSpecs[i];
			if(!group.isGroup())
				throw std::runtime_error(confPath + listtypeMsg);

			// Types (icing or turbulence magnitudes)

			std::string types(boost::algorithm::to_upper_copy(boost::algorithm::trim_copy(configValue<std::string>(group,confPath,"types",NULL,s_optional))));

			if (!types.empty()) {
				std::vector<std::string> it;
				boost::algorithm::split(it,types,boost::is_any_of(","));

				if (it.size() > 1) {
					std::ostringstream its;

					for (unsigned int i = 0,n = 0; (i < it.size()); i++) {
						boost::algorithm::trim(it[i]);

						if (!it[i].empty()) {
							its << (n ? "," : "") << it[i];
							n++;
						}
					}

					types = its.str();
				}
			}

			if (types.empty()) {
				// Global settings have no type(s)
				//
				globalsIdx = i;
				continue;
			}

			// Missing settings from globals when available

			libconfig::Setting * globalScope = ((globalsIdx >= 0) ? &groupSpecs[globalsIdx] : NULL);

			// If symbolonly is true (default: false), single elevation is rendered as symbol (or label) only

			bool symbolOnly = configValue<bool>(group,confPath,"symbolonly",globalScope,s_optional,&isSet);
			if (!isSet)
				symbolOnly = false;

			// Offsets for intermediate curve points on both sides of the elevation's hi/lo range point

			double xStep = axisManager->xStep();

			double xOffset = configValue<double>(group,confPath,"xoffset",globalScope,s_optional,&isSet);
			if (!isSet)
				xOffset = 0.0;
			else
				xOffset *= xStep;

			double yOffset = (double) configValue<int,double>(group,confPath,"yoffset",globalScope,s_optional,&isSet);
			if (!isSet)
				yOffset = 0.0;

			// Relative offset for intermediate curve points (controls how much the ends of the area
			// are extended horizontally)

			double vOffset = configValue<double>(group,confPath,"voffset",globalScope,s_optional,&isSet);
			if (!isSet)
				vOffset = 0.0;
			else
				vOffset *= xStep;

			// Relative top/bottom and side x -offsets for intermediate curve points for single elevation (controls how much the
			// ends of the area are extended horizontally)

			double vSOffset = configValue<double>(group,confPath,"vsoffset",globalScope,s_optional,&isSet);
			if (!isSet)
				vSOffset = xStep / 3;
			else
				vSOffset *= xStep;

			double vSSOffset = configValue<double>(group,confPath,"vssoffset",globalScope,s_optional,&isSet);
			if (!isSet)
				vSSOffset = vSOffset;
			else
				vSSOffset *= xStep;

			// Max offset in px for extending first time instant's elevations to the left

			int sOffset = configValue<int>(group,confPath,"soffset",globalScope,s_optional,&isSet);
			if (!isSet)
				sOffset = 0;

			// Max offset in px for extending last time instant's elevations to the right

			int eOffset = configValue<int>(group,confPath,"eoffset",globalScope,s_optional,&isSet);
			if (!isSet)
				eOffset = 0;

			// Output symbol

			std::string symbol(boost::algorithm::trim_copy(configValue<std::string>(group,confPath,"symbol",NULL,s_optional)));

			// Output label; if symbol is not defined, default label is the types concatenated with ',' as separator.
			// If empty label is given, no label.

			std::string label(boost::algorithm::trim_copy(configValue<std::string>(group,confPath,"label",NULL,s_optional,&isSet)));
			bool hasLabel = isSet;

			// Output placeholders; default label placeholder is the group placeholder + "TEXT"

			std::string placeHolder(boost::algorithm::trim_copy(configValue<std::string>(group,confPath,"output",globalScope,s_optional)));
			if (placeHolder.empty())
				placeHolder = FEATURE;

			std::string labelPlaceHolder(boost::algorithm::trim_copy(configValue<std::string>(group,confPath,"labeloutput",globalScope,s_optional)));
			if (labelPlaceHolder.empty())
				labelPlaceHolder = placeHolder + "TEXT";

			// Standalone types (only one type in the group) have flag controlling whether the group is combined or not

			bool combined = configValue<bool>(group,confPath,"combined",globalScope,s_optional,&isSet);
			if (!isSet)
				combined = true;

			groups.push_back(T(classDef,
							   types,
							   (!symbol.empty()) ? &symbol : NULL,
							   symbolOnly,
							   hasLabel ? &label : NULL,
							   bbCenterLabel,
							   placeHolder,
							   labelPlaceHolder,
							   combined,
							   xOffset,yOffset,
							   vOffset,vSOffset,vSSOffset,
							   sOffset,eOffset,
							   memberSet,
							   &group,
							   globalScope
							));
		}

		return specs;
	}
	catch(libconfig::ConfigException & ex)
	{
		if(!config.exists(confPath))
			// No settings for confPath
			//
			throw std::runtime_error("Settings for " + confPath + " are missing");

		throw ex;
	}
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Timeserie group rendering (for Icing and Turbulence)
   */
  // ----------------------------------------------------------------------

  template <typename GroupCategoryT,typename GroupTypeT>
  void SvgRenderer::render_timeserie(const std::list<woml::TimeSeriesSlot> & ts,
		  	  	  	  	  	  	  	 const std::string & confPath,
		  	  	  	  	  	  	  	 const std::string & classNameExt)
  {
	const std::string FEATURE(boost::algorithm::to_upper_copy(confPath));

	// Get groups from configuration.
	//
	// Member set contains the types (icing or turbulence magnitudes; their starting positions in the group's 'types' setting)
	// in current group; if neither group's symbol nor label is given, label (comma separated list of types) is build using the order
	// the types are defined in 'types' setting.
	//
	// The member set is cleared and then set by all the groups; types contained are added to the set when collecting a group
	// in elevationGroup().

	GroupCategoryT groupCategory;
	std::set<size_t> memberSet;

	double tightness,labelPosHeightFactor;
	int minLabelPosHeight;

	groupConfig<GroupTypeT>(confPath,tightness,minLabelPosHeight,labelPosHeightFactor,groupCategory.groups(),memberSet);

	// x -axis width, y -axis height (px) and symbol class

	double axisWidth = axisManager->axisWidth(),axisHeight = axisManager->axisHeight(),x,y;

	const std::string symClass(boost::algorithm::to_upper_copy(confPath + classNameExt));

	// Elevation group and curve point data

	ElevGrp eGrp;
	bool visible = false,aboveTop = false;
	int nGroups = 0;

	List<DirectPosition> curvePositions;
	std::list<DirectPosition> curvePoints;

	// Set unique group number for members of each overlapping group of elevations

	setGroupNumbers(ts);

	// Search and flag elevation holes

	NFmiFillAreas holeAreas;
	bool hasHole = searchHoles(ts,&groupCategory);

	boost::posix_time::ptime bt,et;

	for ( ; ; ) {
		// Get group of overlapping elevations from the time serie
		//
		// Note: Holes are loaded first; they (fill areas extracted from them) are added to reserved areas
		//		 before loading the icing/turbulence and positioning the symbols/labels
		//
		elevationGroup(ts,bt,et,eGrp,false,true,hasHole,&groupCategory);

		if (eGrp.size() == 0) {
			if (hasHole) {
				hasHole = false;
				continue;
			}
			else
				break;
		}

		nGroups++;

		// Configuration group for current elevation group

		typename std::list<GroupTypeT>::const_iterator itcg = groupCategory.currentGroup();

		// Get curve positions for bezier creation.
		//
		// The scaled lo/hi values for the elevations are returned in scaledLo/scaledHi;
		// they are used to calculate symbol (or label) positions

		std::vector<double> scaledLo,scaledHi;
		std::vector<bool> hasHole;
		bool isHole = eGrp.front().isHole();

		std::ostringstream path;
		path << std::fixed << std::setprecision(3);

		bool single = scaledCurvePositions(eGrp,bt,et,curvePositions,curvePositions,scaledLo,scaledHi,hasHole,
										   itcg->xOffset(),itcg->yOffset(),itcg->vOffset(),itcg->vSOffset(),itcg->vSSOffset(),itcg->sOffset(),itcg->eOffset(),
										   0,0,path);
		bool asSymbol = (single && itcg->symbolOnly());

		if (!asSymbol) {
			// Rendering the group as bezier curve
			//
			if (options.debug)
				texts[FEATURE + "PATH"] << "<path class=\"" << itcg->classDef() << "Visible"
										<< "\" id=\"" << FEATURE << "B" << nGroups
										<< "\" d=\""
										<< path.str()
										<< "\"/>\n";

			// Create bezier curve and get curve points

			BezierModel bm(curvePositions,true,tightness);
			bm.getSteppedCurvePoints(0,0,0,curvePoints);

			// Render path

			std::list<DirectPosition>::iterator cpbeg = curvePoints.begin(),cpend = curvePoints.end(),itcp;

			if (options.debug) {
				path.clear();
				path.str("");
			}

			for (itcp = cpbeg; (itcp != cpend); itcp++) {
				path << ((itcp == cpbeg) ? "M" : " L") << (x = itcp->getX()) << "," << (y = itcp->getY());

				if ((!isHole) && (x >= 0) && (x < (axisWidth + 1))) {
					if ((y > 0) && (y < axisHeight))
						visible = true;
					else if (y < 0)
						aboveTop = true;
				}
			}

			texts[FEATURE] << "<path class=\"" << itcg->classDef() << "Visible"
						   << "\" id=\"" << FEATURE << nGroups
						   << "\" d=\""
						   << path.str()
						   << "\"/>\n";
		}

		// For a hole store it's area as reserved, otherwise render the single symbol or one symbol or label
		// to each separate visible part of the area.
		//
		// x -coord of group's first time instant
		//
		const boost::posix_time::ptime & vt = eGrp.front().validTime();
		double minX = std::max(axisManager->xOffset(vt),0.0);

		const std::string & symbol = groupCategory.groupSymbol();

		if ((!isHole) && (!symbol.empty()))
			renderAreaSymbols(*itcg,curvePoints,holeAreas,confPath,"ZZ" + FEATURE + "Symbol",minX,scaledLo,scaledHi,hasHole,symClass,classNameExt,symbol,asSymbol,visible,aboveTop);
		else
			renderAreaLabels(curvePoints,holeAreas,areaLabels,FEATURE + "LABEL",itcg->markerPlaceHolder(),"ZZ" + FEATURE + "Text",isHole,
							 (symbol.empty() && (!isHole)) ? groupCategory.groupLabel() : "",itcg->bbCenterMarker(),
							 minX,minLabelPosHeight,labelPosHeightFactor,itcg->sOffset(),itcg->eOffset(),scaledLo,scaledHi,hasHole,
							 options.debug ? itcg->placeHolder() + "FILLAREAS" : "");
	}	// for group

	// Visibility information. Class is taken from the first group.

	const std::string & classDef = ((groupCategory.groups().begin() != groupCategory.groups().end())
								   ? groupCategory.groups().front().classDef()
								   : "");

	if (!visible) {
		if (!aboveTop) {
			// Show the fixed template text "No xxx"
			//
			const std::string NOFEATURE(FEATURE + "NO" + FEATURE);
			texts[NOFEATURE] << classDef << "Visible";
		}
		else {
			// Show the fixed template text "xxx is located in the upper"
			//
			const std::string FEATUREUPPER(FEATURE + "INTHEUPPER");
			texts[FEATUREUPPER] << classDef << "Visible";
		}
	}

	// Show the "xxx" legend (even if no data exists or is visible)

	const std::string FEATUREVISIBLE(FEATURE + "VISIBLE");
	texts[FEATUREVISIBLE] << classDef << "Visible";
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Icing rendering
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::render_timeserie(const woml::Icing & icing)
  {
	render_timeserie<IcingGroupCategory,IcingGroup>(icing.timeseries(),"Icing",icing.classNameExt());
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Turbulence rendering
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::render_timeserie(const woml::Turbulence & turbulence)
  {
	render_timeserie<TurbulenceGroupCategory,TurbulenceGroup>(turbulence.timeseries(),"Turbulence",turbulence.classNameExt());
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Migratory birds rendering
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::render_timeserie(const woml::MigratoryBirds & migratorybirds)
  {
	render_aerodromeSymbols<woml::MigratoryBirds>(migratorybirds,"MigratoryBirds",true);
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Surface visibility rendering
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::render_timeserie(const woml::SurfaceVisibility & surfacevisibility)
  {
  	render_aerodromeSymbols<woml::SurfaceVisibility>(surfacevisibility,"SurfaceVisibility");
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Surface weather rendering
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::render_timeserie(const woml::SurfaceWeather & surfaceweather)
  {
	render_aerodromeSymbols<woml::SurfaceWeather>(surfaceweather,"SurfaceWeather");
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Convert wind direction to radians.
   */
  // ----------------------------------------------------------------------

  double windDirectionToRadians(double windDirection)
  {
	if (windDirection < 0.0)
		windDirection = fabs(windDirection);

	if (windDirection > 360.0) {
		double n;
		windDirection = (modf(windDirection / 360.0,&n) * windDirection);
	}

	// Wind direction 0 is from north, counting clockwise.

	double wd;

	if (windDirection <= 90.0)
		wd = 90.0 - windDirection;
	else if (windDirection <= 180.0)
		wd = 360.0 - (windDirection - 90);
	else if (windDirection <= 270.0)
		wd = 270.0 - (windDirection - 180.0);
	else
		wd = 180.0 - (windDirection - 270.0);

	return (wd / (180.0 / 3.141592653589793));
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Calculate wind symbol/arrows positions offset to position
   *		the arrowhead to the elevation line.
   */
  // ----------------------------------------------------------------------

  WindArrowOffsets calculateWindArrowOffsets(unsigned int arrowArmLength,double windDirection)
  {
	// Wind direction 0 is from north, counting clockwise.

	WindArrowOffsets windArrowOffsets;

	if (arrowArmLength == 0)
		return windArrowOffsets;

	if ((fabs(windDirection - 90.0) > 0.001) || (fabs(windDirection - 270.0) > 0.001))
		windArrowOffsets.verticalOffsetPx = (int) floor(((arrowArmLength / 2.0) * sin(windDirectionToRadians(windDirection))) + 0.5);

	if ((fabs(windDirection) > 0.001) || (fabs(windDirection - 180.0) > 0.001))
		windArrowOffsets.horizontalOffsetPx = (int) floor(((arrowArmLength / 2.0) * cos(windDirectionToRadians(windDirection))) + 0.5);

	return windArrowOffsets;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Wind rendering
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::render_timeserie(const woml::Winds & winds)
  {
	const std::string confPath("Wind");

	try {
		const char * typeMsg = " must contain a group in curly brackets";
		const char * valtypeMsg = "render_winds: FlowDirectionMeasure values expected";
		const char * directionMsg = "render_winds: wind speed: '";

		std::string WINDBASEOUT("WINDBASE");
		std::string WINDBASEHOUR("WINDBASEHOUR");
		std::string WINDEXTRAOUT("WINDEXTRA");

		const libconfig::Setting & specs = config.lookup(confPath);
		if(!specs.isGroup())
			throw std::runtime_error(confPath + typeMsg);

		// Symbol background

		std::string href = configValue<std::string>(specs,confPath,"href",NULL,s_optional);

		// Controls to render all winds or only at elevation line heights (default: true; render all)
		// and if the wind at the highest elevation line height is rendered or not (default: true)

		const std::list<Elevation> & elevations = axisManager->elevations();
		std::list<Elevation>::const_iterator elevend = elevations.end();
		double axisHeight = elevations.back().elevation();

		bool isSet;

		bool renderAll = configValue<bool>(specs,confPath,"renderall",NULL,s_optional,&isSet);
		if (!isSet)
			renderAll = true;

		bool renderTop = configValue<bool>(specs,confPath,"rendertop",NULL,s_optional,&isSet);
		if (!isSet)
			renderTop = true;

		// Controls to render first wind time instant by using the x -coordinate of document's first time
		// instant (-1-, the default), by using the x -coordinate of document's first time instant if there
		// are winds for one time instant only (-2-) or by using the first wind time instant as is (-3-).
		//
		// usewindbase			true (or not set)
		// 		autowindbase	false (or not set)		-1-
		// 		autowindbase	true					-2-
		//
		// usewindbase			false					-3-

		bool useWindBase = configValue<bool>(specs,confPath,"usewindbase",NULL,s_optional,&isSet);
		if (!isSet)
			useWindBase = true;

		bool autoWindBase = configValue<bool>(specs,confPath,"autowindbase",NULL,s_optional,&isSet);
		if ((!isSet) || (!useWindBase))
			autoWindBase = false;

		std::string WINDAUTO("WINDAUTO");
		std::string WINDBASE(useWindBase ? (autoWindBase ? WINDAUTO : WINDBASEOUT) : WINDEXTRAOUT);
		std::string WINDEXTRA((useWindBase && autoWindBase) ? WINDAUTO : WINDEXTRAOUT);
		std::string WINDXPOS("--WINDXPOS--");

		double firstXPos = 0.0;

		std::list<WindArrowOffsets> windArrowOffsetsList;
		WindArrowOffsets windArrowOffsets;

		// Loop thru the (ascending) time serie

		bool singleTime = true;
		int nSymbols = 0;

		const boost::posix_time::time_period & tp = axisManager->timePeriod();

		std::list<woml::TimeSeriesSlot>::const_iterator tsbeg = winds.timeseries().begin();
		std::list<woml::TimeSeriesSlot>::const_iterator tsend = winds.timeseries().end();
		std::list<woml::TimeSeriesSlot>::const_iterator fstts = tsend,itts;

		for (itts = tsbeg; (itts != tsend); itts++, nSymbols = 0) {
			// Skip time instants outside the document's time period.
			//
			const boost::posix_time::ptime & vt = itts->validTime();

			if ((vt < tp.begin()) || (vt > tp.end()))
			  {
				if (vt < tp.begin())
					continue;
				else
					break;
			  }

			// Loop thru the parameter sets

			std::list<boost::shared_ptr<woml::GeophysicalParameterValueSet> >::const_iterator pvsend = itts->values().end(),itpvs;

			for (itpvs = itts->values().begin(); (itpvs != pvsend); itpvs++) {
				// Loop thru the parameters
				//
				std::list<woml::GeophysicalParameterValue>::const_iterator pvend = itpvs->get()->values().end(),itpv;

				for (itpv = itpvs->get()->values().begin(); (itpv != pvend); itpv++) {
					// Search for matching condition for the wind speed with nearest comparison value.
					//
					// Take missing config values from the main block

					const woml::FlowDirectionMeasure * fdm = dynamic_cast<const woml::FlowDirectionMeasure *>(itpv->value());

					if (!fdm)
						throw std::runtime_error(valtypeMsg);

					// DirectionVector: space separated wind speed and direction

					double ws,wd;
					std::vector<std::string> cols;
					boost::split(cols,fdm->directionVector(),boost::is_any_of(" "));

					if (cols.size() != 2)
						throw std::runtime_error(directionMsg);
					else try {
						ws = boost::lexical_cast<double>(cols[0]);
						wd = boost::lexical_cast<double>(cols[1]);
					}
					catch(std::exception & ex) {
						throw std::runtime_error(std::string(directionMsg) + fdm->directionVector() + "': " + ex.what());
					}

					const libconfig::Setting * condSpecs = matchingCondition(config,confPath,confPath,"",-1,ws);

					if (condSpecs) {
						// Symbol library and symbol's name, class and scale
						//
						std::string uri = configValue<std::string>(*condSpecs,confPath,"url",&specs);
						std::string symbol = configValue<std::string>(*condSpecs,confPath,"symbol",&specs);
						std::string classDef = configValue<std::string>(*condSpecs,confPath,"class",&specs);

						bool isSet;
						double scale = configValue<double,int>(*condSpecs,confPath,"scale",&specs,s_optional,&isSet);
						if (!isSet)
							scale = 1.0;

						// Wind arrow arm length (in pixels) to calculate horizontal and vertical offsets to position the arrowheads to
						// the elevation lines. Without adjustment the wind arrows are (assumed to be) centered to the elevation lines.

						unsigned int arrowArmLength = configValue<unsigned int>(*condSpecs,confPath,"arrowarmlength",&specs,s_optional,&isSet);
						if (!isSet)
							arrowArmLength = 0;

						const woml::Elevation & e = itpv->elevation();
						boost::optional<woml::NumericalSingleValueMeasure> itsBoundedLower = (e.bounded() ? e.lowerLimit() : woml::NumericalSingleValueMeasure());
						const boost::optional<woml::NumericalSingleValueMeasure> & itsLowerLimit = (e.bounded() ? itsBoundedLower : e.value());

						double height = itsLowerLimit->numericValue();

						if (!renderAll) {
							// Check elevation lines if height matches
							//
							std::list<Elevation>::const_iterator eit = elevations.begin();

							for ( ; ((eit != elevend) && (fabs(eit->elevation() - height) > 1.0)); eit++)
								;

							if (eit == elevend)
								continue;
						}

						// Get scaled elevation
						//
						// Note: Winds seem to have heights like 3500.5m, thus sligthly exceeding the configured
						//		 top elevation line 3500m etc; allow some tolerance to prevent scaledElevation
						//		 ignoring the overflowing elevation

						if (fabs(axisHeight - height) <= 1.0)
							height = (renderTop ? (axisHeight - 1.0) : (axisHeight + 1.0));

						double y = axisManager->scaledElevation(height);

						if (y < 0)
							continue;

						// Get position offsets and adjust y -coordinate

						windArrowOffsets = calculateWindArrowOffsets(arrowArmLength,wd);

						y -= ((windArrowOffsets.scale = scale) * windArrowOffsets.verticalOffsetPx);

						if (fstts == tsend)
							fstts = itts;

						if (itts == fstts) {
							// In 'autoWindBase' mode x -coordinates of the first time instant will be set afterwards
							//
							std::string bgXPos("0.0"),windXPos("0.0");

							if ((!useWindBase) || autoWindBase || (arrowArmLength > 0)) {
								firstXPos = (((!useWindBase) || autoWindBase) ? axisManager->xOffset(vt) : 0.0);
								bgXPos = (autoWindBase ? WINDXPOS : boost::lexical_cast<std::string>(firstXPos));
								windXPos = (autoWindBase ? WINDXPOS : boost::lexical_cast<std::string>(firstXPos + (scale * windArrowOffsets.horizontalOffsetPx)));

								if (autoWindBase)
									windArrowOffsetsList.push_back(windArrowOffsets);
							}

							// Backgroud reference
							//
							if ((nSymbols == 0) && (!href.empty()))
								texts[WINDBASE] << "<use transform=\"translate(" << bgXPos << ",0)\" xlink:href=\""
												<< href
												<< "\"/>\n";

							// Wind symbol
							//
							texts[WINDBASE] << "<g transform=\"translate(" << windXPos << ","
											<< std::fixed << std::setprecision(1) << y
											<< ") scale("
											<< std::fixed << std::setprecision(1) << scale
											<< ")\" class=\""
											<< classDef
											<< "\">\n"
											<< "<use transform=\"rotate("
											<< std::fixed << std::setprecision(1) << wd
											<< ")\" xlink:href=\""
											<< svgescape(uri + "#" + symbol)
											<< "\"/>\n</g>\n";

							nSymbols++;
						}
						else {
							double bgXPos = axisManager->xOffset(vt);

							if (bgXPos > 0.0) {
								double windXPos = bgXPos + (scale * windArrowOffsets.horizontalOffsetPx);

								if ((nSymbols == 0) && (!href.empty()))
									texts[WINDEXTRA] << "<use transform=\"translate("
													 << std::fixed << std::setprecision(1) << bgXPos
													 << ",0)\" xlink:href=\""
													 << href
													 << "\"/>\n";

								texts[WINDEXTRA] << "<g transform=\"translate("
												 << std::fixed << std::setprecision(1) << windXPos
												 << ","
												 << std::fixed << std::setprecision(1) << y
												 << ") scale("
												 << std::fixed << std::setprecision(1) << scale
												 << ")\" class=\""
												 << classDef
												 << "\">\n"
												 << "<use transform=\"rotate("
												 << std::fixed << std::setprecision(1) << wd
												 << ")\" xlink:href=\""
												 << svgescape(uri + "#" + symbol)
												 << "\"/>\n</g>\n";

								nSymbols++;

								singleTime = false;
							}
						}
					}
				}  // for parameters
			}  // for parametervalueset
		}  // for timeserie

		if ((fstts != tsend) && useWindBase) {
			if ((!autoWindBase) || singleTime) {
				// Time (hour) of earliest rendered wind values
				//
				std::string windBaseHour = configValue<std::string>(specs,confPath,"windbasehour",NULL,s_optional,&isSet);

				if (!windBaseHour.empty())
					texts[WINDBASEHOUR] << boost::algorithm::replace_first_copy(windBaseHour,"%hour%",toFormattedString(fstts->validTime(),"HH",axisManager->utc()).CharPtr());
			}

			if (autoWindBase) {
				// Set x -coordinates of the background and the symbols for the first time instant
				//
				std::list<WindArrowOffsets>::iterator ofend = windArrowOffsetsList.end(),itoff = windArrowOffsetsList.begin();
				std::string textOut = texts[WINDAUTO].str();

				boost::algorithm::replace_first(textOut,WINDXPOS,boost::lexical_cast<std::string>(singleTime ? 0.0 : firstXPos));

				for ( ; (itoff != ofend); itoff++) {
					double x = ((singleTime ? 0.0 : firstXPos) + (itoff->scale * itoff->horizontalOffsetPx));
					boost::algorithm::replace_first(textOut,WINDXPOS,boost::lexical_cast<std::string>(x));
				}

				texts[singleTime ? WINDBASEOUT : WINDEXTRAOUT] << textOut;
			}
		}

		return;
	}
  	catch(libconfig::ConfigException & ex)
	{
		if(!config.exists(confPath))
			// No settings for confPath
			//
			throw std::runtime_error("Settings for " + confPath + " are missing");

		throw ex;
	}

	if (options.debug)
		debugoutput << "Settings for "
					<< confPath
					<< " not found\n";
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Zero tolerance group preprocessing
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::checkZeroToleranceGroup(ElevGrp & eGrpIn,ElevGrp & eGrpOut,bool mixed)
  {
	// Generate below 0 elevation forcing the curve path to go to the ground if
	// time instant does not have elevation with lo -range <= (or near) 0

	double nonZ = axisManager->nonZeroElevation();

	ElevGrp::reverse_iterator rbegiter(eGrpIn.end()),renditer(eGrpIn.begin()),riteg,priteg;
	bool groundConnected = true;

	for (riteg = priteg = rbegiter; (riteg != renditer); riteg++) {
		if ((riteg == rbegiter) || (riteg->validTime() != priteg->validTime())) {
			const woml::Elevation & e = riteg->elevation();
			boost::optional<woml::NumericalSingleValueMeasure> itsBoundedLo = (e.bounded() ? e.lowerLimit() : woml::NumericalSingleValueMeasure());
			const boost::optional<woml::NumericalSingleValueMeasure> & itsLoLimit = (e.bounded() ? itsBoundedLo : e.value());

			double lo = ((!itsLoLimit) ? 0.0 : itsLoLimit->numericValue());

			groundConnected = (lo < nonZ);
		}

		// If ground connection was detected before, the condition will remain;
		// the call below won't clear it even if called with false

		riteg->isGroundConnected(groundConnected);

		priteg = riteg;
	}

	eGrpOut.clear();

	ElevGrp::iterator egbeg = eGrpIn.begin(),egend = eGrpIn.end(),iteg,piteg;
	double lo = 0.0,plo = 0.0,hi = 0.0,phi = 0.0;

	for (iteg = piteg = egbeg; ; iteg++) {
		if (iteg != egend) {
			// Get elevation's lo and hi range values
			//
			const woml::Elevation & e = iteg->Pv()->elevation();
			boost::optional<woml::NumericalSingleValueMeasure> itsBoundedLo = (e.bounded() ? e.lowerLimit() : woml::NumericalSingleValueMeasure());
			const boost::optional<woml::NumericalSingleValueMeasure> & itsLoLimit = (e.bounded() ? itsBoundedLo : e.value());
			boost::optional<woml::NumericalSingleValueMeasure> itsBoundedHi = (e.bounded() ? e.upperLimit() : woml::NumericalSingleValueMeasure());
			const boost::optional<woml::NumericalSingleValueMeasure> & itsHiLimit = (e.bounded() ? itsBoundedHi : e.value());

			lo = ((!itsLoLimit) ? 0.0 : itsLoLimit->numericValue());
			hi = ((!itsHiLimit) ? 0.0 : itsHiLimit->numericValue());
		}

		if (iteg != egbeg) {
			if ((iteg != egend) && iteg->validTime() == piteg->validTime())
				;
			else if ((plo >= nonZ) && (!(piteg->isGroundConnected()))) {
			  // Generate below 0 elevation forcing the curve path to go to the ground
			  //
			  eGrpOut.push_back(*piteg);

			  boost::optional<woml::NumericalValueRangeMeasure> gR(woml::NumericalValueRangeMeasure(
							  woml::NumericalSingleValueMeasure(-1200,"",""),
							  woml::NumericalSingleValueMeasure(-1100,"","")));

			  woml::Elevation gE(gR);
			  boost::optional<woml::Elevation> eG(gE);

			  eGrpOut.back().elevation(eG);
			  eGrpOut.back().isGenerated(true);
			}
		  }

		if (iteg == egend)
			break;

		eGrpOut.push_back(*iteg);

		piteg = iteg;
		plo = lo;
		phi = hi;
	}
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Get smallest group number from overlapping left side elevations.
   *
   */
  // ----------------------------------------------------------------------

  unsigned int SvgRenderer::getLeftSideGroupNumber(ElevGrp & eGrp,ElevGrp::iterator & iteg,unsigned int groupNumber,bool mixed)
  {
	std::reverse_iterator<ElevGrp::iterator> egend(eGrp.begin()),liteg(iteg);
	boost::posix_time::ptime vt = iteg->validTime();

	const woml::Elevation & e = iteg->Pv()->elevation();
	boost::optional<woml::NumericalSingleValueMeasure> itsBoundedLo = (e.bounded() ? e.lowerLimit() : woml::NumericalSingleValueMeasure());
	const boost::optional<woml::NumericalSingleValueMeasure> & itsLoLimit = (e.bounded() ? itsBoundedLo : e.value());
	boost::optional<woml::NumericalSingleValueMeasure> itsBoundedHi = (e.bounded() ? e.upperLimit() : woml::NumericalSingleValueMeasure());
	const boost::optional<woml::NumericalSingleValueMeasure> & itsHiLimit = (e.bounded() ? itsBoundedHi : e.value());
	double lo = ((!itsLoLimit) ? 0.0 : itsLoLimit->numericValue());
	double hi = ((!itsHiLimit) ? 0.0 : itsHiLimit->numericValue());
	double nonZ = axisManager->nonZeroElevation();

	for ( ; (liteg != egend); liteg++) {
		int dh = (liteg->validTime() - vt).hours();

		if (dh == -1) {
			const woml::Elevation & e = liteg->Pv()->elevation();
			boost::optional<woml::NumericalSingleValueMeasure> itsBoundedLo = (e.bounded() ? e.lowerLimit() : woml::NumericalSingleValueMeasure());
			const boost::optional<woml::NumericalSingleValueMeasure> & itsLoLimit = (e.bounded() ? itsBoundedLo : e.value());
			boost::optional<woml::NumericalSingleValueMeasure> itsBoundedHi = (e.bounded() ? e.upperLimit() : woml::NumericalSingleValueMeasure());
			const boost::optional<woml::NumericalSingleValueMeasure> & itsHiLimit = (e.bounded() ? itsBoundedHi : e.value());

			double llo = ((!itsLoLimit) ? 0.0 : itsLoLimit->numericValue());
			double lhi = ((!itsHiLimit) ? 0.0 : itsHiLimit->numericValue());

			// 'mixed' controls whether or not ground and nonground elevations are included into same group.

			if ((lhi < lo) || ((!mixed) && ((lo < nonZ) != (llo < nonZ))))
				continue;
			else if (llo > hi)
				break;

			unsigned int leftGroupNumber = liteg->groupNumber();

			if ((leftGroupNumber > 0) && (leftGroupNumber < groupNumber))
				groupNumber = leftGroupNumber;
		}
		else if (dh != 0)
			break;
	}

	return groupNumber;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Get smallest group number from overlapping right side elevations.
   *
   */
  // ----------------------------------------------------------------------

  unsigned int SvgRenderer::getRightSideGroupNumber(ElevGrp & eGrp,ElevGrp::reverse_iterator & itegrev,unsigned int groupNumber,bool mixed)
  {
	ElevGrp::iterator egend = eGrp.end(),riteg = itegrev.base(),iteg = itegrev.base();
	boost::posix_time::ptime vt = itegrev->validTime();

	iteg--;

	const woml::Elevation & e = iteg->Pv()->elevation();
	boost::optional<woml::NumericalSingleValueMeasure> itsBoundedLo = (e.bounded() ? e.lowerLimit() : woml::NumericalSingleValueMeasure());
	const boost::optional<woml::NumericalSingleValueMeasure> & itsLoLimit = (e.bounded() ? itsBoundedLo : e.value());
	boost::optional<woml::NumericalSingleValueMeasure> itsBoundedHi = (e.bounded() ? e.upperLimit() : woml::NumericalSingleValueMeasure());
	const boost::optional<woml::NumericalSingleValueMeasure> & itsHiLimit = (e.bounded() ? itsBoundedHi : e.value());
	double lo = ((!itsLoLimit) ? 0.0 : itsLoLimit->numericValue());
	double hi = ((!itsHiLimit) ? 0.0 : itsHiLimit->numericValue());
	double nonZ = axisManager->nonZeroElevation();

	for ( ; (riteg != egend); riteg++) {
		int dh = (riteg->validTime() - vt).hours();

		if (dh == 1) {
			const woml::Elevation & e = riteg->Pv()->elevation();
			boost::optional<woml::NumericalSingleValueMeasure> itsBoundedLo = (e.bounded() ? e.lowerLimit() : woml::NumericalSingleValueMeasure());
			const boost::optional<woml::NumericalSingleValueMeasure> & itsLoLimit = (e.bounded() ? itsBoundedLo : e.value());
			boost::optional<woml::NumericalSingleValueMeasure> itsBoundedHi = (e.bounded() ? e.upperLimit() : woml::NumericalSingleValueMeasure());
			const boost::optional<woml::NumericalSingleValueMeasure> & itsHiLimit = (e.bounded() ? itsBoundedHi : e.value());

			double rlo = ((!itsLoLimit) ? 0.0 : itsLoLimit->numericValue());
			double rhi = ((!itsHiLimit) ? 0.0 : itsHiLimit->numericValue());

			// 'mixed' controls whether or not ground and nonground elevations are included into same group.

			if ((rlo > hi) || ((!mixed) && ((lo < nonZ) != (rlo < nonZ))))
				continue;
			else if (rhi < lo)
				break;

			unsigned int rightGroupNumber = riteg->groupNumber();

			if ((rightGroupNumber > 0) && (rightGroupNumber < groupNumber))
				groupNumber = rightGroupNumber;
		}
		else if (dh != 0)
			break;
	}

	return groupNumber;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Set unique group number for members of each horizontally overlapping group of elevations
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::setGroupNumbers(const std::list<woml::TimeSeriesSlot> & ts,bool mixed)
  {
	// Get all elevations.
	//
	// Note: 'join' controls whether vertically overlapping elevations are joined or not. It must be passed in as false here
	//		 because some features have additional grouping factor (cloud type for CloudLayers and magnitude for Icing and
	//		 Turbulence), and joining can only be made after each overlapping subgroup of elevations having matching/configured
	//		 type/magnitude have first been extracted.

	ElevGrp eGrp;
	boost::posix_time::ptime bt,et;

	elevationGroup(ts,bt,et,eGrp,true,false);

	unsigned int nextGroupNumber = 1;
	bool reScan;

	do
	{
		// For each elevation, set minimum group number from overlapping left and right side elevations (if any).
		// Repeat until no changes were made.

		ElevGrp::iterator egend(eGrp.end()),iteg = eGrp.begin();
		unsigned int groupNumber,currentGroupNumber;

		reScan = false;

		// Set smallest group number from overlapping elevations on the left side (if any)

		for ( ; (iteg != egend); iteg++) {
			currentGroupNumber = iteg->groupNumber();

			groupNumber = getLeftSideGroupNumber(eGrp,iteg,((currentGroupNumber == 0) ? nextGroupNumber : currentGroupNumber),mixed);

			if ((currentGroupNumber == 0) || (groupNumber < currentGroupNumber)) {
				iteg->groupNumber(groupNumber);
				reScan = true;

				if (groupNumber == nextGroupNumber)
					nextGroupNumber++;
			}
		}

		// Get smallest group number from overlapping elevations on the right side (if any)

		ElevGrp::reverse_iterator regend(eGrp.begin()),riteg(eGrp.end());

		for (riteg++; (riteg != regend); riteg++) {
			groupNumber = getRightSideGroupNumber(eGrp,riteg,currentGroupNumber = riteg->groupNumber(),mixed);

			if (groupNumber < currentGroupNumber) {
				riteg->groupNumber(groupNumber);
				reScan = true;
			}
		}
    }
	while (reScan);
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Zero tolerance rendering
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::render_timeserie(const woml::ZeroTolerance & zerotolerance)
  {
	const std::string confPath("ZeroTolerance");

	try {
		const char * typeMsg = " must contain a group in curly brackets";

		std::string ZEROTOLERANCE(boost::algorithm::to_upper_copy(confPath));
		std::string ZEROTOLERANCEVISIBLE(ZEROTOLERANCE + "VISIBLE");

		// Classes

		const libconfig::Setting & specs = config.lookup(confPath);
		if(!specs.isGroup())
			throw std::runtime_error(confPath + typeMsg);

		std::string classDef = configValue<std::string>(specs,confPath,"class");
		std::string textClassDef = classDef + "Text";

		// Labels for positive (default: "+deg") and negative (default: "-deg") temperature areas.
		// If empty label is given, no label.
		//
		// If bbcenterlabel is true (default: false), the label is positioned to the center of the area's bounding box;
		// otherwise using 1 or 2 elevations in the middle of the area in x -direction

		bool bbCenterLabel = false,isSet;
		std::string posLabel,negLabel,labelPlaceHolder;

		posLabel = boost::algorithm::trim_copy(configValue<std::string>(specs,confPath,"poslabel",NULL,s_optional,&isSet));
		if (!isSet)
			posLabel = "+deg";
		negLabel = boost::algorithm::trim_copy(configValue<std::string>(specs,confPath,"neglabel",NULL,s_optional,&isSet));
		if (!isSet)
			negLabel = "-deg";

		if ((!posLabel.empty()) || (!negLabel.empty())) {
			bbCenterLabel = configValue<bool>(specs,confPath,"bbcenterlabel",NULL,s_optional,&isSet);
			if (!isSet)
				bbCenterLabel = false;

			labelPlaceHolder = boost::algorithm::trim_copy(configValue<std::string>(specs,confPath,"labeloutput",NULL,s_optional));
			if (labelPlaceHolder.empty())
				labelPlaceHolder = ZEROTOLERANCE + "TEXT";
		}

		// Zero tolerance time series and x -axis width/step (px)

		const std::list<woml::TimeSeriesSlot> & ts = zerotolerance.timeseries();

		double axisWidth = axisManager->axisWidth(),xStep = axisManager->xStep();

		// Bezier curve tightness

		double tightness = (double) configValue<double,int>(specs,confPath,"tightness",NULL,s_optional,&isSet);

		if (!isSet)
			// Using the default by setting a negative value
			//
			tightness = -1.0;

		// Offsets for intermediate curve points on both sides of the elevation's hi/lo range point

		double xOffset = configValue<double>(specs,confPath,"xoffset",NULL,s_optional,&isSet);
		if (!isSet)
			xOffset = 0.0;
		else
			xOffset *= xStep;

		double yOffset = (double) configValue<int,double>(specs,confPath,"yoffset",NULL,s_optional,&isSet);
		if ((!isSet) || (yOffset < 0.0))
			yOffset = 0.0;

		// Relative offset for intermediate curve points (controls how much the ends of the area
		// are extended horizontally)

		double vOffset = configValue<double>(specs,confPath,"voffset",NULL,s_optional,&isSet);
		if (!isSet)
			vOffset = 0.0;
		else
			vOffset *= xStep;

		// Relative top/bottom and side x -offsets for intermediate curve points for single elevation (controls how much the
		// ends of the area are extended horizontally)

		double vSOffset = configValue<double>(specs,confPath,"vsoffset",NULL,s_optional,&isSet);
		if (!isSet)
			vSOffset = xStep / 3;
		else
			vSOffset *= xStep;

		double vSSOffset = configValue<double>(specs,confPath,"vssoffset",NULL,s_optional,&isSet);
		if (!isSet)
			vSSOffset = vSOffset;
		else
			vSSOffset *= xStep;

		// Max offset in px for extending first time instant's elevations to the left

		int sOffset = configValue<int>(specs,confPath,"soffset",NULL,s_optional,&isSet);
		if (!isSet)
			sOffset = 0;

		// Max offset in px for extending last time instant's elevations to the right

		int eOffset = configValue<int>(specs,confPath,"eoffset",NULL,s_optional,&isSet);
		if (!isSet)
			eOffset = 0;

		// Minimum absolute/relative height for the label position

		int minLabelPosHeight = configValue<int>(specs,confPath,"minlabelposheight",NULL,s_optional,&isSet);
		double labelPosHeightFactor = 0.0;

		if ((!isSet) || (minLabelPosHeight < (int) labelPosHeightMin)) {
			labelPosHeightFactor = configValue<double>(specs,confPath,"labelposheightfactor",NULL,s_optional,&isSet);

			if ((!isSet) || (labelPosHeightFactor < labelPosHeightFactorMin))
				labelPosHeightFactor = defaultLabelPosHeightFactor;

			minLabelPosHeight = 0;
		}

		// Curve/text visibility

		bool visible = false,setUnvisible = true;

		// Elevation group and curve point data

		ElevGrp eGrpAll,eGrpGen;
		ElevGrp & eGrp = eGrpGen;
		int nGroups = 0;

		List<DirectPosition> curvePositions;
		std::list<DirectPosition> curvePoints;

		// Set unique group number for members of each overlapping group of elevations

		setGroupNumbers(ts);

		// Search and flag elevation holes

		NFmiFillAreas holeAreas;
		bool hasHole = searchHoles(ts);

		boost::posix_time::ptime bt,et;

		for ( ; ; ) {
			// Get group of overlapping elevations from the time serie
			//
			// Note: Holes are loaded first; they (fill areas extracted from them) are added to reserved areas
			//		 before loading the zerotolerance and positioning the labels
			//
			elevationGroup(ts,bt,et,eGrpAll,false,true,hasHole);

			if (eGrpAll.size() == 0) {
				if (hasHole) {
					hasHole = false;
					continue;
				}
				else
					break;
			}

			nGroups++;

			// If a time instant does not have elevation with lo -range <= (or near) 0, below 0 elevation is generated
			// forcing the curve path to go to the ground

			if (!(eGrpAll.front().isHole()))
				checkZeroToleranceGroup(eGrpAll,eGrpGen);
			else
				eGrp = eGrpAll;

			bool isNegative = eGrp.begin()->isNegative();
			std::string & label = (isNegative ? negLabel : posLabel);

			// Get curve positions for overlapping elevations for bezier creation.
			//
			// The scaled lo/hi values for the elevations are returned in scaledLo/scaledHi;
			// they are used to calculate area label positions

			std::vector<double> scaledLo,scaledHi;
			std::vector<bool> hasHole;

			std::ostringstream path;
			path << std::fixed << std::setprecision(3);

			scaledCurvePositions(eGrp,bt,et,curvePositions,curvePositions,scaledLo,scaledHi,hasHole,
								 xOffset,yOffset,vOffset,vSOffset,vSSOffset,sOffset,eOffset,
								 0,0,path,
								 NULL,true,false,true);

			// Create bezier curve and get curve points

			if (options.debug)
				texts[ZEROTOLERANCE + "PATH"] << "<path class=\"" << classDef
											  << "\" id=\"" << "ZeroToleranceB" << nGroups
											  << "\" d=\""
											  << path.str()
											  << "\"/>\n";

//fprintf(stderr,"> Cpos:\n"); {
//List<DirectPosition>::iterator cpbeg = curvePositions.begin(),cpend = curvePositions.end(),itcp;
//for (itcp = cpbeg; (itcp != cpend); itcp++) fprintf(stderr,"> Cpos x=%.1f, y=%.1f\n",itcp->getX(),itcp->getY());
//}
			BezierModel bm(curvePositions,true,tightness);
			bm.getSteppedCurvePoints(10,0,0,curvePoints);

			// Output the path

			path.clear();
			path.str("");
			std::list<DirectPosition>::iterator cpbeg = curvePoints.begin(),cpend = curvePoints.end(),itcp;

//fprintf(stderr,"> Bez:\n");
//for (itcp = cpbeg; (itcp != cpend); itcp++) fprintf(stderr,"> Bez x=%.1f, y=%.1f\n",itcp->getX(),itcp->getY());
			for (itcp = cpbeg; (itcp != cpend); itcp++) {
				// Store the path and check visibility
				//
				double x,y;

				path << ((itcp == cpbeg) ? "M" : " L") << (x = itcp->getX()) << "," << (y = itcp->getY());

				if ((x >= 0) && (x < (axisWidth + 1)) && (y > 0))
					visible = true;
			}

			texts[ZEROTOLERANCE] << "<path class=\"" << classDef
								 << "\" id=\"" << "ZeroTolerance" << nGroups
								 << "\" d=\""
								 << path.str()
								 << "\"/>\n";

			if (visible && setUnvisible) {
				// The curve is visible (elevation(s) above zero did exist); hide the fixed template text "Zerolevel is null"
				//
				texts[ZEROTOLERANCEVISIBLE] << classDef << "Unvisible";
				setUnvisible = false;
			}

			// Render label to each separate visible part of the area
			//
			// Note: Holes (-deg) are labeled too
			//
			if (!label.empty()) {
				// x -coord of group's first time instant
				//
				const boost::posix_time::ptime & vt = eGrp.front().validTime();
				double minX = std::max(axisManager->xOffset(vt),0.0);

				renderAreaLabels(curvePoints,holeAreas,areaLabels,"ZEROTOLERANCELABEL",labelPlaceHolder,"ZZZeroToleranceText",isNegative,label,bbCenterLabel,
								 minX,minLabelPosHeight,labelPosHeightFactor,sOffset,eOffset,scaledLo,scaledHi,hasHole,
								 options.debug ? ZEROTOLERANCE + "FILLAREAS" : "");
			}
		}

		return;
	}
  	catch(libconfig::ConfigException & ex)
	{
		if(!config.exists(confPath))
			// No settings for confPath
			//
			throw std::runtime_error("Settings for " + confPath + " are missing");

		throw ex;
	}

	if (options.debug)
		debugoutput << "Settings for "
					<< confPath
					<< " not found\n";

	return;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Value rendering
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::render_value(const std::string & confPath,
		  	  	  	  	  	  	 std::ostringstream & valOutput,
		  	  	  	  	  	  	 const std::string & valClass,
		  	  	  	  	  	  	 const woml::NumericalSingleValueMeasure * lowerLimit,
		  	  	  	  	  	  	 const woml::NumericalSingleValueMeasure * upperLimit,
		  	  	  	  	  	  	 double lon,
		  	  	  	  	  	  	 double lat,
		  	  	  	  	  	  	 bool asValue)
  {
	// Find the pixel coordinate
	PathProjector proj(area);
	proj(lon,lat);

	++npointvalues;

	try {
		const char * typeMsg = " must contain a list of groups in parenthesis";
		const char * valtypeMsg = ": type must be 'value' or 'svg'";

		const libconfig::Setting & valSpecs = config.lookup(confPath);
		if(!valSpecs.isList())
			throw std::runtime_error(confPath + typeMsg);

		settings s_name((settings) (s_base + 0));

		int valIdx = -1;
		int globalsIdx = -1;

		for(int i=0; i<valSpecs.getLength(); ++i)
		{
			const libconfig::Setting & specs = valSpecs[i];
			if(!specs.isGroup())
				throw std::runtime_error(confPath + typeMsg);

			try {
				if (lookup<std::string>(specs,confPath,"name",s_name) == valClass) {
					valIdx = i;

					// Missing settings from globals when available
					libconfig::Setting * globalScope = ((globalsIdx >= 0) ? &valSpecs[globalsIdx] : NULL);

					// Value type; value (the default) or svg.
					//
					// Note: When rendering (wind symbol with) wind speed asValue is true; block's 'type' setting
					// (for the symbol; svg) is ignored
					std::string type = (asValue ? "value" : configValue<std::string>(specs,valClass,"type",globalScope,s_optional));

					if (type.empty() || (type == "value")) {
						// Class, format and reference for background class
						//
						std::string vtype(upperLimit ? "Range" : "");

						std::string classDef(configValue<std::string>(specs,valClass,"class" + vtype,globalScope,s_optional));
						if (classDef.empty())
							classDef = (vtype.empty() ? valClass : (valClass + " " + valClass + vtype));

						std::string pref(configValue<std::string>(specs,valClass,"pref" + vtype,globalScope,s_optional));
						std::string href(configValue<std::string>(specs,valClass,"href" + vtype,globalScope,s_optional));

						// Output placeholder; by default output to passed stream

						std::string placeHolder(boost::algorithm::trim_copy(configValue<std::string>(specs,valClass,"output",globalScope,s_optional)));

						// Offsets for placing the value (used for wind speed to position the value to the center or to the border
						// of the wind symbol)

						int xoffset = 0,yoffset = 0;
						bool isSet;

						if (asValue) {
							xoffset = static_cast<int>(floor(configValue<float,int>(specs,valClass,"vxoffset",globalScope,s_optional,&isSet)));
							if (!isSet)
								xoffset = 0;

							yoffset = static_cast<int>(floor(configValue<float,int>(specs,valClass,"vyoffset",globalScope,s_optional,&isSet)));
							if (!isSet)
								yoffset = 0;
						}

						// For single value, search for matching condition with nearest comparison value

						const libconfig::Setting * condSpecs = (! upperLimit)
							? matchingCondition(config,confPath,valClass,"",i,lowerLimit->numericValue())
							: NULL;

						if (condSpecs) {
							// Class from the condition or from the parent/parameter block
							//
							classDef = configValue<std::string>(*condSpecs,valClass,"class",&specs);

							std::string cpref = configValue<std::string>(*condSpecs,valClass,"pref" + vtype,NULL,s_optional,&isSet);
							if (isSet)
								pref = cpref;

							std::string chref = configValue<std::string>(*condSpecs,valClass,"href" + vtype,NULL,s_optional,&isSet);
							if (isSet)
								href = chref;

							std::string ph = configValue<std::string>(*condSpecs,valClass,"output",NULL,s_optional,&isSet);
							if (isSet)
								placeHolder = ph;

							if (asValue) {
								int cxoffset = 0,cyoffset = 0;

								cxoffset = static_cast<int>(floor(configValue<float,int>(*condSpecs,valClass,"vxoffset",NULL,s_optional,&isSet)));
								if (isSet)
									xoffset = cxoffset;

								cyoffset = static_cast<int>(floor(configValue<float,int>(*condSpecs,valClass,"vyoffset",NULL,s_optional,&isSet)));
								if (isSet)
									yoffset = cyoffset;
							}
						}

						// Wind speed is not rendered if format is empty (used to supress zero values)

						if (asValue && pref.empty())
							return;

						lon -= xoffset;
						lat += yoffset;

						std::ostringstream & values = (placeHolder.empty() ? valOutput : texts[placeHolder]);

						values << "<g class=\""
							   << classDef
							   << "\">\n";

						if (href.empty())
							values << "<text id=\"text" << boost::lexical_cast<std::string>(npointvalues)
								   << "\" text-anchor=\"middle\" x=\"" << std::fixed << std::setprecision(1) << lon
								   << "\" y=\"" << std::fixed << std::setprecision(1) << lat
								   << "\">" << formattedValue(lowerLimit,upperLimit,confPath,pref) << "</text>\n";
						else
							values << "<g transform=\"translate("
								   << std::fixed << std::setprecision(1) << lon << " "
								   << std::fixed << std::setprecision(1) << lat << ")\">\n"
								   << "<use xlink:href=\"#" << href << "\"/>\n"
								   << "<text id=\"text" << boost::lexical_cast<std::string>(npointvalues)
								   << "\" text-anchor=\"middle\">" << formattedValue(lowerLimit,upperLimit,confPath,pref) << "</text>\n"
								   << "</g>\n";

						values << "</g>\n";

						return;
					}
					else if (type == "svg") {
						throw std::runtime_error(confPath + " type 'svg' not implemented yet");
					}
					else
						throw std::runtime_error(confPath + ": '" + type + "'" + valtypeMsg);
				}
			}
			catch (SettingIdNotFoundException & ex) {
				if (ex.id() == s_name)
					// Global settings have no name
					//
					globalsIdx = i;
			}
		}	// for

		if (options.debug) {
			// No (locale) settings for the value
			//
			const char * p = ((valIdx < 0) ? "Settings for " : "Locale specific settings for " );

			debugoutput << p
					    << valClass
					    << " not found\n";
		}
	}
	catch (libconfig::SettingNotFoundException & ex) {
		// No settings for confPath
		//
		if (options.debug)
			debugoutput << "Settings for "
						<< confPath
						<< " ("
						<< valClass
						<< ") not found\n";
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
						 const boost::shared_ptr<NFmiArea> & theArea,
						 const boost::posix_time::ptime & theValidTime,
						 std::ostringstream * theDebugOutput)
  : options(theOptions)
  , config(theConfig)
  , svgbase(theTemplate)
  , area(theArea)
  , validtime(theValidTime)
  , initAerodrome(true)
  , debugoutput(theDebugOutput ? *theDebugOutput : _debugoutput)
  , ncloudareas(0)
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
 * \brief Render analysis/forecast creation, modification, approval etc.
 * 		  related dates/times
 */
// ----------------------------------------------------------------------

void SvgRenderer::render_header(boost::posix_time::ptime & validTime,
								const boost::posix_time::time_period & timePeriod,
								const boost::posix_time::ptime & forecastTime,
								const boost::posix_time::ptime & creationTime,
								const boost::optional<boost::posix_time::ptime> & modificationTime,
								const std::string & regionName,
								const std::string & regionId,
								const std::string & creator
							   )
{
  // Target region name and id
  render_header("regionName",boost::posix_time::ptime(),false,regionName);
  render_header("regionId",boost::posix_time::ptime(),false,regionId);

  // Creator
  render_header("creator",boost::posix_time::ptime(),false,creator);

  // Feature validtime
  render_header("validTime",validTime);

  // Analysis/forecast time period date, start time and end time
  render_header("timePeriodDate",timePeriod.begin());
  render_header("timePeriodStartTime",timePeriod.begin());
  render_header("timePeriodEndTime",timePeriod.end());

  // Creation time date and time
  render_header("creationTimeDate",creationTime);
  render_header("creationTimeTime",creationTime,creationTime.date() != timePeriod.begin().date());

  // Analysis/forecast date and time
  render_header("forecastTimeDate",forecastTime);
  render_header("forecastTimeTime",forecastTime,forecastTime.date() != timePeriod.begin().date());

  // Modification date and time
  bool useDate = (modificationTime && ((*modificationTime).date() != timePeriod.begin().date()));

  render_header("modificationTimeDate",modificationTime ? *modificationTime : boost::posix_time::ptime());
  render_header("modificationTimeTime",modificationTime ? *modificationTime : boost::posix_time::ptime(),useDate);

  // Approval date and time
//  render_header("Header","approvalTimeDate");
//  render_header("Header","approvalTimeTime");
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
  int hh = static_cast<int>(h / 2);
  int w = static_cast<int>(std::floor(0.5+area->Width()));
  int hw = static_cast<int>(w / 2);

  std::string width = boost::lexical_cast<std::string>(w);
  std::string hwidth = boost::lexical_cast<std::string>(hw);
  std::string height = boost::lexical_cast<std::string>(h);
  std::string hheight = boost::lexical_cast<std::string>(hh);
  
  using boost::algorithm::replace_all;

  replace_all(ret,"--WIDTH--",width);
  replace_all(ret,"--WIDTH/2--",hwidth);
  replace_all(ret,"--HEIGHT--",height);
  replace_all(ret,"--HEIGHT/2--",hheight);

  replace_all(ret,"--PATHS--",paths.str());
  replace_all(ret,"--MASKS--",masks.str());

  replace_all(ret,"--PRECIPITATIONAREAS--",precipitationareas.str());;
  replace_all(ret,"--CLOUDBORDERS--"      ,cloudareas.str());;
  replace_all(ret,"--OCCLUDEDFRONTS--"    ,occludedfronts.str());;
  replace_all(ret,"--WARMFRONTS--"        ,warmfronts.str());;
  replace_all(ret,"--COLDFRONTS--"        ,coldfronts.str());;
  replace_all(ret,"--TROUGHS--"           ,troughs.str());;
  replace_all(ret,"--UPPERTROUGHS--"      ,uppertroughs.str());;
  replace_all(ret,"--JETS--"              ,jets.str());;
  replace_all(ret,"--POINTNOTES--"        ,pointnotes.str());;
  replace_all(ret,"--POINTSYMBOLS--"      ,pointsymbols.str());;
  replace_all(ret,"--POINTVALUES--"       ,pointvalues.str());;

  if (options.debug) {
	  replace_all(ret,"--DEBUGOUTPUT--","<![CDATA[\n" + debugoutput.str() + "]]>\n");
	  std::cerr << debugoutput.str();
  }

  // BOOST_FOREACH does not work nicely with ptr_map

#ifdef CONTOURING

  for(Contours::const_iterator it=contours.begin(); it!=contours.end(); ++it)
	replace_all(ret,it->first, it->second->str());

#endif

  for(Texts::const_iterator it=texts.begin(); it!=texts.end(); ++it)
	replace_all(ret,"--" + it->first + "--", it->second->str());

  // Clear all remaining placeholders

  boost::regex pl("--[[:upper:]+[\\S]*]*--");

  return boost::regex_replace(ret,pl,"");

}

// ----------------------------------------------------------------------
/*!
 * \brief Elevation
 */
// ----------------------------------------------------------------------

Elevation::Elevation(double theElevation,double theScale,int theScaledElevation,const std::string & theLLabel,const std::string & theRLabel,bool line)
: itsElevation(theElevation)
, itsScale(theScale)
, itsScaledElevation(theScaledElevation)
, itsLLabel(theLLabel)
, itsRLabel(theRLabel)
, itsFactor(0.0)
, renderLine(line)
{ }

Elevation::Elevation(double theElevation)
: itsElevation(theElevation)
, itsScale(0.0)
, itsScaledElevation(0)
, itsLLabel("")
, itsRLabel("")
, itsFactor(0.0)
, renderLine(true)
{ }

bool Elevation::operator < (const Elevation & theOther) const
{
	return itsElevation < theOther.elevation();
}

// ----------------------------------------------------------------------
/*!
 * \brief AxisManager
 */
// ----------------------------------------------------------------------

AxisManager::AxisManager(const libconfig::Config & config)
  : itsAxisHeight(0)
  , itsMinElevation(0.0)
  , itsMaxElevation(0.0)
  , itsAxisWidth(0)
  , itsUtc(false)
  , itsTimePeriod(boost::posix_time::ptime(boost::posix_time::not_a_date_time),boost::posix_time::ptime(boost::posix_time::not_a_date_time))
  , itsElevations()
{
	std::string confPath("ElevationAxis");

	try {
		const char * groupMsg = " must contain a group in curly brackets";
		const char * listMsg = " must contain a list of groups in parenthesis";
		const std::string valueMsg(": value between 0.0 and 1.0 expected");
		const char * scaleMsg = ": scale must be rising for rising elevation";

		// Y -axis height (px)

		const libconfig::Setting & elevAxisSpecs = config.lookup(confPath);
		if(!elevAxisSpecs.isGroup())
			throw std::runtime_error(confPath + groupMsg);

		itsAxisHeight = configValue<int>(elevAxisSpecs,confPath,"height");

		// Y -axis labels and their elevation (meters) and relative position [0,1] on y -axis.
		// The listed elevations are used for y -coord linear interpolation when rendering

		confPath += ".elevations";

		const libconfig::Setting & elevSpecs = config.lookup(confPath);
		if((!elevSpecs.isList()) || (elevSpecs.getLength() < 1))
			throw std::runtime_error(confPath + listMsg);

		for(int i=0; i<elevSpecs.getLength(); ++i)
		{
			const libconfig::Setting & specs = elevSpecs[i];
			if(!specs.isGroup())
				throw std::runtime_error(confPath + listMsg);

			double elevation = configValue<double,int>(specs,confPath,"elevation");					// Meters
			double scale = configValue<double,int>(specs,confPath,"scale");							// [0,1]
			std::string lLabel = configValue<std::string>(specs,confPath,"llabel",NULL,s_optional);	// Left side label
			std::string rLabel = configValue<std::string>(specs,confPath,"rlabel",NULL,s_optional);	// Right side label

			// If 'line' is set (the default), render the elevation line

			bool isSet;
			bool line = configValue<bool>(specs,confPath,"line",NULL,s_optional,&isSet);
			if (!isSet)
				line = true;

			if ((scale < 0.0) || (scale > 1.0))
				throw std::runtime_error(confPath + ": " + boost::lexical_cast<std::string>(boost::format("%.3f") % scale) + valueMsg);

			boost::algorithm::trim(lLabel);
			boost::algorithm::trim(rLabel);

			if (i) {
				if (elevation < itsMinElevation)
					itsMinElevation = elevation;
				else if (elevation > itsMaxElevation)
					itsMaxElevation = elevation;
			}
			else
				itsMinElevation = itsMaxElevation = elevation;

			itsElevations.push_back(Elevation(elevation,scale,(int) (itsAxisHeight * scale),lLabel,rLabel,line));
		}

		// Sort the elevations and add elevation 0.0 if it does not exist

		itsElevations.sort();

		std::list<Elevation>::iterator it = itsElevations.begin();
		if (it->elevation() > 0.0) {
			itsMinElevation = 0.0;
			itsElevations.push_front(Elevation(itsMinElevation));
		}

		// Check the elevations have rising scale and precalculate factors for linear interapolation

		std::list<Elevation>::iterator it2 = it = itsElevations.begin();

		for (it++; (it != itsElevations.end()); it++, it2++)
			if (it2->scale() >= it->scale())
				throw std::runtime_error(confPath + ": " + boost::lexical_cast<std::string>(boost::format("%.3f") % it->scale()) + scaleMsg);
			else
				it2->factor((it->scale() - it2->scale()) / (it->elevation() - it2->elevation()));
  	}
  	catch(libconfig::ConfigException & ex)
  	{
		if(!config.exists(confPath))
	  		// No settings for confPath
	  		//
		    throw std::runtime_error("Settings for " + confPath + " are missing");

	    throw ex;
  	}
}

// ----------------------------------------------------------------------
/*!
 * \brief Elevation value scaling
 */
// ----------------------------------------------------------------------

double AxisManager::scaledElevation(double elevation,bool * above,double belowZero,double aboveTop)
{
	// Find lo/hi limits for linear interpolation
	//
	// svg y -axis grows downwards; use axis height to transfrom the y -coordinates

	if (elevation <= itsMinElevation)
		// Go below 0 (invisible) to avoid drawing along the X -axis
		//
		return itsAxisHeight + belowZero;
	else if (elevation >= itsMaxElevation) {
		// Go above the top (invisible) but not too far to prevent a single cloud spreading too much
		// (bezier curve construction feature when control curve 'bbox' height/width relation grows)
		//
		if (above)
			*above = true;

		return -aboveTop;
	}

	std::list<Elevation>::iterator ith = lower_bound(itsElevations.begin(),itsElevations.end(),Elevation(elevation));
	if (ith == itsElevations.end())
		// Never
		return 0;

	if (ith->elevation() > elevation) {
		std::list<Elevation>::iterator itl = ith;
		itl--;

		return itsAxisHeight - (itsAxisHeight * (itl->scale() + ((elevation - itl->elevation()) * itl->factor())));
	}

	return itsAxisHeight - (itsAxisHeight * ith->scale());
}

// ----------------------------------------------------------------------
/*!
 * \brief X -axis offset
 */
// ----------------------------------------------------------------------

double AxisManager::xOffset(const boost::posix_time::ptime & validTime) const
{
	if (validTime < itsTimePeriod.begin())
		return -xStep();
	else if (validTime > itsTimePeriod.end())
		return itsAxisWidth + xStep();

	return (((double) (validTime - itsTimePeriod.begin()).total_seconds()) / itsTimePeriod.length().total_seconds()) * itsAxisWidth;
}

// ----------------------------------------------------------------------
/*!
 * \brief X -axis step
 */
// ----------------------------------------------------------------------

double AxisManager::xStep() const
{
	int nH = (itsTimePeriod.end() - itsTimePeriod.begin()).hours();

	return ((nH > 0) ? (itsAxisWidth / nH) : 0);
}

// ----------------------------------------------------------------------
/*!
 * \brief ElevationGroupItem
 */
// ----------------------------------------------------------------------

ElevationGroupItem::ElevationGroupItem(boost::posix_time::ptime theValidTime,
									   const std::list<boost::shared_ptr<woml::GeophysicalParameterValueSet> >::const_iterator & thePvs,
									   const std::list<woml::GeophysicalParameterValue>::iterator & thePv)
: itsValidTime(theValidTime)
, itsPvs(thePvs)
, itsPv(thePv)
, itsTopConnected(false)
, itsBottomConnected(false)
, itsLeftOpen(true)
, itsBottomConnection()
, itsGenerated(false)
, itsDeleted(false)
, itsElevation()
{ }

const std::list<woml::GeophysicalParameterValue>::iterator & ElevationGroupItem::Pv() const
{
  if (isDeleted())
	  throw std::runtime_error("Internal error: Pv() called for a deleted elevation");

  return itsPv;
}

void ElevationGroupItem::isScanned(bool scanned)
{
  // We only need to set the bit

  if (scanned)
	Pv()->setFlags(b_scanned);
}

bool ElevationGroupItem::isScanned() const
{
  return ((Pv()->getFlags() & b_scanned) ? true : false);
}

void ElevationGroupItem::isGroundConnected(bool connected)
{
  // We only need to set the bit

  if (connected)
	Pv()->setFlags(b_groundConnected);
}

bool ElevationGroupItem::isGroundConnected() const
{
  return ((Pv()->getFlags() & b_groundConnected) ? true : false);
}

void ElevationGroupItem::isNegative(bool negative)
{
  // We only need to set the bit

  if (negative)
	Pv()->setFlags(b_negative);
}

bool ElevationGroupItem::isNegative() const
{
  return ((Pv()->getFlags() & b_negative) ? true : false);
}

void ElevationGroupItem::hasHole(bool hole)
{
  // We only need to set the bit

  if (hole)
	Pv()->setFlags(b_hasHole);
}

bool ElevationGroupItem::hasHole() const
{
  return ((Pv()->getFlags() & b_hasHole) ? true : false);
}

void ElevationGroupItem::isHole(bool hole)
{
  // We only need to set the bit

  if (hole)
	Pv()->setFlags(b_isHole);
}

bool ElevationGroupItem::isHole() const
{
  return ((Pv()->getFlags() & b_isHole) ? true : false);
}

void ElevationGroupItem::groupNumber(unsigned int group)
{
  woml::CategoryValueMeasure * cvm = dynamic_cast<woml::CategoryValueMeasure *>(Pv()->editableValue());

  if (!cvm)
	throw std::runtime_error("groupNumber: CategoryValueMeasure value expected");

  cvm->groupNumber(group);
}

unsigned int ElevationGroupItem::groupNumber() const
{
  const woml::CategoryValueMeasure * cvm = dynamic_cast<const woml::CategoryValueMeasure *>(Pv()->value());

  if (!cvm)
	throw std::runtime_error("groupNumber: CategoryValueMeasure value expected");

  return cvm->groupNumber();
}

// ----------------------------------------------------------------------
/*!
 * \brief ElevationHole operator <
 *
 */
// ----------------------------------------------------------------------

bool ElevationHole::operator < (const ElevationHole & theOther) const
{
  // Order by validtime asc, hirange desc

  const boost::posix_time::ptime & vt1 = aboveElev->validTime(),& vt2 = theOther.aboveElev->validTime();

  if (vt1 < vt2)
	  return true;
  else if (vt1 > vt2)
	  return false;

  const woml::Elevation & e1 = aboveElev->Pv()->elevation();
  boost::optional<woml::NumericalSingleValueMeasure> itsBoundedHi1 = (e1.bounded() ? e1.upperLimit() : woml::NumericalSingleValueMeasure());
  const boost::optional<woml::NumericalSingleValueMeasure> & itsHiLimit1 = (e1.bounded() ? itsBoundedHi1 : e1.value());
  double hi1 = ((!itsHiLimit1) ? 0.0 : itsHiLimit1->numericValue());

  const woml::Elevation & e2 = theOther.aboveElev->Pv()->elevation();
  boost::optional<woml::NumericalSingleValueMeasure> itsBoundedHi2 = (e2.bounded() ? e2.upperLimit() : woml::NumericalSingleValueMeasure());
  const boost::optional<woml::NumericalSingleValueMeasure> & itsHiLimit2 = (e2.bounded() ? itsBoundedHi2 : e2.value());
  double hi2 = ((!itsHiLimit2) ? 0.0 : itsHiLimit2->numericValue());

  return (hi1 > hi2);
}

// ----------------------------------------------------------------------
/*!
 * \brief ConfigGroup
 */
// ----------------------------------------------------------------------

ConfigGroup::ConfigGroup(const std::string & theClassDef,
						 const std::string & theMemberTypes,
						 const std::string * theLabel,
						 bool bbCenterMarker,
						 const std::string & thePlaceHolder,
						 const std::string & theMarkerPlaceHolder,
						 bool combined,
						 double theXOffset,double theYOffset,
						 double theVOffset,double theVSOffset,double theVSSOffset,
						 int theSOffset,int theEOffset,
						 std::set<size_t> & theMemberSet,
						 const libconfig::Setting * theLocalScope,
						 const libconfig::Setting * theGlobalScope)
  : itsMemberSet(theMemberSet)
{
  itsClass = boost::algorithm::trim_copy(theClassDef);

  if (!itsClass.empty()) {
	// Class names for member type (cloud type or icing or turbulence magnitude) output
	//
	std::vector<std::string> cl;
	boost::algorithm::split(cl,itsClass,boost::is_any_of(" "));

	std::ostringstream cls;

	for (unsigned int i = 0,n = 0; (i < cl.size()); i++) {
		cls << (n ? " " : "") << cl[i] << "Text";
		n++;
	}

	itsTextClass = cls.str();
  }

  itsStandalone = (((theMemberTypes.find(",") == std::string::npos)) && (!combined));
  itsMemberTypes = (itsStandalone ? theMemberTypes : ("," + theMemberTypes + ","));
  itsLabel = theLabel ? *theLabel : "";
  hasLabel = theLabel ? true : false;
  bbCenterMarkerPos = bbCenterMarker;
  itsPlaceHolder = thePlaceHolder;
  itsMarkerPlaceHolder = theMarkerPlaceHolder;

  itsXOffset = theXOffset;
  itsYOffset = std::max(theYOffset,0.0);
  itsVOffset = theVOffset;
  itsVSOffset = theVSOffset;
  itsVSSOffset = theVSSOffset;
  itsSOffset = theSOffset;
  itsEOffset = theEOffset;

  itsLocalScope = theLocalScope;
  itsGlobalScope = theGlobalScope;
}

// ----------------------------------------------------------------------
/*!
 * \brief CloudGroup
 */
// ----------------------------------------------------------------------

CloudGroup::CloudGroup(const std::string & theClass,
					   const std::string & theCloudTypes,
					   const std::string & theSymbolType,
					   const std::string * theLabel,
					   bool bbCenterLabel,
					   const std::string & thePlaceHolder,
					   const std::string & theLabelPlaceHolder,
					   bool combined,
					   unsigned int theBaseStep,
					   unsigned int theMaxRand,
					   unsigned int theMaxRepeat,
					   int theScaleHeightMin,
					   int theScaleHeightRandom,
					   int theControlMin,
					   int theControlRandom,
					   double theXOffset,double theYOffset,
					   double theVOffset,double theVSOffset,double theVSSOffset,
					   int theSOffset,int theEOffset,
					   std::set<size_t> & theCloudSet,
					   const libconfig::Setting * theLocalScope,
					   const libconfig::Setting * theGlobalScope)

  : ConfigGroup(theClass,
		  	    theCloudTypes,
		  	    theLabel,
		  	    bbCenterLabel,
		  	    thePlaceHolder,
		  	    theLabelPlaceHolder,
		  	    combined,
		  	    theXOffset,theYOffset,
		  	    theVOffset,theVSOffset,theVSSOffset,
		  	    theSOffset,theEOffset,
		  	    theCloudSet,
		  	    theLocalScope,
		  	    theGlobalScope
			   )
{
  itsSymbolType = (standalone() ? theSymbolType : "");

  itsBaseStep = theBaseStep;
  itsMaxRand = theMaxRand;
  itsMaxRepeat = theMaxRepeat;

  itsScaleHeightMin = theScaleHeightMin;
  itsScaleHeightRandom = theScaleHeightRandom;
  itsControlMin = theControlMin;
  itsControlRandom = theControlRandom;
}

// ----------------------------------------------------------------------
/*!
 * \brief Check if group contains (only) given member type(s)
 */
// ----------------------------------------------------------------------

bool ConfigGroup::contains(const std::string & theMemberType,bool only) const
{
  if (itsStandalone)
	  return (itsMemberTypes == theMemberType);
  else if (only)
	  return (itsMemberTypes == ("," + theMemberType + ","));
  else
	  return (itsMemberTypes.find("," + theMemberType + ",") != std::string::npos);
}

// ----------------------------------------------------------------------
/*!
 * \brief Add a member type the group contains
 */
// ----------------------------------------------------------------------

void ConfigGroup::addType(const std::string & theMemberType) const
{
  if (!itsStandalone) {
	  size_t pos = itsMemberTypes.find("," + theMemberType + ",");

	  if (pos != std::string::npos)
		  itsMemberSet.insert(pos + 1);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Get the types the group contains
 */
// ----------------------------------------------------------------------

std::string ConfigGroup::memberTypes(bool allTypes) const
{
  if (itsStandalone)
	  return itsMemberTypes;

  std::string types;

  if (!allTypes) {
	  std::set<size_t>::iterator setbeg = itsMemberSet.begin(),setend = itsMemberSet.end(),it;
	  std::ostringstream mts;

	  for (it = setbeg; (it != setend); it++) {
		std::string s(itsMemberTypes.substr(*it,itsMemberTypes.find(',',*it) - *it));
		mts << ((it != setbeg) ? "\t" : "") << s;
	  }

	  types = mts.str();
  }
  else
	  types = itsMemberTypes;

  boost::algorithm::replace_all(types,",","");
  boost::algorithm::replace_all(types,"\t",",");

  return types;
}

// ----------------------------------------------------------------------
/*!
 * \brief Check if group contains given type
 */
// ----------------------------------------------------------------------

bool MemberType::operator ()(const ConfigGroup & configGroup, const std::string & memberType) const {
  return configGroup.contains(memberType);
}

// ----------------------------------------------------------------------
/*!
 * \brief Search for a group containing only the given type(s)
 */
// ----------------------------------------------------------------------

template <typename T>
bool GroupType<T>::operator ()(const ConfigGroup & configGroup, const std::string & memberTypes) const {
  return configGroup.contains(memberTypes,true);
}

// ----------------------------------------------------------------------
/*!
 * \brief IcingGroup
 */
// ----------------------------------------------------------------------

IcingGroup::IcingGroup(const std::string & theClass,
					   const std::string & theIcingTypes,
					   const std::string * theSymbol,
					   bool symbolOnly,
					   const std::string * theLabel,
					   bool bbCenterLabel,
					   const std::string & thePlaceHolder,
					   const std::string & theLabelPlaceHolder,
					   bool combined,
					   double theXOffset,double theYOffset,
					   double theVOffset,double theVSOffset,double theVSSOffset,
					   int theSOffset,int theEOffset,
					   std::set<size_t> & theIcingSet,
					   const libconfig::Setting * theLocalScope,
					   const libconfig::Setting * theGlobalScope)
  : ConfigGroup(theClass,
		  	    theIcingTypes,
		  	    theLabel,
		  	    bbCenterLabel,
		  	    thePlaceHolder,
		  	    theLabelPlaceHolder,
		  	    combined,
		  	    theXOffset,theYOffset,
		  	    theVOffset,theVSOffset,theVSSOffset,
		  	    theSOffset,theEOffset,
		  	    theIcingSet,
		  	    theLocalScope,
		  	    theGlobalScope
			   )
{
  itsSymbol = (theSymbol ? *theSymbol : "");
  renderSymbolOnly = symbolOnly;
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a CloudAreaBorder
 */
// ----------------------------------------------------------------------

void
SvgRenderer::visit(const woml::CloudArea & theFeature)
{
  if(options.debug)	std::cerr << "Visiting CloudArea" << std::endl;

  ++ncloudareas;
  std::string id = "cloudarea" + boost::lexical_cast<std::string>(ncloudareas);

  const woml::CubicSplineSurface surface = theFeature.controlSurface();

  Path path = PathFactory::create(surface);

  PathProjector proj(area);
  path.transform(proj);

  render_surface(path,cloudareas,id,theFeature.cloudTypeName(),&theFeature);
}

// ----------------------------------------------------------------------
/*!
 * \brief Render cloud layers
 */
// ----------------------------------------------------------------------

void SvgRenderer::visit(const woml::CloudLayers & theFeature)
{
  if(options.debug)	std::cerr << "Visiting CloudLayers" << std::endl;

  render_aerodrome<woml::CloudLayers>(theFeature);
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a ColdAdvection
 */
// ----------------------------------------------------------------------

void
SvgRenderer::visit(const woml::ColdAdvection & theFeature)
{
  if(options.debug)	std::cerr << "Visiting ColdAdvection" << std::endl;

  if(!config.exists("coldadvection")) return;

  ++ncoldadvections;
  std::string id = "coldadvection" + boost::lexical_cast<std::string>(ncoldadvections);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = getCssSize("coldadvectionglyph","font-size");
  double spacing = getSetting<double>("coldadvection","letter-spacing",60.0);

  const char * Cc = ((theFeature.orientation() == "-") ? "C" : "c");

  render_front(path,paths,coldfronts,id,
			   "coldadvection","coldadvectionglyph",
			   Cc,Cc,
			   fontsize,spacing);
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

  if(!config.exists("coldfront")) return;

  ++ncoldfronts;
  std::string id = "coldfront" + boost::lexical_cast<std::string>(ncoldfronts);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = getCssSize("coldfrontglyph","font-size");
  double spacing = getSetting<double>("coldfront","letter-spacing",60.0);

  const char * Cc = ((theFeature.orientation() == "-") ? "C" : "c");

  render_front(path,paths,coldfronts,id,
			   "coldfront","coldfrontglyph",
			   Cc,Cc,
			   fontsize,spacing);
}

// ----------------------------------------------------------------------
/*!
 * \brief Render contrails
 */
// ----------------------------------------------------------------------

void SvgRenderer::visit(const woml::Contrails & theFeature)
{
  if(options.debug)	std::cerr << "Visiting Contrails" << std::endl;

  render_aerodrome<woml::Contrails>(theFeature);
}

// ----------------------------------------------------------------------
/*!
 * \brief Render icing
 */
// ----------------------------------------------------------------------

void SvgRenderer::visit(const woml::Icing & theFeature)
{
  if(options.debug)	std::cerr << "Visiting Icing" << std::endl;

  render_aerodrome<woml::Icing>(theFeature);
}

// ----------------------------------------------------------------------
/*!
 * \brief Render turbulence
 */
// ----------------------------------------------------------------------

void SvgRenderer::visit(const woml::Turbulence & theFeature)
{
  if(options.debug)	std::cerr << "Visiting Turbulence" << std::endl;

  render_aerodrome<woml::Turbulence>(theFeature);
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a InfoText
 */
// ----------------------------------------------------------------------

void
SvgRenderer::visit(const woml::InfoText & theFeature)
{
  if(options.debug)	std::cerr << "Visiting InfoText" << std::endl;

  // If no text for the locale, render empty text to overwrite the template placeholders

  const std::string & text = theFeature.text(options.locale);
  int x = 0,y = 0;

  render_text(texts,"Text",theFeature.name(),(text != options.locale) ? NFmiStringTools::UrlDecode(text) : "",x,y);
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a Jet
 */
// ----------------------------------------------------------------------

void
SvgRenderer::visit(const woml::JetStream & theFeature)
{
  if(options.debug)	std::cerr << "Visiting Jet" << std::endl;

  if(!config.exists("jet")) return;

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
 * \brief Render migratory birds
 */
// ----------------------------------------------------------------------

void
SvgRenderer::visit(const woml::MigratoryBirds & theFeature)
{
  if(options.debug)	std::cerr << "Visiting MigratoryBirds" << std::endl;

  render_aerodrome<woml::MigratoryBirds>(theFeature);
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

  if(!config.exists("occludedfront")) return;

  ++noccludedfronts;
  std::string id = "occludedfront" + boost::lexical_cast<std::string>(noccludedfronts);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = getCssSize("occludedfrontglyph","font-size");
  double spacing = getSetting<double>("occludedfront","letter-spacing",60.0);

  const char * CWcw = ((theFeature.orientation() == "-") ? "CW" : "cw");

  render_front(path,paths,occludedfronts,id,
			   "occludedfront","occludedfrontglyph",
			   CWcw,CWcw,
			   fontsize,spacing);
}

// ----------------------------------------------------------------------
/*!
 * \brief Render ParameterValueSetPoint
 */
// ----------------------------------------------------------------------

void SvgRenderer::visit(const woml::ParameterValueSetPoint & theFeature)
{
	if(options.debug)	std::cerr << "Visiting ParameterValueSetPoint" << std::endl;

	// Wind with speed consists of two GeophysicalParameterValues; FlowDirectionMeasure and NumericalSingleValueMeasure.
	// Pass the value to symbol rendering so value based conditional settings can be used.

	const woml::GeophysicalParameterValueSet * params = theFeature.parameters().get();
	const woml::FlowDirectionMeasure * fdm = NULL;
	const woml::NumericalSingleValueMeasure * svm = NULL;
	woml::GeophysicalParameterValueList::const_iterator itvfdm = params->values().end(),itvsvm = params->values().end();
	woml::GeophysicalParameterValueList::const_iterator itv;

	for (itv = params->values().begin(); ((itv != params->values().end()) && (!(fdm && svm))); itv++) {
		const woml::GeophysicalParameterValue & theValue = *itv;

		if (!fdm) {
			fdm = dynamic_cast<const woml::FlowDirectionMeasure *>(theValue.value());
			itvfdm = itv;
		}

		if (!svm) {
			svm = dynamic_cast<const woml::NumericalSingleValueMeasure *>(theValue.value());
			itvsvm = itv;
		}
	}

	if (fdm) {
		const woml::GeophysicalParameterValue & theValue = *itvfdm;
		render_symbol("ParameterValueSetPoint",pointsymbols,theValue.parameter().name(),fdm->value(),theFeature.point()->lon(),theFeature.point()->lat(),&theFeature,svm);
	}

	if (svm) {
		const woml::GeophysicalParameterValue & theValue = *itvsvm;
		render_value("ParameterValueSetPoint",pointvalues,theValue.parameter().name(),svm,NULL,theFeature.point()->lon(),theFeature.point()->lat(),fdm ? true : false);
	}
	else {
		const woml::GeophysicalParameterValue & theValue = params->values().front();
		const woml::NumericalValueRangeMeasure * vrm = dynamic_cast<const woml::NumericalValueRangeMeasure *>(theValue.value());

		if (vrm)
			render_value("ParameterValueSetPoint",pointvalues,theValue.parameter().name(),&(vrm->lowerLimit()),&(vrm->upperLimit()),theFeature.point()->lon(),theFeature.point()->lat());
	}
}

// ----------------------------------------------------------------------
/*!
 * \brief Utility function for obtaining PointMeteorologicalSymbol's class and code
 */
// ----------------------------------------------------------------------

std::string
PointMeteorologicalSymbolDefinition(const woml::PointMeteorologicalSymbol & theFeature,std::string & symCode)
{
  const woml::MeteorologicalSymbol symbol = theFeature.symbol();

  woml::MeteorologicalSymbol::DefinitionReferences_const_iterator it = symbol.DefinitionReferences_begin();
  if (it == symbol.DefinitionReferences_end())
	throw std::runtime_error("PointMeteorologicalSymbol's class is unspecified");

  return woml::MeteorologicalSymbol::symbolDefinition(it,symCode);
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

  // Get the symbol class and code
  std::string symCode;
  std::string symClass = PointMeteorologicalSymbolDefinition(theFeature,symCode);

  // Render the symbol
  render_symbol("pointMeteorologicalSymbol",pointsymbols,symClass,symCode,theFeature.point()->lon(),theFeature.point()->lat(),&theFeature);
}

// ----------------------------------------------------------------------
/*!
 * \brief Render PressureCenterType derived classes
 */
// ----------------------------------------------------------------------

#define RenderPressureCenterTypeDerivedClass(c) \
void SvgRenderer::visit(const c & theFeature) \
{ \
  if(options.debug)	std::cerr << "Visiting " << theFeature.className() << std::endl; \
  render_symbol("pointMeteorologicalSymbol",pointsymbols,theFeature.className(),"",theFeature.point()->lon(),theFeature.point()->lat(),&theFeature); \
}

RenderPressureCenterTypeDerivedClass(woml::AntiCyclone)
RenderPressureCenterTypeDerivedClass(woml::Antimesocyclone)
RenderPressureCenterTypeDerivedClass(woml::Cyclone)
RenderPressureCenterTypeDerivedClass(woml::HighPressureCenter)
RenderPressureCenterTypeDerivedClass(woml::LowPressureCenter)
RenderPressureCenterTypeDerivedClass(woml::Mesocyclone)
RenderPressureCenterTypeDerivedClass(woml::Mesolow)
RenderPressureCenterTypeDerivedClass(woml::PolarCyclone)
RenderPressureCenterTypeDerivedClass(woml::PolarLow)
RenderPressureCenterTypeDerivedClass(woml::TropicalCyclone)

// ----------------------------------------------------------------------
/*!
 * \brief Render a Ridge
 */
// ----------------------------------------------------------------------

void
SvgRenderer::visit(const woml::Ridge & theFeature)
{
  if(options.debug)	std::cerr << "Visiting Ridge" << std::endl;

  if(!config.exists("ridge")) return;

  ++nridges;

  std::string id = "ridge" + boost::lexical_cast<std::string>(nridges);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = getCssSize("ridgeglyph","font-size");
  double spacing = getSetting<double>("ridge","letter-spacing",60.0);

  render_front(path,paths,ridges,id,
			   "ridge","ridgeglyph",
			   "r","R",
			   fontsize,spacing);
}

// ----------------------------------------------------------------------
/*!
 * \brief Render StormType derived classes
 */
// ----------------------------------------------------------------------

#define RenderStormTypeDerivedClass(c) RenderPressureCenterTypeDerivedClass(c)

RenderStormTypeDerivedClass(woml::ConvectiveStorm)
RenderStormTypeDerivedClass(woml::Storm)

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

  const woml::CubicSplineSurface surface = theFeature.controlSurface();

  Path path = PathFactory::create(surface);

  PathProjector proj(area);
  path.transform(proj);

  render_surface(path,precipitationareas,id,theFeature.rainPhaseName(),&theFeature);
}

// ----------------------------------------------------------------------
/*!
 * \brief Get area's symbols
 */
// ----------------------------------------------------------------------

void getAreaSymbols(const woml::ParameterValueSetArea & theFeature,std::list<std::string> & symbols)
{
  const boost::shared_ptr<woml::GeophysicalParameterValueSet> & parameters = theFeature.parameters();

  std::list<woml::GeophysicalParameterValue>::iterator pvend = parameters->editableValues().end(),itpv;

  for (itpv = parameters->editableValues().begin(); (itpv != pvend); itpv++) {
	const woml::CategoryValueMeasure * cvm = dynamic_cast<const woml::CategoryValueMeasure *>(itpv->value());

	if (cvm) {
		std::string symbol = boost::algorithm::trim_copy(cvm->category());

		if (!symbol.empty())
			symbols.push_back(symbol);
	}
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a ParameterValueSetArea
 */
// ----------------------------------------------------------------------

void
SvgRenderer::visit(const woml::ParameterValueSetArea & theFeature)
{
  if(options.debug)	std::cerr << "Visiting ParameterValueSetArea" << std::endl;

  ++nprecipitationareas;
  std::string id = "paramvalsetarea" + boost::lexical_cast<std::string>(nprecipitationareas);

  const woml::CubicSplineSurface surface = theFeature.controlSurface();

  Path path = PathFactory::create(surface);

  PathProjector proj(area);
  path.transform(proj);

  // Get the area symbol(s)

  std::list<std::string> areaSymbols;

  getAreaSymbols(theFeature,areaSymbols);

  render_surface(path,precipitationareas,id,theFeature.parameters()->values().front().parameter().name(),&theFeature,&areaSymbols);
}

// ----------------------------------------------------------------------
/*!
 * \brief Render surface visibility
 */
// ----------------------------------------------------------------------

void
SvgRenderer::visit(const woml::SurfaceVisibility & theFeature)
{
  if(options.debug)	std::cerr << "Visiting SurfaceVisibility" << std::endl;

  render_aerodrome<woml::SurfaceVisibility>(theFeature);
}

// ----------------------------------------------------------------------
/*!
 * \brief Render surface weather
 */
// ----------------------------------------------------------------------

void
SvgRenderer::visit(const woml::SurfaceWeather & theFeature)
{
  if(options.debug)	std::cerr << "Visiting SurfaceWeather" << std::endl;

  render_aerodrome<woml::SurfaceWeather>(theFeature);
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

  if(!config.exists("trough")) return;

  ++ntroughs;

  std::string id = "trough" + boost::lexical_cast<std::string>(ntroughs);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = getCssSize("troughglyph","font-size");
  double spacing = getSetting<double>("trough","letter-spacing",60.0);

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

  if(!config.exists("uppertrough")) return;

  ++nuppertroughs;

  std::string id = "uppertrough" + boost::lexical_cast<std::string>(nuppertroughs);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = getCssSize("uppertroughglyph","font-size");
  double spacing = getSetting<double>("uppertrough","letter-spacing",60.0);

  render_front(path,paths,uppertroughs,id,
			   "uppertrough","uppertroughglyph",
			   "t","T",
			   fontsize,spacing);

}

// ----------------------------------------------------------------------
/*!
 * \brief Render a WarmAdvection
 */
// ----------------------------------------------------------------------

void
SvgRenderer::visit(const woml::WarmAdvection & theFeature)
{
  if(options.debug)	std::cerr << "Visiting WarmAdvection" << std::endl;

  if(!config.exists("warmadvection")) return;

  ++nwarmadvections;
  std::string id = "warmadvection" + boost::lexical_cast<std::string>(nwarmadvections);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = getCssSize("warmadvectionglyph","font-size");
  double spacing = getSetting<double>("warmadvection","letter-spacing",60.0);

  const char * Ww = ((theFeature.orientation() == "-") ? "W" : "w");

  render_front(path,paths,warmfronts,id,
			   "warmadvection","warmadvectionglyph",
			   Ww,Ww,
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

  if(!config.exists("warmfront")) return;

  ++nwarmfronts;
  std::string id = "warmfront" + boost::lexical_cast<std::string>(nwarmfronts);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = getCssSize("warmfrontglyph","font-size");
  double spacing = getSetting<double>("warmfront","letter-spacing",60.0);

  const char * Ww = ((theFeature.orientation() == "-") ? "W" : "w");

  render_front(path,paths,warmfronts,id,
			   "warmfront","warmfrontglyph",
			   Ww,Ww,
			   fontsize,spacing);
}

// ----------------------------------------------------------------------
/*!
 * \brief Render winds
 */
// ----------------------------------------------------------------------

void
SvgRenderer::visit(const woml::Winds & theFeature)
{
  if(options.debug)	std::cerr << "Visiting Winds" << std::endl;

  render_aerodrome<woml::Winds>(theFeature);
}

// ----------------------------------------------------------------------
/*!
 * \brief Render zero tolerance
 */
// ----------------------------------------------------------------------

void
SvgRenderer::visit(const woml::ZeroTolerance & theFeature)
{
  if(options.debug)	std::cerr << "Visiting ZeroTolerance" << std::endl;

  render_aerodrome<woml::ZeroTolerance>(theFeature);
}

// ----------------------------------------------------------------------
/*!
 * \brief Test if the given CSS class exists
 */
// ----------------------------------------------------------------------

//bool SvgRenderer::hasCssClass(const std::string & theCssClass) const
//{
//  return (svgbase.find("."+theCssClass) != std::string::npos);
//}

// ----------------------------------------------------------------------
/*!
 * \brief Find the given CSS size setting
 */
// ----------------------------------------------------------------------

double SvgRenderer::getCssSize(const std::string & theCssClass,
							   const std::string & theAttribute,
							   double defaultValue)
{
  std::string::size_type pos = svgbase.find("." + theCssClass);
  if(pos == std::string::npos)
	return defaultValue;

  // Require attribute to come before next "}"

  std::string::size_type pos2 = svgbase.find("}", pos);
  if(pos2 == std::string::npos)
	return defaultValue;

  std::string::size_type pos1 = svgbase.find(theAttribute+":", pos);
  if(pos1 == std::string::npos || pos1 > pos2)
	return defaultValue;

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
/*!
 * \brief Find the given config setting
 */
// ----------------------------------------------------------------------

template <typename T>
T SvgRenderer::getSetting(const std::string & theConfigClass,
						  const std::string & theAttribute,
						  T theDefaultValue)
{
  if(!config.exists(theConfigClass + "." + theAttribute)) return theDefaultValue;

  return lookup<T>(config,theConfigClass + "." + theAttribute);
}

#ifdef CONTOURING

// ----------------------------------------------------------------------
/*
 * TODO: This part should be refactored when finished
 */
// ----------------------------------------------------------------------

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

	  const char * typeMsg = "contourlines must contain a list of groups in parenthesis";

	  const libconfig::Setting & contourSpecs = config.lookup("contourlines");

	  if(!contourSpecs.isList())
		throw std::runtime_error(typeMsg);

	  std::size_t linenumber = 0;

	  for(int i=0; i<contourSpecs.getLength(); ++i)
		{
		  const libconfig::Setting & specs = contourSpecs[i];

		  if(!specs.isGroup())
			throw std::runtime_error(typeMsg);

		  // Now process this individual contour

		  std::string paramname = lookup<std::string>(specs,"contourlines","parameter");
		  std::string classname = lookup<std::string>(specs,"contourlines","class");
		  std::string outputname = lookup<std::string>(specs,"contourlines","output");
		  std::string smoother = lookup<std::string>(specs,"contourlines","smoother");

		  float start = lookup<float>(specs,"contourlines","start");
		  float stop  = lookup<float>(specs,"contourlines","stop");
		  float step  = lookup<float>(specs,"contourlines","step");

		  const libconfig::Setting & labels = specs["labels"];
		  if(!labels.isGroup())
			throw std::runtime_error("contourlines labels must be a group");

		  std::string labelclassname = lookup<std::string>(labels,"contourlines.labels","class");
		  std::string labeloutputname = lookup<std::string>(labels,"contourlines.labels","output");
		  
		  std::string symbol = lookup<std::string>(labels,"contourlines.labels","symbol");
		  std::string symbolclass = lookup<std::string>(labels,"contourlines.labels","symbolclass");

		  double mindistance = lookup<double>(labels,"contourlines.labels","mindistance");
		  int modulo = lookup<int>(labels,"contourlines.labels","modulo");

		  const libconfig::Setting & labelcoordinates = labels["coordinates"];

		  if(!labelcoordinates.isArray())
			throw std::runtime_error("coordinates setting must be an array");
		  
		  if(labelcoordinates.getLength() % 2 != 0)
			throw std::runtime_error("coordinates setting must have an even number of integer coordinates");

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

		  if(matrix.NX() == 0 || matrix.NY() == 0)
			throw std::runtime_error("Could not extract set valid time " +
									 std::string(validtime.ToStr(kYYMMDDHHMM)) +
									 "from query data");

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

		  for(float value=start; value<=stop; value+=step)
			{
			  Path path;
			  MyContourer::line(path,grid,value,false,hints);

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

				  // Do not add labels if the modulo condition is not satisfied

				  int intvalue = modulo*static_cast<int>(round(value/modulo));
				  if(value != intvalue)
					continue;

				  // Add labels for near enough points
				  for(int i=0; i<labelcoordinates.getLength(); i+=2)
					{
					  int xpos = labelcoordinates[i];
					  int ypos = labelcoordinates[i+1];
					  std::pair<double,double> nearest = path.nearestVertex(xpos,ypos);
					  double x = nearest.first;
					  double y = nearest.second;
					  double distance = sqrt((x-xpos)*(x-xpos)+(y-ypos)*(y-ypos));
					  if(distance < mindistance)
						{
						  if(!symbol.empty())
							contours[labeloutputname] << "<use xlink:href=\"#"
													  << symbol
													  << "\" class=\""
													  << symbolclass
													  << "\" x=\""
													  << round(x)
													  << "\" y=\""
													  << round(y)
													  << "\"/>\n";
						  
						  contours[labeloutputname] << "<text class=\""
													<< labelclassname
													<< "\" x=\""
													<< round(x)
													<< "\" y=\""
													<< round(y)
													<< "\">"
													<< intvalue
													<< "</text>\n";
						  
						}
					}

				}
			}

		}

	}

}

#endif

} // namespace frontier
