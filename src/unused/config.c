
enum {
	GUI_SCALE,
	BACKGROUND,
	SLIDE_DELAY,
	HIDE_GUI_DELAY,
	FULLSCREEN_GUI,
	THUMB_ROWS,
	THUMB_COLS,
	SHOW_INFO_BAR,
	X_DELETES_THUMB,
	RELATIVE_OFFSETS,
	CACHE_DIR,
	NUM_KEYS
};

char* keys[] =
{
	"gui_scale",
	"background",
	"slide_delay",
	"hide_gui_delay",
	"fullscreen_gui",
	"thumb_rows",
	"thumb_cols",
	"show_info_bar",
	"x_deletes_thumb",
	"relative_offsets",
	"cache_dir"
};

int find_key(char* key)
{
	for (int i=0; i<NUM_KEYS; i++) {
		if (!strcmp(key, keys[i])) {
			return i;
		}
	}
	return -1;
}

int read_config(char* filename)
{
	char* s;
	char line[STRBUF_SZ] = { 0 };
	char key[101] = { 0 };
	char val[401] = { 0 };
	int len;
	char* sep;
	int red,green,blue;
	float scale, tmp;

	snprintf(line, STRBUF_SZ, "%s/%s", g->prefpath, filename);
	FILE* cfg_file = fopen(line, "r");
	if (!cfg_file) {
		return 0;
	}

	while ((s = fgets(line, STRBUF_SZ, cfg_file))) {
		if (s[0] == '#' || s[0] == '\n')
			continue;

		// TODO/NOTE not allowing spaces in cache_dir path
		// maybe later split with strchr and use two separate sscanfs?
		if (2 != sscanf(line, " %100[^ :] : %400[^ \n]", key, val)) {
			continue;
		}
		//printf("'%s' : '%s'\n", key, val);
		int k = find_key(key);

		switch (k) {
		case GUI_SCALE:
			// TODO
			sscanf(val, "%f", &scale);
			if (scale < 1.0f) {
				scale = 1.0f;
			}
			// make sure only 0.5 increments
			tmp = floor(scale);
			if (tmp != scale) {
				scale = tmp + 0.5f;
			}
			g->y_scale = g->x_scale = scale;

			break;
		case BACKGROUND:
			sscanf(val, "%d,%d,%d", &red, &green, &blue);
			g->bg = nk_rgb(red,green,blue); // clamps for us

			break;
		case SLIDE_DELAY:
			sscanf(val, "%u", &g->slide_delay);
			if (g->slide_delay > MAX_SLIDE_DELAY) {
				g->slide_delay = MAX_SLIDE_DELAY;
			}
			break;
		case HIDE_GUI_DELAY:
			sscanf(val, "%u", &g->gui_delay);
			if (g->gui_delay > MAX_GUI_DELAY) {
				g->gui_delay = MAX_GUI_DELAY;
			}
			break;
		case FULLSCREEN_GUI:
			if (!strcmp(val, "delay")) {
				g->fullscreen_gui = DELAY;
			} else if (!strcmp(val, "always")) {
				g->fullscreen_gui = ALWAYS;
			} else if (!strcmp(val, "never")) {
				g->fullscreen_gui = NEVER;
			} else {
				printf("Error: invalid value for fullscreen_gui: '%s'\n", val);
				puts("defaulting to 'delay'");
				g->fullscreen_gui = DELAY;
			}
			break;
		case THUMB_ROWS:
			sscanf(val, "%d", &g->thumb_rows);
			g->thumb_rows = NK_CLAMP(2, g->thumb_rows, 8);
			break;
		case THUMB_COLS:
			sscanf(val, "%d", &g->thumb_cols);
			g->thumb_cols = NK_CLAMP(4, g->thumb_cols, 15);
			break;


		// anything not "true" is false
		case SHOW_INFO_BAR:
			g->show_infobar = !strcmp(val, "true");
			break;
		case X_DELETES_THUMB:
			g->thumb_x_deletes = !strcmp(val, "true");
			break;
		case RELATIVE_OFFSETS:
			g->ind_mm = !strcmp(val, "true");
			break;

		case CACHE_DIR:
			// NOTE g->cachedir is set to point to local array in main()
			//strcpy(g->cachedir, val);
			break;
		default:
			printf("Error: Ignoring invalid key: '%s'\n", key);
		}
	}
	fclose(cfg_file);
	return 1;
}


int write_config(char* filename)
{
	const char* bool_str[] = { "false", "true" };
	const char* fullscreen_gui_str[] = { "delay", "always", "never" };

	char filepath[STRBUF_SZ];
	snprintf(filepath, STRBUF_SZ, "%s/%s", g->prefpath, filename);
	FILE* cfg_file = fopen(filepath, "w");
	if (!cfg_file) {
		return 0;
	}

	for (int i=0; i<NUM_KEYS; i++) {

		fprintf(cfg_file, "%s: ", keys[i]);

		switch (i) {
		case GUI_SCALE:
			// TODO either save x and y scale separately or combine
			// into a single member g->scale if they're always the same
			fprintf(cfg_file, "%.1f\n", g->x_scale);
			break;
		case BACKGROUND:
			fprintf(cfg_file, "%u,%u,%u\n", g->bg.r, g->bg.g, g->bg.b);
			break;
		case SLIDE_DELAY:
			fprintf(cfg_file, "%d\n", g->slide_delay);
			break;
		case HIDE_GUI_DELAY:
			fprintf(cfg_file, "%d\n", g->gui_delay);
			break;
		case FULLSCREEN_GUI:
			fprintf(cfg_file, "%s\n", fullscreen_gui_str[g->fullscreen_gui]);
			break;
		case THUMB_ROWS:
			fprintf(cfg_file, "%d\n", g->thumb_rows);
			break;
		case THUMB_COLS:
			fprintf(cfg_file, "%d\n", g->thumb_cols);
			break;


		// anything not "true" is false
		case SHOW_INFO_BAR:
			fprintf(cfg_file, "%s\n", bool_str[g->show_infobar]);
			break;
		case X_DELETES_THUMB:
			fprintf(cfg_file, "%s\n", bool_str[g->thumb_x_deletes]);
			break;
		case RELATIVE_OFFSETS:
			fprintf(cfg_file, "%s\n", bool_str[g->ind_mm]);
			break;

		case CACHE_DIR:
			//fprintf(cfg_file, "%s\n", g->cachedir);
			break;
		}
	}
	fclose(cfg_file);
	return 1;
}
