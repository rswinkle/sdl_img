
static const char controls_text[] =
"Controls/Basic Usage\n"
"====================\n"
"    sdl_img\n"
"    sdl_img image_name.jpg\n"
"    sdl_img -f image_name.jpg  (to start in full screen mode)\n"
"\n"
"The first will open in \"File Selection\" mode since you didn't provide it any\n"
"images. The second will open image_name.jpg and all files with recognized image\n"
"extensions in the same directory. On Windows that would be `sdl_img.exe`, but\n"
"better to just right click on an image of each type and change the default\n"
"application to be sdl_img.exe and then double clicking any image of that type\n"
"will open it with sdl_img.  Left and right will go through images in the same\n"
"directory in [alphabetical](http://stereopsis.com/strcmp4humans.html)\n"
"[order](https://github.com/rswinkle/sdl_img/blob/master/src/string_compare.c).\n"
"\n"
"| Basic Controls      | Description |\n"
"| --------------------|-------------|\n"
"| Left (or Up)        | Previous image(s) or pan if appropriate |\n"
"| Right (or Down)     | Next image(s) or pan if appropriate |\n"
"| Space               | Next image(s) |\n"
"| CTRL + Space        | Previous image(s) |\n"
"| CTRL + Direction    | Next or previous image(s) even when zoomed in |\n"
"| +/-                 | Zoom in/out |\n"
"| Mouse Wheel         | Zoom in/out |\n"
"| Left click + drag   | Pan around a zoomed in image |\n"
"| A                   | Actual size |\n"
"| F                   | Toggle fill screen mode |\n"
"| Home                | Go to first image in the list |\n"
"| End                 | Go to last image in the list |\n"
"| M                   | Shuffle (Mix) the images (only in single mode) |\n"
"| N                   | Sort the images by file name (only in single mode) |\n"
"| CTRL+N              | Sort the images by file path (only in single mode) |\n"
"| Z                   | Sort the images by size (only in single mode) |\n"
"| T                   | Sort the images by last modified (only in single mode) |\n"
"| CTRL + F or F11     | Toggle Fullscreen |\n"
"| ESC                 | Exit or \"Back\" similar to Android |\n"
"| L/R                 | Rotate the current image 90 degrees left/right |\n"
"| CTRL + L/R          | Rotate the current image arbitrary degrees (popup GUI) |\n"
"| H/V                 | Flip the current image horizontally/vertically |\n"
"| S                   | Save current image(s) to current playlist |\n"
"| CTRL + S            | Remove current image(s) from current playlist |\n"
"| O                   | File Open New |\n"
"| CTRL + O            | File Open More |\n"
"| Delete              | Delete the current image and move to the next (only in single mode) |\n"
"| Backspace           | Remove the current image and move to the next (only in single mode) |\n"
"| CTRL + 1            | Single image mode |\n"
"| CTRL + 2            | Double image mode |\n"
"| CTRL + 4            | Quad image mode   |\n"
"| CTRL + 8            | 8 image mode      |\n"
"| CTRL + U            | Thumbnail mode    |\n"
"| CTRL + I            | Library mode    |\n"
"| F1 - F10            | Start a slideshow with 1 - 10 second delay |\n"
"\n"
"For GIFS there are extra controls, though the progress bar is only\n"
"displayed if you are viewing a GIF in single image mode:\n"
"\n"
"| GIF Controls            | Description |\n"
"| --------------------    |-------------|\n"
"| CTRL + +/-              | Speed up or slow down an animated gif |\n"
"| CTRL + Mouse Wheel      | Speed up or slow down an animated gif |\n"
"| P                       | Pause/Unpause gif |\n"
"| Mouse over progress bar | Pause |\n"
"| Wheel over progress bar | Scroll through frames |\n"
"| click/drag progress bar | select/scroll frames |\n"
"\n"
"The most obvious unique features are the multi-image modes but others include the\n"
"extra GIF features, the vim inspired thumbnail mode, the music player inspired\n"
"library mode, shuffle/sort etc.\n"
"\n"
"Within each image mode the basic controls work on all images simultaneously\n"
"unless a specific image is selected by hitting 1-n and then they operate only\n"
"on that image, ie cycling, zooming, fit will only effect that image.\n"
"To return to \"all at once\" mode, hit 0.\n"
"\n"
"By default, when you hit next/prev in n-image mode, it will display the n sequential\n"
"images immediately after the nth image or before the 1st image.  This is annoying if\n"
"you're trying to compare a sequence of tuples that aren't interleaved when sorted.  For\n"
"example if you have files abc01.jpg, abc02.jpg,... and xyz01.jpg, xyz02.jpg..., all of\n"
"the former would sort first.  If you wanted to compare abcX with xyzX in 2-image mode\n"
"every time you hit next, you'd go from [abcX,xyzX] to [xyzX+1,xyzX+2].  To solve this\n"
"you can go to preferences and select the toggle \"Preserve relative offsets in multimode\n"
"movement\" which would lead to [abcX,xyzX] to [abcX+1,xyzX+1] behavior.  I'm open to\n"
"a better name/description (\"Independent movement mode\"?) and to making that the default\n"
"behavior.\n"
"\n"
"Switching from a lower to a higher mode will load images to the right of the last image\n"
"currently viewed.  Switching from a higher to a lower will show the first n images\n"
"of the higher mode where n is the lower number.  An exception is if you have an\n"
"image selected and go to single mode, that is the image that will be used.\n"
"\n"
"The slideshow feature is smart enough to wait long enough to finish any gif being\n"
"displayed even if that takes longer than the specificed delay.  ESC ends the slideshow.\n"
"All other controls work while in slideshow mode, meaning starting a slideshow does not\n"
"automatically toggle to fullscreen, it will run in double, quad or oct-image mode, or\n"
"even on a single image selected within those modes.\n"
"\n"
"Something to note about the rotation functionality is that while it will rotate any\n"
"image, it will only ask whether you're sure you want to save single frame images because\n"
"stb_image_write does not support gifs at all, let alone animated gifs.  It will try to\n"
"detect the type from the extension and output that type if possible, otherwise jpg is\n"
"the default.\n"
"\n"
"Advanced Usage\n"
"==============\n"
"    sdl_img -l list_of_images (urls or local paths or mixed, doesn't matter)\n"
"    sdl_img --list list_of_images\n"
"    sdl_img -p playlist_name (will open a playlist by that name if one exists)\n"
"    sdl_img --playlist playlist_name\n"
"    sdl_img dir (scan dir for images)\n"
"    sdl_img -r dir (scan dir recursively for images)\n"
"    sdl_img dir1 -r dir2 dir3 (scan dir1, dir2 recursively, dir3 )\n"
"    sdl_img dir1 -R dir2 dir3 (scan dir1, then dir2 and dir3 recursively)\n"
"    sdl_img image1.jpg image2.png\n"
"    sdl_img image1.jpg -s 4\n"
"    sdl_img -l list_of_urls -c ./custom_cache_dir\n"
"\n"
"Or any combination of those options, ie\n"
"\n"
"    sdl_img image.jpg -p playlist_name -s 8 ~/some/dir example.com/image.jpg -l list3 image4.gif -f\n"
"\n"
"The option -s [delay] means start in slideshow mode with the given delay in seconds.\n"
"If delay is not 1-10 or is missing, delay will be set to 3 seconds.\n"
"\n"
"When using any of these modes, all the images will be collected in a list then sorted by name.\n"
"For now, if you have multiple url images with the same name, downloaded on the same day,\n"
"the one downloaded last will simply overwrite the earlier ones in the cache.\n"
"This is because the cache does create subdirectories by date of the form YYYY-MM-DD\n"
"for easy browsing/cleaning.\n"
"\n"
"There is also the -c/--cache option which allows you to specify a custom cache\n"
"location (only for this instance) which can be useful if you know you want to\n"
"save these images and don't want to bother copying them from the normal cache later.\n"
"\n"
"Vim Inspired Thumb Mode\n"
"=======================\n"
"Switch with CTRL+U to viewing all your current images as thumbnails on a vertically\n"
"scrolling plane.  Animated GIF thumbnails are generated from the first frame.\n"
"You can use the arrows and mouse and scrollwheel to move around\n"
"and select an image but if you're a vim user you'll feel right at home.\n"
"Note the basic controls above are based on scancode (ie what you see on the\n"
"physical key, what the hardware actually sends, not any custom layout you have in\n"
"software) while the Thumbmode controls are based on keycode and do respect your\n"
"layout (otherwise you lose all that vim muscle memory).  This is irrelevant\n"
"to most people but for people like myself who use Dvorak and *don't* change\n"
"the default Vim keybinds, HJKL, are JCVP physically.\n"
"\n"
"| Thumbmode Controls      | Description |\n"
"| --------------------    |-------------|\n"
"| Arrow Keys or HJKL      | Move around |\n"
"| Mouse Wheel             | Move up and down |\n"
"| g or Home               | Move to first image |\n"
"| G or End                | Move to last image |\n"
"| Click                   | Move to that image |\n"
"| CTRL + Click            | Select multiple arbitrary images |\n"
"| SHIFT + Click           | Select a contiguous range of images |\n"
"| Enter or Double Click   | Change to normal mode on current image |\n"
"| CTRL + HJKL             | Adjust the number of rows and columns shown |\n"
"| V                       | Enter Visual Selection Mode |\n"
"| SHIFT + V               | Enter Visual Line Selection Mode |\n"
"| Backspace or R          | Removes current selection from the list |\n"
"| X                       | Removes and possibly Deletes current selection |\n"
"| CTRL + Backspace/R/X    | Invert action (remove/delete unselected items) |\n"
"| S                       | Save current image(s) to current playlist |\n"
"| CTRL + S                | Remove current image(s) from current playlist |\n"
"| /                       | Start typing a search |\n"
"| /pattern + ENTER        | Enter \"Results mode\" (cycle with n/N) |\n"
"| CTRL + ENTER            | (in results mode) View results |\n"
"| ESC                     | Exit or \"Back\" similar to Android |\n"
"| CTRL + C                | Same as Vim, almost the same as ESC but can't exit thumbmode |\n"
"\n"
"The number of rows and columns can also be set in Preferences, as well as whether X deletes\n"
"instead of just removing the selection.\n"
"\n"
"Library Mode\n"
"============\n"
"Switch with CTRL+I to for a view inspired by music player/organizers like iTunes or Rhythmbox.\n"
"Like the inspiration, this is where you can create, delete, and rename playlists, add images\n"
"to playlists, as well as see/search your currently open images, your entire library, or\n"
"individual playlists. Currently the list columns are name, size, and last modified time,\n"
"though I might expand that later. You can select the column headings to sort by that,\n"
"ascending or descending. You can scroll through the list or use up/down or j/k.\n"
"\n"
"Hitting Enter or double clicking on an image in your current images will go back to normal mode\n"
"on that image. If you do that on an image while seeing search results, you will be in \"View\n"
"Results\" mode, which is normal mode but only viewing the results (same as from thumb search).\n"
"ESC to backs out of View Results, Results, and Lib mode entirely. The following\n"
"\n"
"| Library Controls        | Description |\n"
"| ------------------      |-------------|\n"
"| Up/Down or JK           | Move up/down the list |\n"
"| Mouse Wheel             | Move up/down the list |\n"
"| Home/End                | Move to first/last image in list *if* mouse is hovering over it |\n"
"| Page Up/Down            | Move up/down a page in list *if* mouse is hovering over it |\n"
"| Click                   | Select that image |\n"
"| Enter or Double Click   | Change to normal mode on current image *if* in Current |\n"
"| Backspace               | Removes current selection from the library |\n"
"| S                       | Save current image(s) to current playlist |\n"
"| CTRL + S                | Remove current image(s) from current playlist |\n"
"| O                       | File Open New |\n"
"| CTRL + O                | File Open More |\n"
"| ESC                     | Exit, \"Back\", Cancel similar to Android |\n"
"\n"
"Note: Enter/double clicking on an image when *not* in current images is currently a no-op.\n"
"Still trying to decide what that should do, if anything. I can see arguments for adding\n"
"it to current, saving to the active playlist (same as hitting 's'), or replacing the entire\n"
"currently open list with whatever list you're currently viewing, jumping right into \"View\n"
"Results\" if you're in a search.\n"
"\n"
"Note 2: Like in thumb mode the controls are key codes, not scancodes, which is a bit confusing\n"
"for the controls/actions that are the \"same\" in normal mode. Things in the drop down menu\n"
"that show the keyboard shortcuts no mean the keycode so save/unsave/file open etc. respect\n"
"layout in lib mode but not normal mode. This is a tough decision but I think it's the right\n"
"one because while it's less obvious than thumb mode, your hands are more likely to be on\n"
"the keyboard here, for searching, creating/renaming playlists, and like I said above vim j/k\n"
"working like down/up. I think having the menu actions be using scancodes when other things\n"
"aren't is actually worse than having the menu action keyboard shortcuts be possibly inconsistent\n"
"between normal mode and library mode.  And this only applies to a relatively small number of\n"
"users who actually use a keyboard layout different than the actual keyboard they're using.\n"
"For 99.9% of people this whole note is irrelevant.\n";
