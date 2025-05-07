

CVEC_NEW_DEFS2(thumb_state, RESIZE)

extern inline void hash2str(char* str, MD5_HASH* h)
{
	char buf[3];

	for (int i=0; i<MD5_HASH_SIZE; ++i) {
		sprintf(buf, "%02x", h->bytes[i]);
		strcat(str, buf);
	}
}

void get_thumbfilename(const char* img_path, char* thm_name, size_t thmname_len)
{
	MD5_HASH hash;
	//char hash_str[MD5_HASH_SIZE*2+1] = { 0 };

	Md5Calculate(img_path, strlen(img_path), &hash);
	//hash_str[0] = 0;
	//hash2str(hash_str, &hash);
	// could just do the %02x%02x etc. here but that'd be a long format string and 16 extra parameters
	u8* h = hash.bytes;
	snprintf(thm_name, thmname_len,
	    "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x.png", h[0],
	    h[1],h[2],h[3],h[4],h[5],h[6],h[7],h[8],h[9],h[10],h[11],h[12],h[13],h[14],h[15]);

}

void get_thumbpath(const char* img_path, char* thmpath, size_t thmpath_len)
{
	char name[MD5_HASH_SIZE*2+1] = {0};
	get_thumbfilename(img_path, name, sizeof(name));
	int ret = snprintf(thmpath, thmpath_len, "%s/%s", g->thumbdir, name);
	if (ret >= thmpath_len) {
		SDL_Log("path too long\n");
		cleanup(0, 1);
	}
}

void make_thumb_tex(int i, int w, int h, u8* pix)
{
	if (!pix)
		return;

	if (g->thumbs.a[i].tex) {
		// Doing a rotation/flip after loading thumbs, can't change the
		// texture dimensions, so have to destroy and make a new one
		SDL_DestroyTexture(g->thumbs.a[i].tex);
	}
	
	g->thumbs.a[i].tex = SDL_CreateTexture(g->ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, w, h);
	
	if (!g->thumbs.a[i].tex) {
		SDL_Log("Error creating texture: %s\n", SDL_GetError());
		cleanup(0, 1);
	}
	if (SDL_UpdateTexture(g->thumbs.a[i].tex, NULL, pix, w*4)) {
		SDL_Log("Error updating texture: %s\n", SDL_GetError());
		cleanup(0, 1);
	}

	g->thumbs.a[i].w = w;
	g->thumbs.a[i].h = h;
}

void load_thumb_textures(void)
{
	int w, h, channels;
	u8* outpix;
	char thumbpath[STRBUF_SZ] = { 0 };

	int start = g->thumb_start_row * g->thumb_cols;
	int end = start + g->thumb_cols*g->thumb_rows;

	for (int i=start; i<end && i<g->files.size; i++) {
		if (!g->files.a[i].path || g->thumbs.a[i].tex) {
			continue;
		}
		get_thumbpath(g->files.a[i].path, thumbpath, sizeof(thumbpath));

		outpix = stbi_load(thumbpath, &w, &h, &channels, 4);
		if (!outpix) {
			SDL_Log("Error, thumb %d doesn't exist\n", i);
		}
		make_thumb_tex(i, w, h, outpix);
		free(outpix);
	}
}

int make_thumb(int i, int w, int h, u8* pix, const char* thumbpath, int do_load)
{
	int out_w, out_h;
	u8* outpix;

	if (w > h) {
		out_w = THUMBSIZE;
		out_h = THUMBSIZE * h/(float)w;
	} else {
		out_h = THUMBSIZE;
		out_w = THUMBSIZE * w/(float)h;
	}

	if (!(outpix = malloc(out_h*out_w*4))) {
		cleanup(0, 1);
	}

	if (!stbir_resize_uint8_linear(pix, w, h, 0, outpix, out_w, out_h, 0, STBIR_RGBA)) {
		SDL_Log("Failed to resize for thumbnail!\n");
		free(outpix);
		return 0;
	}

	stbi_write_png(thumbpath, out_w, out_h, 4, outpix, out_w*4);

	if (do_load) {
		make_thumb_tex(i, out_w, out_h, outpix);
	}
	free(outpix);

	return 1;
}

