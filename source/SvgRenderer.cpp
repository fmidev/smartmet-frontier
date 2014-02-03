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

#include <smartmet/woml/InfoText.h>

#include <smartmet/newbase/NFmiArea.h>
#include <smartmet/newbase/NFmiStringTools.h>

#ifdef __contouring__
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

#include <iomanip>
#include <iostream>
#include <limits>
#include <list>

#include <cairo.h>

#include <smartmet/newbase/NFmiMetTime.h>
#include <smartmet/newbase/NFmiString.h>

namespace frontier
{

  // ----------------------------------------------------------------------
  /*!
   * \brief Convert ptime NFmiMetTime
   */
  // ----------------------------------------------------------------------

#ifdef __contouring__

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
						scope.push_back(&hdrSpecs[i]);

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
					std::string pref(configValue<std::string>(scope,hdrClass,"pref"));

					// utc
					bool isSet;
					bool utc(configValue<bool>(scope,hdrClass,"utc",s_optional,&isSet));

					// Store formatted datum
					texts[HEADERhdrClass] << (datetime.is_not_a_date_time()
											  ? ""
											  : svgescapetext(toFormattedString(datetime,pref,isSet && utc,language).CharPtr(),true));

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

  void getFillPositions(NFmiFillAreas::const_iterator iter,
		  	  	  	  	int symbolWidth,int symbolHeight,
			  			float scale,
			  			bool verticalRects,
			  			NFmiFillPositions & fpos,
			  			int & symCnt)

