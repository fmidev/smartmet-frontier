// ======================================================================
/*!
 * \brief frontier::SvgRenderer
 */
// ======================================================================

#ifndef FRONTIER_SVGRENDERER_H
#define FRONTIER_SVGRENDERER_H

#include "BezSeg.h"
#include "Options.h"
#include "Path.h"

#include "smartmet/woml/FeatureVisitor.h"
#include <smartmet/woml/GeophysicalParameterValueSet.h>
#include <smartmet/woml/MeasureValue.h>
#include <smartmet/woml/Point.h>
#include <smartmet/woml/TimeSeriesSlot.h>

#include <macgyver/DateTime.h>
#include <boost/ptr_container/ptr_map.hpp>
#include <memory>

#include <libconfig.h++>

#include <functional>
#include <sstream>
#include <string>

class NFmiArea;
class NFmiQueryData;

namespace frontier
{
class Elevation
{
 public:
  Elevation(double theElevation,
            double theScale,
            int theScaledElevation,
            const std::string &theLLabel,
            const std::string &theRLabel,
            bool line = true);
  Elevation(double theElevation);

  bool operator<(const Elevation &theOther) const;

  double elevation() const { return itsElevation; }
  double scale() const { return itsScale; }
  int scaledElevation() const { return itsScaledElevation; }
  const std::string &lLabel() const { return itsLLabel; }
  const std::string &rLabel() const { return itsRLabel; }
  void factor(double theFactor) { itsFactor = theFactor; }
  double factor() const { return itsFactor; }
  bool line() const { return renderLine; }

 private:
  Elevation();

  double itsElevation;
  double itsScale;
  int itsScaledElevation;
  std::string itsLLabel;
  std::string itsRLabel;
  double itsFactor;  // Factor for linear interpolation between this and the next elevation
  bool renderLine;   // If set, render the elevation line
};

class AxisManager
{
 public:
  AxisManager(const libconfig::Config &config);

  // Y -axis height
  double axisHeight() const { return itsAxisHeight; }
  // X -axis width
  void axisWidth(double theWidth) { itsAxisWidth = theWidth; }
  double axisWidth() const { return itsAxisWidth; }
  // Time period
  void timePeriod(const Fmi::TimePeriod &theTimePeriod)
  {
    itsTimePeriod = theTimePeriod;
  }
  const Fmi::TimePeriod &timePeriod() { return itsTimePeriod; }
  // X -axis offset
  double xOffset(const Fmi::DateTime &validTime) const;

  // X -axis step
  double xStep() const;

  // utc
  void utc(bool theUtc) { itsUtc = theUtc; }
  bool utc() const { return itsUtc; }
  // Elevations
  const std::list<Elevation> &elevations() { return itsElevations; }
  // Scaled elevation
  double scaledElevation(double elevation,
                         bool *above = nullptr,
                         double belowZero = 50.0,
                         double aboveTop = 100.0);

  // Min elevation value to be taken as nonzero
  double nonZeroElevation() { return 10.0; }

 private:
  double itsAxisHeight;
  double itsMinElevation;
  double itsMaxElevation;
  double itsAxisWidth;
  bool itsUtc;
  Fmi::TimePeriod itsTimePeriod;

  std::list<Elevation> itsElevations;
};

class ConfigGroup
{
 public:
  ConfigGroup(const std::string &theClassDef,
              const std::string &theMemberTypes,
              const std::string *theLabel,
              bool bbCenterMarker,
              const std::string &thePlaceHolder,
              const std::string &theMarkerPlaceHolder,
              bool combined,
              double theXOffset,
              double theYOffset,
              double theVOffset,
              double theVSOffset,
              double theVSSOffset,
              int theSOffset,
              int theEOffset,
              std::set<size_t> &theMemberSet,
              const libconfig::Setting *localScope,
              const libconfig::Setting *globalScope = nullptr,
              const std::list<const libconfig::Setting *> *scope = nullptr);