int gen_thumbs(void* data)
{
	int w, h, channels;
	char thumbpath[STRBUF_SZ] = { 0 };

	struct stat thumb_stat, orig_stat;

	intptr_t do_load = (intptr_t)data;

	//int thumbdir_fd = open(g->thumbdir, O_RDONLY);

	int start = SDL_GetTicks();
	u8* pix;
	u8* outpix;
	for (int i=0; i<g->files.size; ++i) {

		// either the user exited, or wanted to open files or something that
		// requires us to end the thread early, but we
		// want to cleanly end the thread
		if (g->exit_gen_thumbs) {
			g->thumbs.size = (do_load) ? i : 0;
			goto exit_gen_thumbs;
		}

		// Worth actually going to sleep on thumb_cnd?
		// I don't see how this could cause a problem and it's far simpler
		// on both ends
		if (g->jit_thumb_flag) {
			do {
				// no actual use for this now, still just a true value but may
				// have a use for it later
				g->generating_thumbs = PAUSED;
				SDL_Delay(5);
			} while (g->jit_thumb_flag);
			g->generating_thumbs = RUNNING;
		}

		if (!g->files.a[i].path) {
			continue;
		}

		if (stat(g->files.a[i].path, &orig_stat)) {
			// TODO threading issue if user is trying
			// to load i at the same time, both will try
			// to download it
			SDL_Log("Couldn't stat %d %s\n", i, g->files.a[i].path);
			if (!curl_image(i)) {
				SDL_Log("Couldn't curl %d\n", i);
				free(g->files.a[i].path);
				g->files.a[i].path = NULL;
				g->files.a[i].name = NULL;
				g->bad_path_state = HAS_BAD;
				continue;
			}
		}

		// path was already set to realpath in myscandir
		get_thumbpath(g->files.a[i].path, thumbpath, sizeof(thumbpath));

		if (!stat(thumbpath, &thumb_stat)) {
			// make sure original hasn't been modified since thumb was made
			// don't think it's necessary to check nanoseconds
			if (orig_stat.st_mtime < thumb_stat.st_mtime) {
				if (do_load) {
					outpix = stbi_load(thumbpath, &w, &h, &channels, 4);
					make_thumb_tex(i, w, h, outpix);
					free(outpix);
				}
				continue;
			}
		}

		pix = stbi_load(g->files.a[i].path, &w, &h, &channels, 4);
		if (!pix) {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Couldn't load %s for thumbnail generation\nError %s", g->files.a[i].path, stbi_failure_reason());
			continue;
		}

		if (!make_thumb(i, w, h, pix, thumbpath, do_load)) {
			free(pix);
			continue;
		}
		free(pix);
		SDL_Log("generated thumb %d for %s\n", i, g->files.a[i].path);
	}

	if (do_load) {
		// same as in load_thumbs, generating and loading can take even longer
		if (!(g->state & SEARCH_RESULTS)) {
			g->thumb_sel = g->img[0].index;
		} else {
			g->thumb_sel = g->search_results.a[g->img[0].index];
		}
		g->thumb_sel_end = g->thumb_sel;
		g->thumb_start_row = g->thumb_sel / g->thumb_cols;
	}
	if (!g->save_status_uptodate) {
		if (g->bad_path_state == HAS_BAD) {
			remove_bad_paths();
		}
		update_save_status();
		g->save_status_uptodate = SDL_TRUE;
	}

exit_gen_thumbs:
	SDL_LockMutex(g->thumb_mtx);
	g->generating_thumbs = SDL_FALSE;
	g->thumbs_done = !g->exit_gen_thumbs;
	g->thumbs_loaded = do_load && !g->exit_gen_thumbs;
	// If we didn't exit early and didn't find any bad paths...
	if (g->bad_path_state == UNKNOWN && !g->exit_gen_thumbs) {
		g->bad_path_state = CLEAN;
	}
	SDL_CondSignal(g->thumb_cnd);
	SDL_UnlockMutex(g->thumb_mtx);

	// remember to reset this so we can re-run this thread later
	g->exit_gen_thumbs = SDL_FALSE;
	SDL_Log("Done generating thumbs in %.2f seconds, exiting thread.\n", (SDL_GetTicks()-start)/1000.0f);
	return 0;
}

