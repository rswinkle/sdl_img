
void init_db(void);
void shutdown_db(void);
void create_tables(sqlite3* db);
void prepare_stmts(sqlite3* db);
void finalize_stmts(void);
int create_playlist(const char* name);
int delete_playlist(const char* name);
int rename_playlist(const char* new_name, const char* old_name);
int get_sql_playlists(void);
int get_playlist_size(const char* name);
int insert_img(const char* path);
int get_playlist_id(const char* name);
int load_sql_playlist_name(const char* name, cvector_file* files);
int load_sql_playlist_id(int id, cvector_file* files);
int get_image_id(const char *path);
int sql_save(sqlite3_stmt* stmt, int idx, const char* path);
int sql_unsave(sqlite3_stmt* stmt, int idx, const char* path);
int do_sql_save_idx(int removing, int plist_id, int idx);
int do_sql_save(int removing, int plist_id);
int do_sql_save_all(int removing, cvector_file* files);
int add_file_to_db(const char* path);
int add_files_to_db(cvector_file* files);
int update_save_status(void);
int get_img_playlists(int idx);
int clean_library(void);
int remove_from_lib(const char* path);
int load_library(cvector_file* files, cvector_i* bad_img_ids, cvector_str* bad_img_paths);
int remove_bad_imgs(cvector_i* bad_img_ids);
int export_playlist(const char* name);
int export_playlists(void);



// split out single use like create vs many/most use?
enum {
	CREATE_TABLES,

	INSERT_IMG,
	INSERT_PLIST,
	INSERT_INTO_PLIST,

	RENAME_PLIST,

	// better names/match function names?
	GET_IMG_SAVE_STATUS,
	GET_IMG_SAVE_STATUSES,

	GET_PLIST_ID,
	GET_IMG_ID,
	GET_PLISTS,
	GET_LIBRARY,  // GET_ALL_IMGS?
	GET_PLIST_SZ,

	SELECT_ALL_IN_PLIST_ID, // GET_PLIST?

	DEL_PLIST_NAME,
	DEL_IMG,
	DEL_FROM_PLIST_ID,

// unused below here

	ADD_TO_PLIST,
	SELECT_ALL_IN_PLIST_NAME,

	DEL_PLIST_ID,
	DEL_FROM_PLIST_NAME,

	NUM_STMTS
};

//-- Full image path, unique to avoid duplicates
const char* sql[] = {
	"PRAGMA foreign_keys = ON;"
	"CREATE TABLE IF NOT EXISTS Images ("
		"image_id INTEGER PRIMARY KEY, "
		"path TEXT NOT NULL UNIQUE "
	");"
	"CREATE TABLE IF NOT EXISTS Playlists ("
		"playlist_id INTEGER PRIMARY KEY, "
		"name TEXT NOT NULL UNIQUE"
	");"
	"CREATE TABLE IF NOT EXISTS Playlist_Images ("
		"playlist_id INTEGER, "
		"image_id INTEGER, "
		"PRIMARY KEY (playlist_id, image_id), "
		"FOREIGN KEY (playlist_id) REFERENCES Playlists(playlist_id) ON DELETE CASCADE, "
		"FOREIGN KEY (image_id) REFERENCES Images(image_id) ON DELETE CASCADE"
	");",


	"INSERT OR IGNORE INTO Images (path) VALUES (?);",
	"INSERT OR IGNORE INTO Playlists (name) VALUES (?);",
	"INSERT OR IGNORE INTO Playlist_Images (playlist_id, image_id) VALUES (?, ?);",

	"UPDATE Playlists SET name = ? WHERE name = ?;",

	"SELECT 1 FROM Playlist_Images WHERE playlist_id = ? AND image_id = ?;",

	"SELECT p.playlist_id "  //, p.name
	"FROM Playlist_Images pi "
	"JOIN Playlists p ON p.playlist_id = pi.playlist_id "
	"WHERE pi.image_id = ?;",

	"SELECT playlist_id FROM Playlists WHERE name = ?;",
	"SELECT image_id FROM Images where path = ?;",
	"SELECT playlist_id, name FROM Playlists;",

	"SELECT * FROM Images;",

	"SELECT COUNT(*) FROM Playlist_Images WHERE playlist_id = ?;",

	//-- Get all image paths in playlist id
	"SELECT i.path FROM Images i "
	"JOIN Playlist_Images pi ON i.image_id = pi.image_id "
	"WHERE pi.playlist_id = ?;",

	//-- Delete a playlist (automatically removes entries from Playlist_Images due to CASCADE)
	"DELETE FROM Playlists WHERE name = ?;",

	//-- Delete an image (automatically removes it from all playlists)
	"DELETE FROM Images WHERE image_id = ?;",

	// Delete from playlist
	"DELETE FROM Playlist_Images WHERE playlist_id = ? AND image_id = ?;",

// unused below here

	"INSERT OR IGNORE INTO Playlist_Images (playlist_id, image_id) "
	"SELECT p.playlist_id, i.image_id "
	"FROM Playlists p, Images i "
	"WHERE p.name = ? AND i.path = ?;",

	// get all image paths in playlist name
	"SELECT i.path FROM Images i "
	"JOIN Playlist_Images pi ON i.image_id = pi.image_id "
	"WHERE pi.playlist_id = (SELECT playlist_id FROM Playlists WHERE name = ?);",

	"DELETE FROM Playlists WHERE playlist_id = ?;",

	"DELETE FROM Playlist_Images "
	"WHERE playlist_id = (SELECT playlist_id FROM Playlists WHERE name = ?) "
	"AND image_id = (SELECT image_id FROM Images WHERE path = ?);",


};

