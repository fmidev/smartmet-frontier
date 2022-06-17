%define BINNAME frontier
%define RPMNAME smartmet-%{BINNAME}
Summary: frontier
Name: %{RPMNAME}
Version: 22.5.24
Release: 1%{?dist}.fmi
License: MIT
Group: Development/Libraries
URL: https://github.com/fmidev/smartmet-frontier
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%if 0%{?rhel} && 0%{rhel} < 9
%define smartmet_boost boost169
%else
%define smartmet_boost boost
%endif

BuildRequires: rpm-build
BuildRequires: gcc-c++
BuildRequires: make
BuildRequires: %{smartmet_boost}-devel
BuildRequires: smartmet-library-gis-devel >= 22.6.16
BuildRequires: smartmet-library-macgyver-devel >= 22.6.16
BuildRequires: smartmet-library-newbase-devel >= 22.6.16
BuildRequires: smartmet-library-tron-devel >= 22.5.23
BuildRequires: geos310
BuildRequires: smartmet-library-woml >= 22.5.23
BuildRequires: libconfig17-devel >= 1.7.3
BuildRequires: cairo-devel
BuildRequires: xerces-c-devel
BuildRequires: xqilla-devel
Requires: smartmet-library-macgyver >= 22.6.16
Requires: smartmet-library-newbase >= 22.6.16
Requires: smartmet-library-tron >= 22.5.23
Requires: smartmet-library-woml >= 22.5.23
Requires: cairo
Requires: libconfig17 >= 1.7.3
Requires: %{smartmet_boost}-program-options
Requires: %{smartmet_boost}-iostreams
Requires: %{smartmet_boost}-filesystem
Requires: %{smartmet_boost}-regex
Requires: %{smartmet_boost}-date-time
Requires: %{smartmet_boost}-system
Provides: frontier
Obsoletes: smartmet-frontier-devel
#TestRequires: gcc-c++
#TestRequires: smartmet-library-macgyver-devel >= 22.6.16
#TestRequires: ImageMagick
#TestRequires: bc
#TestRequires: coreutils
#TestRequires: cairo-devel
#TestRequires: librsvg2-tools

%description
WOML weather chart renderer

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q -n %{RPMNAME}
 
%build
make %{_smp_mflags}

%install
%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(0775,root,root,-)
%{_bindir}/frontier

%changelog
* Tue May 24 2022 Mika Heiskanen <mika.heiskanen@fmi.fi> - 22.5.24-1.fmi
- Repackaged due to NFmiArea ABI changes

* Tue Feb  1 2022 Andris Pavenis <andris.pavenis@fmi.fi> 22.2.1-1.fmi
- Repackage due to incorrect RPM dependencies in last build

* Fri Jan 21 2022 Andris Pavēnis <andris.pavenis@fmi.fi> 22.1.21-1.fmi
- Repackage due to upgrade of packages from PGDG repo: gdal-3.4, geos-3.10, proj-8.2

* Fri Jan 14 2022 Pertti Kinnia <pertti.kinnia@fmi.fi> 22.1.14-1.fmi
- Marker (symbol/label) positioning improved (PAK-2121)

* Fri Nov 12 2021 Pertti Kinnia <pertti.kinnia@fmi.fi> 21.11.12-1.fmi
- Fixed bug causing use of unrelated global configuration blocks (succeeding named blocks) to be used when reading config settings, messing up e.g. symbol rendering (BRAINSTORM-2199)

* Tue Sep 28 2021 Pertti Kinnia <pertti.kinnia@fmi.fi> 21.9.28-1.fmi
- Handle exceptions thrown by libconfig::Setting::lookupValue (BRAINSTORM-2161)
- Fixed processing of CB clouds (LENTOSAA-1155)

* Tue Sep  7 2021 Andris Pavēnis <andris.pavenis@fmi.fi> 21.9.7-1.fmi
- Use libconfig17 and libconfig17-devel

* Thu May  6 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.5.6-1.fmi
- Repackaged due to NFmiAzimuthalArea ABI changes

* Fri Apr  9 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.4.9-1.fmi
- Repackaged with the latest Tron library for improved contouring speed

* Thu Mar 25 2021 Pertti Kinnia <pertti.kinnia@fmi.fi> - 21.3.25-1.fmi
- CB clouds are stored separately in woml (LENTOSAA-1155)

