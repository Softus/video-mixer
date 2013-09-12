#!/bin/bash

# Localizations
lrelease *.ts

# Remove non-free code
for f in $(grep -l WITH_DICOM *.h *.cpp);
  do unifdef -o $f -UWITH_DICOM $f;
done
rm -fr dicom*
sed -i '$d' beryllium.pro
sed -i 's/beryllium/beryllium-free/g' docs/*

distro=$(lsb_release -is)
case $distro in
Ubuntu | Debian)  echo "Building DEB package"
    cp docs/* debian/ && dpkg-buildpackage -I.svn -I*.sh -rfakeroot 
    ;;
"openSUSE project" | fedora)  echo "Building RPM package"
    tar czf beryllium.tar.gz ../beryllium --exclude=.svn --exclude=*.sh && rpmbuild -ta beryllium.tar.gz
    ;;
*) echo "$distro is not supported yet"
   ;;
esac