sqlite3_stmt* sqlstmts[NUM_STMTS];
//sqlite3* db;  //images, playlists, possibly thumbs in the future?


void init_db(void)
{
	char db_file[STRBUF_SZ];
	int len;

	char* prefpath = g->prefpath;

	len = snprintf(db_file, STRBUF_SZ, "%slibrary.db", prefpath);
	if (len >= STRBUF_SZ) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "library.db path too long\n");
		cleanup(1, 1);
	}

	if (sqlite3_open(db_file, &g->db)) {
		SDL_LogCriticalApp("Failed to open %s: %s\n", db_file, sqlite3_errmsg(g->db));
		cleanup(1, 1);
	}
	create_tables(g->db);
	prepare_stmts(g->db);
}

void shutdown_db(void)
{
	finalize_stmts();
	sqlite3_close(g->db);
}

void create_tables(sqlite3* db)
{
	char* errmsg = NULL;
	if (sqlite3_exec(db, sql[CREATE_TABLES], NULL, NULL, &errmsg)) {
		SDL_LogCriticalApp("Failed to create table: %s\n", errmsg);
		sqlite3_free(errmsg);
		cleanup(1, 1);
	}
}

void prepare_stmts(sqlite3* db)
{
	for (int i=INSERT_IMG; i<NUM_STMTS; ++i) {
		if (sqlite3_prepare_v2(db, sql[i], -1, &sqlstmts[i], NULL)) {
			SDL_LogCriticalApp("Failed to prep following statement:\n\"%s\"\nproduced errormsg: %s\n", sql[i], sqlite3_errmsg(db));
			exit(1);
			// TODO
			//cleanup(1, 1);
		}
	}
}

void finalize_stmts(void)
{
	for (int i=INSERT_IMG; i<NUM_STMTS; ++i) {
		if (sqlite3_finalize(sqlstmts[i])) {
			SDL_LogCriticalApp("failed to finalize sqlite statement\n");
			//printf("Failed to finalize following statement:\n\"%s\"\nproduced errormsg: %s\n", sql[i], sqlite3_errmsg(db));
			exit(1);
		}
	}
}

int create_playlist(const char* name)
{
	int len = strlen(name);
	if (len >= STRBUF_SZ) {
		SDL_Log("Playlist name is too long: %d >= %d\n", len, STRBUF_SZ);
		return FALSE;
	}

	sqlite3_stmt* stmt = sqlstmts[INSERT_PLIST];
	sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
	int rc;
	if ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
		SDL_Log("Failed add playlist %s: %s\n", name, sqlite3_errmsg(g->db));
		sqlite3_reset(stmt);
		return FALSE;
	}
	sqlite3_reset(stmt);
	return TRUE;
}

