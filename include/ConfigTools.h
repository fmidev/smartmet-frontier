// ======================================================================
/*!
 * \brief Tools for handling libconfig::Config object
 */
// ======================================================================

#ifndef FRONTIER_CONFIGTOOLS_H
#define FRONTIER_CONFIGTOOLS_H

#include <smartmet/macgyver/Cast.h>

#include <libconfig.h++>

#include <stdexcept>
#include <string>


namespace frontier
{

  // ----------------------------------------------------------------------
  /*!
   * \brief Read a configuration value with proper error messages
   */
  // ----------------------------------------------------------------------

  template <typename T>
  T lookup(const libconfig::Config & config,
		   const std::string & name)
  {
	try
	  {
		T value = config.lookup(name);
		return value;
	  }
	catch(libconfig::ConfigException & e)
          {
			if(!config.exists(name))
			  throw std::runtime_error("Setting for "+name+" is missing");
			throw std::runtime_error("Failed to parse value of '"
									 + name
									 + "' as type "
									 + Fmi::number_name<T>());
          }
  }
  
  template <typename T>
  T lookup(const libconfig::Setting & setting,
		   const std::string & prefix,
		   const std::string & name)
  {
	T ret;
	
	if(setting.lookupValue(name,ret))
	  return ret;
	
	if(!setting.exists(name))
	  throw std::runtime_error("Setting for "+name+" is missing");
	
	if(!prefix.empty())
	  throw std::runtime_error("Failed to parse value of '"
							   + (prefix + "." + name)
							   + "' as type "
							   + Fmi::number_name<T>());
	else
	  throw std::runtime_error("Failed to parse value of '"
							   + name
							   + "' as type "
							   + Fmi::number_name<T>());
  }

} // namespace frontier

#endif // FRONTIER_CONFIGTOOLS_H