void free_thumb(void* t)
{
	thumb_state* ts = (thumb_state*)t;
	if (ts->tex) {
		SDL_DestroyTexture(ts->tex);
	}
}

void generate_thumbs(intptr_t do_load)
{
	if (g->generating_thumbs || g->thumbs_done)
		return;

	// still using separate calloc because calling vec constructor uses
	// malloc and I want them 0'd
	if (!g->thumbs.a) {
		thumb_state* tmp;
		if (!(tmp = calloc(g->files.size, sizeof(thumb_state)))) {
			cleanup(0, 1);
		}
		g->thumbs.a = tmp;
		g->thumbs.size = g->files.size;
		g->thumbs.capacity = g->files.size;
		g->thumbs.elem_free = free_thumb;
		// elem_init already NULL
	}

	g->generating_thumbs = SDL_TRUE;
	SDL_Log("Starting thread to generate thumbs\n");
	if (do_load) {
		SDL_Log("Will load them as well\n");
	}
	SDL_Thread* thumb_thrd;
	if (!(thumb_thrd = SDL_CreateThread(gen_thumbs, "gen_thumbs_thrd", (void*)do_load))) {
		// TODO warning?
		SDL_Log("couldn't create thumb thread\n");
	}
	// passing NULL is a no-op like free
	SDL_DetachThread(thumb_thrd);
}

void stop_generating(void)
{
	// Exit gen_thumbs thrd first before we continue
	if (g->generating_thumbs) {
		g->exit_gen_thumbs = SDL_TRUE;
		// wait for thread to exit
		SDL_LockMutex(g->thumb_mtx);
		while (g->generating_thumbs) {
			SDL_LogDebugApp("Waiting for thumb generating thread to exit...\n");
			SDL_CondWait(g->thumb_cnd, g->thumb_mtx);
		}
		SDL_UnlockMutex(g->thumb_mtx);
	}
}

// TODO remove this function entirely since it only works for non-hardware
// accelerated backend?  Or change it to just loading the pixels?  The latter
// cause a huge memory spike
#if 0
int load_thumbs(void* data)
{
	int w, h, channels;
	u8* outpix;
	char thumbpath[STRBUF_SZ] = { 0 };
	int start = SDL_GetTicks();

	g->loading_thumbs = SDL_TRUE;

	for (int i=0; i<g->thumbs.size; i++) {
		// user exited, want to cleanly end thread
		if (g->is_exiting) {
			g->thumbs.size = i;
			goto exit_load_thumbs;
		}

		if (!g->files.a[i].path) {
			continue;
		}
		get_thumbpath(g->files.a[i].path, thumbpath, sizeof(thumbpath));

		outpix = stbi_load(thumbpath, &w, &h, &channels, 4);
		make_thumb_tex(i, w, h, outpix);
		free(outpix);
	}

	// make sure we are on current image after we're done loading
	// since loading can take a while if there are 1000's of images
	if (!(g->state & SEARCH_RESULTS)) {
		g->thumb_sel = g->img[0].index;
	} else {
		g->thumb_sel = g->search_results.a[g->img[0].index];
	}
	g->thumb_sel_end = g->thumb_sel;
	g->thumb_start_row = g->thumb_sel / g->thumb_cols;

	if (!g->save_status_uptodate) {
		if (g->bad_path_state == HAS_BAD) {
			remove_bad_paths();
		}
		update_save_status();
		//UPDATE_PLAYLIST_SAVE_STATUS();
		g->save_status_uptodate = SDL_TRUE;
	}

exit_load_thumbs:
	SDL_LockMutex(g->thumb_mtx);
	g->thumbs_loaded = SDL_TRUE;
	g->loading_thumbs = SDL_FALSE;
	SDL_CondSignal(g->thumb_cnd);
	SDL_UnlockMutex(g->thumb_mtx);

	SDL_Log("Done loading thumbs in %.2f seconds, exiting thread.\n", (SDL_GetTicks()-start)/1000.0f);
	return 0;
}
#endif

