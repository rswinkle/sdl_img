// TODO finish
void do_file_select(int select_dir, intptr_t num_exts, const char** exts)
{
	// TODO using this cause I have it, but do I need it at all, even for file_open?
	g->old_state = g->state;

	g->state |= FILE_SELECTION;

	// TODO customize more with arg, "Select Playlist Dir", Select default playlist etc.
	if (select_dir) {
		SDL_SetWindowTitle(g->win, "Select Directory");
		//g->state |= SELECT_DIR;
	} else {
		SDL_SetWindowTitle(g->win, "Select File");
		//g->state |= SELECT_FILE;
	}

	// TODO think about this function, more args?  right now duplicating work
	reset_file_browser(&g->filebrowser, NULL);

	g->filebrowser.selection = -1; // default to no selection
	g->filebrowser.select_dir = select_dir;

	// whether we're selecting a directory or not we want to show all files
	// for general selection (in this case playlists)
	if (!num_exts) {
		g->filebrowser.ignore_exts = SDL_TRUE; // was reset by reset_file_browser
	} else {
		g->filebrowser.num_exts = num_exts;
		g->filebrowser.exts = exts;
	}
	switch_dir(&g->filebrowser, NULL);
	
	SDL_ShowCursor(SDL_ENABLE);

	const char* fs_type_str[] = { "file", "dir" };
	SDL_LogDebugApp("executing file select %s\n", fs_type_str[select_dir]);
}

void do_file_open(int clear_files)
{
	// TODO support Open more while in view results
	//if (g->n_imgs != 1 || !(g->state & NORMAL) || (g->state != NORMAL && !clear_files)) {
	if (g->n_imgs != 1 || (IS_RESULTS() && !clear_files)) {
		// TODO document this is README/man page, GUI tooltip?
		//SDL_Log("Only support opening files in 1-image mode and for opening more files you must also not be viewing results\n");
		SDL_Log("Only support opening files in 1-image mode and for opening more files you must also not be in search results\n");
		return;
	}

	// Exit gen_thumbs thrd first before we continue
	stop_generating();

	g->is_open_new = clear_files;
	g->old_state = g->state;

	for (int i=0; i<g->sources.size; i++) {
		SDL_Log("src %d: %s\n", i, g->sources.a[i]);
	}

	// TODO Naming "Open" "Open New" "Open New Images" vs
	// "Open More" "Open More Images", "File Selector" etc.
	// Currently matches GUI menu options
	if (clear_files) {
		SDL_SetWindowTitle(g->win, "Open New");
	} else {
		// Any bad paths will cause a crash in strcmp in remove_duplicates()
		remove_bad_paths();
		SDL_SetWindowTitle(g->win, "Open More");
	}

	g->state = FILE_SELECTION;
	reset_file_browser(&g->filebrowser, NULL);
	g->filebrowser.selection = -1; // default to no selection

	// reset these in case we did a file select with a different
	// type filter (ie specifying a .ttf font file)
	g->filebrowser.exts = g->img_exts;
	g->filebrowser.num_exts = g->n_exts;
	//
	// If they're in playlistdir keep settings the same
	if (strcmp(g->filebrowser.dir, g->playlistdir)) {
		g->open_single = SDL_FALSE;
		g->open_list = SDL_FALSE;
		g->open_recursive = SDL_FALSE;
	} else {
		g->open_list = SDL_TRUE;  // should still be true but for clarity
		g->filebrowser.ignore_exts = SDL_TRUE; // was reset by reset_file_browser
	}

	SDL_ShowCursor(SDL_ENABLE);

	const char* open_type_str[] = { "MORE", "NEW" };
	SDL_LogDebugApp("executing OPEN_FILE%s\n", open_type_str[clear_files]);
}

// call it scan?
void do_lib_import(void)
{
	// TODO
}

