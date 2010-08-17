// ======================================================================
/*!
 * \brief WOML renderer
 */
// ======================================================================

#include "Options.h"
#include "FeatureRenderer.h"

#include <smartmet/woml/Parser.h>
#include <smartmet/woml/MeteorologicalAnalysis.h>
#include <smartmet/woml/WeatherForecast.h>

#include <cairomm/context.h>
#include <cairomm/surface.h>

#include <boost/foreach.hpp>

#include <iostream>
#include <stdexcept>

int run(int argc, char * argv[])
{
  Frontier::Options options;

  if(!parse_options(argc,argv,options))
    return 0;

  // Parse the WOML

  woml::Weather weather = woml::parse("test.xml");

  // Create image to write

  Cairo::RefPtr<Cairo::ImageSurface> surface =
	Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, 500*9/6, 500);

  Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(surface);
  cr->set_source_rgb(1,1,1);
  cr->paint();

  if(weather.empty())
	throw std::runtime_error("No MeteorologicalAnalysis to draw");

  const woml::MeteorologicalAnalysis & analysis = weather.analysis();
  frontier::FeatureRenderer renderer(cr);

  BOOST_FOREACH(const woml::Feature & feature, analysis)
	{
	  feature.visit(renderer);
	}

  std::cout << "Writing image.png" << std::endl;
  surface->write_to_png("image.png");

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
