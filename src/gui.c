
void cleanup(int ret, int called_setup);
void set_rect_bestfit(img_state* img, int fill_screen);
void set_fullscreen();
int try_move(int direction);

// TODO better name? cleardir?
int empty_dir(const char* dirpath);

enum { MENU_NONE, MENU_MISC, MENU_SORT, MENU_EDIT, MENU_VIEW };

void draw_prefs(struct nk_context* ctx, int scr_w, int scr_h);
void draw_infobar(struct nk_context* ctx, int scr_w, int scr_h);
void draw_thumb_infobar(struct nk_context* ctx, int scr_w, int scr_h);

#define GUI_BAR_HEIGHT 30

void search_filenames()
{
	// fast enough to do here?  I do it in events?
	text[text_len] = 0;
	SDL_Log("Final text = \"%s\"\n", text);

	// it'd be kind of cool to add results of multiple searches together if we leave this out
	// of course their might be duplicates.  Or we could make it search within the existing
	// search results, so consecutive searches are && together...
	g->search_results.size = 0;
	
	for (int i=0; i<g->files.size; ++i) {
		// GNU function...
		// searching name since I'm showing names not paths in the list
		if (strcasestr(g->files.a[i].name, text)) {
			SDL_Log("Adding %s\n", g->files.a[i].path);
			cvec_push_i(&g->search_results, i);
		}
	}
	SDL_Log("found %d matches\n", (int)g->search_results.size);
}

