

enum { RENAMING_PLIST = 1, NEW_PLIST };

void draw_file_list(struct nk_context* ctx, int scr_w, int scr_h);
void draw_playlists_menu(struct nk_context* ctx, int scr_w, int scr_h);

void handle_selection_removal(void);

// Meant for switching between playlists/current/library
void clear_search_and_preview(void)
{
	// Hmmm
	if (g->state & SEARCH_RESULTS) {
		g->selection = (g->list_view->size) ? 0 : -1;
		if (g->preview.tex) {
			SDL_DestroyTexture(g->preview.tex);
			g->preview.tex = NULL;
		}
		g->state &= ~SEARCH_RESULTS;
		g->list_search_active = SDL_FALSE;
		text_buf[0] = 0;
		text_len = 0;
		g->search_results.size = 0;
	}
}


void draw_library(struct nk_context* ctx, int scr_w, int scr_h)
{
	int horizontal_rule_ht = 2;

	struct nk_rect bounds;
	//const struct nk_input* in = &ctx->input;

	//static nk_bool show_search = nk_false;

	// TODO better name
	// TODO could have a draggable splitter here too?
	const float group_szs[] = { g->gui_sidebar_w, scr_w-g->gui_sidebar_w-8 };

	if (nk_begin(ctx, "Library", nk_rect(0, 0, scr_w, scr_h), NK_WINDOW_NO_SCROLLBAR)) {

		// TODO -4? or 8?
		//int top_ws[2] = { g->gui_menu_w, scr_w - g->gui_menu_w-8 };
		//nk_layout_row(ctx, NK_STATIC, 0, 2, top_ws);

		// TODO what to put in space to the right?
		nk_layout_row_static(ctx, 0, g->gui_menu_w, 1);
		draw_menu(ctx);


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

					// hmm partially redundant with below if was in search
					clear_search_and_preview();

					// TODO should I try to recover img[0].index?
					g->selection = (g->list_view->size) ? 0 : -1;
					if (g->preview.tex) {
						SDL_DestroyTexture(g->preview.tex);
						g->preview.tex = NULL;
					}
					get_img_playlists(g->selection);
				} else {
					// allow rename?
					// keep it highlighted
					g->cur_selected = nk_true;
					// could also interpret this as go to normal mode
					// or open new for anything other than current
				}
			}

			// Could just have the Library be a regular playlist with everything in it...
			// not alow renaming?
			if (nk_selectable_label(ctx, "Library", NK_TEXT_CENTERED, &g->lib_selected)) {
				if (g->lib_selected) {
					g->selected_plist = -1;
					g->cur_selected = nk_false;
					load_library(&g->lib_mode_list, &g->bad_img_ids, &g->bad_img_paths);
					if (g->bad_img_ids.size && g->bad_imgs_behavior == ALWAYS_ASK) {
						g->state |= BAD_IMGS;
					}
					g->list_view = &g->lib_mode_list;
					do_lib_sort(filename_cmp_lt);
					clear_search_and_preview();

					g->selection = (g->list_view->size) ? 0 : -1;
					if (g->preview.tex) {
						SDL_DestroyTexture(g->preview.tex);
						g->preview.tex = NULL;
					}
					get_img_playlists(g->selection);
				} else {
					// allow renaming?
					// keep selected
					g->lib_selected = nk_true;
				}
				clear_search_and_preview();
			}

			nk_label(ctx, "Playlists:", NK_TEXT_LEFT);
			nk_layout_row_dynamic(ctx, horizontal_rule_ht, 1);
			nk_rule_horizontal(ctx, g->color_table[NK_COLOR_TEXT], nk_true);

			nk_layout_row_dynamic(ctx, 0, 2);
			nk_label(ctx, "Active:", NK_TEXT_LEFT);
			nk_label(ctx, g->cur_playlist, NK_TEXT_LEFT);

			nk_label(ctx, "Default:", NK_TEXT_LEFT);
			nk_label(ctx, g->default_playlist, NK_TEXT_LEFT);

			// Make a config/preference
			int preview_enabled = 1;

			bounds = nk_widget_bounds(ctx);
			int height = scr_h - bounds.y - preview_enabled * (g->gui_sidebar_w + 4);

			draw_playlists_menu(ctx, group_szs[0], height);


			int sel = g->selection;

			//SDL_Log("sel = %d\n", sel);
			if (preview_enabled && sel >= 0) {
				SDL_Texture* preview = g->preview.tex;

				//SDL_Log("preview = %p\n", preview);

				if (!preview) {
					if (IS_RESULTS()) {
						sel = g->search_results.a[sel];
					}

					if (g->cur_selected) {
						if (!g->thumbs.a) {
							allocate_thumbs();
						}
						if (!g->thumbs.a[sel].tex) {
							preview = gen_and_load_thumb(&g->thumbs.a[sel], &g->files.a[sel]);
						} else {
							preview = g->thumbs.a[sel].tex;
						}
						g->preview.w = g->thumbs.a[sel].w;
						g->preview.h = g->thumbs.a[sel].h;
					} else {
						//puts("not cur or not thumbs.a");
						preview = gen_and_load_thumb(&g->preview, &g->list_view->a[sel]);
					}
				}

				// TODO get FrostKiwi pull request
				if (preview) {
					// stretch to fill space
					//nk_layout_row_dynamic(ctx, g->gui_sidebar_w, 1);
					//8.00 579.00 172.00 180
					//nk_image(ctx, nk_image_ptr(preview));

					bounds = nk_widget_bounds(ctx);

					nk_layout_space_begin(ctx, NK_STATIC, g->gui_sidebar_w, 1);
					struct nk_rect r = { bounds.x, bounds.y, g->gui_sidebar_w-8, g->gui_sidebar_w };
					float w_over_thm_sz = r.w/(float)THUMBSIZE;
					float h_over_thm_sz = r.h/(float)THUMBSIZE;
					float w = g->preview.w * w_over_thm_sz;
					float h = g->preview.h * h_over_thm_sz;
					float x = (r.w-w)*0.5f;
					float y = (r.h-h)*0.5f;

					nk_layout_space_push(ctx, nk_rect(x, y, w, h));
					nk_image(ctx, nk_image_ptr(preview));
					nk_layout_space_end(ctx);

					/*
					if (thumb.tex) {
						SDL_DestroyTexture(thumb.tex);
					}
					*/
				}
			}

			nk_group_end(ctx);
		}

		if (nk_group_begin(ctx, "Library Main", NK_WINDOW_NO_SCROLLBAR)) {


			nk_layout_row_dynamic(ctx, 0, 1);

			//draw_file_list(ctx, scr_w, scr_h);
			draw_file_list(ctx, group_szs[1], scr_h);

			nk_group_end(ctx);
		}

	}
	nk_end(ctx);


}