int jit_thumbs(void* data)
{
	// TODO check thumbs_done/thumbs_loaded outside or inside to prevent
	// unnecessary work?
	int load_what;
	char thumbpath[STRBUF_SZ] = { 0 };

	// TODO use fstatat or faccessat for for faster lookup
	struct stat thumb_stat, orig_stat;
	SDL_Event load_thumbs_evt = { .type = g->userevent };
	load_thumbs_evt.user.code = LOAD_THUMBS;

	int w, h, channels;
	int not_done;
	//int start = SDL_GetTicks();
	u8* pix;
	//u8* outpix;

	while (1) {
		SDL_LogDebugApp("top of jit_thumbs\n");
		SDL_LockMutex(g->jit_thumb_mtx);
		while (!g->jit_thumb_flag && !g->is_exiting) {
			SDL_CondWait(g->jit_thumb_cnd, g->jit_thumb_mtx);
			SDL_LogDebugApp("jit_thumbs thread woke with load: %d\n", g->jit_thumb_flag);
		}
		load_what = g->jit_thumb_flag;
		SDL_UnlockMutex(g->jit_thumb_mtx);

		if (g->is_exiting) {
			break;
		}

		// Default to do JUMP bounds
		int start = g->thumb_start_row * g->thumb_cols;
		int end = start + g->thumb_cols*g->thumb_rows;
		if (load_what == UP) {
			end = start + g->thumb_cols;
		} else if (load_what == DOWN) {
			start = end - g->thumb_cols;
		}
		do {
			for (int i=start; i<end && i<g->files.size; i++) {
				if (!g->files.a[i].path) {
					continue;
				}

				if (stat(g->files.a[i].path, &orig_stat)) {
					// TODO threading issue if user is trying
					// to load i at the same time, both will try
					// to download it
					SDL_Log("Couldn't stat %d %s\n", i, g->files.a[i].path);
					if (!curl_image(i)) {
						SDL_Log("Couldn't curl %d\n", i);
						free(g->files.a[i].path);
						g->files.a[i].path = NULL;
						g->files.a[i].name = NULL;
						g->bad_path_state = HAS_BAD;
						continue;
					}
				}
				// path was already set to realpath in myscandir
				get_thumbpath(g->files.a[i].path, thumbpath, sizeof(thumbpath));

				if (!stat(thumbpath, &thumb_stat)) {
					// make sure original hasn't been modified since thumb was made
					// don't think it's necessary to check nanoseconds
					if (orig_stat.st_mtime < thumb_stat.st_mtime) {
						/*
						 * // Has to happen on main thread
						if (do_load) {
							outpix = stbi_load(thumbpath, &w, &h, &channels, 4);
							make_thumb_tex(i, w, h, outpix);
							free(outpix);
						}
						*/
						continue;
					}
				}

				pix = stbi_load(g->files.a[i].path, &w, &h, &channels, 4);
				if (!pix) {
					SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Couldn't load %s for thumbnail generation\nError %s", g->files.a[i].path, stbi_failure_reason());
					continue;
				}

				if (!make_thumb(i, w, h, pix, thumbpath, SDL_FALSE)) {
					free(pix);
					continue;
				}
				free(pix);
				SDL_Log("generated thumb %d for %s\n", i, g->files.a[i].path);
			}

			// User can move multiple times while we're generating and
			// we would miss the signals since we're not waiting on the cnd
			// so we just check again in case the position has changed since
			// we started
			start = g->thumb_start_row * g->thumb_cols;
			end = start + g->thumb_cols*g->thumb_rows;
			not_done = 0;
			for (int i=start; i<end && i<g->files.size; i++) {
				get_thumbpath(g->files.a[i].path, thumbpath, sizeof(thumbpath));
				if (stat(thumbpath, &thumb_stat)) {
					not_done = 1;
					break;
				}
			}
			// if it's not done we just do the whole thing ie JUMP
		} while (not_done);

		SDL_PushEvent(&load_thumbs_evt);

		// Do I even need to lock here to set it back to 0?
		SDL_LockMutex(g->jit_thumb_mtx);
		g->jit_thumb_flag = 0;
		SDL_UnlockMutex(g->jit_thumb_mtx);
	}

	SDL_LockMutex(g->jit_thumb_mtx);
	SDL_Log("Exiting jit_thumb thread\n");
	g->jit_thumb_flag = EXIT;
	SDL_CondSignal(g->jit_thumb_cnd);
	SDL_UnlockMutex(g->jit_thumb_mtx);

	return 0;
}