  const std::string &classDef() const { return itsClass; }
  const std::string &textClassDef() const { return itsTextClass; }
  std::string label(bool allTypes = false) const
  {
    return (hasLabel ? itsLabel : memberTypes(allTypes));
  }
  bool bbCenterMarker() const { return bbCenterMarkerPos; }
  const std::string &placeHolder() const { return itsPlaceHolder; }
  const std::string &markerPlaceHolder() const { return itsMarkerPlaceHolder; }
  bool standalone() const { return itsStandalone; }
  bool contains(const std::string &theMemberType, bool only = false) const;
  void addType(const std::string &type) const;
  std::string memberTypes(bool allTypes = false) const;

  double xOffset() const { return itsXOffset; }
  double yOffset() const { return itsYOffset; }
  double vOffset() const { return itsVOffset; }
  double vSOffset() const { return itsVSOffset; }
  double vSSOffset() const { return itsVSSOffset; }
  int sOffset() const { return itsSOffset; }
  int eOffset() const { return itsEOffset; }
  std::set<size_t> &memberSet() const { return itsMemberSet; }
  const libconfig::Setting *localScope() const { return itsLocalScope; }
  const libconfig::Setting *globalScope() const { return itsGlobalScope; }
  const std::list<const libconfig::Setting *> &scope() const { return itsScope; };

 private:
  ConfigGroup();

  std::string itsClass;
  std::string itsTextClass;
  std::string itsMemberTypes;
  std::string itsLabel;
  bool hasLabel;
  bool bbCenterMarkerPos;
  std::string itsPlaceHolder;
  std::string itsMarkerPlaceHolder;
  bool itsStandalone;

  double itsXOffset;
  double itsYOffset;
  double itsVOffset;
  double itsVSOffset;
  double itsVSSOffset;
  int itsSOffset;
  int itsEOffset;

  std::set<size_t> &itsMemberSet;  // Types contained in the current group

  const libconfig::Setting *itsLocalScope;         // Group's configuration block
  const libconfig::Setting *itsGlobalScope;        // Group's global configuration block (if any)
  std::list<const libconfig::Setting *> itsScope;  // Group's configuration blocks; first all global
                                                   // blocks (if any), then local/current block
};

struct MemberType : public std::function<void(ConfigGroup, std::string)>
{
  bool operator()(const ConfigGroup &configGroup, const std::string &memberType) const;
};

template <typename T>
struct GroupType : public std::function<void(ConfigGroup, std::string)>
{
  bool operator()(const ConfigGroup &configGroup, const std::string &memberTypes) const;
};

class CloudGroup : public ConfigGroup
{
 public:
  CloudGroup(const std::string &theClassDef,
             const std::string &theCloudTypes,
             const std::string &theSymbolType,
             const std::string *theLabel,
             bool bbCenterLabel,
             const std::string &thePlaceHolder,
             const std::string &theLabelPlaceHolder,
             bool combined,
             unsigned int theBaseStep,
             unsigned int theMaxRand,
             unsigned int theMaxRepeat,
             int theScaleHeightMin,
             int theScaleHeightRandom,
             int theControlMin,
             int theControlRandom,
             double theXOffset,
             double theYOffset,
             double theVOffset,
             double theVSOffset,
             double theVSSOffset,
             int theSOffset,
             int theEOffset,
             std::set<size_t> &theCloudSet,
             const std::list<const libconfig::Setting *> &theScope);

  const std::string &symbolType() const { return itsSymbolType; }
  unsigned int baseStep() const { return itsBaseStep; }
  unsigned int maxRand() const { return itsMaxRand; }
  unsigned int maxRepeat() const { return itsMaxRepeat; }
  int scaleHeightMin() const { return itsScaleHeightMin; }
  int scaleHeightRandom() const { return itsScaleHeightRandom; }
  int controlMin() const { return itsControlMin; }
  int controlRandom() const { return itsControlRandom; }

