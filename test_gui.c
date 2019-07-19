
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

// TODO sin, cos, sqrt etc.
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
//#define NK_INCLUDE_FONT_BAKING
//#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_sdl.h"



#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

#define STRBUF_SZ 1024

enum { MENU_NONE, MENU_MISC, MENU_EDIT, MENU_VIEW };

int n_imgs = 1;
SDL_Window* win;
SDL_Renderer* ren;
int running;
int slideshow = nk_false;
int slide_delay = 3;
int gui_delay = 2;
int show_about = nk_false;
int show_prefs = nk_false;
int show_rotate = nk_false;
int menu_state = MENU_NONE;

struct nk_colorf bg;
struct nk_color bg2;

float x_scale;
float y_scale;

char license[STRBUF_SZ*4] =
"The MIT License (MIT)\n"
"\n"
"Copyright (c) 2017-2019 Robert Winkler\n"
"\n"
"Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated\n"
"documentation files (the \"Software\"), to deal in the Software without restriction, including without limitation\n"
"the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and\n"
"to permit persons to whom the Software is furnished to do so, subject to the following conditions:\n"
"\n"
"The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.\n"
"\n"
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED\n"
"TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL\n"
"THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF\n"
"CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS\n"
"IN THE SOFTWARE.";


int handle_events(struct nk_context* ctx);
void draw_gui(struct nk_context* ctx);
void draw_simple_gui(struct nk_context* ctx);

int main(void)
{
	int win_width, win_height;
	struct nk_context *ctx;
	/* float bg[4]; */

	/* SDL setup */
	if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		printf( "Can't init SDL:  %s\n", SDL_GetError( ) );
		return 1;
	}

	running = 1;

	int win_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;

	win = SDL_CreateWindow("sdl_img gui",
	                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                       WINDOW_WIDTH, WINDOW_HEIGHT,
	                       win_flags);

	if (!win) {
		printf("Can't create window: %s\n", SDL_GetError());
		return 1;
	}
	/* try VSYNC and ACCELERATED */
	ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
	if (!ren) {
		printf("Can't create ren: %s\n", SDL_GetError());
		return 1;
	}

	float hdpi, vdpi, ddpi;
	SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi);
	printf("DPIs: %.2f %.2f %.2f\n", ddpi, hdpi, vdpi);

	SDL_Rect r;
	SDL_GetDisplayBounds(0, &r);
	printf("display bounds: %d %d %d %d\n", r.x, r.y, r.w, r.h);

	x_scale = 2; //hdpi/72;  // adjust for dpi, then go from 8pt font to 12pt
	y_scale = 2; //vdpi/72;

	SDL_RenderSetScale(ren, x_scale, y_scale);

	/*
	if (SDL_RenderSetLogicalSize(ren, WINDOW_WIDTH*x_scale, WINDOW_HEIGHT*y_scale)) {
		printf("logical size failure: %s\n", SDL_GetError());
		return 1;
	}
	*/

	if (!(ctx = nk_sdl_init(win, ren, x_scale, y_scale))) {
		printf("nk_sdl_init() failed!");
		return 1;
	}


	bg2 = nk_rgb(28,48,62);
	bg = nk_color_cf(bg2);
	while (running)
	{
		SDL_RenderSetScale(ren, x_scale, y_scale);
		nk_sdl_scale(x_scale, y_scale);

		if (handle_events(ctx))
			break;

		/* GUI */
		draw_gui(ctx);
		//draw_simple_gui(ctx);


		SDL_Delay(50);
		SDL_SetRenderDrawColor(ren, bg2.r, bg2.g, bg2.b, bg2.a);
		SDL_RenderSetClipRect(ren, NULL);
		SDL_RenderClear(ren);
		nk_sdl_render(NULL, 0);
		SDL_RenderPresent(ren);

	}

cleanup:
	nk_sdl_shutdown();
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}



