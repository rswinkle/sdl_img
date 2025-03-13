
// TODO put this elsewhere
#define GET_EXT(s) strrchr(s, '.')

int get_playlists(const char* dirpath)
{
	int i;
	char* ext = NULL;

	// clear playlists first (this function is used on startup and if playlist dir changes)
	cvec_clear_str(&g->playlists);

	struct dirent* entry;
	DIR* dir;

	dir = opendir(dirpath);
	if (!dir) {
		perror("opendir");
		cleanup(1, 1);
	}

	while ((entry = readdir(dir))) {

		// faster than 2 strcmp calls? ignore "." and ".."
		if (entry->d_name[0] == '.' && (!entry->d_name[1] || (entry->d_name[1] == '.' && !entry->d_name[2]))) {
			continue;
		}

		// TODO could pick a standard playlist extension or a subset of allowed extensions
		// (ie no extension, ".txt" ".dat" some others
		//
		// For now we just ignore at least the image extensions sdl_img has been configured to open
		// maybe we should move default_exts to global and use that so we get the full list even if
		// the user has configured a subset?
		ext = GET_EXT(entry->d_name);
		if (ext) {
			for (i=0; i<g->n_exts; ++i) {
				if (!strcasecmp(ext, g->img_exts[i]))
					break;
			}
			// skip image extensions we support
			if (i != g->n_exts)
				continue;
		}
		cvec_push_str(&g->playlists, entry->d_name);
	}

	closedir(dir);
	return 1;
}

// simple way to handle both cases.  Will remove paths when/if I switch to
// some other format for favorites, sqlite maybe?
void read_list(cvector_file* files, cvector_str* paths, FILE* list_file)
{
	char* s;
	char line[STRBUF_SZ] = { 0 };
	int len;
	file f = { 0 }; // 0 out time and size since we don't stat lists
	struct tm* tmp_tm;
	char* sep;

	struct stat file_stat;

	while ((s = fgets(line, STRBUF_SZ, list_file))) {
		// ignore comments in gqview/gthumb collection format useful
		// when combined with findimagedupes collection output
		if (s[0] == '#')
			continue;

		len = strlen(s);
		if (len < 2)
			continue;

		if (s[len-1] == '\n') {
			len--;
			s[len] = 0;
		}

		if (len < 2)
			continue;

		// TODO why did I do len-2 instead of len-1?
		// to handle quoted paths
		if ((s[len-2] == '"' || s[len-2] == '\'') && s[len-2] == s[0]) {
			s[len-2] = 0;
			memmove(s, &s[1], len-2);
			if (len < 4) continue;
		}
		normalize_path(s);

		if (files) {
			if (stat(s, &file_stat)) {
				// assume it's a valid url, it will just skip over if it isn't
				f.path = CVEC_STRDUP(s);
				f.size = 0;
				f.modified = 0;

				// TODO/NOTE leave whole url as name so user knows why size and modified are unknown
				// It does mean sorting by name is useless since they all start with http
				f.name = f.path;
				strncpy(f.size_str, "unknown", SIZE_STR_BUF);
				strncpy(f.mod_str, "unknown", MOD_STR_BUF);
				cvec_push_file(&g->files, &f);
			} else if (S_ISDIR(file_stat.st_mode)) {
				// Should I allow directories in a list?  Or make the user
				// do the expansion so the list only has files/urls?
				//
				//// TODO warning not info?
				SDL_Log("Skipping directory found in list, only files and urls allowed.\n%s\n", s);
			} else if(S_ISREG(file_stat.st_mode)) {
				f.path = CVEC_STRDUP(s);
				f.size = file_stat.st_size;
				f.modified = file_stat.st_mtime;

				bytes2str(f.size, f.size_str, SIZE_STR_BUF);
				tmp_tm = localtime(&f.modified);
				strftime(f.mod_str, MOD_STR_BUF, "%Y-%m-%d %H:%M:%S", tmp_tm); // %F %T
				sep = strrchr(f.path, PATH_SEPARATOR); // TODO test on windows but I think I normalize
				f.name = (sep) ? sep+1 : f.path;

				cvec_push_file(&g->files, &f);
			}
		}

		if (paths) {
			cvec_push_str(paths, s);
		}
	}
}

#define UPDATE_PLAYLIST_SAVE_STATUS() \
	do { \
	for (int i=0; i<g->files.size; ++i) { \
		g->files.a[i].playlist_idx = cvec_contains_str(&g->favs, g->files.a[i].path); \
	} \
	} while (0)


void read_cur_playlist(void)
{
	cvec_clear_str(&g->favs);
	FILE* f = NULL;
	if (!(f = fopen(g->cur_playlist_path, "r"))) {
		SDL_Log("%s does not exist, will try creating it\n", g->cur_playlist_path);
		f = fopen(g->cur_playlist_path, "w");
		if (!f) {
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to create %s: %s\n", g->cur_playlist_path, strerror(errno));
		} else {
			fclose(f);
			cvec_push_str(&g->playlists, g->cur_playlist);
		}
	} else {
		read_list(NULL, &g->favs, f);
		SDL_Log("Read %"PRIcv_sz" favorites from %s\n", g->favs.size, g->cur_playlist_path);
		fclose(f);
	}

	// update save status in new playlist
	// could have bad paths, could be in the middle of generating thumbs ... unless I remove
	// that feature only generating when switching to thumb mode where this code couldn't be
	// executing, I have to assume path could be NULLed out from under me
	g->save_status_uptodate = SDL_FALSE;
	if (!g->generating_thumbs && !g->loading_thumbs) {
		if (g->bad_path_state == HAS_BAD) {
			remove_bad_paths();
		}
		UPDATE_PLAYLIST_SAVE_STATUS();
		g->save_status_uptodate = SDL_TRUE;
	}

}

void write_cur_playlist(void)
{
	// TODO could add comments to the top of the file not sure what besides the name
	// or a last modified time (for the list, not the file of course)
	FILE* f = fopen(g->cur_playlist_path, "w");
	if (!f) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to create %s: %s\nAborting save\n", g->cur_playlist_path, strerror(errno));
	} else {
		SDL_Log("Saving %"PRIcv_sz" favorites to %s\n", g->favs.size, g->cur_playlist_path);
		for (int i=0; i<g->favs.size; i++) {
			fprintf(f, "%s\n", g->favs.a[i]);
		}
		fclose(f);
	}
}

// reads all playlists in playlistdir, finds default, reads it etc.
void update_playlists(void)
{
	char buf[STRBUF_SZ];
	// have to do this in two steps because cur_playlist points into cur_playlist_path
	if (!g->cur_playlist) {
		snprintf(buf, STRBUF_SZ, "%s/%s", g->playlistdir, g->default_playlist);
	} else {
		// save cur playlist before going to a new directory and reading a new playlist
		// of the same name;
		write_cur_playlist();
		snprintf(buf, STRBUF_SZ, "%s/%s", g->playlistdir, g->cur_playlist);
	}
	strcpy(g->cur_playlist_path, buf);
	g->cur_playlist = strrchr(g->cur_playlist_path, '/') + 1;

	read_cur_playlist();

	// TODO command line -p playlist.txt to be current playlist?
	// --favorites to open favorites?
	get_playlists(g->playlistdir);
}