void do_thumbmode2(void)
{
	// TODO should I just pre-allocate this whenever I finish
	// a scan?

	// still using separate calloc because calling vec constructor uses
	// malloc and I want them 0'd
	if (!g->thumbs.a) {
		thumb_state* tmp;
		if (!(tmp = calloc(g->files.size, sizeof(thumb_state)))) {
			cleanup(0, 1);
		}
		g->thumbs.a = tmp;
		g->thumbs.size = g->files.size;
		g->thumbs.capacity = g->files.size;
		g->thumbs.elem_free = free_thumb;
		// elem_init already NULL
	}

	//possibly preserve SEARCH_RESULTS
	if (g->state == NORMAL) {
		g->state = THUMB_DFLT;
		g->thumb_sel = g->img[0].index;
	} else {
		// clear NORMAL
		g->state = THUMB_SEARCH | SEARCH_RESULTS; // ^= NORMAL;
		g->thumb_sel = g->search_results.a[g->img[0].index];

		// for thumb mode we switch indices back immediately on leaving VIEW_RESULTS
		for (int i=0; i<g->n_imgs; ++i) {
			g->img[i].index = g->search_results.a[g->img[i].index];
		}
	}

	g->thumb_sel_end = g->thumb_sel;
	g->thumb_start_row = g->thumb_sel / g->thumb_cols;

	// Prevent any entirely black rows at the end if possible
	if (g->thumb_start_row*g->thumb_cols + g->thumb_rows*g->thumb_cols >= g->files.size+g->thumb_cols) {
		g->thumb_start_row = g->files.size / g->thumb_cols - g->thumb_rows + !!(g->files.size % g->thumb_cols);
		if (g->thumb_start_row < 0) g->thumb_start_row = 0;
	}

	SDL_LogDebugApp("signaling a JUMP to jit_thumbs\n");
	SDL_LockMutex(g->jit_thumb_mtx);
	g->jit_thumb_flag = JUMP;
	SDL_CondSignal(g->jit_thumb_cnd);
	SDL_UnlockMutex(g->jit_thumb_mtx);

	// guard out here or let the internal guard handle it?
	if (!g->generating_thumbs && g->run_thumb_thread == ON_THUMB_MODE) {
		generate_thumbs(SDL_FALSE);
	}
}

#if 0
void do_thumbmode(void)
{
	if (g->generating_thumbs) {
		SDL_Log("Cannot switch to thumbmode when generating thumbs already active\n");
		return;
	}

	//possibly preserve SEARCH_RESULTS
	if (g->state == NORMAL) {
		g->state = THUMB_DFLT;
		g->thumb_sel = g->img[0].index;
	} else {
		// clear NORMAL
		g->state = THUMB_SEARCH | SEARCH_RESULTS; // ^= NORMAL;
		g->thumb_sel = g->search_results.a[g->img[0].index];

		// for thumb mode we switch indices back immediately on leaving VIEW_RESULTS
		for (int i=0; i<g->n_imgs; ++i) {
			g->img[i].index = g->search_results.a[g->img[i].index];
		}
	}

	g->thumb_sel_end = g->thumb_sel;
	g->thumb_start_row = g->thumb_sel / g->thumb_cols;

	if (!g->thumbs_done) {
		generate_thumbs(SDL_TRUE);
	} else if (!g->thumbs_loaded) {
		SDL_Thread* thumb_thrd;
		if (!(thumb_thrd = SDL_CreateThread(load_thumbs, "load_thumbs_thrd", NULL))) {
			// TODO warning?
			SDL_Log("couldn't create load thumb thread\n");
		}
		// passing NULL is a no-op like free
		SDL_DetachThread(thumb_thrd);
	}

	g->status = REDRAW;
	// TODO what a mess, need to think about the best way
	// to handle GUI vs mouse in thumb vs normal mode
	// and I definitely want the infobar or a variant of it
	// in visual mode, like with vim show row and image number
	// total rows etc.
	SDL_ShowCursor(SDL_ENABLE);
	g->gui_timer = SDL_GetTicks();
	g->show_gui = nk_true;
}
#endif

void fix_thumb_sel(int dir)
{
	if (g->thumb_sel < 0)
		g->thumb_sel = 0;
	if (g->thumb_sel >= g->files.size)
		g->thumb_sel = g->files.size-1;
}

