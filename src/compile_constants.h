
#define VERSION 1.0
#define VERSION_STR "sdl_img 1.0-RC3"

// in file_browser.h
//#define PATH_SEPARATOR '/'
//#define STRBUF_SZ 1024

#define PAN_RATE 0.05
#define MAX_GIF_FPS 100
#define MIN_GIF_DELAY (1000/MAX_GIF_FPS)
#define DFLT_GIF_FPS 20
#define DFLT_GIF_DELAY (1000/DFLT_GIF_FPS)
#define SLEEP_TIME 50
#define START_WIDTH 1200
#define START_HEIGHT 800
#define THUMBSIZE 128
#define SIZE_STR_BUF 16
#define MOD_STR_BUF 24

// If this is defined, sdl_img will add files without extensions
// to the list in directory scans if stbi_info() returns true.
// This can make the startup a bit slower if you are scanning a
// large directory (possibly recursively) with many files without
// extensions since stbi_info actually has to open those files
// to determine if they are a valid supported image type
#define CHECK_IF_NO_EXTENSION


// Reasons to not use SDL's hardware accelerated rendering:
// No texture dimension limit. Especially on older and/or mobile gpu's you can
// run into images larger than the max texture size which will then fail to
// load/display
//
// Reasons HW acceleration is the default: speed/power savings, real VSYNC instead
// of fake adhoc SDL_Delay()s to save battery, Cleaner rendering, fewer artifacts.
// There are some minor visual artifacts in the GUI for software rendering
//#define USE_SOFTWARE_RENDERER

// If defined, all log output goes to log.txt in the
// same directory as config.lua
//#define USE_LOGFILE

// zoom is calculated
// h = old_h * (1.0 + zoom*ZOOM_RATE)
// zoom is divided by GIF_ZOOM_DIV if any
// current image is gif because fps is higher
// which speeds up GUI button repeat
//
// It doesn't affect mouse/keyboard so I should
// probably change do_zoom to only divide in the
// gif/gui case...
#define ZOOM_RATE 0.01
#define GUI_ZOOM 5
#define SCROLL_ZOOM 12
#define KEY_ZOOM 12
#define PINCH_ZOOM 3
#define GIF_ZOOM_DIV 3


// TODO constants for some controls... that make more sense to be changeable
// Have to think abount names and how I handle modifiers
// most likely I'll leave the modifiers unchangeable so you only
// specify SDL_SCANCODE_F for fullscreen which is CTRL+F
#define CONTROL_NEXT_PAN_RIGHT SDL_SCANCODE_RIGHT
#define CONTROL_NEXT_PAN_DOWN SDL_SCANCODE_DOWN


