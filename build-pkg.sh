#!/bin/bash

# Localizations
lrelease *.ts

for arg in "$@"; do
  if $arg == 'dicom'; then dicom=1; fi
done

# Remove non-free code
if [ -z $dicom]; then  
  for f in $(grep -l WITH_DICOM *.h *.cpp)
    do unifdef -o $f -UWITH_DICOM $f
  done
  rm -fr dicom*
  sed -i '$d' beryllium.pro
  sed -i 's/beryllium/beryllium-free/g' docs/* debian/* beryllium.spec
fi

distro=$(lsb_release -is)
case $distro in
Ubuntu | Debian)  echo "Building DEB package"
    cp docs/* debian/ && dpkg-buildpackage -I.svn -I*.sh -rfakeroot 
    ;;
"openSUSE project" | fedora)  echo "Building RPM package"
    tar czf ../beryllium.tar.gz ../beryllium --exclude=.svn --exclude=*.sh && rpmbuild -Ddicom=$dicom -ta ../beryllium.tar.gz
    ;;
*) echo "$distro is not supported yet"
   ;;
esac
