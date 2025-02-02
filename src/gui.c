// The MIT License (MIT)
// 
// Copyright (c) 2017-2025 Robert Winkler
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
// to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
// CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

void cleanup(int ret, int called_setup);
void set_rect_bestfit(img_state* img, int fill_screen);
void set_fullscreen();
int try_move(int direction);
int start_scanning(void);

// TODO better name? cleardir?
int empty_dir(const char* dirpath);

enum { MENU_NONE, MENU_MISC, MENU_PLAYLIST, MENU_SORT, MENU_EDIT, MENU_VIEW };

void draw_prefs(struct nk_context* ctx, int scr_w, int scr_h);
void draw_infobar(struct nk_context* ctx, int scr_w, int scr_h);
void draw_thumb_infobar(struct nk_context* ctx, int scr_w, int scr_h);
void draw_playlist_manager(struct nk_context* ctx, int scr_w, int scr_h);
int draw_filebrowser(file_browser* fb, struct nk_context* ctx, int scr_w, int scr_h);
void draw_scanning(struct nk_context* ctx, int scr_w, int scr_h);


// TODO nuklear helper functions to separate file?
int do_color_setting(struct nk_context* ctx, const char* label, struct nk_color* color, enum nk_color_format format)
{
	struct nk_color c = *color;
	struct nk_colorf fcolor = nk_color_cf(c);

	// TODO should I let the caller set the layout?  Yes
	//nk_layout_row_dynamic(ctx, 0, 2);
	nk_label(ctx, label, NK_TEXT_LEFT);
	int width = nk_widget_width(ctx);
	if (nk_combo_begin_color(ctx, c, nk_vec2(width, width))) {

		// TODO 240?
		nk_layout_row_dynamic(ctx, 240, 1);
		fcolor = nk_color_picker(ctx, fcolor, format);
		nk_layout_row_dynamic(ctx, 0, 1);
		fcolor.r = nk_propertyf(ctx, "#R:", 0, fcolor.r, 1.0f, 0.01f,0.005f);
		fcolor.g = nk_propertyf(ctx, "#G:", 0, fcolor.g, 1.0f, 0.01f,0.005f);
		fcolor.b = nk_propertyf(ctx, "#B:", 0, fcolor.b, 1.0f, 0.01f,0.005f);

		if (format == NK_RGBA) {
			fcolor.a = nk_propertyf(ctx, "#A:", 0, fcolor.a, 1.0f, 0.01f,0.005f);
			*color = nk_rgba_cf(fcolor);
		} else {
			*color = nk_rgb_cf(fcolor);
		}
		nk_combo_end(ctx);
		return 1;
	}
	return 0;
}

// window dimensions
// TODO most of these need to scale/adjust with font size
// so they should all probably be variables not constants

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


#define GUI_BAR_HEIGHT (DFLT_FONT_SIZE+28)
#define GUI_MENU_WIN_W 550

// Nuklear seems to use the min(necessary, given) for menus so just pick a big height
#define GUI_MENU_WIN_H 1000

//#define GUI_PREFS_W 860
//#define GUI_PREFS_H 580

// button widths
#define GUI_MENU_W 80
#define GUI_PREV_NEXT_W 150
#define GUI_ZOOM_ROTATE_W 50

#define MAX_SLIDE_DELAY 10
#define MAX_GUI_DELAY 60
#define MAX_BUTTON_RPT_DELAY 3.0f

void search_filenames(int is_vimmode)
{
	// fast enough to do here?  I do it in events?
	text_buf[text_len] = 0;
	char* text = text_buf + ((is_vimmode) ? 1 : 0);

	SDL_Log("Final text = \"%s\"\n", text);

	// strcasestr is causing problems on windows
	// so just convert to lower before using strstr
	char lowertext[STRBUF_SZ] = { 0 };
	char lowername[STRBUF_SZ] = { 0 };

	// start at 1 to cut off '/'
	for (int i=0; text[i]; ++i) {
		lowertext[i] = tolower(text[i]);
	}

	// it'd be kind of cool to add results of multiple searches together if we leave this out
	// of course there might be duplicates.  Or we could make it search within the existing
	// search results, so consecutive searches are && together...
	g->search_results.size = 0;

	int j;
	char* name;
	for (int i=0; i<g->files.size; ++i) {
		name = g->files.a[i].name;
		if (!name) {
			continue;
		}

		for (j=0; name[j]; ++j) {
			lowername[j] = tolower(name[j]);
		}
		lowername[j] = 0;

		// searching name since I'm showing names not paths in the list
		if (strstr(lowername, lowertext)) {
			SDL_Log("Adding %s\n", g->files.a[i].path);
			cvec_push_i(&g->search_results, i);
		}
	}
	SDL_Log("found %d matches\n", (int)g->search_results.size);
}

// TODO find a better place to put this function
void transition_to_scanning(char* file)
{
	if (g->is_open_new) {
		cvec_clear_file(&g->files);

		// if we were in VIEW_RESULTS need to clear these
		// TODO For now we don't support "Open More" in view results
		text_buf[0] = 0;
		//memset(text_buf, 0, text_len+1);
		text_len = 0;
		g->search_results.size = 0;
	}

	// easier to do this than try for partial in "open more"
	g->generating_thumbs = SDL_FALSE;
	g->thumbs_done = SDL_FALSE;
	g->thumbs_loaded = SDL_FALSE;
	cvec_free_thumb_state(&g->thumbs);

	if (g->open_playlist) {
		cvec_push_str(&g->sources, "-l");
	} else if (g->open_recursive) {
		char* last_slash = strrchr(file, PATH_SEPARATOR);
		if (last_slash) {
			cvec_push_str(&g->sources, "-r");

			// don't want to turn "/somefile" into ""
			if (last_slash != file) {
				*last_slash = 0;
			} else {
				last_slash[1] = 0;
			}
		} else {
			assert(last_slash); // should never get here
		}
	}
	cvec_push_str(&g->sources, file);
	start_scanning();
}

