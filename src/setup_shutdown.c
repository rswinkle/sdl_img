
int mkdir_p(const char* path, mode_t mode)
{
	char path_buf[STRBUF_SZ] = { 0 };

	strncpy(path_buf, path, STRBUF_SZ);
	if (path_buf[STRBUF_SZ-1]) {
		errno = ENAMETOOLONG;
		return -1;
	}

	char* p = path_buf;

	// minor optimization, and lets us do p[-1] below
	if (*p == '/')
		p++;

	for (; *p; ++p) {
		// no need to handle two / in a row
		if (*p == '/' && p[-1] != '/') {
			*p = 0;
			if (mkdir(path_buf, mode) && errno != EEXIST) {
				return -1;
			}
			*p = '/';
		}
	}

	if (mkdir(path_buf, mode) && errno != EEXIST) {
		return -1;
	}

	return 0;
}

void setup_dirs(void)
{
	char datebuf[200] = { 0 };
	time_t t;
	struct tm *tmp;
	int len;
	

	t = time(NULL);
	srand(t);  // we use rand() in do_shuffle()

	char* prefpath = g->prefpath;

	// cachedir wasn't passed as an argument or loaded from config.lua
	if (!g->cachedir[0]) {
		tmp = localtime(&t);
		strftime(datebuf, sizeof(datebuf), "%Y-%m-%d", tmp);  // %F

		len = snprintf(g->cachedir, STRBUF_SZ, "%scache/%s", prefpath, datebuf);
		if (len >= STRBUF_SZ) {
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "cache path too long\n");
			cleanup(1, 1);
		}
	}
	if (mkdir_p(g->cachedir, S_IRWXU) && errno != EEXIST) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to make cache directory: %s\n", strerror(errno));
		cleanup(1, 1);
	}

	if (!g->thumbdir[0]) {
		len = snprintf(g->thumbdir, STRBUF_SZ, "%sthumbnails", prefpath);
		if (len >= STRBUF_SZ) {
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "thumbnail path too long\n");
			cleanup(1, 1);
		}
	}
	if (mkdir_p(g->thumbdir, S_IRWXU) && errno != EEXIST) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to make thumb directory: %s\n", strerror(errno));
		cleanup(1, 1);
	}

	SDL_Log("cache: %s\nthumbnails: %s\n", g->cachedir, g->thumbdir);

	if (!g->logdir[0]) {
		len = snprintf(g->logdir, STRBUF_SZ, "%slogs", prefpath);
		if (len >= STRBUF_SZ) {
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "logdir path too long\n");
			cleanup(1, 1);
		}
		if (mkdir_p(g->logdir, S_IRWXU) && errno != EEXIST) {
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to make log directory: %s\n", strerror(errno));
			cleanup(1, 1);
		}
	}

	SDL_Log("logs: %s\n", g->logdir);

	if (!g->playlistdir[0]) {
		len = snprintf(g->playlistdir, STRBUF_SZ, "%splaylists", prefpath);
		if (len >= STRBUF_SZ) {
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "playlist path too long\n");
			cleanup(1, 1);
		}
		if (mkdir_p(g->playlistdir, S_IRWXU) && errno != EEXIST) {
			perror("Failed to make playlist directory");
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to make playlist directory: %s\n", strerror(errno));
			cleanup(1, 1);
		}
	}

	SDL_Log("playlists: %s\n", g->playlistdir);

}

// TODO make a macro?
void reset_behavior_prefs(void)
{
	g->slide_delay = DFLT_SLIDE_DELAY;
	g->gui_delay = DFLT_GUI_DELAY;
	g->button_rpt_delay = DFLT_BUTTON_RPT_DELAY;
	g->fullscreen_gui = DFLT_FULLSCREEN_GUI;

	g->thumb_rows = DFLT_THUMB_ROWS;
	g->thumb_cols = DFLT_THUMB_COLS;

	g->fill_mode = DFLT_FILL_MODE;
	g->show_infobar = DFLT_SHOW_INFOBAR;
	g->thumb_x_deletes = DFLT_THUMB_X_DELETES;
	g->confirm_delete = DFLT_CONFIRM_DELETE;
	g->confirm_rotation = DFLT_CONFIRM_ROTATION;
	g->warn_text_copy = DFLT_WARN_TEXT_COPY;

	g->ind_mm = DFLT_IND_MM;
}

