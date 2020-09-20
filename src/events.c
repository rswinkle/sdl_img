
int handle_thumb_events()
{
	SDL_Event e;
	int sym;
	SDL_Keymod mod_state = SDL_GetModState();
	int mouse_x, mouse_y;
	SDL_GetMouseState(&mouse_x, &mouse_y);
	char title_buf[STRBUF_SZ];

	g->status = NOCHANGE;
	nk_input_begin(g->ctx);
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			// don't think I really need these since we'll be exiting anyway
			nk_input_end(g->ctx);
			return 1;
		case SDL_KEYUP:
			sym = e.key.keysym.sym;
			switch (sym) {
			case SDLK_ESCAPE:
				if (g->state & THUMB_DFLT) {
					g->state = NORMAL;
					g->thumb_start_row = 0;
					g->show_gui = SDL_TRUE;
				} else {
					g->state = THUMB_DFLT;
				}
				g->status = REDRAW;
				break;
			case SDLK_c:
				// turn off VISUAL (or any other mode I add later)
				if (mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
					g->state = THUMB_DFLT;
				}
				break;
			case SDLK_SLASH:
				g->state = THUMB_SEARCH;
				text[0] = 0;
				text_len = 0;
				g->search_results.size = 0;
				SDL_StartTextInput();
				break;
			case SDLK_v:
				if (g->state & THUMB_DFLT) {
					g->state = THUMB_VISUAL;
					g->thumb_sel_end = g->thumb_sel;
				} else if (g->state & THUMB_VISUAL) {
					g->state = THUMB_DFLT;
				}
				g->status = REDRAW;
				break;
			case SDLK_g:
				if (g->state != THUMB_SEARCH) {
					if (mod_state & (KMOD_LSHIFT | KMOD_RSHIFT)) {
						g->thumb_start_row = g->files.size-1; // will get fixed at the bottom
						g->thumb_sel = g->files.size-1;
					} else {
						g->thumb_start_row = 0;;
						g->thumb_sel = 0;
					}
				}
				break;

			// I can't decide, backspace for consistency with normal mode
			// but r for "removal" even though in vim r means replace and x
			// means delete but we have it remove and delete.  d would also work
			// for delete...  Also r is rotate right (and CTRL+r is arbitrary rotation)
			// in normal mode.  Also Backspace is kind of a reach and is equivalent to h
			// (move left) in vim...
			case SDLK_BACKSPACE:
			case SDLK_r:
			case SDLK_x:
				// TODO add a one time warning for x?  maybe a preference to turn warning on and off?
				if (g->state != THUMB_SEARCH)
					do_thumb_rem_del(sym == SDLK_x, mod_state & (KMOD_LCTRL | KMOD_RCTRL));
				break;
			case SDLK_RETURN:
				if (g->state & (THUMB_DFLT | SEARCH_RESULTS)) {
					if (g->state & SEARCH_RESULTS && mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
						g->state |= VIEW_MASK;

						// Just going to go to last result they were on
						// not necessarily the closet one
						//g->thumb_sel = g->search_results.a[g->cur_result];
						g->selection = (g->cur_result) ? g->cur_result-1 : g->search_results.size-1;
					} else {
						g->state = NORMAL;
						// subtract 1 since we reuse RIGHT loading code
						g->selection = (g->thumb_sel) ? g->thumb_sel - 1 : g->files.size-1;
					}

					SDL_ShowCursor(SDL_ENABLE);
					g->gui_timer = SDL_GetTicks();
					g->show_gui = SDL_TRUE;
					g->status = REDRAW;
					try_move(SELECTION);
				} else if (g->state == THUMB_SEARCH) {
					SDL_StopTextInput();
					// maybe give a parameter to switch between searching names and paths
					search_filenames();
					if (g->search_results.size) {
						g->thumb_sel = g->search_results.a[0];
						g->state |= SEARCH_RESULTS;
					} else {
						g->state = THUMB_DFLT;
					}
				}
				break;
			}
			break;
		case SDL_KEYDOWN:
			sym = e.key.keysym.sym;
			switch (sym) {
			case SDLK_UP:
			case SDLK_DOWN:
			case SDLK_k:
			case SDLK_j:
				if (mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
					g->thumb_rows += (sym == SDLK_DOWN || sym == SDLK_j) ? 1 : -1;
					if (g->thumb_rows < 2)
						g->thumb_rows = 2;
					if (g->thumb_rows > 8)
						g->thumb_rows = 8;
				} else if (g->state != THUMB_SEARCH) {
					g->thumb_sel += (sym == SDLK_DOWN || sym == SDLK_j) ? g->thumb_cols : -g->thumb_cols;
					fix_thumb_sel((sym == SDLK_DOWN || sym == SDLK_j) ? 1 : -1);
					SDL_ShowCursor(SDL_ENABLE);
					g->gui_timer = SDL_GetTicks();
					g->show_gui = 1;
				}
				break;
			case SDLK_LEFT:
			case SDLK_RIGHT:
			case SDLK_h:
			case SDLK_l:
				if (mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
					g->thumb_cols += (sym == SDLK_LEFT || sym == SDLK_h) ? -1 : 1;
					if (g->thumb_cols < 4)
						g->thumb_cols = 4;
					if (g->thumb_cols > 15)
						g->thumb_cols = 15;
				} else {
					if (g->state != THUMB_SEARCH) {
						g->thumb_sel += (sym == SDLK_h || sym == SDLK_LEFT) ? -1 : 1;
						fix_thumb_sel((sym == SDLK_h || sym == SDLK_LEFT) ? -1 : 1);
						SDL_ShowCursor(SDL_ENABLE);
						g->gui_timer = SDL_GetTicks();
						g->show_gui = 1;
					}
				}
				break;
			case SDLK_n:
				if (g->state & SEARCH_RESULTS) {
					if (mod_state & (KMOD_LSHIFT | KMOD_RSHIFT)) {
						if (g->thumb_sel == g->search_results.a[g->cur_result]) {
							g->cur_result--;
							if (g->cur_result < 0)
								g->cur_result += g->search_results.size;
						} else {
							int i;
							for (i = 0; i<g->search_results.size; ++i) {
								if (g->search_results.a[i] > g->thumb_sel)
									break;
							}
							if (!i)
								g->cur_result = g->search_results.size-1;
							else
								g->cur_result = i-1;
						}
					} else {
						if (g->thumb_sel == g->search_results.a[g->cur_result]) {
							g->cur_result = (g->cur_result + 1) % g->search_results.size;
						} else {
							int i;
							for (i = 0; i<g->search_results.size; ++i) {
								if (g->search_results.a[i] > g->thumb_sel)
									break;
							}
							if (i == g->search_results.size)
								g->cur_result = 0;
							else
								g->cur_result = i;
						}
					}
					g->thumb_sel = g->search_results.a[g->cur_result];
					SDL_ShowCursor(SDL_ENABLE);
					g->gui_timer = SDL_GetTicks();
					g->show_gui = 1;
				}
				break;
			case SDLK_BACKSPACE:
				if (text_len)
					text[--text_len] = 0;
				SDL_Log("text is \"%s\"\n", text);
				break;
			}
			break;
		case SDL_MOUSEMOTION:
			SDL_ShowCursor(SDL_ENABLE);
			g->gui_timer = SDL_GetTicks();
			g->show_gui = 1;
			break;
		case SDL_MOUSEBUTTONUP:
			// TODO should have this behavior in VISUAL MODE too?  Single click changes
			// g->thumb_sel, thus adjusting your visual selection?
			if (e.button.button == SDL_BUTTON_LEFT) {
				g->selection = g->thumb_start_row * g->thumb_cols +
				               (mouse_y / (g->scr_h/g->thumb_rows)) * g->thumb_cols +
				               (mouse_x / (g->scr_w/g->thumb_cols));

				// TODO better way to avoid duplication?
				if (g->selection >= g->files.size)
					break;
				if (e.button.clicks == 2) {
					// since we reuse the RIGHT loading code, have to subtract 1 so we
					// "move right" to the selection
					g->selection = (g->selection) ? g->selection - 1 : g->files.size-1;
					g->state = NORMAL;
					g->show_gui = SDL_TRUE;
					g->thumb_start_row = 0;
					g->status = REDRAW;
					try_move(SELECTION);
				} else {
					// TODO is there anything besides clicks == 1 or 2?
					g->thumb_sel = g->selection;
				}
			}
			break;
		case SDL_MOUSEWHEEL:
			g->status = REDRAW;
			if (g->state == THUMB_DFLT) {
				if (e.wheel.direction == SDL_MOUSEWHEEL_NORMAL) {
					g->thumb_sel -= e.wheel.y * g->thumb_cols;
					fix_thumb_sel(-e.wheel.y);
				} else {
					g->thumb_sel += e.wheel.y * g->thumb_cols;
					fix_thumb_sel(e.wheel.y);
				}
				SDL_ShowCursor(SDL_ENABLE);
				g->gui_timer = SDL_GetTicks();
				g->show_gui = 1;
			}
			break;

		case SDL_TEXTINPUT:
			// could probably just do text[text_len++] = e.text.text[0]
			// since I only handle ascii
			if (g->state == THUMB_SEARCH && text_len < STRBUF_SZ-1) {
				strcat(text, e.text.text);
				text_len += strlen(e.text.text);
				SDL_Log("text is \"%s\" \"%s\" %d %d\n", text, composition, cursor, selection_len);
				//SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "text is \"%s\" \"%s\" %d %d\n", text, composition, cursor, selection_len);
			}
			break;

		case SDL_TEXTEDITING:
			if (g->state == THUMB_SEARCH) {
				SDL_Log("recieved edit \"%s\"\n", e.edit.text);
				//SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "recieved edit \"%s\"\n", e.edit.text);
				composition = e.edit.text;
				cursor = e.edit.start;
				selection_len = e.edit.length;
			}
			break;

		case SDL_WINDOWEVENT: {
			g->status = REDRAW;
			int x, y;
			SDL_GetWindowSize(g->win, &x, &y);
			//SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "windowed %d %d %d %d\n", g->scr_w, g->scr_h, x, y);
			switch (e.window.event) {
			case SDL_WINDOWEVENT_RESIZED:
				g->scr_w = e.window.data1;
				g->scr_h = e.window.data2;
				break;
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				g->scr_w = e.window.data1;
				g->scr_h = e.window.data2;

				if (g->n_imgs == 1) {
					SET_MODE1_SCR_RECT();
				} else if (g->n_imgs == 2) {
					SET_MODE2_SCR_RECTS();
				} else if (g->n_imgs == 4) {
					SET_MODE4_SCR_RECTS();
				} else if (g->n_imgs == 8) {
					SET_MODE8_SCR_RECTS();
				}
				break;
			}
		} break;

		default:
			break;
		}

		// TODO leave it here where it calls for every event
		// or put it back in mouse and key events?
		nk_sdl_handle_event(&e);
	}
	nk_input_end(g->ctx);

	if (g->thumb_start_row*g->thumb_cols + g->thumb_rows*g->thumb_cols >= g->files.size+g->thumb_cols)
		g->thumb_start_row = (g->files.size / g->thumb_cols - g->thumb_rows+1);
	if (g->thumb_start_row < 0)
		g->thumb_start_row = 0;

	// can happen while thumbs are being generated/loaded
	if (!g->thumbs.a[g->thumb_sel].tex) {
		if (!g->thumb_sel) {
			for (; !g->thumbs.a[g->thumb_sel].tex && g->thumb_sel<g->files.size-1; ++g->thumb_sel);
		} else {
			for (; !g->thumbs.a[g->thumb_sel].tex && g->thumb_sel>0; --g->thumb_sel);
		}
		// No valid thumbs found, turn off visual
		// TODO also prevent thumbmode in the first place if there are no valid thumbs?
		if (!g->thumbs.a[g->thumb_sel].tex) {
			g->state = THUMB_DFLT;
		}
	}

	if (g->thumb_sel < g->thumb_start_row*g->thumb_cols) {
		g->thumb_start_row = g->thumb_sel / g->thumb_cols;
	} else if (g->thumb_sel >= g->thumb_start_row*g->thumb_cols + g->thumb_rows*g->thumb_cols) {
		g->thumb_start_row = g->thumb_sel / g->thumb_cols - g->thumb_rows + 1;
	}
		
	if (IS_THUMB_MODE()) {
		SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->thumb_sel].path, title_buf));
	} else if (g->img_focus) {
		SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img_focus->index].path, title_buf));
	} else {
		SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img[0].index].path, title_buf));
	}

	return 0;
}

