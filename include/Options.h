// ======================================================================
/*!
 * \brief Command line options
 */
// ======================================================================

#ifndef FRONTIER_OPTIONS_H
#define FRONTIER_OPTIONS_H

#include <string>

namespace Frontier
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

	// -d defsfile

	std::string defsfile;

	// -w womlfile

	std::string womlfile;

  }; // class Options

  bool parse_options(int argc, char * argv[], Options & theOptions);

} // namespace Frontier

#endif // FRONTIER_OPTIONS_H
