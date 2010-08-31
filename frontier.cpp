// ======================================================================
/*!
 * \brief WOML renderer
 */
// ======================================================================

#include "Options.h"
#include "SvgRenderer.h"

#include <smartmet/newbase/NFmiAreaFactory.h>
#include <smartmet/newbase/NFmiPreProcessor.h>
#include <smartmet/woml/Parser.h>
#include <smartmet/woml/MeteorologicalAnalysis.h>
#include <smartmet/woml/WeatherForecast.h>

#include <libconfig.h++>

#include <boost/filesystem/operations.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>

#include <iostream>
#include <fstream>
#include <stdexcept>

// Does libconfig++ have readString?

#define NEWLIBCONFIG 0

#if !(NEWLIBCONFIG)
 #include <boost/lexical_cast.hpp>
 #include <fstream>
 #include <stdexcept>
 #include <sys/types.h>
 #include <unistd.h>
#endif

// ----------------------------------------------------------------------
/*!
 * \brief Read a file into a string
 */
// ----------------------------------------------------------------------

std::string readfile(const std::string & filename)
{
  std::ifstream in(filename.c_str());
  if(!in)
	throw std::runtime_error("Unable to open '"+filename+"' for reading");
  std::string ret;
  size_t sz = boost::filesystem::file_size(filename);
  ret.resize(sz);
  in.read(&ret[0],sz);
  return ret;
}

// ----------------------------------------------------------------------
/*!
 * \brief Write a string into a file
 */
// ----------------------------------------------------------------------

void writefile(const std::string & filename, const std::string & contents)
{
  std::ofstream out(filename.c_str());
  if(!out)
	throw std::runtime_error("Failed to open '"+filename+"' for writing");
  out << contents;
  out.close();
}

// ----------------------------------------------------------------------
/*!
 * \brief Read the configuration string
 */
// ---------------------------------------------------------------------- 

void readconfig(libconfig::Config & config,
				const std::string & contents)
{
#if NEWLIBCONFIG
  config.readString(contents);
#else
  std::string filename = ("/tmp/frontier_"
						  + boost::lexical_cast<std::string>(getpid())
						  + ".cnf");
  std::ofstream out(filename.c_str());
  if(!out)
	throw std::runtime_error("Failed to open '"+filename+"' for writing");
  out << contents;
  out.close();

  config.readFile(filename.c_str());

  if(unlink(filename.c_str()) != 0)
	throw std::runtime_error("Failed to delete '"+filename+"'");
#endif
}

// ----------------------------------------------------------------------
/*!
 * \brief Generate projection from a file
 */
// ----------------------------------------------------------------------

boost::shared_ptr<NFmiArea> readprojection(const std::string & filename)
{
  const bool strip_pound = false;
  NFmiPreProcessor processor(strip_pound);
  processor.SetIncluding("include","","");
  processor.SetDefine("#define");
  if(!processor.ReadAndStripFile(filename))
	throw std::runtime_error("Preprocessor failed to read '"+filename+"'");

  std::string text = processor.GetString();
  std::istringstream script(text);

  boost::shared_ptr<NFmiArea> area;

  std::string token;
  while(script >> token)
        {
          if(token == "#")
			script.ignore(1000000,'\n');
          else if(token == "projection")
			{
			  script >> token;
			  area = NFmiAreaFactory::Create(token);
			  break;
			}
          else
			;
        }
  
  if(!area)
	throw std::runtime_error("Failed to find a projection from '"+filename+"'");

  return area;
}

// ----------------------------------------------------------------------
/*!
 * \brief Extract section between given delimiters
 */
// ----------------------------------------------------------------------

std::string extract_section(const std::string & str,
							const std::string & startdelim,
							const std::string & enddelim)
{
  std::string::size_type pos1 = str.find(startdelim);
  if(pos1 == std::string::npos)
	return "";

  std::string::size_type pos2 = str.find(enddelim,pos1);
  if(pos2 == std::string::npos)
	return "";

  return str.substr(pos1+startdelim.size(),
					pos2-pos1-startdelim.size());
}

// ----------------------------------------------------------------------
/*!
 * \brief Remove sections between given delimiters
 */