* Tue Mar 23 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.3.23-1.fmi
- Repackaged due to geos39 updates

* Mon Feb 22 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.2.22-1.fmi
- Updated to use new Tron and Newbase APIs

* Thu Jan 14 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.1.14-1.fmi
- Repackaged smartmet to resolve debuginfo issues
- Fixed build system for GEOS 3.9

* Tue Oct 20 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.10.20-1.fmi
- Repackaged with the latest libconfig

* Thu Sep 17 2020 Pertti Kinnia <pertti.kinnia@fmi.fi> - 20.9.17-1.fmi
- Instead of using fixed (center) position for cloud labels for clouds rendered as symbols, build curve paths too to reserve cloud areas to be able to position labels to free areas in case of overlapping clouds (LENTOSAA-1143)
- Store all feature fillareas as reserved to be able to roughly avoid feature's area when processing other possibly overlapping feature's labels/symbols (LENTOSAA-1143)

* Fri Aug 21 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.8.21-1.fmi
- Upgrade to fmt 6.2

* Sat Apr 18 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.4.18-1.fmi
- Upgrade to Boost 1.69

* Thu Nov 21 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.11.21-1.fmi
- Removed explicit cairo version dependency

* Wed Nov 20 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.11.20-1.fmi
- Repackaged due to newbase API changes

* Thu Oct 31 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.10.31-1.fmi
- Rebuilt due to newbase API/ABI changes

* Fri Sep 27 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.9.27-1.fmi
- Repackaged due to ABI changes in SmartMet libraries

* Wed May  2 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.5.2-1.fmi
- Repackaged since newbase NFmiEnumConverter ABI changed

* Sat Apr  7 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.4.7-1.fmi
- Upgrade to boost 1.66

* Tue Oct 31 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.10.31-2.fmi
- Deprecated option --nocontours (-n), the need for a model is now detected from the template

* Tue Oct 31 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.10.31-1.fmi
- Allow model data to be missing

* Tue Oct  3 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.10.3-1.fmi
- Change to MIT license

* Tue Aug 29 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.8.29-1.fmi
- Using define BOOST_FILESYSTEM_NO_DEPRECATED to avoid crashes with boost 1.65

* Mon Aug 28 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.8.28-1.fmi
- Upgrade to boost 1.65

* Mon Jul 10 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.7.10-1.fmi
- Enabled contouring again (SOL-5406)

* Mon Feb 13 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.2.13-1.fmi
- Repackaged due to newbase API changes

* Tue Feb  7 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.2.7-1.fmi
- Recompiled with latest Tron fixes to isoline contouring

* Fri Jan 27 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.1.27-1.fmi
- Recompiled due to NFmiQueryData object size change

* Wed Jan 11 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.1.11-1.fmi
- Switched to FMI open source naming conventions
- Fix for SOL-4471; missing querydata caused segfault in debug mode

* Tue Sep 6 2016 Mikko Visa <mikko.visa@fmi.fi> - 16.9.6-1.fmi
- Restored cairo dependency in spec file

* Tue Aug 30 2016 Mikko Visa <mikko.visa@fmi.fi> - 16.8.30-1.fmi
- Added/removed some dependencies in SConstruct and spec file

* Mon Aug 22 2016 Mikko Visa <mikko.visa@fmi.fi> - 16.8.22-1.fmi
- rebuild with new woml library

* Mon Jan 18 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.1.17-1.fmi
- newbase API changed

* Mon Nov 23 2015 Mikko Visa <mikko.visa@fmi.fi> - 15.11.23-1.fmi
- Rebuild with new woml library

* Mon Nov 16 2015 Mikko Visa <mikko.visa@fmi.fi> - 15.11.16-1.fmi
- LENTOSAA-1089; Render region id as missing when name and id are equal

* Mon Sep 14 2015 Mikko Visa <mikko.visa@fmi.fi> - 15.9.14-1.fmi
- fixed LENTOSAA-1045

* Thu Jun 18 2015 Pertti Kinnia <pertti.kinnia@fmi.fi> - 15.6.18-1.fmi
- fixed LENTOSAA-1039

* Wed Jun 17 2015 Mikko Visa <mikko.visa@fmi.fi> - 15.6.17-1.fmi
- [LENTOSAA-1037] Using scoped configuration for cloudlayers to enable use of multiple global blocks. "symboltype" setting alone (local or global, not using bool "symbol" setting anymore) controls whether or not rendering the group/cloud as a symbol

