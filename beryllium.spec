Summary: Beryllium DICOM edition.
Name: beryllium
Provides: beryllium
Version: 0.3.25
Release: 1
License: LGPL-2.1+
Source: %{name}.tar.gz
#Source: %{name}-%{version}.tar.bz2
URL: http://dc.baikal.ru/products/beryllium
Vendor: Beryllium team <beryllium@dc.baikal.ru>
Packager: Beryllium team <beryllium@dc.baikal.ru>

Requires: gstreamer-0_10-plugins-base >= 0.10.31, gstreamer-0_10-plugins-good >= 0.10.31
Requires: gstreamer-0_10-plugins-bad >= 0.10.23, gstreamer-0_10-plugins-ugly >= 0.10.19
Requires: gstreamer-0_10-plugins-ffmpeg, gstreamer-0_10-plugin-gnonlin
Requires: libgstreamer-0_10-0 >= 0.10.31, libqt4 >= 4.7.0
Requires: libQtGLib-2_0-0, libQtGStreamer-0_10-0, libstdc++6 >= 4.4.0

BuildRequires: boost-devel, make, libqt4-devel, libQtGLib-devel, gstreamer-0_10-plugins-qt-devel

%if %dicom == 1
Requires: libdcmtk3_6, libopenssl1_0_0
Requires: libmediainfo0, libzen0
BuildRequires: libmediainfo-devel, dcmtk-devel, tcp_wrappers-devel
%endif

%description
Beryllium DICOM edition.


Video and image capturing for medicine.

%prep
%setup -c %{name}
 
%build
qmake PREFIX=%{_prefix} QMAKE_CFLAGS+="%optflags" QMAKE_CXXFLAGS+="%optflags";
make %{?_smp_mflags};

%install
make install INSTALL_ROOT="%buildroot";

%files
%doc docs/*
%{_mandir}/man1/%{name}.1.gz
%{_bindir}/beryllium
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/%{name}.png

%changelog
* Wed Sep 11 2013 Pavel Bludov <pbludov@gmail.com>
+ Version 0.3.8
- First rpm spec for automated builds.
