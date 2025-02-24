
// All these are or can be loaded/stored in config.lua
// and can be set from the GUI at runtime
#define DFLT_FONT_SIZE 24.0
#define MIN_FONT_SIZE 16.0
#define MAX_FONT_SIZE 40.0

#define DFLT_PIXEL_SNAP SDL_FALSE
#define DFLT_OVERSAMPLE SDL_FALSE


// Even 3 seems excessive, can't imagine a screen than high density
#define DFLT_GUI_SCALE 1.0f
#define MIN_GUI_SCALE 0.5f
#define MAX_GUI_SCALE 3.0f
#define GUI_SCALE_INCR 0.25f;

#define DFLT_GUI_DELAY 2
#define MIN_GUI_DELAY 1
#define MAX_GUI_DELAY 60

#define DFLT_BUTTON_RPT_DELAY 1.0f
#define MIN_BUTTON_RPT_DELAY 0.25f
#define MAX_BUTTON_RPT_DELAY 3.0f

#define DFLT_SLIDE_DELAY 3
#define MIN_SLIDE_DELAY 1
#define MAX_SLIDE_DELAY 10

#define MIN_THUMB_ROWS 2
#define MIN_THUMB_COLS 4
#define MAX_THUMB_ROWS 12
#define MAX_THUMB_COLS 24
#define DFLT_THUMB_ROWS 8
#define DFLT_THUMB_COLS 15

#define DFLT_THUMB_OPACITY 100
#define MIN_THUMB_OPACITY 32  // Below 32 it gets too transparent imo
#define MAX_THUMB_OPACITY 255  // Should probably make it 200 or something

#define DFLT_THUMB_HIGHLIGHT_COLOR nk_rgb(0,255,0)

#define DFLT_FULLSCREEN_GUI DELAY
#define DFLT_BG_COLOR nk_rgb(0,0,0)
#define DFLT_WINDOW_OPACITY 191

// Should I even use SDL_TRUE/nk_true or just put 1/0?
#define DFLT_FILL_MODE SDL_FALSE
#define DFLT_SHOW_INFOBAR SDL_TRUE
#define DFLT_THUMB_X_DELETES SDL_FALSE
#define DFLT_IND_MM SDL_FALSE
#define DFLT_CONFIRM_DELETE SDL_TRUE
#define DFLT_CONFIRM_ROTATION SDL_TRUE

#define DFLT_FULLSCREEN_GUI DELAY

#define NUM_DFLT_EXTS 11

// End configuration related macros
