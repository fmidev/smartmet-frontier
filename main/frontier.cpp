// ======================================================================
/*!
 * \brief WOML renderer
 */
// ======================================================================

#include "ConfigTools.h"
#include "Options.h"
#include "PreProcessor.h"
#include "SvgRenderer.h"

#include <smartmet/newbase/NFmiArea.h>
#include <smartmet/newbase/NFmiAreaFactory.h>
#include <smartmet/newbase/NFmiPreProcessor.h>
#include <smartmet/newbase/NFmiQueryData.h>

#include <smartmet/woml/DataSource.h>
#include <smartmet/woml/MeteorologicalAnalysis.h>
#include <smartmet/woml/Parser.h>
#include <smartmet/woml/TargetRegion.h>
#include <smartmet/woml/WeatherForecast.h>

#include <libconfig.h++>

#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>

#include <fstream>
#include <iostream>
#include <stdexcept>

// Does libconfig++ have readString?

#define NEWLIBCONFIG 0

#if !(NEWLIBCONFIG)
#include <boost/lexical_cast.hpp>
#include <sys/types.h>
#include <fstream>
#include <stdexcept>
#include <unistd.h>
#endif

// ----------------------------------------------------------------------
/*!
 * \brief Read a file into a string
 */
// ----------------------------------------------------------------------

std::string readfile(const std::string& fileName,
                     const std::string defineDirective = "#define",
                     const std::string includeDirective = "#include",
                     const std::string& includePathDefine = "INCLUDEPATH",
                     const std::string includeFileExtension = "tpl")
{
  const bool stripPound = false, stripDoubleSlash = false;

  PreProcessor processor(defineDirective,
                         includeDirective,
                         includePathDefine,
                         includeFileExtension,
                         stripPound,
                         stripDoubleSlash);

  if (!processor.ReadAndStripFile(fileName))
    throw std::runtime_error("Preprocessor failed to read '" + fileName + "'");

  return processor.GetString();
}

// ----------------------------------------------------------------------
/*!
 * \brief Write a string into a file
 */
// ----------------------------------------------------------------------

void writefile(const std::string& filename, const std::string& contents)
{
  std::ofstream out(filename.c_str());
  if (!out) throw std::runtime_error("Failed to open '" + filename + "' for writing");
  out << contents;
  out.close();
}

// ----------------------------------------------------------------------
/*!
 * \brief Read the configuration string
 */
// ----------------------------------------------------------------------

void readconfig(libconfig::Config& config, const std::string& contents)
{
#if NEWLIBCONFIG
  config.readString(contents);
#else
  std::string filename = ("/tmp/frontier_" + boost::lexical_cast<std::string>(getpid()) + ".cnf");
  std::ofstream out(filename.c_str());
  if (!out) throw std::runtime_error("Failed to open '" + filename + "' for writing");
  out << contents;
  out.close();

  config.readFile(filename.c_str());

  if (unlink(filename.c_str()) != 0)
    throw std::runtime_error("Failed to delete '" + filename + "'");
#endif
}

// ----------------------------------------------------------------------
/*!
 * \brief Generate projection from a file
 */
// ----------------------------------------------------------------------

boost::shared_ptr<NFmiArea> readprojection(const std::string& filename)
{
  const bool strip_pound = false;
  NFmiPreProcessor processor(strip_pound);
  processor.SetIncluding("include", "", "");
  processor.SetDefine("#define");
  if (!processor.ReadAndStripFile(filename))
    throw std::runtime_error("Preprocessor failed to read '" + filename + "'");

  std::string text = processor.GetString();
  std::istringstream script(text);

  boost::shared_ptr<NFmiArea> area;

  std::string token;
  while (script >> token)
  {
    if (token == "#")
      script.ignore(1000000, '\n');
    else if (token == "projection")
    {
      script >> token;
      area = NFmiAreaFactory::Create(token);
      break;
    }
    else
    {
    }
  }

  if (!area) throw std::runtime_error("Failed to find a projection from '" + filename + "'");

  return area;
}

// ----------------------------------------------------------------------
/*!
 * \brief Extract section between given delimiters
 */
// ----------------------------------------------------------------------

std::string extract_section(const std::string& str,
                            const std::string& startdelim,
                            const std::string& enddelim)
{
  std::string::size_type pos1 = str.find(startdelim);
  if (pos1 == std::string::npos) return "";

  std::string::size_type pos2 = str.find(enddelim, pos1);
  if (pos2 == std::string::npos) return "";

  return str.substr(pos1 + startdelim.size(), pos2 - pos1 - startdelim.size());
}

