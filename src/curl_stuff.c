
size_t write_data(void* buf, size_t size, size_t num, void* userp)
{
	return fwrite(buf, 1, size*num, (FILE*)userp);
}

char* curl_image(file* f)
{
	CURL* curl = curl_easy_init();
	CURLcode res;
	char filename[STRBUF_SZ];
	char curlerror[CURL_ERROR_SIZE];
	char* s = f->path;
	FILE* imgfile;

	// Do I even need to set WRITEFUNCTION?  It says it'll use fwrite by default
	// which is all I do...
	//curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlerror);
	//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	#ifdef _WIN32
	curl_easy_setopt(curl, CURLOPT_CAINFO, "ca-bundle.crt");
	curl_easy_setopt(curl, CURLOPT_CAPATH, SDL_GetBasePath());
	#endif

	char* slash = strrchr(s, PATH_SEPARATOR);
	if (!slash) {
		SDL_Log("invalid url\n");
		goto exit_cleanup;
	}
	int len = snprintf(filename, STRBUF_SZ, "%s/%s", g->cachedir, slash+1);
	if (len >= STRBUF_SZ) {
		SDL_Log("url too long\n");
		goto exit_cleanup;
	}

	SDL_Log("Getting %s\n%s\n", s, filename);
	if (!(imgfile = fopen(filename, "wb"))) {
		perror("fopen");

		goto exit_cleanup;
	}

	curl_easy_setopt(curl, CURLOPT_URL, s);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, imgfile);
	// follow redirect
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	res = curl_easy_perform(curl);
	long http_code = 0;
	char* content_type = NULL;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
	curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);
	if (res != CURLE_OK || http_code != 200 ||
		!content_type || strncmp(content_type, "image", 5)) {
		SDL_Log("curlcode: %d '%s'\nhttp_code: %ld", res, curlerror, http_code);
		if (!content_type) {
			SDL_Log("No content-type returned, ignoring\n");
		} else {
			SDL_Log("Not an image: %s\n", content_type);
		}
		fclose(imgfile);
		remove(filename);
		goto exit_cleanup;
	}
	fclose(imgfile);


	struct stat file_stat;
	stat(filename, &file_stat);
	// TODO don't think I need this any more, could also use
	// CURLINFO_CONTENT_LENGTH_DOWNLOAD_T
	/*
	if (!file_stat.st_size) {
		SDL_Log("file size is 0\n");
		remove(filename);
		goto exit_cleanup;
	} else {
		SDL_Log("file size is %ld\n", file_stat.st_size);
	}
	*/

	free(f->path);

	// Have to call realpath in case the user passed
	// a relative path cachedir as a command line argument
	char* tmp = myrealpath(filename, NULL);
	f->path = realloc(tmp, strlen(tmp)+1);
#ifdef _WIN32
	normalize_path(f->path);
#endif

	f->size = file_stat.st_size;
	f->modified = file_stat.st_mtime;

	bytes2str(f->size, f->size_str, SIZE_STR_BUF);
	struct tm* tmp_tm = localtime(&f->modified);
	strftime(f->mod_str, MOD_STR_BUF, "%Y-%m-%d %H:%M:%S", tmp_tm); // %F %T
	char* sep = strrchr(f->path, PATH_SEPARATOR); // TODO test on windows but I think I normalize
	f->name = (sep) ? sep+1 : f->path;


	curl_easy_cleanup(curl);
	return f->path;

exit_cleanup:
	curl_easy_cleanup(curl);
	return NULL;
}