int handle_list_events()
{
	SDL_Event e;
	int sym;
	int code, sort_timer;
	//SDL_Keymod mod_state = SDL_GetModState();

	g->status = NOCHANGE;
	nk_input_begin(g->ctx);
	while (SDL_PollEvent(&e)) {
		// TODO edit menu/GUI as appropriate for list mode, see which
		// actions make sense or are worth supporting (re-evaluate if I
		// have some sort of preview)
		if (e.type == g->userevent) {
			code = e.user.code;
			switch (code) {
			case THUMB_MODE:
				//do_thumbmode();
				break;
			case SHUFFLE:
				do_shuffle();
				break;
			case SORT_NAME:
				SDL_Log("Starting sort by name\n");
				sort_timer = SDL_GetTicks();
				if (g->sorted_state != NAME_UP) {
					do_sort(filename_cmp_lt);
					g->sorted_state = NAME_UP;
				} else {
					do_sort(filename_cmp_gt);
					g->sorted_state = NAME_DOWN;
				}
				SDL_Log("Sort took %d\n", SDL_GetTicks()-sort_timer);
				break;
			case SORT_PATH:
				SDL_Log("Starting sort by path\n");
				sort_timer = SDL_GetTicks();
				if (g->sorted_state != PATH_UP) {
					do_sort(filepath_cmp_lt);
					g->sorted_state = PATH_UP;
				} else {
					do_sort(filepath_cmp_gt);
					g->sorted_state = PATH_DOWN;
				}
				SDL_Log("Sort took %d\n", SDL_GetTicks()-sort_timer);
				break;
			case SORT_SIZE:
				SDL_Log("Starting sort by size\n");
				sort_timer = SDL_GetTicks();
				if (g->sorted_state != SIZE_UP) {
					do_sort(filesize_cmp_lt);
					g->sorted_state = SIZE_UP;
				} else {
					do_sort(filesize_cmp_gt);
					g->sorted_state = SIZE_DOWN;
				}
				SDL_Log("Sort took %d\n", SDL_GetTicks()-sort_timer);
				break;
			case SORT_MODIFIED:
				SDL_Log("Starting sort by modified\n");
				sort_timer = SDL_GetTicks();
				if (g->sorted_state != MODIFIED_UP) {
					do_sort(filemodified_cmp_lt);
					g->sorted_state = MODIFIED_UP;
				} else {
					do_sort(filemodified_cmp_gt);
					g->sorted_state = MODIFIED_DOWN;
				}
				SDL_Log("Sort took %d\n", SDL_GetTicks()-sort_timer);
				break;
				/*
			case DELETE_IMG:
				do_delete(&space);
				break;
				*/
			default:
				SDL_Log("Unknown user event!");
			}
			continue;
		}
		switch (e.type) {
		case SDL_QUIT:
			// don't think I really need these since we'll be exiting anyway
			nk_input_end(g->ctx);
			return 1;
		case SDL_KEYUP:
			sym = e.key.keysym.sym;
			switch (sym) {
			case SDLK_ESCAPE:
				if (g->state & SEARCH_RESULTS) {
					text[0] = 0;
					//memset(text, 0, text_len+1);
					text_len = 0;

					// if nothing was selected among search results set back
					// to current image
					if (g->selection < 0) {
						g->selection = g->img[0].index;
					} else {
						// convert selection
						g->selection = g->search_results.a[g->selection];

						if (g->state & VIEWED_RESULTS) {
							// TODO alternative, force switch to single mode?
							for (int i=0; i<g->n_imgs; ++i) {
								g->img[i].index = g->search_results.a[g->img[i].index];
							}
						}
					}
					g->list_setscroll = SDL_TRUE;
					g->state = LIST_DFLT;

					// redundant since we clear before doing the search atm
					g->search_results.size = 0;
				} else {
					g->state = NORMAL;
					SDL_ShowCursor(SDL_ENABLE);
					g->gui_timer = SDL_GetTicks();
					g->show_gui = SDL_TRUE;
					g->status = REDRAW;
				}
				break;

			// TODO removal and deletion in list mode?
			case SDLK_BACKSPACE:
			case SDLK_r:
			case SDLK_x:
				break;

			// switch to normal mode on that image
			case SDLK_RETURN:
				if (g->selection >= 0) {
					if (g->state & SEARCH_RESULTS) {
						g->state |= VIEW_MASK;
						g->selection = (g->selection) ? g->selection - 1 : g->search_results.size-1;
					} else {
						g->state = NORMAL;
						g->selection = (g->selection) ? g->selection - 1 : g->files.size-1;
					}
					// Same as switching from thumbmode
					SDL_ShowCursor(SDL_ENABLE);
					g->gui_timer = SDL_GetTicks();
					g->show_gui = SDL_TRUE;
					g->status = REDRAW;
					try_move(SELECTION);
				}
				break;
			}
			break;

		case SDL_KEYDOWN:
			sym = e.key.keysym.sym;
			switch (sym) {
			// TODO navigate through the list mode like thumb mode ie vim?
			case SDLK_UP:
			case SDLK_DOWN:
			case SDLK_k:
			case SDLK_j:
				g->selection += (sym == SDLK_DOWN || sym == SDLK_j) ? 1 : -1;
				if (g->state & SEARCH_RESULTS) {
					if (g->selection < 0)
						g->selection += g->search_results.size;
					else
						g->selection %= g->search_results.size;
				} else {
					if (g->selection < 0)
						g->selection += g->files.size;
					else
						g->selection %= g->files.size;
				}
				// TODO don't set unless necessary
				g->list_setscroll = SDL_TRUE;
				break;
			}

		case SDL_WINDOWEVENT: {
			g->status = REDRAW;
			int x, y;
			SDL_GetWindowSize(g->win, &x, &y);
			//SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "windowed %d %d %d %d\n", g->scr_w, g->scr_h, x, y);
			switch (e.window.event) {
			case SDL_WINDOWEVENT_RESIZED:
				g->scr_w = e.window.data1;
				g->scr_h = e.window.data2;
				break;
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				g->scr_w = e.window.data1;
				g->scr_h = e.window.data2;

				// TODO how/where to reset all the "subscreens" rects
				if (g->n_imgs == 1) {
					SET_MODE1_SCR_RECT();
				} else if (g->n_imgs == 2) {
					SET_MODE2_SCR_RECTS();
				} else if (g->n_imgs == 4) {
					SET_MODE4_SCR_RECTS();
				} else if (g->n_imgs == 8) {
					SET_MODE8_SCR_RECTS();
				}
				break;
			}
		} break;

		// all other event types
		default:
			break;
		}

		nk_sdl_handle_event(&e);
	}
	nk_input_end(g->ctx);

	return 0;
}

