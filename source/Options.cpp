// ======================================================================
/*!
 * \brief Command line options
 */
// ======================================================================

#include "Options.h"

#include <boost/program_options.hpp>
#include <boost/filesystem/operations.hpp>
#include <iostream>

namespace frontier {

// ----------------------------------------------------------------------
/*!
 * \brief Default options
 */
// ----------------------------------------------------------------------

  Options::Options()
	: verbose(false)
	, quiet(false)
	, debug(false)
	, projection()
	, svgfile()
	, womlfile()
	, outfile("-")
  {
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Parse the command line
   *
   * \return True, if execution may continue
   */
  // ----------------------------------------------------------------------


  bool parse_options(int argc, char * argv[], Options & theOptions)
  {
	namespace po = boost::program_options;
      
	po::options_description desc("Allowed options");

	std::string msgsvg = ("SVG template file");
	std::string msgproj = ("projection description or file");
	std::string msgwoml = ("WOML file");
	std::string msgoutfile = ("output file, default=- (stdout)");
	
	desc.add_options()
	  ("help,h","print out help message")
	  ("debug,d",po::bool_switch(&theOptions.debug),"debug mode")
	  ("verbose,v",po::bool_switch(&theOptions.verbose),"verbose mode")
	  ("quiet,q",po::bool_switch(&theOptions.quiet),"quiet mode")
	  ("version,V","display version number")
	  ("woml,w",po::value(&theOptions.womlfile),msgwoml.c_str())
	  ("svg,s",po::value(&theOptions.svgfile),msgsvg.c_str())
	  ("proj,p",po::value(&theOptions.projection),msgproj.c_str())
	  ("outfile,o",po::value(&theOptions.outfile),msgoutfile.c_str())
        ;
	
	po::positional_options_description p;
	p.add("woml",1);

	po::variables_map opt;
	po::store(po::command_line_parser(argc,argv)
			  .options(desc)
			  .positional(p)
			  .run(),
			  opt);
	
	po::notify(opt);
	
	if(opt.count("version") != 0)
	  {
		std::cout << "Frontier WOML renderer ("
				  << argv[0]
				  << ") (compiled on "
				  << __DATE__
				  << ' '
				  << __TIME__
				  << ')'
				  << std::endl;
		
		// Do not continue the startup
		return false;
	  }
	
	if(opt.count("help"))
	  {
		std::cout << "Usage: " << argv[0] << " [options]" << std::endl
				  << std::endl
				  << desc << std::endl;
		
		return false;
	  }

	if(theOptions.womlfile.empty())
	  throw std::runtime_error("WOML file not specified");

	if(theOptions.svgfile.empty())
	  throw std::runtime_error("SVG file not specified");

	if(theOptions.projection.empty())
	  throw std::runtime_error("Projection not specified");

	if(theOptions.quiet)   theOptions.verbose = false;
	if(theOptions.verbose) theOptions.quiet = false;

	return true;
  }
	
} // namespace frontier
