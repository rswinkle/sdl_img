
void cleanup(int ret, int called_setup);

void draw_gui(struct nk_context* ctx)
{
	char info_buf[STRBUF_SZ];
	int len;
	int gui_flags = NK_WINDOW_NO_SCROLLBAR; //NK_WINDOW_BORDER|NK_WINDOW_TITLE;
	SDL_Event event = { 0 };
	img_state* img;
	char* sizes[3] = { "bytes", "KB", "MB" }; // GB?  no way

	if (nk_begin(ctx, "Controls", nk_rect(0, g->scr_h-100, g->scr_w, 100), gui_flags))
	{
		g->gui_rect = nk_window_get_bounds(ctx);
		//printf("gui %f %f %f %f\n", g->gui_rect.x, g->gui_rect.y, g->gui_rect.w, g->gui_rect.h);

		nk_layout_row_static(ctx, 0, 80, 7);
		//nk_layout_row_dynamic(ctx, 0, 4);
		nk_button_set_behavior(ctx, NK_BUTTON_REPEATER);
		if (nk_button_symbol_label(ctx, NK_SYMBOL_TRIANGLE_LEFT, "prev", NK_TEXT_RIGHT)) {
			event.type = g->userevents + PREV;
			SDL_PushEvent(&event);
		}
		if (nk_button_symbol_label(ctx, NK_SYMBOL_TRIANGLE_RIGHT, "next", NK_TEXT_LEFT)) {
			event.type = g->userevents + NEXT;
			SDL_PushEvent(&event);
		}

		nk_label(ctx, "zoom:", NK_TEXT_RIGHT);
		if (nk_button_label(ctx, "-")) {
			event.type = g->userevents + ZOOM_MINUS;
			SDL_PushEvent(&event);
		}
		if (nk_button_label(ctx, "+")) {
			event.type = g->userevents + ZOOM_PLUS;
			SDL_PushEvent(&event);
		}
		nk_button_set_behavior(ctx, NK_BUTTON_DEFAULT);
		if (nk_button_label(ctx, "Rot Left")) {
			event.type = g->userevents + ROT_LEFT;
			SDL_PushEvent(&event);
		}
		if (nk_button_label(ctx, "Rot Right")) {
			event.type = g->userevents + ROT_RIGHT;
			SDL_PushEvent(&event);
		}

		nk_label(ctx, "mode:", NK_TEXT_RIGHT);
		if (nk_button_label(ctx, "1")) {
			event.type = g->userevents + MODE_CHANGE;
			event.user.code = MODE1;
			SDL_PushEvent(&event);
		}
		if (nk_button_label(ctx, "2")) {
			event.type = g->userevents + MODE_CHANGE;
			event.user.code = MODE2;
			SDL_PushEvent(&event);
		}
		if (nk_button_label(ctx, "4")) {
			event.type = g->userevents + MODE_CHANGE;
			event.user.code = MODE4;
			SDL_PushEvent(&event);
		}
		if (nk_button_label(ctx, "8")) {
			event.type = g->userevents + MODE_CHANGE;
			event.user.code = MODE8;
			SDL_PushEvent(&event);
		}


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
			nk_layout_row_static(ctx, 0, g->scr_w, 1);
			nk_label(ctx, info_buf, NK_TEXT_LEFT);
		}
	}
	nk_end(ctx);
}

