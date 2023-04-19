sdl_img
=======

[![Coverity Scan Build Status](https://scan.coverity.com/projects/15740/badge.svg)](https://scan.coverity.com/projects/rswinkle-sdl_img)

A "simple" image viewer based on [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h) (with the suggested animated GIF API extension).

It's "simple" in that it would work fine as a drop in replacement for most people since all the normal/expected behavior is there,
but it has tons of extra and unique features for power users, especially on Linux/Unix and if you're comfortable with the terminal.

Download
========
Get the latest [source](https://github.com/rswinkle/sdl_img) or the latest
[release](https://github.com/rswinkle/sdl_img/releases) from Github

Goals / Focus
=============
* Implement features I've always wanted in an image viewer
* Relatively simple, short, readable code
* Learn/Practice/Fun
* Not really focused on speed, if it's fast enough on my chromebook it's good enough

Controls/Basic Usage
====================
    ./sdl_img image_name.jpg
    ./sdl_img -f image_name.jpg  (to start in full screen mode)

Will open image_name.jpg.  On Windows that would be `sdl_img.exe`, but better
to just right click on an image of each type and change the default application
to be sdl_img.exe and then double clicking any image of that type will open it
with sdl_img.  Left and right will go through images in the same directory in
alphabetical order.

| Basic Controls      | Description |
| --------------------|-------------|
| Left (or Up)        | Previous image(s) or pan if appropriate |
| Right (or Down)     | Next image(s) or pan if appropriate |
| Space               | Next image(s) |
| CTRL+Space          | Previous image(s) |
| CTRL + Direction    | Next or previous image(s) even when zoomed in |
| +/-                 | Zoom in/out |
| Mouse Wheel         | Zoom in/out |
| Left click + drag   | Pan around a zoomed in image |
| A                   | Actual size |
| F                   | Toggle fill screen mode |
| Home                | Go to first image in the list |
| M                   | Shuffle (Mix) the images (only in single mode) |
| N                   | Sort the images by file name (only in single mode) |
| CTRL+N              | Sort the images by file path (only in single mode) |
| Z                   | Sort the images by size (only in single mode) |
| T                   | Sort the images by last modified (only in single mode) |
| CTRL + F or F11     | Toggle Fullscreen |
| ESC                 | Exit or "Back" similar to Android |
| L/R                 | Rotate the current image left/right |
| H/V                 | Flip the current image horizontally/vertically |
| Delete              | Delete the current image and move to the next (only in single mode) |
| Backspace           | Remove the current image and move to the next (only in single mode) |
| CTRL + 1            | Single image mode |
| CTRL + 2            | Double image mode |
| CTRL + 4            | Quad image mode   |
| CTRL + 8            | 8 image mode      |
| CTRL + U            | Thumbnail mode    |
| CTRL + I            | List mode    |
| F1 - F10            | Start a slideshow with 1 - 10 second delay |

For GIFS there are extra controls, though the progress bar is only
displayed if you are viewing a GIF in single image mode:

| GIF Controls            | Description |
| --------------------    |-------------|
| CTRL + +/-              | Speed up or slow down an animated gif |
| CTRL + Mouse Wheel      | Speed up or slow down an animated gif |
| P                       | Pause/Unpause gif |
| Mouse over progress bar | Pause |
| Wheel over progress bar | Scroll through frames |
| click/drag progress bar | select/scroll frames |

The most obvious unique features are the multi-image modes but others include the
extra GIF features, the vim inspired thumbnail mode, the list mode, shuffle/sort etc.

Within each image mode the basic controls work on all images simultaneously
unless a specific image is selected by hitting 1-n and then they operate only
on that image, ie cycling, zooming, fit will only effect that image.
To return to "all at once" mode, hit 0.

By default, when you hit next/prev in n-image mode, it will display the n sequential
images immediately after the nth image or before the 1st image.  This is annoying if
you're trying to compare a sequence of tuples that aren't interleaved when sorted.  For
example if you have files abc01.jpg, abc02.jpg,... and xyz01.jpg, xyz02.jpg..., all of
the former would sort first.  If you wanted to compare abcX with xyzX in 2-image mode
every time you hit next, you'd go from [abcX,xyzX] to [xyzX+1,xyzX+2].  To solve this
you can go to preferences and select the toggle "Preserve relative offsets in multimode
movement" which would lead to [abcX,xyzX] to [abcX+1,xyzX+1] behavior.  I'm open to
a better name/description ("Independent movement mode"?) and to making that the default
behavior.

Switching from a lower to a higher mode will load images to the right of the last image
currently viewed.  Switching from a higher to a lower will show the first n images
of the higher mode where n is the lower number.  An exception is if you have an
image selected and go to single mode, that is the image that will be used.

The slideshow feature is smart enough to wait long enough to finish any gif being
displayed even if that takes longer than the specificed delay.  ESC ends the slideshow.
All other controls work while in slideshow mode, meaning starting a slideshow does not
automatically toggle to fullscreen, it will run in double, quad or oct-image mode, or
even on a single image selected within those modes.

Something to note about the rotation functionality is that while it will rotate any
image, it will only ask whether you're sure you want to save single frame images because
stb_image_write does not support gifs at all, let alone animated gifs.  It will try to
detect the type from the extension and output that type if possible, otherwise jpg is
the default.

Advanced Usage
==============
    ./sdl_img -l list_of_images (urls or local paths or mixed, doesn't matter)
    ./sdl_img image1.jpg image2.png
    ./sdl_img image1.jpg -s 4
    ./sdl_img -l list_of_urls -c ./custom_cache_dir

Or any combination of those options, ie

    ./sdl_img image.jpg -l list1 -s 8 example.com/image.jpg -l list3 image4.gif -f

The option -s [delay] means start in slideshow mode with the given delay in seconds.
If delay is not 1-10 or is missing, delay will be set to 3 seconds.

When using any of these modes, all the images will be collected in a list in the
order they're given (not sorted like basic usage).  For now, if you have multiple
url images with the same name, downloaded on the same day, the one downloaded last
will simply overwrite the earlier ones in the cache.  This is because the cache does
create subdirectories by date of the form YYYY-MM-DD for easy browsing/cleaning.

There is also the -c/--cache option which allows you to specify a custom cache
location (only for this instance) which can be useful if you know you want to
save these images and don't want to bother copying them from the normal cache later.

Vim Inspired Thumb Mode
=======================
Switch with CTRL+U to viewing all your current images as thumbnails on a vertically
scrolling plane.  Animated GIF thumbnails are generated from the first frame.
You can use the arrows and mouse and scrollwheel to move around
and select an image but if you're a vim user you'll feel right at home.

| Thumbmode Controls      | Description |
| --------------------    |-------------|
| Arrow Keys or HJKL      | Move around |
| Mouse Wheel             | Move up and down |
| Click                   | Move to that image |
| Enter or Double Click   | Change to normal mode on current image |
| CTRL + HJKL             | Adjust the number of rows and columns shown |
| Backspace or R          | Removes current selection from the list |
| X                       | Removes and possibly Deletes current selection |
| CTRL + Backspace/R/X    | Invert action (remove/delete unselected items) |
| /                       | Start typing a search |
| /pattern + ENTER        | Enter "Results mode" (cycle with n/N) |
| CTRL + ENTER            | (in results mode) View results |
| ESC                     | Exit or "Back" similar to Android |

The number of rows and columns can also be set in Preferences, as well as whether X deletes
instead of just removing the selection.

List Mode
=========
Switch with CTRL+I to view a list of all your current images with columns showing the size
and last modified time.  You can select the column headings to sort by that ascending or
descending.  You can scroll through the list or use up and down.  Hitting Enter or double
clicking will go back to normal mode on that image.

Type something in the search bar at the top and hit enter to show a list of files that match.
Same controls as normal list mode, but if you hit enter or double click you will be in "View
Results" mode, which is normal mode but only viewing the results (same as from thumb search).
ESC to backs out of View Results, Results, and list mode entirely.

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
I'll even try something like AppImage or Flatpak long term just for fun.

Naming Issues
=============
So I know there are some issues with the name.

Originally sdl_img only used SDL2 and stb_image; [stb_image](https://github.com/nothings/stb)
and [stb_imv](https://github.com/nothings/stb-imv) already exist,
so I chose sdl_img rather than stb_img.  Unfortunately, SDL_image and SDL2_image
(libsdl-image1.2 and libsdl2-image in repos) also exist so there's a minor clash there too.

Now I'm using the following libraries:
[SDL2](https://www.libsdl.org), stb_image, stb_image_write, stb_image_resize
[libcurl](https://curl.haxx.se/libcurl/)
[nuklear](https://github.com/vurtun/nuklear) (immediate mode GUI library)
[WjCryptLibMd5](https://github.com/WaterJuice/WjCryptLib)

So potential names are stb_img, sdl_img, sdl2_img, sdl_imv, nuklear_img, or
some cool original name that I'm not creative enough to think of.  I'm open to
suggestions.


TODO/IDEAS
==========
- [x] Let user adjust gif delay up/down with ALT + +/-
- [x] Make initial window size based on initial image dimensions
- [x] Take a text list of image paths as an arg and browse those
- [x] Same as above, but allow URL's (download into a tmp/cache directory)
- [x] Don't waste CPU cycles/battery when viewing static images
- [x] Can delete current image in single image mode
- [x] Can rotate images (and save the changes if not animated gifs)
- [x] Handle quoted paths/urls in -l files
- [x] Handle being given a directory as an argument(s)
- [x] Add argument -s [1-10] to start in slideshow mode
- [ ] Add window icon (either static or based on current image(s))
- [x] Thumbnail generating and browsing
- [ ] Install icon in deb package
- [ ] WebM support with mbew?
- [ ] MP4 support?
- [x] Speed up initial startup when a directory has a large number of images
- [x] Change sorting to use strcmp for humans (could use gnu strverscmp)
- [x] Recursive directory scanning with -r/-R
- [x] shuffle/sort (m / o for mix/order)
- [x] Use image extensions as a filter most common use cases
- [x] Nuklear GUI
- [ ] save to favorites or more general "collection" saving
- [ ] Clean up code a bit (ongoing)
- [ ] Save memory by aliasing when viewing the same image more than once in multimode
- [ ] Save memory by having the main thread update images as they're loaded
- [ ] AppImage and/or Flatpak
- [ ] Automatic updating
- [x] Travis-CI
- [x] Coverity
- [x] Figure out why certain rare animated gifs don't load all the frames
