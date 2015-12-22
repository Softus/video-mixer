Summary: Video mixer
Name: videomixer
Provides: videomixer
Version: 0.0.1
Release: 1
License: LGPL-2.1+
Source: %{name}.tar.gz
#Source: %{name}-%{version}.tar.bz2
URL: http://dc.baikal.ru/products/beryllium
Vendor: Beryllium team <beryllium@dc.baikal.ru>
Packager: Beryllium team <beryllium@dc.baikal.ru>

BuildRequires: make

%if %distro == centos
BuildRequires: gstreamer-devel, qt-devel
Requires: gstreamer >= 0.10.35, qt4 >= 4.6.0
Requires: gstreamer-plugins-base >= 0.10.31, gstreamer-plugins-good >= 0.10.23
%else
BuildRequires: gstreamer-0_10-devel, libqt4-devel
Requires: gstreamer-0_10-plugins-base >= 0.10.31, gstreamer-0_10-plugins-good >= 0.10.23
Requires: libgstreamer-0_10-0 >= 0.10.35, libqt4 >= 4.6.0
%endif

%description
Combines multiple video streams into one.

%define _rpmfilename %{distro}-%{rev}-%%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.rpm

%prep
%setup -c %{name}
 
%build
qmake PREFIX=%{_prefix} QMAKE_CFLAGS+="%optflags" QMAKE_CXXFLAGS+="%optflags";
make -j 2 %{?_smp_mflags};

%install
make install INSTALL_ROOT="%buildroot";

%files
%doc docs/*
%{_mandir}/man1/%{name}.1.gz
%{_bindir}/beryllium
%{_datadir}/dbus-1/services
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/%{name}.png

%changelog
* Mon Jul 20 2015 Pavel Bludov <pbludov@gmail.com>
+ Version 0.0.1
- Initial version.
