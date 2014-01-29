// ======================================================================
/*!
 * \file PreProcessor.h
 * \brief Derived interface of class NFmiPreProcessor
 */
// ======================================================================

#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <smartmet/newbase/NFmiPreProcessor.h>

#include <string>

class PreProcessor : public NFmiPreProcessor
{

public:

  PreProcessor(const std::string & theDefineDirective,
		  	   const std::string & theIncludeDirective = "",
		  	   const std::string & theIncludePathDefine = "",
		  	   const std::string & theIncludeFileExtension = "",
		  	   bool stripPound = false,
		  	   bool stripDoubleSlash = true,
		  	   bool stripSlashAst = true,
		  	   bool stripSlashAstNested = true,
		  	   bool stripEndOfLineSpaces = true);

  bool ReadAndStripFile(const std::string & theFileName);

  const std::string getDefinedValue(const std::string & theDefine,const std::string & theDefaultValue = "") const;

private:

  PreProcessor();

  std::string itsDefineDirective;
  std::string itsIncludeDirective;
  std::string itsIncludePathDefine;
  std::string itsIncludeFileExtension;

};

#endif // PREPOCESSOR_H

// ======================================================================