void draw_gui(struct nk_context* ctx)
{
	// closable gives the x, if you use it it won't come back (probably have to call show() or
	// something...
	int popup_flags = NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_BORDER|NK_WINDOW_TITLE;//NK_WINDOW_CLOSABLE;

	SDL_Event event = { .type = g->userevent };
	img_state* img;
	char buf[STRBUF_SZ];

	// Can't use actual screen size g->scr_w/h have to
	// calculate logical screen size since GUI is scaled
	int scr_w = g->scr_w/g->x_scale;
	int scr_h = g->scr_h/g->y_scale;

	snprintf(buf, STRBUF_SZ, "Active: %s", strrchr(g->cur_playlist, '/')+1);

	struct nk_rect bounds;
	const struct nk_input* in = &ctx->input;

	if (IS_SCANNING_MODE()) {
		draw_scanning(g->ctx, scr_w, scr_h);
		return;
	}

	if (IS_FS_MODE()) {
		if (!draw_filebrowser(&g->filebrowser, g->ctx, scr_w, scr_h)) {
			if (g->filebrowser.file[0]) {
				transition_to_scanning(g->filebrowser.file);
			} else if (g->files.size) {
				// they canceled out but still have current images
				g->state = g->old_state;
				SDL_ShowCursor(SDL_ENABLE);
				g->gui_timer = SDL_GetTicks();
				g->show_gui = SDL_TRUE;
				g->status = REDRAW;
				SDL_SetWindowTitle(g->win, g->files.a[g->img[0].index].name);
			} else {
				// They "Cancel"ed out of an initial startup with no files, so just exit
				event.type = SDL_QUIT;
				SDL_PushEvent(&event);
			}
			
			/*
			 * Do I need to do all state transitions when procesing events? No
			 * Should I keep all state transitions in the same place?  Probably
			if (g->filebrowser.file[0]) {
				start_scanning();
				event.user.code = PROCESS_SELECTION;
				SDL_PushEvent(&event);
			} 
			*/
		}
		return;
	}

	// Do popups first so I can return early if eather is up
	// TODO I can just return after a popup since they're all fullscreen now right?
	if (g->show_rotate) {
		// TODO make full screen or adjust for font size
		int tmp;
		struct nk_rect s = {0, 0, scr_w, scr_h};
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
				g->show_rotate = SDL_FALSE;;
			}
		}
		nk_end(ctx);
	}


	if (g->show_about) {
		// TODO make full screen or adjust for font_size
		struct nk_rect s = { 0, 0, scr_w, scr_h };

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
			nk_layout_row(ctx, NK_DYNAMIC, 0, 2, ratios);

			nk_label(ctx, "stb_image*", NK_TEXT_LEFT);
			nk_label(ctx, "github.com/nothings/stb", NK_TEXT_RIGHT);
			nk_label(ctx, "SDL2", NK_TEXT_LEFT);
			nk_label(ctx, "libsdl.org", NK_TEXT_RIGHT);
			nk_label(ctx, "nuklear GUI", NK_TEXT_LEFT);
			nk_label(ctx, "github.com/Immediate-Mode-UI/Nuklear", NK_TEXT_RIGHT);
			nk_label(ctx, "libcurl", NK_TEXT_LEFT);
			nk_label(ctx, "curl.haxx.se/libcurl/", NK_TEXT_RIGHT);
			nk_label(ctx, "WjCryptLib_Md5", NK_TEXT_LEFT);
			nk_label(ctx, "github.com/WaterJuice/WjCryptLib", NK_TEXT_RIGHT);

			// My own cvector lib

			nk_layout_row_dynamic(ctx, 0, 1);
			if (nk_button_label(ctx, "Ok")) {
				g->show_about = SDL_FALSE;;
			}
		}
		nk_end(ctx);
	}

	if (g->show_prefs) {
		draw_prefs(ctx, scr_w, scr_h);
	}

	if (g->show_pm) {
		draw_playlist_manager(ctx, scr_w, scr_h);
	}

	if (g->fullscreen && g->fullscreen_gui == NEVER) {
		return;
	}

	// don't show main GUI if a popup is up, don't want user to
	// be able to interact with it.  Could look up how to make them inactive
	// but meh, this is simpler
	if (g->show_about || g->show_prefs || g->show_rotate || g->show_pm) {
		//SDL_ShowCursor(SDL_ENABLE);  // shouldn't be necessary
		g->show_gui = SDL_TRUE;
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

	int is_selected = SDL_FALSE;
	int symbol;
	float search_ratio[] = { 0.25f, 0.75f };
	int list_height;
	int active;
	float search_height;

	char* name;

	static const char invalid_img[] = "Could Not Load";
	static struct nk_list_view lview, rview;
	static float header_ratios[] = {0.49f, 0.01f, 0.15f, 0.01f, 0.34f };
	static int splitter_down = 0;

	int search_flags = NK_EDIT_FIELD | NK_EDIT_SIG_ENTER | NK_EDIT_GOTO_END_ON_ACTIVATE;
	struct nk_window* search_text_field = NULL;

	if (!nk_input_is_mouse_down(in, NK_BUTTON_LEFT))
		splitter_down = 0;

	if (IS_LIST_MODE() && !IS_VIEW_RESULTS()) {
		if (nk_begin(ctx, "List", nk_rect(0, GUI_BAR_HEIGHT, scr_w, scr_h-GUI_BAR_HEIGHT), NK_WINDOW_NO_SCROLLBAR)) {

			// TODO With Enter to search, should I even have the Search button?  It's more of a label now... maybe put it on
			// the left?  and make it smaller?
			// TODO How to automatically focus on the search box if they start typing?
			nk_layout_row(ctx, NK_DYNAMIC, 0, 2, search_ratio);
			search_height = nk_widget_bounds(ctx).h;

			nk_label(ctx, "Search Filenames:", NK_TEXT_LEFT);

			if (!IS_RESULTS()) {
				nk_edit_focus(ctx, NK_EDIT_DEFAULT);
			}
			active = nk_edit_string(ctx, search_flags, text_buf, &text_len, STRBUF_SZ, nk_filter_default);
			if (active & NK_EDIT_COMMITED && text_len) {

				search_filenames(SDL_FALSE);
				memset(&rview, 0, sizeof(rview));
				g->state |= SEARCH_RESULTS;

				// use no selection to ignore the "Enter" in events so we don't exit
				// list mode.  Could add state to handle keeping the selection but meh
				g->selection = -1;  // no selection among search
				//nk_edit_unfocus(ctx);
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
						text_buf[0] = 0;
						text_len = 0;
						g->selection = g->img[0].index;
						g->list_setscroll = SDL_TRUE;
						// redundant since we clear before doing the search atm
						g->search_results.size = 0;
					}
				} else {
					if (nk_list_view_begin(ctx, &rview, "Result List", NK_WINDOW_BORDER, DFLT_FONT_SIZE+16, g->search_results.size)) {
						nk_layout_row(ctx, NK_DYNAMIC, 0, 3, ratios);
						int i;
						for (int j=rview.begin; j<rview.end; ++j) {
							i = g->search_results.a[j];
							// TODO Do I really need g->selection?  Can I use g->img[0].index (till I get multiple selection)
							// also thumb_sel serves the same/similar purpose
							is_selected = g->selection == j;
							name = g->files.a[i].name;
							if (!name) name = (char*)invalid_img;
							if (nk_selectable_label(ctx, name, NK_TEXT_LEFT, &is_selected)) {
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

									g->state |= NORMAL;
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
						nk_list_view_end(&rview);
					}
				}
				if (g->list_setscroll) {
					nk_uint x = 0, y;
					int scroll_limit = rview.total_height - list_height; // little off
					y = (g->selection/(float)(g->search_results.size-1) * scroll_limit) + 0.999f;
					nk_group_set_scroll(ctx, "Result List", x, y);
					g->list_setscroll = SDL_FALSE;
				}
			} else {
				if (nk_list_view_begin(ctx, &lview, "Image List", NK_WINDOW_BORDER, DFLT_FONT_SIZE+16, g->files.size)) {
					// TODO ratio layout 0.5 0.2 0.3 ? give or take
					//nk_layout_row_dynamic(ctx, 0, 3);
					nk_layout_row(ctx, NK_DYNAMIC, 0, 3, ratios);
					for (int i=lview.begin; i<lview.end; ++i) {
						assert(i < g->files.size);
						// Do I really need g->selection?  Can I use g->img[0].index (till I get multiple selection)
						// also thumb_sel serves the same/similar purpose
						is_selected = g->selection == i;
						name = g->files.a[i].name;
						if (!name) name = (char*)invalid_img;
						if (nk_selectable_label(ctx, name, NK_TEXT_LEFT, &is_selected)) {
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
		nk_layout_row_template_push_static(ctx, GUI_MENU_W);

		// prev next
		nk_layout_row_template_push_static(ctx, GUI_PREV_NEXT_W);
		nk_layout_row_template_push_static(ctx, GUI_PREV_NEXT_W);

		// zoom, -, +
		nk_layout_row_template_push_static(ctx, GUI_ZOOM_ROTATE_W);
		nk_layout_row_template_push_static(ctx, GUI_ZOOM_ROTATE_W);

		// Rotate left and right
		nk_layout_row_template_push_static(ctx, GUI_ZOOM_ROTATE_W);
		nk_layout_row_template_push_static(ctx, GUI_ZOOM_ROTATE_W);

		// Mode 1 2 4 8
		/*
		nk_layout_row_template_push_static(ctx, 80);
		nk_layout_row_template_push_static(ctx, 40);
		nk_layout_row_template_push_static(ctx, 40);
		nk_layout_row_template_push_static(ctx, 40);
		nk_layout_row_template_push_static(ctx, 40);
		*/
		nk_layout_row_template_end(ctx);

		if (nk_menu_begin_label(ctx, "Menu", NK_TEXT_LEFT, nk_vec2(GUI_MENU_WIN_W, GUI_MENU_WIN_H))) {
			// also don't let GUI disappear when the menu is active
			g->show_gui = SDL_TRUE;
			g->gui_timer = SDL_GetTicks();

			enum nk_collapse_states state;
			float ratios[] = { 0.7f, 0.3f, 0.8f, 0.2f };

			nk_layout_row_dynamic(ctx, 0, 3);
			nk_label(ctx, "GUI:", NK_TEXT_LEFT);
			if (nk_menu_item_label(ctx, "-", NK_TEXT_CENTERED)) {
				g->x_scale -= 0.5;
				g->y_scale -= 0.5;
				if (g->x_scale < 0.5) {
					g->x_scale = 0.5;
					g->y_scale = 0.5;
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
				g->show_prefs = SDL_TRUE;
			}
			if (nk_menu_item_label(ctx, "About", NK_TEXT_LEFT)) {
				g->show_about = SDL_TRUE;
			}
			if (nk_menu_item_label(ctx, "Exit", NK_TEXT_LEFT)) {
				event.type = SDL_QUIT;
				SDL_PushEvent(&event);
			}

			state = (g->menu_state == MENU_MISC) ? NK_MAXIMIZED : NK_MINIMIZED;
			if (nk_tree_state_push(ctx, NK_TREE_TAB, "Misc. Actions", &state)) {
				g->menu_state = MENU_MISC;
				nk_layout_row(ctx, NK_DYNAMIC, 0, 2, ratios);

				// Only support opening new files when in 1 image NORMAL/viewing mode
				// it simplifies things.
				if (g->n_imgs == 1 && (g->state & NORMAL)) {
					if (nk_menu_item_label(ctx, "Open New", NK_TEXT_LEFT)) {
						event.user.code = OPEN_FILE_NEW;
						SDL_PushEvent(&event);
					}
					nk_label(ctx, "O", NK_TEXT_RIGHT);

					// but only support opening additional when in exactly NORMAL mode
					// ie no VIEW_RESULTS (for now, would have to re-run the search and
					// update image indices)
					if (g->state == NORMAL) {
						// TODO naming
						if (nk_menu_item_label(ctx, "Open More", NK_TEXT_LEFT)) {
							event.user.code = OPEN_FILE_MORE;
							SDL_PushEvent(&event);
						}
						nk_label(ctx, "CTRL+O", NK_TEXT_RIGHT);
					}
				}

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
				nk_label(ctx, "CTRL+F/F11", NK_TEXT_RIGHT);

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

				if (g->has_bad_paths) {
					// TODO "Clean List"?
					if (nk_menu_item_label(ctx, "Remove Bad Paths", NK_TEXT_LEFT)) {
						event.user.code = REMOVE_BAD;
						SDL_PushEvent(&event);
					}
					nk_label(ctx, "C", NK_TEXT_RIGHT);
				}

				nk_tree_pop(ctx);
			} else g->menu_state = (g->menu_state == MENU_MISC) ? MENU_NONE : g->menu_state;


			state = (g->menu_state == MENU_PLAYLIST) ? NK_MAXIMIZED : NK_MINIMIZED;
			if (nk_tree_state_push(ctx, NK_TREE_TAB, "Playlist Actions", &state)) {
				g->menu_state = MENU_PLAYLIST;

				nk_layout_row_dynamic(ctx, 0, 1);
				nk_label(ctx, buf, NK_TEXT_LEFT);

				if (nk_menu_item_label(ctx, "Manager", NK_TEXT_LEFT)) {
					g->show_pm = SDL_TRUE;
					//event.user.code = OPEN_PLAYLIST_MANAGER;
					//SDL_PushEvent(&event);
				}

				nk_layout_row(ctx, NK_DYNAMIC, 0, 2, &ratios[2]);
				if (nk_menu_item_label(ctx, "Save", NK_TEXT_LEFT)) {
					event.user.code = SAVE_IMG;
					SDL_PushEvent(&event);
				}
				nk_label(ctx, "S", NK_TEXT_RIGHT);

				if (nk_menu_item_label(ctx, "Unsave", NK_TEXT_LEFT)) {
					event.user.code = UNSAVE_IMG;
					SDL_PushEvent(&event);
				}
				nk_label(ctx, "CTRL+S", NK_TEXT_RIGHT);

				nk_tree_pop(ctx);
			} else g->menu_state = (g->menu_state == MENU_PLAYLIST) ? MENU_NONE : g->menu_state;

			state = (g->menu_state == MENU_SORT) ? NK_MAXIMIZED : NK_MINIMIZED;
			if (nk_tree_state_push(ctx, NK_TREE_TAB, "Sort Actions", &state)) {
				g->menu_state = MENU_SORT;

				if (g->n_imgs == 1 && !g->generating_thumbs && g->state == NORMAL) {
					nk_layout_row(ctx, NK_DYNAMIC, 0, 2, &ratios[2]);
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
					nk_label(ctx, "while not generating thumbs", NK_TEXT_LEFT);
					nk_label(ctx, "and not viewing search results", NK_TEXT_LEFT);
				}

				nk_tree_pop(ctx);
			} else g->menu_state = (g->menu_state == MENU_SORT) ? MENU_NONE : g->menu_state;

			// For now can't do edits or change modes while in list mode, whatever mode you entered is
			// how you'll exit (you can do a search and view results, then do any edits or mode changes
			// in that mode before going back through list mode to normal mode)
			// so only show these tabs in normal or view results mode (thumbmode never draws the top gui at all)
			if (!IS_LIST_MODE() || IS_VIEW_RESULTS()) {
				state = (g->menu_state == MENU_EDIT) ? NK_MAXIMIZED: NK_MINIMIZED;
				if (nk_tree_state_push(ctx, NK_TREE_TAB, "Image Actions", &state)) {
					g->menu_state = MENU_EDIT;
					nk_layout_row(ctx, NK_DYNAMIC, 0, 2, &ratios[2]);

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

					if (g->n_imgs == 1) {
						if (nk_menu_item_label(ctx, "Remove", NK_TEXT_LEFT)) {
							event.user.code = REMOVE_IMG;
							SDL_PushEvent(&event);
						}
						nk_label(ctx, "BKSP", NK_TEXT_RIGHT);


						if (nk_menu_item_label(ctx, "Delete", NK_TEXT_LEFT)) {
							event.user.code = DELETE_IMG;
							SDL_PushEvent(&event);
						}
						nk_label(ctx, "DEL", NK_TEXT_RIGHT);
					}

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
			}

			nk_menu_end(ctx);
		}
		if (IS_LIST_MODE() && !IS_VIEW_RESULTS()) {
			nk_end(ctx);  // end "Controls" window early
			return;
		}

		static int lbutton_pressed_time = 0;
		static int rbutton_pressed_time = 0;
		int ticks = SDL_GetTicks();
		nk_button_set_behavior(ctx, NK_BUTTON_REPEATER);
		if (nk_button_symbol_label(ctx, NK_SYMBOL_TRIANGLE_LEFT, "prev", NK_TEXT_RIGHT)) {
			g->gui_timer = ticks;
			if (!lbutton_pressed_time) {
				lbutton_pressed_time = SDL_GetTicks();
				event.user.code = PREV;
				SDL_PushEvent(&event);
			} else if (ticks - lbutton_pressed_time >= g->button_rpt_delay*1000) {
				event.user.code = PREV;
				SDL_PushEvent(&event);
			}
		} else {
			lbutton_pressed_time = 0;
		}

		if (nk_button_symbol_label(ctx, NK_SYMBOL_TRIANGLE_RIGHT, "next", NK_TEXT_LEFT)) {
			g->gui_timer = ticks;
			if (!rbutton_pressed_time) {
				rbutton_pressed_time = SDL_GetTicks();
				event.user.code = NEXT;
				SDL_PushEvent(&event);
			} else if (ticks - rbutton_pressed_time >= g->button_rpt_delay*1000) {
				event.user.code = NEXT;
				SDL_PushEvent(&event);
			}
		} else {
			rbutton_pressed_time = 0;
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

#define FB_SIDEBAR_W 200

//scr_w and scr_h are logical dimensions not raw pixels
int draw_filebrowser(file_browser* fb, struct nk_context* ctx, int scr_w, int scr_h)
{
	int is_selected = SDL_FALSE;
	int symbol;
	int list_height;
	int active;
	float search_height = 0;
	int ret = 1;

	struct nk_rect bounds;
	const struct nk_input* in = &ctx->input;

	// TODO move to browser?
	static struct nk_list_view lview, rview;
	static int splitter_down = 0;
	char dir_buf[STRBUF_SZ];

#define UP_WIDTH 100
	float header_ratios[] = {0.49f, 0.01f, 0.15f, 0.01f, 0.34f };
	float path_szs[2] = { 0, UP_WIDTH };

	int search_flags = NK_EDIT_FIELD | NK_EDIT_SIG_ENTER | NK_EDIT_GOTO_END_ON_ACTIVATE;
	int text_path_flags = NK_EDIT_SELECTABLE | NK_EDIT_CLIPBOARD | NK_EDIT_AUTO_SELECT;

	SDL_Event event = { .type = g->userevent };

	cvector_file* f = &fb->files;

	if (fb->file[0]) {
		// You've already selected a file why are you calling draw_filebrowser?
		return 0;
	}

	if (!nk_input_is_mouse_down(in, NK_BUTTON_LEFT))
		splitter_down = 0;

	if (nk_begin(ctx, "File Selector", nk_rect(0, 0, scr_w, scr_h), NK_WINDOW_NO_SCROLLBAR)) {

		struct nk_rect win_content_rect = nk_window_get_content_region(ctx);
		struct nk_vec2 win_spacing = ctx->style.window.spacing;


		//printf("scr_w,scr_h = %d, %d\n%f %f\n", scr_w, scr_h, win_content_rect.w, win_content_rect.h);

		nk_layout_row_template_begin(ctx, 0);
		nk_layout_row_template_push_static(ctx, 100);
		nk_layout_row_template_push_dynamic(ctx);
		nk_layout_row_template_push_static(ctx, 100);
		nk_layout_row_template_end(ctx);

		// Can't cancel if there are no files
		//if (!g->files.size) {
		//	nk_widget_disable_begin(ctx);
		//}
		if (nk_button_label(ctx, "Cancel")) {
			// TODO maybe just have a done flag in file browser?
			ret = 0;
		}
		//nk_widget_disable_end(ctx);

		search_height = nk_widget_bounds(ctx).h;
		// Search field
		// TODO
		//nk_button_label(ctx, "Search");
		active = nk_edit_string(ctx, search_flags, fb->text_buf, &fb->text_len, STRBUF_SZ, nk_filter_default);
		if (active & NK_EDIT_COMMITED && fb->text_len) {

			fb_search_filenames(fb);
			memset(&rview, 0, sizeof(rview));
			fb->is_search_results = TRUE;

			// use no selection to ignore the "Enter" in events so we don't exit
			// list mode.  Could add state to handle keeping the selection but meh
			fb->selection = -1;  // no selection among search
			//nk_edit_unfocus(ctx);
		}

		// only enable "Open" button if you have a selection
		if (fb->selection < 0) {
			nk_widget_disable_begin(ctx);
		}
		if (nk_button_label(ctx, "Open")) {
			if (f->a[fb->selection].size == -1) {
				my_switch_dir(f->a[fb->selection].path);
			} else {
				strncpy(fb->file, f->a[fb->selection].path, MAX_PATH_LEN);
				ret = 0;
			}
		}
		nk_widget_disable_end(ctx);

		int path_rows = 1; // default 1 for text path
		// don't show path if recents or in root directory "/"
		if (!fb->is_recents && fb->dir[1]) {
			strncpy(dir_buf, fb->dir, sizeof(dir_buf));
			// method 1
			// breadcrumb buttons
			// I really hate Windows.  TODO Come up with a better way/compromise between visuals
			// and code
			if (!fb->is_text_path) {
				int depth = 0; // number of breadcrumb buttons;
				ctx->style.window.spacing.x = 0;

				char *d = dir_buf;
#ifndef _WIN32
				char *begin = d + 1;
#else
				char *begin = d;
#endif
				char tmp;
				nk_layout_row_dynamic(ctx, 0, 6);
				while (*d++) {
					tmp = *d;
					if (tmp == '/' || (!tmp && begin != d)) {
#ifndef _WIN32
						*d = '\0';
#else
						char tmp2 = 0;
						if (d != &begin[2]) {
							*d = '\0';
							tmp2 = 0;
						} else {
							tmp2 = begin[3];
							begin[3] = '\0';
						}
#endif
						if (nk_button_label(ctx, begin)) {
							my_switch_dir(dir_buf);
							break;
						}
						depth++;
						if (tmp) *d = '/';

#ifdef _WIN32
						if (tmp2) begin[3] = tmp2;
#endif

						begin = d + 1;
					}
				}
				path_rows = depth/6 + 1;
				ctx->style.window.spacing.x = win_spacing.x;
			} else {

				// 2 Methods of text path mode

				// method 2
				// TODO how to make this look like method 3, submit issue/documentation
				path_szs[0] = win_content_rect.w-win_spacing.x-UP_WIDTH;
				nk_layout_row(ctx, NK_STATIC, 0, 2, path_szs);

				// method 3
				/*
				nk_layout_row_template_begin(ctx, 0);
				nk_layout_row_template_push_dynamic(ctx);
				nk_layout_row_template_push_static(ctx, 100);
				nk_layout_row_template_end(ctx);
				*/

				int dir_len = strlen(fb->dir);
				nk_edit_string(ctx, text_path_flags, fb->dir, &dir_len, MAX_PATH_LEN, nk_filter_default);


				if (nk_button_label(ctx, "Up")) {
					char* s = strrchr(dir_buf, '/');
					assert(s); // should never be NULL since "/" or "C:/"
#ifndef _WIN32
					if (s != dir_buf) {
						*s = 0;
#else
					if (s[1]) {
						// Don't want to turn "C:/" into "C:" since that isn't actually a proper path
						// ie opendir() fails on "C:"
						if (s == &dir_buf[2]) {
							s[1] = 0;
						} else {
							*s = 0;
						}
#endif
						my_switch_dir(dir_buf);
					} else {
						my_switch_dir("/");
					}
				}
			}
		}

		const float group_szs[] = { FB_SIDEBAR_W, scr_w-FB_SIDEBAR_W };

		bounds = nk_widget_bounds(ctx);
		nk_layout_row(ctx, NK_STATIC, scr_h-bounds.y, 2, group_szs);

		if (nk_group_begin(ctx, "Sidebar", NK_WINDOW_NO_SCROLLBAR)) {

			int old;
			// no layouting needed for tree header apparently

			if (nk_tree_push(ctx, NK_TREE_NODE, "Settings", NK_MINIMIZED)) {

				// TODO no layout needed here either?  Or am I getting a default somehow?
				// would static be faster?
				nk_layout_row_dynamic(ctx, 0, 1);

				if (fb->num_exts) {
					//nk_checkbox_label(ctx, "All Files", &fb->ignore_exts);
					static const char* ext_opts[] = { FILE_TYPE_STR, "All Files" };
					struct nk_rect bounds = nk_widget_bounds(ctx);
					old = fb->ignore_exts;
					fb->ignore_exts = nk_combo(ctx, ext_opts, NK_LEN(ext_opts), old, DFLT_FONT_SIZE, nk_vec2(bounds.w, 300));
					if (fb->ignore_exts != old) {
						if (!fb->is_recents) {
							my_switch_dir(NULL);
						} else {
							handle_recents(fb);
						}
					}
				}
				static const char* path_opts[] = { "Breadcrumbs", "Text" };
				struct nk_rect bounds = nk_widget_bounds(ctx);
				fb->is_text_path = nk_combo(ctx, path_opts, NK_LEN(path_opts), fb->is_text_path, DFLT_FONT_SIZE, nk_vec2(bounds.w, 300));

				if (nk_checkbox_label(ctx, "Show Hidden", &g->filebrowser.show_hidden)) {
					my_switch_dir(NULL);
				}

				bounds = nk_widget_bounds(ctx);
				if (nk_checkbox_label(ctx, "Single Image", &g->open_single) && g->open_single) {
					g->open_playlist = SDL_FALSE;
					g->open_recursive = SDL_FALSE;
				}
				if (nk_input_is_mouse_hovering_rect(in, bounds)) {
					nk_tooltip(ctx, "Only open the image you select, not all images in the same directory");
				}

				bounds = nk_widget_bounds(ctx);
				nk_checkbox_label(ctx, "Playlist", &g->open_playlist);
				if (nk_input_is_mouse_hovering_rect(in, bounds)) {
					nk_tooltip(ctx, "Select a playlist instead of an image");
				}
				if (g->open_playlist) {
					g->open_single = SDL_FALSE;
					g->open_recursive = SDL_FALSE;

					old = fb->ignore_exts;
					fb->ignore_exts = SDL_TRUE;
					if (fb->ignore_exts != old) {
						if (!fb->is_recents) {
							my_switch_dir(NULL);
						} else {
							handle_recents(fb);
						}
					}
				}

				bounds = nk_widget_bounds(ctx);
				if (nk_checkbox_label(ctx, "Recursive", &g->open_recursive) && g->open_recursive) {
					g->open_playlist = SDL_FALSE;
					g->open_single = SDL_FALSE;
				}
				if (nk_input_is_mouse_hovering_rect(in, bounds)) {
					nk_tooltip(ctx, "Include images in all subdirectories");
				}

				nk_tree_pop(ctx);
			}

			nk_label(ctx, "Bookmarks", NK_TEXT_CENTERED);
			if (fb->get_recents) {
				if (nk_button_label(ctx, "Recents")) {
					handle_recents(fb);
				}
			}
			if (nk_button_label(ctx, "Home")) {
				my_switch_dir(fb->home);
			}
			if (nk_button_label(ctx, "Desktop")) {
				my_switch_dir(fb->desktop);
			}
			if (nk_button_label(ctx, "Computer")) {
				my_switch_dir("/");
			}
			if (nk_button_label(ctx, "Playlists")) {
				my_switch_dir(g->playlistdir);
			}

			if (nk_button_label(ctx, "Add Bookmark")) {
				cvec_push_str(&g->bookmarks, fb->dir);
			}


			bounds = nk_widget_bounds(ctx);

			nk_layout_row_static(ctx, scr_h-bounds.y, FB_SIDEBAR_W-win_spacing.x*2, 1);
			if (nk_group_begin(ctx, "User Bookmarks", 0)) {
				nk_layout_row_dynamic(ctx, 0, 1);
				char** bmarks = g->bookmarks.a;
				int remove = -1;
				for (int i=0; i<g->bookmarks.size; ++i) {
					char* b = bmarks[i];
					char* name = strrchr(b, '/');

					// handle Windows "C:/" correctly
					if (name == &b[2] && b[1] == ':') {
						name = &b[0];
					}
					if (name != b) {
						name++;
					}
					bounds = nk_widget_bounds(ctx);
					if (nk_button_label(ctx, name)) {
						my_switch_dir(b);
					}
					if (nk_contextual_begin(ctx, 0, nk_vec2(100, 300), bounds)) {

						nk_layout_row_dynamic(ctx, 25, 1);
						if (nk_menu_item_label(ctx, "Remove", NK_TEXT_LEFT)) {
							remove = i;
						}

						//nk_slider_int(ctx, 0, &slider, 16, 1);
						nk_contextual_end(ctx);
					}
				}
				if (remove >= 0) {
					cvec_erase_str(&g->bookmarks, remove, remove);
				}
				nk_group_end(ctx);
			}
			nk_group_end(ctx);
		}

		if (nk_group_begin(ctx, "List", 0)) {

			// main list column headers and splitters
			nk_layout_row(ctx, NK_DYNAMIC, 0, 5, header_ratios);

			symbol = NK_SYMBOL_NONE; // 0
			if (fb->sorted_state == NAME_UP)
				symbol = NK_SYMBOL_TRIANGLE_UP;
			else if (fb->sorted_state == NAME_DOWN)
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
			if (fb->sorted_state == SIZE_UP)
				symbol = NK_SYMBOL_TRIANGLE_UP;
			else if (fb->sorted_state == SIZE_DOWN)
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
			if (fb->sorted_state == MODIFIED_UP)
				symbol = NK_SYMBOL_TRIANGLE_UP;
			else if (fb->sorted_state == MODIFIED_DOWN)
				symbol = NK_SYMBOL_TRIANGLE_DOWN;

			if (nk_button_symbol_label(ctx, symbol, "Modified", NK_TEXT_LEFT)) {
				event.user.code = SORT_MODIFIED;
				SDL_PushEvent(&event);
			}

			float ratios[] = { header_ratios[0]+0.01f, header_ratios[2], header_ratios[4]+0.01f };

			// path_rows is 1 for text mode or >=1 for breadcrumb mode, +2 for
			// the search bar and the column header buttons
			nk_layout_row_dynamic(ctx, scr_h-(path_rows+2)*search_height, 1);


			if (fb->is_search_results) {
				if (!fb->search_results.size) {
					if (nk_button_label(ctx, "No matching results")) {
						fb->is_search_results = FALSE;
						fb->text_buf[0] = 0;
						fb->text_len = 0;
						fb->selection = -1;
						fb->list_setscroll = TRUE;
					}
				} else {
					if (nk_list_view_begin(ctx, &rview, "Result List", NK_WINDOW_BORDER, DFLT_FONT_SIZE+16, fb->search_results.size)) {
						nk_layout_row(ctx, NK_DYNAMIC, 0, 3, ratios);
						int i;
						for (int j=rview.begin; j<rview.end; ++j) {
							i = fb->search_results.a[j];
							// TODO Do I really need fb->selection?  Can I use g->img[0].index (till I get multiple selection)
							// also thumb_sel serves the same/similar purpose
							is_selected = fb->selection == j;
							if (nk_selectable_label(ctx, f->a[i].name, NK_TEXT_LEFT, &is_selected)) {
								if (is_selected) {
									fb->selection = j;
								} else {
									// could support unselecting, esp. with CTRL somehow if I ever allow
									// multiple selection
									// fb->selection = -1;

									// for now, treat clicking a selection as a "double click" ie same as return
									if (f->a[i].size == -1) {
										my_switch_dir(f->a[i].path);
										break;
									} else {
										strncpy(fb->file, f->a[i].path, MAX_PATH_LEN);
										ret = 0;
									}
									//break; //?
								}
							}
							nk_label(ctx, f->a[i].size_str, NK_TEXT_RIGHT);
							nk_label(ctx, f->a[i].mod_str, NK_TEXT_RIGHT);
						}
						list_height = ctx->current->layout->clip.h; // ->bounds.h?
						nk_list_view_end(&rview);
					}
				}
				if (fb->list_setscroll && (rview.end-rview.begin < f->size)) {
					nk_uint x = 0, y;
					int scroll_limit = rview.total_height - list_height; // little off
					y = (fb->selection/(float)(f->size-1) * scroll_limit) + 0.999f;
					//nk_group_get_scroll(ctx, "Image List", &x, &y);
					nk_group_set_scroll(ctx, "Result List", x, y);
					fb->list_setscroll = FALSE;
				}
			} else {
				if (nk_list_view_begin(ctx, &lview, "File List", NK_WINDOW_BORDER, DFLT_FONT_SIZE+16, f->size)) {
					// TODO ratio layout 0.5 0.2 0.3 ? give or take
					//nk_layout_row_dynamic(ctx, 0, 3);
					nk_layout_row(ctx, NK_DYNAMIC, 0, 3, ratios);
					for (int i=lview.begin; i<lview.end; ++i) {
						assert(i < f->size);
						is_selected = fb->selection == i;
						if (nk_selectable_label(ctx, f->a[i].name, NK_TEXT_LEFT, &is_selected)) {
							if (is_selected) {
								fb->selection = i;
							} else {
								if (f->a[i].size == -1) {
									my_switch_dir(f->a[i].path);
									break;
								} else {
									strncpy(fb->file, f->a[i].path, MAX_PATH_LEN);
									ret = 0;
								}
							}
						}
						nk_label(ctx, f->a[i].size_str, NK_TEXT_RIGHT);
						nk_label(ctx, f->a[i].mod_str, NK_TEXT_RIGHT);
					}
					list_height = ctx->current->layout->clip.h; // ->bounds.h?
					nk_list_view_end(&lview);
				}

				if (fb->list_setscroll && (lview.end-lview.begin < f->size)) {
					nk_uint x = 0, y;
					int scroll_limit = lview.total_height - list_height; // little off
					y = (fb->selection/(float)(f->size-1) * scroll_limit) + 0.999f;
					//nk_group_get_scroll(ctx, "Image List", &x, &y);
					nk_group_set_scroll(ctx, "File List", x, y);
					fb->list_setscroll = FALSE;
				}
			}
			nk_group_end(ctx);
		}
	}
	nk_end(ctx);

	return ret;
}

void draw_scanning(struct nk_context* ctx, int scr_w, int scr_h)
{
	char scan_buf[STRBUF_SZ] = {0};

	snprintf(scan_buf, STRBUF_SZ, "Collected %"PRIcv_sz" files...", g->files.size);
	if (nk_begin(ctx, "Scanning", nk_rect(0, 0, scr_w, scr_h), NK_WINDOW_NO_SCROLLBAR)) {
		
		nk_layout_row_dynamic(ctx, 0, 1);
		nk_label(ctx, scan_buf, NK_TEXT_CENTERED);
	}
	nk_end(ctx);
}


enum { PREFS_APPEARANCE, PREFS_BEHAVIOR, PREFS_DATA };

const char* color_labels[NK_COLOR_COUNT] =
{
	"TEXT",
	"WINDOW",
	"HEADER",
	"BORDER",
	"BUTTON",
	"BUTTON_HOVER",
	"BUTTON_ACTIVE",
	"TOGGLE",
	"TOGGLE_HOVER",
	"TOGGLE_CURSOR",
	"SELECT",
	"SELECT_ACTIVE",
	"SLIDER",
	"SLIDER_CURSOR",
	"SLIDER_CURSOR_HOVER",
	"SLIDER_CURSOR_ACTIVE",
	"PROPERTY",
	"EDIT",
	"EDIT_CURSOR",
	"COMBO",
	"CHART",
	"CHART_COLOR",
	"CHART_COLOR_HIGHLIGHT",
	"SCROLLBAR",
	"SCROLLBAR_CURSOR",
	"SCROLLBAR_CURSOR_HOVER",
	"SCROLLBAR_CURSOR_ACTIVE",
	"TAB_HEADER",
	"KNOB",
	"KNOB_CURSOR",
	"KNOB_CURSOR_HOVER",
	"KNOB_CURSOR_ACTIVE"
};


void draw_prefs(struct nk_context* ctx, int scr_w, int scr_h)
{
	int popup_flags = NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_BORDER|NK_WINDOW_TITLE;

	struct nk_rect bounds;
	struct nk_rect s = {0, 0, scr_w, scr_h };

	const float group_szs[] = { FB_SIDEBAR_W, scr_w-FB_SIDEBAR_W };

	// for data directory edit_strings
	static int cache_len;
	static int thumb_len;
	static int log_len;
	static int pl_len;

	// ugly hack
	static int has_inited = 0;
	if (!has_inited) {
		cache_len = strlen(g->cachedir);
		thumb_len = strlen(g->thumbdir);
		log_len = strlen(g->logdir);
		pl_len = strlen(g->playlistdir);

		has_inited = 1;
	}
	
	char label_buf[100];
	static int cur_prefs = PREFS_APPEARANCE;
	nk_bool is_selected;

	if (nk_begin(ctx, "Preferences", s, popup_flags)) {
		//bounds = nk_widget_bounds(ctx);
		// header height is
		// font height + 2*win.header.padding.y + 2*win.header.label_padding.y;
		// or font_height + 16
		// if no header, it's panel type.padding.y
		int header_h = DFLT_FONT_SIZE+16;
		nk_layout_row(ctx, NK_STATIC, scr_h-header_h, 2, group_szs);

		if (nk_group_begin(ctx, "Pref Sidebar", NK_WINDOW_BORDER)) {

			nk_layout_row_dynamic(ctx, 0, 1);
			is_selected = cur_prefs == PREFS_APPEARANCE;
			if (nk_selectable_label(ctx, "Appearance", NK_TEXT_LEFT, &is_selected)) {
				cur_prefs = PREFS_APPEARANCE;
			}
			is_selected = cur_prefs == PREFS_BEHAVIOR;
			if (nk_selectable_label(ctx, "Behavior", NK_TEXT_LEFT, &is_selected)) {
				cur_prefs = PREFS_BEHAVIOR;
			}
			is_selected = cur_prefs == PREFS_DATA;
			if (nk_selectable_label(ctx, "Data", NK_TEXT_LEFT, &is_selected)) {
				cur_prefs = PREFS_DATA;
			}

			if (nk_button_label(ctx, "Ok")) {
				g->show_prefs = SDL_FALSE;;
			}
			
			// TODO think about names of categories and where individual
			// items belong

			nk_group_end(ctx);
		}

		if (nk_group_begin(ctx, "Pref Panel", NK_WINDOW_BORDER)) {
			if (cur_prefs == PREFS_APPEARANCE) {
				nk_layout_row_dynamic(ctx, 0, 1);
				nk_label(ctx, "sdl_img colors:", NK_TEXT_LEFT);

				nk_layout_row_dynamic(ctx, 0, 2);
				do_color_setting(ctx, "background:", &g->bg, NK_RGB);

				// TODO split into normal and visual with visual controlling alpha?
				do_color_setting(ctx, "thumb highlight:", &g->thumb_highlight, NK_RGB);

				nk_layout_row_dynamic(ctx, 0, 1);
				if (nk_button_label(ctx, "Reset sdl_img colors to defaults")) {
					g->bg = nk_rgb(0,0,0);
					g->thumb_highlight = nk_rgb(0,255,0);
				}

				nk_label(ctx, "GUI colors:", NK_TEXT_LEFT);
				nk_layout_row_dynamic(ctx, 0, 2);
				int clicked = SDL_FALSE;
				for (int i=0; i<NK_COLOR_COUNT; i++) {
					snprintf(label_buf, sizeof(label_buf), "%s:", color_labels[i]);
					clicked |= do_color_setting(ctx, label_buf, &g->color_table[i], (i != 1) ? NK_RGB : NK_RGBA);
				}
				if (clicked) {
					nk_style_from_table(ctx, g->color_table);
				}


				nk_layout_row_dynamic(ctx, 0, 1);
				// TODO reset sdl_img colors too? or have a separet button for them
				if (nk_button_label(ctx, "Reset GUI colors to defaults")) {
					memcpy(g->color_table, nk_default_color_style, sizeof(nk_default_color_style));
					nk_style_default(ctx);

					// TODO have this be its own setting or keep part of window color?
					g->ctx->style.window.fixed_background.data.color.a *= 0.75;
				}



			} else if (cur_prefs == PREFS_BEHAVIOR) {
				nk_layout_row_dynamic(ctx, 0, 2);

				nk_label(ctx, "Slideshow delay:", NK_TEXT_LEFT);
				nk_property_int(ctx, "#", 1, &g->slide_delay, MAX_SLIDE_DELAY, 1, 0.3);

				nk_label(ctx, "Hide GUI delay:", NK_TEXT_LEFT);
				nk_property_int(ctx, "#", 1, &g->gui_delay, MAX_GUI_DELAY, 1, 0.3);

				nk_label(ctx, "Button repeat delay:", NK_TEXT_LEFT);
				nk_property_float(ctx, "#", 0.25f, &g->button_rpt_delay, MAX_BUTTON_RPT_DELAY, 0.25, 0.08333);

				// TODO should this go in appearance?
				nk_label(ctx, "GUI in Fullscreen mode:", NK_TEXT_LEFT);
				static const char* gui_options[] = { "Delay", "Always", "Never" };
				bounds = nk_widget_bounds(ctx);
				g->fullscreen_gui = nk_combo(ctx, gui_options, NK_LEN(gui_options), g->fullscreen_gui, DFLT_FONT_SIZE+28, nk_vec2(bounds.w, 800));

				/*
				* This doesn't scale well, looks like it's time to do that total Prefs
				* redesign
				nk_label(ctx, "Default Playlist:", NK_TEXT_LEFT);
				//bounds = nk_widget_bounds(ctx);
				g->default_playlist_idx = nk_combo(ctx, (const char* const*)g->playlists.a, g->playlists.size, g->default_playlist_idx, DFLT_FONT_SIZE+28, nk_vec2(bounds.w, 800));
				*/

				// TODO should these go in appearance?
				nk_property_int(ctx, "Thumb rows", MIN_THUMB_ROWS, &g->thumb_rows, MAX_THUMB_ROWS, 1, 0.2);
				nk_property_int(ctx, "Thumb cols", MIN_THUMB_COLS, &g->thumb_cols, MAX_THUMB_COLS, 1, 0.2);

				nk_checkbox_label(ctx, "Show info bar", &g->show_infobar);
				nk_checkbox_label(ctx, "x deletes in Thumb mode", &g->thumb_x_deletes);
				nk_checkbox_label(ctx, "Confirm delete in Normal mode", &g->confirm_delete);
				nk_checkbox_label(ctx, "Confirm rotation", &g->confirm_rotation);

				// TODO come up with better name/description
				nk_layout_row_dynamic(ctx, 0, 1);
				nk_checkbox_label(ctx, "Preserve relative offsets in multimode movement", &g->ind_mm);
			} else if (cur_prefs == PREFS_DATA) {
				// TODO make actually editable?  Have a folder selector popup?
				//float ratios[] = { 0.20, 0.80 };
				//nk_layout_row(ctx, NK_DYNAMIC, 60, 2, ratios);

				int path_flags = NK_EDIT_FIELD | NK_EDIT_AUTO_SELECT;

				nk_layout_row_dynamic(ctx, 0, 1);
				nk_label(ctx, "Cache:", NK_TEXT_LEFT);
				nk_edit_string(ctx, path_flags, g->cachedir, &cache_len, STRBUF_SZ, nk_filter_default);

				nk_label(ctx, "Thumbnails:", NK_TEXT_LEFT);
				nk_edit_string(ctx, path_flags, g->thumbdir, &thumb_len, STRBUF_SZ, nk_filter_default);

				nk_label(ctx, "Logs:", NK_TEXT_LEFT);
				nk_edit_string(ctx, path_flags, g->logdir, &log_len, STRBUF_SZ, nk_filter_default);

				nk_label(ctx, "Playlists:", NK_TEXT_LEFT);
				nk_edit_string(ctx, path_flags, g->playlistdir, &pl_len, STRBUF_SZ, nk_filter_default);

				nk_layout_row_dynamic(ctx, 0, 1);
				if (nk_button_label(ctx, "Clear cache")) {
					SDL_Log("Clearing cache\n");
					int ttimer = SDL_GetTicks();
					empty_dir(g->cachedir);
					SDL_Log("Clearing cache took %d\n", SDL_GetTicks()-ttimer);
				}
				if (nk_button_label(ctx, "Clear thumbnails")) {
					SDL_Log("Clearing thumbnails\n");
					int ttimer = SDL_GetTicks();
					empty_dir(g->thumbdir);
					SDL_Log("Clearing thumbnails took %d\n", SDL_GetTicks()-ttimer);
				}

				if (nk_button_label(ctx, "Clear logs")) {
					SDL_Log("Clearing logs\n");
					int ttimer = SDL_GetTicks();
					empty_dir(g->logdir);
					SDL_Log("Clearing logs took %d\n", SDL_GetTicks()-ttimer);
				}
			}

			nk_group_end(ctx);
		}



			/*
#define OK_WIDTH 200
		nk_layout_space_begin(ctx, NK_STATIC, 60, 1);
		nk_layout_space_push(ctx, nk_rect(scr_w-OK_WIDTH-12, 20, OK_WIDTH, 40));
		if (nk_button_label(ctx, "Ok")) {
			g->show_prefs = SDL_FALSE;;
		}
		nk_layout_space_end(ctx);
#undef OK_WIDTH
			*/
	}
	nk_end(ctx);
}

void draw_infobar(struct nk_context* ctx, int scr_w, int scr_h)
{
	char info_buf[STRBUF_SZ];
	char gif_buf[32];
	char* size_str;
	unsigned long index, total;
	float ratios[] = { 0.5f, 0.1, 0.4f };

	if (nk_begin(ctx, "Info", nk_rect(0, scr_h-GUI_BAR_HEIGHT, scr_w, GUI_BAR_HEIGHT), NK_WINDOW_NO_SCROLLBAR))
	{
		img_state* img = g->img_focus;

		if (g->n_imgs == 1) {
			img = &g->img[0];
		}

		if (img) {
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

			int len = snprintf(info_buf, STRBUF_SZ, "%dx%d %s %d%% %lu/%lu", img->w, img->h, size_str, (int)(img->disp_rect.h*100.0/img->h), index+1, total);
			if (len >= STRBUF_SZ) {
				SDL_LogCriticalApp("info path too long\n");
				cleanup(1, 1);
			}
			if (img->frames > 1) {
				snprintf(gif_buf, 32, "%d/%d", (int)img->frame_i+1, img->frames);

				nk_layout_row(ctx, NK_DYNAMIC, 0, 3, ratios);
				//nk_layout_row_static(ctx, 0, 3);
				nk_label(ctx, info_buf, NK_TEXT_LEFT);
				nk_label(ctx, gif_buf, NK_TEXT_RIGHT);

				// don't hide the GUI if you're interacting with it
				if ((g->progress_hovered = nk_widget_is_hovered(ctx))) {
					SDL_ShowCursor(SDL_ENABLE);
					g->gui_timer = SDL_GetTicks();
					g->show_gui = SDL_TRUE;
				}
				nk_progress(ctx, &img->frame_i, img->frames-1, NK_MODIFIABLE);
			} else {
				nk_layout_row_static(ctx, 0, scr_w, 1);
				nk_label(ctx, info_buf, NK_TEXT_LEFT);
			}
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

	if (nk_begin(ctx, "Thumb Info", nk_rect(0, scr_h-GUI_BAR_HEIGHT, scr_w, GUI_BAR_HEIGHT), NK_WINDOW_NO_SCROLLBAR)) {
		if (!(g->state & SEARCH_RESULTS)) {
			row = (g->thumb_sel + g->thumb_cols)/g->thumb_cols;
			len = snprintf(info_buf, STRBUF_SZ, "rows: %d / %d  image %d / %d", row, num_rows, g->thumb_sel+1, (int)g->files.size);
			if (len >= STRBUF_SZ) {
				SDL_LogCriticalApp("info path too long\n");
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
				SDL_LogCriticalApp("info path too long\n");
				cleanup(1, 1);
			}
		}

		if (g->state != THUMB_SEARCH) {
			nk_layout_row_dynamic(ctx, 0, 1);
			nk_label(ctx, info_buf, NK_TEXT_RIGHT);
		} else {
			// keep GUI up to show what they've typed
			g->gui_timer = SDL_GetTicks();
			nk_layout_row_dynamic(ctx, 0, 2);
			nk_label(ctx, text_buf, NK_TEXT_LEFT);
			nk_label(ctx, info_buf, NK_TEXT_RIGHT);
		}
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


// inline macro?
void get_playlist_path(char* path_buf, char* name)
{
	snprintf(path_buf, STRBUF_SZ, "%s/%s", g->playlistdir, name);
}



void draw_playlist_manager(struct nk_context* ctx, int scr_w, int scr_h)
{
	int popup_flags = NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_BORDER|NK_WINDOW_TITLE;
	int edit_flags = NK_EDIT_FIELD | NK_EDIT_SIG_ENTER | NK_EDIT_GOTO_END_ON_ACTIVATE;
	int is_selected = SDL_FALSE;
	int active, button_pressed;

	struct nk_rect bounds;
	char path_buf[STRBUF_SZ];
	char path_buf2[STRBUF_SZ];

	const char* new_rename[] = { "New", "Rename" };
	
	// should I reuse g->selected?  probably a bad idea
	// unless I want to easily reset to no selection every time
	// we open the pm...
	static int selected = -1;
	static char pm_buf[STRBUF_SZ];
	static int pm_len = 0;
	int nr_idx = 0;

	// TODO think about multiple selections for operations like
	// merge
	// Other operations like Copy/Duplicate?

	// start with same setup as File Browser
	const float group_szs[] = { FB_SIDEBAR_W, scr_w-FB_SIDEBAR_W };

	if (selected >= 0) {
		get_playlist_path(path_buf, g->playlists.a[selected]);
		nr_idx = 1;
	}

	if (nk_begin(ctx, "Playlist Manager", nk_rect(0, 0, scr_w, scr_h), popup_flags)) {

		nk_layout_row(ctx, NK_STATIC, 0, 2, group_szs);
		nk_label(ctx, "Active:", NK_TEXT_LEFT);
		nk_label(ctx, g->cur_playlist, NK_TEXT_LEFT);

		// Not sure if this will work doing things sort of out of order
		button_pressed = nk_button_label(ctx, new_rename[nr_idx]);
		active = nk_edit_string(ctx, edit_flags, pm_buf, &pm_len, STRBUF_SZ, nk_filter_default);
		pm_buf[pm_len] = 0;
		//printf("pm_len = %d\n", pm_len);
		if (button_pressed || (active & NK_EDIT_COMMITED && pm_len)) {
			if (cvec_contains_str(&g->playlists, pm_buf) < 0) {
				get_playlist_path(path_buf2, pm_buf);

				if (!nr_idx) {
					FILE* f = fopen(path_buf2, "w");
					if (!f) {
						// GUI indication of failure
						pm_len = snprintf(pm_buf, STRBUF_SZ, "%s", strerror(errno));
						SDL_Log("Failed to create %s: %s\n", path_buf2, pm_buf);
					} else {
						fclose(f);
						cvec_push_str(&g->playlists, pm_buf);
						pm_buf[0] = 0;
						pm_len = 0;
					}
				} else {
					if (rename(path_buf, path_buf2)) {
						pm_len = snprintf(pm_buf, STRBUF_SZ, "Failed to rename: %s", strerror(errno));
						SDL_Log("Failed to rename %s to %s: %s\n", path_buf, path_buf2, pm_buf);
					} else {
						cvec_replace_str(&g->playlists, selected, pm_buf, NULL);
						pm_buf[0] = 0;
						pm_len = 0;
						// update selection path
						strcpy(path_buf, path_buf2);
					}
				}
			} else {
				pm_len = snprintf(path_buf, STRBUF_SZ, "'%s' already exists!", pm_buf);
				strcpy(pm_buf, path_buf);
			}
		}

		bounds = nk_widget_bounds(ctx);
		nk_layout_row(ctx, NK_STATIC, scr_h-bounds.y, 2, group_szs);

		if (nk_group_begin(ctx, "Playlist Sidebar", NK_WINDOW_NO_SCROLLBAR)) {

			nk_layout_row_dynamic(ctx, 0, 1);
			// Rename?
			// Open New/More?  Or let FB handle that?

			if (nk_button_label(ctx, "Make Active")) {
				if (selected >= 0) {
					strncpy(g->cur_playlist, path_buf, STRBUF_SZ);
					read_cur_playlist();
				}
			}

			if (nk_button_label(ctx, "Open New")) {
				if (selected >= 0) {
					g->is_open_new = SDL_TRUE;
					g->open_playlist = SDL_TRUE;
					g->show_pm = SDL_FALSE;
					transition_to_scanning(path_buf);
				}
			}

			if (nk_button_label(ctx, "Open More")) {
				if (selected >= 0) {
					g->open_playlist = SDL_TRUE;
					g->show_pm = SDL_FALSE;
					transition_to_scanning(path_buf);
				}
			}

			if (nk_button_label(ctx, "Delete")) {
				if (selected >= 0) {
					SDL_Log("Trying to remove %s\n", path_buf);
					if (remove(path_buf)) {
						SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to delete playlist: %s", strerror(errno));
					} else {
						SDL_Log("Deleted playlist %s\n", g->playlists.a[selected]);
						cvec_erase_str(&g->playlists, selected, selected);
						selected = -1;
						path_buf[0] = 0;
					}
				}
			}

			if (nk_button_label(ctx, "Done")) {
				g->show_pm = SDL_FALSE;
			}

			nk_group_end(ctx);
		}
		if (nk_group_begin(ctx, "Playlists", 0)) {
			// TODO do I need this or is the layout from Sidebar still active?
			nk_layout_row_dynamic(ctx, 0, 1);

			for (int i=0; i<g->playlists.size; ++i) {
				is_selected = selected == i;
				if (nk_selectable_label(ctx, g->playlists.a[i], NK_TEXT_CENTERED, &is_selected)) {
					if (is_selected) {
						selected = i;
						get_playlist_path(path_buf, g->playlists.a[selected]);
					} else {
						selected = -1;
						path_buf[0] = 0;
					}
				}
			}
			nk_group_end(ctx);
		}
	}
	
	nk_end(ctx);
}


