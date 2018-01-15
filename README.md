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

Will open image_name.jpg.  On Windows that would be sdl_img.exe, but better
to just right click on an image of each type and change the default application
to be sdl_img.exe and then double clicking any image of that type will open it
with sdl_img.

| Basic Controls    | Description |
| ------------------|-------------|
| Left (or Up)      | Previous image(s) or pan if appropriate |
| Right (or Down)   | Next image(s) or pan if appropriate |
| +/-               | Zoom in/out |
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

The unique features are being able to view more than a single image at once

Within each image mode the basic controls work on all images simultaneously
unless a specific image is selected by hitting 1-n and then they operate only
on that image, ie cycling, zooming, fit will only effect that image.
To return to "all at once" mode, hit 0.  In this way you can compare multiple
images simultaneously with a single viewer instance (even if they aren't sequential
alphabetically).  Switching from a higher mode to a lower will show the first n images
of the higher mode where n is the lower number.  An exception is if you have an
image selected and go to single mode, that is the image that will be used.

Also you can start slideshow mode with F1-F10 which will use a delay of 1-10 seconds.
It's smart enough to wait long enough to finish any gif being displayed even if that
takes longer than the specificed delay.  ESC ends the slideshow.  All other controls
discussed above work while in slideshow mode.

Building
========



TODO/IDEAS
==========
- [ ] Let user adjust gif delay up/down with ALT + +/-
- [ ] Take a text list of image paths as an arg and browse those
- [ ] Same as above, but allow URL's (download into a tmp/cache directory)
- [ ] Save memory by having the main thread update images as they're loaded.
- [ ] Clean up code a bit (ongoing)
- [ ] Figure out why certain rare animated gifs don't load all the frames