int load_config(void)
{
	char config_path[STRBUF_SZ] = { 0 };
	char new_path[STRBUF_SZ] = { 0 };
	char err_buf[STRBUF_SZ] = { 0 };
	snprintf(config_path, STRBUF_SZ, "%sconfig.lua", g->prefpath);
	SDL_LogDebugApp("config file: %s\n", config_path);

	if (!read_config_file(config_path)) {

		if (!access(config_path, F_OK)) {
			snprintf(new_path, STRBUF_SZ, "%s.err", config_path);
			snprintf(err_buf, STRBUF_SZ, "Error attempting to load the config file: %s\nWill attempt to rename file to %s and create a new file for default settings", g->lua_error, new_path);
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error Loading Config File", err_buf, g->win);
			if (rename(config_path, new_path)) {
				SDL_Log("Could not rename config file %s", strerror(errno));
			}
		} else {
			SDL_Log("%s did not exist, will create a new one with default settings\n", config_path);
		}

		// Create a blank config file
		FILE* cfg_file = fopen(config_path, "w");
		if (!cfg_file) {
			snprintf(err_buf, STRBUF_SZ, "Failed to open config file for writing: %s", strerror(errno));
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error Creating Config File", err_buf, g->win);
			SDL_LogCriticalApp("%s", err_buf);
			cleanup(1, 0);
		}
		fclose(cfg_file);

		// read the blank config file to set everything to defaults
		read_config_file(config_path);
		return nk_false;
	}
	SDL_LogDebugApp("Successfully loaded config file\n");

	return nk_true;
}

void print_help(char* prog_name, int verbose)
{
	puts("Usage:");
	printf("  %s image_path\n", prog_name);
	printf("  %s directory\n", prog_name);
	printf("  %s image_URL\n", prog_name);
	printf("  %s -l/--list text_list_of_image_paths/urls\n", prog_name);
	puts("\nOr any combination of those uses, ie:");
	printf("  %s image.jpg -l list1 -s 8 ~/some/dir example.com/image.jpg -l list3 image4.gif -f\n", prog_name);

	if (verbose) {
		puts("\nApplication Options:");
		puts("  -f, --fullscreen                   Start in fullscreen mode");
		puts("  -s, --slide-show [delay=3]         Start in slideshow mode");
		puts("  -l, --list list_file               Add all paths/urls in list_file to list");
		puts("  -p, --playlist playlist            Add all images in playlist from database to list");
		puts("  -r, --recursive dir                Scan dir recursively for images to add to the list");
		puts("  -R                                 Scan all directories that come after recursively (-r after -R is redundant)");
		puts("  -c, --cache ./your_cache_loc       Use specified directory as cache");
		puts("  -v, --version                      Show the version");
		puts("  -h, --help                         Show this help");
	}
}

