Name:           xcur2png
Version:        0.7.1
Release:        1%{?dist}
Summary:        Take PNG images from Xcursor and generate xcursorgen config-file

Group:          User Interface/X
License:        GPLv3
URL:            http://www.sutv.zaq.ne.jp/linuz/tks/
Source0:        http://www.sutv.zaq.ne.jp/linuz/tks/item/%{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  pkgconfig libpng-devel libXcursor-devel

%description
xcur2png is a program which let you take PNG image from X cursor,
and generate config-file which is reusable by xcursorgen.


%prep
%setup -q


%build
%configure
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%doc COPYING ChangeLog README
%{_bindir}/xcur2png
%{_mandir}/man1/xcur2png.1*


%changelog
* Thu Jan 01 2009 tks - 0.7.1-1%{?dist}
- Version 0.7.1
* Thu Oct 09 2008 tks - 0.7.0-1%{?dist}
- Version 0.7.0
* Tue Sep 23 2008 tks - 0.6.3-1%{?dist}
- Version 0.6.3
* Mon Sep 22 2008 tks - 0.6.2-1%{?dist}
- Version 0.6.2
* Thu Sep 18 2008 tks - 0.6.1-1%{?dist}
- Version 0.6.1
* Tue Sep 16 2008 tks - 0.6-1%{?dist}
- Version 0.6
* Sat Aug 08 2008 tks - 0.5.1-1%{?dist}
- Version 0.5.1
* Sat Aug 01 2008 tks - 0.5-1%{?dist}
- Version 0.5
* Sat Jul 26 2008 tks - 0.4.1-1%{?dist}
- Version 0.4.1
* Sat Jul 26 2008 tks - 0.4-1%{?dist}
- Inicial release.