void do_shuffle(void)
{
	if (g->n_imgs != 1) {
		SDL_Log("Only support shuffling in 1 image mode\n");
		return;
	}
	stop_generating();

	if (g->bad_path_state == HAS_BAD) {
		SDL_Log("Removing bad paths before shuffling...\n");
		remove_bad_paths();
	}

	char* save = g->img[0].fullpath;
	file tmpf;

	thumb_state tmp_thumb;
	int j;
	// Fisher-Yates, aka Knuth Shuffle
	for (int i=g->files.size-1; i>0; --i) {
		j = rand() % (i+1);

		tmpf = g->files.a[i];
		g->files.a[i] = g->files.a[j];
		g->files.a[j] = tmpf;

		if (g->thumbs.a) {
			tmp_thumb = g->thumbs.a[i];
			g->thumbs.a[i] = g->thumbs.a[j];
			g->thumbs.a[j] = tmp_thumb;
		}
	}

	if (g->state & RESULT_MASK) {
		search_filenames(SDL_FALSE);

		for (int j=0; j<g->search_results.size; ++j) {
			int i = g->search_results.a[j];
			if (!strcmp(save, g->files.a[i].path)) {
				// selection is used in listmode results, = index in results
				SDL_Log("Setting index to %d\n", j);
				g->selection = g->img[0].index = j;

				// thumb_sel is the actual index in g->files, since results are
				// not separated out, just highlighted like vim
				g->thumb_sel = i;
				g->thumb_start_row = g->thumb_sel / g->thumb_cols;
				break;

			}
		}
	} else {
		for (int i=0; i<g->files.size; ++i) {
			if (!strcmp(save, g->files.a[i].path)) {
				g->img[0].index = i;
				g->thumb_sel = i;
				g->selection = i;
				break;
			}
		}
	}

	g->sorted_state = NONE;
}



void do_sort(compare_func cmp)
{
	if (g->n_imgs != 1) {
		SDL_Log("Can't sort in multi-image modes\n");
		return;
	}
	stop_generating();

	if (g->bad_path_state == HAS_BAD) {
		SDL_Log("Removing bad paths before sorting...");
		remove_bad_paths();
	}

	char* save_cur = g->img[0].fullpath;

	// TODO Should we also save selection separately?  thumb_sel?
	char* save_sel = NULL;
	if (g->selection >= 0) {
		if (g->state & RESULT_MASK) {
			save_sel = g->files.a[g->search_results.a[g->selection]].path;
		} else {
			save_sel = g->files.a[g->selection].path;
		}
	}

	// g->thumbs.a is either NULL or valid
	if (g->thumbs.a) {
		mirrored_qsort(g->files.a, g->files.size, sizeof(file), cmp, 1, g->thumbs.a, sizeof(thumb_state));
	} else {
		mirrored_qsort(g->files.a, g->files.size, sizeof(file), cmp, 0);
	}

	// find new index of img[0]
	// TODO use bsearch?
	int cur_idx = 0;
	for (int i=0; i<g->files.size; ++i) {
		if (!strcmp(save_cur, g->files.a[i].path)) {
			cur_idx = i;
			break;
		}
	}

	// TODO really think about all the different directions/interactions
	//
	// should work even while in result modes
	if (g->state & RESULT_MASK) {
		search_filenames(SDL_FALSE);

		if (save_sel) {
			for (int j=0; j<g->search_results.size; ++j) {
				if (!strcmp(save_sel, g->files.a[g->search_results.a[j]].path)) {
					g->selection = j;
					break;
				}
			}
		}

		// convert current image index if viewing results or not
		if (g->state & NORMAL) {
			for (int j=0; j<g->search_results.size; ++j) {
				if (g->search_results.a[j] == cur_idx) {

					// selection is used in listmode results, = index in results
					g->img[0].index = j;

					// TODO test all state changes to/from thumb mode with and w/o active search
					// and are these even necessary?  Or are they correctly set when ESC back from
					// view results to thumb mode
					//
					// thumb_sel is the actual index in g->files, since results are
					// not separated out, just highlighted like vim
					g->thumb_sel = cur_idx;
					g->thumb_start_row = g->thumb_sel / g->thumb_cols;
					break;
				}
			}
		} else {
			g->img[0].index = cur_idx;
		}
	} else {
		// In non-result modes, index and selection are the files index
		if (save_sel) {
			if (save_sel != save_cur) {
				for (int i=0; i<g->files.size; ++i) {
					if (!strcmp(save_sel, g->files.a[i].path)) {
						g->selection = i;
						break;
					}
				}
			} else {
				g->selection = cur_idx;
			}
		}

		g->img[0].index = cur_idx;
	}
}