void setup(int argc, char** argv)
{
	char error_str[STRBUF_SZ] = { 0 };
	char title_buf[STRBUF_SZ] = { 0 };

	static const char* default_exts[NUM_DFLT_EXTS] =
	{
		".jpg",
		".jpeg",
		".gif",
		".png",
		".bmp",

		".ppm",
		".pgm",

		".tga",
		".hdr",
		".pic",
		".psd"
	};

//#ifndef NDEBUG
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_DEBUG);
//#endif

	// set defaults before loading config file
	g->img_exts = default_exts;
	g->n_exts = NUM_DFLT_EXTS;

	// Not currently used
	// char* exepath = SDL_GetBasePath();

	// TODO think of a company/org name
	g->prefpath = SDL_GetPrefPath("", "sdl_img");
	normalize_path(g->prefpath);
	//SDL_Log("%s\n%s\n\n", exepath, g->prefpath);
	// SDL_free(exepath);

	// have to set these before load_config

	// just point these at a buffer that will live forever
	g->cachedir = g->cachedir_buf;
	g->thumbdir = g->thumbdir_buf;
	g->logdir = g->logdir_buf;
	g->playlistdir = g->playlistdir_buf;

	// TODO all other prefs are set to defaults via load_config() if they
	// don't exist in the file
	g->thumb_highlight = DFLT_THUMB_HIGHLIGHT_COLOR;
	g->thumb_opacity = DFLT_THUMB_OPACITY;
	// TODO compare with config enums
	g->bg = DFLT_BG_COLOR;

	memcpy(g->color_table, nk_get_default_color_table(), sizeof(g->color_table));
	// TODO config
	g->color_table[NK_COLOR_WINDOW].a = DFLT_WINDOW_OPACITY;
	
	for (int i=1; i<argc; ++i) {
		if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--slide-show")) {
			int delay;
			if (i+1 == argc) {
				SDL_Log("No delay following -s, defaulting to 3 second delay.");
				delay = 3;
			} else {
				char* end;
				delay = strtol(argv[++i], &end, 10);
				if (delay <= 0 || delay > 10) {
					if (delay == 0 && end == argv[i]) {
						SDL_Log("No time given for -s, defaulting to 3 seconds\n");
						i--;
					} else {
						SDL_Log("Invalid slideshow time given %d (should be 1-10), defaulting to 3 seconds\n", delay);
					}
					delay = 3;
				}
			}
			g->slideshow = delay*1000;
		} else if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--cache")) {
			if (i+1 == argc) {
				SDL_Log("no cache directory provieded, using default cache location.\n");
			} else {
				if (mkdir_p(argv[++i], S_IRWXU) && errno != EEXIST) {
					SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to make cache directory %s: %s\n", argv[i], strerror(errno));
					cleanup(1, 0);
				}
				g->cachedir = argv[i];
			}
		} else if (!strcmp(argv[i], "-f") || !strcmp(argv[i], "--fullscreen")) {
			g->fullscreen = 1;
		} else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version")) {
			puts(VERSION_STR);
			cleanup(1, 0);
		} else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			print_help(argv[0], SDL_TRUE);
			cleanup(1, 0);
		}

	}

	int got_config = load_config();

	// TODO
	//FILE* controls_file = fopen("src/controls.txt", "r");
	//g->ct_len = file_read(controls_file, &g->controls_text);
	g->controls_text = (char*)controls_text;
	g->ct_len = strlen(controls_text);

	// already NULL from static initialization
	g->win = NULL;
	g->ren = NULL;

	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		snprintf(error_str, STRBUF_SZ, "Couldn't initialize SDL: %s; exiting.", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error_str, g->win);
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "%s\n", error_str);
		exit(1);
	}


	if (curl_global_init(CURL_GLOBAL_ALL)) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to initialize libcurl\n");
		cleanup(1, 0);
	}
	cvec_file(&g->files, 0, 100, free_file, NULL);
	cvec_str(&g->favs, 0, 50);
	cvec_str(&g->playlists, 0, 50);
	// g->thumbs initialized if needed in generate_thumbs()

	// Call this after creating logfile
	setup_dirs();

// NOTE by doing it here we miss all log calls above but code above could change
#ifdef USE_LOGFILE
	char log_path[STRBUF_SZ];
	char datebuf[200] = { 0 };
	time_t t = time(NULL);
	struct tm *tmp = localtime(&t);
	strftime(datebuf, sizeof(datebuf), "%Y-%m-%d_%H-%M-%S", tmp);  // %F
	snprintf(log_path, sizeof(log_path), "%s/log_%s.txt", g->logdir, datebuf);
	g->logfile = fopen(log_path, "w");
	SDL_LogSetOutputFunction(log_output_func, g->logfile);