* Thu Apr 23 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.4.23-1.fmi
- Enabled contouring again!

* Tue Mar 31 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.3.30-1.fmi
- Use dynamic linking of smartmet libraries
- MIRWA-1070
- LENTOSAA-1010
- MIRWA-839

* Thu Jan 15 2015 Mikko Visa <mikko.visa@fmi.fi> - 15.1.15-1.fmi
- Rebuild for RHEL7

* Wed Jan 7 2015 Pertti Kinnia <pertti.kinnia@fmi.fi> - 15.1.7-1.fmi
- MIRWA-1056; Lopputuotteiden puolustusvoimien kartoissa ongelma (viivamaiset)
- LENTOSAA-1008; Frontier kaatuu piirrettäessä pilviä 3.5 km lopputuotteeseen
- MIRWA-1051; placing symbols primarily horizontally (ncols >= nrows)

* Wed Dec 17 2014 Pertti Kinnia <pertti.kinnia@fmi.fi>) - 14.12.17-1.fmi
- using separate factors for adjusting calculated text width/height
- MIRWA-1051: (single point warning areas not yet gracefully handled)
- MIRWA-1044: lämpötilan vaihteluvälin raja-arvojen tulostusjärjestyksen ohjaus
- calculated text width/height differs from rendered result (batik ?), adjusting by a factor to avoid exceeding limits