void draw_gui(struct nk_context* ctx)
{
	// closable gives the x, if you use it it won't come back (probably have to call show() or
	// something...
	int popup_flags = NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_BORDER|NK_WINDOW_TITLE;//NK_WINDOW_CLOSABLE;

	SDL_Event event = { .type = g->userevent };
	img_state* img;

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

		if (nk_begin(ctx, "Arbitrary Rotation", s, popup_flags)) {

			nk_layout_row_dynamic(ctx, 0, 1);

			nk_label_wrap(ctx, "Click and drag, type, or use the arrows to select the desired degree of rotation");

			tmp = nk_propertyi(ctx, "Degrees:", -180, img->rotdegs, 180, 1, 0.5);
			if (tmp != img->rotdegs) {
				img->edited = TO_ROTATE;
				img->rotdegs = tmp;
			}
			//nk_label(ctx, "Degrees:", NK_TEXT_LEFT);
			//nk_slider_int(ctx, -180, &slider_degs, 180, 1);

			//nk_button_set_behavior(ctx, NK_BUTTON_DEFAULT);
			nk_layout_row_dynamic(ctx, 0, 2);
			if (nk_button_label(ctx, "Preview")) {
				if (img->edited == TO_ROTATE) {
					event.user.code = ROT360;
					SDL_PushEvent(&event);
				}
			}
			if (nk_button_label(ctx, "Ok")) {
				if (img->edited == TO_ROTATE) {
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
		int w = 550, h = 285; ///scale_x, h = 400/scale_y;
		struct nk_rect s;
		s.x = scr_w/2-w/2;
		s.y = scr_h/2-h/2;
		s.w = w;
		s.h = h;

		if (nk_begin(ctx, "About sdl_img", s, popup_flags))
		{
			nk_layout_row_dynamic(ctx, 0, 1);
			nk_label(ctx, VERSION_STR, NK_TEXT_CENTERED);
			nk_label(ctx, "By Robert Winkler", NK_TEXT_LEFT);
			nk_label(ctx, "robertwinkler.com", NK_TEXT_LEFT);  //TODO project website
			nk_label(ctx, "sdl_img is licensed under the MIT License.",  NK_TEXT_LEFT);

			nk_label(ctx, "Credits:", NK_TEXT_CENTERED);

			//nk_layout_row_dynamic(ctx, 10, 2);
			float ratios[] = { 0.3f, 0.7f, 0.2f, 0.8f };
			nk_layout_row(ctx, NK_DYNAMIC, 10, 2, ratios);

			nk_label(ctx, "stb_image*", NK_TEXT_LEFT);
			nk_label(ctx, "github.com/nothings/stb", NK_TEXT_RIGHT);
			nk_label(ctx, "SDL2", NK_TEXT_LEFT);
			nk_label(ctx, "libsdl.org", NK_TEXT_RIGHT);
			nk_label(ctx, "SDL2_gfx", NK_TEXT_LEFT);
			nk_label(ctx, "ferzkopp.net", NK_TEXT_RIGHT);
			nk_label(ctx, "nuklear GUI", NK_TEXT_LEFT);
			nk_label(ctx, "github.com/vurtun/nuklear", NK_TEXT_RIGHT);
			nk_label(ctx, "libcurl", NK_TEXT_LEFT);
			nk_label(ctx, "curl.haxx.se/libcurl/", NK_TEXT_RIGHT);
			nk_label(ctx, "WjCryptLib_Md5", NK_TEXT_LEFT);
			nk_label(ctx, "github.com/WaterJuice/WjCryptLib", NK_TEXT_RIGHT);

			// My own cvector lib

			nk_layout_row_dynamic(ctx, 0, 1);
			if (nk_button_label(ctx, "Ok")) {
				g->show_about = 0;;
			}
		}
		nk_end(ctx);
	}

	if (g->show_prefs) {
		draw_prefs(ctx, scr_w, scr_h);
	}

	if (g->fullscreen && g->fullscreen_gui == NEVER) {
		return;
	}

	// don't show main GUI if a popup is up, don't want user to
	// be able to interact with it.  Could look up how to make them inactive
	// but meh, this is simpler
	if (g->show_about || g->show_prefs || g->show_rotate) {
		g->show_gui = 1;
		g->gui_timer = SDL_GetTicks();
		return;
	}

	if (IS_THUMB_MODE() && !IS_VIEW_RESULTS()) {
		// TODO always show infobar in thumb_mode regardless of preference?
		if (g->show_infobar) {
			draw_thumb_infobar(ctx, scr_w, scr_h);
		}
		return;
	}

	int is_selected = 0;
	int symbol;
	float search_ratio[] = { 0.25f, 0.75f };
	int list_height;
	int active;
	float search_height;

	static struct nk_list_view lview, rview;
	static float header_ratios[] = {0.49f, 0.01f, 0.15f, 0.01f, 0.34f };
	static int splitter_down = 0;

	int search_flags = NK_EDIT_FIELD | NK_EDIT_SIG_ENTER | NK_EDIT_GOTO_END_ON_ACTIVATE;

	if (!nk_input_is_mouse_down(in, NK_BUTTON_LEFT))
		splitter_down = 0;

	if (IS_LIST_MODE() && !IS_VIEW_RESULTS()) {
		if (nk_begin(ctx, "List", nk_rect(0, GUI_BAR_HEIGHT, scr_w, scr_h-GUI_BAR_HEIGHT), NK_WINDOW_NO_SCROLLBAR)) {

			// TODO With Enter to search, should I even have the Search button?  It's more of a label now... maybe put it on
			// the left?  and make it smaller?
			nk_layout_row(ctx, NK_DYNAMIC, 0, 2, search_ratio);
			search_height = nk_widget_bounds(ctx).h;
			nk_label(ctx, "Search Filenames:", NK_TEXT_LEFT);
			active = nk_edit_string(ctx, search_flags, text, &text_len, STRBUF_SZ, nk_filter_default);
			if (active & NK_EDIT_COMMITED) {

				search_filenames();
				memset(&rview, 0, sizeof(rview));
				g->state |= SEARCH_RESULTS;

				// use no selection to ignore the "Enter" in events so we don't exit
				// list mode.  Could add state to handle keeping the selection but meh
				g->selection = -1;  // no selection among search
			}
			nk_layout_row(ctx, NK_DYNAMIC, 0, 5, header_ratios);

			symbol = NK_SYMBOL_NONE; // 0
			if (g->sorted_state == NAME_UP)
				symbol = NK_SYMBOL_TRIANGLE_UP;
			else if (g->sorted_state == NAME_DOWN)
				symbol = NK_SYMBOL_TRIANGLE_DOWN;

			// TODO name or path?
			if (nk_button_symbol_label(ctx, symbol, "Name", NK_TEXT_LEFT)) {
				event.user.code = SORT_NAME;
				SDL_PushEvent(&event);
			}

			bounds = nk_widget_bounds(ctx);
			nk_spacing(ctx, 1);
			if ((splitter_down == 1 || (nk_input_is_mouse_hovering_rect(in, bounds) && !splitter_down)) &&
			    nk_input_is_mouse_down(in, NK_BUTTON_LEFT)) {
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

			// I hate redundant logic but the alternative is repeated gui code
			// TODO think of a better way
			symbol = NK_SYMBOL_NONE; // 0
			if (g->sorted_state == SIZE_UP)
				symbol = NK_SYMBOL_TRIANGLE_UP;
			else if (g->sorted_state == SIZE_DOWN)
				symbol = NK_SYMBOL_TRIANGLE_DOWN;

			if (nk_button_symbol_label(ctx, symbol, "Size", NK_TEXT_LEFT)) {
				event.user.code = SORT_SIZE;
				SDL_PushEvent(&event);
			}

			bounds = nk_widget_bounds(ctx);
			nk_spacing(ctx, 1);
			if ((splitter_down == 2 || (nk_input_is_mouse_hovering_rect(in, bounds) && !splitter_down)) &&
			    nk_input_is_mouse_down(in, NK_BUTTON_LEFT)) {
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

			symbol = NK_SYMBOL_NONE; // 0
			if (g->sorted_state == MODIFIED_UP)
				symbol = NK_SYMBOL_TRIANGLE_UP;
			else if (g->sorted_state == MODIFIED_DOWN)
				symbol = NK_SYMBOL_TRIANGLE_DOWN;

			if (nk_button_symbol_label(ctx, symbol, "Modified", NK_TEXT_LEFT)) {
				event.user.code = SORT_MODIFIED;
				SDL_PushEvent(&event);
			}

			float ratios[] = { header_ratios[0]+0.01f, header_ratios[2], header_ratios[4]+0.01f };
			
			nk_layout_row_dynamic(ctx, scr_h-GUI_BAR_HEIGHT-2*search_height, 1);

			if (g->state & SEARCH_RESULTS) {
				if (!g->search_results.size) {
					if (nk_button_label(ctx, "No matching results")) {
						g->state = LIST_DFLT;
						text[0] = 0;
						text_len = 0;
						g->selection = g->img[0].index;
						g->list_setscroll = SDL_TRUE;
						// redundant since we clear before doing the search atm
						g->search_results.size = 0;
					}
				} else {
					if (nk_list_view_begin(ctx, &rview, "Result List", NK_WINDOW_BORDER, 24, g->search_results.size)) {
						nk_layout_row(ctx, NK_DYNAMIC, 0, 3, ratios);
						for (int j=rview.begin, i=g->search_results.a[j]; j<rview.end; ++j, i=g->search_results.a[j]) {
							// Do I really need g->selection?  Can I use g->img[0].index (till I get multiple selection)
							// also thumb_sel serves the same/similar purpose
							is_selected = g->selection == j;
							if (nk_selectable_label(ctx, g->files.a[i].name, NK_TEXT_LEFT, &is_selected)) {
								if (is_selected) {
									g->selection = j;
								} else {
									// could support unselecting, esp. with CTRL somehow if I ever allow
									// multiple selection
									// g->selection = -1;

									// for now, treat clicking a selection as a "double click" ie same as return
									//int tmp = g->search_results.a[g->selection];
									//g->selection = (tmp) ? tmp - 1 : g->files.size-1;
									g->selection = (g->selection) ? g->selection - 1 : g->search_results.size-1;

									g->state |= VIEW_RESULTS;
									SDL_ShowCursor(SDL_ENABLE);
									g->gui_timer = SDL_GetTicks();
									g->show_gui = SDL_TRUE;
									g->status = REDRAW;
									try_move(SELECTION);
								}
							}
							nk_label(ctx, g->files.a[i].size_str, NK_TEXT_RIGHT);
							nk_label(ctx, g->files.a[i].mod_str, NK_TEXT_RIGHT);
						}
						nk_list_view_end(&rview);
					}
				}
			} else {
				// TODO figure out why border goes off the edge (ie we don't see the bottom border)
				if (nk_list_view_begin(ctx, &lview, "Image List", NK_WINDOW_BORDER, 24, g->files.size)) {
					// TODO ratio layout 0.5 0.2 0.3 ? give or take
					//nk_layout_row_dynamic(ctx, 0, 3);
					nk_layout_row(ctx, NK_DYNAMIC, 0, 3, ratios);
					for (int i=lview.begin; i<lview.end; ++i) {
						// Do I really need g->selection?  Can I use g->img[0].index (till I get multiple selection)
						// also thumb_sel serves the same/similar purpose
						is_selected = g->selection == i;
						if (nk_selectable_label(ctx, g->files.a[i].name, NK_TEXT_LEFT, &is_selected)) {
							if (is_selected) {
								g->selection = i;
							} else {
								// could support unselecting, esp. with CTRL somehow if I ever allow
								// multiple selection
								// g->selection = -1;

								// for now, treat clicking a selection as a "double click" ie same as return
								g->selection = (g->selection) ? g->selection - 1 : g->files.size-1;

								g->state = NORMAL;
								SDL_ShowCursor(SDL_ENABLE);
								g->gui_timer = SDL_GetTicks();
								g->show_gui = SDL_TRUE;
								g->status = REDRAW;
								try_move(SELECTION);
							}
						}
						nk_label(ctx, g->files.a[i].size_str, NK_TEXT_RIGHT);
						nk_label(ctx, g->files.a[i].mod_str, NK_TEXT_RIGHT);
					}
					list_height = ctx->current->layout->clip.h; // ->bounds.h?
					nk_list_view_end(&lview);
				}

				if (g->list_setscroll) {
					nk_uint x = 0, y;
					int scroll_limit = lview.total_height - list_height; // little off
					y = (g->selection/(float)(g->files.size-1) * scroll_limit) + 0.999f;
					//nk_group_get_scroll(ctx, "Image List", &x, &y);
					nk_group_set_scroll(ctx, "Image List", x, y);
					g->list_setscroll = SDL_FALSE;
				}
				//printf("scroll %u %u\n", x, y);
			} // end main list
		}
		nk_end(ctx);

		//return;
	}


	if (nk_begin(ctx, "Controls", nk_rect(0, 0, scr_w, GUI_BAR_HEIGHT), NK_WINDOW_NO_SCROLLBAR))
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
			// also don't let GUI disappear when the menu is active
			g->show_gui = 1;
			g->gui_timer = SDL_GetTicks();

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

			state = (g->menu_state == MENU_MISC) ? NK_MAXIMIZED : NK_MINIMIZED;
			if (nk_tree_state_push(ctx, NK_TREE_TAB, "Misc. Actions", &state)) {
				g->menu_state = MENU_MISC;
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

				if (nk_selectable_label(ctx, "Best fit", NK_TEXT_LEFT, &g->fill_mode)) {
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i)
							set_rect_bestfit(&g->img[i], g->fullscreen | g->slideshow | g->fill_mode);
					} else {
						set_rect_bestfit(g->img_focus, g->fullscreen | g->slideshow | g->fill_mode);
					}
				}
				nk_label(ctx, "F", NK_TEXT_RIGHT);

				if (nk_menu_item_label(ctx, "Actual Size", NK_TEXT_LEFT)) {
					event.user.code = ACTUAL_SIZE;
					SDL_PushEvent(&event);
				}
				nk_label(ctx, "A", NK_TEXT_RIGHT);

				nk_tree_pop(ctx);
			} else g->menu_state = (g->menu_state == MENU_MISC) ? MENU_NONE : g->menu_state;

			state = (g->menu_state == MENU_SORT) ? NK_MAXIMIZED : NK_MINIMIZED;
			if (nk_tree_state_push(ctx, NK_TREE_TAB, "Sort Actions", &state)) {
				g->menu_state = MENU_SORT;

				if (g->n_imgs == 1) {
					nk_layout_row(ctx, NK_DYNAMIC, 0, 2, ratios);
					if (nk_menu_item_label(ctx, "Mix images", NK_TEXT_LEFT)) {
						event.user.code = SHUFFLE;
						SDL_PushEvent(&event);
					}
					nk_label(ctx, "M", NK_TEXT_RIGHT);

					// TODO I don't think it's worth it supporting showing state and reverse sorting in normal mode
					//if (nk_menu_item_symbol_label(ctx, NK_SYMBOL_TRIANGLE_UP, "Sort by file name (default)", NK_TEXT_RIGHT)) {
					if (nk_menu_item_label(ctx, "Sort by file name (default)", NK_TEXT_LEFT)) {
						event.user.code = SORT_NAME;
						SDL_PushEvent(&event);
					}
					nk_label(ctx, "N", NK_TEXT_RIGHT);

					if (nk_menu_item_label(ctx, "Sort by file path", NK_TEXT_LEFT)) {
						event.user.code = SORT_PATH;
						SDL_PushEvent(&event);
					}
					nk_label(ctx, "CTRL+N", NK_TEXT_RIGHT);

					if (nk_menu_item_label(ctx, "Sort by size", NK_TEXT_LEFT)) {
						event.user.code = SORT_SIZE;
						SDL_PushEvent(&event);
					}
					nk_label(ctx, "Z", NK_TEXT_RIGHT);   // S is save...

					if (nk_menu_item_label(ctx, "Sort by last modified", NK_TEXT_LEFT)) {
						event.user.code = SORT_MODIFIED;
						SDL_PushEvent(&event);
					}
					nk_label(ctx, "T", NK_TEXT_RIGHT);
				} else {
					nk_layout_row_dynamic(ctx, 0, 1);
					nk_label(ctx, "Only available in 1 image mode", NK_TEXT_LEFT);
				}

				nk_tree_pop(ctx);
			} else g->menu_state = (g->menu_state == MENU_SORT) ? MENU_NONE : g->menu_state;

			state = (g->menu_state == MENU_EDIT) ? NK_MAXIMIZED: NK_MINIMIZED;
			if (nk_tree_state_push(ctx, NK_TREE_TAB, "Image Actions", &state)) {
				g->menu_state = MENU_EDIT;
				nk_layout_row(ctx, NK_DYNAMIC, 0, 2, ratios);

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

				if (nk_menu_item_label(ctx, "Flip Horizontal", NK_TEXT_LEFT)) {
					event.user.code = FLIP_H;
					SDL_PushEvent(&event);
				}
				nk_label(ctx, "H", NK_TEXT_RIGHT);

				if (nk_menu_item_label(ctx, "Flip Vertical", NK_TEXT_LEFT)) {
					event.user.code = FLIP_V;
					SDL_PushEvent(&event);
				}
				nk_label(ctx, "V", NK_TEXT_RIGHT);

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

				// have to treat switching back from list the same as I do switching
				// back from thumb, selection is the 1st image, have to load following
				// if in 2,4, or 8 mode
				if (nk_menu_item_label(ctx, "List Mode", NK_TEXT_LEFT)) {
					event.user.code = LIST_MODE;
					SDL_PushEvent(&event);
				}
				nk_label(ctx, "CTRL+I", NK_TEXT_RIGHT);

				if (nk_menu_item_label(ctx, "Thumb Mode", NK_TEXT_LEFT)) {
					event.user.code = THUMB_MODE;
					SDL_PushEvent(&event);
				}
				nk_label(ctx, "CTRL+U", NK_TEXT_RIGHT);

				nk_tree_pop(ctx);
			} else g->menu_state = (g->menu_state == MENU_VIEW) ? MENU_NONE: g->menu_state;

			nk_menu_end(ctx);
		}
		if (IS_LIST_MODE() && !IS_VIEW_RESULTS()) {
			nk_end(ctx);  // end "Controls" window early
			return;
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
	}
	nk_end(ctx);

	if (g->show_infobar) {
		draw_infobar(ctx, scr_w, scr_h);
	}
}

void draw_prefs(struct nk_context* ctx, int scr_w, int scr_h)
{
	int popup_flags = NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_BORDER|NK_WINDOW_TITLE;

	int w = 550, h = 300;
	struct nk_rect bounds;
	struct nk_rect s;
	s.x = scr_w/2-w/2;
	s.y = scr_h/2-h/2;
	s.w = w;
	s.h = h;

	static struct nk_colorf bgf = { 0, 0, 0, 1 };

	if (nk_begin(ctx, "Preferences", s, popup_flags)) {
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

		nk_label(ctx, "Slideshow delay:", NK_TEXT_LEFT);
		nk_property_int(ctx, "#", 1, &g->slide_delay, 10, 1, 0.05);

		nk_label(ctx, "Hide GUI delay:", NK_TEXT_LEFT);
		nk_property_int(ctx, "#", 1, &g->gui_delay, 60, 1, 0.3);

		nk_label(ctx, "GUI in Fullscreen mode:", NK_TEXT_LEFT);
		static const char* gui_options[] = { "Delay", "Always", "Never" };
		bounds = nk_widget_bounds(ctx);
		g->fullscreen_gui = nk_combo(ctx, gui_options, NK_LEN(gui_options), g->fullscreen_gui, 12, nk_vec2(bounds.w, 300));

		nk_property_int(ctx, "Thumb rows", 2, &g->thumb_rows, 8, 1, 0.05);
		nk_property_int(ctx, "Thumb cols", 4, &g->thumb_cols, 15, 1, 0.05);


		nk_checkbox_label(ctx, "Show info bar", &g->show_infobar);

		if (nk_button_label(ctx, "Clear thumbnail cache")) {
			puts("Clearing thumbnails");
			SDL_Log("Clearing thumbnails\n");
			int ttimer = SDL_GetTicks();
			empty_dir(g->thumbdir);
			SDL_Log("Clearing thumbnails took %d\n", SDL_GetTicks()-ttimer);
		}


		nk_layout_row_dynamic(ctx, 0, 1);
		nk_label(ctx, "Cache directory:", NK_TEXT_LEFT);
		nk_label_wrap(ctx, g->cachedir);


		if (nk_button_label(ctx, "Ok")) {
			g->show_prefs = 0;;
		}
	}
	nk_end(ctx);
}

void draw_infobar(struct nk_context* ctx, int scr_w, int scr_h)
{
	char info_buf[STRBUF_SZ];
	char* size_str;
	unsigned long index, total;

	if (nk_begin(ctx, "Info", nk_rect(0, scr_h-GUI_BAR_HEIGHT, scr_w, GUI_BAR_HEIGHT), NK_WINDOW_NO_SCROLLBAR))
	{
		if (g->n_imgs == 1) {
			img_state* img = &g->img[0];

			size_str = g->files.a[img->index].size_str;

			// 2 options when viewing results, showing n/total like normal (so it'd jump between matches)
			// or showing n/results which is more useful imo
			//
			// Method 1
			//index = (IS_VIEW_RESULTS()) ? g->search_results.a[img->index] : img->index;
			//total = g->files.size;
			
			// Method 2
			index = img->index;
			total = (IS_VIEW_RESULTS()) ? g->search_results.size : g->files.size;

			int len = snprintf(info_buf, STRBUF_SZ, "%d x %d pixels  %s  %d %%    %lu / %lu", img->w, img->h, size_str, (int)(img->disp_rect.h*100.0/img->h), index+1, total);
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

void draw_thumb_infobar(struct nk_context* ctx, int scr_w, int scr_h)
{
	char info_buf[STRBUF_SZ];
	int len;
	int num_rows = (g->files.size+g->thumb_cols-1)/g->thumb_cols;
	int row;

	if (nk_begin(ctx, "Thumb Info", nk_rect(0, scr_h-GUI_BAR_HEIGHT, scr_w, GUI_BAR_HEIGHT), NK_WINDOW_NO_SCROLLBAR))
	{
		if (!(g->state & SEARCH_RESULTS)) {
			row = (g->thumb_sel + g->thumb_cols)/g->thumb_cols;
			len = snprintf(info_buf, STRBUF_SZ, "rows: %d / %d  image %d / %d", row, num_rows, g->thumb_sel+1, (int)g->files.size);
			if (len >= STRBUF_SZ) {
				puts("info path too long");
				cleanup(1, 1);
			}
		} else {
			row = (g->thumb_sel + g->thumb_cols)/g->thumb_cols;

			int i;
			if (g->thumb_sel == g->search_results.a[g->cur_result]) {
				i = g->cur_result + 1;
				len = snprintf(info_buf, STRBUF_SZ, "result: %d / %d  rows: %d / %d  image %d / %d", i, (int)g->search_results.size, row, num_rows, g->thumb_sel+1, (int)g->files.size);
			} else {
				len = snprintf(info_buf, STRBUF_SZ, "rows: %d / %d  image %d / %d", row, num_rows, g->thumb_sel+1, (int)g->files.size);
			}
			if (len >= STRBUF_SZ) {
				puts("info path too long");
				cleanup(1, 1);
			}
		}

		nk_layout_row_dynamic(ctx, 0, 1);
		nk_label(ctx, info_buf, NK_TEXT_RIGHT);
	}
	nk_end(ctx);
}


// removes all top level normal files from dirpath
int empty_dir(const char* dirpath)
{
	char fullpath[STRBUF_SZ] = { 0 };
	struct dirent* entry;
	DIR* dir;
	int ret;

	dir = opendir(dirpath);
	if (!dir) {
		// I feel like this func could be moved to a utilities library
		// so exiting here would be wrong
		perror("opendir");
		return 0;
	}

	while ((entry = readdir(dir))) {
		ret = snprintf(fullpath, STRBUF_SZ, "%s/%s", dirpath, entry->d_name);
		if (ret >= STRBUF_SZ) {
			printf("path too long\n");
		}
		// don't care about failures, won't remove
		// non-empty subdirs etc.
		remove(fullpath);
	}
	closedir(dir);

	return 1;
}

