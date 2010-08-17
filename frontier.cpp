// ======================================================================
/*!
 * \brief WOML renderer
 */
// ======================================================================

#include "FeatureRenderer.h"
#include "Options.h"

#include <smartmet/woml/Parser.h>
#include <smartmet/woml/MeteorologicalAnalysis.h>
#include <smartmet/woml/WeatherForecast.h>

#include <boost/foreach.hpp>

#include <iostream>
#include <stdexcept>

int run(int argc, char * argv[])
{
  frontier::Options options;

  if(!parse_options(argc,argv,options))
    return 0;

  // Parse the WOML

  woml::Weather weather = woml::parse(options.womlfile);
  if(weather.empty())
	throw std::runtime_error("No MeteorologicalAnalysis to draw");

  const woml::MeteorologicalAnalysis & analysis = weather.analysis();
  frontier::FeatureRenderer renderer;

  BOOST_FOREACH(const woml::Feature & feature, analysis)
	{
	  feature.visit(renderer);
	}

  if(options.verbose)
	{
	  if(options.outfile != "-")
		std::cout << "Writing " << options.outfile << std::endl;
	  else
		std::cerr << "Writing " << options.outfile << std::endl;
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