// ----------------------------------------------------------------------

std::string remove_sections(const std::string & str,
							const std::string & startdelim,
							const std::string & enddelim)
{
  std::string ret = str;
  while(true)
	{
	  std::string::size_type pos1 = ret.find(startdelim);
	  if(pos1 == std::string::npos)
		return ret;

	  std::string::size_type pos2 = ret.find(enddelim,pos1);
	  if(pos2 == std::string::npos)
		return ret;
	  
	  std::string part1 = ret.substr(0,pos1);
	  std::string part2 = ret.substr(pos2+enddelim.size(),ret.size());
	  ret = part1+part2;
	}
}

// ----------------------------------------------------------------------
/*!
 * \brief Extract available validtimes
 */
// ----------------------------------------------------------------------

typedef std::set<boost::posix_time::ptime> ValidTimes;

template <typename T>
ValidTimes extract_valid_times(const T & collection)
{
  ValidTimes times;

  BOOST_FOREACH(const woml::Feature & f, collection)
	times.insert(f.validTime());

  return times;
}

// ----------------------------------------------------------------------
/*!
 * \brief Main program without exception handling
 */
// ----------------------------------------------------------------------

int run(int argc, char * argv[])
{
  frontier::Options options;

  if(!parse_options(argc,argv,options))
    return 0;

  // Read the SVG template

  std::string svg = readfile(options.svgfile);

  // Establish the projection

  boost::shared_ptr<NFmiArea> area;
  if(boost::filesystem::is_regular_file(options.projection))
	area = readprojection(options.projection);
  else
	area.reset(NFmiAreaFactory::Create(options.projection).release());

  // Parse the WOML

  woml::Weather weather = woml::parse(options.womlfile);
  if(weather.empty())
	throw std::runtime_error("No MeteorologicalAnalysis to draw");

  // Extract libconfig section

  std::string configstring = extract_section(svg,"<frontier>","</frontier>");

  // Remove comments

  svg = remove_sections(svg,"<!--","-->");
  svg = remove_sections(svg,"/*","*/");

  // Configure

  libconfig::Config config;
  readconfig(config,configstring);

  // Check what data is available

  if(weather.hasAnalysis() && weather.hasForecast())
	throw std::runtime_error("WOML data contains both analysis and forecast");

  // Extract available times

  ValidTimes validtimes;

  if(weather.hasAnalysis())
	validtimes = extract_valid_times(weather.analysis());
  else
	validtimes = extract_valid_times(weather.forecast());

  if(options.verbose)
	{
	  std::cerr << "Available valid times:" << std::endl;
	  BOOST_FOREACH(const boost::posix_time::ptime & validtime, validtimes)
		std::cerr << validtime << std::endl;
	}

  if(validtimes.size() != 1)
	throw std::runtime_error("Currently only one valid time can be rendered");

  // Render

  frontier::SvgRenderer renderer(options, config, svg, area);

  if(weather.hasAnalysis())
	{
	  BOOST_FOREACH(const woml::Feature & feature, weather.analysis())
		feature.visit(renderer);
	}
  else
	{
	  BOOST_FOREACH(const woml::Feature & feature, weather.forecast())
		feature.visit(renderer);
	}

  // Output

  if(options.outfile != "-")
	{
	  if(options.verbose)
		std::cout << "Writing " << options.outfile << std::endl;
	  writefile(options.outfile,renderer.svg());
	}
  else
	{
	  if(options.verbose)
		std::cerr << "Writing " << options.outfile << std::endl;
	  std::cout << renderer.svg();
	}
  return 0;
}

int main(int argc, char * argv[])
try
  {
	return run(argc,argv);
  }
catch(std::exception & e)
  {
	std::cerr << "Error: " << e.what() << std::endl;
	return 1;
  }
 catch(libconfig::ParseException & e)
   {
	 std::cerr << "Frontier configuration parse error '" << e.getError() << "'" << std::endl;
	 return 1;
   }
 catch(libconfig::ConfigException & e)
   {
	 std::cerr << "Frontier configuration error" << std::endl;
	 return 1;
   }
catch(...)
  {
	std::cerr << "Error: Unknown exception occurred" << std::endl;
	return 1;
  }
