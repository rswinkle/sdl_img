

enum { RENAMING_PLIST = 1, NEW_PLIST };

void draw_file_list(struct nk_context* ctx, int scr_w, int scr_h);
void draw_playlists_menu(struct nk_context* ctx, int scr_w, int scr_h);

void draw_library(struct nk_context* ctx, int scr_w, int scr_h)
{
	int edit_flags = NK_EDIT_FIELD | NK_EDIT_SIG_ENTER | NK_EDIT_GOTO_END_ON_ACTIVATE;
	SDL_Event event = { .type = g->userevent };

	//// 2*minrowpadding which is 8 + win.spacing.y which is 4 = 20
	//int row_height = g->font_size + 20;
	//// set to number of *fully visible* rows in the list_view
	//// ie clip.h or bounds.h / row_height
	//int full_rows;

	struct nk_rect bounds;
	//const struct nk_input* in = &ctx->input;

	static nk_bool show_search = nk_false;
	static char buf[STRBUF_SZ];
	static int buf_len = 0;

	char tmp_buf[STRBUF_SZ];

	int active;
	int is_selected;

	// TODO better name
	// TODO could have a draggable splitter here too?
	const float group_szs[] = { g->gui_sidebar_w, scr_w-g->gui_sidebar_w-8 };

	if (nk_begin(ctx, "Library", nk_rect(0, 0, scr_w, scr_h), NK_WINDOW_NO_SCROLLBAR)) {

		// TODO subtract some height for info bar? or only
		// separately/internally to the groups?
		nk_layout_row(ctx, NK_STATIC, scr_h, 2, group_szs);

		if (nk_group_begin(ctx, "Library Sidebar", NK_WINDOW_NO_SCROLLBAR)) {
			nk_layout_row_dynamic(ctx, 0, 1);


			// "Image Queue" "Currently Open" hmm
			if (nk_selectable_label(ctx, "Current", NK_TEXT_CENTERED, &g->cur_selected)) {
				if (g->cur_selected) {
					g->lib_selected = nk_false;
					g->selected_plist = -1;
					g->list_view = &g->files;
				} else {
					// allow rename?
				}
			}

			// Could just have the Library be a regular playlist with everything in it...
			// not alow renaming?
			if (nk_selectable_label(ctx, "Library", NK_TEXT_CENTERED, &g->lib_selected)) {
				if (g->lib_selected) {
					g->selected_plist = -1;
					load_library(&g->lib_mode_list);
					g->list_view = &g->lib_mode_list;
				} else {
					// allow renaming?
					;
				}
			}

			nk_label(ctx, "Playlists", NK_TEXT_LEFT);
			bounds = nk_widget_bounds(ctx);
			draw_playlists_menu(ctx, group_szs[0], scr_h-bounds.y);

			nk_group_end(ctx);
		}

		if (nk_group_begin(ctx, "Library Main", NK_WINDOW_NO_SCROLLBAR)) {

			/*
			if (show_search) {
				nk_layout_row_dynamic(ctx, 0, 1);
				active = nk_edit_string(ctx, search_flags, text_buf, &text_len, STRBUF_SZ, nk_filter_default);

				// TODO rethink search when you can search lists that aren't actually
				// open...
			}
			*/

			//draw_file_list(ctx, scr_w, scr_h);
			draw_file_list(ctx, group_szs[1], scr_h);




			nk_group_end(ctx);
		}

	}
	nk_end(ctx);


}