* Wed Nov 19 2014 Mikko Visa <mikko.visa@fmi.fi> - 14.11.19-1.fmi
-using the center of the top edge as starting point to get symmetrical (ends to the) result curve for a single elevat
-in addition to the selected position storing free half and 1/4 timestep offset positions to be used instead if marke
-yet another scaling factor fixes
-increased minimum marker down scaling factor to 3/4
-LENTOSAA-989; ignoring MigratoryBirds, SurfaceVisibility and SurfaceWeather data outside document's time range
-LENTOSAA-989
-MIRWA-984; some additional tuning
-MIRWA-984
-Enable contouring in frontier.cpp and SConstruct.
-Fixes to enable build with contouring. Note: 'CPPDEFINES=CONTOURING' setting needs to be uncommented in SConstruct f
-To better keep markers within the area adjust marker position if using areas first or last elevation
-fixed some code indentation
-Add newline after use elements to clarify output.
-MIRWA-998
-LENTOSAA-983, removed buggy handling for candidate fill areas having multiple reservations (LENTOSAA-951)
-LENTOSAA-951; added handling for candidate fill areas (label/symbol positions) having multiple reservations
-LENTOSAA-976
-LENTOSAA-951; ignoring marker's own fill areas when checking for reserved areas, corrected marker positioning when r
-LENTOSAA-951; some improvenments
-LENTOSAA-982
-MIRWA-991
-fixed regex expression to match all placeholders starting with uppercase letter; placeholders with underscore were n
-LENTOSAA-980
-LENTOSAA-978
-LENTOSAA-966; negative y -offset setting is ignored
-LENTOSAA-966
-LENTOSAA-954,LENTOSAA-955
-LENTOSAA-956
-added 'fillmode' setting to control the amount of (warning)symbols placed onto an (warning)area
-locale is not used when selecting configuraton settings for rendering text
-added missing 'px' to css class generated for component info text
-front rendering is omitted if configuration block is missing
-LENTOSAA-937
-LENTOSAA-941; bug fix
-fixed bug in extracting the default elevation group label using configured member types and the types contained in c
-LENTOSAA-948
-LENTOSAA-941
-LENTOSAA-940, Esa's bezier java code upgrade/reconversion (no notable curve changes, some other/preliminary code cha
-fixed bug in calculating text area height
-added symbol infotext position control (like areas), setting text stroke/fill color from configuration
-LENTOSAA-929
-LENTOSAA-920; elevationGroup(); fixed bug in skipping elevations which don't belong to current categorized group. Mi
-LENTOSAA-920
-text rendering errors (due to missing configuration values) are silently ignored
-surface rendering reads configuration in 'scoped' mode (using all encountered global blocks instead of just the last
-added on/off control for text background handling; background setting currently implemented for document level text
-added on/off control for text background handling; background setting currently implemented for document level text
-fixed bug in handling surface info text configuration keys/names
-fixed bug in handling area text configuration keys/names
-generating css class for each surface info text instead of one common class
-surface info text placement (within or outside the area), text splitting accuracy improved (using line extent instea
-fill symbol positioning improved for ParameterValueSetArea (warning areas)

* Thu Oct 30 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.10.30-2.fmi
- Improved contour labeling algorithm

* Thu Oct 30 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.10.30-1.fmi
- Added contour label support

* Wed Oct 29 2014 Santeri Oksman <santeri.oksman@fmi.fi> - 14.10.29-1.fmi
- New release to enable contouring

* Wed Oct 22 2014 Santeri Oksman <santeri.oksman@fmi.fi> - 14.10.22-1.fmi
- Added possibility to use inline images with <use> tags and to scale those images (MIRWA-998)

* Mon Apr 14 2014 Mikko Visa <mikko.visa@fmi.fi> - 14.4.14-1.fmi
-LENTOSAA-914
-LENTOSAA-899; fixed css class names for turbulence
-LENTOSAA-911
-LENTOSAA-898,LENTOSAA-899
-LENTOSAA-909
-LENTOSAA-888, support for area info text (incoplete)
-added support for redefinition of #define contants
-LENTOSAA-743
-MIRWA-894; implemented a) for ParameterValueSetPoint, PointMeteorologicalSymbol and for classes derived from PressureCenterType
-LENTOSAA-894
-MIRWA-895; substracting value x position offset from the original position (same logic with symbol position)
-MIRWA-895; value position offset configuration keys changed (conflicted with symbol offset keys)
-MIRWA-895; using plain 'code' key to conditionally map all wind symbols (to calm) when wind speed is nan
-MIRWA-895; Wind speed is not rendered if format is empty (used to supress zero values)
-MIRWA-895
-MIRWA-934, output placeholder ignored for warning area symbols, rendering order changed (area[,fillareas][,symbols]) for surfaces
-MIRWA-931
-MIRWA-931
-LENTOSAA-839
-added missing function
-MIRWA-892; area symbol code may contain folder too (folder/symbol), fill logic simplified
-fixed bug in positioning area symbols
-MIRWA-892
-LENTOSAA-877,LENTOSAA-883
-LENTOSAA-881, added missing files (PreProcesssor; LENTOSAA-734)
-LENTOSAA-796
-new features and fixes; LENTOSAA-734,762,773,795,800,808,837,840,852,876
-using PreProcessor to enable usage of defined constants and include templates
-LENTOSAA-769, LENTOSAA-792

* Thu Nov 28 2013 Mikko Visa <mikko.visa@fmi.fi> - 13.11.28-1.fmi
- modifications to support elevation hole handling (not complete)
- code cleanup; using common method scaledCurvePositions() to get curve points for bezier creation
- https://jira.fmi.fi:8443/LENTOSAA-688
- https://jira.fmi.fi:8443/LENTOSAA-706
- https://jira.fmi.fi:8443/LENTOSAA-753
- https://jira.fmi.fi:8443/LENTOSAA-754
- https://jira.fmi.fi:8443/LENTOSAA-687

* Wed Jul  3 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> 13.7.3-1.fmi
- Update to boost 1.54

* Wed Feb 27 2013 Mikko Visa <mikko.visa@fmi.fi> - 13.2.27-1.fmi
- Using svgescape for header texts

* Tue Aug  7 2012 Mika Heiskanen <mika.heiskanen@fmi.fi> - 12.8.7-1.fmi
- RHEL6 recompile

* Fri Jun  1 2012 Mikko Visa <mikko.visa@fmi.fi> - 12.6.1-1.fmi
- First version supporting WOML schema instead of metobjects schema.

* Fri Sep 23 2011 Mika Heiskanen <mika.heiskanen@fmi.fi> - 11.9.23-1.fmi
- Added detection fro empty grids

* Fri Sep 16 2011 Mika Heiskanen <mika.heiskanen@fmi.fi> - 11.9.16-1.fmi
- Added possibility to configure model paths

* Tue Aug  2 2011 Mika Heiskanen <mika.heiskanen@fmi.fi> - 11.8.2-1.fmi
- Ported to use boost 1.46

* Wed Apr  7 2010 Mika Heiskanen <mika.heiskanen@fmi.fi> - 10.4.7-1.fmi
- Initial build
