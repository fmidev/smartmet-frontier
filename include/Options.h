// ======================================================================
/*!
 * \brief Command line options
 */
// ======================================================================

#ifndef FRONTIER_OPTIONS_H
#define FRONTIER_OPTIONS_H

#include <string>
#include "Weather.h"

namespace frontier
{
  class Options
  {
  public:
	Options();

	// --verbose, --quiet and --debug

	bool verbose;
	bool quiet;
	bool debug;

	// -p projection | -p projectionfile

	std::string projection;				// the command line description

	// -s svgfile

	std::string svgfile;

	// -w womlfile

	std::string womlfile;

	// -o outfile

	std::string outfile;

	// -t type { conceptualmodelanalysis | conceptualmodelforecast | aerodromeforecast }

	std::string type;
	woml::documentType doctype;

	// -l locale; defaults to "" (using configuration blocks having empty ("") or non specified locale)

	std::string locale;

  }; // class Options

  bool parse_options(int argc, char * argv[], Options & theOptions);

} // namespace frontier

#endif // FRONTIER_OPTIONS_H