void draw_file_list(struct nk_context* ctx, int scr_w, int scr_h)
{
	static const char invalid_img[] = "Could Not Load";
	static struct nk_list_view lview, rview;
	static float header_ratios[] = {0.49f, 0.01f, 0.15f, 0.01f, 0.34f };
	static int splitter_down = 0;

	// 2*minrowpadding which is 8 + win.spacing.y which is 4 = 20
	int row_height = g->font_size + 20;
	// set to number of *fully visible* rows in the list_view
	// ie clip.h or bounds.h / row_height
	int full_rows;

	int list_height;
	int is_selected;
	char* name;
	struct nk_rect bounds;

	int sorted_state;
	int is_current;
	const struct nk_input* in = &ctx->input;
	cvector_file* lv = g->list_view;

	SDL_Event event = { .type = g->userevent };

	if (lv != &g->files) {
		event.user.data1 = (void*)nk_true;
		sorted_state = g->lib_sorted_state;
		is_current = nk_false;
	} else {
		// already 0
		//event.user.data1 = (void*)nk_false;
		sorted_state = g->sorted_state;
		is_current = nk_true;
	}

	if (!nk_input_is_mouse_down(in, NK_BUTTON_LEFT))
		splitter_down = 0;

	nk_layout_row(ctx, NK_DYNAMIC, 0, 5, header_ratios);

	// TODO ask users which makes more sense up arrow to indicate sorting
	// lexicographically ascending (ie alphabetically) or down arrow to
	// indicate what clicking the button will do (ie sort descending)
	int symbol = NK_SYMBOL_NONE;
	if (sorted_state == NAME_UP)
		symbol = NK_SYMBOL_TRIANGLE_UP;
	else if (sorted_state == NAME_DOWN)
		symbol = NK_SYMBOL_TRIANGLE_DOWN;

	// TODO name or path?
	// TODO separate events is probably the best solution to separate
	// sorting files from sorting list_view
	if (!g->generating_thumbs || !is_current) {
		if (nk_button_symbol_label(ctx, symbol, "Name", NK_TEXT_LEFT)) {
			event.user.code = SORT_NAME;
			SDL_PushEvent(&event);
		}
	} else {
		nk_label(ctx, "Name", NK_TEXT_CENTERED);
	}

	bounds = nk_widget_bounds(ctx);
	nk_spacing(ctx, 1);
	if ((splitter_down == 1 ||
	     (nk_input_is_mouse_hovering_rect(in, bounds) && !splitter_down)) &&
	    nk_input_is_mouse_down(in, NK_BUTTON_LEFT)) {
		// TODO why -8 exactly? 2*win_padding?
		float change = in->mouse.delta.x/(ctx->current->layout->bounds.w-8);
		//float change = in->mouse.delta.x/(scr_w-8);
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

	// I hate redundant logic but the alternative is repeated gui code
	// TODO think of a better way
	symbol = NK_SYMBOL_NONE; // 0
	if (sorted_state == SIZE_UP)
		symbol = NK_SYMBOL_TRIANGLE_UP;
	else if (sorted_state == SIZE_DOWN)
		symbol = NK_SYMBOL_TRIANGLE_DOWN;

	if (!g->generating_thumbs || !is_current) {
		if (nk_button_symbol_label(ctx, symbol, "Size", NK_TEXT_LEFT)) {
			event.user.code = SORT_SIZE;
			SDL_PushEvent(&event);
		}
	} else {
		nk_label(ctx, "Size", NK_TEXT_CENTERED);
	}

	bounds = nk_widget_bounds(ctx);
	nk_spacing(ctx, 1);
	if ((splitter_down == 2 ||
	     (nk_input_is_mouse_hovering_rect(in, bounds) && !splitter_down)) &&
	    nk_input_is_mouse_down(in, NK_BUTTON_LEFT)) {
		float change = in->mouse.delta.x/(ctx->current->layout->bounds.w-8);
		//float change = in->mouse.delta.x/(scr_w-8);
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

	symbol = NK_SYMBOL_NONE; // 0
	if (sorted_state == MODIFIED_UP)
		symbol = NK_SYMBOL_TRIANGLE_UP;
	else if (sorted_state == MODIFIED_DOWN)
		symbol = NK_SYMBOL_TRIANGLE_DOWN;

	if (!g->generating_thumbs || !is_current) {
		if (nk_button_symbol_label(ctx, symbol, "Modified", NK_TEXT_LEFT)) {
			event.user.code = SORT_MODIFIED;
			SDL_PushEvent(&event);
		}
	} else {
		nk_label(ctx, "Modified", NK_TEXT_CENTERED);
	}

	
	float ratios[] = { header_ratios[0]+0.01f, header_ratios[2], header_ratios[4]+0.01f };

	//struct nk_vec2 win_spacing = ctx->style.window.spacing;
	//nk_layout_row_dynamic(ctx, scr_h-2*search_height-8, 1);
	// 2*(font_ht + 16 + win_spacing(4))
	// 2 * win_padding (4)
	bounds = nk_widget_bounds(ctx);
	nk_layout_row_dynamic(ctx, scr_h-bounds.y, 1);
	//nk_layout_row_dynamic(ctx, scr_h-2*(g->font_size + 20)-8, 1);
	
	// Not sure we even need this except for g->files
	if (g->state & SEARCH_RESULTS) {
		if (!g->search_results.size) {
			if (nk_button_label(ctx, "No matching results")) {
				g->state = LIST_DFLT;
				text_buf[0] = 0;
				text_len = 0;
				g->selection = g->img[0].index;
			}
		} else {
			// nk_list_view_begin
		}
	} else {
		if (nk_list_view_begin(ctx, &lview, "Image List", NK_WINDOW_BORDER, g->font_size+16, lv->size)) {
			nk_layout_row(ctx, NK_DYNAMIC, 0, 3, ratios);
			for (int i=lview.begin; i<lview.end; ++i) {
				assert(i < lv->size);

				is_selected = g->selection == i;

				// This is unnecessary for anything but g->files...
				name = lv->a[i].name;
				if (!name) name = (char*)invalid_img;

				// TODO right click menu
				if (nk_selectable_label(ctx, name, NK_TEXT_LEFT, &is_selected)) {
					if (is_selected) {
						g->selection = i;
					} else {
						// could support unselecting, esp. with CTRL somehow if I ever allow
						// multiple selection
						// g->selection = -1;

						// for now, treat clicking a selection as a "double click" ie same as return
						// TODO only for files?  worth it?
						/*
						g->selection = (g->selection) ? g->selection - 1 : g->files.size-1;

						g->state = NORMAL;
						SDL_ShowCursor(SDL_ENABLE);
						g->gui_timer = SDL_GetTicks();
						g->show_gui = SDL_TRUE;
						g->status = REDRAW;
						try_move(SELECTION);
						*/
					}
				}
				nk_label(ctx, lv->a[i].size_str, NK_TEXT_RIGHT);
				nk_label(ctx, lv->a[i].mod_str, NK_TEXT_RIGHT);
			}
			list_height = ctx->current->layout->clip.h; // ->bounds.h?
			full_rows = list_height / row_height;
			nk_list_view_end(&lview);
		}
		if (g->list_setscroll) {
			if (g->selection < lview.begin) {
				nk_group_set_scroll(ctx, "Image List", 0, g->selection*row_height);
			} else if (g->selection >= lview.begin + full_rows) {
				nk_group_set_scroll(ctx, "Image List", 0, (g->selection-full_rows+1)*row_height);
			}
			g->list_setscroll = FALSE;
		}
	}
}


void draw_playlists_menu(struct nk_context* ctx, int scr_w, int scr_h)
{
	int active;
	int is_selected;

	int edit_flags = NK_EDIT_FIELD | NK_EDIT_SIG_ENTER | NK_EDIT_AUTO_SELECT | NK_EDIT_ALWAYS_INSERT_MODE;

	static char buf[STRBUF_SZ];
	static int buf_len = 0;
	static int focus_flag;

	char tmp_buf[STRBUF_SZ];
	int loc;

	// Ugly hack to work around Nuklear issues
	// Can't get it to automatically have focus *and* auto select I have
	// to pick one or the other, so I immediately push another click when
	// they click on a selected playlist to trigger focus the "natural" way
	SDL_Event click_event = { .type = SDL_MOUSEBUTTONDOWN };
	click_event.button.button = SDL_BUTTON_LEFT;
	int x, y;
	SDL_GetMouseState(&x, &y);
	click_event.button.x = x;
	click_event.button.y = x;

	int footer_size = g->font_size+20+4;

	if (!g->is_new_renaming) {
		focus_flag = 0;
	}

	nk_layout_row_dynamic(ctx, scr_h-footer_size, 1);
	if (nk_group_begin(ctx, "Lib Playlist List", NK_WINDOW_SCROLL_AUTO_HIDE)) {
		nk_layout_row_dynamic(ctx, 0, 1);

		for (int i=0; i<g->playlists.size; ++i) {
			is_selected = g->selected_plist == i;

			// TODO reorganize this code
			if (g->is_new_renaming && is_selected) {

				if (!focus_flag) {
					// nk_edit_focus(ctx, edit_flags);
					SDL_Log("Pushing click\n");
					SDL_PushEvent(&click_event);
				}

				active = nk_edit_string(ctx, edit_flags, buf, &buf_len, STRBUF_SZ, nk_filter_default);
				if ((active & NK_EDIT_ACTIVATED)) {
					focus_flag = 1;
				}

				buf[buf_len] = 0;
				if (active & NK_EDIT_COMMITED && buf_len) {
					// TODO function? better way to structure this
					loc = cvec_contains_str(&g->playlists, buf);
					if (loc < 0 || (loc == g->playlists.size-1 && g->is_new_renaming == NEW_PLIST)) {
						if (g->is_new_renaming == NEW_PLIST) {
							if (create_playlist(buf)) {
								SDL_Log("Created playlist %s\n", buf);
								// we already pushed a placeholder at the end
								//cvec_push_str(&g->playlists, buf);
								cvec_replace_str(&g->playlists, g->playlists.size-1, buf, NULL);
								cvec_push_i(&g->playlist_ids, get_playlist_id(buf));
								buf[0] = 0;
								buf_len = 0;
								g->is_new_renaming = 0;
								// TODO "load" new playlist?
								// or just switch to current
								g->selected_plist = -1;
								g->lib_selected = nk_false;
								g->cur_selected = nk_true;
							} else {
								buf_len = snprintf(buf, STRBUF_SZ, "Failed to add playlist");
							}
						} else if (rename_playlist(buf, g->playlists.a[g->selected_plist])) {
							// TODO better way to do this
							char* tmp_str;
							cvec_replacem_str(g->playlists, g->selected_plist, CVEC_STRDUP(buf), tmp_str);
							//cvec_replace_str(&g->playlists, selected, buf, tmp_buf);
							if (!strcmp(tmp_str, g->cur_playlist)) {
								g->cur_playlist = g->playlists.a[g->selected_plist];
							}
							if (!strcmp(tmp_str, g->default_playlist)) {
								free(g->default_playlist);
								g->default_playlist = CVEC_STRDUP(g->playlists.a[g->selected_plist]);
							}
							free(tmp_str);
							buf[0] = 0;
							buf_len = 0;
							g->is_new_renaming = 0;
						} else {
							buf_len = snprintf(buf, STRBUF_SZ, "Failed to rename playlist");
						}
					} else {
						buf_len = snprintf(tmp_buf, STRBUF_SZ, "'%s' already exists!", buf);
						strcpy(buf, tmp_buf);
					}
				}
			} else if (nk_selectable_label(ctx, g->playlists.a[i], NK_TEXT_CENTERED, &is_selected)) {
				if (is_selected) {
					g->selected_plist = i;
					g->lib_selected = nk_false;
					g->cur_selected = nk_false;
					g->is_new_renaming = 0;
					cvec_clear_file(&g->lib_mode_list);
					load_sql_playlist_id(g->playlist_ids.a[i], &g->lib_mode_list);
					g->list_view = &g->lib_mode_list;
				} else {
					g->is_new_renaming = RENAMING_PLIST;
					// we know all playlists are less than STRBUF_SZ
					buf_len = strlen(g->playlists.a[i]);
					memcpy(buf, g->playlists.a[i], buf_len+1);
				}
			}
		}
		nk_group_end(ctx);
	}

	// TODO use awesome font icons
	nk_layout_row_dynamic(ctx, 0, 2);
	if (nk_button_label(ctx, "+")) {
		cvec_push_str(&g->playlists, "New Playlist");
		g->is_new_renaming = NEW_PLIST;
		g->selected_plist = g->playlists.size-1;
		g->cur_selected = nk_false;
		g->lib_selected = nk_false;
		// we know all playlists are less than STRBUF_SZ
		buf_len = strlen(g->playlists.a[g->playlists.size-1]);
		memcpy(buf, g->playlists.a[g->playlists.size-1], buf_len+1);

	}
	if (g->selected_plist < 0) {
		nk_widget_disable_begin(ctx);
	}
	if (nk_button_label(ctx, "-")) {
		delete_playlist(g->playlists.a[g->selected_plist]);
		SDL_Log("Deleted playlist %s\n", g->playlists.a[g->selected_plist]);
		cvec_erase_str(&g->playlists, g->selected_plist, g->selected_plist);
		cvec_erase_i(&g->playlist_ids, g->selected_plist, g->selected_plist);
		g->selected_plist = -1;
	}
	nk_widget_disable_end(ctx);
}