// ----------------------------------------------------------------------
/*!
 * \brief Remove sections between given delimiters
 */
// ----------------------------------------------------------------------

std::string remove_sections(const std::string& str,
                            const std::string& startdelim,
                            const std::string& enddelim)
{
  std::string ret = str;
  while (true)
  {
    std::string::size_type pos1 = ret.find(startdelim);
    if (pos1 == std::string::npos) return ret;

    std::string::size_type pos2 = ret.find(enddelim, pos1);
    if (pos2 == std::string::npos) return ret;

    std::string part1 = ret.substr(0, pos1);
    std::string part2 = ret.substr(pos2 + enddelim.size(), ret.size());
    ret = part1 + part2;
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Extract available validtimes
 */
// ----------------------------------------------------------------------

typedef std::set<boost::posix_time::ptime> ValidTimes;

template <typename T>
ValidTimes extract_valid_times(const T& collection)
{
  ValidTimes times;

  BOOST_FOREACH (const woml::Feature& f, collection)
  {
    const boost::optional<boost::posix_time::ptime> theTime = f.validTime();

    // Some features (forecast/analysis shortInfo and longInfo texts) have no valid time

    if (theTime && (!(theTime->is_not_a_date_time()))) times.insert(*theTime);
  }

  return times;
}

// ----------------------------------------------------------------------
/*!
 * \brief Convert NFmiMetTime to ptime
 */
// ----------------------------------------------------------------------

boost::posix_time::ptime to_ptime(const NFmiMetTime& theTime)
{
  boost::gregorian::date date(theTime.GetYear(), theTime.GetMonth(), theTime.GetDay());

  boost::posix_time::ptime utc(date,
                               boost::posix_time::hours(theTime.GetHour()) +
                                   boost::posix_time::minutes(theTime.GetMin()) +
                                   boost::posix_time::seconds(theTime.GetSec()));
  return utc;
}

// ----------------------------------------------------------------------
/*!
 * \brief Search the model with the given origin time
 *
 * Returns an empty pointer if the correct model is not found.
 */
// ----------------------------------------------------------------------

boost::shared_ptr<NFmiQueryData> search_model_origintime(const frontier::Options& options,
                                                         const std::string& path,
                                                         const boost::posix_time::ptime& origintime)
{
  namespace fs = boost::filesystem;

  if (!fs::exists(path)) throw std::runtime_error("Path '" + path + "' does not exist");

  if (!fs::is_directory(path)) throw std::runtime_error("Path '" + path + "' is not a directory");

  fs::directory_iterator end_dir;
  for (fs::directory_iterator dirptr(path); dirptr != end_dir; ++dirptr)
  {
    if (!fs::is_regular_file(dirptr->status())) continue;

    try
    {
      NFmiQueryInfo qi(dirptr->path().string());

      if (to_ptime(qi.OriginTime()) == origintime)
      {
        if (options.debug)
          std::cerr << "File '" << dirptr->path() << "' matched origin time "
                    << to_simple_string(origintime) << std::endl;

        return boost::shared_ptr<NFmiQueryData>(new NFmiQueryData(dirptr->path().string()));
      }
    }
    catch (...)
    {
    }
  }

  return boost::shared_ptr<NFmiQueryData>();
}

// ----------------------------------------------------------------------
/*!
 * \brief Resolve the used numerical model
 *
 * Returns empty shared pointer if there is no model in the WOML.
 * If there is a model, and the respective file is not found,
 * an error is thrown *unless* the debug flag is set.

 */
// ----------------------------------------------------------------------

#ifdef CONTOURING

boost::shared_ptr<NFmiQueryData> resolve_model(const frontier::Options& options,
                                               const libconfig::Config& config,
                                               const woml::DataSource& source)
{
  boost::shared_ptr<NFmiQueryData> ret;

  if (!source.numericalModelRun()) return ret;

  // Shorthand help variables

  const std::string& name = source.numericalModelRun()->name();
  const boost::posix_time::ptime& origintime = source.numericalModelRun()->analysisTime();

  if (name.find_first_not_of(" ") == std::string::npos) return ret;

  std::string path = frontier::lookup<std::string>(config, "models." + name);

  if (path.empty())
  {
    if (options.debug)
    {
      if (options.verbose) std::cerr << "Ignoring unknown model '" + name + "'" << std::endl;
      return ret;
    }
    else
    {
      throw std::runtime_error("Unknown model called '" + name + "' in WOML data");
    }
  }

  ret = search_model_origintime(options, path, origintime);

  if (!ret)
    throw std::runtime_error("Numerical model '" + name + "' referenced in data for origin time " +
                             to_simple_string(origintime) + " was not found from directory " +
                             path);

  return ret;
}

#endif

// ----------------------------------------------------------------------
/*!
 * \brief Main program without exception handling
 */
// ----------------------------------------------------------------------

int run(int argc,
        char* argv[],
        boost::shared_ptr<NFmiArea>& area,
        std::string& outfile,
        bool& debug,
        std::ostringstream& debugoutput)
{
  frontier::Options options;

  outfile.clear();
  debug = false;

  if (!parse_options(argc, argv, options)) return 0;

  outfile = options.outfile;

  // Read the SVG template

  std::string svg = readfile(options.svgfile);

  // --DEBUGOUTPUT-- placeholder in the template sets debug mode

  if (!options.debug) options.debug = (svg.find("--DEBUGOUTPUT--") != std::string::npos);

  // Establish the projection

  if (boost::filesystem::is_regular_file(options.projection))
    area = readprojection(options.projection);
  else
    area = NFmiAreaFactory::Create(options.projection);

  // Parse the WOML

  woml::Weather weather = woml::parse(options.womlfile, options.doctype, options.debug);
  if (weather.empty()) throw std::runtime_error("No MeteorologicalAnalysis to draw");

  // Extract libconfig section

  std::string configstring = extract_section(svg, "<frontier>", "</frontier>");

  // Remove comments

  svg = remove_sections(svg, "<!--", "-->");
  svg = remove_sections(svg, "/*", "*/");

  // Configure

  libconfig::Config config;
  readconfig(config, configstring);

  // Check what data is available

  if (weather.hasAnalysis() && weather.hasForecast())
    throw std::runtime_error("WOML data contains both analysis and forecast");

  // Extract available times

  ValidTimes validtimes;

  if (weather.hasAnalysis())
    validtimes = extract_valid_times(weather.analysis());
  else
    validtimes = extract_valid_times(weather.forecast());

  if (options.debug)
  {
    std::cerr << "Available valid times:" << std::endl;
    BOOST_FOREACH (const boost::posix_time::ptime& validtime, validtimes)
      std::cerr << validtime << std::endl;
  }

  if (validtimes.size() != 1)
    throw std::runtime_error("Currently only one valid time can be rendered");

  boost::posix_time::ptime validtime = *validtimes.begin();

// Determine respective numerical model
//
// == Model not used anymore; background data is handled by frontier frontend ==

#ifdef CONTOURING

  boost::shared_ptr<NFmiQueryData> qd;

  if (!options.nocontours)
  {
    const woml::DataSource& dataSource = weather.analysis().dataSource();

    try
    {
      if (weather.hasAnalysis())
        qd = resolve_model(options, config, dataSource);
      else
        qd = resolve_model(options, config, dataSource);
    }
    catch (std::exception& e)
    {
      if (!options.quiet) std::cerr << "Warning: " << e.what() << std::endl;
    }

    if (!qd)
    {
      options.nocontours = true;

      if (!options.quiet)
      {
        const boost::optional<woml::NumericalModelRun>& modelRun = dataSource.numericalModelRun();
        const std::string& modelName = (modelRun ? modelRun->name() : "");
        const std::string& name =
            ((modelName.find_first_not_of(" ") != std::string::npos) ? modelName : "?");

        std::cerr << "Contouring omitted; model (" << name << ") not available" << std::endl;
      }
    }
  }

#endif

  frontier::SvgRenderer renderer(options, config, svg, area, validtime, &debugoutput);

#ifdef CONTOURING

  // Render contours

  if (!options.nocontours) renderer.contour(qd, validtime);

#endif

  // Synchronize some aerodrome forecast features (SurfaceWeather and SurfaceVisibility)
  // to have common time serie
  //
  // 22-Nov-2012: No sync
  //
  // if (options.doctype == woml::aerodromeforecast)
  // 	weather.synchronize();

  // Render picture header

  bool analysis = weather.hasAnalysis();

  std::list<woml::TargetRegion>::const_iterator trbeg(
      analysis ? weather.analysis().TargetRegions_begin()
               : weather.forecast().TargetRegions_begin());
  std::list<woml::TargetRegion>::const_iterator trend(
      analysis ? weather.analysis().TargetRegions_end() : weather.forecast().TargetRegions_end());

  std::list<std::pair<std::string, std::string> >::const_iterator lnbeg(
      trbeg->LocalizedNames_begin());
  std::list<std::pair<std::string, std::string> >::const_iterator lnend(
      trbeg->LocalizedNames_end());
  std::list<std::pair<std::string, std::string> >::const_iterator ribeg(trbeg->RegionIds_begin());
  std::list<std::pair<std::string, std::string> >::const_iterator riend(trbeg->RegionIds_end());

  std::string regionName((lnbeg != lnend) ? lnbeg->second : "");
  std::string regionId((ribeg != riend) ? ribeg->second : "");

  std::string creator(analysis ? weather.analysis().creator() : weather.forecast().creator());

  renderer.render_header(
      validtime,
      (analysis ? weather.analysis().validTime() : weather.forecast().validTime()),
      (analysis ? weather.analysis().analysisTime() : weather.forecast().forecastTime()),
      (analysis ? weather.analysis().creationTime() : weather.forecast().creationTime()),
      (analysis ? weather.analysis().latestModificationTime()
                : weather.forecast().latestModificationTime()),
      regionName,
      regionId,
      creator);

  // Render woml.
  // Some features (forecast/analysis shortInfo and longInfo texts) have no valid time

  if (weather.hasAnalysis())
  {
    BOOST_FOREACH (const woml::Feature& feature, weather.analysis())
    {
      const boost::optional<boost::posix_time::ptime> theTime = feature.validTime();

      if (theTime && (theTime->is_not_a_date_time() || (theTime == validtime)))
        feature.visit(renderer);
    }
  }
  else
  {
    BOOST_FOREACH (const woml::Feature& feature, weather.forecast())
    {
      const boost::optional<boost::posix_time::ptime> theTime = feature.validTime();

      if (theTime && (theTime->is_not_a_date_time() || (theTime == validtime)))
        feature.visit(renderer);
    }
  }

  // Output

  if (options.outfile != "-")
  {
    if (options.verbose) std::cerr << "Writing " << options.outfile << std::endl;
    writefile(options.outfile, renderer.svg());
  }
  else
  {
    if (options.verbose) std::cerr << "Writing to stdout" << std::endl;
    std::cout << renderer.svg();
  }
  return 0;
}

// ----------------------------------------------------------------------
/*!
 * \brief In debug mode generate a svg file containing debug/error message(s).
 *
 * Otherwise output the message(s) to stderr and return nonzero exit value.
 */
// ----------------------------------------------------------------------

void generatesvg(int exitCode,
                 const boost::shared_ptr<NFmiArea>& area,
                 const std::string& outfile,
                 bool debug,
                 std::ostringstream& debugoutput)
{
  if (!debug)
  {
    std::cerr << debugoutput.str();
    exit(exitCode);
  }

  std::string sbeg(
      "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
      "width=\"--WIDTH--px\" height=\"--HEIGHT--px\" viewBox=\"0 0 --WIDTH-- --HEIGHT--\"><text "
      "x=\"10\" y=\"10\">");
  std::string send("</text></svg>");

  int h = static_cast<int>(std::floor(0.5 + area->Height()));
  int w = static_cast<int>(std::floor(0.5 + area->Width()));

  std::string width = boost::lexical_cast<std::string>(w);
  std::string height = boost::lexical_cast<std::string>(h);

  boost::replace_all(sbeg, "--WIDTH--", width);
  boost::replace_all(sbeg, "--HEIGHT--", height);

  typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
  boost::char_separator<char> sep("\n");
  std::string s(debugoutput.str());
  tokenizer tok(s, sep);

  std::ostringstream output;

  for (tokenizer::iterator it = tok.begin(); it != tok.end(); ++it)
    output << "<tspan x=\"10\" dy=\"20\">" << *it << "</tspan>";

  if (outfile != "-")
  {
    writefile(outfile, sbeg + output.str() + send);
  }
  else
    std::cout << sbeg << output.str() << send;

  exit(0);
}

int main(int argc, char* argv[])
{
  std::string outfile;
  bool debug;
  std::ostringstream debugoutput;
  boost::shared_ptr<NFmiArea> area;

  try
  {
    run(argc, argv, area, outfile, debug, debugoutput);
    return 0;
  }
  catch (libconfig::ParseException& e)
  {
    debugoutput << "Frontier configuration parse error '" << e.getError() << "' at or near line "
                << e.getLine() << std::endl;
    generatesvg(1, area, outfile, debug, debugoutput);
  }
  catch (libconfig::ConfigException& e)
  {
    debugoutput << "Frontier configuration error" << std::endl;
    generatesvg(2, area, outfile, debug, debugoutput);
  }
  catch (std::exception& e)
  {
    debugoutput << "Error: " << e.what() << std::endl;
    generatesvg(3, area, outfile, debug, debugoutput);
  }
  catch (...)
  {
    debugoutput << "Error: Unknown exception occurred" << std::endl;
    generatesvg(4, area, outfile, debug, debugoutput);
  }
}