// TODO macro?
void mode_focus_change(intptr_t mode, SDL_Keymod mod_state, char* title_buf)
{
	// modes are 1,2,4,8
	int is_mode = (mode == 1 || mode == 2 || mode == 4 || mode == 8) ? 1 : 0;

	// parameter mode is used as mode or focus selection (1-8)

	if (is_mode && !g->loading && mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
		do_mode_change(mode);
	} else if (g->n_imgs >= mode && g->n_imgs >= 2) {
		// we don't use/set img_focus in single image mode
		g->img_focus = &g->img[mode-1];

		char* path;
		if (IS_RESULTS())
			path = g->files.a[g->search_results.a[g->img_focus->index]].path;
		else
			path = g->files.a[g->img_focus->index].path;

		SDL_SetWindowTitle(g->win, mybasename(path, title_buf));
	}
}

int handle_events_normally()
{
	SDL_Event e;
	int sc;
	int zoomed;
	char title_buf[STRBUF_SZ];
	img_state* img;

	// eat all escapes this frame after copy dialog ended with "no"
	int copy_escape = 0;

	g->status = NOCHANGE;

	SDL_Keymod mod_state = SDL_GetModState();

	// use space to move to next image(s) even if zoomed in, ie during slideshow
	SDL_Event space;
	space.type = SDL_KEYDOWN;
	space.key.keysym.scancode = SDL_SCANCODE_SPACE;

	// I only set this to clear valgrind errors of jumps in
	// nk_sdl_handle_event based uninitialized values
	space.key.keysym.sym = SDLK_SPACE;

	int ticks = SDL_GetTicks();

	SDL_LockMutex(g->mtx);
	if (g->done_loading) {
		if (g->done_loading >= LEFT) {
			img = (g->img == g->img1) ? g->img2 : g->img1;
			if (g->img_focus) {
				clear_img(g->img_focus);
				replace_img(g->img_focus, &img[0]);
				create_textures(g->img_focus);
			} else {
				for (int i=0; i<g->n_imgs; ++i) {
					create_textures(&img[i]);
					clear_img(&g->img[i]);
				}
				g->img = img;
			}
		} else {
			for (int i=g->n_imgs; i<g->done_loading; ++i)
				create_textures(&g->img[i]);

			if (g->done_loading == MODE2) {
				SET_MODE2_SCR_RECTS();
				g->n_imgs = 2;
				g->img_focus = NULL;
			} else if (g->done_loading == MODE4) {
				SET_MODE4_SCR_RECTS();
				g->n_imgs = 4;
				g->img_focus = NULL;
			} else {
				SET_MODE8_SCR_RECTS();
				g->n_imgs = 8;
				g->img_focus = NULL;
			}
		}
		g->done_loading = 0;
		g->status = REDRAW;
		if (g->slideshow)
			g->slide_timer =  SDL_GetTicks();
	}
	SDL_UnlockMutex(g->mtx);

	if (g->slideshow) {
		// pause slideshow if popup is up
		if (g->show_about || g->show_prefs) {
			g->slide_timer = ticks;
		} else if (!g->loading && ticks - g->slide_timer > g->slideshow) {
			int i;
			// make sure all current gifs have gotten to the end
			// at least once
			for (i=0; i<g->n_imgs; ++i) {
				if (!g->img[i].looped)
					break;
			}
			if (i == g->n_imgs) {
				SDL_PushEvent(&space);
			}
		}
	}

	int mouse_x, mouse_y;
	u32 mouse_button_mask = SDL_GetMouseState(&mouse_x, &mouse_y);
	int sort_timer;
	
	int done_rotate = 0;
	int code;
	nk_input_begin(g->ctx);
	while (SDL_PollEvent(&e)) {
		if (e.type == g->userevent) {
			// reset this everytime they interact with GUI
			// so it doesn't disappear even if they're holding
			// the mouse down but still (on zoom controls for example)
			g->gui_timer = SDL_GetTicks();

			code = e.user.code;
			switch (code) {
			case NEXT:
			case PREV:
				try_move(code == NEXT ? RIGHT : LEFT);
				break;
			case ZOOM_PLUS:
			case ZOOM_MINUS:
				do_zoom(code == ZOOM_PLUS ? GUI_ZOOM : -GUI_ZOOM, SDL_FALSE);
				break;
			case ROT_LEFT:
			case ROT_RIGHT:
				do_rotate(code == ROT_LEFT, SDL_TRUE);
				break;
			case FLIP_H:
			case FLIP_V:
				do_flip(code == FLIP_V);
				break;
			case ROT360:
				// TODO
				rotate_img((g->n_imgs == 1) ? &g->img[0] : g->img_focus);
				break;
			case LIST_MODE:
				do_listmode();
				break;
			case THUMB_MODE:
				do_thumbmode();
				break;
			case MODE_CHANGE:
				g->status = REDRAW;
				g->slide_timer =  SDL_GetTicks();
				do_mode_change((intptr_t)e.user.data1);
				break;
			case ACTUAL_SIZE:
				do_actual_size();
				break;
			case SHUFFLE:
				do_shuffle();
				break;
			case SORT_NAME:
				SDL_Log("Starting sort by name\n");
				sort_timer = SDL_GetTicks();
				do_sort(filename_cmp_lt);
				g->sorted_state = NAME_UP;
				SDL_Log("Sort took %d\n", SDL_GetTicks()-sort_timer);
				break;
			case SORT_PATH:
				SDL_Log("Starting sort by path\n");
				sort_timer = SDL_GetTicks();
				do_sort(filepath_cmp_lt);
				g->sorted_state = PATH_UP;
				SDL_Log("Sort took %d\n", SDL_GetTicks()-sort_timer);
				break;
			case SORT_SIZE:
				SDL_Log("Starting sort by size\n");
				sort_timer = SDL_GetTicks();
				do_sort(filesize_cmp_lt);
				g->sorted_state = SIZE_UP;
				SDL_Log("Sort took %d\n", SDL_GetTicks()-sort_timer);
				break;
			case SORT_MODIFIED:
				SDL_Log("Starting sort by modified\n");
				sort_timer = SDL_GetTicks();
				do_sort(filemodified_cmp_lt);
				g->sorted_state = MODIFIED_UP;
				SDL_Log("Sort took %d\n", SDL_GetTicks()-sort_timer);
				break;
			case DELETE_IMG:
				do_delete(&space);
				break;
			default:
				SDL_Log("Unknown user event!");
			}
			continue;
		}

		switch (e.type) {
		case SDL_QUIT:
			//nk_input_end(g->ctx); // TODO need these?
			return 1;
		case SDL_KEYUP:
			sc = e.key.keysym.scancode;
			switch (sc) {
			case SDL_SCANCODE_ESCAPE:
				if (!copy_escape && !g->fullscreen && !g->slideshow && !g->show_about &&
					!g->show_prefs && !g->show_rotate && g->state == NORMAL) {
					//nk_input_end(g->ctx);
					return 1;
				} else {
					if (g->show_rotate) {
						// TODO handle case where user hits ESC with image as
						// TO_ROTATE (could still be pristine or they rotated with preview, changed the angle
						// again and then hit ESC, only want to prompt to save in the latter case)
						g->show_rotate = nk_false;
					} else if (g->show_about) {
						g->show_about = nk_false;
					} else if (g->show_prefs) {
						g->show_prefs = nk_false;
					}else if (g->slideshow) {
						SDL_Log("Ending slideshow");
						g->slideshow = 0;
					} else if (g->fullscreen) {
						g->status = REDRAW;
						SDL_SetWindowFullscreen(g->win, 0);
						g->fullscreen = 0;
					} else if (IS_VIEW_RESULTS()) {
						g->state ^= VIEW_RESULTS;

						// selection is used in listmode results, = index in results
						g->selection = g->img[0].index;

						// thumb_sel is the actual index in g->files, since results are
						// not separated out, just highlighted like vim
						g->thumb_sel = g->search_results.a[g->selection];
						g->thumb_start_row = g->thumb_sel / g->thumb_cols;

						// for thumb mode we switch indices back immediately on leaving VIEW_RESULTS
						// compared to list mode SEARCH_RESULTS we leave them, till we go back to
						// LIST_DFLT.
						if (g->state & THUMB_SEARCH) {
							// TODO alternative, force switch to single mode?
							for (int i=0; i<g->n_imgs; ++i) {
								g->img[i].index = g->search_results.a[g->img[i].index];
							}
						}

						// TODO macro or function?  check how many uses
						SDL_ShowCursor(SDL_ENABLE);
						g->gui_timer = SDL_GetTicks();
						g->show_gui = SDL_TRUE;

						g->status = REDRAW; // necessary here or below?
					} else if (IS_THUMB_MODE() || IS_LIST_MODE()) {
						// TODO this can't be reached! ... right?
						g->state = NORMAL;
					}
				}
				break;

			case SDL_SCANCODE_DELETE:
				do_delete(&space);
				break;

			// CAPSLOCK comes right before F1 and F1-F12 are contiguous
			case SDL_SCANCODE_F1:
			case SDL_SCANCODE_F2:
			case SDL_SCANCODE_F3:
			case SDL_SCANCODE_F4:
			case SDL_SCANCODE_F5:
			case SDL_SCANCODE_F6:
			case SDL_SCANCODE_F7:
			case SDL_SCANCODE_F8:
			case SDL_SCANCODE_F9:
			case SDL_SCANCODE_F10:
				g->slideshow = (sc - SDL_SCANCODE_CAPSLOCK)*1000;
				g->slide_timer =  SDL_GetTicks();
				SDL_Log("Starting slideshow");
				break;

			case SDL_SCANCODE_F11:
				g->fullscreen = !g->fullscreen;
				set_fullscreen();
				break;

			case SDL_SCANCODE_0:
				g->img_focus = NULL;
				if (IS_VIEW_RESULTS())
					SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->search_results.a[g->img[0].index]].path, title_buf));
				else
					SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img[0].index].path, title_buf));
				break;
			case SDL_SCANCODE_1:
				mode_focus_change(MODE1, mod_state, title_buf);
				break;
			case SDL_SCANCODE_2:
				mode_focus_change(MODE2, mod_state, title_buf);
				break;
			case SDL_SCANCODE_3:
				g->status = REDRAW;
				mode_focus_change(3, mod_state, title_buf);
				break;
			case SDL_SCANCODE_4:
				mode_focus_change(MODE4, mod_state, title_buf);
				break;
			case SDL_SCANCODE_5:
				g->status = REDRAW;
				mode_focus_change(5, mod_state, title_buf);
				break;
			case SDL_SCANCODE_6:
				g->status = REDRAW;
				mode_focus_change(6, mod_state, title_buf);
				break;
			case SDL_SCANCODE_7:
				g->status = REDRAW;
				mode_focus_change(7, mod_state, title_buf);
				break;
			case SDL_SCANCODE_8:
				mode_focus_change(MODE8, mod_state, title_buf);
				break;

			case SDL_SCANCODE_A:
				do_actual_size();
				break;

			case SDL_SCANCODE_M:
				do_shuffle();
				break;

			case SDL_SCANCODE_N:
				if (mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
					do_sort(filepath_cmp_lt);
					g->sorted_state = PATH_UP;
				} else {
					do_sort(filename_cmp_lt);
					g->sorted_state = NAME_UP;
				}
				break;
			case SDL_SCANCODE_Z:
				do_sort(filesize_cmp_lt);
				g->sorted_state = SIZE_UP;
				break;
			case SDL_SCANCODE_T:
				do_sort(filemodified_cmp_lt);
				g->sorted_state = MODIFIED_UP;
				break;

			case SDL_SCANCODE_P:
				// doesn't matter if we "pause" static images
				if (!g->img_focus) {
					for (int i=0; i<g->n_imgs; ++i) {
						g->img[i].paused = !g->img[i].paused;
					}
				} else {
					g->img_focus->paused = !g->img_focus->paused;
				}
				break;

				// L is used by rotate dang.  could get rid of 360
				// rotation (since it's not in the GUI and was always
				// kind of janky anyway) to clear up CTRL+L...
			case SDL_SCANCODE_I:
				if (mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
					do_listmode();
				}
				break;

			case SDL_SCANCODE_U:
				if (mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
					do_thumbmode();
				} else {
					// TODO GUI for this?
					generate_thumbs(SDL_FALSE);
				}
				break;

			case SDL_SCANCODE_C:
				if (mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
					// TODO maybe just flush events here and return 0 so
					// no input for the current frame after CTRL+V? can I do
					// that without breaking the GUI?
					copy_escape = do_copy();
				}
			break;