  {
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
	std::ostringstream line;
	std::string word;

// for regenerate with smaller fontsize {
//

	unsigned int maxLineWidth = 0;
	maxLineHeight = 0;

	for (l = 0; (l < textlc); l++) {
		// Next line
		//
		std::vector<std::string> words;
		boost::split(words,inputLines[l],boost::is_any_of(" \t"));

		cairo_text_extents(cr,inputLines[l].c_str(),&extents);
		unsigned int lineHeight = static_cast<unsigned int>(ceil(extents.height));

		if (lineHeight > maxLineHeight)
			maxLineHeight = lineHeight;

		size_t w,linewc = 0,textwc = words.size(),pos = std::string::npos;
		double lineWidth = 0;
		bool fits;

		for (w = 0; (w < textwc); ) {
			// Next word.
			// Add a space in front of it if not the first word of the line
			//
			word = std::string((linewc > 0) ? " " : "") + words[w];
			cairo_text_extents(cr,word.c_str(),&extents);

			if (! (fits = (ceil(lineWidth + extents.x_advance) < maxWidth))) {
				// Goes too wide, try cutting to first comma or period
				//
				pos = word.find_first_of(",.");

				if (pos != std::string::npos) {
					std::string cutted = word.substr(0,pos + 1);
					cairo_text_extents(cr,cutted.c_str(),&extents);

					if ((fits = (ceil(lineWidth + extents.x_advance) < maxWidth)))
						word = cutted;
				}
			}

			if (fits) {
				line << word;

				lineWidth += extents.x_advance;
				linewc++;

				if ((fits = (pos == std::string::npos))) {
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

			if (linewc > 0) {
				unsigned int width = static_cast<unsigned int>(ceil(lineWidth));

				if (width > maxLineWidth)
					maxLineWidth = width;
			}
			else {
				// A single word exceeding the max line width
				//
				line << word;
				w++;
			}

			// Store the line; maximum line width or end of line reached

			outputLines.push_back(line.str());

			line.str("");
			line.clear();
			lineWidth = 0;
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

  void SvgRenderer::render_text(const std::string & confPath,
		  	  	  	  	  	    const std::string & textName,
		  	  	  	  	  	    const std::string & text)
  {
	try {
		const char * typeMsg = " must contain a list of groups in parenthesis";
		const char * styleMsg = ": slant must be 'normal', 'italic' or 'oblique'";
		const char * weightMsg = ": weight must be 'normal' or 'bold'";

		std::string TEXTCLASStextName("TEXTCLASS" + textName);
		std::string TEXTAREAtextName("TEXTAREA" + textName);
		std::string TEXTtextName("TEXT" + textName);

		// Set initial textarea rect in case no output is generated

		texts[TEXTAREAtextName] << " x=\"0\" y=\"0\" width=\"0\" height=\"0\"";

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
			if (nameMatch || noName || ((i == lastIdx) && hasLocaleGlobals)) {
				if (nameMatch || noName) {
					if (nameMatch)
						textIdx = i;

					// Locale
					//
					std::string locale(toLower(configValue<std::string>(specs,textName,"locale",NULL,s_optional)));
					bool localeMatch = (locale == options.locale);

					if (localeMatch || (locale == "")) {
						scope.push_back(&textSpecs[i]);

						if ((nameMatch && (locale == "")) || (noName && localeMatch))
							hasLocaleGlobals = true;
					}

					if ((noName || (!localeMatch)) && ((i < lastIdx) || (!hasLocaleGlobals)))
						continue;
				}

				// Font name
				std::string font = configValue<std::string>(scope,textName,"font-family");

				// Font size
				unsigned int fontsize = configValue<unsigned int>(scope,textName,"font-size");

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

				// Max width, height and x/y margin
				unsigned int maxWidth = configValue<unsigned int>(scope,textName,"width");

				bool isSet;
				unsigned int maxHeight = configValue<unsigned int>(scope,textName,"height",s_optional,&isSet);
				if (! isSet)
					maxHeight = 0;

				unsigned int margin = configValue<unsigned int>(scope,textName,"margin",s_optional,&isSet);
				if (! isSet)
					margin = 2;

				if (maxWidth <= 20)
					maxWidth = 20;
				if ((maxHeight != 0) && (maxHeight <= 20))
					maxHeight = 20;

				// Split the text into lines using given max. width and height

				std::list<std::string> textLines;
				unsigned int textWidth,textHeight,maxLineHeight;

				getTextLines(text,
						  	 font,fontsize,slant,_weight,
						  	 maxWidth,maxHeight,
							 textLines,
							 textWidth,textHeight,maxLineHeight);

				// Store the css class definition

				std::string textClass("text"+ textName);

				texts[TEXTCLASStextName] << "." << textClass
										 << " {\nfont-family: " << font
										 << ";\nfont-size: " << fontsize
										 << ";\nfont-weight : " << weight
										 << ";\nfont-style : " << style
										 << ";\n}\n";

				// Store geometry for text border

				texts[TEXTAREAtextName].str("");
				texts[TEXTAREAtextName].clear();

				texts[TEXTAREAtextName] << " x=\"0\" y=\"0\""
										<< " width=\"" << (2 * margin) + textWidth
										<< "\" height=\"" << (2 * margin) + textHeight
										<< "\" ";

				// Store the text. Final position (transformation) is (supposed to be) defined in the template

				std::list<std::string>::const_iterator it = textLines.begin();

				for (int y = (margin + maxLineHeight); (it != textLines.end()); it++, y += maxLineHeight)
					texts[TEXTtextName] << "<text class=\"" << textClass
										<< "\" x=\"" << margin
										<< "\" y=\"" << y
										<< "\">" << svgescapetext(*it) << "</text>\n";

				return;
			}  // if
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
   * \brief Surface rendering
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::render_surface(const Path & path,
		  	  	  	  	  	  	   std::ostringstream & surfaces,
		  	  	  	  	  	  	   const std::string & id,
		  	  	  	  	  	  	   const std::string & surfaceName)		// cloudType (for CloudArea) or rainPhase (for SurfacePrecipitationArea)
  {
	const std::string confPath("Surface");

	try {
		const char * typeMsg = " must contain a list of groups in parenthesis";
		const char * surfTypeMsg = ": surface type must be 'pattern', 'mask', 'glyph' or 'svg'";

		const libconfig::Setting & surfSpecs = config.lookup(confPath);
		if(!surfSpecs.isList())
			throw std::runtime_error(confPath + typeMsg);

		settings s_name((settings) (s_base + 0));

		int surfIdx = -1;
		int globalsIdx = -1;

		for(int i=0; i<surfSpecs.getLength(); ++i)
		{
			const libconfig::Setting & specs = surfSpecs[i];
			if(!specs.isGroup())
				throw std::runtime_error(confPath + typeMsg);

			try {
				if (lookup<std::string>(specs,confPath,"name",s_name) == surfaceName) {
					surfIdx = i;

					// Missing settings from globals when available
					libconfig::Setting * globalScope = ((globalsIdx >= 0) ? &surfSpecs[globalsIdx] : NULL);

					// Surface type; pattern, mask, glyph or svg

					std::string type = configValue<std::string>(specs,surfaceName,"type",globalScope);
					bool filled = ((type == "fill") || (type == "fill+mask"));
					bool masked = ((type == "mask") || (type == "fill+mask"));

					if ((type == "pattern") || (type == "glyph") || filled || masked) {
						// Class and style
						//
						std::string classDef((masked || (type == "glyph")) ? configValue<std::string>(specs,surfaceName,"class",globalScope) : "");
						std::string style((type != "fill") ? configValue<std::string>(specs,surfaceName,"style",globalScope) : "");

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

							if (filled) {
								// Fill symbol width and height and scale factor for bounding box for
								// calculating suitable symbol positions
								//
								int _width = configValue<int>(specs,surfaceName,"width",globalScope);
								int _height = configValue<int>(specs,surfaceName,"height",globalScope);
								float scale = configValue<float>(specs,surfaceName,"scale",globalScope);

								if (scale < 0.3)
									throw std::runtime_error(confPath + ": minimum scale is 0.3");

								// If autoscale is true (default: false) symbol is shrinken until
								// at least one symbol can be placed on the surface
								bool isSet;
								bool autoScale = configValue<bool>(specs,surfaceName,"autoscale",globalScope,s_optional,&isSet);
								if (!isSet)
									autoScale = false;

								// If mode is 'vertical' (default: horizontal) the symbols are placed using y -axis as the
								// primary axis (using maximum vertical fillrects instead of maximum horizontal fillrects)
								std::string mode = configValue<std::string>(specs,surfaceName,"mode",globalScope,s_optional);
								bool verticalRects = (mode == "vertical");

								// If direction is 'both' (default: up) iterating vertically over the surface y-coordinates
								// from both directions; otherwise from bottom up
								std::string direction = configValue<std::string>(specs,surfaceName,"direction",globalScope,s_optional);
								bool scanUpDown = (direction == "both");

								// If showareas is true (default: false) output the fill area bounding boxes too
								bool showAreas = configValue<bool>(specs,surfaceName,"showareas",globalScope,s_optional,&isSet);
								if (!isSet)
									showAreas = false;

								// Get fill areas.
								//
								// If none available and autoscale is set, retry with smaller symbol size
								// down to half of the original size.
								//
								NFmiFillMap fmap;
								NFmiFillAreas areas;
								path.length(&fmap);

								int w = static_cast<int>(std::floor(0.5+area->Width()));
								int h = static_cast<int>(std::floor(0.5+area->Height()));
								int width = _width;
								int height = _height;

								bool ok;
								do {
									ok = fmap.getFillAreas(w,h,width,height,scale,verticalRects,areas,false,scanUpDown);

									if (! ok) {
										width -= 2;
										height -= 2;
									}

								}
								while ((! ok) && (width >= (_width / 2)) && (height >= (_height / 2)));

								// Get symbol positions withing the fill areas

								NFmiFillPositions fpos;

								const char *clrs[] =
								{ "red",
								  "blue",
								  "green",
								  "orange",
								  "brown",
								  "yellow"
								};
								int symCnt = 0,clrCnt = (sizeof(clrs) / sizeof(char *)),clrIdx = 0;

								NFmiFillAreas::const_iterator iter;
								for (iter = areas.begin(); (iter != areas.end()); iter++) {
									getFillPositions(iter,width,height,scale,verticalRects,fpos,symCnt);

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

								// Render the symbols
								render_symbol(confPath,surfaceName,"",0,0,&fpos);
							}

							if (type != "fill") {
								surfaces << "<use class=\""
										 << classDef
										 << "\" xlink:href=\"#"
										 << pathId;

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
								else {
									// glyph
									std::string _glyph("U");
									double textlength = static_cast<double>(_glyph.size());
									double len = path.length();
									double fontsize = getCssSize(".cloudborderglyph","font-size");
									double spacing = 0.0;

									int CosmologicalConstant = 2;
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
//												 << "<!-- len=" << len << " textlength=" << textlength << " fontsize=" << fontsize << " -->"
												 << "\n</text>\n";
									}
									else
										surfaces << "\"/>\n";
								}
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
			}
			catch (SettingIdNotFoundException & ex) {
				if (ex.id() == s_name)
					// Global settings have no name
					//
					globalsIdx = i;
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
		  	  	  	  	  	  	 std::list<const libconfig::Setting *> & scope,
		  	  	  	  	  	  	 const std::string & confPath,
		  	  	  	  	  	  	 const std::string & symClass,
		  	  	  	  	  	  	 const std::string & symCode,
		  	  	  	  	  	  	 settings s_code,
		  	  	  	  	  	  	 const boost::posix_time::ptime & dateTimeValue,
		  	  	  	  	  	  	 std::string & folder)
  {
	const char * codeMsg = ": symbol code: [<folder>/]<code> expected";

	// Search for the code truncating the scope to have the matching block as the last block

	std::string code = configValue<std::string,int>(scope,symClass,symCode,s_code,NULL,true);

	std::vector<std::string> cols;
	boost::split(cols,code,boost::is_any_of("/"));

	int codeIdx;
	boost::trim(cols[codeIdx = 0]);

	if (cols.size() == 2)
		boost::trim(cols[codeIdx = 1]);

	if ((cols.size() > 2) || (cols[0] == "") || (cols[codeIdx] == ""))
		throw std::runtime_error(confPath + ": '" + code + "'" + codeMsg);

	folder = ((codeIdx == 1) ? cols[0] : "");

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
   * \brief Symbol mapping. Sets the mapped code into 'mappedCode' and
   *		returns true if nonempty mapping exists.
   */
  // ----------------------------------------------------------------------

  bool symbolMapping(const libconfig::Config & config,const std::string & confPath,const std::string & symCode,settings s_code,std::string & mappedCode)
  {
	const char * typeMsg = " must contain a group in curly brackets";

	const libconfig::Setting & symbolSpecs = config.lookup(confPath);
	if(!symbolSpecs.isGroup())
		throw std::runtime_error(confPath + typeMsg);

	// Symbol's <code>; code_<symCode> = <code>
	//
	// Note: libconfig does not support special characters in configuration keys; the code values
	//		 have their +/- characters replaced with P_ and M_ (e.g. '+RA' is converted to 'P_RA')

	mappedCode = boost::algorithm::trim_copy(symCode);
	bool isSet = false;

	if (!mappedCode.empty()) {
		boost::algorithm::replace_all(mappedCode,"+","P_");
		boost::algorithm::replace_all(mappedCode,"-","M_");

		mappedCode = configValue<std::string>(symbolSpecs,confPath,mappedCode,NULL,s_code,&isSet);
	}

	return (isSet && (!mappedCode.empty()));
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
		  	  	  	  	  	  	  	  	   bool codeValue,bool mappedCode)
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
		double scale = (double) configValue<double,int>(symbolSpecs,confPath,"scale",NULL,s_optional,&hasScale);

		std::ostringstream sc;
		if (hasScale)
			sc << " scale(" << std::fixed << std::setprecision(1) << scale << ")";

		npointsymbols++;
		std::string id = confPath + boost::lexical_cast<std::string>(npointsymbols);

		if (codeValue)
			texts[symClass] << "<g id=\"" << id
							<< "\" class=\"" << class1 << class2
							<< "\" transform=\"translate(" << std::fixed << std::setprecision(1) << x << "," << y << ")"
							<< sc.str()
							<< "\">\n"
							<< "<use xlink:href=\"" << uri << "\"/>\n"
							<< "</g>\n";
		else
			texts[symClass] << "<g id=\"" << id
							<< "\" class=\"" << class1
							<< "\" transform=\"translate(" << std::fixed << std::setprecision(1) << x << "," << y << ")"
							<< sc.str()
							<< "\">\n"
							<< "<text>" << value << "</text>\n"
							<< "</g>\n";

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
		  	  	  	  	  	  	  	  	    const std::string & confPath)
  {
	// Document's time period

	const boost::posix_time::time_period & tp = axisManager->timePeriod();
	boost::posix_time::ptime bt = tp.begin(),et = tp.end();

	// Get the elevations from the time serie

	ElevGrp eGrp;

	bool ground = (elevationGroup(theFeature.timeseries(),bt,et,eGrp) == t_ground);

	if (eGrp.size() == 0)
		return;

	// Class

	std::string symClass(boost::algorithm::to_upper_copy(confPath + theFeature.classNameExt()));

	// Render the symbols

	try {
		const char * grouptypeMsg = " must contain a group in curly brackets";

		const libconfig::Setting & specs = config.lookup(confPath);
		if(!specs.isGroup())
			throw std::runtime_error(confPath + grouptypeMsg);

		ElevGrp::const_iterator egend = eGrp.end(),iteg;

		for (iteg = eGrp.begin(); (iteg != egend); iteg++) {
			// Note: Using the lo-hi range average value for "nonground" elevations (MigratoryBirds) and
			// 		 0 for "ground" elevation (SurfaceVisibility, SurfaceWeather)
			//
			double x = axisManager->xOffset(iteg->validTime()),y = 0.0;

			if (!ground) {
				const woml::Elevation & e = iteg->Pv()->elevation();
				boost::optional<woml::NumericalSingleValueMeasure> itsBoundedLo = (e.bounded() ? e.lowerLimit() : woml::NumericalSingleValueMeasure());
				const boost::optional<woml::NumericalSingleValueMeasure> & itsLoLimit = (e.bounded() ? itsBoundedLo : e.value());
				boost::optional<woml::NumericalSingleValueMeasure> itsBoundedHi = (e.bounded() ? e.upperLimit() : woml::NumericalSingleValueMeasure());
				const boost::optional<woml::NumericalSingleValueMeasure> & itsHiLimit = (e.bounded() ? itsBoundedHi : e.value());

				double lo = ((!itsLoLimit) ? 0.0 : itsLoLimit->numericValue());
				double hi = ((!itsHiLimit) ? 0.0 : itsHiLimit->numericValue());

				y = axisManager->scaledElevation(lo + ((hi - lo) / 2));

				if (y < 0)
					continue;

				// Move first and last time instant's symbols to keep them better withing the area

				if (iteg->validTime() == bt) {
					// Offset in px for moving first time instant's symbols to the right
					//
					bool isSet;
					int sOffset = configValue<int>(specs,confPath,"soffset",NULL,s_optional,&isSet);

					if (isSet)
						x += sOffset;
				}
				else if (iteg->validTime() == et) {
					// Offset in px for moving last time instant's symbols to the left
					//
					bool isSet;
					int eOffset = configValue<int>(specs,confPath,"eoffset",NULL,s_optional,&isSet);

					if (isSet)
						x -= eOffset;
				}
			}

			// Category/magnitude or visibility (meters)

			const woml::CategoryValueMeasure * cvm = dynamic_cast<const woml::CategoryValueMeasure *>(iteg->Pv()->value());
			const woml::NumericalSingleValueMeasure * nsv = (cvm ? NULL : dynamic_cast<const woml::NumericalSingleValueMeasure *>(iteg->Pv()->value()));

			if ((!cvm) && (!nsv))
				throw std::runtime_error("render_aerodromeSymbol: " + confPath + ": CategoryValueMeasure or NumericalSingleValueMeasure values expected");

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
		  	  	  	  	  	  	  const std::string & symClass,
		  	  	  	  	  	  	  const std::string & symCode,
		  	  	  	  	  	  	  double lon,double lat,
		  						  NFmiFillPositions * fpos)
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
				scope.push_back(&symbolSpecs[i]);
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
						scope.push_back(&symbolSpecs[i]);

						if (localeMatch)
							localeIdx = i;
						else
							hasLocaleGlobals = _hasLocaleGlobals = true;

						// Search for conditional settings based on the documents validTime (it's time value)

						const libconfig::Setting * condSpecs = matchingCondition(config,confPath,symClass,_symCode,symbolIdx,validtime);

						if (condSpecs)
							scope.push_back(condSpecs);
					}

					// Continue searching unless got (new) current block or reached the end of configuration
					// and there is (new) locale global blocks to use

					if ((!localeMatch) && ((i < lastIdx) || (!_hasLocaleGlobals)))
						continue;
				}

				// Symbol code. May contain folder too; [<folder>/]<code>
				//
				std::string folder;
				std::string code;

				try {
					code = getFolderAndSymbol(config,scope,confPath,symClass,_symCode,s_code,validtime,folder);
				}
				catch (SettingIdNotFoundException & ex) {
					// Symbol code not found
					//
					// Clear _hasLocaleGlobals to indicate the settings have been scanned upto current block and
					// there no use do enter here again unless new locale global block(s) or current block is met
					//
					_hasLocaleGlobals = false;
					continue;
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

				if ((type == "svg") || (type == "img")) {
					// Some settings are required for svg but optional for img
					//
					settings reqORopt = ((type == "svg") ? s_required : s_optional);
					bool hasWidth,hasHeight;

					if (folder.empty())
						folder = configValue<std::string,int>(scope,symClass,"folder",reqORopt);

					int width = configValue<int>(scope,symClass,"width",reqORopt,&hasWidth);
					int height = configValue<int>(scope,symClass,"height",reqORopt,&hasHeight);

					std::string uri = configValue<std::string>(scope,symClass,"url");

					boost::algorithm::replace_all(uri,"%folder%",folder);
					boost::algorithm::replace_all(uri,"%symbol%",code + "." + type);

					std::ostringstream wh;
					if (hasWidth)
						wh << " width=\"" << std::fixed << std::setprecision(0) << width << "px\"";
					if (hasHeight)
						wh << " height=\"" << std::fixed << std::setprecision(0) << height << "px\"";

					if (!fpos)
						pointsymbols << "<image xlink:href=\""
									 << svgescape(uri)
									 << "\" x=\""
									 << std::fixed << std::setprecision(1) << ((lon-width/2) - xoffset)
									 << "\" y=\""
									 << std::fixed << std::setprecision(1) << ((lat-height/2) + yoffset)
									 << "\"" << wh.str() << "/>\n";
					else {
						NFmiFillPositions::const_iterator piter;

						for (piter = fpos->begin(); (piter != fpos->end()); piter++)
							pointsymbols << "<image xlink:href=\""
										 << svgescape(uri)
										 << "\" x=\""
										 << std::fixed << std::setprecision(1) << (piter->x - (width/2))
										 << "\" y=\""
										 << std::fixed << std::setprecision(1) << (piter->y - (height/2))
										 << "\"" << wh.str() << "/>\n";
					}

					return;
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
						pointsymbols << "<text class=\""
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
							pointsymbols << "<text class=\""
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

					return;
				}
				else
					throw std::runtime_error(confPath + ": '" + type + "'" + symTypeMsg);
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
		  	  	  	  	  	  	  	  	  	  	  	  	   double & tightness,bool & borderCompensation,double & minLabelPosHeight,
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

		// Minimum height (px) for the label position

		minLabelPosHeight = (double) configValue<double,int>(specs,confPath,"labelheight",NULL,s_optional,&isSet);

		if (!isSet)
			minLabelPosHeight = 40.0;

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

			// Relative offset for intermediate curve points (controls how much the ends of the area
			// are extended horizontally)

			double xStep = axisManager->xStep();

			double vOffset = configValue<double>(group,confPath,"voffset",globalScope,s_optional,&isSet);
			if (!isSet)
				vOffset = 0.0;
			else
				vOffset *= xStep;

			// Relative offset for intermediate curve point for single elevation (controls how much the
			// ends of the area are extended horizontally)

			double vSOffset = configValue<double>(group,confPath,"vsoffset",globalScope,s_optional,&isSet);
			if (!isSet)
				vSOffset = xStep / 3;
			else
				vSOffset *= xStep;

			// Max offset in px for extending first time instant's elevations to the left

			int sOffset = configValue<int>(group,confPath,"soffset",globalScope,s_optional,&isSet);
			if (!isSet)
				sOffset = 0;

			// Max offset in px for extending last time instant's elevations to the right

			int eOffset = configValue<int>(group,confPath,"eoffset",globalScope,s_optional,&isSet);
			if (!isSet)
				eOffset = 0;

			cloudGroups.push_back(CloudGroup(classDef,
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
											 vOffset,vSOffset,
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
		texts[cg.labelPlaceHolder()] << "<text class=\"" << cg.textClassDef()
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
   * \brief Utility function for getting area's label position(s)
   */
  // ----------------------------------------------------------------------

  void getAreaLabelPos(bool bbCenterLabel,
		  	  	  	   double minx,double axisHeight,double axisWidth,double xStep,double minh,double sOffset,double eOffset,
		  	  	  	   std::vector<double> & _lopx,std::vector<double> & _hipx,
		  	  	  	   std::vector<bool> & hasHole,
		  	  	  	   std::vector<double> & labelX,std::vector<double> & labelY)
  {
	double lopx,lx,ly;

	for (int npx = 0,nX = 0,n0 = 0; (npx < ((int) _lopx.size())); ) {
		// Step until end of data or area is no longer visible
		//
		lopx = _lopx[npx];

		if (lopx > 0) {
			if (nX == 0)
				// First visible time instant
				//
				n0 = npx;

			if (bbCenterLabel && (npx > 0)) {
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

		if (!bbCenterLabel) {
			// Use 1 or 2 elevations in the middle or a elevation as near as possible to the middle of
			// the area in x -direction and having enough height for the label.
			//
			// To favour the center of the area, use the middlest position having at least 70% of
			// the selected position's height.
			//
			// If the 2 middle elevations have holes, use the taller one; by using the center of the
			// elevations without additional checks the label might end up in a hole.
			//
			double sellx = 0.0,selly = 0.0;			// Selected label position
			double sellfvx = 0.0,sellfvy = 0.0;		// Favoured label position
			double selh = 0.0,selfvh = 0.0,h;		// Height of label position
			double fvlim = 0.7;						// Using 70% as the favouring limit
			bool nearest = false;					// Set if the primary position (middle elevation) did not have enough room for the label
			bool multiple = false,fvmul = false;	// Set if using center of 2 middle elevations as label position

			int n = n0 + ((nX - 1) / 2);		// Middle elevation
			int nS = n;							// Selected elevation
			int nN = n0 + nX - 1;				// Last elevation
			int n2 = n + ((nX > 1) ? 1 : 0);	// Right side elevation
			int n1 = n2 - ((nX > 1) ? 1 : 0);	// Left side elevation

			for ( ; (n2 <= nN); ) {
				if (nearest || (nX % 2) || hasHole[n1] || hasHole[n2]) {
					if (nearest || ((nX % 2) == 0)) {
						// Select the taller elevation
						//
						n = (((_lopx[n2] - _hipx[n2]) > (_lopx[n1] - _hipx[n1])) ? n2 : n1);

						n2++;
						n1--;
					}
					else
						n1--;

					double lo = std::min(_lopx[n],axisHeight),hi = std::max(_hipx[n],0.0);

					lx = minx + ((nS = n) * xStep);
					ly = lo - ((lo - hi) / 2);

					h = lo - hi;

					multiple = false;
				}
				else {
					double lo2 = std::min(_lopx[n2],axisHeight),hi2 = std::max(_hipx[n2],0.0);
					double lo1 = std::min(_lopx[n1],axisHeight),hi1 = std::max(_hipx[n1],0.0);

					lx = minx + (n1 * xStep) + (xStep / 2);

					double y2 = lo2 - ((lo2 - hi2) / 2);
					double y1 = lo1 - ((lo1 - hi1) / 2);

					if (y2 > y1) {
						ly = lo1 - ((lo1 - hi2) / 2);
						h = lo2 - hi2;
					}
					else {
						ly = lo2 - ((lo2 - hi1) / 2);
						h = lo1 - hi1;
					}

					n2++;
					n1--;

					multiple = true;
				}

				if ((h > minh) || (h > selh)) {
					selh = h;
					sellx = lx;
					selly = ly;

					if (selh > minh)
						break;

					if (selfvh < (fvlim * selh)) {
						selfvh = selh;
						sellfvx = sellx;
						sellfvy = selly;
						fvmul = multiple;
					}
				}

				nearest = true;
			}

			if ((selh < minh) || (selfvh >= (fvlim * selh))) {
				lx = sellfvx;
				ly = sellfvy;
				multiple = fvmul;
			}
			else {
				lx = sellx;
				ly = selly;
			}

			// To better keep the label within the area adjust the x -position slightly if using area's
			// first or last elevation for labeling.
			//
			// For single elevation areas make adjustment only for document's first and last time instant's elevations.
			// Take the extension offsets (how much the elevation exceeds the begin or end of the x -axis) into
			// account when calculating the label position.

			if ((!multiple) && ((nS == n0) || (nS == nN))) {
				if (nN != n0)
					lx -= (((nS == n0) ? -1 : 1) * (xStep / 5));
				else if (minx < (xStep / 2))
					lx += ((xStep / 5) - sOffset);
				else if (fabs(axisWidth - minx) < (xStep / 2))
					lx -= ((xStep / 5) - eOffset);
			}
		}
		else {
			double xmin = minx + (n0 * xStep);
			double xmax = xmin + ((nX - 1) * xStep);

			lx = (xmax - ((xmax - xmin) / 2));
			ly = (_lopx[0] - ((_lopx[0] - std::max(_hipx[0],0.0)) / 2));
		}

		labelX.push_back(lx);
		labelY.push_back(ly);

		nX = 0;
	}
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Group membership detection based on group number
   */
  // ----------------------------------------------------------------------

  bool CategoryValueMeasureGroup::groupMember(bool first,const woml::CategoryValueMeasure * cvm,const woml::CategoryValueMeasure * cvm2)
  {
	// Store first member of the group or check if group matches

	if (!cvm)
		throw std::runtime_error("CategoryValueMeasureGroup::groupMember: CategoryValueMeasure value expected");
	else if (cvm2) {
		groupMember(true,cvm);

		first = false;
		cvm = cvm2;
	}

	if (first)
		itsFirstMember = cvm;
	else if (cvm->groupNumber() != itsFirstMember->groupNumber())
		return false;

	return true;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Cloud group membership detection
   */
  // ----------------------------------------------------------------------

  CloudGroupCategory::CloudGroupCategory() : CategoryValueMeasureGroup()
  {
  }

  bool CloudGroupCategory::groupMember(const woml::CategoryValueMeasure * cvm) const
  {
	// Check if the given cloud type is known

	if (!cvm)
		throw std::runtime_error("CloudGroupCategory::groupMember: CategoryValueMeasure value expected");

	std::string category(boost::algorithm::to_upper_copy(cvm->category()));

	return (std::find_if(itsCloudGroups.begin(),itsCloudGroups.end(),std::bind2nd(CloudType(),category)) != itsCloudGroups.end());
  }

  bool CloudGroupCategory::groupMember(bool first,const woml::CategoryValueMeasure * cvm,const woml::CategoryValueMeasure * cvm2)
  {
	// Add first member to the cloud group or check if the given cloud type is compatible with the group

	(void) cvm2;

	if (!cvm)
		throw std::runtime_error("CloudGroupCategory::groupMember: CategoryValueMeasure value expected");

	std::string category(boost::algorithm::to_upper_copy(cvm->category()));

	if (first) {
		itcg = std::find_if(itsCloudGroups.begin(),itsCloudGroups.end(),std::bind2nd(CloudType(),category));

		if (itcg == itsCloudGroups.end())
			throw std::runtime_error("CloudGroupCategory::groupMember: unknown category '" + category + "'");

		// The cloud set is common for all groups; clear it

		itcg->cloudSet().clear();

		itsFirstMember = cvm;
	}
	else {
		// Check if elevations overlap (have the same group number) and have matching category
		//
		if (
			(!CategoryValueMeasureGroup::groupMember(false,itsFirstMember,cvm)) ||
			(std::find_if(itsCloudGroups.begin(),itsCloudGroups.end(),std::bind2nd(CloudType(),category)) != itcg)
		   )
			return false;
	}

	// Add contained cloud type into the cloud set

	itcg->addType(category);

	return true;
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
	Phase phase;

	const boost::posix_time::time_period & tp = axisManager->timePeriod();
	boost::posix_time::ptime bt = tp.begin(),et = tp.end();

	for (iteg = egbeg, phase = fst; (iteg != egend); iteg++) {
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

  bool SvgRenderer::scaledCurvePositions(ElevGrp & eGrp,
		  	  	  	  	  	  	  	     List<DirectPosition> & curvePositions,
		  	  	  	  	  	  	  	     std::vector<double> & scaledLo,std::vector<double> & scaledHi,
		  	  	  	  	  	  	  	     std::vector<bool> & hasHole,
		  	  	  	  	  	  	  	     double vOffset,double vSOffset,
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
	Phase phase;
	double lopx = 0.0,plopx = 0.0;
	double hipx = 0.0,phipx = 0.0;
	double maxx = 0.0;
	bool single = true;

	const boost::posix_time::time_period & tp = axisManager->timePeriod();
	boost::posix_time::ptime bt = tp.begin(),et = tp.end();

	double xStep = axisManager->xStep(),nonZ = axisManager->nonZeroElevation();

	srand(time(NULL));

	for (iteg = egbeg, phase = fst; (iteg != egend); ) {
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

		plopx = lopx;
		phipx = hipx;

		// Adjust baseline points to keep decorated curve baseline at the right level.
		//
		// Note: 1/6'th of the decoration control point offset just seems to do the job well enough.

		double loffset = scaleHeightMin,hoffset = scaleHeightMin;

		if (scaleHeightRandom > 0) {
			loffset += ((scaleHeightRandom + 1) * (rand() / (RAND_MAX + 1.0)));
			hoffset += ((scaleHeightRandom + 1) * (rand() / (RAND_MAX + 1.0)));
		}

		lopx = axisManager->scaledElevation(lo) - (loffset / 6);
		hipx = axisManager->scaledElevation(hi) + (hoffset / 6);

		// x -coord of this time instant

		const boost::posix_time::ptime & vt = iteg->validTime();
		double x = axisManager->xOffset(vt);

		// Keep track of elevations to calculate the label position(s)

		trackAreaLabelPos(iteg,x,maxx,lopx,scaledLo,hipx,scaledHi,hasHole);

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
				curvePositions.push_back(DirectPosition(x - xStep,axisManager->axisHeight()));
				single = false;
			}

			curvePositions.push_back(DirectPosition(x,hipx));
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

			curvePositions.push_back(DirectPosition(x - (xStep / 2),ipx));
			curvePositions.push_back(DirectPosition(x,hipx));
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
			curvePositions.push_back(DirectPosition(x + (xStep / 2),phipx - ((phipx - lopx) / 2)));
			curvePositions.push_back(DirectPosition(x,lopx));
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
			curvePositions.push_back(DirectPosition(x - (xStep / 2),hipx - ((hipx - plopx) / 2)));
			curvePositions.push_back(DirectPosition(x,hipx));
			iteg->topConnected(true);

			if (options.debug)
				path << " L" << x << "," << hipx;

			phase = fwd;
		}
		else if (phase == eup) {
			// Move up connecting "nonground" (or non ZeroTolorence) elevation's lo range point to hi range point
			//
#ifdef STEPS
fprintf(stderr,">>>> eup hi=%.0f %s\n",hi,cs.c_str());
#endif
			if (!ground) {
				// Limit the extent of the left side of first time instant
				//
				double offset = (((vt == bt) && (vOffset > sOffset)) ? sOffset : vOffset);

				curvePositions.push_back(DirectPosition(x - offset,lopx - ((lopx - hipx) / 2)));
				curvePositions.push_back(DirectPosition(x,hipx));

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
			if (ground && (vt < et)) {
				curvePositions.push_back(DirectPosition(x + xStep,axisManager->axisHeight()));
				single = false;
			}
			else {
				// Limit the extent of the right side of last time instant.
				//
				double offset = (((vt == et) && (vOffset > eOffset)) ? eOffset : vOffset);

				curvePositions.push_back(DirectPosition(x + offset,lopx - ((lopx - hipx) / 2)));
			}

			curvePositions.push_back(DirectPosition(x,lopx));
			iteg->bottomConnected(true);

			if (options.debug) {
				if (ground && (vt < et))
					path << " L" << (x + xStep) << "," << axisManager->axisHeight();

				path << (((vt != bt) && (vt != et)) ? " L" : " M") << x << "," << lopx;
			}

			phase = bwd;
		}
		else if (phase == bwd) {
			// Move backward thru intermediate point connecting to previous elevation's lo range point
			//
#ifdef STEPS
fprintf(stderr,">>>> bwd lo=%.0f %s\n",lo,cs.c_str());
#endif
			double ipx = ((lopx < plopx) ? lopx : plopx) + (fabs(lopx - plopx) / 2);

			curvePositions.push_back(DirectPosition(x + (xStep / 2),ipx));
			curvePositions.push_back(DirectPosition(x,lopx));
			iteg->bottomConnected(true);

			if (options.debug)
				path << " L" << x << "," << lopx;
		}
		else {	// lst
			if (single) {
				// Bounding box for single elevation. Limit the extent of the left side
				// of first time instant and of the right side of last time instant.
				//
				double leftSide = x - (((vt == bt) && (vSOffset > sOffset)) ? sOffset : vSOffset);
				double rightSide = x + (((vt == et) && (vSOffset > eOffset))? eOffset : vSOffset);

				if (options.debug) {
					path.clear();
					path.str("");

					path << "M" << leftSide << "," << hipx
						 << " L" << rightSide << "," << hipx
						 << " L" << rightSide << "," << lopx
						 << " L" << leftSide << "," << lopx
						 << " L" << leftSide << "," << hipx;
				}

				curvePositions.clear();

				curvePositions.push_back(DirectPosition(leftSide,hipx));
				curvePositions.push_back(DirectPosition(rightSide,hipx));
				curvePositions.push_back(DirectPosition(rightSide,lopx));
				curvePositions.push_back(DirectPosition(leftSide,lopx));
				curvePositions.push_back(DirectPosition(leftSide,hipx));

				break;
			}

			// Close the path
			//
			phase = eup;

			continue;
		}

		if (phase == fwd)
			phase = uprightdown(eGrp,iteg,lo,hi,nonGndFwd2Gnd);
		else
			phase = downleftup(eGrp,iteg,lo,hi,nonGndVdn2Gnd);

		if ((phase == fwd) || (phase == bwd)) {
			// Path covers multiple time instants
			//
			single = false;
		}
	}	// for iteg

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
	// Get bezier curve tightness and cloud groups from configuration
	//
	// Cloud types (their starting positions in the group's 'types' setting) contained in current group;
	// if group's label is not given, label (comma separated list of types) is build using the order
	// the types are defined in 'types' setting
	//
	// The cloud set is cleared and then set by all the groups; cloud types contained are
	// added to the set when collecting a group in elevationGroup()

	const std::string confPath("CloudLayers");

	bool borderCompensation;
	double tightness,minLabelPosHeight;
	CloudGroupCategory cloudGroupCategory;

	std::set<size_t> cloudSet;

	cloudLayerConfig(confPath,tightness,borderCompensation,minLabelPosHeight,cloudGroupCategory.cloudGroups(),cloudSet);

	// Document's time period, cloud layer time series and x -axis step (px)

	const boost::posix_time::time_period & tp = axisManager->timePeriod();
	const std::list<woml::TimeSeriesSlot> & ts = cloudlayers.timeseries();

	boost::posix_time::ptime bt = tp.begin(),et = tp.end();
	double xStep = axisManager->xStep();

	// Elevation group and curve point data

	ElevGrp eGrp;
	int nGroups = 0;

	List<DirectPosition> curvePositions;
	std::list<DirectPosition> curvePoints;
	std::list<doubleArr> decoratorPoints;

	// Set unique group number for members of each overlapping group of elevations

	setGroupNumbers(ts);

	// Search and flag elevation holes

	searchHoles(ts,&cloudGroupCategory);

	for ( ; ; ) {
		// Get group of overlapping elevations from the time serie
		//
		elevationGroup(ts,bt,et,eGrp,false,&cloudGroupCategory);

		if (eGrp.size() == 0)
			break;

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
		// Use cloud decoration offsets if the decoration is compensated when constructing the base line.

		std::vector<double> scaledLo,scaledHi;
		std::vector<bool> hasHole;

		int scaleHeightMin = (borderCompensation ? itcg->scaleHeightMin() : 0);
		int scaleHeightRandom = (borderCompensation ? itcg->scaleHeightRandom() : 0);

		std::ostringstream path;
		path << std::fixed << std::setprecision(3);

		scaledCurvePositions(eGrp,curvePositions,scaledLo,scaledHi,hasHole,itcg->vOffset(),itcg->vSOffset(),itcg->sOffset(),itcg->eOffset(),scaleHeightMin,scaleHeightRandom,path);

		if (options.debug)
			texts[itcg->placeHolder() + "PATH"] << "<path class=\"" << itcg->classDef()
												<< "\" id=\"" << "CloudLayersB" << nGroups
												<< "\" d=\""
												<< path.str()
												<< "\"/>\n";

		// Create bezier curve and get curve and decorator points

		BezierModel bm(curvePositions,false,tightness);
		bm.getSteppedCurvePoints(itcg->baseStep(),itcg->maxRand(),itcg->maxRepeat(),curvePoints);
		bm.decorateCurve(curvePoints,(!isHole),itcg->scaleHeightMin(),itcg->scaleHeightRandom(),itcg->controlMin(),itcg->controlRandom(),decoratorPoints);

		// Render path

		std::list<DirectPosition>::iterator cpbeg = curvePoints.begin(),cpend = curvePoints.end(),itcp;
		std::list<doubleArr>::iterator itdp = decoratorPoints.begin();

		if (options.debug) {
			path.clear();
			path.str("");
		}

		for (itcp = cpbeg; (itcp != cpend); itcp++) {
			if (itcp == cpbeg)
				path << "M";
			else {
				path << " Q" << (*itdp)[0] << "," << (*itdp)[1];
				itdp++;
			}

			path << " " << itcp->getX() << "," << itcp->getY();
		}

		texts[itcg->placeHolder()] << "<path class=\"" << itcg->classDef()
								   << "\" id=\"" << "CloudLayers" << nGroups
								   << "\" d=\""
								   << path.str()
								   << "\"/>\n";

		if (!isHole) {
			std::string label = itcg->label();

			if (!label.empty()) {
				// Render label to the center of each visible part of the cloud
				//
				// x -coord of group's first time instant
				//
				const boost::posix_time::ptime & vt = eGrp.front().validTime();
				double minX = axisManager->xOffset(vt);

				std::vector<double> labelX,labelY;
				std::vector<double>::const_iterator itx,ity;
				int n;

				getAreaLabelPos(itcg->bbCenterLabel(),minX,axisManager->axisHeight(),axisManager->axisWidth(),xStep,minLabelPosHeight,itcg->sOffset(),itcg->eOffset(),
								scaledLo,scaledHi,hasHole,labelX,labelY);

				for (itx = labelX.begin(),ity = labelY.begin(),n = 0; (itx != labelX.end()); itx++, ity++, n++) {
					texts[itcg->labelPlaceHolder()] << "<text class=\"" << itcg->textClassDef()
													<< "\" id=\"" << "CloudLayersText" << nGroups << n
													<< "\" text-anchor=\"middle"
													<< "\" x=\"" << *itx
													<< "\" y=\"" << *ity
													<< "\">" << label << "</text>\n";
				}
			}
		}

		curvePositions.clear();
		curvePoints.clear();
		decoratorPoints.clear();
	}	// for group

	return;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Get group of elevations from a time serie.
   * 		Returns group's type.
   */
  // ----------------------------------------------------------------------

  SvgRenderer::GroupType SvgRenderer::elevationGroup(const std::list<woml::TimeSeriesSlot> & ts,
		  	  	  	  	  	  	  	  	  	  	  	 const boost::posix_time::ptime & bt,const boost::posix_time::ptime & et,
		  	  	  	  	  	  	  	  	  	  	  	 ElevGrp & eGrp,
		  	  	  	  	  	  	  	  	  	  	  	 bool all,
													 CategoryValueMeasureGroup * categoryGroup)
  {
	// Collect group of elevations.
	//
	// If 'all' is true, all elevations are included into the group.
	//
	// Otherwise preset group number (set by setGroupNumbers()) and value category
	// (if given; cloud type or magnitude for icing or turbulence) is taken into account.
	//

	CategoryValueMeasureGroup overlappingGroup;

	std::list<woml::TimeSeriesSlot>::const_iterator tsbeg = ts.begin();
	std::list<woml::TimeSeriesSlot>::const_iterator tsend = ts.end();
	std::list<woml::TimeSeriesSlot>::const_iterator itts;

	bool byCategory = (categoryGroup != NULL),groundGrp = false,nonGroundGrp = false,hole = false;
	double nonZ = axisManager->nonZeroElevation();

	eGrp.clear();

	for (itts = tsbeg; (itts != tsend); itts++) {
		// Skip time instants outside the given time range
		//
		const boost::posix_time::ptime & vt = itts->validTime();

		if ((vt < bt) || (vt > et)) {
			if (vt < bt)
				continue;
			else
				break;
		}

		// Loop thru the parameter sets and their parameters

		std::list<boost::shared_ptr<woml::GeophysicalParameterValueSet> >::const_iterator pvsend = itts->values().end(),itpvs;

		for (itpvs = itts->values().begin(); (itpvs != pvsend); itpvs++) {
			std::list<woml::GeophysicalParameterValue>::iterator pvend = itpvs->get()->editableValues().end(),itpv;

			for (itpv = itpvs->get()->editableValues().begin(); (itpv != pvend); itpv++) {
				// Not mixing holes and nonholes
				//
				if ((eGrp.size() > 0) && (hole != ((itpv->getFlags() & ElevationGroupItem::b_isHole) ? true : false)))
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

				if (eGrp.size() == 0) {
					groundGrp = ground;
					hole = ((itpv->getFlags() & ElevationGroupItem::b_isHole) ? true : false);
				}
				else if (!ground)
					nonGroundGrp = true;

				eGrp.push_back(ElevationGroupItem(vt,itpvs,itpv));

				if (byCategory && categoryGroup->standalone())
					return (groundGrp ? t_ground : t_nonground);
			}	// for itpv
		}	// for itpvs
	}	// for itts

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
				// Check that elevation belongs to the same category (cloud type or icing magnitude) as the hole
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
				// Check that the holes belong to the same category (cloud type or icing magnitude)
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
				// Check that elevation belongs to the same category (cloud type or icing magnitude) as the hole
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
				// Check that the holes belong to the same category (cloud type or icing magnitude)
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
				// Check that the holes belong to the same category (cloud type or icing magnitude)
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
				// Check that the holes belong to the same category (cloud type or icing magnitude)
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

  void SvgRenderer::checkHoles(ElevGrp & eGrp,ElevationHoles & holes,CategoryValueMeasureGroup * groupCategory,bool setNegative)
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

	bool reScan;

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
		holeElev->isHole(true);

		if (setNegative)
			holeElev->isNegative(true);
	}
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

  void SvgRenderer::searchHoles(const std::list<woml::TimeSeriesSlot> & ts,CategoryValueMeasureGroup * groupCategory,bool setNegative)
  {
	// Get all elevations

	const boost::posix_time::time_period & tp = axisManager->timePeriod();
	boost::posix_time::ptime bt = tp.begin(),et = tp.end();

	ElevGrp eGrp;
	elevationGroup(ts,bt,et,eGrp);

	// Search for holes.

	ElevGrp::iterator egbeg = eGrp.begin(),egend = eGrp.end();
	ElevGrp::iterator iteg,piteg;

	if (groupCategory) {
		// For categorized data the elevations are scanned one category (cloud type or icing magnitude) at a time.
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
			elevationGroup(ts,bt,et,eGrp);

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
			if (iteg->validTime() == tp.begin())
				continue;
			else if (iteg->validTime() == tp.end())
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
					// Check first that above and below elevation belong to the same category (cloud type or icing magnitude).
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

	checkHoles(eGrp,holes,groupCategory,setNegative);
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
		double xStep = axisManager->xStep();
		const std::string CONTRAILS(boost::algorithm::to_upper_copy(confPath));
		bool visible = false,aboveTop = false;
		int nGroups = 0;

		const char * typeMsg = " must contain a group in curly brackets";

		// Document's time period

		const boost::posix_time::time_period & tp = axisManager->timePeriod();
		boost::posix_time::ptime bt = tp.begin(),et = tp.end();

		const std::list<woml::TimeSeriesSlot> & ts = contrails.timeseries();

		// Class

		const libconfig::Setting & specs = config.lookup(confPath);
		if(!specs.isGroup())
			throw std::runtime_error(confPath + typeMsg);

		const std::string classDef = configValue<std::string>(specs,confPath,"class");

		// Set unique group number for members of each overlapping group of elevations.
		// Do not mix ground and nonground elevations.

		setGroupNumbers(ts,false);

		for ( ; ; ) {
			// Get group of overlapping elevations from the time serie
			//
			elevationGroup(ts,bt,et,eGrp,false);

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
				size_t visibleCnt = 0;
				bool segmentVisible = false;

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

					if (((val <= 0) || prevMissing) && (visibleCnt > 0)) {
						// Continue ending path segment half timestep forwards.
						// Connect the hi range point to the lo range point.
						//
						path << " L" << (prevX + (xStep / 2)) << "," << prevVal;

						if (rng == 1) {
							const woml::Elevation & e = prevIteg->Pv()->elevation();
							boost::optional<woml::NumericalSingleValueMeasure> itsBoundedLo = (e.bounded() ? e.lowerLimit() : woml::NumericalSingleValueMeasure());
							const boost::optional<woml::NumericalSingleValueMeasure> & itsLoLimit = (e.bounded() ? itsBoundedLo : e.value());

							lo = ((!itsLoLimit) ? 0.0 : itsLoLimit->numericValue());

							path << " L" << (prevX + (xStep / 2)) << "," << axisManager->scaledElevation(lo);
						}
					}

					if (val > 0) {
						// Visible
						//
						if (prevMissing) {
							// Start new path segment half timestep backwards.
							// Connect the lo range point to the hi range point.
							//
							path << " M" << (x - (xStep / 2)) << "," << val;

							if (rng == 0)
								path << " L" << (x - (xStep / 2)) << "," << axisManager->scaledElevation(hi)
									 << " M" << (x - (xStep / 2)) << "," << val;

							visibleCnt++;
						}

						path << ((visibleCnt == 0) ? "M" : " L") << x << "," << val;
						visibleCnt++;
					}
					else {
						// Missing or above the top
						//
						if (above)
							aboveTop = true;

						if (visibleCnt > 0)
							segmentVisible = true;

						visibleCnt = 0;
					}

					prevX = x;
					prevVal = val;

					iteg++;

					if ((iteg == egend) && ((visibleCnt > 0) || segmentVisible)) {
						// At least part of the curve is visible (scaled elevation(s) above zero and below the top of y -axis did exist)
						//
						if ((visibleCnt > 0) && (eGrp.back().validTime() < et)) {
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

						visible = true;
					}
				}	// for iteg
			}	// for rng

			// Remove group's elevations from the collection

			for (iteg = eGrp.begin(); (iteg != eGrp.end()); iteg++)
				iteg->Pvs()->get()->editableValues().erase(iteg->Pv());
		}	// for group

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
   * \brief Loads icing configuration
   */
  // ----------------------------------------------------------------------

  const libconfig::Setting & SvgRenderer::icingConfig(const std::string & _confPath,
		  	  	  	  	  	  	  	  	  	  	  	  double & tightness,double & minLabelPosHeight,
		  	  	  	  	  	  	  	  	  	  	  	  std::list<IcingGroup> & icingGroups,
		  	  	  	  	  	  	  	  	  	  	  	  std::set<size_t> & icingSet)
  {
	std::string confPath(_confPath);

	try {
		const char * grouptypeMsg = " must contain a group in curly brackets";
		const char * listtypeMsg = " must contain a list of groups in parenthesis";

		std::string ICING(boost::algorithm::to_upper_copy(confPath));

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

		// Minimum height (px) for the symbol (or label) position

		minLabelPosHeight = (double) configValue<double,int>(specs,confPath,"labelheight",NULL,s_optional,&isSet);

		if (!isSet)
			minLabelPosHeight = 40.0;

		// If bbcenterlabel is true (default: false), icing symbol is positioned to the center of the area's bounding box;
		// otherwise using 1 or 2 elevations in the middle of the area in x -direction

		bool bbCenterLabel = configValue<bool>(specs,confPath,"bbcenterlabel",NULL,s_optional,&isSet);
		if (!isSet)
			bbCenterLabel = false;

		// Icing groups

		confPath += ".groups";

		const libconfig::Setting & groupSpecs = config.lookup(confPath);
		if(!groupSpecs.isList())
			throw std::runtime_error(confPath + listtypeMsg);

		for(int i=0, globalsIdx = -1; i<groupSpecs.getLength(); i++)
		{
			const libconfig::Setting & group = groupSpecs[i];
			if(!group.isGroup())
				throw std::runtime_error(confPath + listtypeMsg);

			// Icing magnitudes

			std::string icingTypes(boost::algorithm::to_upper_copy(boost::algorithm::trim_copy(configValue<std::string>(group,confPath,"types",NULL,s_optional))));

			if (!icingTypes.empty()) {
				std::vector<std::string> it;
				boost::algorithm::split(it,icingTypes,boost::is_any_of(","));

				if (it.size() > 1) {
					std::ostringstream its;

					for (unsigned int i = 0,n = 0; (i < it.size()); i++) {
						boost::algorithm::trim(it[i]);

						if (!it[i].empty()) {
							its << (n ? "," : "") << it[i];
							n++;
						}
					}

					icingTypes = its.str();
				}
			}

			if (icingTypes.empty()) {
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

			// Relative offset for intermediate curve points (controls how much the ends of the area
			// are extended horizontally)

			double xStep = axisManager->xStep();

			double vOffset = configValue<double>(group,confPath,"voffset",globalScope,s_optional,&isSet);
			if (!isSet)
				vOffset = 0.0;
			else
				vOffset *= xStep;

			// Relative offset for intermediate curve point for single elevation (controls how much the
			// ends of the area are extended horizontally)

			double vSOffset = configValue<double>(group,confPath,"vsoffset",globalScope,s_optional,&isSet);
			if (!isSet)
				vSOffset = xStep / 3;
			else
				vSOffset *= xStep;

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

			// Output label; if symbol is not defined, default label is the icing magnitudes concatenated with ',' as separator.
			// If empty label is given, no label.

			std::string label(boost::algorithm::trim_copy(configValue<std::string>(group,confPath,"label",NULL,s_optional,&isSet)));
			bool hasLabel = isSet;

			// Output placeholders; default label placeholder is the icing placeholder + "TEXT"

			std::string placeHolder(boost::algorithm::trim_copy(configValue<std::string>(group,confPath,"output",globalScope,s_optional)));
			if (placeHolder.empty())
				placeHolder = ICING;

			std::string labelPlaceHolder(boost::algorithm::trim_copy(configValue<std::string>(group,confPath,"labeloutput",globalScope,s_optional)));
			if (labelPlaceHolder.empty())
				labelPlaceHolder = placeHolder + "TEXT";

			// Standalone icing magnitudes (only one magnitude in the group) have flag controlling whether the
			// group/icing is combined or not

			bool combined = configValue<bool>(group,confPath,"combined",globalScope,s_optional,&isSet);
			if (!isSet)
				combined = true;

			icingGroups.push_back(IcingGroup(classDef,
											 icingTypes,
											 (!symbol.empty()) ? &symbol : NULL,
											 symbolOnly,
											 hasLabel ? &label : NULL,
											 bbCenterLabel,
											 placeHolder,
											 labelPlaceHolder,
											 combined,
											 vOffset,vSOffset,
											 sOffset,eOffset,
											 icingSet,
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
   * \brief Icing category membership detection
   */
  // ----------------------------------------------------------------------

  IcingGroupCategory::IcingGroupCategory() : CategoryValueMeasureGroup()
  {
  }

  bool IcingGroupCategory::groupMember(const woml::CategoryValueMeasure * cvm) const
  {
	// Check if the given icing magnitude is known

	if (!cvm)
		throw std::runtime_error("IcingGroupCategory::groupMember: CategoryValueMeasure value expected");

	std::string category(boost::algorithm::to_upper_copy(cvm->category()));

	return (std::find_if(itsIcingGroups.begin(),itsIcingGroups.end(),std::bind2nd(IcingType(),category)) != itsIcingGroups.end());
  }

  bool IcingGroupCategory::groupMember(bool first,const woml::CategoryValueMeasure * cvm,const woml::CategoryValueMeasure * cvm2)
  {
	// Add first member to the icing group or check if the given icing magnitude is compatible with the group

	(void) cvm2;

	if (!cvm)
		throw std::runtime_error("IcingGroupCategory::groupMember: CategoryValueMeasure value expected");

	std::string category(boost::algorithm::to_upper_copy(cvm->category()));

	if (first) {
		itig = std::find_if(itsIcingGroups.begin(),itsIcingGroups.end(),std::bind2nd(IcingType(),category));

		if (itig == itsIcingGroups.end())
			throw std::runtime_error("IcingGroupCategory::groupMember: unknown category '" + category + "'");

		// The icing set is common for all groups; clear it

		itig->icingSet().clear();

		itsFirstMember = cvm;
	}
	else {
		// Check if elevations overlap (have the same group number) and have matching category
		//
		if (
			(!CategoryValueMeasureGroup::groupMember(false,itsFirstMember,cvm)) ||
			(std::find_if(itsIcingGroups.begin(),itsIcingGroups.end(),std::bind2nd(IcingType(),category)) != itig)
		   )
			return false;
	}

	// Add contained icing magnitude into the icing set

	itig->addType(category);

	return true;
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Icing rendering
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::render_timeserie(const woml::Icing & icing)
  {
	const std::string confPath("Icing");
	const std::string ICING(boost::algorithm::to_upper_copy(confPath));

	// Get icing groups from configuration.
	//
	// icingSet contains the icing magnitudes (their starting positions in the group's 'types' setting) in current group;
	// if neither group's symbol nor label are given, label (comma separated list of types) is build using the order
	// the types are defined in 'types' setting.
	//
	// The icing set is cleared and then set by all the groups; icing magnitudes contained are
	// added to the set when collecting a group in elevationGroup()

	IcingGroupCategory icingGroupCategory;
	std::set<size_t> icingSet;

	double tightness,minLabelPosHeight;

	icingConfig(confPath,tightness,minLabelPosHeight,icingGroupCategory.icingGroups(),icingSet);

	// Document's time period, icing time series and x -axis step (px)

	const boost::posix_time::time_period & tp = axisManager->timePeriod();
	const std::list<woml::TimeSeriesSlot> & ts = icing.timeseries();

	boost::posix_time::ptime bt = tp.begin(),et = tp.end();
	double xStep = axisManager->xStep();

	const std::string symClass(boost::algorithm::to_upper_copy(confPath + icing.classNameExt()));

	// Elevation group and curve point data

	ElevGrp eGrp;
	int nGroups = 0;

	List<DirectPosition> curvePositions;
	std::list<DirectPosition> curvePoints;

	// Set unique group number for members of each overlapping group of elevations

	setGroupNumbers(ts);

	// Search and flag elevation holes

	searchHoles(ts,&icingGroupCategory);

	for ( ; ; ) {
		// Get group of overlapping elevations from the time serie
		//
		elevationGroup(ts,bt,et,eGrp,false,&icingGroupCategory);

		if (eGrp.size() == 0)
			break;

		nGroups++;

		// Icing group for current elevation group

		std::list<IcingGroup>::const_iterator itig = icingGroupCategory.currentGroup();

		// Get curve positions for bezier creation.
		//
		// The scaled lo/hi values for the elevations are returned in scaledLo/scaledHi;
		// they are used to calculate icing symbol (or label) positions

		std::vector<double> scaledLo,scaledHi;
		std::vector<bool> hasHole;
		bool isHole = eGrp.front().isHole();

		std::ostringstream path;
		path << std::fixed << std::setprecision(3);

		bool single = scaledCurvePositions(eGrp,curvePositions,scaledLo,scaledHi,hasHole,itig->vOffset(),itig->vSOffset(),itig->sOffset(),itig->eOffset(),0,0,path);

		if (!(single && itig->symbolOnly())) {
			// Rendering the group as bezier curve
			//
			if (options.debug)
				texts[ICING + "PATH"] << "<path class=\"" << itig->classDef()
									  << "\" id=\"" << "IcingB" << nGroups
									  << "\" d=\""
									  << path.str()
									  << "\"/>\n";

			// Create bezier curve and get curve points

			BezierModel bm(curvePositions,false,tightness);
			bm.getSteppedCurvePoints(0,0,0,curvePoints);

			// Render path

			std::list<DirectPosition>::iterator cpbeg = curvePoints.begin(),cpend = curvePoints.end(),itcp;

			if (options.debug) {
				path.clear();
				path.str("");
			}

			for (itcp = cpbeg; (itcp != cpend); itcp++)
				path << ((itcp == cpbeg) ? "M" : " L") << itcp->getX() << "," << itcp->getY();

			texts[ICING] << "<path class=\"" << itig->classDef()
						 << "\" id=\"" << "Icing" << nGroups
						 << "\" d=\""
						 << path.str()
						 << "\"/>\n";

			curvePoints.clear();
		}

		curvePositions.clear();

		// Render the single symbol or one symbol or label to the center of each visible part of the area.

		if (!isHole) {
			// x -coord of group's first time instant
			//
			const boost::posix_time::ptime & vt = eGrp.front().validTime();
			double minX = axisManager->xOffset(vt);
			int n;

			std::vector<double> labelX,labelY;
			std::vector<double>::const_iterator itx,ity;

			getAreaLabelPos(itig->bbCenterLabel(),minX,axisManager->axisHeight(),axisManager->axisWidth(),xStep,minLabelPosHeight,itig->sOffset(),itig->eOffset(),
							scaledLo,scaledHi,hasHole,labelX,labelY);

			for (itx = labelX.begin(),ity = labelY.begin(), n = 0; (itx != labelX.end()); itx++, ity++, n++)
				if (!(itig->symbol().empty()))
					render_aerodromeSymbol(confPath,symClass,icing.classNameExt(),itig->symbol(),*itx,*ity,true,true);
				else
					texts[itig->labelPlaceHolder()] << "<text class=\"" << itig->textClassDef()
													<< "\" id=\"" << "IcingText" << nGroups << n
													<< "\" text-anchor=\"middle"
													<< "\" x=\"" << *itx
													<< "\" y=\"" << *ity
													<< "\">" << itig->label() << "</text>\n";
		}
	}	// for group
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Turbulence rendering
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::render_timeserie(const woml::Turbulence & turbulence)
  {
	const std::string confPath("Turbulence");
	const std::string TURBULENCE(boost::algorithm::to_upper_copy(confPath));

	// Get icing groups from configuration.
	//
	// icingSet contains the icing magnitudes (their starting positions in the group's 'types' setting) in current group;
	// if neither group's symbol nor label are given, label (comma separated list of types) is build using the order
	// the types are defined in 'types' setting.
	//
	// The icing set is cleared and then set by all the groups; icing magnitudes contained are
	// added to the set when collecting a group in elevationGroup()

	IcingGroupCategory icingGroupCategory;
	std::set<size_t> icingSet;

	double tightness,minLabelPosHeight;

	icingConfig(confPath,tightness,minLabelPosHeight,icingGroupCategory.icingGroups(),icingSet);

	// Document's time period, icing time series and x -axis step (px)

	const boost::posix_time::time_period & tp = axisManager->timePeriod();
	const std::list<woml::TimeSeriesSlot> & ts = turbulence.timeseries();

	boost::posix_time::ptime bt = tp.begin(),et = tp.end();
	double xStep = axisManager->xStep();

	const std::string symClass(boost::algorithm::to_upper_copy(confPath + turbulence.classNameExt()));

	// Elevation group and curve point data

	ElevGrp eGrp;
	int nGroups = 0;

	List<DirectPosition> curvePositions;
	std::list<DirectPosition> curvePoints;

	// Set unique group number for members of each overlapping group of elevations

	setGroupNumbers(ts);

	// Search and flag elevation holes

	searchHoles(ts,&icingGroupCategory);

	for ( ; ; ) {
		// Get group of overlapping elevations from the time serie
		//
		elevationGroup(ts,bt,et,eGrp,false,&icingGroupCategory);

		if (eGrp.size() == 0)
			break;

		nGroups++;

		// Icing group for current elevation group

		std::list<IcingGroup>::const_iterator itig = icingGroupCategory.currentGroup();

		// Get curve positions for bezier creation.
		//
		// The scaled lo/hi values for the elevations are returned in scaledLo/scaledHi;
		// they are used to calculate icing symbol (or label) positions

		std::vector<double> scaledLo,scaledHi;
		std::vector<bool> hasHole;
		bool isHole = eGrp.front().isHole();

		std::ostringstream path;
		path << std::fixed << std::setprecision(3);

		bool single = scaledCurvePositions(eGrp,curvePositions,scaledLo,scaledHi,hasHole,itig->vOffset(),itig->vSOffset(),itig->sOffset(),itig->eOffset(),0,0,path);

		if (!(single && itig->symbolOnly())) {
			// Rendering the group as bezier curve
			//
			if (options.debug)
				texts[TURBULENCE + "PATH"] << "<path class=\"" << itig->classDef()
										   << "\" id=\"" << "TurbulenceB" << nGroups
										   << "\" d=\""
										   << path.str()
										   << "\"/>\n";

			// Create bezier curve and get curve points

			BezierModel bm(curvePositions,false,tightness);
			bm.getSteppedCurvePoints(0,0,0,curvePoints);

			// Render path

			std::list<DirectPosition>::iterator cpbeg = curvePoints.begin(),cpend = curvePoints.end(),itcp;

			if (options.debug) {
				path.clear();
				path.str("");
			}

			for (itcp = cpbeg; (itcp != cpend); itcp++)
				path << ((itcp == cpbeg) ? "M" : " L") << itcp->getX() << "," << itcp->getY();

			texts[TURBULENCE] << "<path class=\"" << itig->classDef()
							  << "\" id=\"" << "Turbulence" << nGroups
							  << "\" d=\""
							  << path.str()
							  << "\"/>\n";

			curvePoints.clear();
		}

		curvePositions.clear();

		// Render the single symbol or one symbol or label to the center of each visible part of the area.

		if (!isHole) {
			// x -coord of group's first time instant
			//
			const boost::posix_time::ptime & vt = eGrp.front().validTime();
			double minX = axisManager->xOffset(vt);
			int n;

			std::vector<double> labelX,labelY;
			std::vector<double>::const_iterator itx,ity;

			getAreaLabelPos(itig->bbCenterLabel(),minX,axisManager->axisHeight(),axisManager->axisWidth(),xStep,minLabelPosHeight,itig->sOffset(),itig->eOffset(),
							scaledLo,scaledHi,hasHole,labelX,labelY);

			for (itx = labelX.begin(),ity = labelY.begin(), n = 0; (itx != labelX.end()); itx++, ity++, n++)
				if (!(itig->symbol().empty()))
					render_aerodromeSymbol(confPath,symClass,turbulence.classNameExt(),itig->symbol(),*itx,*ity,true,true);
				else
					texts[itig->labelPlaceHolder()] << "<text class=\"" << itig->textClassDef()
													<< "\" id=\"" << "IcingText" << nGroups << n
													<< "\" text-anchor=\"middle"
													<< "\" x=\"" << *itx
													<< "\" y=\"" << *ity
													<< "\">" << itig->label() << "</text>\n";
		}
	}	// for group
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Migratory birds rendering
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::render_timeserie(const woml::MigratoryBirds & migratorybirds)
  {
	render_aerodromeSymbols<woml::MigratoryBirds>(migratorybirds,"MigratoryBirds");
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

		// Wind arrow arm length (in pixels) to calculate horizontal and vertical offsets to position the arrowheads to
		// the elevation lines. Without adjustment the wind arrows are (assumed to be) centered to the elevation lines.

		unsigned int arrowArmLength = configValue<unsigned int>(specs,confPath,"arrowarmlength",NULL,s_optional,&isSet);
		if (!isSet)
			arrowArmLength = 0;

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
   * \brief Set unique group number for members of each overlapping group of elevations
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::setGroupNumbers(const std::list<woml::TimeSeriesSlot> & ts,bool mixed)
  {
	unsigned int nextGroupNumber = 1;

	const boost::posix_time::time_period & tp = axisManager->timePeriod();
	boost::posix_time::ptime bt = tp.begin(),et = tp.end();

	// Get all elevations

	ElevGrp eGrp;
	elevationGroup(ts,bt,et,eGrp);
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

		// Document's time period, zero tolerance time series and x -axis step (px)

		const boost::posix_time::time_period & tp = axisManager->timePeriod();
		const std::list<woml::TimeSeriesSlot> & ts = zerotolerance.timeseries();

		boost::posix_time::ptime bt = tp.begin(),et = tp.end();
		double xStep = axisManager->xStep();

		// Bezier curve tightness

		double tightness = (double) configValue<double,int>(specs,confPath,"tightness",NULL,s_optional,&isSet);

		if (!isSet)
			// Using the default by setting a negative value
			//
			tightness = -1.0;

		// Relative offset for intermediate curve points (controls how much the ends of the area
		// are extended horizontally)

		double vOffset = configValue<double>(specs,confPath,"voffset",NULL,s_optional,&isSet);
		if (!isSet)
			vOffset = 0.0;
		else
			vOffset *= xStep;

		// Relative offset for intermediate curve point for single elevation (controls how much the
		// ends of the area are extended horizontally)

		double vSOffset = configValue<double>(specs,confPath,"vsoffset",NULL,s_optional,&isSet);
		if (!isSet)
			vSOffset = xStep / 3;
		else
			vSOffset *= xStep;

		// Max offset in px for extending first time instant's elevations to the left

		int sOffset = configValue<int>(specs,confPath,"soffset",NULL,s_optional,&isSet);
		if (!isSet)
			sOffset = 0;

		// Max offset in px for extending last time instant's elevations to the right

		int eOffset = configValue<int>(specs,confPath,"eoffset",NULL,s_optional,&isSet);
		if (!isSet)
			eOffset = 0;

		// Minimum height (px) for the label position

		double minLabelPosHeight = (double) configValue<double,int>(specs,confPath,"labelheight",NULL,s_optional,&isSet);

		if (!isSet)
			minLabelPosHeight = 50.0;

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

		searchHoles(ts);

		for ( ; ; ) {
			// Get group of overlapping elevations from the time serie
			//
			elevationGroup(ts,bt,et,eGrpAll,false);

			if (eGrpAll.size() == 0)
				break;

			nGroups++;

			// If a time instant does not have elevation with lo -range <= (or near) 0, below 0 elevation is generated
			// forcing the curve path to go to the ground

			if (!(eGrpAll.front().isHole()))
				checkZeroToleranceGroup(eGrpAll,eGrpGen);
			else
				eGrp = eGrpAll;

			std::string & label = (eGrp.begin()->isNegative() ? negLabel : posLabel);

			// Get curve positions for overlapping elevations for bezier creation.
			//
			// The scaled lo/hi values for the elevations are returned in scaledLo/scaledHi;
			// they are used to calculate area label positions

			std::vector<double> scaledLo,scaledHi;
			std::vector<bool> hasHole;

			std::ostringstream path;
			path << std::fixed << std::setprecision(3);

			scaledCurvePositions(eGrp,curvePositions,scaledLo,scaledHi,hasHole,vOffset,vSOffset,sOffset,eOffset,0,0,path,&visible,true,false,true);

			// Create bezier curve and get curve points

			if (options.debug)
				texts[ZEROTOLERANCE + "PATH"] << "<path class=\"" << classDef
											  << "\" id=\"" << "ZeroToleranceB" << nGroups
											  << "\" d=\""
											  << path.str()
											  << "\"/>\n";

//printf("> Cpos:\n"); {
//List<DirectPosition>::iterator cpbeg = curvePositions.begin(),cpend = curvePositions.end(),itcp;
//for (itcp = cpbeg; (itcp != cpend); itcp++) printf("> Cpos x=%.1f, y=%.1f\n",itcp->getX(),itcp->getY());
//}
			BezierModel bm(curvePositions,false,tightness);
			bm.getSteppedCurvePoints(10,0,0,curvePoints);

			// Output the path

			path.clear();
			path.str("");
			std::list<DirectPosition>::iterator cpbeg = curvePoints.begin(),cpend = curvePoints.end(),itcp;

//printf("> Bez:\n");
//for (itcp = cpbeg; (itcp != cpend); itcp++) printf("> Bez x=%.1f, y=%.1f\n",itcp->getX(),itcp->getY());
			for (itcp = cpbeg; (itcp != cpend); itcp++)
				path << ((itcp == cpbeg) ? "M" : " L") << itcp->getX() << "," << itcp->getY();

			texts[ZEROTOLERANCE] << "<path class=\"" << classDef
								 << "\" id=\"" << "ZeroTolerance" << nGroups
								 << "\" d=\""
								 << path.str()
								 << "\"/>\n";

			curvePositions.clear();
			curvePoints.clear();

			if (visible && setUnvisible) {
				// The curve is visible (elevation(s) above zero did exist); hide the fixed template text "Zerolevel is null"
				//
				texts[ZEROTOLERANCEVISIBLE] << classDef << "Unvisible";
				setUnvisible = false;
			}

			// Render label to the center of each visible part of the area
			//
			if (!label.empty()) {
				// x -coord of group's first time instant
				//
				const boost::posix_time::ptime & vt = eGrp.front().validTime();
				double minX = axisManager->xOffset(vt);

				std::vector<double> labelX,labelY;
				std::vector<double>::const_iterator itx,ity;
				int n;

				getAreaLabelPos(bbCenterLabel,minX,axisManager->axisHeight(),axisManager->axisWidth(),xStep,minLabelPosHeight,sOffset,eOffset,
								scaledLo,scaledHi,hasHole,labelX,labelY);

				for (itx = labelX.begin(),ity = labelY.begin(),n = 0; (itx != labelX.end()); itx++, n++)
					texts[labelPlaceHolder] << "<text class=\"" << textClassDef
											<< "\" id=\"" << "ZeroToleranceText" << nGroups << n
											<< "\" text-anchor=\"middle"
											<< "\" x=\"" << *itx
											<< "\" y=\"" << *ity
											<< "\">" << label << "</text>\n";
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
		  	  	  	  	  	  	 const std::string & valClass,
		  	  	  	  	  	  	 const woml::NumericalSingleValueMeasure * lowerLimit,
		  	  	  	  	  	  	 const woml::NumericalSingleValueMeasure * upperLimit,
		  	  	  	  	  	  	 double lon,
		  	  	  	  	  	  	 double lat)
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

					// Value type; value (the default) or svg
					std::string type = configValue<std::string>(specs,valClass,"type",globalScope,s_optional);

					if (type.empty() || (type == "value")) {
						// Class, format and reference for background class
						//
						std::string vtype(upperLimit ? "Range" : "");

						std::string classDef(configValue<std::string>(specs,valClass,"class" + vtype,globalScope,s_optional));
						if (classDef.empty())
							classDef = (vtype.empty() ? valClass : (valClass + " " + valClass + vtype));

						std::string pref(configValue<std::string>(specs,valClass,"pref" + vtype,globalScope,s_optional));
						std::string href(configValue<std::string>(specs,valClass,"href" + vtype,globalScope,s_optional));

						// For single value, search for matching condition with nearest comparison value

						const libconfig::Setting * condSpecs = (! upperLimit)
							? matchingCondition(config,confPath,valClass,"",i,lowerLimit->numericValue())
							: NULL;

						if (condSpecs) {
							// Class from the condition or from the parent/parameter block
							//
							classDef = configValue<std::string>(*condSpecs,valClass,"class",&specs);

							bool isSet;
							std::string cpref = configValue<std::string>(*condSpecs,valClass,"pref" + vtype,NULL,s_optional,&isSet);
							if (isSet)
								pref = cpref;

							std::string chref = configValue<std::string>(*condSpecs,valClass,"href" + vtype,NULL,s_optional,&isSet);
							if (isSet)
								href = chref;
						}

						pointvalues << "<g class=\""
									<< classDef
									<< "\">\n";

						if (href.empty())
							pointvalues << "<text id=\"text" << boost::lexical_cast<std::string>(npointvalues)
										<< "\" text-anchor=\"middle\" x=\"" << std::fixed << std::setprecision(1) << lon
										<< "\" y=\"" << std::fixed << std::setprecision(1) << lat
										<< "\">" << formattedValue(lowerLimit,upperLimit,confPath,pref) << "</text>\n";
						else
							pointvalues << "<g transform=\"translate("
										<< std::fixed << std::setprecision(1) << lon << " "
										<< std::fixed << std::setprecision(1) << lat << ")\">\n"
										<< "<use xlink:href=\"#" << href << "\"/>\n"
										<< "<text id=\"text" << boost::lexical_cast<std::string>(npointvalues)
										<< "\" text-anchor=\"middle\">" << formattedValue(lowerLimit,upperLimit,confPath,pref) << "</text>\n"
										<< "</g>\n";

						pointvalues << "</g>\n";

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
  render_header("regionName",boost::posix_time::ptime(),regionName);
  render_header("regionId",boost::posix_time::ptime(),regionId);

  // Creator
  render_header("creator",boost::posix_time::ptime(),creator);

  // Feature validtime
  render_header("validTime",validTime);

  // Analysis/forecast time period date, start time and end time
  render_header("timePeriodDate",timePeriod.begin());
  render_header("timePeriodStartTime",timePeriod.begin());
  render_header("timePeriodEndTime",timePeriod.end());

  // Creation time date and time
  render_header("creationTimeDate",creationTime);
  render_header("creationTimeTime",creationTime);

  // Analysis/forecast date and time
  render_header("forecastTimeDate",forecastTime);
  render_header("forecastTimeTime",forecastTime);

  // Modification date and time
  render_header("modificationTimeDate",modificationTime ? *modificationTime : boost::posix_time::ptime());
  render_header("modificationTimeTime",modificationTime ? *modificationTime : boost::posix_time::ptime());

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

#ifdef __contouring__

  for(Contours::const_iterator it=contours.begin(); it!=contours.end(); ++it)
	replace_all(ret,it->first, it->second->str());

#endif

  for(Texts::const_iterator it=texts.begin(); it!=texts.end(); ++it)
	replace_all(ret,"--" + it->first + "--", it->second->str());

  // Clear all remaining placeholders

  boost::regex pl("--[[:upper:]*[:lower:]*[:digit:]]*--");

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
		return 0.0;
	else if (validTime > itsTimePeriod.end())
		return itsAxisWidth;

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
					   double theVOffset,double theVSOffset,
					   int theSOffset,int theEOffset,
					   std::set<size_t> & theCloudSet,
					   const libconfig::Setting * theLocalScope,
					   const libconfig::Setting * theGlobalScope)
  : itsCloudSet(theCloudSet)
{
  itsClass = boost::algorithm::trim_copy(theClass);

  if (!itsClass.empty()) {
	// Class names for cloud type output
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

  itsStandalone = (((theCloudTypes.find(",") == std::string::npos)) && (!combined));
  itsCloudTypes = (itsStandalone ? theCloudTypes : ("," + theCloudTypes + ","));
  itsSymbolType = (itsStandalone ? theSymbolType : "");
  itsLabel = theLabel ? *theLabel : "";
  hasLabel = theLabel ? true : false;
  bbCenterLabelPos = bbCenterLabel;
  itsPlaceHolder = thePlaceHolder;
  itsLabelPlaceHolder = theLabelPlaceHolder;

  itsBaseStep = theBaseStep;
  itsMaxRand = theMaxRand;
  itsMaxRepeat = theMaxRepeat;

  itsScaleHeightMin = theScaleHeightMin;
  itsScaleHeightRandom = theScaleHeightRandom;
  itsControlMin = theControlMin;
  itsControlRandom = theControlRandom;

  itsVOffset = theVOffset;
  itsVSOffset = theVSOffset;
  itsSOffset = theSOffset;
  itsEOffset = theEOffset;

  itsLocalScope = theLocalScope;
  itsGlobalScope = theGlobalScope;
}

// ----------------------------------------------------------------------
/*!
 * \brief Check if group contains given cloud type
 */
// ----------------------------------------------------------------------

bool CloudGroup::contains(const std::string & theCloudType) const
{
  if (itsStandalone)
	  return (itsCloudTypes == theCloudType);
  else
	  return (itsCloudTypes.find("," + theCloudType + ",") != std::string::npos);
}

// ----------------------------------------------------------------------
/*!
 * \brief Add a cloud type the cloud contains
 */
// ----------------------------------------------------------------------

void CloudGroup::addType(const std::string & theCloudType) const
{
  if (!itsStandalone) {
	  size_t pos = itsCloudTypes.find("," + theCloudType + ",");

	  if (pos != std::string::npos)
		  itsCloudSet.insert(pos + 1);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Get the types the cloud contains
 */
// ----------------------------------------------------------------------

std::string CloudGroup::cloudTypes() const
{
  if (itsStandalone)
	  return itsCloudTypes;

  std::set<size_t>::iterator setbeg = itsCloudSet.begin(),setend = itsCloudSet.end(),it;
  std::ostringstream cts;
  std::string types;

  for (it = setbeg; (it != setend); it++) {
	std::string s(itsCloudTypes.substr(*it,itsCloudTypes.find(',',*it)));
	cts << ((it != setbeg) ? "\t" : "") << s;
  }

  types = cts.str();
  boost::algorithm::replace_all(types,",","");
  boost::algorithm::replace_all(types,"\t",",");

  return types;
}

// ----------------------------------------------------------------------
/*!
 * \brief CloudType
 */
// ----------------------------------------------------------------------

bool CloudType::operator ()(const CloudGroup & cloudGroup, const std::string & cloudType) const {
  return cloudGroup.contains(cloudType);
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
					   double theVOffset,double theVSOffset,
					   int theSOffset,int theEOffset,
					   std::set<size_t> & theIcingSet,
					   const libconfig::Setting * theLocalScope,
					   const libconfig::Setting * theGlobalScope)
  : itsIcingSet(theIcingSet)
{
  itsClass = boost::algorithm::trim_copy(theClass);

  if (!itsClass.empty()) {
	// Class names for icing magnitude output
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

  itsStandalone = (((theIcingTypes.find(",") == std::string::npos)) && (!combined));
  itsIcingTypes = (itsStandalone ? theIcingTypes : ("," + theIcingTypes + ","));
  itsSymbol = (theSymbol ? *theSymbol : "");
  renderSymbolOnly = symbolOnly;
  itsLabel = theLabel ? *theLabel : "";
  hasLabel = theLabel ? true : false;
  itsPlaceHolder = thePlaceHolder;
  itsLabelPlaceHolder = theLabelPlaceHolder;

  itsVOffset = theVOffset;
  itsVSOffset = theVSOffset;
  itsSOffset = theSOffset;
  itsEOffset = theEOffset;

  itsLocalScope = theLocalScope;
  itsGlobalScope = theGlobalScope;
}

// ----------------------------------------------------------------------
/*!
 * \brief Check if group contains given icing magnitude
 */
// ----------------------------------------------------------------------

bool IcingGroup::contains(const std::string & theIcingType) const
{
  if (itsStandalone)
	  return (itsIcingTypes == theIcingType);
  else
	  return (itsIcingTypes.find("," + theIcingType + ",") != std::string::npos);
}

// ----------------------------------------------------------------------
/*!
 * \brief Add a icing magnitude the icing group contains
 */
// ----------------------------------------------------------------------

void IcingGroup::addType(const std::string & theIcingType) const
{
  if (!itsStandalone) {
	  size_t pos = itsIcingTypes.find("," + theIcingType + ",");

	  if (pos != std::string::npos)
		  itsIcingSet.insert(pos + 1);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Get the magnitudes the icing group contains
 */
// ----------------------------------------------------------------------

std::string IcingGroup::icingTypes() const
{
  if (itsStandalone)
	  return itsIcingTypes;

  std::set<size_t>::iterator setbeg = itsIcingSet.begin(),setend = itsIcingSet.end(),it;
  std::ostringstream its;
  std::string types;

  for (it = setbeg; (it != setend); it++) {
	std::string s(itsIcingTypes.substr(*it,itsIcingTypes.find(',',*it)));
	its << ((it != setbeg) ? "\t" : "") << s;
  }

  types = its.str();
  boost::algorithm::replace_all(types,",","");
  boost::algorithm::replace_all(types,"\t",",");

  return types;
}

// ----------------------------------------------------------------------
/*!
 * \brief IcingType
 */
// ----------------------------------------------------------------------

bool IcingType::operator ()(const IcingGroup & icingGroup, const std::string & icingType) const {
  return icingGroup.contains(icingType);
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

  render_surface(path,cloudareas,id,theFeature.cloudTypeName());
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

  ++ncoldadvections;
  std::string id = "coldadvection" + boost::lexical_cast<std::string>(ncoldadvections);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = getCssSize("coldfrontglyph","font-size");
  double spacing = lookup<double>(config,"coldfront.letter-spacing");

  const char * Cc = ((theFeature.orientation() == "-") ? "C" : "c");

  render_front(path,paths,coldfronts,id,
			   "coldfront","coldfrontglyph",
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

  ++ncoldfronts;
  std::string id = "coldfront" + boost::lexical_cast<std::string>(ncoldfronts);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = getCssSize("coldfrontglyph","font-size");
  double spacing = lookup<double>(config,"coldfront.letter-spacing");

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

  render_text("Text",theFeature.name(),(text != options.locale) ? NFmiStringTools::UrlDecode(text) : "");
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

  ++noccludedfronts;
  std::string id = "occludedfront" + boost::lexical_cast<std::string>(noccludedfronts);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = getCssSize("occludedfrontglyph","font-size");
  double spacing = lookup<double>(config,"occludedfront.letter-spacing");

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

	woml::GeophysicalParameterValueSet * params = theFeature.parameters().get();

	if (!(params->values().empty())) {
		woml::GeophysicalParameterValue theValue = params->values().front();
		const woml::FlowDirectionMeasure * fdm = dynamic_cast<const woml::FlowDirectionMeasure *>(theValue.value());

		if (fdm)
			// FlowDirectionMeasure (wind direction) is visualized as symbol
			render_symbol("ParameterValueSetPoint",theValue.parameter().name(),fdm->value(),theFeature.point()->lon(),theFeature.point()->lat());
		else {
			const woml::NumericalSingleValueMeasure * svm = dynamic_cast<const woml::NumericalSingleValueMeasure *>(theValue.value());
			const woml::NumericalValueRangeMeasure * vrm = dynamic_cast<const woml::NumericalValueRangeMeasure *>(theValue.value());

			if (svm || vrm)
				// NumericalSingleValueMeasure and NumericalValueRangeMeasure are visualized as value
				render_value("ParameterValueSetPoint",theValue.parameter().name(),svm ? svm : &(vrm->lowerLimit()),svm ? NULL : &(vrm->upperLimit()),theFeature.point()->lon(),theFeature.point()->lat());
		}
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
  render_symbol("pointMeteorologicalSymbol",symClass,symCode,theFeature.point()->lon(),theFeature.point()->lat());
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
  render_symbol("pointMeteorologicalSymbol",theFeature.className(),"",theFeature.point()->lon(),theFeature.point()->lat()); \
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

  ++nridges;

  std::string id = "ridge" + boost::lexical_cast<std::string>(nridges);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = getCssSize("ridgeglyph","font-size");
  double spacing = lookup<double>(config,"ridge.letter-spacing");

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

  render_surface(path,precipitationareas,id,theFeature.rainPhaseName());
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
 * \brief Render a WarmAdvection
 */
// ----------------------------------------------------------------------

void
SvgRenderer::visit(const woml::WarmAdvection & theFeature)
{
  if(options.debug)	std::cerr << "Visiting ColdAdvection" << std::endl;

  ++nwarmadvections;
  std::string id = "warmadvection" + boost::lexical_cast<std::string>(nwarmadvections);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = getCssSize("warmfrontglyph","font-size");
  double spacing = lookup<double>(config,"warmfront.letter-spacing");

  const char * Ww = ((theFeature.orientation() == "-") ? "W" : "w");

  render_front(path,paths,warmfronts,id,
			   "warmfront","warmfrontglyph",
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

  ++nwarmfronts;
  std::string id = "warmfront" + boost::lexical_cast<std::string>(nwarmfronts);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  Path path = PathFactory::create(splines);

  PathProjector proj(area);
  path.transform(proj);

  double fontsize = getCssSize("warmfrontglyph","font-size");
  double spacing = lookup<double>(config,"warmfront.letter-spacing");

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

#ifdef __contouring__

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

#endif

} // namespace frontier
