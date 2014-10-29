%define LIBNAME frontier
Summary: frontier library
Name: smartmet-%{LIBNAME}
Version: 14.10.29
Release: 1%{?dist}.fmi
License: FMI
Group: Development/Libraries
URL: http://www.weatherproof.fi
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: boost-devel >= 1.54.0
BuildRequires: libsmartmet-macgyver >= 14.2.27
BuildRequires: libsmartmet-tron >= 14.2.21
BuildRequires: libsmartmet-woml >= 13.11.28
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
