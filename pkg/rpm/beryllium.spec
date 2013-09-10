Summary: A music player that doesn't interfere.
Name: beryllium
Provides: beryllium
Version: 0.3.8
Release: 1
License: LGPL
Source: %{name}-%{version}.tar.bz2
URL: http://dc.baikal.ru/products/beryllium
Vendor: Beryllium team <beryllium@dc.baikal.ru>
Packager: Beryllium team <beryllium@dc.baikal.ru>
#BuildArch: x86_64
BuildArch: i386

Requires: gstreamer-0_10-plugins-base >= 0.10.31, gstreamer-0_10-plugins-good >= 0.10.31, gstreamer-0_10-plugins-bad >= 0.10.23, gstreamer-0_10-plugins-ugly >= 0.10.19
Requires: libgstreamer-0_10-0 >= 0.10.31, libmediainfo0 >= 0.7.52, libqt4 >= 4.7.0, libQtGLib-2_0-0, libQtGStreamer-0_10-0, libstdc++6 >= 4.4.0
Requires: libwrap0 >= 7.6, zlib >= 1.1.4, libdcmtk3_6

#BuildRequires: libgnomeui-devel, gstreamer-devel

%description
Beryllium DICOM edition.

Video and image capturing for medicine.
 
%files
/usr/bin/beryllium
/usr/share/applications/beryllium.desktop
/usr/share/beryllium/translations/beryllium_ru.qm
/usr/share/doc/beryllium/copyright
/usr/share/doc/beryllium/changelog
/usr/share/icons/beryllium.png
/usr/share/man/man1/beryllium.1
