#!/usr/bin/env bash

# source of package related files
PKGSRC=package_files

# staging area
PKGDIR=package

# Clean and prepare PKGDIR
rm -rf $PKGDIR
mkdir -p $PKGDIR

# Install into PKGDIR with /usr prefix
make config=release DESTDIR=$PKGDIR PREFIX=/usr install

# Add Debian-specific files
mkdir -p $PKGDIR/DEBIAN
mkdir -p $PKGDIR/usr/share/doc/sdl-img
cp $PKGSRC/control $PKGDIR/DEBIAN
cp $PKGSRC/copyright $PKGDIR/usr/share/doc/sdl-img/copyright
cd $PKGSRC && ./make_changelog.sh && cd ..
cp $PKGSRC/changelog $PKGDIR/usr/share/doc/sdl-img/changelog.Debian
gzip -9n $PKGDIR/usr/share/doc/sdl-img/changelog.Debian
gzip -9n $PKGDIR/usr/share/man/man1/sdl_img.1
strip --strip-unneeded --remove-section=.comment --remove-section=.note $PKGDIR/usr/bin/sdl_img

# Set permissions
find $PKGDIR -type d -exec chmod 0755 {} \;
find $PKGDIR -type f -exec chmod 0644 {} \;
chmod 0755 $PKGDIR/usr/bin/sdl_img

# Build the deb
# fakeroot dpkg-deb -b -Zgzip $PKGDIR sdl-img_0.101.0_amd64.deb
fakeroot dpkg-deb -b $PKGDIR sdl-img_0.101.0_amd64.deb
# alternatively
# dpkg-deb --root-owner-group -b $PKGDIR sdl-img_0.101.0_amd64.deb

