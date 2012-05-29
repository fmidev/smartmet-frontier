// ======================================================================
/*!
 * \brief frontier::SvgRenderer
 */
// ======================================================================

#include "SvgRenderer.h"
#include "ConfigTools.h"
#include "PathFactory.h"
#include "PathTransformation.h"

#include <smartmet/woml/CubicSplineSurface.h>
#include <smartmet/woml/GeophysicalParameterValueSet.h>
#include <smartmet/woml/JetStream.h>
#include <smartmet/woml/OccludedFront.h>
#include <smartmet/woml/TimeGeophysicalParameterValueSet.h>

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
//#include <smartmet/newbase/NFmiEnumConverter.h>
//#include <smartmet/newbase/NFmiQueryData.h>
//#include <smartmet/newbase/NFmiFastQueryInfo.h>
#endif

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include <iomanip>
#include <iostream>
#include <limits>

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
		replace_all(ret,"\304","\303\204");
		replace_all(ret,"\303\244","\344");
		replace_all(ret,"\344","\303\244");
		replace_all(ret,"\303\226","\326");
		replace_all(ret,"\326","\303\226");
		replace_all(ret,"\303\266","\366");
		replace_all(ret,"\366","\303\266");
		replace_all(ret,"\303\205","\305");
		replace_all(ret,"\305","\303\205");
		replace_all(ret,"\303\245","\345");
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

  template <typename T,typename T2>
  T configValue(const libconfig::Setting & localScope,
		  	    const std::string & localScopeName,
		  	    const std::string & param,
		  	    const libconfig::Setting * globalScope = NULL,
		  	    settings settingId = s_required,
		  	    bool *isSet = NULL)
  {
    // Read the setting as optional. If the setting is missing, try to read with secondary type

    bool _isSet;

    T value = lookup<T>(localScope,localScopeName,param,s_optional,&_isSet);
    if ((!_isSet) &&  (typeid(T) != typeid(T2))) {
        T2 value2 = lookup<T2>(localScope,localScopeName,param,s_optional,&_isSet);

        if (_isSet)
        	value = boost::lexical_cast<T>(value2);
    }

    // Try global scope if available

    if ((!_isSet) && globalScope) {
  	  value = lookup<T>(*globalScope,localScopeName,param,s_optional,&_isSet);

      if ((!_isSet) &&  (typeid(T) != typeid(T2))) {
          T2 value2 = lookup<T2>(*globalScope,localScopeName,param,s_optional,&_isSet);

          if (_isSet)
          	value = boost::lexical_cast<T>(value2);
      }
    }

    if (isSet)
  	  *isSet = _isSet;

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
		  	    bool *isSet = NULL)
  {
	return configValue<T,T>(localScope,localScopeName,param,globalScope,settingId,isSet);
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Utility function for reading configuration value from active scope
   */
  // ----------------------------------------------------------------------

  template <typename T,typename T2>
  T configValue(const std::list<const libconfig::Setting *> & scope,
		  	    const std::string & localScopeName,
		  	    const std::string & param,
		  	    settings settingId = s_required,
		  	    bool *isSet = NULL)
  {
	T value = T();
    bool _isSet = false;

	std::list<const libconfig::Setting *>::const_reverse_iterator riter(scope.end()),renditer(scope.begin()),it,git;

	for( ; ((riter!=renditer) && (!_isSet)); ) {
		it = riter;
		riter++;
    	git = riter;

	    if (riter!=renditer)
			riter++;

	    // For the last block(s) use the given settingId; the call throws if the setting is required but not found
	    //
	    value = configValue<T,T2>(**it,localScopeName,param,((git!=renditer) ? *git : NULL),(riter!=renditer) ? s_optional : settingId,&_isSet);
	}

    if (isSet)
      *isSet = _isSet;

    return value;
  }

  template <typename T>
  T configValue(std::list<const libconfig::Setting *> & blocks,
		  	    const std::string & localScopeName,
		  	    const std::string & param,
		  	    settings settingId = s_required,
		  	    bool *isSet = NULL)
  {
	return configValue<T,T>(blocks,localScopeName,param,settingId,isSet);
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Utility function for converting locale to FmiLanguage
   */
  // ----------------------------------------------------------------------

  FmiLanguage locale2FmiLanguage(const std::string & confPath,
  	  	  	  	  	  	  	  	 const std::string & theLocale)
  {
	const char * langmsg = ": language must be 'fi','en' or 'sv'";

	if (theLocale.empty() || (theLocale.find("fi") == 0))
		return kFinnish;
	else if (theLocale.find("en") == 0)
		return kEnglish;
	else if (theLocale.find("sv") == 0)
		return kSwedish;
	else
		throw std::runtime_error(confPath + ": '" + theLocale + "'" + langmsg);
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
   * \brief Picture header rendering
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::render_header(const std::string & confPath,
		  	  	  	  	  	  	  const std::string & hdrClass)
  {
	try {
		const char * typemsg = " must contain a list of groups in parenthesis";
		const char * hdrtypemsg = ": type must be 'datetime'";

		// Set empty text to replace the template's placeholder even if no output is generated

		std::string HEADERhdrClass("HEADER" + hdrClass);
		texts[HEADERhdrClass] << "";

		const libconfig::Setting & hdrspecs = config.lookup(confPath);
		if(!hdrspecs.isList())
			throw std::runtime_error(confPath + typemsg);

		// configValue()/lookup() logic:
		//
		// If s_required (default) setting is not found, std::runtime_error is thrown (not catched here).
		//
		// If s_optional setting is not found, unset (default constructor) value is returned and
		// 'isSet' flag (if available) is set to false.
		//
		// If specific (s_base + n) setting is not found, SettingIdNotFoundException is thrown.

		settings s_name((settings) (s_base + 0));

		// Configuration blocks to search values for the settings; global blocks (blocks with no name),
		// locale global blocks (blocks with matching name but having no locale) and the
		// current block (block with matching name and locale).
		//
		// The blocks are stored to the list in the order of appearance (and having current block
		// as the last element). The list is processed in reverse order when searching.
		//
		// Current block or locale global block is needed to render the text.
		//
		std::list<const libconfig::Setting *> scope;
		bool nameMatch;
		bool hasLocaleGlobals = false;

		int hdrIdx = -1;
		int lastIdx = hdrspecs.getLength() - 1;

		for(int i=0; i<=lastIdx; ++i)
		{
			const libconfig::Setting & specs = hdrspecs[i];
			if(!specs.isGroup())
				throw std::runtime_error(confPath + typemsg);

			try {
				// On last config entry enter the block anyway to use locale globals
				//
				if (
					(nameMatch = (lookup<std::string>(specs,confPath,"name",s_name) == hdrClass)) ||
					((i == lastIdx) && hasLocaleGlobals)
				   )
				{
					if (nameMatch) {
						hdrIdx = i;

						// Locale
						//
						std::string locale(toLower(configValue<std::string>(specs,hdrClass,"locale",NULL,s_optional)));

						if (locale != options.locale) {
							if (locale == "") {
								// Locale globals have no locale
								//
								scope.push_back(&hdrspecs[i]);
								hasLocaleGlobals = true;
							}

							if ((i < lastIdx) || (!hasLocaleGlobals))
								continue;
						}
						else
							scope.push_back(&hdrspecs[i]);
					}

					// Header type (currently only datetime supported)
					std::string type = configValue<std::string>(scope,hdrClass,"type",s_optional);

					if (type.empty() || (type == "datetime")) {
						// NFmiTime language and format
						//
						FmiLanguage language = locale2FmiLanguage(confPath,options.locale);
						std::string pref(svgescapetext(configValue<std::string>(scope,hdrClass,"pref"),true));

						// Convert from ptime (to_iso_string() format YYYYMMDDTHHMMSS,fffffffff
						// where T is the date-time separator) to NFmiMetTime (utc) and further
						// to local time if (utc not) requested

						NFmiMetTime utcTime(1);
						NFmiString datum;
						std::string isoString = to_iso_string(validtime);
						boost::replace_first(isoString,"T","");
						utcTime.FromStr(isoString);

						bool isSet;
						bool utc(configValue<bool>(scope,hdrClass,"utc",s_optional,&isSet));
						if ((!isSet) || (!utc)) {
							// LocalTime() takes daylight saving time information using current system time; adjust the
							// result using daylight saving time information for the target datetime.
							//
							// The need to correct the result (if the result is based on wrong dst) is detected
							// from the offset given by NFmiTime (instead of checking the current system time dst
							// which could have changed after queried by NFmiMetTime), so this correction only works for
							// certain (here GMT+2) timezone
							//
							NFmiTime localTime = utcTime.LocalTime();
							struct tm vtm = to_tm(validtime);
							mktime(&vtm);

							short tzDiff = abs(localTime.GetZoneDifferenceHour());

							if ((tzDiff == 3) && (vtm.tm_isdst == 0))
								localTime.ChangeByHours(-1);
							else if ((tzDiff == 2) && (vtm.tm_isdst == 1))
								localTime.ChangeByHours(1);

							datum = localTime.ToStr(pref,language);
						}
						else
							datum = utcTime.ToStr(pref,language);

						texts[HEADERhdrClass] << datum.CharPtr();

						return;
					}
					else
						throw std::runtime_error(confPath + ": '" + type + "'" + hdrtypemsg);
				}
			}
			catch (SettingIdNotFoundException & ex) {
				if (ex.id() == s_name)
					// Global settings have no name
					//
					scope.push_back(&hdrspecs[i]);
			}
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
		if (options.debug) {
			debugoutput << "Settings for "
						<< confPath
						<< " ("
						<< hdrClass
						<< ") not found\n";
		}
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
   * \brief Text rendering
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::render_text(const std::string & confPath,
		  	  	  	  	  	    const std::string & textName,
		  	  	  	  	  	    const std::string & text)
  {
	try {
		const char * typemsg = " must contain a list of groups in parenthesis";
		const char * stylemsg = ": slant must be 'normal', 'italic' or 'oblique'";
		const char * weightmsg = ": weight must be 'normal' or 'bold'";

		// Set initial texts to replace template's placeholders even if no output is generated

		std::string TEXTCLASStextName("TEXTCLASS" + textName);
		std::string TEXTAREAtextName("TEXTAREA" + textName);
		std::string TEXTtextName("TEXT" + textName);

		texts[TEXTCLASStextName] << "";
		texts[TEXTAREAtextName] << " x=\"0\" y=\"0\" width=\"0\" height=\"0\"";
		texts[TEXTtextName] << "";

		if (text.empty())
			return;

		const libconfig::Setting & textspecs = config.lookup(confPath);
		if(!textspecs.isList())
			throw std::runtime_error(confPath + typemsg);

		// configValue()/lookup() logic:
		//
		// If s_required (default) setting is not found, std::runtime_error is thrown (not catched here).
		//
		// If s_optional setting is not found, unset (default constructor) value is returned and
		// 'isSet' flag (if available) is set to false.
		//
		// If specific (s_base + n) setting is not found, SettingIdNotFoundException is thrown.

		settings s_name((settings) (s_base + 0));

		// Configuration blocks to search values for the settings; global blocks (blocks with no name),
		// locale global blocks (blocks with matching name but having no locale) and the
		// current block (block with matching name and locale).
		//
		// The blocks are stored to the list in the order of appearance (and having current block
		// as the last element). The list is processed in reverse order when searching.
		//
		// Current block or locale global block is needed to render the text.
		//
		std::list<const libconfig::Setting *> scope;
		bool nameMatch;
		bool hasLocaleGlobals = false;

		int textIdx = -1;
		int lastIdx = textspecs.getLength() - 1;

		for(int i=0; i<=lastIdx; ++i)
		{
			const libconfig::Setting & specs = textspecs[i];
			if(!specs.isGroup())
				throw std::runtime_error(confPath + typemsg);

			try {
				// On last config entry enter the block anyway to use locale globals
				//
				if (
					(nameMatch = (lookup<std::string>(specs,confPath,"name",s_name) == textName)) ||
					((i == lastIdx) && hasLocaleGlobals)
				   )
				{
					if (nameMatch) {
						textIdx = i;

						// Locale
						//
						std::string locale(toLower(configValue<std::string>(specs,textName,"locale",NULL,s_optional)));

						if (locale != options.locale) {
							if (locale == "") {
								// Locale globals have no locale
								//
								scope.push_back(&textspecs[i]);
								hasLocaleGlobals = true;
							}

							if ((i < lastIdx) || (!hasLocaleGlobals))
								continue;
						}
						else
							scope.push_back(&textspecs[i]);
					}

					// Font name
					std::string font = configValue<std::string>(scope,textName,"font-family");

					// Font size
					int size = configValue<int>(scope,textName,"font-size");

					// Font style
					cairo_font_slant_t slant = CAIRO_FONT_SLANT_NORMAL;
					std::string style = configValue<std::string>(scope,textName,"font-style",s_optional);

					if (style == "italic")
						slant = CAIRO_FONT_SLANT_ITALIC;
					else if (style == "oblique")
				    	slant = CAIRO_FONT_SLANT_OBLIQUE;
					else if (style != "")
						throw std::runtime_error(confPath + stylemsg);
					else
						style = "normal";

					// Font weight
					cairo_font_weight_t _weight = CAIRO_FONT_WEIGHT_NORMAL;
					std::string weight = configValue<std::string>(scope,textName,"font-weight",s_optional);

					if (weight == "bold")
						_weight = CAIRO_FONT_WEIGHT_BOLD;
					else if (weight != "")
						throw std::runtime_error(confPath + weightmsg);
					else
						weight = "normal";

					// Max width, height and x/y margin
					int maxWidth = configValue<int>(scope,textName,"width");

					bool isSet;
					int maxHeight = configValue<int>(scope,textName,"height",s_optional,&isSet);
					if (! isSet)
						maxHeight = 0;

					int margin = configValue<int>(scope,textName,"margin",s_optional,&isSet);
					if (! isSet)
						margin = 2;
					else
						margin = abs(margin);

					if (maxWidth <= 20)
						maxWidth = 20;
					if ((maxHeight != 0) && (maxHeight <= 20))
						maxHeight = 20;

					// cairo surface to get font mectrics

					cairo_surface_t *cs = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,1,1);
					cairo_t * cr = cairo_create(cs);
					cairo_surface_destroy(cs);

					cairo_select_font_face(cr,font.c_str(),slant,_weight);
					cairo_set_font_size(cr,size);

					// Split the text to lines shorter than given max. width.
					// Keep track of max. line width and height

					std::list<std::string> outputLines;
					std::vector<std::string> inputLines;
					boost::split(inputLines,text,boost::is_any_of("\n"));
					size_t l,textlc = inputLines.size();
					int maxLineWidth = 0,maxLineHeight = 0;

					cairo_text_extents_t extents;
					std::ostringstream line;
					std::string word;

					for (l = 0; (l < textlc); l++) {
						// Next line
						//
						std::vector<std::string> words;
						boost::split(words,inputLines[l],boost::is_any_of(" \t"));

						cairo_text_extents(cr,inputLines[l].c_str(),&extents);
						int lineHeight = static_cast<int>(ceil(extents.height));

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
								int width = static_cast<int>(ceil(lineWidth));

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
						}  // for word
					}  // for line

					cairo_destroy(cr);
//					cairo_debug_reset_static_data();

					int textHeight = static_cast<int>((outputLines.size() * (maxLineHeight + 2)) - 2);

					if ((maxHeight > 0) && (maxHeight < textHeight))
						// Max height exceeded
						;

					// Store the css class definition

					std::string textClass("text"+ textName);

					texts[TEXTCLASStextName] << "." << textClass
											 << " {\nfont-family: " << font
											 << ";\nfont-size: " << size
											 << ";\nfont-weight : " << weight
											 << ";\nfont-style : " << style
											 << ";\n}\n";

					// Store geometry for text border

					texts[TEXTAREAtextName].str("");
					texts[TEXTAREAtextName].clear();

					int textWidth = ((maxLineWidth == 0) ? maxWidth : maxLineWidth);

					texts[TEXTAREAtextName] << " x=\"0\" y=\"0\""
											<< " width=\"" << (2 * margin) + textWidth
											<< "\" height=\"" << (2 * margin) + textHeight
											<< "\" ";

					// Store the text. Final position (transformation) is supposed to be defined in the template

					std::list<std::string>::const_iterator it = outputLines.begin();

					for (int y = (margin + maxLineHeight); (it != outputLines.end()); it++, y += maxLineHeight)
						texts[TEXTtextName] << "<text class=\"" << textClass
											<< "\" x=\"" << margin
											<< "\" y=\"" << y
											<< "\">" << svgescapetext(*it) << "</text>\n";

					return;
				}  // if
			}
			catch (SettingIdNotFoundException & ex) {
				if (ex.id() == s_name)
					// Global settings have no name
					//
					scope.push_back(&textspecs[i]);
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
   * \brief Surface rendering
   */
  // ----------------------------------------------------------------------

  void SvgRenderer::render_surface(const Path & path,
		  	  	  	  	  	  	   std::ostringstream & surfaces,
		  	  	  	  	  	  	   const std::string & id,
		  	  	  	  	  	  	   const std::string & surfaceName)		// cloudType (for CloudArea) or rainPhase (for SurfacePrecipitationArea)
  {
	std::string confPath("Surface");

	try {
		const char * typemsg = " must contain a list of groups in parenthesis";
		const char * surftypemsg = ": surface type must be 'pattern', 'mask', 'glyph' or 'svg'";

		const libconfig::Setting & surfspecs = config.lookup(confPath);
		if(!surfspecs.isList())
			throw std::runtime_error(confPath + typemsg);

		// configValue()/lookup() logic:
		//
		// If s_required (default) setting is not found, std::runtime_error is thrown (not catched here).
		//
		// If s_optional setting is not found, unset (default constructor) value is returned and
		// 'isSet' flag (if available) is set to false.
		//
		// If specific (s_base + n) setting is not found, SettingIdNotFoundException is thrown.

		settings s_name((settings) (s_base + 0));

		int surfIdx = -1;
		int globalsIdx = -1;

		for(int i=0; i<surfspecs.getLength(); ++i)
		{
			const libconfig::Setting & specs = surfspecs[i];
			if(!specs.isGroup())
				throw std::runtime_error(confPath + typemsg);

			try {
				if (lookup<std::string>(specs,confPath,"name",s_name) == surfaceName) {
					surfIdx = i;

					// Missing settings from globals when available
					libconfig::Setting * globalScope = ((globalsIdx >= 0) ? &surfspecs[globalsIdx] : NULL);

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
								}
							}
						}

						return;
					}
					else if (type == "svg") {
						throw std::runtime_error(confPath + " type 'svg' not implemented yet");
					}
					else
						throw std::runtime_error(confPath + ": '" + type + "'" + surftypemsg);
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
		  	  	  	  	  	  	  	  	  	   const libconfig::Setting & condspecs,
		  	  	  	  	  	  	  	  	  	   const std::string & confPath,
		  	  	  	  	  	  	  	  	  	   const std::string & className,
		  	  	  	  	  	  	  	  	  	   int specsIdx,
		  	  	  	  	  	  	  	  	  	   const std::string & parameter,
		  	  	  	  	  	  	  	  	  	   const double numericValue)
  {
	const char * typemsg = " must contain a list of groups in parenthesis";
	const char * condmsg = ": refix must be 'eq', 'le', 'lt', 'ge' or 'gt'";

	double ltCondValue = 0.0,gtCondValue = 0.0,eps = 0.00001;
	int ltCondIdx = -1,gtCondIdx = -1,idx = -1;

	for(int i=0; i<condspecs.getLength(); i++)
	{
		const libconfig::Setting & conds = condspecs[i];
		if(!conds.isGroup())
			throw std::runtime_error(confPath + ".conditions" + typemsg);

		std::string refix(configValue<std::string>(conds,className,"refix"));
		double condValue(configValue<double>(conds,className,"value"));

		if (refix == "eq") {
			if (fabs(condValue - numericValue) <= eps) {
				ltCondIdx = gtCondIdx = i;
				break;
			}
		}
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
			throw std::runtime_error(confPath + ".conditions: '" + refix + "'" + condmsg);
	}

	if ((ltCondIdx >= 0) || (gtCondIdx >= 0)) {
		idx = ((ltCondIdx < 0) || ((gtCondIdx >= 0) && ((numericValue - ltCondValue) > (gtCondValue - numericValue))))
			  ? gtCondIdx
			  : ltCondIdx;

		return &(condspecs[idx]);
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
		  	  	  	  	  	  	  	  	  	   const libconfig::Setting & condspecs,
		  	  	  	  	  	  	  	  	  	   const std::string & confPath,
		  	  	  	  	  	  	  	  	  	   const std::string & className,
		  	  	  	  	  	  	  	  	  	   int specsIdx,
		  	  	  	  	  	  	  	  	  	   const std::string & parameter,
		  	  	  	  	  	  	  	  	  	   const boost::posix_time::ptime & dateTimeValue)
  {
	const char * typemsg = " must contain a list of groups in parenthesis";
	const char * condmsg = ": refix must be 'range'";
	const char * rngmsg = ": hh[:mm]-hh[:mm] range expected";

	for(int i=0; i<condspecs.getLength(); i++)
	{
		const libconfig::Setting & conds = condspecs[i];
		if(!conds.isGroup())
			throw std::runtime_error(confPath + ".conditions" + typemsg);

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
												 ".[" + boost::lexical_cast<std::string>(specsIdx) + "].conditions" +
												 ".[" + boost::lexical_cast<std::string>(i) + "]." + parameter);

							if (parameter.empty() || config.exists(condPath))
								return &(condspecs[i]);
						}

						continue;
					}
				}
				catch(std::exception & ex) {
					rngErr = std::string(": ") + ex.what();
				}
			}

			throw std::runtime_error(confPath + ".conditions: '" + condValue + "'" + rngmsg + rngErr);
		}
		else
			throw std::runtime_error(confPath + ".conditions: '" + refix + "'" + condmsg);
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
	const char * typemsg = " must contain a list of groups in parenthesis";

	std::string condPath(confPath + ".[" + boost::lexical_cast<std::string>(specsIdx) + "].conditions");

	if (!config.exists(condPath))
		return NULL;

	const libconfig::Setting & condspecs = config.lookup(condPath);
	if(!condspecs.isList())
		throw std::runtime_error(confPath + ".conditions" + typemsg);

	return matchingCondition(config,condspecs,confPath,className,specsIdx,parameter,condValue);
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Utility function for obtaining symbol code and optional folder; [<folder>/]<code>.
   */
  // ----------------------------------------------------------------------

  std::string getFolderAndSymbol(const libconfig::Config & config,
		  	  	  	  	  	  	 std::list<const libconfig::Setting *> scope,
		  	  	  	  	  	  	 const libconfig::Setting & specs,
		  	  	  	  	  	  	 const std::string & confPath,
		  	  	  	  	  	  	 const std::string & symClass,
		  	  	  	  	  	  	 const std::string & symCode,
		  	  	  	  	  	  	 settings s_code,
		  	  	  	  	  	  	 int specsIdx,
		  	  	  	  	  	  	 const boost::posix_time::ptime & dateTimeValue,
		  	  	  	  	  	  	 std::string & folder)
  {
	// First search for conditional settings based on the documents validTime (it's time value)

	if (specsIdx >= 0) {
		const libconfig::Setting * condSpecs = matchingCondition(config,confPath,symClass,symCode,specsIdx,dateTimeValue);

		if (condSpecs)
			scope.push_back(condSpecs);
	}

	const char * codemsg = ": symbol code: [<folder>/]<code> expected";

	std::string code = configValue<std::string,int>(scope,symClass,symCode,s_code);

	std::vector<std::string> cols;
	boost::split(cols,code,boost::is_any_of("/"));

	int codeIdx;
	boost::trim(cols[codeIdx = 0]);

	if (cols.size() == 2)
		boost::trim(cols[codeIdx = 1]);

	if ((cols.size() > 2) || (cols[0] == "") || (cols[codeIdx] == ""))
		throw std::runtime_error(confPath + ": '" + code + "'" + codemsg);

	folder = ((codeIdx == 1) ? cols[0] : "");

	return cols[codeIdx];
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Symbol rendering
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
	std::string _symCode(symCode.empty() ? "" : ("_" + symCode));

	// Find the pixel coordinate. For surface fill the positions are already projected
	if (!fpos) {
		PathProjector proj(area);
		proj(lon,lat);
	}

	++npointsymbols;

	try {
		const char * typemsg = " must contain a list of groups in parenthesis";
		const char * symtypemsg = ": symbol type must be 'svg', 'img' or 'font'";

		const libconfig::Setting & symbolspecs = config.lookup(confPath);
		if(!symbolspecs.isList())
			throw std::runtime_error(confPath + typemsg);

		// configValue()/lookup() logic:
		//
		// If s_required (default) setting is not found, std::runtime_error is thrown (not catched here).
		//
		// If s_optional setting is not found, unset (default constructor) value is returned and
		// 'isSet' flag (if available) is set to false.
		//
		// If specific (s_base + n) setting is not found, SettingIdNotFoundException is thrown.

		settings s_name((settings) (s_base + 0));
		settings s_code((settings) (s_base + 1));

		// Storage for configuration blocks to search values for the settings; 'global' blocks (blocks with no name),
		// 'locale global' blocks (blocks with matching name but having no locale) and the 'current' block
		// (block with matching name and locale).
		//
		// The blocks are stored to the list in the order of appearance (having the 'current' block if present
		// as the last element). The list is processed in reverse order when searching values for the settings.
		//
		// Current block or locale global block is needed to render the text.
		//
		std::list<const libconfig::Setting *> scope;
		bool nameMatch;
		bool hasLocaleGlobals = false;

		int symbolIdx = -1;
		int lastIdx = symbolspecs.getLength() - 1;

		for(int i=0; i<=lastIdx; ++i)
		{
			const libconfig::Setting & specs = symbolspecs[i];
			if(!specs.isGroup())
				throw std::runtime_error(confPath + typemsg);

			try {
				// On last config entry enter the block anyway to use locale globals
				//
				if (
					(nameMatch = (lookup<std::string>(specs,confPath,"name",s_name) == symClass)) ||
					((i == lastIdx) && hasLocaleGlobals)
				   )
				{
					if (nameMatch) {
						symbolIdx = i;

						// Locale
						//
						std::string locale(toLower(configValue<std::string>(specs,symClass,"locale",NULL,s_optional)));

						if (locale != options.locale) {
							if (locale == "") {
								// Locale globals have no locale
								//
								scope.push_back(&symbolspecs[i]);
								hasLocaleGlobals = true;
							}

							if ((i < lastIdx) || (!hasLocaleGlobals))
								continue;
						}
						else
							scope.push_back(&symbolspecs[i]);
					}

					// Symbol code. May contain folder too; [<folder>/]<code>
					//
					std::string folder;
					std::string code = getFolderAndSymbol(config,scope,symbolspecs[i],confPath,symClass,"code" + _symCode,s_code,symbolIdx,validtime,folder);

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
						throw std::runtime_error(confPath + ": '" + type + "'" + symtypemsg);
				}
			}
			catch (SettingIdNotFoundException & ex) {
				if (ex.id() == s_name)
					// Global settings have no name
					//
					scope.push_back(&symbolspecs[i]);
				else
					// Symbol code not found
					;
			}
		}	// for

		if (options.debug) {
			// No (locale) settings for the symbol
			//
			const char * p = ((symbolIdx < 0) ? "Settings for " : "Locale specific settings for " );

			debugoutput << p
						<< symClass
						<< " (code"
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
   * \brief Utility function for formatting
   */
  // ----------------------------------------------------------------------

  std::string formattedValue(const woml::NumericalSingleValueMeasure * lowerLimit,
		  	  	  	  	     const woml::NumericalSingleValueMeasure * upperLimit,
		  	  	  	  	     const std::string & confPath,
		  	  	  	  	     const std::string & pref)
  {
	if (pref.empty())
		return upperLimit ? (lowerLimit->value() + ".." + upperLimit->value()) : lowerLimit->value();

	std::ostringstream os;

	try {
		if (upperLimit)
			os << boost::format(pref) % lowerLimit->numericValue() % upperLimit->numericValue();
		else
			os << boost::format(pref) % lowerLimit->numericValue();

		return os.str();
	}
	catch(std::exception & ex) {
		throw std::runtime_error(confPath + ": '" + pref + "': " + ex.what());
	}
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
		const char * typemsg = " must contain a list of groups in parenthesis";
		const char * valtypemsg = ": type must be 'value' or 'svg'";

		const libconfig::Setting & valspecs = config.lookup(confPath);
		if(!valspecs.isList())
			throw std::runtime_error(confPath + typemsg);

		// configValue()/lookup() logic:
		//
		// If s_required (default) setting is not found, std::runtime_error is thrown (not catched here).
		//
		// If s_optional setting is not found, unset (default constructor) value is returned and
		// 'isSet' flag (if available) is set to false.
		//
		// If specific (s_base + n) setting is not found, SettingIdNotFoundException is thrown.

		settings s_name((settings) (s_base + 0));

		int valIdx = -1;
		int globalsIdx = -1;

		for(int i=0; i<valspecs.getLength(); ++i)
		{
			const libconfig::Setting & specs = valspecs[i];
			if(!specs.isGroup())
				throw std::runtime_error(confPath + typemsg);

			try {
				if (lookup<std::string>(specs,confPath,"name",s_name) == valClass) {
					valIdx = i;

					// Missing settings from globals when available
					libconfig::Setting * globalScope = ((globalsIdx >= 0) ? &valspecs[globalsIdx] : NULL);

					// Value type; value (the default) or svg
					std::string type = configValue<std::string>(specs,valClass,"type",globalScope,s_optional);

					if (type.empty() || (type == "value")) {
						// Class, format and reference for "background" class
						//
						std::string vtype(upperLimit ? "Range" : "");

						std::string classdef(configValue<std::string>(specs,valClass,"class" + vtype,globalScope,s_optional));
						if (classdef.empty())
							classdef = (vtype.empty() ? valClass : (valClass + " " + valClass + vtype));

						std::string pref(configValue<std::string>(specs,valClass,"pref" + vtype,globalScope,s_optional));
						std::string href(configValue<std::string>(specs,valClass,"href" + vtype,globalScope,s_optional));

						// For single value, search for matching condition with nearest comparison value

						const libconfig::Setting * condspecs = (! upperLimit)
							? matchingCondition(config,confPath,valClass,"",i,lowerLimit->numericValue())
							: NULL;

						if (condspecs) {
							// Class from the condition or from the parent/parameter block
							//
							classdef = configValue<std::string>(*condspecs,valClass,"class",&specs);

							bool isSet;
							std::string cpref = configValue<std::string>(*condspecs,valClass,"pref" + vtype,NULL,s_optional,&isSet);
							if (isSet)
								pref = cpref;

							std::string chref = configValue<std::string>(*condspecs,valClass,"href" + vtype,NULL,s_optional,&isSet);
							if (isSet)
								href = chref;
						}

						pointvalues << "<g class=\""
									<< classdef
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
						throw std::runtime_error(confPath + ": '" + type + "'" + valtypemsg);
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
		if (options.debug) {
			debugoutput << "Settings for "
						<< confPath
						<< " ("
						<< valClass
						<< ") not found\n";
		}
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
  // Render validtime as picture header

  render_header("Header","validTime");
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
	  replace_all(ret,"--DEBUGOUTPUT--"       ,"<![CDATA[\n" + debugoutput.str() + "]]>\n");
	  std::cerr << debugoutput.str();
  }

  // BOOST_FOREACH does not work nicely with ptr_map

#ifdef __contouring__

  for(Contours::const_iterator it=contours.begin(); it!=contours.end(); ++it)
	replace_all(ret,it->first, it->second->str());

#endif

  for(Texts::const_iterator it=texts.begin(); it!=texts.end(); ++it)
	replace_all(ret,"--" + it->first + "--", it->second->str());

  return ret;

}

void SvgRenderer::visit(const woml::TimeGeophysicalParameterValueSet & theFeature) { };

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