 private:
  CloudGroup();

  std::string itsSymbolType;

  unsigned int itsBaseStep;
  unsigned int itsMaxRand;
  unsigned int itsMaxRepeat;

  int itsScaleHeightMin;
  int itsScaleHeightRandom;
  int itsControlMin;
  int itsControlRandom;
};

class IcingGroup : public ConfigGroup
{
 public:
  IcingGroup(const std::string &theClassDef,
             const std::string &theIcingTypes,
             const std::string *theSymbol,
             bool symbolOnly,
             const std::string *theLabel,
             bool bbCenterLabel,
             const std::string &thePlaceHolder,
             const std::string &theLabelPlaceHolder,
             bool combined,
             double theXOffset,
             double theYOffset,
             double theVOffset,
             double theVSOffset,
             double theVSSOffset,
             int theSOffset,
             int theEOffset,
             std::set<size_t> &theIcingSet,
             const libconfig::Setting *localScope,
             const libconfig::Setting *globalScope = nullptr);

  const std::string &symbol() const { return itsSymbol; }
  bool symbolOnly() const { return renderSymbolOnly; }

 private:
  IcingGroup();

  std::string itsSymbol;
  bool renderSymbolOnly;
};

typedef IcingGroup TurbulenceGroup;

class CategoryValueMeasureGroup
{
 public:
  virtual ~CategoryValueMeasureGroup() {}
  CategoryValueMeasureGroup() : itsFirstMember(nullptr) {}
  virtual bool groupMember(const woml::CategoryValueMeasure *cvm) const { return true; }
  virtual bool groupMember(bool first, const woml::CategoryValueMeasure *cvm);
  virtual bool standalone() { return false; }

 protected:
  const woml::CategoryValueMeasure *itsFirstMember;
};

template <typename T>
class GroupCategory : public CategoryValueMeasureGroup
{
 public:
  GroupCategory();

  typename std::list<T> &groups() { return itsGroups; }
  typename std::list<T>::const_iterator currentGroup() { return itcg; }
  bool groupMember(const woml::CategoryValueMeasure *cvm) const;
  bool groupMember(bool first, const woml::CategoryValueMeasure *cvm);
  bool standalone() { return ((itsGroups.size() > 0) && (*itcg).standalone()); }
  const std::string &groupSymbol() const;
  std::string groupLabel() const;

 private:
  typename std::list<T> itsGroups;
  typename std::list<T>::const_iterator itcg;
};

typedef GroupCategory<CloudGroup> CloudGroupCategory;
typedef GroupCategory<IcingGroup> IcingGroupCategory;
typedef GroupCategory<TurbulenceGroup> TurbulenceGroupCategory;

class ElevationGroupItem
{
 public:
  ElevationGroupItem(
      Fmi::DateTime theValidTime,
      const std::list<std::shared_ptr<woml::GeophysicalParameterValueSet> >::const_iterator
          &thePvs,
      const std::list<woml::GeophysicalParameterValue>::iterator &thePv);

