%define LIBNAME frontier
Summary: frontier library
Name: smartmet-%{LIBNAME}
Version: 12.6.1
Release: 1.el6.fmi
License: FMI
Group: Development/Libraries
URL: http://www.weatherproof.fi
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: boost-devel >= 1.49
BuildRequires: libsmartmet-macgyver >= 11.7.20
BuildRequires: libsmartmet-tron >= 11.9.23
BuildRequires: libsmartmet-woml >= 12.6.1-1
BuildRequires: libxml++-devel >= 2.20.0-1
Requires: libxml++ >= 2.20.0-1
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
* Fri Jun  1 2012 Mikko Visa <mikko.visa@fmi.fi> - 12.6.1-1.el6.fmi
- First version supporting WOML schema instead of metobjects schema.
* Fri Sep 23 2011 Mika Heiskanen <mika.heiskanen@fmi.fi> - 11.9.23-1.el5.fmi
- Added detection fro empty grids
* Fri Sep 16 2011 Mika Heiskanen <mika.heiskanen@fmi.fi> - 11.9.16-1.el5.fmi
- Added possibility to configure model paths
* Tue Aug  2 2011 Mika Heiskanen <mika.heiskanen@fmi.fi> - 11.8.2-1.el5.fmi
- Ported to use boost 1.46
* Wed Apr  7 2010 Mika Heiskanen <mika.heiskanen@fmi.fi> - 10.4.7-1.el5.fmi
- Initial build
