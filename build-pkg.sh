#!/bin/bash

# Localizations
lrelease *.ts

dicom=0
for arg in "$@"; do
  if [ $arg == 'dicom' ]; then dicom=1; fi
done

# Remove non-free code
if [ $dicom == 0 ]; then
  for f in $(grep -l WITH_DICOM *.h *.cpp)
    do unifdef -o $f -UWITH_DICOM $f
  done
  rm -fr dicom*
  sed -i '$d' beryllium.pro
  sed -i 's/DICOM/free/g' debian/control
fi

distro=$(lsb_release -is)
case $distro in
Ubuntu | Debian)  echo "Building DEB package"
    cp docs/* debian/
    dpkg-buildpackage -I.svn -I*.sh -rfakeroot 
    ;;
"openSUSE project" | fedora)  echo "Building RPM package"
    tar czf ../beryllium.tar.gz ../beryllium --exclude=.svn --exclude=*.sh && rpmbuild -D"dicom $dicom" -ta ../beryllium.tar.gz
    ;;
*) echo "$distro is not supported yet"
   ;;
esac
