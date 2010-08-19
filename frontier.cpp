// ======================================================================
/*!
 * \brief WOML renderer
 */
// ======================================================================

#include "SvgRenderer.h"
#include "Options.h"

#include <smartmet/newbase/NFmiAreaFactory.h>
#include <smartmet/newbase/NFmiPreProcessor.h>
#include <smartmet/woml/Parser.h>
#include <smartmet/woml/MeteorologicalAnalysis.h>
#include <smartmet/woml/WeatherForecast.h>

#include <boost/filesystem/operations.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>

#include <iostream>
#include <fstream>
#include <stdexcept>

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
 * \brief Main program without exception handling
 */
// ----------------------------------------------------------------------

int run(int argc, char * argv[])
{
  frontier::Options options;

  if(!parse_options(argc,argv,options))
    return 0;

  // Read the SVG template and the projection

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

  // TODO PART: HANDLE ANALYSIS/FORECAST

  const woml::MeteorologicalAnalysis & analysis = weather.analysis();
  frontier::SvgRenderer renderer(svg,area);

  BOOST_FOREACH(const woml::Feature & feature, analysis)
	{
	  feature.visit(renderer);
	}

  if(options.outfile != "-")
	{
	  if(options.verbose)
		std::cout << "Writing " << options.outfile << std::endl;
	  std::ofstream out(options.outfile.c_str());
	  if(!out)
		throw std::runtime_error("Failed to open '"+options.outfile+"' for writing");
	  out << renderer.svg();
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
catch(...)
  {
	std::cerr << "Error: Unknown exception occurred" << std::endl;
	return 1;
  }