int delete_playlist(const char* name)
{
	int rc;
	sqlite3_stmt* stmt = sqlstmts[DEL_PLIST_NAME];
	sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
	sqlite3_step(stmt);

	if ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
		SDL_Log("Failed to delete playlist %s: %s\n", name, sqlite3_errmsg(g->db));
		sqlite3_reset(stmt);
		return FALSE;
	}

	sqlite3_reset(stmt);
	return TRUE;
}

int rename_playlist(const char* new_name, const char* old_name)
{
	int rc;
	sqlite3_stmt* stmt = sqlstmts[RENAME_PLIST];
	sqlite3_bind_text(stmt, 1, new_name, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, old_name, -1, SQLITE_STATIC);

	if ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
		SDL_Log("Failed to rename playlist %s: %s\n", old_name, sqlite3_errmsg(g->db));
		sqlite3_reset(stmt);
		return FALSE;
	}
	sqlite3_reset(stmt);

	return TRUE;
}

int get_sql_playlists(void)
{
	cvec_clear_str(&g->playlists);
	cvec_clear_i(&g->playlist_ids);
	sqlite3_stmt* stmt = sqlstmts[GET_PLISTS];
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		int id = sqlite3_column_int(stmt, 0);
		cvec_push_i(&g->playlist_ids, id);

		// TODO it returns const unsigned char*
		char *s = (char *)sqlite3_column_text(stmt, 1);
		cvec_push_str(&g->playlists, s);
	}
	sqlite3_reset(stmt);
	return TRUE;
}

int get_playlist_size(const char* name)
{
	int id;
	if (!(id = get_playlist_id(name))) {
		return -1;
	}

	sqlite3_stmt* stmt = sqlstmts[GET_PLIST_SZ];
	sqlite3_bind_int(stmt, 1, id);

	if (sqlite3_step(stmt) != SQLITE_ROW) {
		SDL_Log("Failed to get playlist size: %s\n", sqlite3_errmsg(g->db));
		sqlite3_reset(stmt);
		return -1;
	}

	int count = sqlite3_column_int(stmt, 0);
	sqlite3_reset(stmt);

	return count;
}

// Probably won't use this much by itself since it's slower than doing them all at once
int insert_img(const char* path)
{
	sqlite3_stmt* stmt = sqlstmts[INSERT_IMG];
	sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
	int rc;
	if ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
		SDL_Log("Failed to add image %s (%d): %s\n", path, rc, sqlite3_errmsg(g->db));
		sqlite3_reset(stmt);
		return FALSE;
	}
	sqlite3_reset(stmt);
	return TRUE;
}

int get_playlist_id(const char* name)
{
	int id;
	sqlite3_stmt* stmt = sqlstmts[GET_PLIST_ID];
	sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		id = sqlite3_column_int(stmt, 0);
	} else {
		// TODO check for error instead of SQLITE_DONE?
		sqlite3_reset(stmt);
		SDL_Log("No playlist with the name %s\n", name);
		return 0;
	}

	sqlite3_reset(stmt);

	return id;
}


// TODO use SELECT_ALL_IN_PLIST_NAME,
int load_sql_playlist_name(const char* name, cvector_file* files)
{
	int id;
	if (!(id = get_playlist_id(name))) {
		return FALSE;
	}

	SDL_Log("Added %d images from %s\n", load_sql_playlist_id(id, files), name);
	return TRUE;
}

