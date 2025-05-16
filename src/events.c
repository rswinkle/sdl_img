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

// TODO move elsewhere?  rename?
void discard_rotation(img_state* img)
{
	if (!g->orig_pix) {
		return;
	}

	free(img->pixels);  // free rotated pixels
	img->pixels = g->orig_pix;
	img->w = g->orig_w;
	img->h = g->orig_h;
	img->edited = NOT_EDITED;
	img->rotdegs = 0;
	g->orig_pix = NULL;
	g->orig_w = 0;
	g->orig_h = 0;
	g->status = REDRAW;
	for (int i=0; i<img->frames; ++i) {
		SDL_DestroyTexture(img->tex[i]);
	}
	if (!create_textures(img)) {
		cleanup(0, 1);
	}

	// TODO refactor this, rotate_img90 and do_rotate
	g->adj_img_rects = SDL_TRUE;
}


// From wiki: If the filter function returns 1 when called, then the event will be added to the internal queue. If it returns 0, then the event will be dropped from the queue, but the internal state will still be updated.
//
// There's a bug in SDL_RENDERER_SOFTWARE
// because I return 0 and it breaks completely like it's not updating the internal
// state anymore, complaining about an invalid SDL_Surface
//
// It works better with SDL_RENDERER ACCELERATED but is still broken
// It doesn't complain that anything is invalid but if you make the window bigger
// than the starting size it just clips the content to the original size

// TODO rename?
int handle_common_evts(void* userdata, SDL_Event* e)
{
	//int w, h;
	// TODO technically this could run on a separate thread so
	// anything in anything I read/write to in g should be atomic
	// or guarded...
	if (e->type == SDL_WINDOWEVENT) {
		g->status = REDRAW;
		switch (e->window.event) {
		case SDL_WINDOWEVENT_SIZE_CHANGED:
			g->scr_w = e->window.data1;
			g->scr_h = e->window.data2;
			SDL_Log("filter size change %d %d\n", g->scr_w, g->scr_h);

			g->progress_hovered = nk_false;
			g->needs_scr_rect_update = TRUE;
			g->adj_img_rects = TRUE;

			//set_show_gui(SDL_TRUE);
			if (g->fullscreen) {
				// always use the full screen (even if we're showing the GUI)
				// the images will be under the GUI in fullscreen mode
				g->scr_rect.x = 0;
				g->scr_rect.y = 0;
				g->scr_rect.w = g->scr_w;
				g->scr_rect.h = g->scr_h;
			} else {
				g->scr_rect.x = 0;
				g->scr_rect.y = g->gui_bar_ht;
				g->scr_rect.w = g->scr_w;

				if (g->show_infobar)
					g->scr_rect.h = g->scr_h - 2*g->gui_bar_ht;
				else
					g->scr_rect.h = g->scr_h - g->gui_bar_ht;
			}

		break;

		}
		// this breaks both renderers in different ways
		// return 0
	} else if (e->type == SDL_KEYUP) {
		int sc = e->key.keysym.scancode;
		switch (sc) {
		case SDL_SCANCODE_F11:
			g->status = REDRAW;
			g->fullscreen = !g->fullscreen;
			set_fullscreen();
			break;
		case SDL_SCANCODE_F:
			if (e->key.keysym.mod & KMOD_CTRL) {
				SDL_Log("filter CTRL+F\n");
				g->status = REDRAW;
				g->fullscreen = !g->fullscreen;
				set_fullscreen();
			}
			break;
		}
	}
	return 1;
}



int handle_fb_events(file_browser* fb, struct nk_context* ctx)
{
	SDL_Event e;
	int sym;
	int code, sort_timer;
	int ret = 0;
	//SDL_Keymod mod_state = SDL_GetModState();

	cvector_file* f = &fb->files;

	nk_input_begin(ctx);
	while (SDL_PollEvent(&e)) {
		// TODO edit menu/GUI as appropriate for list mode, see which
		// actions make sense or are worth supporting (re-evaluate if I
		// have some sort of preview)
		if (e.type == g->userevent) {
			code = e.user.code;
			switch (code) {
			case SORT_NAME:
				SDL_Log("Starting sort by name\n");
				sort_timer = SDL_GetTicks();
				fb_sort_name(fb);
				SDL_Log("Sort took %d\n", SDL_GetTicks()-sort_timer);
				break;
			case SORT_SIZE:
				SDL_Log("Starting sort by size\n");
				sort_timer = SDL_GetTicks();
				fb_sort_size(fb);
				SDL_Log("Sort took %d\n", SDL_GetTicks()-sort_timer);
				break;
			case SORT_MODIFIED:
				SDL_Log("Starting sort by modified\n");
				sort_timer = SDL_GetTicks();
				fb_sort_modified(fb);
				SDL_Log("Sort took %d\n", SDL_GetTicks()-sort_timer);
				break;
			default:
				SDL_Log("Unknown user event! %d", code);
			}
			continue;
		}
		switch (e.type) {
		case SDL_QUIT:
			// don't think I really need these since we'll be exiting anyway
			nk_input_end(ctx);
			return 1;
		case SDL_KEYUP:
			sym = e.key.keysym.sym;
			switch (sym) {
			case SDLK_ESCAPE:
				if (fb->is_search_results) {
					fb->text_buf[0] = 0;
					fb->text_len = 0;
					fb->is_search_results = FALSE;
					if (fb->selection >= 0) {
						fb->selection = fb->search_results.a[fb->selection];
					}
					fb->list_setscroll = TRUE;
				} else if (g->fullscreen) {
					g->fullscreen = 0;
					set_fullscreen();
				} else if (g->files.size) {
					g->state = g->old_state;
					set_show_gui(SDL_TRUE);
					SDL_SetWindowTitle(g->win, g->files.a[g->img[0].index].name);
				} else {
					// ESC from an initial startup with no files, just exit
					nk_input_end(ctx);
					return 1;
				}
				break;

			case SDLK_KP_ENTER:
			case SDLK_RETURN:
				if (fb->selection >= 0) {
					int sel = (fb->is_search_results) ? fb->search_results.a[fb->selection] : fb->selection;
					if (f->a[sel].size == -1) {
						my_switch_dir(f->a[sel].path);
					} else {
						strncpy(fb->file, f->a[sel].path, MAX_PATH_LEN);
						ret = 1;
					}
				}
				break;
			}
			break;

		case SDL_KEYDOWN:
			sym = e.key.keysym.sym;
			switch (sym) {
			case SDLK_k:
			case SDLK_j:
				// don't scroll while typing a search
				if (g->list_search_active) {
					break;
				}
				//else fall through
			case SDLK_UP:
			case SDLK_DOWN:
				// to prevent arrows from moving through the list and the text at the same time
				if (!fb->text_len || !g->list_search_active) {
					fb->selection += (sym == SDLK_DOWN || sym == SDLK_j) ? 1 : -1;
					if (fb->is_search_results && fb->search_results.size) {
						if (fb->selection < 0)
							fb->selection += fb->search_results.size;
						else
							fb->selection %= fb->search_results.size;
					} else {
						if (fb->selection < 0)
							fb->selection += f->size;
						else
							fb->selection %= f->size;
					}
					fb->list_setscroll = TRUE;
				}
				break;
			}
			break; // KEYDOWN
		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
			set_show_gui(SDL_TRUE);
			break;
		}
		nk_sdl_handle_event(&e);
	}
	nk_input_end(g->ctx);

	return ret;
}


