// ======================================================================
/*!
 * \file PreProcessor.cpp
 * \brief Derived interface of class NFmiPreProcessor
 */
// ======================================================================

#include "PreProcessor.h"

#include <iostream>
#include <utility>

using namespace std;

// ----------------------------------------------------------------------
/*!
 * \brief Constructor.
 */
// ----------------------------------------------------------------------

PreProcessor::PreProcessor(std::string theDefineDirective,
                           std::string theIncludeDirective,
                           std::string theIncludePathDefine,
                           std::string theIncludeFileExtension,
                           bool stripPound,
                           bool stripDoubleSlash,
                           bool stripSlashAst,
                           bool stripSlashAstNested,
                           bool stripEndOfLineSpaces)
    : NFmiPreProcessor(
          stripPound, stripDoubleSlash, stripSlashAst, stripSlashAstNested, stripEndOfLineSpaces),
      itsDefineDirective(std::move(theDefineDirective)),
      itsIncludeDirective(std::move(theIncludeDirective)),
      itsIncludePathDefine(std::move(theIncludePathDefine)),
      itsIncludeFileExtension(std::move(theIncludeFileExtension))
{
}

// ----------------------------------------------------------------------
/*!
 * \brief Read and strip a file.
 */
// ----------------------------------------------------------------------

bool PreProcessor::ReadAndStripFile(const std::string& theFileName)
{
  if (!ReadFile(theFileName))
    return false;

  SetDefine(itsDefineDirective, true);

  SetIncluding(itsIncludeDirective, getDefinedValue(itsIncludePathDefine), itsIncludeFileExtension);

  if (Strip())
    return ReplaceAll();

  return false;
}

// ----------------------------------------------------------------------
/*!
 * \brief Return value for given define key.
 *
 * \return The defined value or the given default if the value is not defined.
 */
// ----------------------------------------------------------------------

std::string PreProcessor::getDefinedValue(const std::string& theDefine,
                                                const std::string& theDefaultValue) const
{
  istringstream input(itsString);

  string line;
  while (getline(input, line))
  {
    istringstream lineinput(line);
    string token;
    lineinput >> token;
    if (token == itsDefineDirective)
    {
      // Extract the variable name and value
      string name;
      lineinput >> name;

      if (name == theDefine)
      {
        string value;
        getline(lineinput, value);
        NFmiStringTools::TrimL(value);
        NFmiStringTools::TrimR(value);
        return value;
      }
    }
  }

  return theDefaultValue;
}
