// TODO sin, cos, sqrt etc.
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
//#define NK_INCLUDE_FONT_BAKING
//#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear_sdl.c"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

#define STRBUF_SZ 1024

int n_imgs = 1;
SDL_Window* win;
SDL_Renderer* ren;

void draw_gui(struct nk_context* ctx);

int main(void)
{
	struct nk_color background;
	struct nk_colorf bg;
	int win_width, win_height;
	int running = 1;
	struct nk_context *ctx;
	/* float bg[4]; */

	/* SDL setup */
	if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		printf( "Can't init SDL:  %s\n", SDL_GetError( ) );
		return 1;
	}

	int win_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;

	win = SDL_CreateWindow("SDL2_gfx demo",
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

	if (!(ctx = nk_sdl_init(win, ren))) {
		printf("nk_sdl_init() failed!");
		return 1;
	}


	background = nk_rgb(28,48,62);
	bg = nk_color_cf(background);
	while (running)
	{
		/* Input */
		SDL_Event evt;
		nk_input_begin(ctx);
		while (SDL_PollEvent(&evt)) {
			if (evt.type == SDL_QUIT || evt.type == SDL_KEYUP && evt.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
				goto cleanup;
			nk_sdl_handle_event(&evt);
		}
		nk_input_end(ctx);

		/* GUI */
		draw_gui(ctx);


		SDL_Delay(50);
		SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
		SDL_RenderSetClipRect(ren, NULL);
		SDL_RenderClear(ren);
		nk_sdl_render();
		SDL_RenderPresent(ren);

	}

cleanup:
	nk_sdl_shutdown();
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}

void draw_gui(struct nk_context* ctx)
{
	char info_buf[STRBUF_SZ];
	int len;
	int gui_flags = NK_WINDOW_NO_SCROLLBAR; //NK_WINDOW_BORDER|NK_WINDOW_TITLE;
	SDL_Event event = { 0 };

	int scr_w, scr_h;
	SDL_GetWindowSize(win, &scr_w, &scr_h);

	struct nk_rect bounds;
	const struct nk_input* in = &ctx->input;


	if (nk_begin(ctx, "Menu", nk_rect(0, 0, scr_w, 60), gui_flags)) {

		//g->gui_rect = nk_window_get_bounds(ctx);
		//printf("gui %f %f %f %f\n", g->gui_rect.x, g->gui_rect.y, g->gui_rect.w, g->gui_rect.h);

		//nk_layout_row_static(ctx, 0, 80, 12);
		//nk_layout_row_dynamic(ctx, 0, 12);
		
		nk_layout_row_template_begin(ctx, 0);
		nk_layout_row_template_push_static(ctx, 80); 
		nk_layout_row_template_push_static(ctx, 80);

		nk_layout_row_template_push_static(ctx, 80);
		nk_layout_row_template_push_static(ctx, 40);
		nk_layout_row_template_push_static(ctx, 40);

		nk_layout_row_template_push_static(ctx, 90);
		nk_layout_row_template_push_static(ctx, 90);

		nk_layout_row_template_push_static(ctx, 80);
		nk_layout_row_template_push_static(ctx, 40);
		nk_layout_row_template_push_static(ctx, 40);
		nk_layout_row_template_push_static(ctx, 40);
		nk_layout_row_template_push_static(ctx, 40);
		nk_layout_row_template_end(ctx);

		nk_button_set_behavior(ctx, NK_BUTTON_REPEATER);
		if (nk_button_symbol_label(ctx, NK_SYMBOL_TRIANGLE_LEFT, "prev", NK_TEXT_RIGHT)) {
			;
		}
		if (nk_button_symbol_label(ctx, NK_SYMBOL_TRIANGLE_RIGHT, "next", NK_TEXT_LEFT)) {
			;
		}

		nk_label(ctx, "zoom:", NK_TEXT_RIGHT);
		if (nk_button_label(ctx, "-")) {
			;
		}
		if (nk_button_label(ctx, "+")) {
			;
		}
		nk_button_set_behavior(ctx, NK_BUTTON_DEFAULT);

		bounds = nk_widget_bounds(ctx);
		if (nk_input_is_mouse_hovering_rect(in, bounds)) {
			nk_tooltip(ctx, "Testing 1 2 3");
		}
		if (nk_button_label(ctx, "Rot Left")) {
			;
		}
		if (nk_button_label(ctx, "Rot Right")) {
			;
		}

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
	}
	nk_end(ctx);


	if (nk_begin(ctx, "Controls", nk_rect(0, scr_h-30, scr_w, 30), gui_flags))
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
}




