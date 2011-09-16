// ======================================================================
/*!
 * \brief WOML renderer
 */
// ======================================================================

#include "ConfigTools.h"
#include "Options.h"
#include "SvgRenderer.h"

#include <smartmet/newbase/NFmiArea.h>
#include <smartmet/newbase/NFmiAreaFactory.h>
#include <smartmet/newbase/NFmiPreProcessor.h>
#include <smartmet/newbase/NFmiQueryData.h>

#include <smartmet/woml/DataSource.h>
#include <smartmet/woml/MeteorologicalAnalysis.h>
#include <smartmet/woml/Parser.h>
#include <smartmet/woml/WeatherForecast.h>

#include <libconfig.h++>

#include <boost/date_time/posix_time/posix_time.hpp>
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
 * \brief Convert NFmiMetTime to ptime
 */
// ----------------------------------------------------------------------

boost::posix_time::ptime to_ptime(const NFmiMetTime & theTime)
{
  boost::gregorian::date date(theTime.GetYear(),
                              theTime.GetMonth(),
                              theTime.GetDay());
  
  boost::posix_time::ptime utc(date,
                               boost::posix_time::hours(theTime.GetHour())+
                               boost::posix_time::minutes(theTime.GetMin())+
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

boost::shared_ptr<NFmiQueryData>
search_model_origintime(const frontier::Options & options,
						const std::string & path,
						const boost::posix_time::ptime & origintime)
{
  namespace fs = boost::filesystem;

  if(!fs::exists(path))
	throw std::runtime_error("Path '"+path+"' does not exist");

  if(!fs::is_directory(path))
	throw std::runtime_error("Path '"+path+"' is not a directory");

  fs::directory_iterator end_dir;
  for(fs::directory_iterator dirptr(path); dirptr!=end_dir; ++dirptr)
	{
	  if(!fs::is_regular_file(dirptr->status()))
		continue;

	  try
		{
		  NFmiQueryInfo qi(dirptr->path().string());

		  if(to_ptime(qi.OriginTime()) == origintime)
			{
			  if(options.debug)
				std::cerr << "File '"
						  << dirptr->path()
						  << "' matched origin time "
						  << to_simple_string(origintime)
						  << std::endl;

			  return boost::shared_ptr<NFmiQueryData>(new NFmiQueryData(dirptr->path().string()));
			}
		}
	  catch(...)
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

boost::shared_ptr<NFmiQueryData> resolve_model(const frontier::Options & options,
											   const libconfig::Config & config,
											   const woml::DataSource & source)
{
  boost::shared_ptr<NFmiQueryData> ret;

  if(!source.numericalModelRun())
	return ret;

  // Shorthand help variables

  const std::string & name = source.numericalModelRun()->name();
  const boost::posix_time::ptime & origintime = source.numericalModelRun()->originTime();

  std::string path = frontier::lookup<std::string>(config,"models."+name);

  if(path.empty())
	{
	  if(options.debug)
		{
		  if(options.verbose)
			std::cerr << "Ignoring unknown model '" + name +"'" << std::endl;
		  return ret;
		}
	  else
		{
		  throw std::runtime_error("Unknown model called '"+name+"' in WOML data");
		}
	}

  ret = search_model_origintime(options,path,origintime);

  if(!ret && !options.debug)
	throw std::runtime_error("Numerical model '"
							 + name
							 + "' referenced in data for origin time "
							 + to_simple_string(origintime)
							 + " was not found from directory "
							 + path);

  return ret;
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

  if(options.debug)
	{
	  std::cerr << "Available valid times:" << std::endl;
	  BOOST_FOREACH(const boost::posix_time::ptime & validtime, validtimes)
		std::cerr << validtime << std::endl;
	}

  if(validtimes.size() != 1)
	throw std::runtime_error("Currently only one valid time can be rendered");

  boost::posix_time::ptime validtime = *validtimes.begin();

  // Determine respective numerical model

  boost::shared_ptr<NFmiQueryData> qd;

  if(weather.hasAnalysis())
	qd = resolve_model(options,config, weather.analysis().dataSource());
  else
	qd = resolve_model(options,config, weather.forecast().dataSource());

  // Render contours

  frontier::SvgRenderer renderer(options, config, svg, area);
  renderer.contour(qd,validtime);

  // Render woml

  if(weather.hasAnalysis())
	{
	  BOOST_FOREACH(const woml::Feature & feature, weather.analysis())
		if(feature.validTime() == validtime)
		  feature.visit(renderer);
	}
  else
	{
	  BOOST_FOREACH(const woml::Feature & feature, weather.forecast())
		if(feature.validTime() == validtime)
		  feature.visit(renderer);
	}

  // Output

  if(options.outfile != "-")
	{
	  if(options.verbose)
		std::cerr << "Writing " << options.outfile << std::endl;
	  writefile(options.outfile,renderer.svg());
	}
  else
	{
	  if(options.verbose)
		std::cerr << "Writing to stdout" << std::endl;
	  std::cout << renderer.svg();
	}
  return 0;
}

int main(int argc, char * argv[])
try
  {
	return run(argc,argv);
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
 catch(std::exception & e)
  {
	std::cerr << "Error: " << e.what() << std::endl;
	return 1;
  }
 catch(...)
  {
	std::cerr << "Error: Unknown exception occurred" << std::endl;
	return 1;
  }