// TODO any more reason for this function?  Worth having?
void do_lib_sort(compare_func cmp)
{
	if (g->lib_mode_list.size) {
		mirrored_qsort(g->lib_mode_list.a, g->lib_mode_list.size, sizeof(file), cmp, 0);
		g->selection = 0;
		get_img_playlists(0);
	} else {
		g->selection = -1;
	}
	g->list_setscroll = SDL_TRUE;
	if (g->preview.tex) {
		SDL_DestroyTexture(g->preview.tex);
		g->preview.tex = NULL;
	}
}

void do_zoom(int dir, int use_mouse)
{
	if (!g->img_focus) {
		for (int i=0; i<g->n_imgs; ++i) {
			if (g->img[i].frames > 1) {
				dir /= GIF_ZOOM_DIV;
				break;
			}
		}
		for (int i=0; i<g->n_imgs; ++i)
			set_rect_zoom(&g->img[i], dir, use_mouse);
	} else {
		if (g->img_focus->frames > 1)
			dir /= GIF_ZOOM_DIV;
		set_rect_zoom(g->img_focus, dir, use_mouse);
	}
}

void do_pan(int dx, int dy)
{
	img_state* img = NULL;
	if (!g->img_focus) {
		for (int i=0; i<g->n_imgs; ++i) {
			img = &g->img[i];
			if (dx != 0 && img->disp_rect.w > img->scr_rect.w) {
				img->disp_rect.x += dx;
			}
			if (dy != 0 && img->disp_rect.h > img->scr_rect.h) {
				img->disp_rect.y += dy;
			}
			fix_rect(img);
		}
	} else {
		img = g->img_focus;
		if (dx != 0 && img->disp_rect.w > img->scr_rect.w) {
			img->disp_rect.x += dx;
		}
		if (dy != 0 && img->disp_rect.h > img->scr_rect.h) {
			img->disp_rect.y += dy;
		}
		fix_rect(img);
	}
}

void do_rotate(int left, int is_90)
{
	img_state* img;
	if (!g->loading) {
		img = (g->n_imgs == 1) ? &g->img[0] : g->img_focus;
		if (img) {
			if (is_90) {
				rotate_img90(img, left);
				create_textures(img);
			} else {
				g->state |= ROTATE;
				set_show_gui(SDL_TRUE);
				return;
			}

			g->adj_img_rects = SDL_TRUE;
			g->status = REDRAW;
		}
	}
}

void do_flip(int is_vertical)
{
	int w, h;
	int sz;
	int frames;

	img_state* img = (g->n_imgs == 1) ? &g->img[0] : g->img_focus;
	if (!g->loading && img) {
		w = img->w;
		h = img->h;
		frames = img->frames;

		sz = w * h;
		u8* pix = img->pixels;
		u8* flip_pix = NULL;

		if (!(flip_pix = malloc(frames * (sz*4)))) {
			perror("Couldn't allocate flipped");
			cleanup(0, 1);
		}

		i32* p;
		i32* flip;
		if (is_vertical) {
			for (int i=0; i<frames; ++i) {
				p = (i32*)&pix[i*(sz*4)];
				flip = (i32*)&flip_pix[i*(sz*4)];
				for (int j=0; j<h; ++j) {
					memcpy(&flip[(h-1-j)*w], &p[j*w], w*sizeof(i32));
					//for (int k=0; k<w; ++k) {
					//	flip[(h-1-j)*w + k] = p[j*w + k];
					//}
				}
			}

		} else {
			for (int i=0; i<frames; ++i) {
				p = (i32*)&pix[i*(sz*4)];
				flip = (i32*)&flip_pix[i*(sz*4)];
				for (int j=0; j<h; ++j) {
					for (int k=0; k<w; ++k) {
						flip[j*w + w-1-k] = p[j*w+k];
					}
				}
			}

		}
		// don't call clear_img here because could do multiple
		// edits (flips/rotations etc.) and don't want to prompt to save every time
		for (int i=0; i<img->frames; ++i) {
			SDL_DestroyTexture(img->tex[i]);
		}
		free(pix);

		img->pixels = flip_pix;
		img->edited = FLIPPED;

		create_textures(img);

		g->adj_img_rects = SDL_TRUE;
		g->status = REDRAW;
	}
}

