Summary: Beryllium DICOM edition.
Name: beryllium
Provides: beryllium
Version: 1.2.4
Release: 1
License: LGPL-2.1+
Source: %{name}.tar.gz
#Source: %{name}-%{version}.tar.bz2
URL: http://dc.baikal.ru/products/beryllium
Vendor: Beryllium team <beryllium@dc.baikal.ru>
Packager: Beryllium team <beryllium@dc.baikal.ru>

Requires: opencv
BuildRequires: boost-devel, make, opencv-devel, libsoup-devel

%if %distro == centos
BuildRequires: gstreamer-devel, qt-devel
Requires: gstreamer >= 0.10.35, qt4 >= 4.6.0
Requires: gstreamer-plugins-base >= 0.10.31, gstreamer-plugins-good >= 0.10.23
Requires: gstreamer-plugins-bad >= 0.10.19, gstreamer-plugins-ugly >= 0.10.18
Requires: gstreamer-ffmpeg, gnonlin
%else
BuildRequires: gstreamer-0_10-devel, libqt4-devel
Requires: gstreamer-0_10-plugins-base >= 0.10.31, gstreamer-0_10-plugins-good >= 0.10.23
Requires: gstreamer-0_10-plugins-bad >= 0.10.19, gstreamer-0_10-plugins-ugly >= 0.10.18
Requires: gstreamer-0_10-plugins-ffmpeg, gstreamer-0_10-plugin-gnonlin
Requires: libgstreamer-0_10-0 >= 0.10.35, libqt4 >= 4.6.0
%endif

%if %dicom == 1
Requires: libmediainfo0, libzen0
BuildRequires: libmediainfo-devel, dcmtk-devel, tcp_wrappers-devel
%if %distro == centos
Requires: dcmtk, openssl
BuildRequires: openssl-devel
%else
Requires: libdcmtk3_6, libopenssl1_0_0
BuildRequires: libopenssl-devel
%endif

%endif

%description
Beryllium DICOM edition.


Video and image capturing for medicine.

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
* Wed Sep 11 2013 Pavel Bludov <pbludov@gmail.com>
+ Version 0.3.8
- First rpm spec for automated builds.