// No URLs in database so if stat fails it was deleted or renamed
// Only regular files in database so if stat succeeds we can assume it's a regular file
// not a directory or link
int load_sql_playlist_id(int id, cvector_file* files)
{
	file f = {0};
	struct stat file_stat;
	struct tm* tmp_tm;
	char* sep;
	int n_imgs = 0;

	sqlite3_stmt* stmt = sqlstmts[SELECT_ALL_IN_PLIST_ID];
	sqlite3_bind_int(stmt, 1, id);  // playlist_id = 1
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		// We only store normalized fullpaths in db so no need to do anything with path
		const char *s = (const char *)sqlite3_column_text(stmt, 0);

		// TODO
		// have to get idx in current playlist
		//f.playlist_idx = i++;

		if (stat(s, &file_stat)) {
			// TODO can I run a delete statement in the middle of this one?
			// Should I?
			//
			// f.path = CVEC_STRDUP("Deleted"); // ?
			f.path = CVEC_STRDUP(s);
			f.size = 0;
			f.modified = 0;

			sep = strrchr(f.path, PATH_SEPARATOR); // TODO test on windows but I think I normalize
			f.name = (sep) ? sep+1 : f.path;

			f.name = f.path;
			strncpy(f.size_str, "unknown", SIZE_STR_BUF);
			strncpy(f.mod_str, "unknown", MOD_STR_BUF);
			cvec_push_file(files, &f);
		} else {
			f.path = CVEC_STRDUP(s);
			f.size = file_stat.st_size;
			f.modified = file_stat.st_mtime;

			bytes2str(f.size, f.size_str, SIZE_STR_BUF);
			tmp_tm = localtime(&f.modified);
			strftime(f.mod_str, MOD_STR_BUF, "%Y-%m-%d %H:%M:%S", tmp_tm); // %F %T
			sep = strrchr(f.path, PATH_SEPARATOR); // TODO test on windows but I think I normalize
			f.name = (sep) ? sep+1 : f.path;

			cvec_push_file(files, &f);
		}
		n_imgs++;
	}
	sqlite3_reset(stmt);
	SDL_Log("Added %d images from playlist %d\n", n_imgs, id);
	return n_imgs;
}

int get_image_id(const char *path)
{
    sqlite3_stmt *stmt = sqlstmts[GET_IMG_ID];
    int image_id = 0;
    assert(path);

    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        image_id = sqlite3_column_int(stmt, 0);
    } else {
        SDL_Log("Unexpected error: Image '%s' not found (should exist).\n", path);
        //assert(0 && "Image id doesn't exist!");

        sqlite3_reset(stmt);
        // SQLite rowid's start at 1 so returning 0 is a valid/unique error value
        return 0;
    }

    sqlite3_reset(stmt);
    return image_id;
}


// TODO check playlist_idx before actually touching the database and use that
// to tell whether it's not already saved or not there?
//
// Assumes stmt is INSERT_INTO_PLIST and playlist_id is already bound
int sql_save(sqlite3_stmt* stmt, int idx, const char* path)
{
	int img_id;
	if (!(img_id = get_image_id(path))) {
		assert(img_id >= 0 && "This should never happen");
	}

	sqlite3_bind_int(stmt, 2, img_id);

	int rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		SDL_Log("Failed to add image %s: %s\n", path, sqlite3_errmsg(g->db));
		sqlite3_reset(stmt);
		return FALSE;
	}

	if (sqlite3_changes(g->db)) {
		SDL_Log("saving %s\n", path);
	} else {
		// TODO propagate playlist name in or status out
		SDL_Log("%s already in that playlist\n", path);
	}
	sqlite3_reset(stmt);
	return TRUE;
}

// Assumes stmt is DEL_FROM_PLIST_ID and playlist_id is already bound
int sql_unsave(sqlite3_stmt* stmt, int idx, const char* path)
{
	int img_id;
	if (!(img_id = get_image_id(path))) {
		assert(img_id >= 0 && "This should never happen");
	}

	sqlite3_bind_int(stmt, 2, img_id);

	int rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		SDL_Log("Failed to remove image: %s\n", sqlite3_errmsg(g->db));
		sqlite3_reset(stmt);
		return FALSE;
	}

	int rows_affected = sqlite3_changes(g->db);
	if (rows_affected > 0) {
		SDL_Log("removing %s\n", path);
	} else {
		SDL_Log("%s not in %s\n", path, g->cur_playlist);
	}

	sqlite3_reset(stmt);
	return TRUE;
}

