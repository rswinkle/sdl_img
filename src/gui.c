
void cleanup(int ret, int called_setup);
void set_rect_bestfit(img_state* img, int fill_screen);
void set_fullscreen();

enum { MENU_NONE, MENU_EDIT, MENU_VIEW };

void draw_gui(struct nk_context* ctx)
{
	char info_buf[STRBUF_SZ];
	int len;

	int gui_flags = NK_WINDOW_NO_SCROLLBAR; //NK_WINDOW_BORDER|NK_WINDOW_TITLE;

	// closable gives the x, if you use it it won't come back (probably have to call show() or
	// something...
	int prefs_flags = NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_BORDER|NK_WINDOW_TITLE;//NK_WINDOW_CLOSABLE;

	SDL_Event event = { .type = g->userevent };
	img_state* img;
	char* sizes[3] = { "bytes", "KB", "MB" }; // GB?  no way

	static struct nk_colorf bgf = { 0, 0, 0, 1 };

	//bgf = nk_color_cf(g->bg);

	// Can't use actual screen size g->scr_w/h have to
	// calculate logical screen size since GUI is scaled
	int scr_w = g->scr_w/g->x_scale;
	int scr_h = g->scr_h/g->y_scale;

	struct nk_rect bounds;
	const struct nk_input* in = &ctx->input;

	if (nk_begin(ctx, "Controls", nk_rect(0, 0, scr_w, 30), gui_flags))
	{
		nk_layout_row_template_begin(ctx, 0);

		// menu
		nk_layout_row_template_push_static(ctx, 50);

		// prev next
		nk_layout_row_template_push_static(ctx, 80);
		nk_layout_row_template_push_static(ctx, 80);

		// zoom, -, +
		//nk_layout_row_template_push_static(ctx, 80);
		nk_layout_row_template_push_static(ctx, 40);
		nk_layout_row_template_push_static(ctx, 40);

		// best fit, slideshow, and actual size
		nk_layout_row_template_push_static(ctx, 80);
		nk_layout_row_template_push_static(ctx, 80);
		nk_layout_row_template_push_static(ctx, 80);

		// Rotate left and right
		nk_layout_row_template_push_static(ctx, 90);
		nk_layout_row_template_push_static(ctx, 90);

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

			// as long as menu is open don't let gui disappear
			g->mouse_timer = SDL_GetTicks();

			nk_layout_row_dynamic(ctx, 0, 3);
			nk_label(ctx, "GUI:", NK_TEXT_LEFT);
			if (nk_menu_item_label(ctx, "-", NK_TEXT_CENTERED)) {
				g->x_scale -= 0.5;
				g->y_scale -= 0.5;
				if (g->x_scale < 1) {
					g->x_scale = 1;
					g->y_scale = 1;
				}
				nk_sdl_scale(g->x_scale, g->y_scale);
			}
			if (nk_menu_item_label(ctx, "+", NK_TEXT_CENTERED)) {
				g->x_scale += 0.5;
				g->y_scale += 0.5;
				nk_sdl_scale(g->x_scale, g->y_scale);
			}

			nk_layout_row_dynamic(ctx, 0, 1);
			if (nk_menu_item_label(ctx, "Preferences", NK_TEXT_LEFT)) {
				g->show_prefs = nk_true;
			}
			if (nk_menu_item_label(ctx, "About", NK_TEXT_LEFT)) {
				g->show_about = nk_true;
			}
			if (nk_menu_item_label(ctx, "Exit", NK_TEXT_LEFT)) {
				event.type = SDL_QUIT;
				SDL_PushEvent(&event);
			}

			state = (g->menu_state == MENU_EDIT) ? NK_MAXIMIZED: NK_MINIMIZED;
			if (nk_tree_state_push(ctx, NK_TREE_TAB, "Image Actions", &state)) {
				g->menu_state = MENU_EDIT;
				nk_layout_row(ctx, NK_DYNAMIC, 0, 2, ratios);

				if (nk_selectable_label(ctx, "Slideshow", NK_TEXT_LEFT, &g->slideshow)) {
					if (g->slideshow)
						g->slideshow = g->slide_delay*1000;
				}
				nk_label(ctx, "F1 - F10/ESC", NK_TEXT_RIGHT);

				// TODO figure out why menu_item_labels are slightly wider (on both sides)
				// than selectables
				if (nk_selectable_label(ctx, "Fullscreen", NK_TEXT_LEFT, &g->fullscreen)) {
					set_fullscreen();
				}
				nk_label(ctx, "ALT+F or F11", NK_TEXT_RIGHT);

				if (nk_menu_item_label(ctx, "Actual Size", NK_TEXT_LEFT)) {
					event.user.code = ACTUAL_SIZE;
					SDL_PushEvent(&event);
				}
				nk_label(ctx, "A", NK_TEXT_RIGHT);

				if (nk_selectable_label(ctx, "Best fit", NK_TEXT_LEFT, &g->fill_mode)) {
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i)
							set_rect_bestfit(&g->img[i], g->fullscreen | g->slideshow | g->fill_mode);
					} else {
						set_rect_bestfit(g->img_focus, g->fullscreen | g->slideshow | g->fill_mode);
					}
				}
				nk_label(ctx, "F", NK_TEXT_RIGHT);

				if (nk_menu_item_label(ctx, "Rotate Left", NK_TEXT_LEFT)) {
					event.user.code = ROT_LEFT;
					SDL_PushEvent(&event);
				}
				nk_label(ctx, "L", NK_TEXT_RIGHT);

				if (nk_menu_item_label(ctx, "Rotate Right", NK_TEXT_LEFT)) {
					event.user.code = ROT_RIGHT;
					SDL_PushEvent(&event);
				}
				nk_label(ctx, "R", NK_TEXT_RIGHT);

				if (nk_menu_item_label(ctx, "Delete", NK_TEXT_LEFT)) {
					event.user.code = DELETE;
					SDL_PushEvent(&event);
				}
				nk_label(ctx, "DEL", NK_TEXT_RIGHT);

				nk_tree_pop(ctx);
			} else g->menu_state = (g->menu_state == MENU_EDIT) ? MENU_NONE: g->menu_state;


			state = (g->menu_state == MENU_VIEW) ? NK_MAXIMIZED: NK_MINIMIZED;
			if (nk_tree_state_push(ctx, NK_TREE_TAB, "Viewing Mode", &state)) {
				g->menu_state = MENU_VIEW;
				nk_layout_row(ctx, NK_DYNAMIC, 0, 2, &ratios[2]);

				if (nk_menu_item_label(ctx, "1 image", NK_TEXT_LEFT)) {
					event.user.code = MODE_CHANGE;
					event.user.data1 = (void*)MODE1;
					SDL_PushEvent(&event);
				}
				nk_label(ctx, "CTRL+1", NK_TEXT_RIGHT);

				if (nk_menu_item_label(ctx, "2 images", NK_TEXT_LEFT)) {
					event.user.code = MODE_CHANGE;
					event.user.data1 = (void*)MODE2;
					SDL_PushEvent(&event);
				}
				nk_label(ctx, "CTRL+2", NK_TEXT_RIGHT);

				if (nk_menu_item_label(ctx, "4 images", NK_TEXT_LEFT)) {
					event.user.code = MODE_CHANGE;
					event.user.data1 = (void*)MODE4;
					SDL_PushEvent(&event);
				}
				nk_label(ctx, "CTRL+4", NK_TEXT_RIGHT);

				if (nk_menu_item_label(ctx, "8 images", NK_TEXT_LEFT)) {
					event.user.code = MODE_CHANGE;
					event.user.data1 = (void*)MODE8;
					SDL_PushEvent(&event);
				}
				nk_label(ctx, "CTRL+8", NK_TEXT_RIGHT);

				nk_tree_pop(ctx);
			} else g->menu_state = (g->menu_state == MENU_VIEW) ? MENU_NONE: g->menu_state;

			nk_menu_end(ctx);
		}

		nk_button_set_behavior(ctx, NK_BUTTON_REPEATER);
		if (nk_button_symbol_label(ctx, NK_SYMBOL_TRIANGLE_LEFT, "prev", NK_TEXT_RIGHT)) {
			event.user.code = PREV;
			SDL_PushEvent(&event);
		}
		if (nk_button_symbol_label(ctx, NK_SYMBOL_TRIANGLE_RIGHT, "next", NK_TEXT_LEFT)) {
			event.user.code = NEXT;
			SDL_PushEvent(&event);
		}


		//bounds = nk_widget_bounds(ctx);
		if (nk_button_symbol(ctx, NK_SYMBOL_MINUS)) {
			event.user.code = ZOOM_MINUS;
			SDL_PushEvent(&event);
		}
		//bounds = nk_widget_bounds(ctx);
		if (nk_button_symbol(ctx, NK_SYMBOL_PLUS)) {
			event.user.code = ZOOM_PLUS;
			SDL_PushEvent(&event);
		}


		/*
		nk_button_set_behavior(ctx, NK_BUTTON_DEFAULT);
		
		if (nk_selectable_label(ctx, "Best fit", NK_TEXT_RIGHT, &g->fill_mode)) {
			if (!g->img_focus) {
				for (int i=0; i<g->n_imgs; ++i)
					set_rect_bestfit(&g->img[i], g->fullscreen | g->slideshow | g->fill_mode);
			} else {
				set_rect_bestfit(g->img_focus, g->fullscreen | g->slideshow | g->fill_mode);
			}
		}
		// TODO
		if (nk_selectable_label(ctx, "Slideshow", NK_TEXT_RIGHT, &g->slideshow)) {
			if (g->slideshow)
				g->slideshow = g->slide_delay*1000;
		}

		if (nk_button_label(ctx, "Actual")) {
			event.user.code = ACTUAL_SIZE;
			SDL_PushEvent(&event);
		}

		if (nk_button_label(ctx, "Rot Left")) {
			event.user.code = ROT_LEFT;
			SDL_PushEvent(&event);
		}
		if (nk_button_label(ctx, "Rot Right")) {
			event.user.code = ROT_RIGHT;
			SDL_PushEvent(&event);
		}

		nk_label(ctx, "mode:", NK_TEXT_RIGHT);
		if (nk_button_label(ctx, "1")) {
			event.user.code = MODE_CHANGE;
			event.user.data1 = MODE1;
			SDL_PushEvent(&event);
		}
		if (nk_button_label(ctx, "2")) {
			event.user.code = MODE_CHANGE;
			event.user.data1 = MODE2;
			SDL_PushEvent(&event);
		}
		if (nk_button_label(ctx, "4")) {
			event.user.code = MODE_CHANGE;
			event.user.data1 = MODE4;
			SDL_PushEvent(&event);
		}
		if (nk_button_label(ctx, "8")) {
			event.user.code = MODE_CHANGE;
			event.user.data1 = MODE8;
			SDL_PushEvent(&event);
		}
		*/

	}
	nk_end(ctx);

	if (g->show_about) {
		int w = 500, h = 400; ///scale_x, h = 400/scale_y;
		struct nk_rect s;
		s.x = scr_w/2-w/2;
		s.y = scr_h/2-h/2;
		s.w = w;
		s.h = h;

		// as long as a "popup" is open don't let gui disappear
		g->mouse_timer = SDL_GetTicks();
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

			nk_layout_row_dynamic(ctx, 0, 1);
			if (nk_button_label(ctx, "Ok")) {
				g->show_about = 0;;
			}
		}
		nk_end(ctx);
	}

	if (g->show_prefs) {
		int w = 400, h = 400; ///scale_x, h = 400/scale_y;
		struct nk_rect s;
		s.x = scr_w/2-w/2;
		s.y = scr_h/2-h/2;
		s.w = w;
		s.h = h;

		// as long as a "popup" is open don't let gui disappear
		g->mouse_timer = SDL_GetTicks();

		if (nk_begin(ctx, "Preferences", s, prefs_flags)) {
			nk_layout_row_dynamic(ctx, 0, 2);
			nk_label(ctx, "background:", NK_TEXT_LEFT);
			if (nk_combo_begin_color(ctx, nk_rgb_cf(bgf), nk_vec2(nk_widget_width(ctx), 400))) {
				nk_layout_row_dynamic(ctx, 120, 1);
				bgf = nk_color_picker(ctx, bgf, NK_RGBA);
				nk_layout_row_dynamic(ctx, 25, 1);
				bgf.r = nk_propertyf(ctx, "#R:", 0, bgf.r, 1.0f, 0.01f,0.005f);
				bgf.g = nk_propertyf(ctx, "#G:", 0, bgf.g, 1.0f, 0.01f,0.005f);
				bgf.b = nk_propertyf(ctx, "#B:", 0, bgf.b, 1.0f, 0.01f,0.005f);
				bgf.a = nk_propertyf(ctx, "#A:", 0, bgf.a, 1.0f, 0.01f,0.005f);
				g->bg = nk_rgb_cf(bgf);
				nk_combo_end(ctx);
			}

			nk_label(ctx, "Slideshow delay:", NK_TEXT_LEFT);
			nk_property_int(ctx, "", 1, &g->slide_delay, 10, 1, 0.05);


			nk_layout_row_dynamic(ctx, 0, 1);
			nk_label(ctx, "Cache directory:", NK_TEXT_LEFT);
			nk_label(ctx, g->cachedir, NK_TEXT_RIGHT);


			nk_layout_row_dynamic(ctx, 0, 1);
			if (nk_button_label(ctx, "Ok")) {
				g->show_prefs = 0;;
			}
		}
		nk_end(ctx);

	}

	if (nk_begin(ctx, "Info", nk_rect(0, scr_h-30, scr_w, 30), gui_flags))
	{
		if (g->n_imgs == 1) {
			img = &g->img[0];

			int i = 0;
			double sz = img->file_size;
			if (sz >= 1000000) {
				sz /= 1000000;
				i = 2;
			} else if (sz >= 1000) {
				sz /= 1000;
				i = 1;
			} else {
				i = 0;
			}

			len = snprintf(info_buf, STRBUF_SZ, "%d x %d pixels  %.1f %s  %d %%", img->w, img->h, sz, sizes[i], (int)(img->disp_rect.h*100.0/img->h));
			if (len >= STRBUF_SZ) {
				puts("info path too long");
				cleanup(1, 1);
			}
			nk_layout_row_static(ctx, 0, scr_w, 1);
			nk_label(ctx, info_buf, NK_TEXT_LEFT);
		}
	}
	nk_end(ctx);
}

