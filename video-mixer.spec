Name: video-mixer
Provides: video-mixer
Version: 1.0.0
Release: 1%{?dist}
License: LGPL-2.1+
Source: %{name}.tar.gz
URL: http://softus.org/products/beryllium
Vendor: Softus Inc. <contact@softus.org>
Packager: Softus Inc. <contact@softus.org>
Summary: Video mixer

%description
Video mixer.


Combines multiple video streams into one.

%global debug_package %{nil}

Requires(pre): /usr/sbin/useradd
Requires(postun): /usr/sbin/userdel
BuildRequires: make, gcc-c++, systemd

%{?fedora:BuildRequires: gstreamer1-devel, qt5-qtbase-devel, qt5-gstreamer-devel}
%{?fedora:Requires: gstreamer1-plugins-base, gstreamer1-plugins-good, gstreamer1-plugins-bad-free}

%{?rhel:BuildRequires: gstreamer1-devel, qt5-qtbase-devel}
%{?rhel:Requires: gstreamer1-plugins-base, gstreamer1-plugins-good, gstreamer1-plugins-bad-free}

%{?suse_version:BuildRequires: libqt5-linguist, libqt5-qtbase-devel, gstreamer-plugins-qt5-devel}
%{?suse_version:Requires: gstreamer-plugins-base, gstreamer-plugins-good, gstreamer-plugins-bad}

%if 0%{?mageia}
%define qmake qmake
%define lrelease lrelease
BuildRequires: qttools5
%ifarch x86_64 amd64
BuildRequires: lib64qt5-gstreamer-devel, lib64boost-devel, lib64gstreamer1.0-devel, lib64gstreamer-plugins-base1.0-devel, lib64qt5base5-devel
Requires: lib64gstreamer-plugins-base1.0_0
%else
BuildRequires: libqt5-gstreamer-devel,   libboost-devel,   libgstreamer1.0-devel,   libgstreamer-plugins-base1.0-devel,   libqt5base5-devel
Requires: libgstreamer-plugins-base1.0_0
%endif
%else
%define qmake qmake-qt5
%define lrelease lrelease-qt5
%endif

%prep
%setup -c %{name}
 
%build
%{qmake} PREFIX=%{_prefix} QMAKE_CFLAGS+="%optflags" QMAKE_CXXFLAGS+="%optflags";
make %{?_smp_mflags};

%install
make install INSTALL_ROOT="%buildroot"

%files
%defattr(-,root,root)
%doc docs/*
%config(noreplace) %{_sysconfdir}/xdg/softus.org/%{name}.conf
%{_mandir}/man1/%{name}.1.*
%{_bindir}/%{name}
%{_unitdir}/%{name}.service

%pre
/usr/sbin/useradd -rmd /var/lib/%{name} -s /sbin/nologin %{name} || :

%post
service %{name} start || :

%preun
service %{name} stop || :

%postun
/usr/sbin/userdel %{name} || :

%changelog
* Mon Jul 20 2015 Pavel Bludov <pbludov@gmail.com>
+ Version 0.0.1
- Initial version.
