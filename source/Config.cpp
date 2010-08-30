// ======================================================================
/*!
 * \brief frontier::Config
 */
// ======================================================================

#include "Config.h"
#include <libconfig.h++>

#define NEWLIBCONFIG 0

#if !(NEWLIBCONFIG)
 #include <boost/lexical_cast.hpp>
 #include <fstream>
 #include <stdexcept>
 #include <sys/types.h>
 #include <unistd.h>
#endif

namespace frontier
{
  // ----------------------------------------------------------------------
  /*!
   * \brief Construct from string
   */
  // ----------------------------------------------------------------------

  Config::Config(const std::string & theConfig)
  {
	libconfig::Config config;
#if NEWLIBCONFIG
	// A newer libconfig++ would support this:
	config.readString(theConfig);
#else
	std::string filename = ("/tmp/frontier_"
							+ boost::lexical_cast<std::string>(getpid())
							+ ".cnf");
	std::ofstream out(filename.c_str());
	if(!out)
	  throw std::runtime_error("Failed to open '"+filename+"' for writing");
	out << theConfig;
	out.close();

	config.readFile(filename.c_str());

	if(unlink(filename.c_str()) != 0)
	  throw std::runtime_error("Failed to delete '"+filename+"'");
	
#endif

  }


} // namespace frontier