void draw_simple_gui(struct nk_context* ctx)
{
	int gui_flags = NK_WINDOW_NO_SCROLLBAR; //NK_WINDOW_BORDER|NK_WINDOW_TITLE;

	int win_w, win_h;
	int scr_w, scr_h;
	int out_w, out_h;
	SDL_GetWindowSize(win, &win_w, &win_h);
	SDL_RenderGetLogicalSize(ren, &scr_w, &scr_h);
	SDL_GetRendererOutputSize(ren, &out_w, &out_h);

	float cur_x_scale, cur_y_scale;
	SDL_RenderGetScale(ren, &cur_x_scale, &cur_y_scale);
	//printf("scale = %.2f x %.2f\n", cur_x_scale, cur_y_scale);

	int fill = 0, slideshow = 0;

	if (nk_begin(ctx, "demo", nk_rect(100/cur_x_scale, 100/cur_y_scale, 200, 400), gui_flags)) {
		nk_layout_row_static(ctx, 0, 80, 2);
		if (nk_button_label(ctx, "hello")) {
			puts("down");
			x_scale -= 0.25;
			y_scale -= 0.25;
		}
		if (nk_button_label(ctx, "goodbye")) {
			puts("up");
			x_scale += 0.25;
			y_scale += 0.25;
		}
		nk_checkbox_label(ctx, "Best Fit", &fill);
		nk_checkbox_label(ctx, "Slideshow", &slideshow);
	}
	nk_end(ctx);

	/*
	if (nk_begin(ctx, "demo2", nk_rect(400/cur_x_scale, 100/cur_y_scale, 200, 400), gui_flags)) {
		nk_layout_row_static(ctx, 0, 80, 2);
		if (nk_button_label(ctx, "hello")) {
			;
		}
		if (nk_button_label(ctx, "goodbye")) {
			;
		}
		nk_checkbox_label(ctx, "Best Fit", &fill);
		nk_checkbox_label(ctx, "Slideshow", &slideshow);
	}
	nk_end(ctx);
	*/

}



