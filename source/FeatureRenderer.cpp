// ======================================================================
/*!
 * \brief frontier::FeatureRenderer
 */
// ======================================================================

#include "FeatureRenderer.h"
#include <smartmet/woml/ColdFront.h>
#include <smartmet/woml/Jet.h>
#include <smartmet/woml/OccludedFront.h>
#include <smartmet/woml/Trough.h>
#include <smartmet/woml/UpperTrough.h>
#include <smartmet/woml/WarmFront.h>
#include <boost/foreach.hpp>

namespace frontier
{

// ----------------------------------------------------------------------
/*!
 * \brief Project latlon to image
 */
// ----------------------------------------------------------------------

woml::Point FeatureRenderer::project(const woml::Point & thePoint)
{
  double x = (thePoint.lon()+70)*5;
  double y = 500-(thePoint.lat()-20)*9;
  return woml::Point(x,y);
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a CloudAreaBorder
 */
// ----------------------------------------------------------------------

void
FeatureRenderer::visit(const woml::CloudAreaBorder & theFeature)
{
  // TODO
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a ColdFront
 */
// ----------------------------------------------------------------------

void
FeatureRenderer::visit(const woml::ColdFront & theFeature)
{
  // TODO
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a Jet
 */
// ----------------------------------------------------------------------

void
FeatureRenderer::visit(const woml::Jet & theFeature)
{
  // TODO
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a OccludedFront
 */
// ----------------------------------------------------------------------

void
FeatureRenderer::visit(const woml::OccludedFront & theFeature)
{
  // TODO
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a PointGeophysicalParameterValueSet
 */
// ----------------------------------------------------------------------

void
FeatureRenderer::visit(const woml::PointGeophysicalParameterValueSet & theFeature)
{
  // TODO
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a PointMeteorologicalSymbol
 */
// ----------------------------------------------------------------------

void
FeatureRenderer::visit(const woml::PointMeteorologicalSymbol & theFeature)
{
  // TODO
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a PointNote
 */
// ----------------------------------------------------------------------

void
FeatureRenderer::visit(const woml::PointNote & theFeature)
{
  // TODO
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a SurfacePrecipitationArea
 */
// ----------------------------------------------------------------------

void
FeatureRenderer::visit(const woml::SurfacePrecipitationArea & theFeature)
{
  // TODO
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a Trough
 */
// ----------------------------------------------------------------------

void
FeatureRenderer::visit(const woml::Trough & theFeature)
{
  // TODO
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a UpperTrough
 */
// ----------------------------------------------------------------------

void
FeatureRenderer::visit(const woml::UpperTrough & theFeature)
{
  // TODO
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a WarmFront
 */
// ----------------------------------------------------------------------

void
FeatureRenderer::visit(const woml::WarmFront & theFeature)
{
  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  BOOST_FOREACH(const woml::SimpleCubicSpline & spline, splines)
	{
	  if(!spline.empty())
		{
#if 0
		  move_to(spline[0]);
		  for(woml::SimpleCubicSpline::size_type i=1; i<spline.size(); ++i)
			line_to(spline[i]);
#endif
		}
	}
}

} // namespace frontier