#ifndef _WIN32
			case SDL_SCANCODE_S:
				do_save();
			break;
#endif

			case SDL_SCANCODE_H:
			case SDL_SCANCODE_V:
				do_flip(sc == SDL_SCANCODE_V);
			break;

			case SDL_SCANCODE_L:
			case SDL_SCANCODE_R:
				if (!done_rotate) {
					if (mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
						do_rotate(sc == SDL_SCANCODE_L, SDL_FALSE);
					} else {
						do_rotate(sc == SDL_SCANCODE_L, SDL_TRUE);
					}
					done_rotate = 1;
				}
				break;

			case SDL_SCANCODE_BACKSPACE:
				do_remove(&space);
				break;

			case SDL_SCANCODE_F: {
				g->status = REDRAW;
				if (mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
					g->fullscreen = !g->fullscreen;
					set_fullscreen();
				} else {
					g->fill_mode = !g->fill_mode;
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i)
							set_rect_bestfit(&g->img[i], g->fullscreen | g->slideshow | g->fill_mode);
					} else {
						set_rect_bestfit(g->img_focus, g->fullscreen | g->slideshow | g->fill_mode);
					}
				}
			}
				break;

			// Go to first image in list (useful after sorting different ways without having to use
			// list or thumb mode to scroll to the top/bottom)
			case SDL_SCANCODE_HOME:
				g->selection = g->files.size-1;
				try_move(SELECTION);
				break;
		}
			break;  //end SDL_KEYUP

		case SDL_KEYDOWN:
			// TODO use symcodes?
			sc = e.key.keysym.scancode;
			switch (sc) {

			case SDL_SCANCODE_SPACE:
				if (mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
					try_move(LEFT);
				} else {
					try_move(RIGHT);
				}
				break;

			// TODO merge RIGHT/DOWN and LEFT/UP?
			case SDL_SCANCODE_RIGHT:
				zoomed = 0;
				g->status = REDRAW;
				if (g->loading || !(mod_state & (KMOD_LCTRL | KMOD_RCTRL))) {
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i) {
							img = &g->img[i];
							if (img->disp_rect.w > img->scr_rect.w) {
								img->disp_rect.x -= PAN_RATE * img->disp_rect.w;
								fix_rect(img);
								zoomed = 1;
							}
							zoomed = zoomed || img->disp_rect.h > img->scr_rect.h;
						}
					} else {
						img = g->img_focus;
						if (img->disp_rect.w > img->scr_rect.w) {
							img->disp_rect.x -= PAN_RATE * img->disp_rect.w;
							fix_rect(img);
							zoomed = 1;
						}
						zoomed = zoomed || img->disp_rect.h > img->scr_rect.h;
					}
				}
				if (!zoomed) {
					try_move(RIGHT);
				}
				break;
			case SDL_SCANCODE_DOWN:
				zoomed = 0;
				g->status = REDRAW;
				if (g->loading || !(mod_state & (KMOD_LCTRL | KMOD_RCTRL))) {
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i) {
							img = &g->img[i];
							if (img->disp_rect.h > img->scr_rect.h) {
								img->disp_rect.y -= PAN_RATE * img->disp_rect.h;
								fix_rect(img);
								zoomed = 1;
							}
							zoomed = zoomed || img->disp_rect.w > img->scr_rect.w;
						}
					} else {
						img = g->img_focus;
						if (img->disp_rect.h > img->scr_rect.h) {
							img->disp_rect.y -= PAN_RATE * img->disp_rect.h;
							fix_rect(img);
							zoomed = 1;
						}
						zoomed = zoomed || img->disp_rect.w > img->scr_rect.w;
					}
				}
				if (!zoomed) {
					try_move(RIGHT);
				}
				break;

			case SDL_SCANCODE_LEFT:
				zoomed = 0;
				g->status = REDRAW;
				if (g->loading || !(mod_state & (KMOD_LCTRL | KMOD_RCTRL))) {
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i) {
							img = &g->img[i];
							if (img->disp_rect.w > img->scr_rect.w) {
								img->disp_rect.x += PAN_RATE * img->disp_rect.w;
								fix_rect(img);
								zoomed = 1;
							}
							zoomed = zoomed || img->disp_rect.h > img->scr_rect.h;
						}
					} else {
						img = g->img_focus;
						if (img->disp_rect.w > img->scr_rect.w) {
							img->disp_rect.x += PAN_RATE * img->disp_rect.w;
							fix_rect(img);
							zoomed = 1;
						}
						zoomed = zoomed || img->disp_rect.h > img->scr_rect.h;
					}
				}
				if (!zoomed) {
					try_move(LEFT);
				}
				break;
			case SDL_SCANCODE_UP:
				zoomed = 0;
				g->status = REDRAW;
				if (g->loading || !(mod_state & (KMOD_LCTRL | KMOD_RCTRL))) {
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i) {
							img = &g->img[i];
							if (img->disp_rect.h > img->scr_rect.h) {
								img->disp_rect.y += PAN_RATE * img->disp_rect.h;
								fix_rect(img);
								zoomed = 1;
							}
							zoomed = zoomed || img->disp_rect.w > img->scr_rect.w;
						}
					} else {
						img = g->img_focus;
						if (img->disp_rect.h > img->scr_rect.h) {
							img->disp_rect.y += PAN_RATE * img->disp_rect.h;
							fix_rect(img);
							zoomed = 1;
						}
						zoomed = zoomed || img->disp_rect.w > img->scr_rect.w;
					}
				}
				if (!zoomed) {
					try_move(LEFT);
				}
				break;

			case SDL_SCANCODE_MINUS:
				g->status = REDRAW;
				if (!(mod_state & (KMOD_LCTRL | KMOD_RCTRL))) {
					do_zoom(-KEY_ZOOM, SDL_FALSE);
				} else {
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i) {
							if (g->img[i].frames > 1)
								g->img[i].delay += 10;
						}
					} else {
						if (g->img_focus->frames > 1)
							g->img_focus->delay += 10;
					}
				}
				break;
			case SDL_SCANCODE_EQUALS:
				g->status = REDRAW;
				if (!(mod_state & (KMOD_LCTRL | KMOD_RCTRL))) {
					do_zoom(KEY_ZOOM, SDL_FALSE);
				} else {
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i) {
							if (g->img[i].frames > 1) {
								g->img[i].delay -= 10;
								g->img[i].delay = MAX(MIN_GIF_DELAY, g->img[i].delay);
							}
						}
					} else {
						if (g->img_focus->frames > 1) {
							g->img_focus->delay -= 10;
							g->img_focus->delay = MAX(MIN_GIF_DELAY, g->img_focus->delay);
						}
					}
				}
				break;
			default:
				;
			}

			break;

		case SDL_MOUSEMOTION:
			if (mouse_button_mask & SDL_BUTTON(SDL_BUTTON_LEFT)) {
				img = NULL;
				if (!g->img_focus) {
					for (int i=0; i<g->n_imgs; ++i) {
						img = &g->img[i];
						if (e.motion.xrel != 0 && img->disp_rect.w > img->scr_rect.w) {
							img->disp_rect.x += e.motion.xrel;
						}
						if (e.motion.yrel != 0 && img->disp_rect.h > img->scr_rect.h) {
							img->disp_rect.y += e.motion.yrel;
						}
						fix_rect(img);
					}
				} else {
					img = g->img_focus;
					if (e.motion.xrel != 0 && img->disp_rect.w > img->scr_rect.w) {
						img->disp_rect.x += e.motion.xrel;
					}
					if (e.motion.yrel != 0 && img->disp_rect.h > img->scr_rect.h) {
						img->disp_rect.y += e.motion.yrel;
					}
					fix_rect(img);
				}
			}
			g->status = REDRAW;

			SDL_ShowCursor(SDL_ENABLE);
			g->gui_timer = SDL_GetTicks();
			g->show_gui = 1;
			break;

		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			g->status = REDRAW;
			SDL_ShowCursor(SDL_ENABLE);
			g->gui_timer = SDL_GetTicks();
			g->show_gui = 1;
			break;

		case SDL_MOUSEWHEEL:
			g->status = REDRAW;
			if (e.wheel.direction == SDL_MOUSEWHEEL_NORMAL) {
				do_zoom(e.wheel.y*SCROLL_ZOOM, SDL_TRUE);
			} else {
				do_zoom(-e.wheel.y*SCROLL_ZOOM, SDL_TRUE);
			}
			break;

		case SDL_WINDOWEVENT: {
			g->status = REDRAW;
			int x, y;
			SDL_GetWindowSize(g->win, &x, &y);
			//SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "windowed %d %d %d %d\n", g->scr_w, g->scr_h, x, y);
			switch (e.window.event) {
			case SDL_WINDOWEVENT_RESIZED:
				g->scr_w = e.window.data1;
				g->scr_h = e.window.data2;
				break;
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				g->scr_w = e.window.data1;
				g->scr_h = e.window.data2;

				// TODO how/where to reset all the "subscreens" rects
				if (g->n_imgs == 1) {
					SET_MODE1_SCR_RECT();
				} else if (g->n_imgs == 2) {
					SET_MODE2_SCR_RECTS();
				} else if (g->n_imgs == 4) {
					SET_MODE4_SCR_RECTS();
				} else if (g->n_imgs == 8) {
					SET_MODE8_SCR_RECTS();
				}
				break;
			case SDL_WINDOWEVENT_EXPOSED:
				//SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "exposed event");
				//SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "windowed %d %d %d %d\n", g->scr_w, g->scr_h, x, y);
				break;
			}
		} break; // end WINDOWEVENTS
		case SDL_FINGERDOWN:
			puts("finger down");
			break;
		case SDL_FINGERUP:
			puts("finger up");
			break;
		case SDL_FINGERMOTION:
			puts("finger motion");
			break;
		case SDL_MULTIGESTURE:
			printf("multi motion\n");
			break;

		default: // all other event types
			break;
		}

		// TODO leave it here where it calls for every event
		// or put it back in mouse and key events?
		nk_sdl_handle_event(&e);
	}
	nk_input_end(g->ctx);

	return 0;
}

int handle_events()
{
	if (g->state & (NORMAL | VIEW_RESULTS))
		return handle_events_normally();

	if (IS_LIST_MODE())
		return handle_list_events();

	return handle_thumb_events();

}

