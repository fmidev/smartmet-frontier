// ======================================================================
/*!
 * \brief frontier::FeatureRenderer
 */
// ======================================================================

#ifndef FRONTIER_FEATURERENDERER_H
#define FRONTIER_FEATURERENDERER_H

#include <smartmet/newbase/NFmiArea.h>
#include <smartmet/woml/FeatureVisitor.h>
#include <smartmet/woml/Point.h>
#include <boost/shared_ptr.hpp>
#include <sstream>
#include <string>

namespace frontier
{
  class FeatureRenderer : public woml::FeatureVisitor
  {
  public:

	virtual ~FeatureRenderer() {}
	FeatureRenderer(const std::string & theTemplate,
					const boost::shared_ptr<NFmiArea> & theArea);

	virtual void visit(const woml::CloudAreaBorder & theFeature);
    virtual void visit(const woml::ColdFront & theFeature);
    virtual void visit(const woml::Jet & theFeature);
    virtual void visit(const woml::OccludedFront & theFeature);
    virtual void visit(const woml::PointGeophysicalParameterValueSet & theFeature);
    virtual void visit(const woml::PointMeteorologicalSymbol & theFeature);
    virtual void visit(const woml::PointNote & theFeature);
    virtual void visit(const woml::SurfacePrecipitationArea & theFeature);
    virtual void visit(const woml::Trough & theFeature);
    virtual void visit(const woml::UpperTrough & theFeature);
    virtual void visit(const woml::WarmFront & theFeature);

	std::string svg() const;

  private:

	FeatureRenderer();
	woml::Point project(const woml::Point & latlon) const;
	std::string tostring(const woml::Point & xy) const;
	void moveto(std::ostringstream & out, const woml::Point & latlon);
	void lineto(std::ostringstream & out, const woml::Point & latlon);

	std::string svgbase;
	boost::shared_ptr<NFmiArea> area;

	// defs
	std::ostringstream masks;
	std::ostringstream paths;

	// body
	std::ostringstream cloudborders;
	std::ostringstream coldfronts;
	std::ostringstream jets;
	std::ostringstream occludedfronts;
	std::ostringstream pointnotes;
	std::ostringstream pointsymbols;
	std::ostringstream pointvalues;
	std::ostringstream precipitationareas;
	std::ostringstream troughs;
	std::ostringstream uppertroughs;
	std::ostringstream warmfronts;

	int ncloudborders;
	int ncoldfronts;
	int njets;
	int noccludedfronts;
	int npointnotes;
	int npointsymbols;
	int npointvalues;
	int nprecipitationareas;
	int ntroughs;
	int nuppertroughs;
	int nwarmfronts;

  }; // class FeatureRenderer


} // namespace frontier

#endif // FRONTIER_FEATURERENDERER_H
