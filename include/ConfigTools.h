// ======================================================================
/*!
 * \brief Tools for handling libconfig::Config object
 */
// ======================================================================

#ifndef FRONTIER_CONFIGTOOLS_H
#define FRONTIER_CONFIGTOOLS_H

#include <libconfig.h++>

#include <stdexcept>
#include <string>

#include <typeinfo>

namespace frontier
{
enum settings
{
  s_optional = 0,
  s_required,
  s_base
};

class SettingIdNotFoundException
{
 public:
  SettingIdNotFoundException(settings theId, const std::string& theMsg)
  {
    itsId = theId;
    itsMsg = theMsg;
  }

  settings id() const { return itsId; }
  const std::string& what() const { return itsMsg; }

 private:
  SettingIdNotFoundException();
  settings itsId;
  std::string itsMsg;
};
// ----------------------------------------------------------------------
/*!
 * \brief Type information
 */
// ----------------------------------------------------------------------

template <typename T>
inline const char* number_name()
{
  return "number";
}

template <>
inline const char* number_name<char>()
{
  return "char";
}
template <>
inline const char* number_name<int>()
{
  return "int";
}
template <>
inline const char* number_name<short>()
{
  return "short";
}
template <>
inline const char* number_name<long>()
{
  return "long";
}

template <>
inline const char* number_name<unsigned char>()
{
  return "unsigned char";
}
template <>
inline const char* number_name<unsigned int>()
{
  return "unsigned int";
}
template <>
inline const char* number_name<unsigned short>()
{
  return "unsigned short";
}
template <>
inline const char* number_name<unsigned long>()
{
  return "unsigned long";
}

template <>
inline const char* number_name<float>()
{
  return "float";
}
template <>
inline const char* number_name<double>()
{
  return "double";
}

template <typename T, typename T2>
std::string type_names()
{
  std::string t;

  if (typeid(T) == typeid(t))
    t = "string";
  else if (typeid(T) == typeid(bool))
    t = "bool";
  else
  {
    t = number_name<T>();

    if (typeid(T) != typeid(T2))
      t += (std::string(" or ") + std::string(number_name<T2>()));
  }

  return t;
}

template <typename T>
std::string type_name()
{
  return type_names<T,T>();
}

// ----------------------------------------------------------------------
/*!
 * \brief Read a configuration value with proper error messages
 */
// ----------------------------------------------------------------------

template <typename T>
T lookup(const libconfig::Config& config, const std::string& name)
{
  try
  {
    T value = config.lookup(name);
    return value;
  }
  catch (libconfig::ConfigException& e)
  {
    if (!config.exists(name)) throw std::runtime_error("Setting for " + name + " is missing");
    throw std::runtime_error("Failed to parse value of '" + name + "' as type " + type_name<T>());
  }
}

template <typename T>
T lookup(const libconfig::Setting& setting,
         const std::string& prefix,
         const std::string& name,
         settings settingId = s_required,
         bool* isSet = nullptr)
{
  // If 's_required' setting is not found, std::runtime_error is thrown.
  //
  // If 's_optional' setting is not found, unset (default constructor) value is returned and
  // 'isSet' flag (if available) is set to false.
  //
  // If specific ('s_base' + n) setting is not found, SettingIdNotFoundException is thrown.

  T ret = T();
  bool bSet;
  bool* _isSet = &bSet;
  bool** pSet = (isSet ? &isSet : &_isSet);
  std::string prefixName(prefix.empty() ? name : (prefix + "." + name));

  // lookupValue throws on missing setting etc, even though it shouldn't ?

  try
  {
    **pSet = setting.lookupValue(name, ret);
  }
  catch (const libconfig::SettingNotFoundException&)
  {
    **pSet = false;
  }
  catch (const libconfig::SettingTypeException&)
  {
    throw;
  }
  catch (const libconfig::ParseException&)
  {
    throw std::runtime_error("Failed to parse value of '" + prefixName + "' as type " +
                             type_name<T>());
  }
  catch (...)
  {
    throw std::runtime_error("Failed to get value of '" + prefixName + "'");
  }

  if (**pSet || (settingId == s_optional))
    return ret;

  if (settingId == s_required)
    throw std::runtime_error("Setting for " + prefixName + " is missing");
  else
    throw SettingIdNotFoundException(settingId, prefixName);
}

}  // namespace frontier

#endif  // FRONTIER_CONFIGTOOLS_H