int do_sql_save_idx(int removing, int plist_id, int idx)
{
	if (g->loading)
		return FALSE;

	assert(idx >= 0);

	if (IS_RESULTS()) {
		idx = g->search_results.a[idx];
	}

	assert(idx >= 0 && g->list_view->size > idx);

	int ret;
	if (removing) {
		sqlite3_stmt* stmt = sqlstmts[DEL_FROM_PLIST_ID];
		sqlite3_bind_int(stmt, 1, plist_id);
		ret = sql_unsave(stmt, idx, g->list_view->a[idx].path);
	} else {
		sqlite3_stmt* stmt = sqlstmts[INSERT_INTO_PLIST];
		sqlite3_bind_int(stmt, 1, plist_id);
		ret = sql_save(stmt, idx, g->list_view->a[idx].path);
	}

	// TODO move this out? use g->cur_selected?  Or guard this with a parameter
	// instead
	if (g->list_view == &g->files && ret && plist_id == g->cur_playlist_id) {
		g->files.a[idx].playlist_idx = !removing;
	}

	return TRUE;
}

int do_sql_save(int removing, int plist_id)
{
	if (g->loading)
		return FALSE;

	int idx;
	
	img_state* imgs;
	int n_imgs;
	if (g->img_focus) {
		imgs = g->img_focus;
		n_imgs = 1;
	} else {
		imgs = g->img;
		n_imgs = g->n_imgs;
	}

	if (removing) {
		sqlite3_stmt* stmt = sqlstmts[DEL_FROM_PLIST_ID];
		sqlite3_bind_int(stmt, 1, plist_id);

		for (int i=0; i<n_imgs; i++) {
			idx = imgs[i].index;
			if (IS_VIEW_RESULTS()) {
				idx = g->search_results.a[idx];
			}
			if (sql_unsave(stmt, idx, imgs[i].fullpath)) {
				g->files.a[idx].playlist_idx = 0;
			}
		}
	} else {
		sqlite3_stmt* stmt = sqlstmts[INSERT_INTO_PLIST];
		sqlite3_bind_int(stmt, 1, plist_id);

		for (int i=0; i<n_imgs; i++) {
			idx = imgs[i].index;
			if (IS_VIEW_RESULTS()) {
				idx = g->search_results.a[idx];
			}
			if (sql_save(stmt, idx, imgs[i].fullpath)) {
				g->files.a[idx].playlist_idx = 1;
			}
		}
	}

	set_show_gui(SDL_TRUE);
	return TRUE;
}

int do_sql_save_all(int removing, cvector_file* files)
{
	int img_id;
	int cur_plist_id = g->cur_playlist_id;
	char* playlist = g->cur_playlist;
	int success_cnt = 0;
	int rc;
	cvec_sz already_there; // Or already not there
	
	const char* save_remove = NULL;
	sqlite3_stmt* stmt = NULL;

	if (removing) {
		stmt = sqlstmts[DEL_FROM_PLIST_ID];
		save_remove = "remove";
	} else {
		stmt = sqlstmts[INSERT_INTO_PLIST];
		save_remove = "save";
	}
	sqlite3_bind_int(stmt, 1, cur_plist_id);

	rc = sqlite3_exec(g->db, "BEGIN TRANSACTION", NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		SDL_Log("Failed to begin transaction: %s\n", sqlite3_errmsg(g->db));
		return FALSE;
	}

	if (IS_RESULTS()) {
		already_there = g->search_results.size;
		for (int j=0; j<g->search_results.size; j++) {
			int i = g->search_results.a[j];
			// skip URLS/bad files (note we allow empty images, see attempt_image_load())
			if (!files->a[i].modified) continue;

			img_id = get_image_id(files->a[i].path);
			assert(img_id > 0 && "This should never happen");

			sqlite3_bind_int(stmt, 2, img_id);

			rc = sqlite3_step(stmt);
			if (rc == SQLITE_DONE) {
				success_cnt += sqlite3_changes(g->db) > 0;
			} else {
				SDL_Log("Failed to %s image %s: %s\n", save_remove, files->a[i].path, sqlite3_errmsg(g->db));
				sqlite3_exec(g->db, "ROLLBACK", NULL, NULL, NULL);
				sqlite3_reset(stmt);
				return FALSE;
			}

			sqlite3_reset(stmt);
		}
	} else {
		already_there = files->size;
		for (int i=0; i<files->size; i++) {
			// skip URLS/bad files (note we allow empty images, see attempt_image_load())
			if (!files->a[i].modified) continue;

			img_id = get_image_id(files->a[i].path);
			assert(img_id > 0 && "This should never happen");

			sqlite3_bind_int(stmt, 2, img_id);

			rc = sqlite3_step(stmt);
			if (rc == SQLITE_DONE) {
				success_cnt += sqlite3_changes(g->db) > 0;
			} else {
				SDL_Log("Failed to %s image %s: %s\n", save_remove, files->a[i].path, sqlite3_errmsg(g->db));
				sqlite3_exec(g->db, "ROLLBACK", NULL, NULL, NULL);
				sqlite3_reset(stmt);
				return FALSE;
			}

			sqlite3_reset(stmt);
		}
	}

	rc = sqlite3_exec(g->db, "COMMIT", NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		SDL_Log("Failed to commit transaction: %s\n", sqlite3_errmsg(g->db));
		sqlite3_exec(g->db, "ROLLBACK", NULL, NULL, NULL);
		return FALSE;
	}

	// TODO Or call after this function when I know it was called on g->files?
	if (files == &g->files) {
		update_save_status();
	}

	already_there -= success_cnt;

	if (removing) {
		SDL_Log("Removed %d images from %s. %"PRIcv_sz" images were not there.\n", success_cnt, playlist, already_there);
	} else {
		SDL_Log("Saved %d new images to %s. %"PRIcv_sz" images were already there.\n", success_cnt, playlist, already_there);
	}

	return TRUE;
}

