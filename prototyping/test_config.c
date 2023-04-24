#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define STRBUF_SZ 1024
typedef struct rgb
{
	uint8_t r,g,b;
} rgb;

enum { DELAY, ALWAYS, NEVER };

#define MAX_SLIDE_DELAY 10
#define MAX_GUI_DELAY 60

rgb bg;
int slide_delay;
int hide_gui_delay;

int fullscreen_gui;

int thumb_rows;
int thumb_cols;
int show_info_bar;
int x_deletes_thumb;
int relative_offsets;
char cache_dir[STRBUF_SZ];

int read_config(FILE* cfg_file);

char* fullscreen_gui_str(int fsg_enum)
{
	switch (fsg_enum) {
	case DELAY: return "delay";
	case ALWAYS: return "always";
	case NEVER: return "never";
	default: return "INVALID";
	}
}

int main(int argc, char** argv)
{
	printf("%d,%d,%d\n", bg.r, bg.g, bg.b);
	printf("%d\n", slide_delay);
	printf("%d\n", hide_gui_delay);
	printf("%d %s\n", fullscreen_gui, fullscreen_gui_str(fullscreen_gui));
	printf("%d\n", thumb_rows);
	printf("%d\n", thumb_cols);
	printf("%d\n", show_info_bar);
	printf("%d\n", x_deletes_thumb);
	printf("%d\n", relative_offsets);
	printf("'%s'\n", cache_dir);


	FILE* file = fopen("config.txt", "r");
	read_config(file);
	fclose(file);

	printf("%d,%d,%d\n", bg.r, bg.g, bg.b);
	printf("%d\n", slide_delay);
	printf("%d\n", hide_gui_delay);
	printf("%d %s\n", fullscreen_gui, fullscreen_gui_str(fullscreen_gui));
	printf("%d\n", thumb_rows);
	printf("%d\n", thumb_cols);
	printf("%d\n", show_info_bar);
	printf("%d\n", x_deletes_thumb);
	printf("%d\n", relative_offsets);
	printf("'%s'\n", cache_dir);

	file = fopen("config_out.txt", "w");
	write_config(file);
	fclose(file);

	return 0;
}

enum {
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

#define CLAMP_255(a) ((a) > 255) ? 255 : (a)
#define CLAMP_ab(x, a, b) ((x) < (a)) ? (a) : (((x) > (b)) ? (b) : (x))

int read_config(FILE* cfg_file)
{
	char* s;
	char line[STRBUF_SZ] = { 0 };
	char key[101] = { 0 };
	char val[401] = { 0 };
	int len;
	char* sep;

	unsigned int r,g,b;


	while ((s = fgets(line, STRBUF_SZ, cfg_file))) {
		if (s[0] == '#' || s[0] == '\n')
			continue;

		// TODO/NOTE not allowing spaces in cache_dir path
		// maybe split with strchr and use two separate sscanfs?
		if (2 != sscanf(line, " %100[^ :] : %400[^ \n]", key, val)) {
			continue;
		}

		printf("'%s' : '%s'\n", key, val);

		int k = find_key(key);

		switch (k) {
		case BACKGROUND:
			sscanf(val, "%u,%u,%u", &r, &g, &b);
			bg.r = CLAMP_255(r);
			bg.g = CLAMP_255(g);
			bg.b = CLAMP_255(b);

			break;
		case SLIDE_DELAY:
			sscanf(val, "%u", &slide_delay);
			if (slide_delay > MAX_SLIDE_DELAY) {
				slide_delay = MAX_SLIDE_DELAY;
			}
			break;
		case HIDE_GUI_DELAY:
			sscanf(val, "%u", &hide_gui_delay);
			if (hide_gui_delay > MAX_GUI_DELAY) {
				hide_gui_delay = MAX_GUI_DELAY;
			}
			break;
		case FULLSCREEN_GUI:
			if (!strcmp(val, "delay")) {
				fullscreen_gui = DELAY;
			} else if (!strcmp(val, "always")) {
				fullscreen_gui = ALWAYS;
			} else if (!strcmp(val, "never")) {
				fullscreen_gui = NEVER;
			} else {
				printf("Error: invalid value for fullscreen_gui: '%s'\n", val);
				puts("defaulting to 'delay'");
			}
			break;
		case THUMB_ROWS:
			sscanf(val, "%d", &thumb_rows);
			thumb_rows = CLAMP_ab(thumb_rows, 2, 8);
			break;
		case THUMB_COLS:
			sscanf(val, "%d", &thumb_cols);
			thumb_cols = CLAMP_ab(thumb_cols, 4, 15);
			break;


		// anything not "true" is false
		case SHOW_INFO_BAR:
			show_info_bar = !strcmp(val, "true");
			break;
		case X_DELETES_THUMB:
			x_deletes_thumb = !strcmp(val, "true");
			break;
		case RELATIVE_OFFSETS:
			relative_offsets = !strcmp(val, "true");
			break;

		case CACHE_DIR:
			strcpy(cache_dir, val);
			break;
		default:
			printf("Error: Ignoring invalid key: '%s'\n", key);
		}
	}
	return 1;
}


int write_config(FILE* cfg_file)
{
	const char* bool_str[] = { "false", "true" };

	for (int i=0; i<NUM_KEYS; i++) {

		fprintf(cfg_file, "%s: ", keys[i]);

		switch (i) {
		case BACKGROUND:
			fprintf(cfg_file, "%u,%u,%u\n", bg.r, bg.g, bg.b);
			break;
		case SLIDE_DELAY:
			fprintf(cfg_file, "%d\n", slide_delay);
			break;
		case HIDE_GUI_DELAY:
			fprintf(cfg_file, "%d\n", hide_gui_delay);
			break;
		case FULLSCREEN_GUI:
			fprintf(cfg_file, "%s\n", fullscreen_gui_str(fullscreen_gui));
			break;
		case THUMB_ROWS:
			fprintf(cfg_file, "%d\n", thumb_rows);
			break;
		case THUMB_COLS:
			fprintf(cfg_file, "%d\n", thumb_cols);
			break;


		// anything not "true" is false
		case SHOW_INFO_BAR:
			fprintf(cfg_file, "%s\n", bool_str[show_info_bar]);
			break;
		case X_DELETES_THUMB:
			fprintf(cfg_file, "%s\n", bool_str[x_deletes_thumb]);
			break;
		case RELATIVE_OFFSETS:
			fprintf(cfg_file, "%s\n", bool_str[relative_offsets]);
			break;

		case CACHE_DIR:
			fprintf(cfg_file, "%s\n", cache_dir);
			break;
		}
	}
	return 1;
}