#endif


	// TODO some of these could be stored preferences?
	g->n_imgs = 1;
	g->img = g->img1;
	g->do_next = nk_false;
	g->progress_hovered = nk_false;
	g->sorted_state = NAME_UP;  // ie by name ascending
	g->bad_path_state = CLEAN;
	if (g->sources.size) {
		g->state = SCANNING;
	} else {
		g->state = FILE_SELECTION;
 		// need this in case they exit FS so cleanup can signal it awake to exit
		// plus it makes logical sense, there's nothing to scan yet so it's done till it gets
		// set to 0 in start_scanning()
		g->done_scanning = 1;
	}

	init_db();
	create_playlist(g->default_playlist); // usually no-op
	get_sql_playlists();
	g->cur_playlist_id = get_playlist_id(g->default_playlist);
	strncpy(g->cur_playlist_buf, g->default_playlist, STRBUF_SZ);
	g->cur_playlist = g->cur_playlist_buf;

	SDL_Rect r;
	if (SDL_GetDisplayUsableBounds(0, &r)) {
		SDL_Log("Error getting usable bounds: %s\n", SDL_GetError());
		r.w = START_WIDTH;
		r.h = START_HEIGHT;
	} else {
		SDL_Log("Usable Bounds: %d %d %d %d\n", r.x, r.y, r.w, r.h);
	}

	if (!(g->img[0].tex = malloc(100*sizeof(SDL_Texture*)))) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Couldn't allocate tex array: %s\n", strerror(errno));
		cleanup(0, 1);
	}
	g->img[0].frame_capacity = 100;

	g->scr_w = START_WIDTH;
	g->scr_h = START_HEIGHT;

	g->scr_w = MIN(g->scr_w, r.w - 20);  // to account for window borders/titlebar on non-X11 platforms
	g->scr_h = MIN(g->scr_h, r.h - 40);

	int max_w, max_h;
	float hdpi = 0, vdpi = 0, ddpi = 0;
	if (!SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi))
		SDL_Log("DPIs: %.2f %.2f %.2f\n", ddpi, hdpi, vdpi);
	if (!SDL_GetDisplayBounds(0, &r)) {
		SDL_Log("Display Bounds: %d %d %d %d\n", r.x, r.y, r.w, r.h);
		if (hdpi && vdpi && ddpi)
			SDL_Log("Physical Screen size: %f %f %f\n", r.w / hdpi, r.h / vdpi, sqrt(r.w*r.w + r.h*r.h) / ddpi);
	}

	u32 win_flags = SDL_WINDOW_RESIZABLE;
	win_flags |= SDL_WINDOW_ALLOW_HIGHDPI;

	if (g->fullscreen) {
		win_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}

	// just experimenting
	//win_flags |= SDL_WINDOW_BORDERLESS;

	// TODO do I need to update scr_w and src_h if it's fullscreen?  is there an initial window event?

	snprintf(title_buf, STRBUF_SZ, "Select File/Folder");

	g->win = SDL_CreateWindow(title_buf, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, g->scr_w, g->scr_h, win_flags);
	if (!g->win) {
		snprintf(error_str, STRBUF_SZ, "Couldn't create window: %s; exiting.", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error_str, g->win);
		SDL_LogCriticalApp("%s", error_str);
		exit(1);
	}

	if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2")) {
		SDL_Log("render quality hint was not set\n");
	} else {
		SDL_Log("render quality hint was set\n");
	}

	// Handle window resizing etc. in a single place
	SDL_SetEventFilter(handle_common_evts, NULL);

	// GetWindowBorderSize is only supported on X11 (as of 2019)
	int top, bottom, left, right;
	if (!g->fullscreen && !SDL_GetWindowBordersSize(g->win, &top, &bottom, &left, &right)) {
		SDL_Log("border (lrtb): %d %d %d %d\n", left, right, top, bottom);
		SDL_Log("scr_w scr_h before: %d %d\n", g->scr_w, g->scr_h);
		g->scr_w -= left + right;
		g->scr_h -= top + bottom;
		SDL_Log("scr_w scr_h after: %d %d\n", g->scr_w, g->scr_h);
		SDL_SetWindowSize(g->win, g->scr_w, g->scr_h);
		SDL_SetWindowPosition(g->win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	}

#ifdef USE_SOFTWARE_RENDERER
	int ren_flags = SDL_RENDERER_SOFTWARE;
#else
	int ren_flags = SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC;
#endif
	g->ren = SDL_CreateRenderer(g->win, -1, ren_flags);
	if (!g->ren) {

#ifdef USE_SOFTWARE_RENDERER
		snprintf(error_str, STRBUF_SZ, "Creating a software renderer failed: %s; exiting.", SDL_GetError());
#else
		snprintf(error_str, STRBUF_SZ, "Creating a HW-accelerated renderer failed: %s; exiting.", SDL_GetError());
#endif
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error_str, g->win);
		SDL_LogCriticalApp("%s", error_str);
		cleanup(1, 1);
	}

	SDL_Log("Render hint %s\n", SDL_GetHint(SDL_HINT_RENDER_SCALE_QUALITY));

	SDL_GetWindowMaximumSize(g->win, &max_w, &max_h);
	SDL_Log("Window Max dimensions: %d %d\n", max_w, max_h);

	// init file browser