void handle_mouse_selection(SDL_Keymod mod_state)
{
	int i;
	// Trying to make it work exactly like file browsers is
	// too much of a pain
	// You can do CTRL or SHIFT, or SHIFT and then CTRL but not SHIFT+CTRL
	if (mod_state & KMOD_CTRL) {
		if (!g->search_results.size) {
			cvec_push_i(&g->search_results, g->selection);
		} else {
			// Keep arbitrary selections sorted
			for (i=0; i<g->search_results.size; i++) {
				if (g->search_results.a[i] == g->selection) {
					cvec_erase_i(&g->search_results, i, i);
					break;
				} else if (g->search_results.a[i] > g->selection) {
					cvec_insert_i(&g->search_results, i, g->selection);
					break;
				}
			}
			if (i == g->search_results.size) {
				cvec_push_i(&g->search_results, g->selection);
			}
		}
		if (g->search_results.size) {
			g->state = THUMB_SEARCH | SEARCH_RESULTS;  // Need both
		}
	} else if (mod_state & KMOD_SHIFT) {
		int start = g->thumb_sel_end;
		int end = g->selection;
		if (g->selection < g->thumb_sel_end) {
			start = g->selection;
			end = g->thumb_sel_end;
		}
		cvec_clear_i(&g->search_results);
		for (i=start; i<=end; i++) {
			cvec_push_i(&g->search_results, i);
		}
		if (g->search_results.size) {
			g->state = THUMB_SEARCH | SEARCH_RESULTS;  // Need both
		}
	} else {
		cvec_clear_i(&g->search_results);
		g->thumb_sel_end = g->selection;
		g->state = THUMB_DFLT;
	}
}