int add_file_to_db(const char* path)
{
	sqlite3_stmt* stmt = sqlstmts[INSERT_IMG];
	sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);

	if (sqlite3_step(stmt) != SQLITE_DONE) {
		SDL_Log("Failed to add image %s: %s\n", path, sqlite3_errmsg(g->db));
		sqlite3_reset(stmt);
		return FALSE;
	}
	sqlite3_reset(stmt);
	return TRUE;
}

// Do this or do it as we scan sources and stat() files?
// TODO more efficient...
int add_files_to_db(cvector_file* files)
{
	int rc;
	int n_added = 0;

	sqlite3_stmt* stmt = sqlstmts[INSERT_IMG];

	rc = sqlite3_exec(g->db, "BEGIN TRANSACTION", NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		SDL_Log("Failed to begin transaction: %s\n", sqlite3_errmsg(g->db));
		return FALSE;
	}

	for (int i=0; i<files->size; i++) {
		// skip URLS/bad files
		if (!files->a[i].modified) continue;

		sqlite3_bind_text(stmt, 1, files->a[i].path, -1, SQLITE_STATIC);

		// TODO decide whether to always check here or always check myself before
		// calling this function
		if ((rc = sqlite3_step(stmt)) == SQLITE_DONE) {
			n_added += sqlite3_changes(g->db) > 0;
			//files->a[idx].playlist_idx = 1;
		} else {
			SDL_Log("Failed to add image %s: %s\n", files->a[i].path, sqlite3_errmsg(g->db));
			sqlite3_exec(g->db, "ROLLBACK", NULL, NULL, NULL);
			sqlite3_reset(stmt);
			return FALSE;
		}
		sqlite3_reset(stmt);
	}

	rc = sqlite3_exec(g->db, "COMMIT", NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		SDL_Log("Failed to commit transaction: %s\n", sqlite3_errmsg(g->db));
		sqlite3_exec(g->db, "ROLLBACK", NULL, NULL, NULL);
		return FALSE;
	}

	SDL_Log("Added %d new images to database\n", n_added);

	return TRUE;
}

