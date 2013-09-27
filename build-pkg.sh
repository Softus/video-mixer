#!/bin/bash

# Localizations
lrelease *.ts

dicom=0
debug=0
touch=0

for arg in "$@"; do
  case $arg in
  'dicom')
    dicom=1
     ;;
  'debug')
    debug=1
     ;;
  'touch')
    touch=1
     ;;
  *) echo "Unknown argument $arg"
     ;;
  esac
done

if [ $dicom == 0 ]; then
  # Remove all non-free code
  for f in $(grep -l WITH_DICOM *.h *.cpp)
    do unifdef -o $f -UWITH_DICOM $f
  done
  rm -fr dicom*
  sed -i '$d' beryllium.pro
  # Beryllium DICOM edition => Beryllium free edition
  sed -i 's/DICOM/free/g' debian/control beryllium.spec
else
  sed -i '1 i CONFIG+=dicom 
  ' beryllium.pro
fi

if [ $debug == 1 ]; then
  sed -i '1 i CONFIG+=debug
  ' beryllium.pro
fi

if [ $touch == 1 ]; then
  sed -i '1 i CONFIG+=touch
  ' beryllium.pro
fi

distro=$(lsb_release -is)
case $distro in
Ubuntu | Debian)  echo "Building DEB package"
    cp docs/* debian/
    dpkg-buildpackage -I.svn -I*.sh -rfakeroot 
    ;;
"openSUSE project" | fedora)  echo "Building RPM package"
    tar czf ../beryllium.tar.gz ../beryllium --exclude=.svn --exclude=*.sh && rpmbuild -D"dicom $dicom" -D"debug $debug" -D"touch $touch" -ta ../beryllium.tar.gz
    ;;
*) echo "$distro is not supported yet"
   ;;
esac