void do_mode_change(intptr_t mode)
{
	// mode is an enum that also == the number of images
	if (g->n_imgs != mode && g->files.size >= mode) {
		g->status = REDRAW;
		g->slide_timer =  SDL_GetTicks();
		g->adj_img_rects = SDL_TRUE;
		g->needs_scr_rect_update = SDL_TRUE;

		if (g->n_imgs < mode) {
			SDL_LockMutex(g->img_loading_mtx);
			g->loading = mode;
			SDL_CondSignal(g->img_loading_cnd);
			SDL_UnlockMutex(g->img_loading_mtx);
			//g->n_imgs gets updated in handle_events() once loading finishes
		} else {
			if (mode != MODE1 || !g->img_focus || g->img_focus == &g->img[0]) {
				for (int i=g->n_imgs-1; i>mode-1; --i)
					clear_img(&g->img[i]);

				if (g->img_focus >= &g->img[mode] || mode == MODE1) {
					g->img_focus = NULL;
					int index = (IS_VIEW_RESULTS()) ? g->search_results.a[g->img[0].index] : g->img[0].index;
					SDL_SetWindowTitle(g->win, g->files.a[index].name);
				}
			} else {
				// if mode1 and focus and focus != img[0] have to
				// clear the others and move focused img to img[0]
				for (int i=0; i<g->n_imgs; ++i) {
					if (g->img_focus != &g->img[i]) {
						clear_img(&g->img[i]);
					}
				}
				replace_img(&g->img[0], g->img_focus);
				g->img_focus = NULL;
			}
			g->n_imgs = mode;

			// TODO don't always do this? seee above and in event done_loading code
			//g->img_focus = NULL;
			//SDL_SetWindowTitle(g->win, g->files.a[0].name);
		}
	}
}

void do_remove(SDL_Event* next)
{
	// GUI alert for these?  Could move this logic directly into GUI or events
	// only remove in single image mode to avoid confusion and complication
	if (g->loading || g->n_imgs != 1) {
		SDL_Log("Can't remove images while loading images, or in any mode but single image mode");
		return;
	}
	stop_generating();

	int files_index = g->img[0].index;

	if (IS_VIEW_RESULTS()) {
		// Have to remove from results and decrement all higher results (this works
		// because results are always found from front to back so later results always have higher
		// g->files indices)
		cvec_erase_i(&g->search_results, g->img[0].index, g->img[0].index);
		for (int i=g->img[0].index; i<g->search_results.size; ++i) {
			g->search_results.a[i]--;
		}

		// get actual index to delete correct location from files and thumbs
		files_index = g->search_results.a[g->img[0].index];
	}

	SDL_Log("Removing %s\n", g->files.a[files_index].path);
	// TODO should remove in VIEW_RESULTS remove from results only or also files and thumbs?
	cvec_erase_file(&g->files, files_index, files_index);

	if (g->thumbs.a) {
		cvec_erase_thumb_state(&g->thumbs, files_index, files_index);
	}

	// since everything shifted left, we need to pre-decrement to not skip an image
	if (!g->img[0].index) {
		g->img[0].index = g->files.size-1;
	} else {
		g->img[0].index--;
	}

	if (g->files.size) {
		SDL_PushEvent(next);
	} else {
		g->img[0].index = 0; // not sure if necesary but not a bad idea
		SDL_Event user_event = { .type = g->userevent };
		user_event.user.code = OPEN_FILE_NEW;
		SDL_PushEvent(&user_event);
	}
}

void do_delete(SDL_Event* next)
{
	SDL_MessageBoxButtonData buttons[] = {
		//{ /* .flags, .buttonid, .text */        0, 0, "no" },
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "yes" },
		{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "cancel" },
	};

	SDL_MessageBoxData messageboxdata = {
		SDL_MESSAGEBOX_WARNING, /* .flags */
		NULL, /* .window */
		"Warning", /* .title */
		NULL, /* .message to be set later */
		SDL_arraysize(buttons), /* .numbuttons */
		buttons, /* .buttons */
		NULL /* .colorScheme, NULL = system default */
	};
	int buttonid = 1;

	char msgbox_prompt[STRBUF_SZ];

	// TODO GUI alert for these?  Could move this logic directly into GUI or events
	// only delete in single image mode to avoid confusion and complication
	// and not while generating/loading thumbs or loading images
	if (g->loading || g->n_imgs != 1) {
		SDL_Log("Can't delete images while loading images, or in any mode but single image mode");
		return;
	}
	stop_generating();

	char* full_img_path;
	if (!IS_VIEW_RESULTS()) {
		full_img_path = g->files.a[g->img[0].index].path;
	} else {
		full_img_path = g->files.a[g->search_results.a[g->img[0].index]].path;
	}

	// TODO Could remove if they don't want to delete?  Or still remove if delete fails?
	// Check MessageBox options
	snprintf(msgbox_prompt, STRBUF_SZ, "Are you sure you want to delete '%s'?", full_img_path);
	messageboxdata.message = msgbox_prompt;

	if (g->confirm_delete) {
		SDL_ShowMessageBox(&messageboxdata, &buttonid);
	}

	if (buttonid == 1) {
		if (remove(full_img_path)) {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to delete image: %s", strerror(errno));
		} else {
			SDL_Log("Deleted %s\n", full_img_path);
			do_remove(next);
		}
	}
}

