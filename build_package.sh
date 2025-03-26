# assumes release build already present
# otherwise uncomment the following line

#./build_release.sh

mkdir -p package_linux/usr/local/bin
mkdir -p package_linux/usr/local/share/man/man1
mkdir -p package_linux/usr/share/applications
mkdir -p package_linux/usr/share/icons/hicolor/48x48/apps

cp ./sdl_img ./package_linux/usr/local/bin
cp sdl_img.1 ./package_linux/usr/local/share/man/man1
cp sdl_img.desktop ./package_linux/usr/share/applications
cp ./package/sdl_img.png ./package_linux/usr/share/icons/hicolor/48x48/apps

fpm -s dir -t deb -v 1.0.0-beta -n sdl_img -C $(PKGDIR) \
	--log debug --verbose \
	-d "libc6" -d "libsdl2-2.0-0 >= 2.0.20" -d "libcurl4" \
	-m "Robert Winkler <rob121618@gmail.com>" \
	--description "`cat $(PKGSRC)/deb_desc.txt`" \
	--deb-changelog $(PKG_DIR)/share/doc/sdl-img/changelog.Debian.gz \
	--deb-field 'Vendor: ' \
	--license MIT \
	--url "https://github.com/rswinkle/sdl_img"

fpm -s dir -t tar -v 1.0.0-beta -n sdl_img_1.0.0-beta -C $(PKG_DIR) \
	--log info --verbose \
	-d "libc6" -d "libsdl2-2.0-0 >= 2.0.20" -d "libcurl4" \
	-m "Robert Winkler <rob121618@gmail.com>" \
	--description "A simple image viewer based on SDL2 and stb_image" \
	--license MIT \
	--url "https://github.com/rswinkle/sdl_img"
