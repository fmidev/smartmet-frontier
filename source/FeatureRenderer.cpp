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
  // 2*(360x180)

  double x = (thePoint.lon())*4;
  double y = (90-thePoint.lat())*4;
  return woml::Point(x,y);
}

// ----------------------------------------------------------------------
/*!
 * \brief Move to given lat/lon
 */
// ----------------------------------------------------------------------

void FeatureRenderer::move_to(const woml::Point & thePoint)
{
  woml::Point xy = project(thePoint);
  itsCR->move_to(xy.lon(), xy.lat());
}

// ----------------------------------------------------------------------
/*!
 * \brief Draw line to given lat/lon
 */
// ----------------------------------------------------------------------

void FeatureRenderer::line_to(const woml::Point & thePoint)
{
  woml::Point xy = project(thePoint);
  itsCR->line_to(xy.lon(), xy.lat());
}

// ----------------------------------------------------------------------
/*!
 * \brief Constructor
 */
// ----------------------------------------------------------------------

FeatureRenderer::FeatureRenderer(Cairo::RefPtr<Cairo::Context> theCR)
  : itsCR(theCR)
{
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
  itsCR->save();
  itsCR->set_source_rgb(0,0,1);
  itsCR->set_line_width(10);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  BOOST_FOREACH(const woml::SimpleCubicSpline & spline, splines)
	{
	  if(!spline.empty())
		{
		  move_to(spline[0]);
		  for(woml::SimpleCubicSpline::size_type i=1; i<spline.size(); ++i)
			line_to(spline[i]);
		}
	}

  itsCR->stroke();
  itsCR->restore();
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
  itsCR->save();
  itsCR->set_source_rgb(1,0,0);
  itsCR->set_line_width(10);

  const woml::CubicSplineCurve splines = theFeature.controlCurve();

  BOOST_FOREACH(const woml::SimpleCubicSpline & spline, splines)
	{
	  if(!spline.empty())
		{
		  move_to(spline[0]);
		  for(woml::SimpleCubicSpline::size_type i=1; i<spline.size(); ++i)
			line_to(spline[i]);
		}
	}

  itsCR->stroke();
  itsCR->restore();
  // TODO
}

} // namespace frontier