int handle_thumb_events()
{
	SDL_Event e;
	int sym;
	SDL_Keymod mod_state = SDL_GetModState();

	int ctrl_down = mod_state & KMOD_CTRL;
	int shift_down = mod_state & KMOD_SHIFT;

	int mouse_x, mouse_y;
	u32 mouse_state = SDL_GetMouseState(&mouse_x, &mouse_y);
	char title_buf[STRBUF_SZ];
	int code;

	int orig_start_row = g->thumb_start_row;

	nk_input_begin(g->ctx);
	while (SDL_PollEvent(&e)) {
		if (e.type == g->userevent) {
			code = e.user.code;
			switch (code) {
			case LOAD_THUMBS:
				load_thumb_textures();
				break;
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
				// TODO really think and test state changes using ESC, thumb_sel/selection
				if (g->state & THUMB_DFLT) {
					g->state = NORMAL;
					g->thumb_start_row = 0;

					// if current image was removed/deleted need to load next image
					if (g->do_next) {
						try_move(RIGHT);
						g->do_next = nk_false;
						
					}
				} else {
					SDL_StopTextInput();
					g->state = THUMB_DFLT;
					g->thumb_sel_end = g->thumb_sel;
					g->is_thumb_visual_line = SDL_FALSE;

					g->search_results.size = 0;
					text_buf[0] = 0;
					text_len = 0;
				}
				g->status = REDRAW;
				break;
			case SDLK_c:
				// turn off VISUAL (or any other mode I add later)
				if (ctrl_down) {
					SDL_StopTextInput();
					g->state = THUMB_DFLT;
					g->is_thumb_visual_line = SDL_FALSE;

					// CTRL+C doesn't actualy exit search mode in vim but we do
					g->search_results.size = 0;
					text_buf[0] = 0;
					text_len = 0;
				}
				break;
			case SDLK_SLASH:
				g->state = THUMB_SEARCH;
				text_buf[0] = '/';
				text_buf[1] = 0;
				text_len = 1;
				g->search_results.size = 0;
				SDL_StartTextInput();
				break;
			case SDLK_v:
				if (g->state & THUMB_DFLT) {
					g->state = THUMB_VISUAL;
					g->thumb_sel_end = g->thumb_sel;

					// TODO visual block mode
					g->is_thumb_visual_line = shift_down;
				} else if (g->state & THUMB_VISUAL) {
					g->state = THUMB_DFLT;
					g->is_thumb_visual_line = SDL_FALSE;
				}
				g->status = REDRAW;
				break;
			case SDLK_g:
				if (g->state != THUMB_SEARCH) {
					if (shift_down) {
						g->thumb_start_row = g->files.size-1; // will get fixed at the bottom
						g->thumb_sel = g->files.size-1;
					} else {
						g->thumb_start_row = 0;;
						g->thumb_sel = 0;
					}
				}
				break;
			case SDLK_HOME:
				g->thumb_start_row = 0;;
				g->thumb_sel = 0;
				break;
			case SDLK_END:
				g->thumb_start_row = g->files.size-1; // will get fixed at the bottom
				g->thumb_sel = g->files.size-1;
				break;

			case SDLK_s:
				if (g->state != THUMB_SEARCH) {
					do_thumb_save(ctrl_down);
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
				if (g->state != THUMB_SEARCH) {
					// TODO Also support Delete key?
					do_thumb_rem_del(sym == SDLK_x && g->thumb_x_deletes, ctrl_down);
				}
				break;

			case SDLK_KP_ENTER:
			case SDLK_RETURN:
				// TODO why am I or'ing with THUMB_DFLT?
				if (g->state & (THUMB_DFLT | SEARCH_RESULTS)) {
					// Go to view results
					if (g->state & SEARCH_RESULTS && ctrl_down) {
						g->state |= NORMAL;

						// Just going to go to last result they were on
						// not necessarily the closest one
						//g->thumb_sel = g->search_results.a[g->cur_result];
						g->selection = g->cur_result;
					} else {
						g->state = NORMAL;
						g->selection = g->thumb_sel;

						// Only clear search when going back to normal mode
						g->search_results.size = 0;
						text_buf[0] = 0;
						text_len = 0;
					}

					g->adj_img_rects = SDL_TRUE;
					try_move(SELECTION);
				} else if (g->state == THUMB_SEARCH) {
					SDL_StopTextInput();
					// maybe give a parameter to switch between searching names and paths
					search_filenames(SDL_TRUE);
					if (g->search_results.size) {
						g->thumb_sel = g->search_results.a[0];
						// TODO think about this it's THUMB_SEARCH | SEARCH_RESULTS
						// THUMB_DFLT is not a general THUMB state and while in this mode
						// we still can't remove individual images only all the search results
	
						// also can't use the keyboard to navigate list and hit enter because
						// Enter just repeats the search

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
				if (ctrl_down) {
					g->thumb_rows += (sym == SDLK_DOWN || sym == SDLK_j) ? 1 : -1;
					if (g->thumb_rows < MIN_THUMB_ROWS) {
						g->thumb_rows = MIN_THUMB_ROWS;
					} else if (g->thumb_rows > MAX_THUMB_ROWS) {
						g->thumb_rows = MAX_THUMB_ROWS;
					} else {
						orig_start_row = g->thumb_start_row + 2; // trigger JUMP
					}
				} else if (g->state != THUMB_SEARCH) {
					g->thumb_sel += (sym == SDLK_DOWN || sym == SDLK_j) ? g->thumb_cols : -g->thumb_cols;
					fix_thumb_sel((sym == SDLK_DOWN || sym == SDLK_j) ? 1 : -1);
				}
				break;
			case SDLK_LEFT:
			case SDLK_RIGHT:
			case SDLK_h:
			case SDLK_l:
				if (ctrl_down) {
					g->thumb_cols += (sym == SDLK_LEFT || sym == SDLK_h) ? -1 : 1;
					if (g->thumb_cols < MIN_THUMB_COLS) {
						g->thumb_cols = MIN_THUMB_COLS;
					} else if (g->thumb_cols > MAX_THUMB_COLS) {
						g->thumb_cols = MAX_THUMB_COLS;
					} else {
						orig_start_row = g->thumb_start_row + 2; // trigger JUMP
					}
				} else {
					if (g->state != THUMB_SEARCH) {
						g->thumb_sel += (sym == SDLK_h || sym == SDLK_LEFT) ? -1 : 1;
						fix_thumb_sel((sym == SDLK_h || sym == SDLK_LEFT) ? -1 : 1);
					}
				}
				break;
			case SDLK_f:
			case SDLK_b:
				if (ctrl_down) {
					// TODO match vim behavior, doesn't jump a full page,
					// jumps so the top/bottom row is the first row below/above the page
					// so the amount depends on which row screen thumb_sel is on
					int rows = g->thumb_rows;
					g->thumb_start_row += (sym == SDLK_f) ? rows : -rows;
					int tmp = g->thumb_sel % g->thumb_cols;
					g->thumb_sel = g->thumb_start_row * g->thumb_cols + tmp;
					g->thumb_sel += (sym == SDLK_b) ? (rows-1)*g->thumb_cols : 0;
					fix_thumb_sel((sym == SDLK_f) ? 1 : -1);
				}
				break;
			case SDLK_PAGEUP:
			case SDLK_PAGEDOWN:
				int rows = g->thumb_rows;
				g->thumb_start_row += (sym == SDLK_PAGEDOWN) ? rows : -rows;
				int tmp = g->thumb_sel % g->thumb_cols;
				g->thumb_sel = g->thumb_start_row * g->thumb_cols + tmp;
				g->thumb_sel += (sym == SDLK_b) ? (rows-1)*g->thumb_cols : 0;
				fix_thumb_sel((sym == SDLK_PAGEDOWN) ? 1 : -1);
				break;
			case SDLK_n:
				if (g->state & SEARCH_RESULTS) {
					if (shift_down) {
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
				}
				break;
			case SDLK_BACKSPACE:
				if (text_len > 1)
					text_buf[--text_len] = 0;
				SDL_Log("text is \"%s\"\n", text_buf);
				break;
			}
			break;
		case SDL_MOUSEMOTION:
			set_show_gui(SDL_TRUE);

			// TODO
			// Have to think about this, best way to do drag select while still supporting
			// CTRL and SHIFT selection modifiers, and other existing mouse/selection
			// behaviors and edge cases and scrolling

			/*
			if (SDL_BUTTON_LMASK & mouse_state) {
				if (g->state & THUMB_DFLT) {
					g->state = THUMB_VISUAL;
					g->thumb_sel_end = g->thumb_sel;
					g->status = REDRAW;
				}
			}
			*/
			break;
		case SDL_MOUSEBUTTONDOWN:
			set_show_gui(SDL_TRUE);
			// TODO
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
					g->state = NORMAL;
					g->thumb_start_row = 0;
					try_move(SELECTION);
				} else {
					handle_mouse_selection(mod_state);

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
			}
			break;

		case SDL_TEXTINPUT:
			// could probably just do text_buf[text_len++] = e.text.text[0]
			// since I only handle ascii
			if (g->state == THUMB_SEARCH && text_len < STRBUF_SZ-1) {
				strcat(text_buf, e.text.text);
				text_len += strlen(e.text.text);
				SDL_Log("text is \"%s\" \"%s\" %d %d\n", text_buf, composition, cursor, selection_len);
				//SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "text is \"%s\" \"%s\" %d %d\n", text, composition, cursor, selection_len);
			}
			break;

		case SDL_TEXTEDITING:
			if (g->state == THUMB_SEARCH) {
				SDL_Log("received edit \"%s\"\n", e.edit.text);
				//SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "recieved edit \"%s\"\n", e.edit.text);
				composition = e.edit.text;
				cursor = e.edit.start;
				selection_len = e.edit.length;
			}
			break;

		default:
			break;
		}

		// TODO leave it here where it calls for every event
		// or put it back in mouse and key events?
		nk_sdl_handle_event(&e);
	}
	nk_input_end(g->ctx);

	// TODO think about this logic
	if (g->thumb_start_row*g->thumb_cols + g->thumb_rows*g->thumb_cols >= g->files.size+g->thumb_cols) {
		g->thumb_start_row = g->files.size / g->thumb_cols - g->thumb_rows + !!(g->files.size % g->thumb_cols);
	}
	if (g->thumb_start_row < 0) {
		g->thumb_start_row = 0;
	}

	/*
	// can happen while thumbs are being generated/loaded
	if (!g->thumbs.a[g->thumb_sel].tex) {
		if (!g->thumb_sel) {
			for (; !g->thumbs.a[g->thumb_sel].tex && g->thumb_sel<g->files.size-1; ++g->thumb_sel);

			// no valid thumbs, probably just started generating, stay on 0 so the title doesn't flicker
			if (g->thumb_sel == g->files.size-1)
				g->thumb_sel = 0;
		} else {
			for (; !g->thumbs.a[g->thumb_sel].tex && g->thumb_sel>0; --g->thumb_sel);
		}
		// No valid thumbs found, turn off visual
		// TODO also prevent thumbmode in the first place if there are no valid thumbs?
		if (!g->thumbs.a[g->thumb_sel].tex) {
			g->state = THUMB_DFLT;
		}
	}
	*/

	if (g->thumb_sel < g->thumb_start_row*g->thumb_cols) {
		g->thumb_start_row = g->thumb_sel / g->thumb_cols;
	} else if (g->thumb_sel >= g->thumb_start_row*g->thumb_cols + g->thumb_rows*g->thumb_cols) {
		g->thumb_start_row = g->thumb_sel / g->thumb_cols - g->thumb_rows + 1;
	}

	if (g->thumb_start_row != orig_start_row) {
		int action = JUMP;
		int diff = g->thumb_start_row - orig_start_row;
		// UP and DOWN refers to visually moving up and down
		if (diff == 1) action = DOWN;
		if (diff == -1) action = UP;
		SDL_LogDebugApp("signaling a %d to jit_thumbs\n", action);
		SDL_LockMutex(g->jit_thumb_mtx);
		g->jit_thumb_flag = action;
		SDL_CondSignal(g->jit_thumb_cnd);
		SDL_UnlockMutex(g->jit_thumb_mtx);
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
	int sym, code;
	int sort_timer;

	SDL_Keymod mod_state = SDL_GetModState();
	int ctrl_down = mod_state & KMOD_CTRL;
	int shift_down = mod_state & KMOD_SHIFT;

	nk_input_begin(g->ctx);
	while (SDL_PollEvent(&e)) {
		// Oops, got rid of all controls when I got rid of the Menu, still need these sorting events
		// for the column header buttons
		if (e.type == g->userevent) {
			code = e.user.code;
			switch (code) {
			case SORT_NAME:
				SDL_Log("Starting sort by name\n");
				sort_timer = SDL_GetTicks();
				if (!e.user.data1) {
					if (g->sorted_state != NAME_UP) {
						do_sort(filename_cmp_lt);
						g->sorted_state = NAME_UP;
					} else {
						do_sort(filename_cmp_gt);
						g->sorted_state = NAME_DOWN;
					}
				} else {
					if (g->lib_sorted_state != NAME_UP) {
						do_lib_sort(filename_cmp_lt);
						g->lib_sorted_state = NAME_UP;
					} else {
						do_lib_sort(filename_cmp_gt);
						g->lib_sorted_state = NAME_DOWN;
					}
				}
				SDL_Log("Sort took %d\n", SDL_GetTicks()-sort_timer);
				break;
			case SORT_SIZE:
				SDL_Log("Starting sort by size\n");
				sort_timer = SDL_GetTicks();
				if (!e.user.data1) {
					if (g->sorted_state != SIZE_UP) {
						do_sort(filesize_cmp_lt);
						g->sorted_state = SIZE_UP;
					} else {
						do_sort(filesize_cmp_gt);
						g->sorted_state = SIZE_DOWN;
					}
				} else {
					if (g->lib_sorted_state != SIZE_UP) {
						do_lib_sort(filesize_cmp_lt);
						g->lib_sorted_state = SIZE_UP;
					} else {
						do_lib_sort(filesize_cmp_gt);
						g->lib_sorted_state = SIZE_DOWN;
					}
				}
				SDL_Log("Sort took %d\n", SDL_GetTicks()-sort_timer);
				break;
			case SORT_MODIFIED:
				SDL_Log("Starting sort by modified\n");
				sort_timer = SDL_GetTicks();
				if (!e.user.data1) {
					if (g->sorted_state != MODIFIED_UP) {
						do_sort(filemodified_cmp_lt);
						g->sorted_state = MODIFIED_UP;
					} else {
						do_sort(filemodified_cmp_gt);
						g->sorted_state = MODIFIED_DOWN;
					}
				} else {
					if (g->lib_sorted_state != MODIFIED_UP) {
						do_lib_sort(filemodified_cmp_lt);
						g->lib_sorted_state = MODIFIED_UP;
					} else {
						do_lib_sort(filemodified_cmp_gt);
						g->lib_sorted_state = MODIFIED_DOWN;
					}
				}
				SDL_Log("Sort took %d\n", SDL_GetTicks()-sort_timer);
				break;
			case SAVE_IMG:
				do_sql_save_idx(SDL_FALSE, g->cur_playlist_id, g->selection);
				break;
			case UNSAVE_IMG:
				do_sql_save_idx(SDL_TRUE, g->cur_playlist_id, g->selection);
				break;
			case SAVE_ALL:
				do_sql_save_all(g->list_view);
				break;
			}
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
				if (g->is_new_renaming > 0) {
					// Stay on whatever was selected before
					//if (g->is_new_renaming == NEW_PLIST) {
					//	// switch to cur
					//	g->selected_plist = -1;
					//	g->lib_selected = nk_false;
					//	g->cur_selected = nk_true;
					//	g->list_view = &g->files;
					//}

					// for renaming, stay on that playlist otherwise
					// we're in a weird state

					// set to -2 to signal to call nk_edit_unfocus()
					g->is_new_renaming = -2;
				} else if (g->state & SEARCH_RESULTS) {
					if (g->list_view == &g->files) {
						// if nothing was selected among search results set back
						// to current image
						if (g->selection < 0) {
							// TODO what if current image was still set to overall list index?
							g->selection = g->img[0].index;
						} else {
							// convert selection
							g->selection = g->search_results.a[g->selection];
						}

						// if they went to view results image index is no longer files index
						// and need to convert back
						if (g->img[0].fullpath != g->files.a[g->img[0].index].path) {
							g->img[0].index = g->search_results.a[g->img[0].index];
						}

						g->list_setscroll = SDL_TRUE;
					} else {
						if (g->selection >= 0) {
							g->selection = g->search_results.a[g->selection];
						}
						g->list_setscroll = SDL_TRUE;
					}
					g->state = LIST_DFLT;

					// redundant since we clear before doing the search atm
					g->search_results.size = 0;
				} else {
					// NOTE UI decision: don't move to selection if they hit ESC, only
					// if they hit Enter (this is also how it works in thumb mode)
					g->state = NORMAL;

					// for sql functions like get_img_playlists()
					g->list_view = &g->files;

						// set on enter to list mode?
					//g->cur_selected = SDL_TRUE;
					//g->lib_selected = SDL_FALSE;
					set_show_gui(SDL_TRUE);
				}

				// always clear search field
				text_buf[0] = 0;
				//memset(text_buf, 0, text_len+1);
				text_len = 0;

				break;

			// TODO removal and deletion in list mode?
			case SDLK_BACKSPACE:
			case SDLK_r:
			case SDLK_x:
				// If I do use g->do_next like thumb mode and it'll affect other code
				// if I try to avoid unnecessary loading of current image (ie in 4-mode
				// but I remove 1 of the 4, can't just check img[0])
				break;

			// switch to normal mode on that image
			case SDLK_RETURN:
			case SDLK_KP_ENTER:

				// TODO this is an ugly hack
				if (g->is_new_renaming == -1) {
					g->is_new_renaming = 0;
				} else if (!g->is_new_renaming && g->cur_selected && g->selection >= 0) {
					if (g->state & SEARCH_RESULTS) {
						g->state |= NORMAL;
					} else {
						// TODO avoid unnecessary loads for current image
						g->state = NORMAL;
					}
					// Same as switching from thumbmode
					set_show_gui(SDL_TRUE);
					try_move(SELECTION);
				}
				break;
			case SDLK_s:
				if (g->selection >= 0) {
					if (shift_down && ctrl_down) {
						do_sql_save_all(g->list_view);
					} else {
						do_sql_save_idx(ctrl_down, g->cur_playlist_id, g->selection);
					}
					get_img_playlists(g->selection);
				}
				break;
			}
			break;

		case SDL_KEYDOWN:
			sym = e.key.keysym.sym;
			switch (sym) {
			case SDLK_k:
			case SDLK_j:
				// don't scroll while typing a search or creating/renaming a playlist
				if (g->list_search_active || g->is_new_renaming) {
					break;
				}
				//else fall through
			case SDLK_UP:
			case SDLK_DOWN:
				// to prevent arrows from moving through the list and the text at the same time
				if (!text_len || !g->list_search_active) {
					g->selection += (sym == SDLK_DOWN || sym == SDLK_j) ? 1 : -1;
					if ((g->state & SEARCH_RESULTS) && g->search_results.size) {
						if (g->selection < 0)
							g->selection += g->search_results.size;
						else
							g->selection %= g->search_results.size;
					} else {
						if (g->selection < 0)
							g->selection += g->list_view->size;
						else
							g->selection %= g->list_view->size;
					}
					g->list_setscroll = SDL_TRUE;
					if (g->preview.tex) {
						SDL_DestroyTexture(g->preview.tex);
						g->preview.tex = NULL;
					}
					get_img_playlists(g->selection);
				}
				break;
			}
		case SDL_MOUSEMOTION:
			set_show_gui(SDL_TRUE);
			break;

		// all other event types
		default:
			break;
		}

		nk_sdl_handle_event(&e);
	}
	nk_input_end(g->ctx);

	return 0;
}

int handle_scanning_events()
{
	SDL_Event e;
	int sym;
	int code;
	//SDL_Keymod mod_state = SDL_GetModState();
	
	// TODO make sure loading is done before setting state to NORMAL
	// can either do the loading mtx lock here or load and setup manually
	SDL_LockMutex(g->scanning_mtx);
	if (g->done_scanning) {
		SDL_LogDebugApp("done scanning\n");

		g->status = REDRAW;
		if (g->files.size) {
			SDL_LogDebugApp("files.size != 0\n");
			//SDL_LockMutex(g->img_loading_mtx);
			if (g->done_loading) {
				SDL_LogDebugApp("done loading\n");

				g->state = NORMAL;
				SDL_LogDebugApp("switching to NORMAL mode\n");
			}
			//SDL_UnlockMutex(g->img_loading_mtx);
		} else {
			SDL_Log("Switching to file selection because scanning sources discovered 0 images\n");
			g->state = FILE_SELECTION;
			g->done_loading = 0;
			reset_file_browser(&g->filebrowser, NULL);
			g->filebrowser.selection = -1; // default to no selection

			// If we tried to open an empty playlist need to clear this
			g->open_playlist = SDL_FALSE;

			// If they're in playlistdir keep settings the same
			if (strcmp(g->filebrowser.dir, g->playlistdir)) {
				g->open_single = SDL_FALSE;
				g->open_list = SDL_FALSE;
				g->open_recursive = SDL_FALSE;
			} else {
				g->open_list = SDL_TRUE;  // should still be true but for clarity
				g->filebrowser.ignore_exts = SDL_TRUE; // was reset by reset_file_browser
			}

			g->is_open_new = SDL_TRUE;
		}

		//SDL_UnlockMutex(g->scanning_mtx);
		//return 0;
	}
	SDL_UnlockMutex(g->scanning_mtx);

	nk_input_begin(g->ctx);
	while (SDL_PollEvent(&e)) {
		if (e.type == g->userevent) {
			code = e.user.code;
			switch (code) {
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
				if (g->fullscreen) {
					g->fullscreen = 0;
					set_fullscreen();
				} else {
					nk_input_end(g->ctx);
					return 1;
				}
				break;
			}
			break;
		}
	}
	nk_input_end(g->ctx);
	return 0;
}

// TODO macro?
void mode_focus_change(intptr_t mode, int ctrl_down, char* title_buf)
{
	// modes are 1,2,4,8
	int is_mode = (mode == 1 || mode == 2 || mode == 4 || mode == 8) ? 1 : 0;

	// parameter mode is used as mode or focus selection (1-8)

	if (is_mode && !g->loading && ctrl_down) {
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

int handle_popup_events()
{
	SDL_Event e;
	int sc;

	SDL_Keymod mod_state = SDL_GetModState();

	int ticks = SDL_GetTicks();

	if (g->slideshow) {
		// don't advance slideshow while a popup is active
		g->slide_timer = ticks;
	}

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
			case ROT360:
				// TODO
				rotate_img((g->n_imgs == 1) ? &g->img[0] : g->img_focus);
				break;
			case SELECT_DIR:
			case SELECT_FILE:
				do_file_select(code == SELECT_DIR, (intptr_t)e.user.data1, (const char**)e.user.data2);
				break;
			case FONT_CHANGE:
				setup_font(NULL, g->font_size);
				break;
			default:
				SDL_Log("Shouldn't get any other user event in popup mode!\n");
			}
		}
		switch (e.type) {
		case SDL_QUIT:
			//nk_input_end(g->ctx); // TODO need these?
			return 1;
		case SDL_KEYUP:
			sc = e.key.keysym.scancode;
			switch (sc) {
			case SDL_SCANCODE_ESCAPE:
				if (g->state & ROTATE) {
					// ESC discards all changes, including previewed changes which means
					// we may have to clean up/free
					discard_rotation((g->n_imgs == 1) ? &g->img[0] : g->img_focus);
				}
				// safe to just always set these to false, only 1 "popup" at a time
				g->state &= ~POPUP_MASK;
				break;
			}
			break;
		case SDL_KEYDOWN:
			sc = e.key.keysym.scancode;
			switch (sc) {
				default: ;
			}
			break;
		} // end switch event type

		// TODO leave it here where it calls for every event
		// or put it back in mouse and key events?
		nk_sdl_handle_event(&e);
	}
	nk_input_end(g->ctx);

	return 0;
}

void handle_loading(void);

int handle_events_normally()
{
	SDL_Event e;
	int sc;
	int zoomed;
	char title_buf[STRBUF_SZ];
	img_state* img;
	int scroll_y;

	// eat all escapes this frame after copy dialog ended with "no"
	int copy_escape = SDL_FALSE;

	SDL_Keymod mod_state = SDL_GetModState();
	int ctrl_down = mod_state & KMOD_CTRL;
	int shift_down = mod_state & KMOD_SHIFT;

	// use space to move to next image(s) even if zoomed in, ie during slideshow
	SDL_Event space;
	space.type = SDL_KEYDOWN;
	space.key.keysym.scancode = SDL_SCANCODE_SPACE;
	// I only set this to clear valgrind errors of jumps in
	// nk_sdl_handle_event based uninitialized values
	space.key.keysym.sym = SDLK_SPACE;


	// Use if to push any user events
	SDL_Event user_event = { .type = g->userevent };

	int ticks = SDL_GetTicks();

	//handle_loading();
	/*
	SDL_LockMutex(g->img_loading_mtx);
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
			for (int i=g->n_imgs; i<g->done_loading; ++i) {
				create_textures(&g->img[i]);
			}
			// TODO duplication in do_mode_change
			g->needs_scr_rect_update = SDL_TRUE;
			g->adj_img_rects = SDL_TRUE;

			g->n_imgs = g->done_loading;
		}
		g->done_loading = 0;
		g->status = REDRAW;
		if (g->slideshow && IS_NORMAL())
			g->slide_timer =  SDL_GetTicks();
	}
	SDL_UnlockMutex(g->img_loading_mtx);
	
	if (!g->loading) {
		if (g->needs_scr_rect_update) {
			//SDL_Log("update after loading mode = %d %d", g->n_imgs, g->adj_img_rects);
			switch (g->n_imgs) {
			case 1: SET_MODE1_SCR_RECT(); break;
			case 2: SET_MODE2_SCR_RECTS(); break;
			case 4: SET_MODE4_SCR_RECTS(); break;
			case 8: SET_MODE8_SCR_RECTS(); break;
			}
			g->needs_scr_rect_update = SDL_FALSE;
		}
		if (g->adj_img_rects) {
			for (int i=0; i<g->n_imgs; ++i) {
				set_rect_bestfit(&g->img[i], g->fullscreen | g->slideshow | g->fill_mode);
			}
			g->adj_img_rects = SDL_FALSE;
		}
	}
	*/

	// TODO but in handle_loading?
	if (g->slideshow) {
		if (!g->loading && ticks - g->slide_timer > g->slideshow) {
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
	
	int done_rotate = SDL_FALSE;
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
			case REMOVE_BAD:
				remove_bad_paths();
				break;
			case SHUFFLE:
				do_shuffle();
				break;

				// TODO support toggling between ascending/descending in normal mode?
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

			case SAVE_IMG:
				do_sql_save(SDL_FALSE, g->cur_playlist_id);
				break;
			case UNSAVE_IMG:
				do_sql_save(SDL_TRUE, g->cur_playlist_id);
				break;
			case SAVE_ALL:
				do_sql_save_all(&g->files);
				break;
			case REMOVE_IMG:
				do_remove(&space);
				break;
			case DELETE_IMG:
				do_delete(&space);
				break;
			case OPEN_FILE_NEW:
			case OPEN_FILE_MORE:
				do_file_open(code == OPEN_FILE_NEW);
				break;
			case ROT360:
				// handle last rotation when hitting OK without preview first
				// in arbitrary rotation
				rotate_img((g->n_imgs == 1) ? &g->img[0] : g->img_focus);
				break;
			default:
				SDL_Log("Unknown or unhandled user event!'\n");
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
				// NOTE the order of else if's is purposeful, since you can have a
				// slideshow while img_focus and it makes sense to end the former before
				// "exiting" the latter, same with the rest
				if (!g->img_focus && !copy_escape && !g->fullscreen && !g->slideshow && g->state == NORMAL) {
					//nk_input_end(g->ctx);
					return 1;
				} else if (g->slideshow) {
					SDL_Log("Ending slideshow");
					g->slideshow = 0;
				} else if (g->img_focus) {
					g->img_focus = NULL;
					if (g->n_imgs > 1) {
						g->progress_hovered = SDL_FALSE;
					}
					if (IS_VIEW_RESULTS())
						SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->search_results.a[g->img[0].index]].path, title_buf));
					else
						SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img[0].index].path, title_buf));

				} else if (g->fullscreen) {
					g->fullscreen = 0;
					set_fullscreen();
				} else if (IS_VIEW_RESULTS()) {
					g->state ^= NORMAL;

					// selection is used in listmode results, = index in results
					g->selection = g->img[0].index;
					g->list_setscroll = SDL_TRUE;

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

						// We need this here because we could jump to a result where
						// we haven't generated (or loaded) the thumbs around it
						// TODO better place? could it even finish fast enough to
						// push the event while we're still here processing events
						// in normal mode?
						SDL_LogDebugApp("signaling a JUMP to jit_thumbs\n");
						SDL_LockMutex(g->jit_thumb_mtx);
						g->jit_thumb_flag = JUMP;
						SDL_CondSignal(g->jit_thumb_cnd);
						SDL_UnlockMutex(g->jit_thumb_mtx);
					}

					set_show_gui(SDL_TRUE);
				} else if (IS_THUMB_MODE() || IS_LIST_MODE()) {
					// TODO this can't be reached! ... right?
					g->state = NORMAL;
				}
				break;

			case SDL_SCANCODE_DELETE:
				do_delete(&space);
				break;

			case SDL_SCANCODE_O:
				if (ctrl_down) {
					user_event.user.code = OPEN_FILE_MORE;
					SDL_PushEvent(&user_event);
				} else {
					user_event.user.code = OPEN_FILE_NEW;
					SDL_PushEvent(&user_event);
				}
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

			case SDL_SCANCODE_0:
				g->img_focus = NULL;
				if (g->n_imgs > 1) {
					g->progress_hovered = SDL_FALSE;
				}
				if (IS_VIEW_RESULTS())
					SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->search_results.a[g->img[0].index]].path, title_buf));
				else
					SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img[0].index].path, title_buf));
				break;
			case SDL_SCANCODE_1:
				mode_focus_change(MODE1, ctrl_down, title_buf);
				break;
			case SDL_SCANCODE_2:
				mode_focus_change(MODE2, ctrl_down, title_buf);
				break;
			case SDL_SCANCODE_3:
				g->status = REDRAW;
				mode_focus_change(3, ctrl_down, title_buf);
				break;
			case SDL_SCANCODE_4:
				mode_focus_change(MODE4, ctrl_down, title_buf);
				break;
			case SDL_SCANCODE_5:
				g->status = REDRAW;
				mode_focus_change(5, ctrl_down, title_buf);
				break;
			case SDL_SCANCODE_6:
				g->status = REDRAW;
				mode_focus_change(6, ctrl_down, title_buf);
				break;
			case SDL_SCANCODE_7:
				g->status = REDRAW;
				mode_focus_change(7, ctrl_down, title_buf);
				break;
			case SDL_SCANCODE_8:
				mode_focus_change(MODE8, ctrl_down, title_buf);
				break;

			case SDL_SCANCODE_A:
				do_actual_size();
				break;

			case SDL_SCANCODE_M:
				do_shuffle();
				break;

			case SDL_SCANCODE_N:
				if (ctrl_down) {
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
				if (ctrl_down) {
					do_listmode();
				}
				break;

			case SDL_SCANCODE_U:
				if (ctrl_down) {
					do_thumbmode();
				} else {
					// TODO GUI for this?  starts thumb thread in the background
					// without switching to thumb mode
					generate_thumbs(SDL_FALSE);
				}
				break;

			case SDL_SCANCODE_C:
				if (ctrl_down) {
					// TODO maybe just flush events here and return 0 so
					// no input for the current frame after CTRL+V? can I do
					// that without breaking the GUI?
					copy_escape = do_copy();
				} else {
					remove_bad_paths();
				}
			break;

			case SDL_SCANCODE_S:
				if (shift_down && ctrl_down) {
					do_sql_save_all(&g->files);
				} else {
					do_sql_save(ctrl_down, g->cur_playlist_id);
				}
			break;

			case SDL_SCANCODE_H:
			case SDL_SCANCODE_V:
				do_flip(sc == SDL_SCANCODE_V);
			break;

			case SDL_SCANCODE_L:
			case SDL_SCANCODE_R:
				if (!done_rotate) {
					if (ctrl_down) {
						do_rotate(sc == SDL_SCANCODE_L, SDL_FALSE);
					} else {
						do_rotate(sc == SDL_SCANCODE_L, SDL_TRUE);
					}
					done_rotate = SDL_TRUE;
				}
				break;

			case SDL_SCANCODE_BACKSPACE:
				do_remove(&space);
				break;

			case SDL_SCANCODE_F: {
				g->status = REDRAW;
				if (!ctrl_down) {
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
				g->selection = 0;
				try_move(SELECTION);
				break;

			// not a lot of use for this since you can just hit HOME and then back one
			// but users will probably expect it to work
			case SDL_SCANCODE_END:
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
				if (ctrl_down) {
					try_move(LEFT);
				} else {
					try_move(RIGHT);
				}
				break;

			// TODO merge RIGHT/DOWN and LEFT/UP?
			case SDL_SCANCODE_RIGHT:
				zoomed = SDL_FALSE;
				g->status = REDRAW;
				if (!ctrl_down) {
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i) {
							img = &g->img[i];
							if (img->disp_rect.w > img->scr_rect.w) {
								img->disp_rect.x -= PAN_RATE * img->disp_rect.w;
								fix_rect(img);
								zoomed = SDL_TRUE;
							}
							zoomed = zoomed || img->disp_rect.h > img->scr_rect.h;
						}
					} else {
						img = g->img_focus;
						if (img->disp_rect.w > img->scr_rect.w) {
							img->disp_rect.x -= PAN_RATE * img->disp_rect.w;
							fix_rect(img);
							zoomed = SDL_TRUE;
						}
						zoomed = zoomed || img->disp_rect.h > img->scr_rect.h;
					}
				}
				if (!zoomed) {
					try_move(RIGHT);
				}
				break;
			case SDL_SCANCODE_DOWN:
				zoomed = SDL_FALSE;
				g->status = REDRAW;
				if (!ctrl_down) {
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i) {
							img = &g->img[i];
							if (img->disp_rect.h > img->scr_rect.h) {
								img->disp_rect.y -= PAN_RATE * img->disp_rect.h;
								fix_rect(img);
								zoomed = SDL_TRUE;
							}
							zoomed = zoomed || img->disp_rect.w > img->scr_rect.w;
						}
					} else {
						img = g->img_focus;
						if (img->disp_rect.h > img->scr_rect.h) {
							img->disp_rect.y -= PAN_RATE * img->disp_rect.h;
							fix_rect(img);
							zoomed = SDL_TRUE;
						}
						zoomed = zoomed || img->disp_rect.w > img->scr_rect.w;
					}
				}
				if (!zoomed) {
					try_move(RIGHT);
				}
				break;

			case SDL_SCANCODE_LEFT:
				zoomed = SDL_FALSE;
				g->status = REDRAW;
				if (!ctrl_down) {
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i) {
							img = &g->img[i];
							if (img->disp_rect.w > img->scr_rect.w) {
								img->disp_rect.x += PAN_RATE * img->disp_rect.w;
								fix_rect(img);
								zoomed = SDL_TRUE;
							}
							zoomed = zoomed || img->disp_rect.h > img->scr_rect.h;
						}
					} else {
						img = g->img_focus;
						if (img->disp_rect.w > img->scr_rect.w) {
							img->disp_rect.x += PAN_RATE * img->disp_rect.w;
							fix_rect(img);
							zoomed = SDL_TRUE;
						}
						zoomed = zoomed || img->disp_rect.h > img->scr_rect.h;
					}
				}
				if (!zoomed) {
					try_move(LEFT);
				}
				break;
			case SDL_SCANCODE_UP:
				zoomed = SDL_FALSE;
				g->status = REDRAW;
				if (!ctrl_down) {
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i) {
							img = &g->img[i];
							if (img->disp_rect.h > img->scr_rect.h) {
								img->disp_rect.y += PAN_RATE * img->disp_rect.h;
								fix_rect(img);
								zoomed = SDL_TRUE;
							}
							zoomed = zoomed || img->disp_rect.w > img->scr_rect.w;
						}
					} else {
						img = g->img_focus;
						if (img->disp_rect.h > img->scr_rect.h) {
							img->disp_rect.y += PAN_RATE * img->disp_rect.h;
							fix_rect(img);
							zoomed = SDL_TRUE;
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
				if (!ctrl_down) {
					do_zoom(-KEY_ZOOM, SDL_FALSE);
				} else {
					// Should MINUS slow it down or decrease the delay amount?
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i) {
							if (g->img[i].frames > 1)
								for (int j=0; j<g->img[i].frames; ++j) {
									g->img[i].delays[j] += 10;
								}
						}
					} else {
						if (g->img_focus->frames > 1) {
							for (int j=0; j<g->img_focus->frames; ++j) {
								g->img_focus->delays[j] += 10;
							}
						}
					}
				}
				break;
			case SDL_SCANCODE_EQUALS:
				g->status = REDRAW;
				if (!ctrl_down) {
					do_zoom(KEY_ZOOM, SDL_FALSE);
				} else {
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i) {
							if (g->img[i].frames > 1) {
								for (int j=0; j<g->img[i].frames; ++j) {
									g->img[i].delays[j] -= 10;
									g->img[i].delays[j] = MAX(MIN_GIF_DELAY, g->img[i].delays[j]);
								}
							}
						}
					} else {
						if (g->img_focus->frames > 1) {
							for (int j=0; j<g->img_focus->frames; ++j) {
								g->img_focus->delays[j] -= 10;
								g->img_focus->delays[j] = MAX(MIN_GIF_DELAY, g->img_focus->delays[j]);
							}
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
				do_pan(e.motion.xrel, e.motion.yrel);
			}

			set_show_gui(SDL_TRUE);
			break;

		case SDL_MOUSEBUTTONDOWN:
			switch (e.button.button) {
			case SDL_BUTTON_RIGHT:
				// TODO this is handle_events_normallly(), why are we
				// checking g->state & NORMAL? state transition issue I'm sure
				if (g->state & NORMAL && !(g->state & PLAYLIST_CONTEXT)) {
					if (g->n_imgs == 1) {
						get_img_playlists(g->img[0].index);
						g->state |= PLAYLIST_CONTEXT;
					}
					break;
				}
			case SDL_BUTTON_LEFT:
			case SDL_BUTTON_MIDDLE:
				break;
			}

			if (e.button.which == SDL_TOUCH_MOUSEID) {
				SDL_LogDebugApp("mouse button touch down\n");
			}
			set_show_gui(SDL_TRUE);
			break;
		case SDL_MOUSEBUTTONUP:
			if (e.button.which == SDL_TOUCH_MOUSEID) {
				SDL_LogDebugApp("mouse button touch up\n");
			}
			set_show_gui(SDL_TRUE);
			break;

		case SDL_MOUSEWHEEL:
			g->status = REDRAW;
			scroll_y = e.wheel.y;
			if (e.wheel.direction != SDL_MOUSEWHEEL_NORMAL)
				scroll_y = -scroll_y;

			int amt = scroll_y*10;
			if (!g->progress_hovered) {
				if (!ctrl_down) {
					do_zoom(scroll_y*SCROLL_ZOOM, SDL_TRUE);
				} else if (!g->img_focus) {
					for (int i=0; i<g->n_imgs; ++i) {
						if (g->img[i].frames > 1) {
							for (int j=0; j<g->img[i].frames; ++j) {
								g->img[i].delays[j] -= amt;
								g->img[i].delays[j] = MAX(MIN_GIF_DELAY, g->img[i].delays[j]);
							}
							SDL_LogDebugApp("new delays[0]: %d\n", g->img[i].delays[0]);
						}
					}
				} else {
					if (g->img_focus->frames > 1) {
						for (int j=0; j<g->img_focus->frames; ++j) {
							g->img_focus->delays[j] -= amt;
							g->img_focus->delays[j] = MAX(MIN_GIF_DELAY, g->img_focus->delays[j]);
						}
					}
				}
			} else {
				int f;
				img = g->img_focus;
				if (g->n_imgs == 1) {
					img = &g->img[0];
				}

				f = img->frame_i;
				f += scroll_y;
				if (f < 0) {
					f += img->frames;
				}
				f %= img->frames;

				img->frame_i = f;
			}
			break;

		case SDL_FINGERDOWN:
			SDL_LogDebugApp("finger down\n");
			SDL_LogDebugApp("x y %f %f\n", e.tfinger.x, e.tfinger.y);
			break;
		case SDL_FINGERUP:
			SDL_LogDebugApp("finger up\n");
			SDL_LogDebugApp("x y %f %f\n", e.tfinger.x, e.tfinger.y);
			break;
		case SDL_FINGERMOTION:
			SDL_LogDebugApp("finger motion\n");
			// documentation says dx and dy are normalized (-1, 1) but apparently they're not.
			// Even if they were, it doesn't clarify if it's normalized in screen space or window space.
			SDL_LogDebugApp("dx dy %f %f\n", e.tfinger.dx, e.tfinger.dy);
			g->status = REDRAW;
			do_pan((int)(e.tfinger.dx+0.99), (int)(e.tfinger.dy+0.99));
			break;
		case SDL_MULTIGESTURE: {
			SDL_LogDebugApp("multi motion\n");
			SDL_LogDebugApp("theta = %f dist = %f\n", e.mgesture.dTheta, e.mgesture.dDist);
			SDL_LogDebugApp("x y = %f %f\n", e.mgesture.x, e.mgesture.y);
			SDL_LogDebugApp("numfingers = %d\n", e.mgesture.numFingers);
			g->status = REDRAW;
			do_zoom(PINCH_ZOOM * e.mgesture.dDist, SDL_FALSE);
		} break;

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


void handle_loading(void)
{
	img_state* img;
	SDL_LockMutex(g->img_loading_mtx);
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
			for (int i=g->n_imgs; i<g->done_loading; ++i) {
				create_textures(&g->img[i]);
			}
			// TODO duplication in do_mode_change
			g->needs_scr_rect_update = SDL_TRUE;
			g->adj_img_rects = SDL_TRUE;

			g->n_imgs = g->done_loading;
		}
		g->done_loading = 0;
		g->status = REDRAW;
		if (g->slideshow && IS_NORMAL())
			g->slide_timer =  SDL_GetTicks();
	}
	SDL_UnlockMutex(g->img_loading_mtx);
	
	if (!g->loading) {
		if (g->needs_scr_rect_update) {
			//SDL_Log("update after loading mode = %d %d", g->n_imgs, g->adj_img_rects);
			switch (g->n_imgs) {
			case 1: SET_MODE1_SCR_RECT(); break;
			case 2: SET_MODE2_SCR_RECTS(); break;
			case 4: SET_MODE4_SCR_RECTS(); break;
			case 8: SET_MODE8_SCR_RECTS(); break;
			}
			g->needs_scr_rect_update = SDL_FALSE;
		}
		if (g->adj_img_rects) {
			for (int i=0; i<g->n_imgs; ++i) {
				set_rect_bestfit(&g->img[i], g->fullscreen | g->slideshow | g->fill_mode);
			}
			g->adj_img_rects = SDL_FALSE;
		}
	}
}

int handle_events()
{
	if (IS_SCANNING_MODE()) {
		int ret = handle_scanning_events();
		if (IS_SCANNING_MODE()) {
			return ret;
		}
	}

	// like in draw_gui FS takes precedence over any underlying popups
	if (IS_FS_MODE()) {
		// TODO multiple return codes?
		if (handle_fb_events(&g->filebrowser, g->ctx)) {
			if (g->state == FILE_SELECTION) {
				if (g->filebrowser.file[0]) {
					transition_to_scanning(g->filebrowser.file);
					return 0;
				} else {
					// can only happen when SDL_QUIT or ESC on initial startup with no files
					return 1;
				}
			} else {
				// we're in a popup file browser for preferences or something
				if (g->filebrowser.file[0]) {
					strcpy(g->fs_output, g->filebrowser.file);
				} else {
					g->fs_output = NULL;
				}
				g->state = g->old_state;
				//g->state &= ~FILE_SELECTION;
			}
		}
		return 0;
	}

	if (IS_POPUP_ACTIVE()) {
		return handle_popup_events();
	}

	handle_loading();

	if (g->state & NORMAL) {
		return handle_events_normally();
	}

	if (IS_LIST_MODE()) {
		return handle_list_events();
	}

	return handle_thumb_events();

}

