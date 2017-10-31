// ======================================================================
/*!
 * \brief Command line options
 */
// ======================================================================

#include "Options.h"
#include <woml/Weather.h>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/program_options.hpp>
#include <iostream>

namespace frontier
{
// ----------------------------------------------------------------------
/*!
 * \brief Default options
 */
// ----------------------------------------------------------------------

Options::Options()
    : verbose(false),
      quiet(false),
      debug(false),
      projection(),
      svgfile(),
      womlfile(),
      outfile("-"),
      type(""),
      locale("")
{
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse the command line
 *
 * \return True, if execution may continue
 */
// ----------------------------------------------------------------------

bool parse_options(int argc, char* argv[], Options& theOptions)
{
  namespace po = boost::program_options;

  po::options_description desc("Allowed options");

  std::string msgsvg = ("SVG template file");
  std::string msgproj = ("projection description or file");
  std::string msgwoml = ("WOML file");
  std::string msgoutfile = ("output file, default=- (stdout)");
  std::string msgtype =
      ("Document type: 'conceptualmodelanalysis', 'conceptualmodelforecast' or "
       "'aerodromeforecast'");
  std::string msglocale = ("locale");

  bool dummy = false;

  desc.add_options()("help,h", "print out help message")(
      "debug,d", po::bool_switch(&theOptions.debug), "debug mode")(
      "verbose,v", po::bool_switch(&theOptions.verbose), "verbose mode")(
      "quiet,q", po::bool_switch(&theOptions.quiet), "quiet mode")("version,V",
                                                                   "display version number")(
      "woml,w", po::value(&theOptions.womlfile), msgwoml.c_str())(
      "svg,s", po::value(&theOptions.svgfile), msgsvg.c_str())(
      "proj,p", po::value(&theOptions.projection), msgproj.c_str())(
      "outfile,o", po::value(&theOptions.outfile), msgoutfile.c_str())(
      "type,t", po::value(&theOptions.type), msgtype.c_str())(
      "locale,l", po::value(&theOptions.locale), msglocale.c_str())(
      "nocontours,n", po::bool_switch(&dummy), "deprecated options which have no effect");

  po::positional_options_description p;
  p.add("woml", 1);

  po::variables_map opt;
  po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), opt);

  po::notify(opt);

  if (opt.count("version") != 0)
  {
    std::cout << "Frontier WOML renderer (" << argv[0] << ") (compiled on " << __DATE__ << ' '
              << __TIME__ << ')' << std::endl;

    // Do not continue the startup
    return false;
  }

  if (opt.count("help"))
  {
    std::cout << "Usage: " << argv[0] << " [options]" << std::endl
              << std::endl
              << desc << std::endl;

    return false;
  }

  if (dummy && !theOptions.quiet)
    std::cerr << "Warning:: option --nocontours (-n) is deprecated" << std::endl;

  if (theOptions.womlfile.empty()) throw std::runtime_error("WOML file not specified");

  if (theOptions.svgfile.empty()) throw std::runtime_error("SVG file not specified");

  if (theOptions.projection.empty()) throw std::runtime_error("Projection not specified");

  if (theOptions.quiet) theOptions.verbose = false;
  if (theOptions.verbose) theOptions.quiet = false;

  if (theOptions.type == "conceptualmodelanalysis")
    theOptions.doctype = woml::conceptualmodelanalysis;
  else if (theOptions.type == "conceptualmodelforecast")
    theOptions.doctype = woml::conceptualmodelforecast;
  else if (theOptions.type == "aerodromeforecast")
    theOptions.doctype = woml::aerodromeforecast;
  else if (theOptions.type == "")
    throw std::runtime_error("'type' option is missing");
  else
    throw std::runtime_error("'type' option is not valid");

  // The locale string is used for case insensitive comparisons (only).
  // In woml and templates locales are entered with hyphen (fi-FI); convert underscore

  boost::replace_first(theOptions.locale, "_", "-");
  boost::to_lower(theOptions.locale);

  return true;
}

}  // namespace frontier