// TODO doing rem/del while generating/loading problematic
void do_thumb_rem_del_search(int do_delete, int invert)
{
	int i;
	if (!invert) {
		// this can only happen normally, with invert, obviously whatever you selected
		// is still there, should be no duplicates in search_results so we can
		// check up front without actually doing the removal
		if (g->search_results.size == g->files.size) {
			// TODO goto to FILE_SELECTION
			SDL_Log("You removed all your currently viewed images, exiting\n");
			cleanup(0, 1);
		}

		// search_results is sorted so we can go backward
		// to not mess up earlier result indices
		do {
			i = cvec_pop_i(&g->search_results);
			// TODO try to detect runs to erase more at once?

			if (do_delete) {
				if (remove(g->files.a[i].path))
					perror("Failed to delete image");
				else
					SDL_Log("Deleted %s\n", g->files.a[i].path);
			}
			cvec_erase_file(&g->files, i, i);
			cvec_erase_thumb_state(&g->thumbs, i, i);
		} while (g->search_results.size);
	} else {
		int end = g->files.size-1;
		do {
			i = cvec_pop_i(&g->search_results);

			if (do_delete) {
				for (int j=i+1; j<=end; ++j) {
					if (remove(g->files.a[j].path))
						perror("Failed to delete image");
					else
						SDL_Log("Deleted %s\n", g->files.a[j].path);
				}
			}
			// This works even if i == end.  erase becomes a no-op
			cvec_erase_file(&g->files, i+1, end);
			cvec_erase_thumb_state(&g->thumbs, i+1, end);

			end = i-1;
		} while (g->search_results.size);
	}

	// TODO?
	// Not worth trying to handle arbitrary selection imo so just reset to 0
	g->do_next = nk_true;
	int idx = g->files.size-1;
	for (int i=0; i<g->n_imgs; ++i) {
		g->img[i].index = idx++;
	}
	g->thumb_sel = 0;
}

void do_thumb_rem_del_dflt_visual(int do_delete, int invert)
{
	// so code below works for both THUMB_DFLT and VISUAL mode
	if (g->state == THUMB_DFLT) {
		g->thumb_sel_end = g->thumb_sel;
	}

	int start = g->thumb_sel, end = g->thumb_sel_end;
	if (g->thumb_sel > g->thumb_sel_end) {
		end = g->thumb_sel;
		start = g->thumb_sel_end;
	}
	if (g->is_thumb_visual_line) {
		start = (start / g->thumb_cols) * g->thumb_cols;
		end = (end / g->thumb_cols) * g->thumb_cols + g->thumb_cols - 1;
	}

	if (!invert) {
		if (end - start + 1 == g->files.size) {
			SDL_Log("You removed all your currently viewed images, exiting\n");
			cleanup(0, 1);
		}

		if (do_delete) {
			for (int i=start; i<=end; ++i) {
				if (remove(g->files.a[i].path))
					perror("Failed to delete image");
				else
					SDL_Log("Deleted %s\n", g->files.a[i].path);
			}
		}
		cvec_erase_file(&g->files, start, end);
		cvec_erase_thumb_state(&g->thumbs, start, end);
	} else {
		// invert selection means erase the pictures after and before
		// (in that order so less shifting is needed ... could also
		// just make a new vector, remove the selection and copy into
		// that and then free the original
		int start1 = end+1, end1 = g->files.size-1;
		if (do_delete) {
			for (int i=start1; i<=end1; ++i) {
				if (remove(g->files.a[i].path))
					perror("Failed to delete image");
				else
					SDL_Log("Deleted %s\n", g->files.a[i].path);
			}
		}
		// NOTE(rswinkle) cvector does not check for invalid parameters
		// but erase will become a no-op if start1 is end1+1
		// as long as cvec_sz is a signed integer type
		cvec_erase_file(&g->files, start1, end1);
		cvec_erase_thumb_state(&g->thumbs, start1, end1);

		// now the images to the left
		start1 = 0, end1 = start-1;
		if (do_delete) {
			for (int i=start1; i<=end1; ++i) {
				if (remove(g->files.a[i].path))
					perror("Failed to delete image");
				else
					SDL_Log("Deleted %s\n", g->files.a[i].path);
			}
		}
		// Not an error but would waste time copying the entire vectors
		// onto themselves, becomes memmove(&v[0], &v[0], v.size*sizeof(int))
		if (end1 >= 0) {
			cvec_erase_file(&g->files, start1, end1);
			cvec_erase_thumb_state(&g->thumbs, start1, end1);
		}
	}
	// If the current images are among the removed, update to 1
	// to the left and turn on the do_next flag (do when exiting thumb
	// mode).
	//
	// If the current images remain but images to the left were
	// removed, their index needs to be updated for the shift in
	// position
	if (g->files.size) {
		for (int i=0; i<g->n_imgs; ++i) {
			if (!invert) {
				if (g->img[i].index >= start && g->img[i].index <= end) {
					g->img[i].index = (start) ? start-1 : g->files.size-1;
					g->do_next = nk_true;
				} else if (g->img[i].index > end) {
					g->img[i].index -= end - start + 1;
				}
			} else if (g->img[i].index < start || g->img[i].index > end) {
				g->img[i].index = (i) ? (g->img[i-1].index + 1) % g->files.size : g->files.size - 1;
				g->do_next = nk_true;
			} else {
				g->img[i].index -= start;
			}
		}
	}
	g->thumb_sel = (!invert) ? start : 0;  // in case it was > _sel_end
}

