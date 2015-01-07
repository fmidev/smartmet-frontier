%define LIBNAME frontier
Summary: frontier library
Name: smartmet-%{LIBNAME}
Version: 14.11.19
Release: 1%{?dist}.fmi
License: FMI
Group: Development/Libraries
URL: http://www.weatherproof.fi
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: boost-devel >= 1.54.0
BuildRequires: libsmartmet-macgyver >= 14.9.5
BuildRequires: libsmartmet-tron >= 14.10.23
BuildRequires: libsmartmet-woml >= 14.11.19
BuildRequires: libxml++-devel >= 2.30.0
Requires: libxml++ >= 2.30.0
Requires: cairo >= 1.8.8
Provides: frontier

%description
FMI FRONTIER library

%package -n smartmet-frontier-devel
Summary: Frontier development headers and library
Group: Development/Libraries
Provides: frontier-devel
%description -n smartmet-frontier-devel
FMI Frontier development headers and library

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q -n %{LIBNAME}
 
%build
make %{_smp_mflags}

%install
%makeinstall includedir=%{buildroot}%{_includedir}/smartmet

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,0775)
%{_bindir}/frontier

%files -n smartmet-frontier-devel
%defattr(-,root,root,0775)
%{_includedir}/smartmet/%{LIBNAME}
%{_libdir}/libsmartmet_%{LIBNAME}.a

%changelog
* Wed Jan 7 2015 Pertti Kinnia <pertti.kinnia@fmi.fi> - 15.1.7-1.fmi
- MIRWA-1056; Lopputuotteiden puolustusvoimien kartoissa ongelma (viivamaiset)
- LENTOSAA-1008; Frontier kaatuu piirrettäessä pilviä 3.5 km lopputuotteeseen
- MIRWA-1051; placing symbols primarily horizontally (ncols >= nrows)
* Wed Dec 17 Pertti Kinnia <pertti.kinnia@fmi.fi>) - 14.12.17-1.fmi
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