  const Fmi::DateTime &validTime() const { return itsValidTime; }
  const std::list<std::shared_ptr<woml::GeophysicalParameterValueSet> >::const_iterator &Pvs()
      const
  {
    return itsPvs;
  }
  const std::list<woml::GeophysicalParameterValue>::iterator &Pv() const;
  void topConnected(bool connected) { itsTopConnected = connected; }
  bool topConnected() const { return itsTopConnected; }
  void bottomConnected(bool connected) { itsBottomConnected = connected; }
  bool bottomConnected() const { return itsBottomConnected; }
  void leftOpen(bool open) { itsLeftOpen = open; }
  bool leftOpen() const { return itsLeftOpen; }
  void bottomConnection(woml::Elevation theBottomConnection)
  {
    itsBottomConnection = theBottomConnection;
  }
  const std::optional<woml::Elevation> &bottomConnection() const { return itsBottomConnection; }
  void isScanned(bool scanned);
  bool isScanned() const;
  void isGroundConnected(bool connected);
  bool isGroundConnected() const;
  void isNegative(bool negative);
  bool isNegative() const;
  void hasHole(bool hole);
  bool hasHole() const;
  void isHole(bool hole);
  bool isHole() const;
  bool assHole()
  { /* K.W.H */
    return (hasHole() && isHole());
  }
  void isGenerated(bool generated) { itsGenerated = generated; }
  bool isGenerated() const { return itsGenerated; }
  void isDeleted(bool deleted) { itsDeleted = deleted; }
  bool isDeleted() const { return itsDeleted; }
  void groupNumber(unsigned int group);
  unsigned int groupNumber() const;
  void elevation(std::optional<woml::Elevation> &theElevation) { itsElevation = theElevation; }
  const woml::Elevation &elevation() const
  {
    return (itsElevation ? *itsElevation : Pv()->elevation());
  }

  enum elevationFlags
  {
    b_scanned = 1,
    b_groundConnected = (1 << 1),
    b_negative = (1 << 2),
    b_hasHole = (1 << 3),
    b_isHole = (1 << 4)
  };

 private:
  ElevationGroupItem();

  Fmi::DateTime itsValidTime;
  std::list<std::shared_ptr<woml::GeophysicalParameterValueSet> >::const_iterator itsPvs;
  std::list<woml::GeophysicalParameterValue>::iterator itsPv;
  bool itsTopConnected;
  bool itsBottomConnected;
  bool itsLeftOpen;
  std::optional<woml::Elevation> itsBottomConnection;  // Rigth side elevation
  bool itsGenerated;                                     // Set for generated (below 0) elevation
  bool itsDeleted;  // Set when elevation is deleted from the underlying woml object collection
  std::optional<woml::Elevation> itsElevation;  // ZeroTolerance; generated below zero elevation
};

typedef std::list<ElevationGroupItem> ElevGrp;

struct ElevationHole
{
  ElevationHole()
      : aboveElev(),
        belowElev(),
        leftClosed(false),
        leftAboveBounded(false),
        rightClosed(false),
        rightAboveBounded(false)
  {
  }

  bool operator<(const ElevationHole &theOther) const;

  ElevGrp::iterator aboveElev;  // Elevation above the hole
  ElevGrp::iterator belowElev;  // Elevation below the hole
  bool leftClosed;  // Set if the hole is closed by an elevation or by left side closed hole on the
                    // left side
  bool leftAboveBounded;  // Set if the hole is bounded by an elevation above on the left side
  bool rightClosed;  // Set if the hole is closed by an elevation or by right side closed hole on
                     // the right side
  bool rightAboveBounded;  // Set if the hole is bounded by an elevation above on the right side
  double lo;               // Lo range of the hole (hi range of the elevation below the hole)
  double loLo;             // Lo range of the elevation below the hole
  double hi;               // Hi range of the hole (lo range of the elevation above the hole)
  double hiHi;             // Hi range of the elevation above the hole
};

typedef std::list<ElevationHole> ElevationHoles;

struct WindArrowOffsets
{
  WindArrowOffsets() : horizontalOffsetPx(0), verticalOffsetPx(0), scale(0.0) {}
  int horizontalOffsetPx;
  int verticalOffsetPx;
  double scale;
};

typedef boost::ptr_map<std::string, std::ostringstream> Texts;

typedef struct
{
  std::string areaId;              // Area id for area reservation
  std::list<std::string> markers;  // Marker names of reserved fill areas for candidate fill areas
  double x;
  double y;       // Marker position for reserved fill area (the nearest (free) fill area is used)
  double scale;   // Configured marker scale for reserved fill area
  bool centered;  // Set for reserved fill area if the marker is centered to the selected position
                  // (set for symbols)
  NFmiFillAreas fillAreas;                       // One reserved or all free/candidate fill areas
  std::list<std::pair<double, double> > scales;  // Additional marker scales to scale the marker
                                                 // down (to half size) if necessary to make it fit
} FillAreaData;
typedef std::map<std::string, FillAreaData> FillAreas;  // Marker name and related fill areas

class SvgRenderer : public woml::FeatureVisitor
{
 public:
  virtual ~SvgRenderer() {}
  SvgRenderer(const Options &theOptions,
              const libconfig::Config &theConfig,
              const std::string &theTemplate,
              const std::shared_ptr<NFmiArea> &theArea,
              const Fmi::DateTime &theValidTime,
              std::ostringstream *theDebugOutput = nullptr);

