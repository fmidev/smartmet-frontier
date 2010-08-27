// ======================================================================
/*!
 * \brief frontier::Config
 */
// ======================================================================

#include "Config.h"
#include <libconfig.h++>

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
	config.readString(theConfig);

  }


} // namespace frontier
