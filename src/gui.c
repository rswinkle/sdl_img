
void cleanup(int ret, int called_setup);

void draw_gui(struct nk_context* ctx)
{
	char info_buf[STRBUF_SZ];
	int len;
	int gui_flags = NK_WINDOW_NO_SCROLLBAR; //NK_WINDOW_BORDER|NK_WINDOW_TITLE;
	SDL_Event event = { .type = g->userevent };
	img_state* img;
	char* sizes[3] = { "bytes", "KB", "MB" }; // GB?  no way

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

		if (nk_menu_begin_label(ctx, "Menu", NK_TEXT_LEFT, nk_vec2(200, 400))) {
			nk_layout_row_dynamic(ctx, 0, 1);
			if (nk_menu_item_label(ctx, "About", NK_TEXT_LEFT)) {
				g->show_about = nk_true;
			}

			// TODO maybe make dynamic menus, ie since Nuklear doesn't support popups of popups
			// make Image Actions/Viewing mode change state of entire menu to only
			// show sub buttons
			nk_label(ctx, "Image Actions", NK_TEXT_LEFT);
				if (nk_menu_item_label(ctx, "Delete", NK_TEXT_RIGHT)) {
					event.user.code = DELETE;
					SDL_PushEvent(&event);
				}
				if (nk_menu_item_label(ctx, "Rotate Left", NK_TEXT_RIGHT)) {
					event.user.code = ROT_LEFT;
					SDL_PushEvent(&event);
				}
				if (nk_menu_item_label(ctx, "Rotate Right", NK_TEXT_RIGHT)) {
					event.user.code = ROT_RIGHT;
					SDL_PushEvent(&event);
				}

			nk_label(ctx, "Viewing Mode", NK_TEXT_LEFT);
				if (nk_menu_item_label(ctx, "1 image", NK_TEXT_RIGHT)) {
					event.user.code = MODE_CHANGE;
					event.user.data1 = (void*)MODE1;
					SDL_PushEvent(&event);
				}
				if (nk_menu_item_label(ctx, "2 images", NK_TEXT_RIGHT)) {
					event.user.code = MODE_CHANGE;
					event.user.data1 = (void*)MODE2;
					SDL_PushEvent(&event);
				}
				if (nk_menu_item_label(ctx, "4 images", NK_TEXT_RIGHT)) {
					event.user.code = MODE_CHANGE;
					event.user.data1 = (void*)MODE4;
					SDL_PushEvent(&event);
				}
				if (nk_menu_item_label(ctx, "8 images", NK_TEXT_RIGHT)) {
					event.user.code = MODE_CHANGE;
					event.user.data1 = (void*)MODE8;
					SDL_PushEvent(&event);
				}

			if (nk_menu_item_label(ctx, "Exit", NK_TEXT_LEFT)) {
				event.type = SDL_QUIT;
				SDL_PushEvent(&event);
			}

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
		
		nk_selectable_label(ctx, "Best fit", NK_TEXT_RIGHT, &g->fill_mode);
		// TODO
		nk_selectable_label(ctx, "Slideshow", NK_TEXT_RIGHT, &g->fill_mode);

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

		/*
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

		// still don't know if this has to be inside the if or just
		// before the nk_end()
		if (g->show_about)
		{
			int w = 400, h = 400;
			struct nk_rect s;
			s.x = scr_w/2-w/2;
			s.y = scr_h/2-h/2;
			s.w = w;
			s.h = h;
			if (nk_popup_begin(ctx, NK_POPUP_STATIC, "About sdl_img", NK_WINDOW_CLOSABLE, s))
			{
				nk_layout_row_dynamic(ctx, 20/scale_y, 1);
				nk_label(ctx, "sdl_img 1.0", NK_TEXT_CENTERED);
				nk_label(ctx, "By Robert Winkler", NK_TEXT_LEFT);
				nk_label(ctx, "robertwinkler.com", NK_TEXT_LEFT);  //TODO project website
				nk_label(ctx, "sdl_img is licensed under the MIT License.",  NK_TEXT_LEFT);

				// credits
				// Sean T Barret (sp?) single header libraries
				// stb_image, stb_image_write
				//
				// nuklear (which also uses stb libs)
				//
				// My own cvector lib

				nk_popup_end(ctx);
			} else g->show_about = nk_false;
		}
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

