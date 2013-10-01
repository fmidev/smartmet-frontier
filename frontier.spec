%define LIBNAME frontier
Summary: frontier library
Name: smartmet-%{LIBNAME}
Version: 13.7.3
Release: 1%{?dist}.fmi
License: FMI
Group: Development/Libraries
URL: http://www.weatherproof.fi
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: boost-devel >= 1.54
BuildRequires: libsmartmet-macgyver >= 13.7.3
BuildRequires: libsmartmet-tron >= 13.7.3
BuildRequires: libsmartmet-woml >= 13.7.3
BuildRequires: libxml++-devel >= 2.20.0-1
Requires: libxml++ >= 2.30.0-1
Requires: cairo >= 1.8.8-3.1
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
* Upcoming:
- LENTOSAA-688
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