  virtual void visit(const woml::CloudArea &theFeature);
  virtual void visit(const woml::ColdAdvection &theFeature);
  virtual void visit(const woml::ColdFront &theFeature);
  virtual void visit(const woml::JetStream &theFeature);
  virtual void visit(const woml::OccludedFront &theFeature);
  virtual void visit(const woml::PointMeteorologicalSymbol &theFeature);
  virtual void visit(const woml::Ridge &theFeature);
  virtual void visit(const woml::SurfacePrecipitationArea &theFeature);
  virtual void visit(const woml::ParameterValueSetArea &theFeature);
  virtual void visit(const woml::Trough &theFeature);
  virtual void visit(const woml::UpperTrough &theFeature);
  virtual void visit(const woml::WarmAdvection &theFeature);
  virtual void visit(const woml::WarmFront &theFeature);

  virtual void visit(const woml::AntiCyclone &theFeature);
  virtual void visit(const woml::Antimesocyclone &theFeature);
  virtual void visit(const woml::Cyclone &theFeature);
  virtual void visit(const woml::HighPressureCenter &theFeature);
  virtual void visit(const woml::LowPressureCenter &theFeature);
  virtual void visit(const woml::Mesocyclone &theFeature);
  virtual void visit(const woml::Mesolow &theFeature);
  virtual void visit(const woml::ParameterValueSetPoint &theFeature);
  virtual void visit(const woml::PolarCyclone &theFeature);
  virtual void visit(const woml::PolarLow &theFeature);
  virtual void visit(const woml::TropicalCyclone &theFeature);
  virtual void visit(const woml::ConvectiveStorm &theFeature);
  virtual void visit(const woml::Storm &theFeature);

  virtual void visit(const woml::InfoText &theFeature);

  virtual void visit(const woml::CloudLayers &theFeature);
  virtual void visit(const woml::Contrails &theFeature);
  virtual void visit(const woml::Icing &theFeature);
  virtual void visit(const woml::Turbulence &theFeature);
  virtual void visit(const woml::MigratoryBirds &theFeature);
  virtual void visit(const woml::SurfaceVisibility &theFeature);
  virtual void visit(const woml::SurfaceWeather &theFeature);
  virtual void visit(const woml::Winds &theFeature);
  virtual void visit(const woml::ZeroTolerance &theFeature);

  void contour(const std::shared_ptr<NFmiQueryData> &theQD,
               const Fmi::DateTime &theTime);

  void render_header(Fmi::DateTime &validTime,
                     const Fmi::TimePeriod &timePeriod,
                     const Fmi::DateTime &forecastTime,
                     const Fmi::DateTime &creationTime,
                     const std::optional<Fmi::DateTime> &modificationTime,
                     const std::string &regionName,
                     const std::string &regionId,
                     const std::string &creator);

  std::string svg() const;

 private:
  SvgRenderer();

  bool hasCssClass(const std::string &theCssClass) const;

  double getCssSize(const std::string &theCssClass,
                    const std::string &theAttribute,
                    double defaultValue = 30);
  template <typename T>
  T getSetting(const std::string &theConfigClass,
               const std::string &theAttribute,
               T theDefaultValue);

