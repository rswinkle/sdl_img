
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
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_IMPLEMENTATION
#define NK_SDL_RENDERER_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_sdl_renderer.h"

#define CVECTOR_IMPLEMENTATION
#include "cvector.h"

#include "style_configurator.c"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

#define STRBUF_SZ 1024

int GUI_BAR_HEIGHT;
#define GUI_MENU_WIN_W 550
#define GUI_MENU_WIN_H 600

enum { MENU_NONE, MENU_MISC, MENU_SORT, MENU_EDIT, MENU_VIEW };
enum { DELAY, ALWAYS, NEVER };

int n_imgs = 1;
SDL_Window* win;
SDL_Renderer* ren;
int running;
int slideshow = nk_false;
int slide_delay = 3;
int thumb_rows = 8;
int thumb_cols = 15;
int gui_delay = 2;
nk_size gif_prog = 0;
int gif_prog_active = 0;
int show_about = nk_false;
int show_prefs = nk_false;
int show_rotate = nk_false;
int show_infobar = nk_true;
int thumb_x_deletes = nk_false;
int independent_multimode = nk_false;
int list_mode = nk_false;
int menu_state = MENU_NONE;

struct nk_colorf bg;
struct nk_color bg2;

float x_scale;
float y_scale;

cvector_str list1;
//cvector_i selected;


char license[STRBUF_SZ*4] =
"The MIT License (MIT)\n"
"\n"
"Copyright (c) 2017-2023 Robert Winkler\n"
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
void draw_prefs(struct nk_context* ctx, int scr_w, int scr_h);
void draw_simple_gui(struct nk_context* ctx);

