# assumes release build already present
# otherwise uncomment the following line

#./build_release.sh

mkdir -p package_linux/usr/local/bin
mkdir -p package_linux/usr/local/share/man/man1

cp ./sdl_img ./package_linux/usr/local/bin
cp sdl_img.1 ./package_linux/usr/local/share/man/man1

fpm -s dir -t deb -v 0.97 -n sdl_img -C package_linux \
--log info --verbose \
-d "libsdl2-2.0-0 >= 2.0.5" -d "libsdl2-gfx-1.0-0" -d "libcurl3" \
-m "Robert Winkler <rob121618@gmail.com>" \
--license MIT \
--url "https://github.com/rswinkle/sdl_img"


fpm -s dir -t tar -v 0.97 -n sdl_img_0.97 -C package_linux \
--log info --verbose \
-d "libsdl2-2.0-0 >= 2.0.5" -d "libsdl2-gfx-1.0-0" -d "libcurl3" \
-m "Robert Winkler <rob121618@gmail.com>" \
--license MIT \
--url "https://github.com/rswinkle/sdl_img"