  void render_header(const std::string &hdrClass,
                     const Fmi::DateTime &datetime,
                     bool useDate = false,
                     const std::string &text = "",
                     const std::string &confPath = "Header");
  void render_text(Texts &texts,
                   const std::string &confPath,
                   const std::string &textName,
                   const std::string &text,
                   int &xPosW,
                   int &yPosH,  // I: text's starting x/y pos O: text's width/height
                   bool ignoreErr = true,
                   bool setTextArea = true,
                   bool centerToStartX = false,
                   const std::string &TEXTPOSid = "",
                   bool useTextName = false,
                   int *maxTextWidth = nullptr,  // I
                   int *fontSize = nullptr,      // I
                   int *tXOffset = nullptr,      // I/O: area text's border x -offset
                   int *tYOffset = nullptr);     // I/O: area text's border y -offset
  void render_surface(Path &path,
                      std::ostringstream &surfaceOutput,
                      const std::string &id,
                      const std::string &surfaceName,
                      const woml::Feature *feature = nullptr,
                      const std::list<std::string> *areaSymbols = nullptr);
  void render_aerodromeSymbol(const std::string &confPath,
                              const std::string &symClass,
                              const std::string &classNameExt,
                              const std::string &value,
                              double x,
                              double y,
                              bool codeValue = true,
                              bool mappedCode = false,
                              double xScale = -1.0,
                              double yScale = -1.0,
                              const std::string symbolPosIdN = "");
  template <typename T>
  void render_aerodromeSymbols(const T &theFeature,
                               const std::string &confPath,
                               bool setVisibility = false);
  void render_symbol(const std::string &path,
                     std::ostringstream &symOutput,
                     const std::string &symClass,
                     const std::string &symCode,
                     double lon,
                     double lat,
                     const woml::Feature *feature = nullptr,
                     bool trueNorthAdjustment = false,
                     const woml::NumericalSingleValueMeasure *svm = nullptr,
                     NFmiFillPositions *fpos = nullptr,
                     const std::list<std::string> *areaSymbols = nullptr,
                     int width = 0,
                     int height = 0);
  void render_value(const std::string &path,
                    std::ostringstream &valOutput,
                    const std::string &valClass,
                    const woml::NumericalSingleValueMeasure *lowerLimit,
                    const woml::NumericalSingleValueMeasure *upperLimit,
                    double lon,
                    double lat,
                    bool asValue = false);

  typedef enum
  {
    fst,
    fwd,
    vup,
    eup,
    bwd,
    vdn,
    edn,
    lst
  } Phase;
  typedef enum
  {
    t_ground,
    t_nonground,
    t_mixed
  } ElevationGroupType;
  ElevationGroupType elevationGroup(const std::list<woml::TimeSeriesSlot> &ts,
                                    Fmi::DateTime &bt,
                                    Fmi::DateTime &et,
                                    ElevGrp &eGrp,
                                    bool all = true,
                                    bool join = true,
                                    bool getHole = false,
                                    CategoryValueMeasureGroup *categoryGroup = nullptr);
  unsigned int getLeftSideGroupNumber(ElevGrp &eGrp,
                                      ElevGrp::iterator &iteg,
                                      unsigned int nextGroupNumber,
                                      bool mixed = true);
  unsigned int getRightSideGroupNumber(ElevGrp &eGrp,
                                       ElevGrp::reverse_iterator &itegrev,
                                       unsigned int groupNumber,
                                       bool mixed = true);
  void setGroupNumbers(const std::list<woml::TimeSeriesSlot> &ts, bool mixed = true);
  void checkLeftSide(ElevGrp &eGrp,
                     ElevationHole &hole,
                     CategoryValueMeasureGroup *groupCategory = nullptr);
  bool checkLeftSideHoles(ElevationHoles &holes,
                          ElevationHoles::iterator &iteh,
                          CategoryValueMeasureGroup *groupCategory = nullptr);
  void checkRightSide(ElevGrp &eGrp,
                      ElevationHoles::iterator &iteh,
                      CategoryValueMeasureGroup *groupCategory = nullptr);
  bool checkRightSideHoles(ElevationHoles &holes,
                           ElevationHoles::reverse_iterator iteh,
                           CategoryValueMeasureGroup *groupCategory = nullptr);
  bool checkBothSideHoles(ElevationHoles &holes,
                          ElevationHoles::iterator &iteh,
                          CategoryValueMeasureGroup *groupCategory = nullptr);
  bool checkHoles(ElevGrp &eGrp,
                  ElevationHoles &holes,
                  CategoryValueMeasureGroup *groupCategory = nullptr,
                  bool setNegative = true);
  bool checkCategory(CategoryValueMeasureGroup *groupCategory, ElevGrp::iterator &e) const;
  bool checkCategory(CategoryValueMeasureGroup *groupCategory,
                     ElevGrp::iterator &e1,
                     ElevGrp::iterator &e2) const;
  bool searchHoles(const std::list<woml::TimeSeriesSlot> &ts,
                   CategoryValueMeasureGroup *groupCategory = nullptr,
                   bool setNegative = true);
  Phase uprightdown(
      ElevGrp &eGrp, ElevGrp::iterator &iteg, double lo, double hi, bool nonGndFwd2Gnd = true);
  Phase downleftup(
      ElevGrp &eGrp, ElevGrp::iterator &iteg, double lo, double hi, bool nonGndVdn2Gnd = false);