int main(int argc, char** argv)
{
	struct nk_context *ctx;
	/* float bg[4]; */

	float font_size = 24.0f;
	if (argc > 1) {
		font_size = atof(argv[1]);
		if (!font_size) font_size = 24.0f;
	}

	// win.padding.y = 4
	// text.padding.y = 0
	// min_row_height_padding = 8
	// win.spacing.y = 4
	//
	// row height = height + win.spacing.y;
	//
	// if height is 0 use min height which is:
	// font_height + 2*min_row_height_padding + 2*text.padding.y;
	//
	// so automatic row height is
	// font_size + 16 + 4
	//
	// so total window height needed for that row
	// row_height + 2 * win.padding.y
	//
	// (font_size + 16 + 4) + 8 = font_size + 28
	//
	// plus extra for borders?
	GUI_BAR_HEIGHT = font_size + 28;
	
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

	x_scale = 1; //hdpi/72;  // adjust for dpi, then go from 8pt font to 12pt
	y_scale = 1; //vdpi/72;


	/*
	if (SDL_RenderSetLogicalSize(ren, WINDOW_WIDTH*x_scale, WINDOW_HEIGHT*y_scale)) {
		printf("logical size failure: %s\n", SDL_GetError());
		return 1;
	}
	*/

	char buffer[256];

	cvec_str(&list1, 0, 100);
	//cvec_i(&selected, 50, 100);
	//cvec_set_val_sz_i(&selected, 0);
	for (int i=0; i<50; i++) {
		sprintf(buffer, "hello %d", i);
		cvec_push_str(&list1, buffer);
	}

	if (!(ctx = nk_sdl_init(win, ren))) {
		printf("nk_sdl_init() failed!");
		return 1;
	}

	// TODO Font stuff, refactor/reorganize
	int render_w, render_h;
	int window_w, window_h;
	SDL_GetRendererOutputSize(ren, &render_w, &render_h);
	SDL_GetWindowSize(win, &window_w, &window_h);
	x_scale = (float)(render_w) / (float)(window_w);
	y_scale = (float)(render_h) / (float)(window_h);
	// could adjust for dpi, then adjust for font size if necessary
	//g->x_scale = 2; //hdpi/72;
	//g->y_scale = 2; //vdpi/72;
	//SDL_RenderSetScale(g->ren, g->x_scale, g->y_scale);
	float font_scale = y_scale;

	printf("scale %f %f\n", x_scale, y_scale);
	SDL_RenderSetScale(ren, x_scale, y_scale);

	struct nk_font_atlas* atlas;
	struct nk_font_config config = nk_font_config(0);
	struct nk_font* font;

	nk_sdl_font_stash_begin(&atlas);
	font = nk_font_atlas_add_default(atlas, font_size*font_scale, &config);
	//font = nk_font_atlas_add_from_file(atlas, "../fonts/kenvector_future_thin.ttf", 13 * font_scale, &config);
	nk_sdl_font_stash_end();

	font->handle.height /= font_scale;
	nk_style_set_font(ctx, &font->handle);

	struct nk_style_toggle* tog = &ctx->style.option;
	printf("padding = %f %f border = %f\n", tog->padding.x, tog->padding.y, tog->border);
	//tog->padding.x = 2;
	//tog->padding.y = 2;

    static struct nk_color color_table[NK_COLOR_COUNT];
    memcpy(color_table, nk_default_color_style, sizeof(color_table));


	bg2 = nk_rgb(128,158,62);
	bg = nk_color_cf(bg2);
	int start_time = SDL_GetTicks();
	int time;
	int mx, my;
	Uint32 mstate;
	while (running)
	{
		/*
		time = SDL_GetTicks();
		mstate = SDL_GetMouseState(&mx, &my);
		if (!(SDL_BUTTON_LMASK & mstate) && time - start_time > 50) {
			gif_prog++;
			gif_prog %= 112;
			printf("gif_prog = %lu\n", gif_prog);
			start_time = SDL_GetTicks();
		}
		*/
		//SDL_RenderSetScale(ren, x_scale, y_scale);

		if (handle_events(ctx))
			break;

		/* GUI */
		draw_gui(ctx);
		//draw_simple_gui(ctx);

		//style_configurator(ctx, color_table);
		//
		//
		
		//SDL_RenderSetScale(g->ren, g->x_scale, g->y_scale);
		//nk_sdl_render(NK_ANTI_ALIASING_ON);
		//SDL_RenderSetScale(g->ren, 1, 1);

		SDL_SetRenderDrawColor(ren, bg2.r, bg2.g, bg2.b, bg2.a);
		SDL_RenderSetClipRect(ren, NULL);
		SDL_RenderClear(ren);
		nk_sdl_render(NK_ANTI_ALIASING_ON);


		SDL_RenderPresent(ren);
		SDL_Delay(15);

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
	//int gui_flags = NK_WINDOW_NO_SCROLLBAR; //NK_WINDOW_BORDER|NK_WINDOW_TITLE;
	int gui_flags = NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER; //|NK_WINDOW_TITLE;
	//ctx->style.window.border_color = nk_rgb(255,255,255);

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
	//int gui_flags = NK_WINDOW_NO_SCROLLBAR; //NK_WINDOW_BORDER|NK_WINDOW_TITLE;
	int gui_flags = NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER; //|NK_WINDOW_TITLE;
	ctx->style.window.border_color = nk_rgb(255,255,255);

	// closable gives the x, if you use it it won't come back (probably have to call show() or
	// something...
	int popup_flags = NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_BORDER|NK_WINDOW_TITLE;//NK_WINDOW_CLOSABLE;
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

		if (nk_begin(ctx, "Rotation", s, popup_flags)) {

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
		int w = 700, h = 580; ///scale_x, h = 400/scale_y;
		struct nk_rect s;
		s.x = scr_w/2-w/2;
		s.y = scr_h/2-h/2;
		s.w = w;
		s.h = h;
		if (nk_begin(ctx, "About sdl_img", s, popup_flags))
		{
			nk_layout_row_dynamic(ctx, 0, 1);
			nk_label(ctx, "sdl_img 1.0", NK_TEXT_CENTERED);
			nk_label(ctx, "By Robert Winkler", NK_TEXT_LEFT);
			nk_label(ctx, "robertwinkler.com", NK_TEXT_LEFT);  //TODO project website
			nk_label(ctx, "sdl_img is licensed under the MIT License.",  NK_TEXT_LEFT);

			nk_label(ctx, "Credits:", NK_TEXT_CENTERED);
			//nk_layout_row_dynamic(ctx, 10, 2);
			float ratios[] = { 0.3f, 0.7f, 0.2f, 0.8f };
			nk_layout_row(ctx, NK_DYNAMIC, 0, 2, ratios);

			nk_label(ctx, "stb_image*", NK_TEXT_LEFT);
			nk_label(ctx, "github.com/nothings/stb", NK_TEXT_RIGHT);
			nk_label(ctx, "SDL2", NK_TEXT_LEFT);
			nk_label(ctx, "libsdl.org", NK_TEXT_RIGHT);
			nk_label(ctx, "Nuklear GUI", NK_TEXT_LEFT);
			nk_label(ctx, "github.com/Immediate-Mode-UI/Nuklear", NK_TEXT_RIGHT);
			nk_label(ctx, "libcurl", NK_TEXT_LEFT);
			nk_label(ctx, "curl.haxx.se/libcurl/", NK_TEXT_RIGHT);
			nk_label(ctx, "WjCryptLib_Md5", NK_TEXT_LEFT);
			nk_label(ctx, "github.com/WaterJuice/WjCryptLib", NK_TEXT_RIGHT);

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
		draw_prefs(ctx, scr_w, scr_h);

	}


	// don't show main GUI if a popup is up, don't want user to
	// be able to interact with it.  Could look up how to make them inactive
	// but meh, this is simpler
	if (show_about || show_prefs || show_rotate)
		return;


	struct nk_style_button s = ctx->style.button;
	//memcpy(&s, &ctx->style.button, sizeof(nk_button_style));
	s.border = 0;
	s.text_alignment = NK_TEXT_LEFT;

	static int selected = 25;
	static int first_time = 1;
	int is_selected = 0;

	int search_state = 0;
	static struct nk_list_view lview;
	int list_height;
	float search_height = 24;
	static float header_ratios[] = {0.49f, 0.01f, 0.15f, 0.01f, 0.34f };
	float search_ratio[] = { 0.25f, 0.75f };
	static int splitter_down = 0;
	static char field_buffer[64];
	static int field_len;

	int search_flags = NK_EDIT_FIELD | NK_EDIT_SIG_ENTER | NK_EDIT_GOTO_END_ON_ACTIVATE;

	if (!nk_input_is_mouse_down(in, NK_BUTTON_LEFT))
		splitter_down = 0;

	if (list_mode) {
		if (nk_begin(ctx, "List", nk_rect(0, GUI_BAR_HEIGHT, scr_w, scr_h-GUI_BAR_HEIGHT), gui_flags)) {

			nk_layout_row(ctx, NK_DYNAMIC, 0, 2, search_ratio);
			search_height = nk_widget_bounds(ctx).h;
			//printf("height = %f\n", search_height);
			nk_label(ctx, "Search Filenames:", NK_TEXT_LEFT);
			if ((search_state = nk_edit_string(ctx, search_flags, field_buffer, &field_len, 64, nk_filter_default))) {
				field_buffer[field_len] = 0;
				printf("edit state %d %d \"%s\"\n", search_state, SDL_GetTicks(), field_buffer);
			}

			nk_layout_row(ctx, NK_DYNAMIC, 0, 5, header_ratios);
			nk_button_label(ctx, "Name");

			/* scaler */
			bounds = nk_widget_bounds(ctx);
			nk_spacing(ctx, 1);
			if ((splitter_down == 1 || (nk_input_is_mouse_hovering_rect(in, bounds) && !splitter_down)) &&
			    nk_input_is_mouse_down(in, NK_BUTTON_LEFT)) {
				printf("spacer %f %f\n", ctx->current->layout->bounds.w, ctx->current->layout->clip.w);
				float change = in->mouse.delta.x/(ctx->current->layout->bounds.w-8);
				header_ratios[0] += change;
				header_ratios[2] -= change;
				if (header_ratios[2] < 0.05f) {
					header_ratios[2] = 0.05f;
					header_ratios[0] = 0.93f - header_ratios[4];
				} else if (header_ratios[0] < 0.05f) {
					header_ratios[0] = 0.05f;
					header_ratios[2] = 0.93f - header_ratios[4];
				}
				splitter_down = 1;
			}
			nk_button_label(ctx, "Size");

			bounds = nk_widget_bounds(ctx);
			nk_spacing(ctx, 1);
			if ((splitter_down == 2 || (nk_input_is_mouse_hovering_rect(in, bounds) && !splitter_down)) &&
			    nk_input_is_mouse_down(in, NK_BUTTON_LEFT)) {
				//printf("spacer %f %f\n", ctx->current->layout->bounds.w, ctx->current->layout->clip.w);
				float change = in->mouse.delta.x/(ctx->current->layout->bounds.w-8);
				header_ratios[2] += change;
				header_ratios[4] -= change;
				if (header_ratios[2] < 0.05f) {
					header_ratios[2] = 0.05f;
					header_ratios[4] = 0.93f - header_ratios[0];
				} else if (header_ratios[4] < 0.05f) {
					header_ratios[4] = 0.05f;
					header_ratios[2] = 0.93f - header_ratios[0];
				}
				splitter_down = 2;
			}
			nk_button_label(ctx, "Modified");

			//printf("%.3f %.3f %.3f %.3f %.3f\n", header_ratios[0], header_ratios[1], header_ratios[2], header_ratios[3], header_ratios[4]);

			float ratios[] = { header_ratios[0]+0.01f, header_ratios[2], header_ratios[4]+0.01f };

			// TODO figure out why border goes off the edge (ie we don't see the bottom border)
			nk_layout_row_dynamic(ctx, scr_h-GUI_BAR_HEIGHT-2*search_height, 1);
//nk_list_view_begin(struct nk_context *ctx, struct nk_list_view *view,
 //   const char *title, nk_flags flags, int row_height, int row_count)
			if (nk_list_view_begin(ctx, &lview, "Image List", NK_WINDOW_BORDER, 24, list1.size)) {
				nk_layout_row(ctx, NK_DYNAMIC, 0, 3, ratios);
				for (int i=lview.begin; i<lview.end; ++i) {
					is_selected = selected == i;
					//bounds = nk_widget_bounds(ctx);
					//printf("%f %f %f %f\n", bounds.x, bounds.y, bounds.w, bounds.h);
					if (nk_selectable_label(ctx, list1.a[i], NK_TEXT_LEFT, &is_selected)) {
						if (is_selected)
							selected = i;
						else
							selected = -1;
						printf("%s clicked %d\n", list1.a[i], selected);
					}
					/*
					if (nk_selectable_label(ctx, list1.a[i], NK_TEXT_LEFT, &selected.a[i])) {
						printf("%s clicked %d\n", list1.a[i], selected.a[i]);
					}
					*/
					/*
					if (nk_button_label_styled(ctx, &s, list1.a[i])) {
						printf("%s clicked\n", list1.a[i]);
					}
					*/
					nk_label(ctx, "col2", NK_TEXT_RIGHT);
					nk_label(ctx, "col3", NK_TEXT_RIGHT);
				}
				//printf("list height = %d\n", scr_h-GUI_BAR_HEIGHT-40);
    //view->count = (int)NK_MAX(nk_iceilf((layout->clip.h)/(float)row_height),0);
				//printf("list layout = %f %f %d %d\n", ctx->current->layout->clip.h, ctx->current->layout->bounds.h, nk_iceilf(ctx->current->layout->clip.h/28.0), lview.count);
				list_height = ctx->current->layout->clip.h;
				nk_list_view_end(&lview);
				//nk_group_end(ctx);
			}

			// view->begin = (int)NK_MAX(((float)view->scroll_value / (float)row_height), 0.0f);
			nk_uint x = 0, y;
			int scroll_limit = (lview.total_height - list_height);
			//printf("scroll_limit = %d\n", scroll_limit);
			y = (selected/(float)(list1.size-1) * scroll_limit) + 0.999f;
			if (first_time) {
				nk_group_set_scroll(ctx, "Image List", x, y);
				first_time = 0;
			}
			nk_group_get_scroll(ctx, "Image List", &x, &y);
			//printf("scroll %u %u\n", x, y);

			/*
			nk_uint x, y;
			if (first_time) {
				float group_height = scr_h-GUI_BAR_HEIGHT-40;
				nk_uint x = 0, y = (selected*14 / group_height) * group_height;
				printf("height = %f\n", nk_window_get_height(ctx));
				nk_group_set_scroll(ctx, "Image List", x, y);
				first_time = 0;
			}
			nk_group_get_scroll(ctx, "Image List", &x, &y);
			printf("scroll %u %u\n", x, y);
			*/
		}
		nk_end(ctx);

		//return;
	}



	if (nk_begin(ctx, "Controls", nk_rect(0, 0, scr_w, GUI_BAR_HEIGHT), gui_flags)) {

		//g->gui_rect = nk_window_get_bounds(ctx);
		//printf("gui %f %f %f %f\n", g->gui_rect.x, g->gui_rect.y, g->gui_rect.w, g->gui_rect.h);

		//nk_layout_row_static(ctx, 0, 80, 16);
		//nk_layout_row_dynamic(ctx, 0, 16);
		//
		struct nk_vec2 win_spacing = ctx->style.window.spacing;
		//ctx->style.menu_button.border = 1;
		//ctx->style.menu_button.border_color = nk_rgb(255, 0, 0);
		//ctx->style.window.padding.y = 0;
		//ctx->style.window.min_row_height_padding = 0;
		//ctx->style.window.menu_border = 0;
		
		//ctx->style.window.spacing.y = 0;
		//ctx->style.window.padding.y = 0;
		
		nk_layout_row_template_begin(ctx, 0);

#define MENU_BUTTON_WIDTH 80
#define PREV_NEXT_WIDTH 150
#define ZOOM_ROTATE_WIDTH 50
		// menu
		nk_layout_row_template_push_static(ctx, MENU_BUTTON_WIDTH);

		// prev next
		nk_layout_row_template_push_static(ctx, PREV_NEXT_WIDTH);
		nk_layout_row_template_push_static(ctx, PREV_NEXT_WIDTH);

		total_width += MENU_BUTTON_WIDTH + 2*PREV_NEXT_WIDTH;

		// zoom, -, +
		//nk_layout_row_template_push_static(ctx, 80);
		nk_layout_row_template_push_static(ctx, ZOOM_ROTATE_WIDTH);
		nk_layout_row_template_push_static(ctx, ZOOM_ROTATE_WIDTH);

		total_width += 2*ZOOM_ROTATE_WIDTH;
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
		nk_layout_row_template_push_static(ctx, ZOOM_ROTATE_WIDTH);
		nk_layout_row_template_push_static(ctx, ZOOM_ROTATE_WIDTH);
		total_width += 2*ZOOM_ROTATE_WIDTH;
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

		static int once = 0;


		/*
		struct nk_vec2 win_spacing = ctx->style.window.spacing;
		ctx->style.menu_button.border = 1;
		ctx->style.menu_button.border_color = nk_rgb(255, 0, 0);
		ctx->style.window.padding.y = 0;
		ctx->style.window.min_row_height_padding = 0;
		ctx->style.window.menu_border = 0;
		*/
		//ctx->style.menu_button.border = 1;
		//ctx->style.menu_button.border_color = nk_rgb(255, 0, 0);

		if (!once) {
			bounds = nk_widget_bounds(ctx);
			printf("win.spacing = %f %f\n", win_spacing.x, win_spacing.y);
			printf("win.padding.y = %f\n", ctx->style.window.padding.y);
			printf("bounds= { %f, %f, %f, %f}\n", bounds.x, bounds.y, bounds.w, bounds.h);
			once = 1;
		}
		//ctx->style.window.spacing.y = 0;
		//ctx->style.window.padding.y = 0;

		if (nk_menu_begin_label(ctx, "Menu", NK_TEXT_LEFT, nk_vec2(GUI_MENU_WIN_W, GUI_MENU_WIN_H))) {
			puts("menu clicked");

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

				nk_tree_pop(ctx);
			} else menu_state = (menu_state == MENU_MISC) ? MENU_NONE : menu_state;

			state = (menu_state == MENU_SORT) ? NK_MAXIMIZED : NK_MINIMIZED;
			if (nk_tree_state_push(ctx, NK_TREE_TAB, "Sort Actions", &state)) {
				menu_state = MENU_SORT;

				if (n_imgs == 1) {
					nk_layout_row(ctx, NK_DYNAMIC, 0, 2, &ratios[2]);
					nk_menu_item_label(ctx, "Mix images", NK_TEXT_LEFT);
					nk_label(ctx, "M", NK_TEXT_RIGHT);

					nk_menu_item_label(ctx, "Sort by name (default)", NK_TEXT_LEFT);
					nk_label(ctx, "N", NK_TEXT_RIGHT);
					nk_menu_item_label(ctx, "Sort by full path", NK_TEXT_LEFT);
					nk_label(ctx, "CTRL+N", NK_TEXT_RIGHT);
					nk_menu_item_label(ctx, "Sort by size", NK_TEXT_LEFT);
					nk_label(ctx, "Z", NK_TEXT_RIGHT);   // S is save...
					nk_menu_item_label(ctx, "Sort by last modified", NK_TEXT_LEFT);
					nk_label(ctx, "T", NK_TEXT_RIGHT);  // CTRL+T is thumb mode...
				} else {
					nk_layout_row_dynamic(ctx, 0, 1);
					nk_label(ctx, "Only available in 1 image mode", NK_TEXT_LEFT);
				}

				nk_tree_pop(ctx);
			} else menu_state = (menu_state == MENU_SORT) ? MENU_NONE : menu_state;



			state = (menu_state == MENU_EDIT) ? NK_MAXIMIZED : NK_MINIMIZED;
			if (nk_tree_state_push(ctx, NK_TREE_TAB, "Edit Actions", &state)) {
				menu_state = MENU_EDIT;
				nk_layout_row(ctx, NK_DYNAMIC, 0, 2, &ratios[2]);

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

				if (nk_menu_item_label(ctx, "1 image", NK_TEXT_LEFT)) {
					n_imgs = 1;
				}
				nk_label(ctx, "CTRL+1", NK_TEXT_RIGHT);
				if (nk_menu_item_label(ctx, "2 images", NK_TEXT_LEFT)) {
					n_imgs = 2;
				}
				nk_label(ctx, "CTRL+2", NK_TEXT_RIGHT);
				if (nk_menu_item_label(ctx, "4 images", NK_TEXT_LEFT)) {
					n_imgs = 4;
				}
				nk_label(ctx, "CTRL+4", NK_TEXT_RIGHT);
				if (nk_menu_item_label(ctx, "8 images", NK_TEXT_LEFT)) {
					n_imgs = 8;
				}
				nk_label(ctx, "CTRL+8", NK_TEXT_RIGHT);

				// have to treat switching back from list the same as I do switching
				// back from thumb, selection is the 1st image, have to load following
				// if in 2,4, or 8 mode
				if (nk_menu_item_label(ctx, "List Mode", NK_TEXT_LEFT))
					list_mode = nk_true;
				nk_label(ctx, "CTRL+L", NK_TEXT_RIGHT);

				if (nk_menu_item_label(ctx, "Thumb Mode", NK_TEXT_LEFT)) {
					list_mode = nk_false;
				}
				nk_label(ctx, "CTRL+U", NK_TEXT_RIGHT);


				nk_tree_pop(ctx);
			} else menu_state = (menu_state == MENU_VIEW) ? MENU_NONE: menu_state;


			nk_menu_end(ctx);
		}

		if (list_mode) {
			nk_end(ctx);  // end "Controls" window early
			return;
		}

		nk_button_set_behavior(ctx, NK_BUTTON_REPEATER);
		if (nk_button_symbol_label(ctx, NK_SYMBOL_TRIANGLE_LEFT, "Prev", NK_TEXT_RIGHT)) {
			;
		}
		if (nk_button_symbol_label(ctx, NK_SYMBOL_TRIANGLE_RIGHT, "Next", NK_TEXT_LEFT)) {
			puts("Next hit");
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




	if (show_infobar) {
		if (nk_begin(ctx, "Info", nk_rect(0, scr_h-GUI_BAR_HEIGHT, scr_w, GUI_BAR_HEIGHT), gui_flags))
		{
			if (n_imgs == 1) {
				len = snprintf(info_buf, STRBUF_SZ, "1000 x 600 pixels   55 %%");
				if (len >= STRBUF_SZ) {
					puts("info path too long");
					exit(1);
				}
				nk_layout_row_dynamic(ctx, 0, 2);
				nk_label(ctx, info_buf, NK_TEXT_LEFT);
				nk_progress(ctx, &gif_prog, 112, NK_MODIFIABLE);
			}
		}
		nk_end(ctx);
	}

	if (!what_hover)
		hovering = 0;
}

void draw_prefs(struct nk_context* ctx, int scr_w, int scr_h)
{
#define PREFS_W 860
#define PREFS_H 530
	int w = PREFS_W, h = PREFS_H; ///scale_x, h = 400/scale_y;
	struct nk_rect bounds;
	struct nk_rect s;
	s.x = scr_w/2-w/2;
	s.y = scr_h/2-h/2;
	s.w = w;
	s.h = h;
	char cache[] = "/home/someone/really/long/path_that_goes_forever/blahblahblah/.local/share/sdl_img";

	int popup_flags = NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_BORDER|NK_WINDOW_TITLE;//NK_WINDOW_CLOSABLE;

	if (nk_begin(ctx, "Preferences", s, popup_flags)) {
		nk_layout_row_dynamic(ctx, 0, 2);
		nk_label(ctx, "background:", NK_TEXT_LEFT);
		if (nk_combo_begin_color(ctx, nk_rgb_cf(bg), nk_vec2(nk_widget_width(ctx), PREFS_W/2))) {
			nk_layout_row_dynamic(ctx, 240, 1);
			bg = nk_color_picker(ctx, bg, NK_RGB);
			nk_layout_row_dynamic(ctx, 0, 1);
			bg.r = nk_propertyf(ctx, "#R:", 0, bg.r, 1.0f, 0.01f,0.005f);
			bg.g = nk_propertyf(ctx, "#G:", 0, bg.g, 1.0f, 0.01f,0.005f);
			bg.b = nk_propertyf(ctx, "#B:", 0, bg.b, 1.0f, 0.01f,0.005f);
			//bg.a = nk_propertyf(ctx, "#A:", 0, bg.a, 1.0f, 0.01f,0.005f);
			bg2 = nk_rgb_cf(bg);
			nk_combo_end(ctx);
		}

		nk_label(ctx, "Slideshow delay:", NK_TEXT_LEFT);
		nk_property_int(ctx, "#", 1, &slide_delay, 10, 1, 0.05);

		nk_label(ctx, "Hide GUI delay:", NK_TEXT_LEFT);
		nk_property_int(ctx, "#", 1, &gui_delay, 60, 1, 0.3);


		nk_label(ctx, "GUI in Fullscreen mode:", NK_TEXT_LEFT);
		static int fullscreen_gui = DELAY;
		static const char* gui_options[] = { "Delay", "Always", "Never" };
		bounds = nk_widget_bounds(ctx);
		//printf("bounds %f %f %f %f\n", bounds.x, bounds.y, bounds.w, bounds.h);
		fullscreen_gui = nk_combo(ctx, gui_options, NK_LEN(gui_options), fullscreen_gui, 12, nk_vec2(bounds.w, 300));
		// if (nk_option_label(ctx, "Delay", (fullscreen_gui == DELAY))) fullscreen_gui = DELAY;
		// if (nk_option_label(ctx, "Always", (fullscreen_gui == ALWAYS))) fullscreen_gui = ALWAYS;
		// if (nk_option_label(ctx, "Never", (fullscreen_gui == NEVER))) fullscreen_gui = NEVER;

		nk_property_int(ctx, "Thumb rows", 2, &thumb_rows, 8, 1, 0.05);
		nk_property_int(ctx, "Thumb cols", 4, &thumb_cols, 15, 1, 0.05);

		nk_checkbox_label(ctx, "Show info bar", &show_infobar);
		nk_checkbox_label(ctx, "x deletes in Thumb mode", &thumb_x_deletes);

		nk_layout_row_dynamic(ctx, 0, 1);
		nk_checkbox_label(ctx, "Preserve relative offsets in multimode movement", &independent_multimode);


		float ratios[] = { 0.25, 0.75 };
		nk_layout_row(ctx, NK_DYNAMIC, 60, 2, ratios);
		nk_label(ctx, "Cache directory:", NK_TEXT_LEFT);
		int cache_len = strlen(cache);
		nk_edit_string(ctx, NK_EDIT_SELECTABLE|NK_EDIT_CLIPBOARD, cache, &cache_len, cache_len+1, nk_filter_default);

		nk_layout_row_dynamic(ctx, 0, 1);
		if (nk_button_label(ctx, "Clear thumbnail cache")) {
			puts("Clearing thumbnails");
		}

#define OK_WIDTH 200
		nk_layout_space_begin(ctx, NK_STATIC, 60, 1);
		nk_layout_space_push(ctx, nk_rect(PREFS_W-OK_WIDTH-12, 20, OK_WIDTH, 40));
		if (nk_button_label(ctx, "Ok")) {
			show_prefs = 0;;
		}
		nk_layout_space_end(ctx);
#undef OK_WIDTH
	}
	nk_end(ctx);
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