void draw_file_list(struct nk_context* ctx, int scr_w, int scr_h)
{
	int search_flags = NK_EDIT_FIELD | NK_EDIT_SIG_ENTER | NK_EDIT_GOTO_END_ON_ACTIVATE;
	static const char invalid_img[] = "Could Not Load";
	static struct nk_list_view lview, rview;
	static float header_ratios[] = {0.49f, 0.01f, 0.15f, 0.01f, 0.34f };
	static int splitter_down = 0;

	// 2*minrowpadding which is 8 + win.spacing.y which is 4 = 20
	int row_height = g->font_size + 20;
	// set to number of *fully visible* rows in the list_view
	// ie clip.h or bounds.h / row_height
	int full_rows = 0;
	int footer_size = g->font_size+20+4;

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

	// If I guard this, don't always show it it might be better
	int active = nk_edit_string(ctx, search_flags, text_buf, &text_len, STRBUF_SZ, nk_filter_default);
	if (active & NK_EDIT_COMMITED && text_len) {

		search_filenames(SDL_FALSE);
		memset(&rview, 0, sizeof(rview));
		g->state |= SEARCH_RESULTS;

		// use no selection to ignore the "Enter" in events so we don't exit
		// lib mode.  Could add state to handle keeping the selection but meh
		g->selection = -1;  // no selection among search
		g->list_search_active = SDL_FALSE;
		// do I need this?
		nk_edit_unfocus(ctx);
	}
	g->list_search_active = active & NK_EDIT_ACTIVE;


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
	nk_layout_row_dynamic(ctx, scr_h-bounds.y-footer_size, 1);
	//nk_layout_row_dynamic(ctx, scr_h-2*(g->font_size + 20)-8, 1);
	
	// Not sure we even need this except for g->files
	if (g->state & SEARCH_RESULTS) {
		if (!g->search_results.size) {
			if (nk_button_label(ctx, "No matching results")) {
				g->state = LIB_DFLT;
				text_buf[0] = 0;
				text_len = 0;
				g->selection = -1;
			}
		} else {
			// nk_list_view_begin
			if (nk_list_view_begin(ctx, &rview, "Result List", NK_WINDOW_BORDER, g->font_size+16, g->search_results.size)) {
				nk_layout_row(ctx, NK_DYNAMIC, 0, 3, ratios);
				int i;
				for (int j=rview.begin; j<rview.end; ++j) {
					i = g->search_results.a[j];
					// TODO Do I really need g->selection?  Can I use g->img[0].index (till I get multiple selection)
					// also thumb_sel serves the same/similar purpose
					is_selected = g->selection == j;
					name = lv->a[i].name;
					if (!name) name = (char*)invalid_img;
					if (nk_selectable_label(ctx, name, NK_TEXT_LEFT, &is_selected)) {
						if (is_selected) {
							g->selection = j;
							if (g->preview.tex) {
								SDL_DestroyTexture(g->preview.tex);
								g->preview.tex = NULL;
							}
							// TODO think about making it take the real index
							get_img_playlists(j);
						} else {
							// could support unselecting, esp. with CTRL somehow if I ever allow
							// multiple selection
							// g->selection = -1;

							// for now, treat clicking a selection as a "double click" ie same as return
							//int tmp = g->search_results.a[g->selection];
							//g->selection = (tmp) ? tmp - 1 : lv->size-1;
							if (is_current) {
								g->state |= NORMAL;
								SDL_ShowCursor(SDL_ENABLE);
								g->gui_timer = SDL_GetTicks();
								g->show_gui = SDL_TRUE;
								g->status = REDRAW;
								try_move(SELECTION);
							}
						}

						if (g->is_new_renaming) {
							g->is_new_renaming = -2;
						}
					}
					nk_label(ctx, lv->a[i].size_str, NK_TEXT_RIGHT);
					nk_label(ctx, lv->a[i].mod_str, NK_TEXT_RIGHT);
				}
				list_height = ctx->current->layout->clip.h; // ->bounds.h?
				full_rows = list_height / row_height;
				nk_list_view_end(&rview);
			}
			if (g->list_setscroll) {
				if (g->selection < rview.begin) {
					nk_group_set_scroll(ctx, "Result List", 0, g->selection*row_height);
				} else if (g->selection >= rview.begin + full_rows) {
					nk_group_set_scroll(ctx, "Result List", 0, (g->selection-full_rows+1)*row_height);
				}
				g->list_setscroll = FALSE;
			}
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
						if (g->preview.tex) {
							SDL_DestroyTexture(g->preview.tex);
							g->preview.tex = NULL;
						}
						get_img_playlists(i);
					} else {
						// could support unselecting, esp. with CTRL somehow if I ever allow
						// multiple selection
						// g->selection = -1;

						// for now, treat clicking a selection as a "double click" ie same as return
						// TODO only for files?  worth it?
						if (is_current) {
							g->state = NORMAL;
							SDL_ShowCursor(SDL_ENABLE);
							g->gui_timer = SDL_GetTicks();
							g->show_gui = SDL_TRUE;
							g->status = REDRAW;
							try_move(SELECTION);
						}
					}
					if (g->is_new_renaming) {
						g->is_new_renaming = -2;
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

	static char footer_buf[STRBUF_SZ];

	i64 num_imgs = lv->size;
	int is_search = nk_false;
	if (g->state & SEARCH_RESULTS) {
		num_imgs = g->search_results.size;
		is_search = nk_true;
	}

	int idx = g->selection+1;
	if (idx) {
		snprintf(footer_buf, STRBUF_SZ, "%d/%"PRIcv_sz " Images", idx, num_imgs);
	} else {
		snprintf(footer_buf, STRBUF_SZ, "%"PRIcv_sz " Images", num_imgs);
	}
	/*
	if (ret >= STRBUF_SZ) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "path too long\n");
		cleanup(0, 1);
	}
	*/

	// TODO better handle different cases (no images, in the middle of new_renaming, etc.)
	int cols = 1;
	int plist_i = g->selected_plist;
	char* selected_pl = NULL;
	if (plist_i >= 0) {
		if (num_imgs) {
			cols = 5;
		} else {
			cols = 3;
		}
		selected_pl = g->playlists.a[plist_i];
	} else if (!is_current && num_imgs) {
		cols = 3;
	}
	nk_layout_row_dynamic(ctx, 0, cols);

	if (plist_i >= 0) {
		if (nk_button_label(ctx, "Make Active")) {
			if (strcmp(selected_pl, g->cur_playlist)) {
				strncpy(g->cur_playlist_buf, selected_pl, STRBUF_SZ);
				g->cur_playlist_id = g->playlist_ids.a[plist_i];
				update_save_status();
			}
		}
		if (nk_button_label(ctx, "Make Default")) {
			if (strcmp(selected_pl, g->default_playlist)) {
				free(g->default_playlist);
				g->default_playlist = CVEC_STRDUP(selected_pl);
			}
		}
	}

	if (!is_current && num_imgs) {
		// Should Either of these open file browser?  Or just the current
		// view?  maybe only if it's empty?
		// could also get rid of open new and just let double clicking on
		// an image mean that
		file f;
		char* sep;
		if (nk_button_label(ctx, "Open New")) {
			stop_generating();

			g->thumbs_done = SDL_FALSE;
			g->thumbs_loaded = SDL_FALSE;
			cvec_free_thumb_state(&g->thumbs);
			cvec_clear_file(&g->files);

			char* saved_path = NULL;

			// TODO should we stay in lib mode or jump to normal?
			// let's at least jump to current to show we actualy did something
			if (!is_search) {

				if (g->selection >= 0) {
					saved_path = lv->a[g->selection].path;
				}
				// Method 1
				//cvector_file tmp;
				//// TODO Could add file copy constructor and use cvec_copy_file() here
				//// and use pushm everywhere else (like myscandir)
				//// instead of reloading the lib/playlist
				//memcpy(&tmp, &g->files, sizeof(cvector_file));
				//memcpy(&g->files, lv, sizeof(cvector_file));
				//memcpy(lv, &tmp, sizeof(cvector_file));

				// Method 2
				if (g->lib_selected) {
					load_library(&g->files, NULL, NULL);
				} else {
					load_sql_playlist_id(g->playlist_ids.a[g->selected_plist], &g->files);
				}

				// In case of Method 1, we don't really care if we leave lib_mode_list empty
				// but if we did...
				//if (g->lib_selected) {
				//	load_library(&g->files, NULL, NULL);
				//} else {
				//	load_sql_playlist_id(g->playlist_ids.a[g->selected_plist], &g->lib_mode_list);
				//}
			} else {
				if (g->selection >= 0) {
					saved_path = lv->a[g->search_results.a[g->selection]].path;
				}
				for (int i=0; i<g->search_results.size; i++) {
					// need a separate copy of string so we don't double free
					f = lv->a[g->search_results.a[i]];
					f.path = strdup(f.path);
					sep = strrchr(f.path, PATH_SEPARATOR);
					f.name = (sep) ? sep+1 : f.path;
					cvec_push_file(&g->files, &f);
				}
				// have to clear the search state if we jump to current or normal mode
				g->state = LIB_DFLT;
				text_buf[0] = 0;
				text_len = 0;
				g->search_results.size = 0;
			}

			remove_duplicates();
			mirrored_qsort(g->files.a, g->files.size, sizeof(file), filename_cmp_lt, 0);
			g->sorted_state = NAME_UP;
			update_save_status();
			g->save_status_uptodate = SDL_TRUE;

			g->selected_plist = -1;
			g->lib_selected = nk_false;
			g->cur_selected = nk_true;
			g->list_view = &g->files;

			if (saved_path) {
				g->selection = find_file_simple(saved_path);
			} else {
				g->selection = 0;
			}
			g->list_setscroll = TRUE;
			try_move(SELECTION);
		}

		if (nk_button_label(ctx, "Open More")) {
			stop_generating();
			g->thumbs_done = SDL_FALSE;
			g->thumbs_loaded = SDL_FALSE;
			cvec_free_thumb_state(&g->thumbs);

			if (!is_search) {
				for (int i=0; i<lv->size; i++) {
					// need a separate copy of string so we don't double free
					f = lv->a[i];
					f.path = strdup(f.path);
					sep = strrchr(f.path, PATH_SEPARATOR);
					f.name = (sep) ? sep+1 : f.path;
					cvec_push_file(&g->files, &f);
				}
			} else {
				for (int i=0; i<g->search_results.size; i++) {
					// need a separate copy of string so we don't double free
					f = lv->a[g->search_results.a[i]];
					f.path = strdup(f.path);
					sep = strrchr(f.path, PATH_SEPARATOR);
					f.name = (sep) ? sep+1 : f.path;
					cvec_push_file(&g->files, &f);
				}
				// have to clear the search state if we jump to current or normal mode
				g->state = LIB_DFLT;
				text_buf[0] = 0;
				text_len = 0;
				g->search_results.size = 0;
			}

			remove_duplicates();

			char* path = g->files.a[g->selection].path;
			mirrored_qsort(g->files.a, g->files.size, sizeof(file), filename_cmp_lt, 0);
			g->sorted_state = NAME_UP;

			int idx = find_file_simple(path);
			g->img[0].index = idx;
			g->selection = idx;
			if (g->preview.tex) {
				SDL_DestroyTexture(g->preview.tex);
				g->preview.tex = NULL;
			}
			g->list_setscroll = TRUE;

			g->selected_plist = -1;
			g->lib_selected = nk_false;
			g->cur_selected = nk_true;
			g->list_view = &g->files;
			update_save_status();
			g->save_status_uptodate = SDL_TRUE;
		}
	}

	nk_label(ctx, footer_buf, NK_TEXT_RIGHT);
}


void draw_playlists_menu(struct nk_context* ctx, int scr_w, int scr_h)
{
	int active;
	int is_selected;

	// TODO maybe I should just call it NK_EDIT_FOCUS?
	int edit_flags = NK_EDIT_FIELD | NK_EDIT_SIG_ENTER | NK_EDIT_AUTO_SELECT | NK_EDIT_IMMEDIATE_FOCUS;
	//int edit_flags = NK_EDIT_FIELD | NK_EDIT_SIG_ENTER | NK_EDIT_AUTO_SELECT;
	//int edit_flags = NK_EDIT_FIELD | NK_EDIT_SIG_ENTER | NK_EDIT_GOTO_END_ON_ACTIVATE;

	static char buf[STRBUF_SZ];
	static int buf_len = 0;

	static char tmp_buf[STRBUF_SZ];
	int loc;

	// Ugly hack to work around Nuklear issues
	// Can't get it to automatically have focus *and* auto select I have
	// to pick one or the other, so I immediately push another click when
	// they click on a selected playlist to trigger focus the "natural" way
	SDL_Event click_event = { .type = SDL_MOUSEBUTTONDOWN };
	click_event.button.button = SDL_BUTTON_LEFT;
	int x, y;
	SDL_GetMouseState(&x, &y);

	SDL_Event event = { .type = g->userevent };

	int footer_size = g->font_size+20+4;

	float ratios[] = { 0.8, 0.2 };

	if (g->is_new_renaming < 0) {
		buf[0] = 0;
		buf_len = 0;
		if (g->is_new_renaming < -1) {
			nk_edit_unfocus(ctx);
			g->is_new_renaming++;
		}
	}


	nk_layout_row_dynamic(ctx, scr_h-footer_size, 1);
	if (nk_group_begin(ctx, "Lib Playlist List", NK_WINDOW_SCROLL_AUTO_HIDE)) {

		if (g->selection < 0) {
			nk_layout_row_dynamic(ctx, 0, 1);
		} else {
			nk_layout_row(ctx, NK_DYNAMIC, 0, 2, ratios);
		}

		for (int i=0; i<g->playlists.size; ++i) {
			is_selected = g->selected_plist == i;

			// TODO reorganize this code
			if (g->is_new_renaming == RENAMING_PLIST && is_selected) {

				active = nk_edit_string(ctx, edit_flags, buf, &buf_len, STRBUF_SZ, nk_filter_default);
				buf[buf_len] = 0;
				if (active & NK_EDIT_COMMITED && buf_len) {
					// TODO function? better way to structure this
					loc = cvec_contains_str(&g->playlists, buf);
					if (loc < 0) {
						if (rename_playlist(buf, g->playlists.a[g->selected_plist])) {
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
							g->is_new_renaming = -1;
							nk_edit_unfocus(ctx);
						} else {
							buf_len = snprintf(buf, STRBUF_SZ, "Failed to rename playlist");
						}
					} else if (loc == i) {
						// Allow enter on the same name when renaming, same as ESC
						buf[0] = 0;
						buf_len = 0;
						g->is_new_renaming = -1;
						nk_edit_unfocus(ctx);
					} else {
						buf_len = snprintf(tmp_buf, STRBUF_SZ, "'%s' already exists!", buf);
						strcpy(buf, tmp_buf);
						// select all
						ctx->current->edit.sel_start = 0;
						ctx->current->edit.sel_end = buf_len;
						// not sure this is really necessary
						ctx->current->edit.cursor = 0;
					}
				}
			} else if (nk_selectable_label(ctx, g->playlists.a[i], NK_TEXT_LEFT, &is_selected)) {
				if (is_selected) {
					g->selected_plist = i;
					g->lib_selected = nk_false;
					g->cur_selected = nk_false;
					g->is_new_renaming = 0;
					cvec_clear_file(&g->lib_mode_list);

					load_sql_playlist_id(g->playlist_ids.a[i], &g->lib_mode_list);
					g->list_view = &g->lib_mode_list;

					//do_lib_sort(filename_cmp_lt);
					//g->lib_sorted_state = NAME_UP;
					// called after above three, partially redundant with do_lib_sort
					clear_search_and_preview();

					g->selection = (g->list_view->size) ? 0 : -1;
					if (g->preview.tex) {
						SDL_DestroyTexture(g->preview.tex);
						g->preview.tex = NULL;
					}

					g->lib_sorted_state = NONE;
					event.user.code = SORT_NAME;
					event.user.data1 = (void*)nk_true;
					SDL_PushEvent(&event);

					//mirrored_qsort(g->list_view->a, g->list_view->size, sizeof(file), filename_cmp_lt, 0);
					//g->lib_sorted_state = NAME_UP;
				} else {
					g->is_new_renaming = RENAMING_PLIST;
					// we know all playlists are less than STRBUF_SZ
					buf_len = strlen(g->playlists.a[i]);
					memcpy(buf, g->playlists.a[i], buf_len+1);
				}
			}

			int idx = g->selection;
			if (idx >= 0) {
				// We have the selectable on the left, don't need the name here.
				// TODO there really should be a nk_checkbox() with no label at all
				//if (nk_checkbox_label(ctx, g->playlists.a[i], &g->img_saved_status[i])) {
				if (nk_checkbox_label(ctx, "", &g->img_saved_status[i])) {
					int ret = do_sql_save_idx(!g->img_saved_status[i], g->playlist_ids.a[i], idx);
					// if we are on the playlist we just removed it from we need to
					// remove it from the list as well
					if (is_selected && ret && !g->img_saved_status[i]) {
						handle_selection_removal();
					}
				}
			}
		}

		if (g->is_new_renaming == NEW_PLIST) {
			active = nk_edit_string(ctx, edit_flags, buf, &buf_len, STRBUF_SZ, nk_filter_default);
			buf[buf_len] = 0;
			if (active & NK_EDIT_COMMITED && buf_len) {
				// TODO function? better way to structure this
				loc = cvec_contains_str(&g->playlists, buf);
				if (loc < 0) {
					if (create_playlist(buf)) {
						SDL_Log("Created playlist %s\n", buf);
						cvec_push_str(&g->playlists, buf);
						cvec_push_i(&g->playlist_ids, get_playlist_id(buf));
						buf[0] = 0;
						buf_len = 0;
						g->is_new_renaming = -1;
						nk_edit_unfocus(ctx);

						// "load" new playlist?
						/*
						load_sql_playlist_id(g->playlist_ids.a[i], &g->lib_mode_list);
						g->selection = -1;
						if (g->preview.tex) {
							SDL_DestroyTexture(g->preview.tex);
							g->preview.tex = NULL;
						}
						*/

						//g->selected_plist = -1;
						//
						// or just switch to current..which combined
						// with the "Enter" the user just hit will exit lib mode
						// hmmm
						//g->lib_selected = nk_false;
						//g->cur_selected = nk_true;
						//g->list_view = &g->files;
					} else {
						buf_len = snprintf(buf, STRBUF_SZ, "Failed to add playlist");
					}
				} else {
					buf_len = snprintf(tmp_buf, STRBUF_SZ, "'%s' already exists!", buf);
					strcpy(buf, tmp_buf);
					// select all
					ctx->current->edit.sel_start = 0;
					ctx->current->edit.sel_end = buf_len;
					// not sure this is really necessary
					ctx->current->edit.cursor = 0;
				}
			}
			// scroll to end to make/keep field visible
			nk_group_set_scroll(ctx, "Lib Playlist List", 0, UINT_MAX);
		}

		nk_group_end(ctx);
	}

	// TODO move add new edit_string outside of group to here so it will never be hidden
	// at the bottom of the scrolling playlist list

	// TODO use awesome font icons
	nk_layout_row_dynamic(ctx, 0, 2);
	if (nk_button_label(ctx, "+")) {
		g->is_new_renaming = NEW_PLIST;

		//cvec_push_str(&g->playlists, "New Playlist");

		// keep selection the same until done
		//g->selected_plist = g->playlists.size-1;
		//g->cur_selected = nk_false;
		//g->lib_selected = nk_false;
		//g->list_view = &g->lib_mode_list;
		// clear saved status
		//memset(&g->img_saved_status, 0, g->playlist_ids.size*sizeof(int));

		const char new_plist_str[] = "New Playlist";
		buf_len = sizeof(new_plist_str)-1;
		memcpy(buf, new_plist_str, sizeof(new_plist_str));
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
		g->cur_selected = nk_true;
		g->list_view = &g->files;

		// TODO hmm partially redundant with below if was in search
		clear_search_and_preview();

		// TODO should I try to recover img[0].index?
		g->selection = (g->list_view->size) ? 0 : -1;
		if (g->preview.tex) {
			SDL_DestroyTexture(g->preview.tex);
			g->preview.tex = NULL;
		}
		// update for new selection (if >= 0) checked inside
		get_img_playlists(g->selection);
	}
	nk_widget_disable_end(ctx);
}

void handle_selection_removal(void)
{
	// TODO handle removal from current as well?  bool param?
	int idx = g->selection;
	if (IS_RESULTS()) {
		idx = g->search_results.a[idx];
		cvec_erase_i(&g->search_results, g->selection, g->selection);
		for (int j=g->selection; j<g->search_results.size; ++j) {
			g->search_results.a[j]--;
		}
		cvec_erase_file(g->list_view, idx, idx);

		if (g->selection == g->search_results.size) {
			g->selection = g->search_results.size-1;
		}
	} else {
		cvec_erase_file(g->list_view, idx, idx);
		if (g->selection == g->list_view->size) {
			g->selection = g->list_view->size-1;
		}
	}

	// update for new selection (if >= 0) checked inside
	get_img_playlists(g->selection);
	if (g->preview.tex) {
		SDL_DestroyTexture(g->preview.tex);
		g->preview.tex = NULL;
	}
}

void draw_bad_lib_imgs_popup(struct nk_context* ctx, int scr_w, int scr_h)
{
	struct nk_rect s = { 0, 0, scr_w, scr_h };
	int popup_flags = NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_BORDER|NK_WINDOW_TITLE;//NK_WINDOW_CLOSABLE;

	static struct nk_list_view lview;
	static nk_bool make_choice_pref;
	struct nk_rect bounds;

	char label_buf[STRBUF_SZ];

	int len = snprintf(label_buf, STRBUF_SZ, "There were %"PRIcv_sz" invalid paths found in your library. This could be because the images are on a drive that is not mounted, or they were moved or deleted.", g->bad_img_ids.size);

	// not really accurate especially for non-monospace fonts
	int rows = len*g->font_size / scr_w + 1;
	int label_height = rows * g->font_size; // plus text padding?

	// TODO tabs like terminal About, License, Credits?
	if (nk_begin(ctx, "Bad Images in Library", s, popup_flags))
	{
		// TODO what's the point of wrapping if you have to calculate the height
		// needed for it to be seen yourself?
		nk_layout_row_dynamic(ctx, label_height, 1);
		nk_label_wrap(ctx, label_buf);

		nk_checkbox_label(ctx, "Perform your current decision automatically next time", &make_choice_pref);

		// Could put these buttons at the bottom?
		nk_layout_row_dynamic(ctx, 0, 3);
		if (nk_button_label(ctx, "Remove All")) {
			remove_bad_imgs(&g->bad_img_ids);
			cvec_clear_i(&g->bad_img_ids);
			cvec_clear_str(&g->bad_img_paths);
			g->state &= ~BAD_IMGS;
			if (make_choice_pref) {
				g->bad_imgs_behavior = REMOVE_IMGS;
			}
		}
		if (nk_button_label(ctx, "Ignore")) {
			g->state &= ~BAD_IMGS;
			if (make_choice_pref) {
				g->bad_imgs_behavior = IGNORE_IMGS;
			}
		}

		if (nk_button_label(ctx, "Try reloading library")) {
			load_library(g->list_view, &g->bad_img_ids, &g->bad_img_paths);
			// immediately close it?
			if (!g->bad_img_ids.size) {
				g->state &= ~BAD_IMGS;
			}
		}

		bounds = nk_widget_bounds(ctx);
		nk_layout_row_dynamic(ctx, scr_h-bounds.y-4, 1);
		if (nk_list_view_begin(ctx, &lview, "Bad Image List", NK_WINDOW_BORDER, g->font_size+16, g->bad_img_ids.size)) {
			nk_layout_row_dynamic(ctx, 0, 1);
			for (int i=lview.begin; i<lview.end; i++) {
				nk_label(ctx, g->bad_img_paths.a[i], NK_TEXT_LEFT);
			}
			nk_list_view_end(&lview);
		}

	}
	nk_end(ctx);

}