  template <typename T>
  void render_aerodrome(const T &theFeature);
  void render_aerodromeFrame(const Fmi::TimePeriod &theTimePeriod);
  void render_elevationAxis();
  void render_timeAxis(const Fmi::TimePeriod &theTimePeriod);
  const libconfig::Setting &cloudLayerConfig(const std::string &confPath,
                                             double &tightness,
                                             bool &borderCompensation,
                                             int &minLabelPosHeight,
                                             double &labelPosHeightFactor,
                                             std::list<CloudGroup> &cloudGroups,
                                             std::set<size_t> &cloudSet);
  void render_cloudSymbol(const std::string &confPath,
                          const CloudGroup &cg,
                          int nGroups,
                          double x,
                          double lopx,
                          double hipx);
  void render_cloudSymbols(const std::string confPath,
                           const ElevGrp &eGrp,
                           std::list<CloudGroup>::const_iterator itcg,
                           int nGroups,
                           bool &visible,
                           bool &aboveTop);
  bool scaledCurvePositions(ElevGrp &eGrp,
                            const Fmi::DateTime &bt,
                            const Fmi::DateTime &et,
                            std::vector<DirectPosition> &curvePositions,
                            std::vector<DirectPosition> &curvePositions0,
                            std::vector<double> &scaledLo,
                            std::vector<double> &scaledHi,
                            std::vector<bool> &hasHole,
                            double xOffset,
                            double yOffset,
                            double vOffset,
                            double vSOffset,
                            double vSSOffset,
                            int sOffset,
                            int eOffset,
                            int scaleHeightMin,
                            int scaleHeightRandom,
                            std::ostringstream &path,
                            bool *isVisible = nullptr,
                            bool checkGround = false,
                            bool nonGndFwd2Gnd = true,
                            bool nonGndVdn2Gnd = false);
  void render_timeserie(const woml::CloudLayers &cloudlayers);
  void render_timeserie(const woml::Contrails &contrails);
  template <typename T>
  const libconfig::Setting &groupConfig(const std::string &groupConfPath,
                                        double &tightness,
                                        int &minLabelPosHeight,
                                        double &labelPosHeightFactor,
                                        std::list<T> &groups,
                                        std::set<size_t> &memberSet);
  template <typename GroupCategoryT, typename GroupTypeT>
  void render_timeserie(const std::list<woml::TimeSeriesSlot> &ts,
                        const std::string &confPath,
                        const std::string &classNameExt);
  void render_timeserie(const woml::Icing &icing);
  void render_timeserie(const woml::Turbulence &turbulence);
  void render_timeserie(const woml::MigratoryBirds &migratorybirds);
  void render_timeserie(const woml::SurfaceVisibility &surfacevisibility);
  void render_timeserie(const woml::SurfaceWeather &surfaceweather);
  void render_timeserie(const woml::Winds &winds);
  void checkZeroToleranceGroup(ElevGrp &eGrpIn, ElevGrp &eGrpOut, bool mixed = true);
  void render_timeserie(const woml::ZeroTolerance &zerotolerance);
  void setTextPosition(const Path &path,
                       const std::string &TEXTPOSid,
                       std::string &textPosition,
                       const NFmiFillRect &infoTextRect,
                       int areaWidth,
                       int areaHeight,
                       int textWidth,
                       int textHeight,
                       int tXOffset,
                       int tYOffset,
                       Path::BBox *boundingBox = nullptr);
  void renderAreaLabels(const std::list<DirectPosition> &curvePoints,
                        NFmiFillAreas &holeAreas,
                        const std::string &confPath,
                        const std::string &textName,
                        const std::string &labelPlaceHolder,
                        const std::string &textPosId,
                        bool isHole,
                        const std::string &label,
                        bool bbCenterLabel,
                        double minX,
                        int minH,
                        double hFactor,
                        double sOffset,
                        double eOffset,
                        std::vector<double> &loPx,
                        std::vector<double> &hiPx,
                        std::vector<bool> &hasHole,
                        bool reserveFillAreas,
                        const std::string &areaPlaceHolder = "");
  template <typename T>
  void renderAreaSymbols(const T &cg,
                         const std::list<DirectPosition> &curvePoints,
                         NFmiFillAreas &holeAreas,
                         const std::string &confPath,
                         const std::string &symbolPosId,
                         double minx,
                         std::vector<double> &loPx,
                         std::vector<double> &hiPx,
                         std::vector<bool> &hasHole,
                         const std::string &symClass,
                         const std::string &classNameExt,
                         const std::string &symbol,
                         bool asSymbol,
                         bool &visible,
                         bool &aboveTop);

