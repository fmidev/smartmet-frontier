// ======================================================================
/*!
 * \brief Command line options
 */
// ======================================================================

#include "Options.h"

#include <boost/program_options.hpp>
#include <boost/filesystem/operations.hpp>
#include <iostream>

namespace Frontier {

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
	, defsfile()
	, womlfile()
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

	std::string msgdefs = ("set the SVG defs file");
	std::string msgproj = ("set the projection description or file");
	std::string msgwoml = ("set the WOML file");
	
	desc.add_options()
	  ("help,h","print out help message")
	  ("debug,d",po::bool_switch(&theOptions.debug),"set debug mode on")
	  ("verbose,v",po::bool_switch(&theOptions.verbose),"set verbose mode on")
	  ("quiet,q",po::bool_switch(&theOptions.quiet),"set quiet mode on")
	  ("version,V","display version number")
	  ("woml,w",po::value(&theOptions.womlfile),msgwoml.c_str())
	  ("defs,d",po::value(&theOptions.defsfile),msgdefs.c_str())
	  ("proj,p",po::value(&theOptions.projection),msgproj.c_str())
        ;
	
	po::positional_options_description p;
	p.add("projfile",1);
	p.add("defsfile",2);
	p.add("womlfile",3);

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

	if(theOptions.projection.empty())
	  throw std::runtime_error("Projection not specified");

	if(theOptions.defsfile.empty())
	  throw std::runtime_error("SVG defs file not specified");

	if(theOptions.womlfile.empty())
	  throw std::runtime_error("WOML file not specified");

	return true;
  }
	
} // namespace Frontier