void do_actual_size(void)
{
	g->status = REDRAW;
	if (!g->img_focus) {
		for (int i=0; i<g->n_imgs; ++i) {
			adjust_rect(&g->img[i], g->img[i].w, g->img[i].h, SDL_FALSE);
		}
	} else {
		adjust_rect(g->img_focus, g->img_focus->w, g->img_focus->h, SDL_FALSE);
	}
}

int cvec_contains_str(cvector_str* list, char* s)
{
	if (s) {
		for (int i=0; i<list->size; ++i) {
			if (!strcmp(list->a[i], s)) {
				return i;
			}
		}
	}
	return -1;
}

/*
void do_save(int removing)
{
	if (g->loading)
		return;

	char* playlist = g->cur_playlist;
	int idx;
	cvec_sz loc;
	if (removing) {
		if (g->img_focus) {
			idx = g->img_focus->index;
			if (IS_VIEW_RESULTS()) {
				idx = g->search_results.a[idx];
			}
			if (!g->save_status_uptodate) {
				g->files.a[idx].playlist_idx = cvec_contains_str(&g->favs, g->files.a[idx].path);
			}

			if ((loc = g->files.a[idx].playlist_idx) < 0) {
				SDL_Log("%s not in %s\n", g->img_focus->fullpath, playlist);
			} else {
				SDL_Log("removing %s\n", g->img_focus->fullpath);
				cvec_erase_str(&g->favs, loc, loc);
				g->files.a[idx].playlist_idx = -1;
				SDL_Log("%"PRIcv_sz" left after removal\n", g->favs.size);

				if (g->save_status_uptodate) {
					for (int i=0; i<g->files.size; ++i) {
						if (g->files.a[i].playlist_idx > loc) {
							g->files.a[i].playlist_idx--;
						}
					}
				}
			}
		} else {
			for (int i=0; i<g->n_imgs; ++i) {
				idx = g->img[i].index;
				if (IS_VIEW_RESULTS()) {
					idx = g->search_results.a[idx];
				}
				if (!g->save_status_uptodate) {
					g->files.a[idx].playlist_idx = cvec_contains_str(&g->favs, g->files.a[idx].path);
				}
				if ((loc = g->files.a[idx].playlist_idx) < 0) {
					SDL_Log("%s not in %s\n", g->img[i].fullpath, playlist);
				} else {
					SDL_Log("removing %s\n", g->img[i].fullpath);
					cvec_erase_str(&g->favs, loc, loc);
					g->files.a[idx].playlist_idx = -1;
					SDL_Log("%"PRIcv_sz" after removal\n", g->favs.size);

					if (g->save_status_uptodate) {
						for (int i=0; i<g->files.size; ++i) {
							if (g->files.a[i].playlist_idx > loc) {
								g->files.a[i].playlist_idx--;
							}
						}
					}
				}
			}
		}
	} else {
		if (g->img_focus) {
			idx = g->img_focus->index;
			if (IS_VIEW_RESULTS()) {
				idx = g->search_results.a[idx];
			}
			if (!g->save_status_uptodate) {
				g->files.a[idx].playlist_idx = cvec_contains_str(&g->favs, g->files.a[idx].path);
			}
			if (g->files.a[idx].playlist_idx >= 0) {
				SDL_Log("%s already in %s\n", g->img_focus->fullpath, playlist);
			} else {
				SDL_Log("saving %s\n", g->img_focus->fullpath);
				cvec_push_str(&g->favs, g->img_focus->fullpath);
				g->files.a[idx].playlist_idx = g->favs.size-1;
			}
		} else {
			for (int i=0; i<g->n_imgs; ++i) {
				idx = g->img[i].index;
				if (IS_VIEW_RESULTS()) {
					idx = g->search_results.a[idx];
				}
				if (!g->save_status_uptodate) {
					g->files.a[idx].playlist_idx = cvec_contains_str(&g->favs, g->files.a[idx].path);
				}
				if (g->files.a[idx].playlist_idx >= 0) {
					SDL_Log("%s already in %s\n", g->img[i].fullpath, playlist);
				} else {
					SDL_Log("saving %s\n", g->img[i].fullpath);
					cvec_push_str(&g->favs, g->img[i].fullpath);
					g->files.a[idx].playlist_idx = g->favs.size-1;
				}
			}
		}
	}

	// Make sure to show the GUI for a second so the user has visual confirmation
	set_show_gui(SDL_TRUE);
}
*/