  const Options &options;
  const libconfig::Config &config;
  std::string svgbase;
  std::shared_ptr<NFmiArea> area;
  const Fmi::DateTime validtime;
  std::ostringstream _debugoutput;
  std::shared_ptr<AxisManager> axisManager;
  bool initAerodrome;

  // defs
  std::ostringstream masks;
  std::ostringstream paths;

  // body
  std::ostringstream cloudareas;
  std::ostringstream coldadvections;
  std::ostringstream coldfronts;
  std::ostringstream jets;
  std::ostringstream longinfotext;
  std::ostringstream occludedfronts;
  std::ostringstream pointnotes;
  std::ostringstream pointsymbols;
  std::ostringstream pointvalues;
  std::ostringstream precipitationareas;
  std::ostringstream ridges;
  std::ostringstream shortinfotext;
  std::ostringstream troughs;
  std::ostringstream uppertroughs;
  std::ostringstream warmadvections;
  std::ostringstream warmfronts;
  std::ostringstream &debugoutput;

  // For text and various stuff
  Texts texts;

  typedef boost::ptr_map<std::string, std::ostringstream> Contours;
  Contours contours;

  int ncloudareas;
  int ncoldadvections;
  int ncoldfronts;
  int njets;
  int noccludedfronts;
  int npointnotes;
  int npointsymbols;
  int npointvalues;
  int nprecipitationareas;
  int nridges;
  int ntroughs;
  int nuppertroughs;
  int nwarmadvections;
  int nwarmfronts;

  FillAreas reservedAreas;
  FillAreas freeAreas;
  FillAreas candidateAreas;
};  // class SvgRenderer

}  // namespace frontier

#endif  // FRONTIER_SVGRENDERER_H
