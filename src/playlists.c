

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

		// handle quoted paths
		if ((s[len-1] == '"' || s[len-1] == '\'') && s[len-1] == s[0]) {
			s[len-1] = 0;
			memmove(s, &s[1], len-2);
			len -= 2;
			s[len] = 0;
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