// TODO Use left join with temporary table or IN clause with batches if performance
// becomes an issue
int update_save_status(void)
{
	int img_id;
	int rc;
	int cur_plist_id = g->cur_playlist_id;

	SDL_Log("current playlist = %s\n", g->cur_playlist);

	sqlite3_stmt* stmt = sqlstmts[GET_IMG_SAVE_STATUS];
	sqlite3_bind_int(stmt, 1, cur_plist_id);

	for (int i=0; i<g->files.size; i++) {
		// skip URLS/bad files
		if (!g->files.a[i].modified) continue;

		if (!(img_id = get_image_id(g->files.a[i].path))) {
			SDL_Log("Failed to get img_id for %d at path %s\n", i, g->files.a[i].path);
			assert(img_id >= 0 && "This should never happen");
		}

		sqlite3_bind_int(stmt, 2, img_id);

		// in playlist if it was SQLITE_ROW or != SQLITE_DONE
		rc = sqlite3_step(stmt);
		//switch (rc) {
		//case SQLITE_DONE: SDL_Log("sqlite_done\n"); break;
		//case SQLITE_ROW: SDL_Log("sqlite_row\n"); break;
		//default: SDL_Log("something else %d\n", rc); break;
		//}

		//g->files.a[i].playlist_idx = sqlite3_step(stmt) == SQLITE_ROW;
		g->files.a[i].playlist_idx = rc != SQLITE_DONE;
		//SDL_Log("%s save status: %"PRIcv_sz"\n", g->files.a[i].path, g->files.a[i].playlist_idx);
		sqlite3_reset(stmt);
	}
	return TRUE;
}

// pass idx and path?
int get_img_playlists(int idx)
{
	int img_id;
	if (idx < 0) return FALSE;

	if (IS_RESULTS()) {
		idx = g->search_results.a[idx];
	}

	// set all to false
	memset(&g->img_saved_status, 0, g->playlist_ids.size*sizeof(int));

	if (!g->list_view->a[idx].path) {
		return FALSE;
	}

	// set all to false
	memset(&g->img_saved_status, 0, g->playlist_ids.size*sizeof(int));

	if (!(img_id = get_image_id(g->list_view->a[idx].path))) {
		return FALSE;
		//assert(img_id >= 0 && "This should never happen");
	}

	sqlite3_stmt* stmt = sqlstmts[GET_IMG_SAVE_STATUSES];
	sqlite3_bind_int(stmt, 1, img_id);

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		int plist_id = sqlite3_column_int(stmt, 0);

		// TODO better way?  SQL doesn't fill holes in primary keys
		// and while I assume it always returns them in ascending order
		// I don't know if that is part of the spec.  So for now just have
		// to do this
		for (int i=0; i<g->playlist_ids.size; i++) {
			if (g->playlist_ids.a[i] == plist_id) {
				g->img_saved_status[i] = TRUE;
				break;
			}
		}
	}
	sqlite3_reset(stmt);
	return TRUE;

}

int remove_from_lib(const char* path)
{
	int img_id;

	assert(path);

	if (!(img_id = get_image_id(path))) {
		assert(img_id >= 0 && "This should never happen");
	}

	sqlite3_stmt* stmt = sqlstmts[DEL_IMG];
	sqlite3_bind_int(stmt, 1, img_id);
	sqlite3_step(stmt);
	// TODO check for failure?
	sqlite3_reset(stmt);

	return TRUE;
}

// remove out of date files (deleted/renamed etc.)
// TODO add GUI button for this
int clean_library(void)
{
	int id;
	//char* path;
	struct stat file_stat;

	cvector_i bad_img_ids;
	cvec_i(&bad_img_ids, 0, 1024);
	
	sqlite3_stmt* stmt = sqlstmts[GET_LIBRARY];
	while (sqlite3_step(stmt) == SQLITE_ROW) {

		id = sqlite3_column_int(stmt, 0);
		// TODO it returns const unsigned char*
		char *s = (char *)sqlite3_column_text(stmt, 1);

		if (stat(s, &file_stat)) {
			// cache id for deletion after we finish select statement
			cvec_push_i(&bad_img_ids, id);
		}
	}
	sqlite3_reset(stmt);

	remove_bad_imgs(&bad_img_ids);
	cvec_free_i(&bad_img_ids);

	return TRUE;
}