#ifndef _WIN32
	init_file_browser(&g->filebrowser, default_exts, NUM_DFLT_EXTS, NULL, linux_recents, NULL);
#else
	init_file_browser(&g->filebrowser, default_exts, NUM_DFLT_EXTS, NULL, windows_recents, NULL);
#endif
	g->filebrowser.selection = -1; // default to no selection
	g->is_open_new = SDL_TRUE;

	if (!(g->ctx = nk_sdl_init(g->win, g->ren))) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "nk_sdl_init() failed!\n");
		cleanup(1, 1);
	}

	// TODO Font stuff, refactor/reorganize
	if (!got_config) {
		int render_w, render_h;
		int window_w, window_h;
		SDL_GetRendererOutputSize(g->ren, &render_w, &render_h);
		SDL_GetWindowSize(g->win, &window_w, &window_h);
		g->x_scale = (float)(render_w) / (float)(window_w);
		g->y_scale = (float)(render_h) / (float)(window_h);
	} else {
		// We might have read GUI colors from config so apply them
		// (worst case it's a no-op since we initialize color_table to default above
		nk_style_from_table(g->ctx, g->color_table);

	}
	// TODO could adjust for dpi, then adjust for font size if necessary
	//g->x_scale = 2; //hdpi/72;
	//g->y_scale = 2; //vdpi/72;

	SDL_Log("scale %f %f\n", g->x_scale, g->y_scale);
	nk_sdl_scale(g->x_scale, g->y_scale);

	// by default nuklear has pixel_snap off, oversample_h = 3
	// but things look better with oversample off (ie =1) and pixel_snap on so...
	setup_font(NULL, g->font_size);

	g->scr_rect.y = g->gui_bar_ht;
	g->scr_rect.w = g->scr_w;
	g->scr_rect.h = g->scr_h - 2*g->gui_bar_ht;

	// first load will switch and copy img[0].scr_rect
	g->img = g->img2;

	// Should be a way to use existing code for this but there isn't without changes/additions
	g->img[0].scr_rect.x = g->scr_rect.x;
	g->img[0].scr_rect.y = g->scr_rect.y;
	g->img[0].scr_rect.w = g->scr_rect.w;
	g->img[0].scr_rect.h = g->scr_rect.h;

	// Trying to figure out/fix why menu_item_labels are wider than selectables
	//g->ctx->style.selectable.padding = nk_vec2(4.0f,4.0f);
	//g->ctx->style.selectable.touch_padding = nk_vec2(4.0f,4.0f);

	// type of event for all GUI initiated events
	g->userevent = SDL_RegisterEvents(1);
	if (g->userevent == (u32)-1) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Error: %s\n", SDL_GetError());
		cleanup(0, 1);
	}

	if (!(g->thumb_cnd = SDL_CreateCond())) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR,"Error: %s\n", SDL_GetError());
		cleanup(0, 1);
	}

	if (!(g->thumb_mtx = SDL_CreateMutex())) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Error: %s\n", SDL_GetError());
		cleanup(0, 1);
	}

	if (!(g->jit_thumb_cnd = SDL_CreateCond())) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR,"Error: %s\n", SDL_GetError());
		cleanup(0, 1);
	}

	if (!(g->jit_thumb_mtx = SDL_CreateMutex())) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Error: %s\n", SDL_GetError());
		cleanup(0, 1);
	}

	SDL_Thread* jit_thumb_thrd;
	if (!(jit_thumb_thrd = SDL_CreateThread(jit_thumbs, "jit_thumb_thrd", NULL))) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "couldn't create image loader thread: %s\n", SDL_GetError());
		cleanup(0, 1);
	}
	SDL_DetachThread(jit_thumb_thrd);

	if (!(g->img_loading_cnd = SDL_CreateCond())) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR,"Error: %s\n", SDL_GetError());
		cleanup(0, 1);
	}

	if (!(g->img_loading_mtx = SDL_CreateMutex())) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Error: %s\n", SDL_GetError());
		cleanup(0, 1);
	}

	SDL_Thread* loading_thrd;
	if (!(loading_thrd = SDL_CreateThread(load_new_images, "loading_thrd", NULL))) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "couldn't create image loader thread: %s\n", SDL_GetError());
		cleanup(0, 1);
	}
	SDL_DetachThread(loading_thrd);

	if (!(g->scanning_cnd = SDL_CreateCond())) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR,"Error: %s\n", SDL_GetError());
		cleanup(0, 1);
	}

	if (!(g->scanning_mtx = SDL_CreateMutex())) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Error: %s\n", SDL_GetError());
		cleanup(0, 1);
	}

	SDL_Thread* scanning_thrd;
	if (!(scanning_thrd = SDL_CreateThread(scan_sources, "scanning_thrd", NULL))) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "couldn't create scanner thread: %s\n", SDL_GetError());
		cleanup(0, 1);
	}
	SDL_DetachThread(scanning_thrd);


	// Setting both of these last to maximize time/accuracy
	g->gui_timer = SDL_GetTicks();
	g->show_gui = nk_true;
}

