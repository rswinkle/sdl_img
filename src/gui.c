
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
	//static struct nk_colorf win_color;
	//win_color = nk_color_cf(ctx->style.window.fixed_background.data.color);

	//bgf = nk_color_cf(g->bg);

	// Can't use actual screen size g->scr_w/h have to
	// calculate logical screen size since GUI is scaled
	int scr_w = g->scr_w/g->x_scale;
	int scr_h = g->scr_h/g->y_scale;

	struct nk_rect bounds;
	const struct nk_input* in = &ctx->input;




	// Do popups first so I can return early if eather is up
	if (g->show_rotate) {
		int w = 400, h = 300, tmp;
		struct nk_rect s;
		s.x = scr_w/2-w/2;
		s.y = scr_h/2-h/2;
		s.w = w;
		s.h = h;
		img = (g->n_imgs == 1) ? &g->img[0] : g->img_focus;

		if (nk_begin(ctx, "Arbitrary Rotation", s, prefs_flags)) {

			nk_layout_row_dynamic(ctx, 0, 1);

			nk_label_wrap(ctx, "Click and drag, type, or use the arrows to select the desired degree of rotation");

			tmp = nk_propertyi(ctx, "Degrees:", -180, img->rotdegs, 180, 1, 0.5);
			if (tmp != img->rotdegs) {
				img->rotated = TO_ROTATE;
				img->rotdegs = tmp;
			}
			//nk_label(ctx, "Degrees:", NK_TEXT_LEFT);
			//nk_slider_int(ctx, -180, &slider_degs, 180, 1);

			//nk_button_set_behavior(ctx, NK_BUTTON_DEFAULT);
			nk_layout_row_dynamic(ctx, 0, 2);
			if (nk_button_label(ctx, "Preview")) {
				if (img->rotated == TO_ROTATE) {
					event.user.code = ROT360;
					SDL_PushEvent(&event);
				}
			}
			if (nk_button_label(ctx, "Ok")) {
				if (img->rotated == TO_ROTATE) {
					event.user.code = ROT360;
					SDL_PushEvent(&event);
				} else {
					// clear state here since we don't need it
					// anymore and rotate_img won't be called
					free(g->orig_pix);
					g->orig_pix = NULL;
					g->orig_w = 0;
					g->orig_h = 0;
				}
				g->show_rotate = 0;;
			}
		}
		nk_end(ctx);
	}


	if (g->show_about) {
		int w = 550, h = 350; ///scale_x, h = 400/scale_y;
		struct nk_rect s;
		s.x = scr_w/2-w/2;
		s.y = scr_h/2-h/2;
		s.w = w;
		s.h = h;

		if (nk_begin(ctx, "About sdl_img", s, prefs_flags))
		{
			nk_layout_row_dynamic(ctx, 0, 1);
			nk_label(ctx, VERSION_STR, NK_TEXT_CENTERED);
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
			nk_label(ctx, "libcurl", NK_TEXT_LEFT);
			nk_label(ctx, "curl.haxx.se/libcurl/", NK_TEXT_RIGHT);

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
		int w = 550, h = 250; ///scale_x, h = 400/scale_y;
		struct nk_rect s;
		s.x = scr_w/2-w/2;
		s.y = scr_h/2-h/2;
		s.w = w;
		s.h = h;

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

				// doesn't make sense to have the background be transparent
				//bgf.a = nk_propertyf(ctx, "#A:", 0, bgf.a, 1.0f, 0.01f,0.005f);
				g->bg = nk_rgb_cf(bgf);
				nk_combo_end(ctx);
			}


			/*
			nk_label(ctx, "GUI Color:", NK_TEXT_LEFT);
			if (nk_combo_begin_color(ctx, nk_rgb_cf(win_color), nk_vec2(nk_widget_width(ctx), 400))) {
				nk_layout_row_dynamic(ctx, 120, 1);
				win_color = nk_color_picker(ctx, win_color, NK_RGBA);
				nk_layout_row_dynamic(ctx, 25, 1);
				win_color.r = nk_propertyf(ctx, "#R:", 0, win_color.r, 1.0f, 0.01f,0.005f);
				win_color.g = nk_propertyf(ctx, "#G:", 0, win_color.g, 1.0f, 0.01f,0.005f);
				win_color.b = nk_propertyf(ctx, "#B:", 0, win_color.b, 1.0f, 0.01f,0.005f);
				win_color.a = nk_propertyf(ctx, "#A:", 0, win_color.a, 1.0f, 0.01f,0.005f);
				ctx->style.window.fixed_background.data.color = nk_rgba_cf(win_color);
				nk_combo_end(ctx);
			}
			*/

			nk_label(ctx, "Slideshow delay:", NK_TEXT_LEFT);
			nk_property_int(ctx, "", 1, &g->slide_delay, 10, 1, 0.05);


			nk_layout_row_dynamic(ctx, 0, 1);
			nk_label(ctx, "Cache directory:", NK_TEXT_LEFT);
			nk_label_wrap(ctx, g->cachedir);


			nk_layout_row_dynamic(ctx, 0, 1);
			if (nk_button_label(ctx, "Ok")) {
				g->show_prefs = 0;;
			}
		}
		nk_end(ctx);

	}

	// don't show main GUI if a popup is up, don't want user to
	// be able to interact with it.  Could look up how to make them inactive
	// but meh, this is simpler
	if (g->show_about || g->show_prefs || g->show_rotate) {
		g->mouse_state = 1;
		g->mouse_timer = SDL_GetTicks();
		return;
	}

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

		// Rotate left and right
		nk_layout_row_template_push_static(ctx, 40);
		nk_layout_row_template_push_static(ctx, 40);

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
					event.user.code = DELETE_IMG;
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

		nk_button_set_behavior(ctx, NK_BUTTON_DEFAULT);

		// My brother insists that rotation be in buttons at the top
		// I think they're not that important, will be too often cut off in
		// small windows and using the keyboard is better anyway.  This
		// is my compromise, using single character buttons.
		if (nk_button_label(ctx, "L")) {
			event.user.code = ROT_LEFT;
			SDL_PushEvent(&event);
		}
		if (nk_button_label(ctx, "R")) {
			event.user.code = ROT_RIGHT;
			SDL_PushEvent(&event);
		}

		/*
		
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

			len = snprintf(info_buf, STRBUF_SZ, "%d x %d pixels  %.1f %s  %d %%    %d / %zu", img->w, img->h, sz, sizes[i], (int)(img->disp_rect.h*100.0/img->h), img->index+1, g->files.size);
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