void do_thumb_rem_del(int do_delete, int invert)
{
	if (g->state & THUMB_SEARCH) {
		do_thumb_rem_del_search(do_delete, invert);
	} else {
		do_thumb_rem_del_dflt_visual(do_delete, invert);
	}

	if (g->files.size) {
		fix_thumb_sel(1);
		// exit Visual mode after r/x (and backspace in this case) like vim
		g->state = THUMB_DFLT;
		g->is_thumb_visual_line = SDL_FALSE;
	} else {
		g->img[0].index = 0; // not sure if necesary but not a bad idea
		SDL_Event user_event = { .type = g->userevent };
		user_event.user.code = OPEN_FILE_NEW;
		SDL_PushEvent(&user_event);
	}
}

// TODO support invert like thumb_rem_del?
void do_thumb_save(int removing)
{
	if (g->loading) {
		return;
	}

	SDL_Log("In do_thumb_save()\n");

	int cur_plist_id = g->cur_playlist_id;
	//i64 loc;
	char* fullpath;

	// so we can share code for default and visual modes
	if (g->state == THUMB_DFLT) {
		g->thumb_sel_end = g->thumb_sel;
	}

	// Unused if search mode but meh
	int start = g->thumb_sel;
	int end = g->thumb_sel_end;
	if (g->thumb_sel > g->thumb_sel_end) {
		end = g->thumb_sel;
		start = g->thumb_sel_end;
	}
	if (g->is_thumb_visual_line) {
		start = (start / g->thumb_cols) * g->thumb_cols;
		end = (end / g->thumb_cols) * g->thumb_cols + g->thumb_cols - 1;
	}

	int idx;
	if (removing) {
		sqlite3_stmt* stmt = sqlstmts[DEL_FROM_PLIST_ID];
		sqlite3_bind_int(stmt, 1, cur_plist_id);

		if (g->state & SEARCH_RESULTS) {
			for (int i=0; i<g->search_results.size; ++i) {
				idx = g->search_results.a[i];
				fullpath = g->files.a[idx].path;
				sql_unsave(stmt, idx, fullpath);
			}
		} else {
			for (int i=start; i<=end; ++i) {
				fullpath = g->files.a[i].path;
				sql_unsave(stmt, i, fullpath);
			}
		}
	} else {
		sqlite3_stmt* stmt = sqlstmts[INSERT_INTO_PLIST];
		sqlite3_bind_int(stmt, 1, cur_plist_id);

		if (g->state & SEARCH_RESULTS) {
			for (int i=0; i<g->search_results.size; ++i) {
				idx = g->search_results.a[i];
				fullpath = g->files.a[idx].path;
				sql_save(stmt, idx, fullpath);
			}
		} else {
			for (int i=start; i<=end; ++i) {
				fullpath = g->files.a[i].path;
				sql_save(stmt, i, fullpath);
			}
		}
	}
}
