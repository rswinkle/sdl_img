
void do_file_select(int select_dir, intptr_t num_exts, const char** exts);
void do_file_open(int clear_files);
void do_lib_import(void);
void do_shuffle(void);
void do_sort(compare_func cmp);
void do_lib_sort(compare_func cmp);
void do_zoom(int dir, int use_mouse);
void do_pan(int dx, int dy);
void do_rotate(int left, int is_90);
void do_flip(int is_vertical);
void do_mode_change(intptr_t mode);
void do_remove(SDL_Event* next);
void do_delete(SDL_Event* next);
void do_actual_size(void);
int cvec_contains_str(cvector_str* list, char* s);
int do_copy(void);
void do_libmode(void);
void do_remove_from_lib(void);