void draw_gui(struct nk_context* ctx)
{
	char info_buf[STRBUF_SZ];
	int len;
	int gui_flags = NK_WINDOW_NO_SCROLLBAR; //NK_WINDOW_BORDER|NK_WINDOW_TITLE;

	// closable gives the x, if you use it it won't come back (probably have to call show() or
	// something...
	int prefs_flags = NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_BORDER|NK_WINDOW_TITLE;//NK_WINDOW_CLOSABLE;
	SDL_Event event = { 0 };

	int win_w, win_h;
	int scr_w, scr_h;
	int out_w, out_h;
	SDL_GetWindowSize(win, &win_w, &win_h);
	//SDL_RenderGetLogicalSize(ren, &scr_w, &scr_h);
	SDL_GetRendererOutputSize(ren, &out_w, &out_h);

	float scale_x, scale_y;
	SDL_RenderGetScale(ren, &scale_x, &scale_y);
	//scale_y = 2;
	//printf("scale = %.2f x %.2f\n", scale_x, scale_y);

	scr_w = out_w/scale_x;
	scr_h = out_h/scale_y;

	//printf("win %d x %d\nlog %d x %d\nout %d x %d\n", win_w, win_h, scr_w, scr_h, out_w, out_h);

	struct nk_rect bounds;
	const struct nk_input* in = &ctx->input;

	static int fill = nk_false;
	static int fullscreen = nk_false;
	static int hovering = 0;
	static int what_hover = 0;

	int total_width = 0;
	int do_zoom = 0, do_toggles = 0, do_rotates = 0;
	what_hover = 0;




	if (show_rotate) {
		int w = 400, h = 300; ///scale_x, h = 400/scale_y;
		struct nk_rect s;
		s.x = scr_w/2-w/2;
		s.y = scr_h/2-h/2;
		s.w = w;
		s.h = h;
		static int slider_degs = 0;

		if (nk_begin(ctx, "Rotation", s, prefs_flags)) {

			//nk_button_set_behavior(ctx, NK_BUTTON_REPEATER);
			//nk_layout_row_dynamic(ctx, 0, 2);
			nk_layout_row_dynamic(ctx, 0, 1);

			nk_label_wrap(ctx, "Click and drag, type, or use the arrows to select the desired degree of rotation");

			//nk_label(ctx, "Degrees:", NK_TEXT_LEFT);
			slider_degs = nk_propertyi(ctx, "Degrees:", -180, slider_degs, 180, 1, 0.5);
			//nk_slider_int(ctx, -180, &slider_degs, 180, 1);

			//nk_button_set_behavior(ctx, NK_BUTTON_DEFAULT);
			nk_layout_row_dynamic(ctx, 0, 2);
			if (nk_button_label(ctx, "Preview")) {
				;
			}
			if (nk_button_label(ctx, "Ok")) {
				show_rotate = 0;;
			}
		}
		nk_end(ctx);
	}


	if (show_about) {
		static int license_len;;
		license_len = strlen(license);
		int w = 500, h = 400; ///scale_x, h = 400/scale_y;
		struct nk_rect s;
		s.x = scr_w/2-w/2;
		s.y = scr_h/2-h/2;
		s.w = w;
		s.h = h;
		if (nk_begin(ctx, "About sdl_img", s, prefs_flags))
		{
			nk_layout_row_dynamic(ctx, 0, 1);
			nk_label(ctx, "sdl_img 1.0", NK_TEXT_CENTERED);
			nk_label(ctx, "By Robert Winkler", NK_TEXT_LEFT);
			nk_label(ctx, "robertwinkler.com", NK_TEXT_LEFT);  //TODO project website
			nk_label(ctx, "sdl_img is licensed under the MIT License.",  NK_TEXT_LEFT);

			nk_label(ctx, "Credits:", NK_TEXT_CENTERED);
			nk_layout_row_dynamic(ctx, 0, 2);
			nk_label(ctx, "stb_image, stb_image_write", NK_TEXT_LEFT);
			nk_label(ctx, "github.com/nothings/stb", NK_TEXT_RIGHT);
			nk_label(ctx, "SDL2", NK_TEXT_LEFT);
			nk_label(ctx, "libsdl.org", NK_TEXT_RIGHT);
			nk_label(ctx, "SDL2_gfx", NK_TEXT_LEFT);
			nk_label(ctx, "ferzkopp.net", NK_TEXT_RIGHT);
			nk_label(ctx, "nuklear GUI", NK_TEXT_LEFT);
			nk_label(ctx, "github.com/vurtun/nuklear", NK_TEXT_RIGHT);

			// Sean T Barret (sp?) single header libraries
			// stb_image, stb_image_write
			//
			// nuklear (which also uses stb libs)
			//
			// My own cvector lib

			//nk_layout_row_dynamic(ctx, 200, 1);
			//nk_label_wrap(ctx, license);
			//nk_edit_string(ctx, NK_EDIT_BOX, license, &license_len, 1024, nk_filter_default);
			//
			nk_layout_row_dynamic(ctx, 0, 1);
			if (nk_button_label(ctx, "Ok")) {
				show_about = 0;;
			}
		}
		nk_end(ctx);
	}

	if (show_prefs) {
		int w = 400, h = 400; ///scale_x, h = 400/scale_y;
		struct nk_rect s;
		s.x = scr_w/2-w/2;
		s.y = scr_h/2-h/2;
		s.w = w;
		s.h = h;
		char cache[] = "/home/someone/really/long/path_that_goes_forever/blahblahblah/.local/share/sdl_img";

		if (nk_begin(ctx, "Preferences", s, prefs_flags)) {
			nk_layout_row_dynamic(ctx, 0, 2);
			nk_label(ctx, "background:", NK_TEXT_LEFT);
			if (nk_combo_begin_color(ctx, nk_rgb_cf(bg), nk_vec2(nk_widget_width(ctx), 400))) {
				nk_layout_row_dynamic(ctx, 120, 1);
				bg = nk_color_picker(ctx, bg, NK_RGBA);
				nk_layout_row_dynamic(ctx, 25, 1);
				bg.r = nk_propertyf(ctx, "#R:", 0, bg.r, 1.0f, 0.01f,0.005f);
				bg.g = nk_propertyf(ctx, "#G:", 0, bg.g, 1.0f, 0.01f,0.005f);
				bg.b = nk_propertyf(ctx, "#B:", 0, bg.b, 1.0f, 0.01f,0.005f);
				bg.a = nk_propertyf(ctx, "#A:", 0, bg.a, 1.0f, 0.01f,0.005f);
				bg2 = nk_rgb_cf(bg);
				nk_combo_end(ctx);
			}

			nk_label(ctx, "Slideshow delay:", NK_TEXT_LEFT);
			nk_property_int(ctx, "", 1, &slide_delay, 10, 1, 0.05);

			nk_label(ctx, "Hide GUI delay:", NK_TEXT_LEFT);
			nk_property_int(ctx, "", 1, &gui_delay, 60, 1, 0.05);


			nk_layout_row_dynamic(ctx, 0, 1);
			nk_label(ctx, "Cache directory:", NK_TEXT_LEFT);
			nk_label_wrap(ctx, cache);

			nk_layout_row_dynamic(ctx, 0, 1);
			if (nk_button_label(ctx, "Ok")) {
				show_prefs = 0;;
			}
		}
		nk_end(ctx);

	}


	// don't show main GUI if a popup is up, don't want user to
	// be able to interact with it.  Could look up how to make them inactive
	// but meh, this is simpler
	if (show_about || show_prefs || show_rotate)
		return;






	if (nk_begin(ctx, "Controls", nk_rect(0, 0, scr_w, 30), gui_flags)) {

		//g->gui_rect = nk_window_get_bounds(ctx);
		//printf("gui %f %f %f %f\n", g->gui_rect.x, g->gui_rect.y, g->gui_rect.w, g->gui_rect.h);

		//nk_layout_row_static(ctx, 0, 80, 16);
		//nk_layout_row_dynamic(ctx, 0, 16);
		
		nk_layout_row_template_begin(ctx, 0);

		// menu
		nk_layout_row_template_push_static(ctx, 50);

		// prev next
		nk_layout_row_template_push_static(ctx, 80);
		nk_layout_row_template_push_static(ctx, 80);

		total_width += 50 + 80 + 80;

		// zoom, -, +
		//nk_layout_row_template_push_static(ctx, 80);
		nk_layout_row_template_push_static(ctx, 40);
		nk_layout_row_template_push_static(ctx, 40);

		total_width += 80;
		if (total_width < scr_w)
			do_zoom = 1;

		// best fit, slideshow, and actual size
		//nk_layout_row_template_push_static(ctx, 80);
		//nk_layout_row_template_push_static(ctx, 80);
		//nk_layout_row_template_push_static(ctx, 80);

		//total_width += 240;
		//if (total_width < scr_w)
		//	do_toggles = 1;

		// Rotate left and right
		nk_layout_row_template_push_static(ctx, 40);
		nk_layout_row_template_push_static(ctx, 40);
		total_width += 80;
		if (total_width < scr_w)
			do_rotates = 1;

		// Mode 1 2 4 8
		/*
		nk_layout_row_template_push_static(ctx, 80);
		nk_layout_row_template_push_static(ctx, 40);
		nk_layout_row_template_push_static(ctx, 40);
		nk_layout_row_template_push_static(ctx, 40);
		nk_layout_row_template_push_static(ctx, 40);
		*/
		nk_layout_row_template_end(ctx);

		if (nk_menu_begin_label(ctx, "Menu", NK_TEXT_LEFT, nk_vec2(400, 400))) {

			enum nk_collapse_states state;
			float ratios[] = { 0.7f, 0.3f, 0.8f, 0.2f };

			// always show these
			nk_layout_row_dynamic(ctx, 0, 3);
			nk_label(ctx, "GUI:", NK_TEXT_LEFT);
			if (nk_menu_item_label(ctx, "-", NK_TEXT_CENTERED)) {
				x_scale -= 0.5;
				y_scale -= 0.5;
				if (x_scale < 1 || y_scale < 1) {
					x_scale = 1;
					y_scale = 1;
				}
			}
			if (nk_menu_item_label(ctx, "+", NK_TEXT_CENTERED)) {
				x_scale += 0.5;
				y_scale += 0.5;
			}

			nk_layout_row_dynamic(ctx, 0, 1);
			if (nk_menu_item_label(ctx, "Preferences", NK_TEXT_LEFT)) {
				show_prefs = nk_true;
			}
			if (nk_menu_item_label(ctx, "About", NK_TEXT_LEFT)) {
				show_about = nk_true;
			}
			if (nk_menu_item_label(ctx, "Exit", NK_TEXT_LEFT)) {
				running = 0;
			}

			state = (menu_state == MENU_MISC) ? NK_MAXIMIZED : NK_MINIMIZED;
			if (nk_tree_state_push(ctx, NK_TREE_TAB, "Misc. Actions", &state)) {
				menu_state = MENU_MISC;
				nk_layout_row(ctx, NK_DYNAMIC, 0, 2, ratios);

				nk_selectable_label(ctx, "Slideshow", NK_TEXT_LEFT, &slideshow);
				nk_label(ctx, "F1 - F10/ESC", NK_TEXT_RIGHT);

				nk_selectable_label(ctx, "Fullscreen", NK_TEXT_LEFT, &fullscreen);
				nk_label(ctx, "ALT+F or F11", NK_TEXT_RIGHT);

				nk_menu_item_label(ctx, "Best Fit", NK_TEXT_LEFT);
				nk_label(ctx, "F", NK_TEXT_RIGHT);

				nk_menu_item_label(ctx, "Actual Size", NK_TEXT_LEFT);
				nk_label(ctx, "A", NK_TEXT_RIGHT);

				if (n_imgs == 1) {
					nk_menu_item_label(ctx, "Mix images", NK_TEXT_LEFT);
					nk_label(ctx, "M", NK_TEXT_RIGHT);


					// TODO sort submenu with sort by name, size, modified etc.
					nk_menu_item_label(ctx, "Sort by name (default)", NK_TEXT_LEFT);
					nk_label(ctx, "O", NK_TEXT_RIGHT);
				}

				nk_tree_pop(ctx);
			} else menu_state = (menu_state == MENU_MISC) ? MENU_NONE : menu_state;

			state = (menu_state == MENU_EDIT) ? NK_MAXIMIZED : NK_MINIMIZED;
			if (nk_tree_state_push(ctx, NK_TREE_TAB, "Edit Actions", &state)) {
				menu_state = MENU_EDIT;
				nk_layout_row(ctx, NK_DYNAMIC, 0, 2, ratios);

				nk_menu_item_label(ctx, "Rotate Left", NK_TEXT_LEFT);
				nk_label(ctx, "L", NK_TEXT_RIGHT);

				nk_menu_item_label(ctx, "Rotate Right", NK_TEXT_LEFT);
				nk_label(ctx, "R", NK_TEXT_RIGHT);

				nk_menu_item_label(ctx, "Flip Horizontal", NK_TEXT_LEFT);
				nk_label(ctx, "H", NK_TEXT_RIGHT);

				nk_menu_item_label(ctx, "Flip Vertical", NK_TEXT_LEFT);
				nk_label(ctx, "V", NK_TEXT_RIGHT);

				//nk_menu_item_label(ctx, "Save", NK_TEXT_LEFT);
				//nk_label(ctx, "E", NK_TEXT_RIGHT);

				nk_menu_item_label(ctx, "Delete", NK_TEXT_LEFT);
				nk_label(ctx, "DEL", NK_TEXT_RIGHT);

				nk_tree_pop(ctx);
			} else menu_state = (menu_state == MENU_EDIT) ? MENU_NONE: menu_state;

			state = (menu_state == MENU_VIEW) ? NK_MAXIMIZED: NK_MINIMIZED;
			if (nk_tree_state_push(ctx, NK_TREE_TAB, "Viewing Mode", &state)) {
				menu_state = MENU_VIEW;
				nk_layout_row(ctx, NK_DYNAMIC, 0, 2, &ratios[2]);

				if (nk_menu_item_label(ctx, "1 image", NK_TEXT_LEFT))
					n_imgs = 1;
				nk_label(ctx, "CTRL+1", NK_TEXT_RIGHT);
				if (nk_menu_item_label(ctx, "2 images", NK_TEXT_LEFT))
					n_imgs = 2;
				nk_label(ctx, "CTRL+2", NK_TEXT_RIGHT);
				if (nk_menu_item_label(ctx, "4 images", NK_TEXT_LEFT))
					n_imgs = 4;
				nk_label(ctx, "CTRL+4", NK_TEXT_RIGHT);
				if (nk_menu_item_label(ctx, "8 images", NK_TEXT_LEFT))
					n_imgs = 8;
				nk_label(ctx, "CTRL+8", NK_TEXT_RIGHT);
				nk_tree_pop(ctx);
			} else menu_state = (menu_state == MENU_VIEW) ? MENU_NONE: menu_state;


			nk_menu_end(ctx);
		}

		nk_button_set_behavior(ctx, NK_BUTTON_REPEATER);
		if (nk_button_symbol_label(ctx, NK_SYMBOL_TRIANGLE_LEFT, "prev", NK_TEXT_RIGHT)) {
			;
		}
		if (nk_button_symbol_label(ctx, NK_SYMBOL_TRIANGLE_RIGHT, "next", NK_TEXT_LEFT)) {
			;
		}

		//nk_label(ctx, "zoom:", NK_TEXT_RIGHT);
		bounds = nk_widget_bounds(ctx);
		if (nk_input_is_mouse_hovering_rect(in, bounds)) {
			hovering++;
			if (hovering > 20)
				nk_tooltip(ctx, "Shrink the image");
			what_hover = 1;
		}
		if (nk_button_symbol(ctx, NK_SYMBOL_MINUS)) {
			;
		}
		bounds = nk_widget_bounds(ctx);
		if (nk_input_is_mouse_hovering_rect(in, bounds)) {
			hovering++;
			if (hovering > 20)
				nk_tooltip(ctx, "Enlarge the image");
			what_hover = 2;
		}
		if (nk_button_symbol(ctx, NK_SYMBOL_PLUS)) {
			;
		}
		nk_button_set_behavior(ctx, NK_BUTTON_DEFAULT);


		/*
		//nk_select_symbol_label(ctx, NK_SYMBOL_RECT_OUTLINE, "Best fit", NK_TEXT_RIGHT, fill);
		//nk_selectable_symbol_label(ctx, NK_SYMBOL_RECT_OUTLINE, "Best fit", NK_TEXT_RIGHT, &fill);
		bounds = nk_widget_bounds(ctx);
		if (nk_input_is_mouse_hovering_rect(in, bounds)) {
			hovering++;
			if (hovering > 20)
				nk_tooltip(ctx, "Toggle fit image to window");
			what_hover = 3;
		}
		nk_selectable_label(ctx, "Best fit", NK_TEXT_RIGHT, &fill);
		bounds = nk_widget_bounds(ctx);
		if (nk_input_is_mouse_hovering_rect(in, bounds)) {
			hovering++;
			if (hovering > 20)
				nk_tooltip(ctx, "Toggle start a slideshow");
			what_hover = 4;
		}
		nk_selectable_label(ctx, "Slideshow", NK_TEXT_RIGHT, &slideshow);


		if (nk_button_label(ctx, "Actual")) {
			;
		}
		*/


		if (nk_button_label(ctx, "L")) {
			;
		}
		if (nk_button_label(ctx, "R")) {
			;
		}
		/*

		nk_label(ctx, "mode:", NK_TEXT_RIGHT);
		if (nk_button_label(ctx, "1")) {
			n_imgs = 1;
		}
		if (nk_button_label(ctx, "2")) {
			n_imgs = 2;
		}
		if (nk_button_label(ctx, "4")) {
			n_imgs = 4;
		}
		if (nk_button_label(ctx, "8")) {
			n_imgs = 8;
		}
		*/

		// still don't know if this has to be inside the if or just
		// before the nk_end()
	}

	nk_end(ctx);




	if (nk_begin(ctx, "Info", nk_rect(0, scr_h-30, scr_w, 30), gui_flags))
	{
		if (n_imgs == 1) {
			len = snprintf(info_buf, STRBUF_SZ, "1000 x 600 pixels   55 %%");
			if (len >= STRBUF_SZ) {
				puts("info path too long");
				exit(1);
			}
			nk_layout_row_static(ctx, 0, scr_w, 1);
			nk_label(ctx, info_buf, NK_TEXT_LEFT);
		}
	}
	nk_end(ctx);

	if (!what_hover)
		hovering = 0;
}


int handle_events(struct nk_context* ctx)
{
	SDL_Event evt;
	int sc;
	nk_input_begin(ctx);


	while (SDL_PollEvent(&evt)) {
		sc = evt.key.keysym.scancode;
		if (evt.type == SDL_QUIT)
			return 1;

		if (evt.type == SDL_KEYUP) {
			if (sc == SDL_SCANCODE_ESCAPE) {
				if (show_rotate) {
					show_rotate = nk_false;
				} else if (show_about) {
					show_about = nk_false;
					//nk_popup_close(ctx);
					//nk_popup_end(ctx);
				} else if (show_prefs) {
					show_prefs = nk_false;
				} else if (slideshow) {
					slideshow = 0;
				} else {
					return 1;
				}
			} else if (sc == SDL_SCANCODE_R) {
				show_rotate = nk_true;
			}
		} else if (evt.type == SDL_WINDOWEVENT) {
			if (evt.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
				//SDL_RenderSetLogicalSize(ren, evt.window.data1*x_scale, evt.window.data2*y_scale);
			}
		}
		nk_sdl_handle_event(&evt);
	}
	nk_input_end(ctx);

	return 0;
}