// There is no easy way to do cross platform visual copy paste.
// SDL lets you do text but to get visual, I'd have to be using something
// like Qt, or start pulling in x11, winapi, etc. and write it myself
// which defeats the purpose of using/preferring single header libraries
// and trying to minimize external dependencies.
int do_copy(void)
{
	if (g->loading)
		return 0;

	img_state* img = (g->n_imgs == 1) ? &g->img[0] : g->img_focus;
	if (!img)
		return 0;

	SDL_SetClipboardText(g->files.a[img->index].path);

	// TODO what if I want ESC to mean no change?  third button cancel?
	SDL_MessageBoxButtonData buttons[] = {
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 2, "yes" },
		{ 0,                                       1, "no" },
		{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "cancel" }
	};

	SDL_MessageBoxData messageboxdata = {
		SDL_MESSAGEBOX_WARNING, /* .flags */
		g->win, /* .window */
		"Warning: No Visual Copy Supported", /* .title */
		NULL, /* .message to be set later */
		SDL_arraysize(buttons), /* .numbuttons */
		buttons, /* .buttons */
		NULL /* .colorScheme, NULL = system default */
	};
	int buttonid = 123;

	char msgbox_prompt[] =
	"No visual copy supported. The path of the image has been copied to the clipboard.\n"
	"Use ALT + Print Screen, or copy it from your file browser to get a visual copy.\n\n"
	"Show this warning next time?";
	messageboxdata.message = msgbox_prompt;

	if (g->warn_text_copy) {
		// NOTE hitting x sets buttonid to -1 which we treat like cancel
		// ESC sets it to 0, tabbing doesn't seem to work to switch buttons
		if (SDL_ShowMessageBox(&messageboxdata, &buttonid)) {
			SDL_Log("messagebox error: %s\n", SDL_GetError());
			return 1; // probably an error big enough to exit but meh
		}
		SDL_Log("buttonid = %d\n", buttonid);
		if (buttonid > 0) {
			g->warn_text_copy = buttonid == 2;
		}
		return !buttonid;
	}

	return 0;
}

void do_libmode(void)
{
	// Automatically go to n_imgs = 1
	// if (g->n_imgs != 1) {
	// 	do_mode_change(1);
	// }
	
	// TODO different function for general library so we don't care?  Or just goto
	// viewing the Library or disable sorting if generating?
	if (g->n_imgs != 1) {
		SDL_Log("Can't go to libmode from multi-image modes");
		return;
	}

	// TODO hmm handle switching directly from thumb to lib and vice versa
	if (g->state == NORMAL) {
		g->state = LIB_DFLT;
	} else {
		g->state = LIB_DFLT | SEARCH_RESULTS;
		if (text_buf[0] == '/') {
			memmove(text_buf, &text_buf[1], text_len);
			text_len--;
		}
	}
	
	g->list_view = &g->files;
	g->cur_selected = SDL_TRUE;
	g->lib_selected = SDL_FALSE;
	g->selected_plist = -1;

	g->selection = g->img[0].index;
	get_img_playlists(g->img[0].index);

	g->list_setscroll = SDL_TRUE;

	if (g->preview.tex) {
		SDL_DestroyTexture(g->preview.tex);
		g->preview.tex = NULL;
	}

	/*
	 * // this should always be cleared when exiting a search mode
	text_buf[0] = 0;
	text_len = 0;
	g->search_results.size = 0;
	*/
	SDL_ShowCursor(SDL_ENABLE);
}

void do_remove_from_lib(void)
{
	remove_from_lib(g->selection);

	// TODO should I remove it from current? If I do I need
	// to handle if it's currently open, img[0]
	if (!g->cur_selected) {
		handle_selection_removal();
	}
}

