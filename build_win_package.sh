rm package/*.dll
cp ./*.dll package/
cp LICENSE* package/
cp README.md package/
unix2dos package/README.md package/LICENSE*
cp sdl_img.exe package/
# cp -r src/ package/