int load_library(cvector_file* files, cvector_i* bad_img_ids, cvector_str* bad_img_paths)
{
	int id;
	file f = {0};
	struct stat file_stat;
	struct tm* tmp_tm;
	char* sep;

	assert(files);
	assert((bad_img_ids && bad_img_paths) || !(bad_img_ids || bad_img_paths));

	if (bad_img_ids) {
		cvec_clear_i(bad_img_ids);
		cvec_reserve_i(bad_img_ids, 1024);

		cvec_clear_str(bad_img_paths);
		cvec_reserve_str(bad_img_paths, 1024);
	}

	cvec_clear_file(files);
	
	sqlite3_stmt* stmt = sqlstmts[GET_LIBRARY];
	while (sqlite3_step(stmt) == SQLITE_ROW) {

		id = sqlite3_column_int(stmt, 0);
		// TODO it returns const unsigned char*
		char *s = (char *)sqlite3_column_text(stmt, 1);

		if (stat(s, &file_stat) && bad_img_ids) {
			// cache id for deletion after we finish select statement
			cvec_push_i(bad_img_ids, id);
			cvec_push_str(&g->bad_img_paths, s);
		} else {
			f.path = CVEC_STRDUP(s);
			f.size = file_stat.st_size;
			f.modified = file_stat.st_mtime;

			bytes2str(f.size, f.size_str, SIZE_STR_BUF);
			tmp_tm = localtime(&f.modified);
			strftime(f.mod_str, MOD_STR_BUF, "%Y-%m-%d %H:%M:%S", tmp_tm); // %F %T
			sep = strrchr(f.path, PATH_SEPARATOR); // TODO test on windows but I think I normalize
			f.name = (sep) ? sep+1 : f.path;

			cvec_push_file(files, &f);
		}
	}
	sqlite3_reset(stmt);

	if (bad_img_ids && bad_img_ids->size && g->bad_imgs_behavior == REMOVE_IMGS) {
		remove_bad_imgs(bad_img_ids);
		cvec_clear_i(bad_img_ids);
		cvec_clear_str(bad_img_paths);
	}

	return TRUE;
}

int remove_bad_imgs(cvector_i* bad_img_ids)
{
	sqlite3_stmt* stmt = sqlstmts[DEL_IMG];

	int rc = sqlite3_exec(g->db, "BEGIN TRANSACTION", NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		SDL_Log("Failed to begin transaction: %s\n", sqlite3_errmsg(g->db));
		return FALSE;
	}

	// TODO Log paths...warn which playlists they're in?
	for (int i=0; i<bad_img_ids->size; i++) {
		sqlite3_bind_int(stmt, 1, bad_img_ids->a[i]);
		rc = sqlite3_step(stmt);
		if (rc != SQLITE_DONE) {
			SDL_Log("Failed to remove image %d from library: %s\n", bad_img_ids->a[i], sqlite3_errmsg(g->db));
			sqlite3_exec(g->db, "ROLLBACK", NULL, NULL, NULL);
			sqlite3_reset(stmt);
			return FALSE;
		}
		// TODO check for failure?
		sqlite3_reset(stmt);
	}

	rc = sqlite3_exec(g->db, "COMMIT", NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		SDL_Log("Failed to commit transaction: %s\n", sqlite3_errmsg(g->db));
		sqlite3_exec(g->db, "ROLLBACK", NULL, NULL, NULL);
		return FALSE;
	}

	SDL_Log("Removed %"PRIcv_sz" bad files from library\n", bad_img_ids->size);
	
	return TRUE;
}

int export_playlist(const char* name)
{
	int id;
	char path_buf[STRBUF_SZ];

	sqlite3_stmt* stmt = sqlstmts[SELECT_ALL_IN_PLIST_ID];
	if (!(id = get_playlist_id(name))) {
		sqlite3_reset(stmt);
		return FALSE;
	}

	snprintf(path_buf, STRBUF_SZ, "%s/%s", g->playlistdir, name);

	FILE* f = fopen(path_buf, "w");
	if (!f) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to create %s: %s\nAborting export\n", path_buf, strerror(errno));

		sqlite3_reset(stmt);
		return FALSE;
	}

	sqlite3_bind_int(stmt, 1, id);
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		// We only store normalized fullpaths in db so no need to do anything with path
		const char *s = (const char *)sqlite3_column_text(stmt, 0);
		fprintf(f, "%s\n", s);
	}
	fclose(f);
	sqlite3_reset(stmt);

	SDL_Log("Exported playlist '%s' to %s\n", name, path_buf);
	return TRUE;
}

// TODO better to abstract get_playlist() to return a cvec_str and use that
// both here and in load_sql_playlist*()?
int export_playlists(void)
{
	for (int i=0; i<g->playlists.size; i++) {
		export_playlist(g->playlists.a[i]);
	}

	return TRUE;
}
