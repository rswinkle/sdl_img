sdl_img
=======
A simple image viewer based on stb_image.h (with the suggested animated GIF API extension).

Goals / Focus
=============
* Implement features I've always wanted in an image viewer
* Relatively simple, short, readable code
* Learn/Practice/Fun
* Not really focused on speed, if it's fast enough on my chromebook it's good enough

Controls/Usage
==============
    ./sdl_img image_name.jpg

Will open image_name.jpg.  On Windows that would be `sdl_img.exe`, but better
to just right click on an image of each type and change the default application
to be sdl_img.exe and then double clicking any image of that type will open it
with sdl_img.  Left and right will go through images in the same directory in
alphabetical order.


| Controls          | Description |
| ------------------|-------------|
| Left (or Up)      | Previous image(s) or pan if appropriate |
| Right (or Down)   | Next image(s) or pan if appropriate |
| +/-               | Zoom in/out |
| ALT + +/-         | Speed up or slow down an animated gif |
| ALT + Direction   | Next or previous image(s) even when zoomed in |
| Mouse Wheel       | Zoom in/out |
| Left click + drag | Pan around a zoomed in image |
| A                 | Actual Size |
| F                 | Best Fit |
| ALT + F or F11    | Toggle Fullscreen |
| ESC               | Exit (or exit Slideshow mode then fullscreen) |
| CTRL + 1          | Single image mode |
| CTRL + 2          | Double image mode |
| CTRL + 4          | Quad image mode   |
| CTRL + 8          | 8 image mode      |
| F1 - F10          | Start a slideshow with 1 - 10 second delay |

The unique features are being able to view more than a single image at once

Within each image mode the basic controls work on all images simultaneously
unless a specific image is selected by hitting 1-n and then they operate only
on that image, ie cycling, zooming, fit will only effect that image.
To return to "all at once" mode, hit 0.  In this way you can compare multiple
images simultaneously with a single viewer instance (even if they aren't sequential
alphabetically).  Switching from a higher mode to a lower will show the first n images
of the higher mode where n is the lower number.  An exception is if you have an
image selected and go to single mode, that is the image that will be used.

The slideshow feature is smart enough to wait long enough to finish any gif being
displayed even if that takes longer than the specificed delay.  ESC ends the slideshow.
All other controls work while in slideshow mode.

Advanced Usage
==============
    ./sdl_img -f list_of_images
    ./sdl_img -u list_of_image_urls
    ./sdl_img image1.jpg image2.png

Or any combination of those options, ie
    ./sdl_img image.jpg -f list1 -u list2 -f list3 image4.gif

When using any of these modes, all the images will be collected in a list in the
order they're given (not sorted like basic usage).  In addition, any urls will be
downloaded to a cache directory before startup.  If you have a large list or lists
of urls, that could take a bit to download so you'd startup would be slow.
In addition, for now if you have multiple url images with the same name, the later
ones will simply overwrite the earlier ones in the cache.

Building
========
On Linux, just run `./build.sh` for debug, `./build_release.sh` for release.

On Windows I use [MSYS2](https://github.com/msys2/msys2/wiki/MSYS2-installation).  I don't
like/use IDE's and I can't stand the Window's command line.  MSYS2 allows me to have the
same environment and tools as Linux.  So it's basically the same, `./build_win.sh` and
`./build_win_release.sh`.


Packaging
=========
I'll expand this section later, once I get to 1.0 and have packages and
more finalized packaging processes but I'm using
[NSIS](http://nsis.sourceforge.net/Main_Page) to create my windows installer and
[fpm](https://github.com/jordansissel/fpm) on linux to create linux packages.  Maybe
I'll even try something like AppImage or flatpak long term just for fun.


TODO/IDEAS
==========
- [x] Let user adjust gif delay up/down with ALT + +/-
- [x] Make initial window size based on initial image dimensions
- [x] Take a text list of image paths as an arg and browse those
- [x] Same as above, but allow URL's (download into a tmp/cache directory)
- [x] Don't waste CPU cycles/battery when viewing static images
- [ ] Clean up code a bit (ongoing)
- [ ] Save memory by aliasing when viewing the same image more than once in multimode
- [ ] Save memory by having the main thread update images as they're loaded
- [ ] Automatic updating
- [ ] Travis-CI
- [ ] Coverity
- [ ] Figure out why certain rare animated gifs don't load all the frames
