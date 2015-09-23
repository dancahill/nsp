Name:		nesla
Version:	0.9.4pre1
Release:	1
Group:		Networking/Daemons
Packager:	Dan Cahill <http://nesla.sourceforge.net/nesla/>
URL:		http://nesla.sourceforge.net/nesla/
Source:		http://prdownloads.sourceforge.net/nesla/nesla-%{PACKAGE_VERSION}.tar.gz
Summary:	Nesla
Vendor:		NullLogic
License:	GNU GPL Version 2
BuildRoot:	/tmp/nesla-rpm
Provides:	nesla
Obsoletes:	nesla

%description
NullLogic Embedded Scripting Language

%prep

%setup

%build
rm -rf ${RPM_BUILD_ROOT}
make

%install
install -d ${RPM_BUILD_ROOT}/usr/bin
cp -a bin/nesla ${RPM_BUILD_ROOT}/usr/bin

%clean
rm -rf ${RPM_BUILD_ROOT}

%post

%preun

%files
%defattr(-,root,root)
%doc doc/changelog.html doc/copying.html doc/copyright.html
/usr/bin/*

%changelog
* Sun Jun 01 2008 Dan Cahill <nulllogic@users.sourceforge.net>
- Release Version 0.9.2
