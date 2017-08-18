sdl_img
=======

A simple image viewer based on stb_image.h (with the suggested animated gif api extension).

Goals / Focus
=============
features I've always wanted in an image viewer
Simple, readable code
not focused on speed, if it's fast enough on my chromebook it's good enough

Controls/Usage
==============
./sdl_img image_name.jpg

Will open image_name.jpg.  Left and Right (or Up and Down) will cycle
through images in the directory of the original image (in alphabetical order.
The center wheel can zoom in and out.

A              - Actual Size
F              - Best Fit
ALT + F or F11 - Toggle Fullscreen
ESC            - Exit (or if in Fullscreen, go to windowed mode)

The unique features are being able to view more than a single image at once
CTRL + 1   - Single image mode
CTRL + 2   - Double image mode
CTRL + 4   - Quad image mode
CTRL + 8   - 8 image mode

Within each image mode the basic controls work on all images simultaneously
unless a specific image is selected by hitting 1-n and then they operate only
on that image, ie cycling, zooming, fit will only effect that image.
To return to "all at once" mode, hit 0.  In this way you can compare multiple
images simultaneously with a single viewer instance (even if they aren't sequential
alphabetically).



TODO
====
figure out why certain animated gifs don't load all the frames
Add panning when zoomed in?
Clean up code a bit