void cleanup(int ret, int called_setup)
{
	//char buf[STRBUF_SZ] = { 0 };

	int start_time, cur_time;;
	g->is_exiting = SDL_TRUE;

	SDL_LogDebugApp("In cleanup()");
	if (called_setup) {

		// not really necessary to exit detached threads but for completion's sake
		// and to get rid of Valgrind's "possibly lost warnings"

		if (g->generating_thumbs) {
			// wait for thread to exit
			SDL_LockMutex(g->thumb_mtx);
			while (!g->thumbs_done) {
				SDL_LogDebugApp("Waiting for thumb generating thread to exit...\n");
				SDL_CondWait(g->thumb_cnd, g->thumb_mtx);
			}
			SDL_UnlockMutex(g->thumb_mtx);
		}

		if (g->loading_thumbs) {
			// wait for thread to exit
			SDL_LockMutex(g->thumb_mtx);
			while (!g->thumbs_loaded) {
				SDL_LogDebugApp("Waiting for thumb loading thread to exit...\n");
				SDL_CondWait(g->thumb_cnd, g->thumb_mtx);
			}
			SDL_UnlockMutex(g->thumb_mtx);
		}

		if (!g->jit_thumb_flag) {
			SDL_LogDebugApp("Waking/Signaling jit_thumb thread so it can exit...\n");
			SDL_LockMutex(g->jit_thumb_mtx);
			SDL_CondSignal(g->jit_thumb_cnd);
			SDL_UnlockMutex(g->jit_thumb_mtx);
		}

		SDL_LockMutex(g->jit_thumb_mtx);
		start_time = SDL_GetTicks();
		while (g->jit_thumb_flag != EXIT) {
			if ((cur_time = SDL_GetTicks()) - start_time >= 2000) {
				SDL_LogDebugApp("Waiting for jit_thumb thread to exit...");
				start_time = cur_time;
			}
			SDL_CondWait(g->jit_thumb_cnd, g->jit_thumb_mtx);
		}
		SDL_UnlockMutex(g->jit_thumb_mtx);


		if (g->done_scanning) {
			SDL_LogDebugApp("Waking/Signaling Scanning thread so it can exit...\n");
			SDL_LockMutex(g->scanning_mtx);
			SDL_CondSignal(g->scanning_cnd);
			SDL_UnlockMutex(g->scanning_mtx);
		}

		SDL_LockMutex(g->scanning_mtx);
		start_time = SDL_GetTicks();
		while (g->done_scanning != EXIT) {
			if ((cur_time = SDL_GetTicks()) - start_time >= 2000) {
				SDL_LogDebugApp("Waiting for scanning thread to exit...");
				start_time = cur_time;
			}
			SDL_CondWait(g->scanning_cnd, g->scanning_mtx);
		}
		SDL_UnlockMutex(g->scanning_mtx);

		// TODO get rid of this requirement
		// First wait for anything currently loading to exit
		start_time = SDL_GetTicks();
		while (g->loading) {
			if ((cur_time = SDL_GetTicks()) - start_time >= 2000) {
				SDL_LogDebugApp("Waiting for loading to finish...");
				start_time = cur_time;
			}
		}

		//try_move(EXIT); // can't do this because done_loading
		SDL_LockMutex(g->img_loading_mtx);
		g->loading = EXIT;
		SDL_Log("Sending EXIT to loading thread");
		SDL_CondSignal(g->img_loading_cnd);
		SDL_UnlockMutex(g->img_loading_mtx);

		// Now just in case the rest of cleanup() went to quickly, we
		// actually wait for it to exit
		SDL_LockMutex(g->img_loading_mtx);
		while (g->loading) {
			SDL_LogDebugApp("Waiting for loading to exit...\n");
			SDL_CondWait(g->img_loading_cnd, g->img_loading_mtx);
		}
		SDL_UnlockMutex(g->img_loading_mtx);



		// appends prefpath inside
		write_config_file("config.lua");
#ifndef NDEBUG
		write_config(stdout);
#endif

		free(g->default_playlist);

		// free allocated img exts if we read them from config file
		if (g->cfg_img_exts) {
			for (int i=0; i<g->n_exts; ++i) {
				free((void*)g->img_exts[i]);
			}
			free(g->img_exts);
		}

		for (int i=0; i<g->n_imgs; ++i) {
			clear_img(&g->img[i]);
		}
		for (int i=0; i<8; ++i) {
			free(g->img1[i].tex);
			free(g->img2[i].tex);
		}

		shutdown_db();

		// Have to free these *before* Destroying the Renderer and
		// exiting SDL
		cvec_free_thumb_state(&g->thumbs);
		nk_sdl_shutdown();

		// Exit SDL and close logfile last so we can use SDL_Log*() above and in other threads
		// till the end
		SDL_DestroyRenderer(g->ren);
		SDL_DestroyWindow(g->win);
		if (g->logfile) {
			fclose(g->logfile);
		}

		SDL_Quit();
	}

	free(g->lua_error);
	free(g->prefpath);
	free_file_browser(&g->filebrowser);
	cvec_free_file(&g->files);
	cvec_free_i(&g->search_results);
	cvec_free_str(&g->favs);
	cvec_free_str(&g->playlists);
	curl_global_cleanup();
	exit(ret);
}
