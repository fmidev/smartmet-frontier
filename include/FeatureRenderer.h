// ======================================================================
/*!
 * \brief frontier::FeatureRenderer
 */
// ======================================================================

#ifndef FRONTIER_FEATURERENDERER_H
#define FRONTIER_FEATURERENDERER_H

#include <smartmet/woml/FeatureVisitor.h>
#include <smartmet/woml/Point.h>

namespace frontier
{
  class FeatureRenderer : public woml::FeatureVisitor
  {
  public:

	virtual ~FeatureRenderer() {}

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

	woml::Point project(const woml::Point & thePoint);

  }; // class FeatureRenderer


} // namespace frontier

#endif // FRONTIER_FEATURERENDERER_H
