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
  enum settings { s_optional = 0, s_required, s_base };

  class SettingIdNotFoundException
  {
	public:
		SettingIdNotFoundException(settings theId,const std::string & theMsg)
		{
			itsId = theId;
			itsMsg = theMsg;
		}

		settings id() const { return itsId; }
		const std::string & what() const { return itsMsg; }

	private:
		SettingIdNotFoundException();
		settings itsId;
		std::string itsMsg;
  };

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
		   const std::string & name,
		   settings settingId = s_required,
		   bool * isSet = NULL)
  {
	// If 's_required' setting is not found, std::runtime_error is thrown.
	//
	// If 's_optional' setting is not found, unset (default constructor) value is returned and
	// 'isSet' flag (if available) is set to false.
	//
	// If specific ('s_base' + n) setting is not found, SettingIdNotFoundException is thrown.

	T ret = T();
	bool bSet;
	bool *_isSet = &bSet;
	bool **pSet = (isSet ? &isSet : &_isSet);

	if((**pSet = setting.lookupValue(name,ret)) || (settingId == s_optional))
	  return ret;
	
	if(!setting.exists(name))
	  if ((settingId != s_optional) && (settingId != s_required))
	    throw SettingIdNotFoundException(settingId,prefix.empty() ? name : (prefix + "." + name));
	  else
	    throw std::runtime_error("Setting for "+(prefix.empty() ? name : (prefix + "." + name))+" is missing");
	
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

