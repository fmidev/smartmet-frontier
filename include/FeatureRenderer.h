// ======================================================================
/*!
 * \brief frontier::FeatureRenderer
 */
// ======================================================================

#ifndef FRONTIER_FEATURERENDERER_H
#define FRONTIER_FEATURERENDERER_H

#include <smartmet/woml/FeatureVisitor.h>
#include <smartmet/woml/Point.h>
#include <cairomm/context.h>

namespace frontier
{
  class FeatureRenderer : public woml::FeatureVisitor
  {
  public:

	virtual ~FeatureRenderer() {}
	FeatureRenderer(Cairo::RefPtr<Cairo::Context> theCR);

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

  private:

	FeatureRenderer();
	woml::Point project(const woml::Point & thePoint);
	void move_to(const woml::Point & thePoint);
	void line_to(const woml::Point & thePoint);

	Cairo::RefPtr<Cairo::Context> itsCR;


  }; // class FeatureRenderer


} // nameaspace frontier

#endif // FRONTIER_FEATURERENDERER_H
