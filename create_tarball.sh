#!/bin/sh

# start this script inside the acpi source tree to create a releasable tarball

version=`grep AC_INIT configure.ac |sed -e 's/AC_INIT(\[acpi\], \[//' -e 's/\])//'`
major=`echo $version |cut -f1 -d"."`
minor=`echo $version |cut -f2 -d"."`
if [ -d ../acpi-$major.$minor ]
then
	echo "target directory exists"
	exit 1
fi

cp -a . ../acpi-$major.$minor
cd ../acpi-$major.$minor
autoreconf -i
cd ..
tar --exclude CVS --exclude .git --exclude autom4te.cache -zcf acpi-$major.$minor.tar.gz acpi-$major.$minor && rm -fr acpi-$major.$minor

