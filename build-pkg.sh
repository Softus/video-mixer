#!/bin/bash

# Localizations
lrelease *.ts

distro=$(lsb_release -is)
case $distro in
Ubuntu | Debian)  echo "Building DEB package"
    cp docs/* debian/
    dpkg-buildpackage -I.svn -rfakeroot 
    ;;
"openSUSE project" | fedora)  echo "Building RPM package"
    cd .. && tar czf beryllium.tar.gz beryllium --exclude=.svn && rpmbuild -ta beryllium.tar.gz
    ;;
*) echo "$distro is not supported yet"
   ;;
esac


