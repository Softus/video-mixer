Summary: Beryllium DICOM edition.
Name: beryllium
Provides: beryllium
Version: 0.3.16
Release: 1
License: LGPL-2.1+
Source: %{name}.tar.gz
#Source: %{name}-%{version}.tar.bz2
URL: http://dc.baikal.ru/products/beryllium
Vendor: Beryllium team <beryllium@dc.baikal.ru>
Packager: Beryllium team <beryllium@dc.baikal.ru>

Requires: libgstbase-0.10.so.0, libgstinterfaces-0.10.so.0, libgstreamer-0.10.so.0
Requires: libQtGLib-2.0.so.0, libQtGStreamer-0.10.so.0, libQtGStreamerUi-0.10.so.0
Requires: libQtCore.so.4, libQtGui.so.4 libQtNetwork.so.4

BuildRequires: boost-devel, make, libqt4-devel

%if %dicom == 1
Requires: libdcmdata.so.3.6, libdcmnet.so.3.6, libssl.so.1.0.0, libwrap.so.0
Requires: libmediainfo.so.0, libzen.so.0
BuildRequires: dcmtk-devel, libmediainfo-devel
%endif

%description
Beryllium DICOM edition.


Video and image capturing for medicine.

%prep
%setup -qn %{name}
 
%build
qmake PREFIX=%{_prefix} QMAKE_CFLAGS+="%optflags" QMAKE_CXXFLAGS+="%optflags";
make %{?_smp_mflags};

%install
make install INSTALL_ROOT="%buildroot";

%files
%{_bindir}/beryllium
%dir %{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%dir %{_datadir}/%{name}/translations
%{_datadir}/%{name}/translations/%{name}_ru.qm
%{_iconsdir}/%{name}.png
%doc docs/*
%{_mandir}/man1/%{name}.1.gz

%changelog
* Wed Sep 11 2013 Pavel Bludov <pbludov@gmail.com>
+ Version 0.3.8
- First rpm spec for automated builds.
