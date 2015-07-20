#!/bin/bash

debug=0

for arg in "$@"; do
  case $arg in
  'debug')
    debug=1
     ;;
  *) echo "Unknown argument $arg"
     echo "Valid arguments are debug dicom"
     ;;
  esac
done

if [ $debug == 1 ]; then
  sed -i '1 i CONFIG+=debug
  ' videomixer.pro
fi

distro=$( (lsb_release -is || echo windows) | awk '{print tolower($1)}' )
rev=$( lsb_release -rs || (cmd.exe /c ver | gawk 'match($0,/[0-9]\.[0-9]/){print substr($0,RSTART,RLENGTH)}') )
case $distro in
ubuntu | debian)  echo "Building DEB package"
    rm -f ../*.deb ../*.tar.gz ../*.dsc ../*.changes
    cp docs/* debian/
    dpkg-buildpackage -I.git -I*.sh -rfakeroot
    cd ..
    for fname in *.deb *.tar.gz *.dsc *.changes;
    do
        mv $fname $distro-$rev-$fname;
    done
    ;;
opensuse | fedora | suse | centos)  echo "Building RPM package"
    rm -f ../*.rpm ../*.tar.gz
    tar czf ../videomixer.tar.gz * --exclude=.git --exclude=*.sh && rpmbuild -D"debug $debug" -D"distro $distro" -D"rev $rev" -ta ../videomixer.tar.gz
    mv ~/rpmbuild/RPMS/*-videomixer-*.rpm ..
    ;;
windows)  echo "Building MSI package"
    qmake && nmake -f Makefile.Release && cp -r c:/tmp/release . && cmd.exe "/c" "wix\build.cmd"
    ;;
*) echo "$distro is not supported yet"
    ;;
esac

