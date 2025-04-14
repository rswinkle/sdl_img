
// split out single use like create vs many/most use?
enum {
	CREATE_TABLES,

	INSERT_IMG,
	INSERT_PLIST,
	INSERT_INTO_PLIST,
	ADD_TO_PLIST,

	RENAME_PLIST,

	// better names
	GET_PLIST_ID,
	GET_IMG_ID,
	GET_PLISTS,

	SELECT_ALL_IN_PLIST_ID,
	SELECT_ALL_IN_PLIST_NAME,

	DEL_PLIST_ID,
	DEL_PLIST_NAME,
	DEL_IMG,
	DEL_FROM_PLIST_ID,
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


	"INSERT INTO Images (path) VALUES (?);",
	"INSERT INTO Playlists (name) VALUES (?);",

	"INSERT INTO Playlist_Images (playlist_id, image_id) VALUES (?, ?);",

	"INSERT INTO Playlist_Images (playlist_id, image_id) "
	"SELECT p.playlist_id, i.image_id "
	"FROM Playlists p, Images i "
	"WHERE p.name = ? AND i.path = ?;",

	"UPDATE Playlists SET name = ? WHERE name = ?;",

	"SELECT playlist_id FROM Playlists WHERE name = ?;",
	"SELECT image_id FROM Images where path = ?;",
	"SELECT name FROM Playlists;",

	//-- Get all image paths in playlist id
	"SELECT i.path FROM Images i "
	"JOIN Playlist_Images pi ON i.image_id = pi.image_id "
	"WHERE pi.playlist_id = ?;",

	// get all image paths in playlist name
	"SELECT i.path FROM Images i "
	"JOIN Playlist_Images pi ON i.image_id = pi.image_id "
	"WHERE pi.playlist_id = (SELECT playlist_id FROM Playlists WHERE name = ?);",

	//-- Delete a playlist (automatically removes entries from Playlist_Images due to CASCADE)
	"DELETE FROM Playlists WHERE playlist_id = ?;",
	"DELETE FROM Playlists WHERE name = ?;",

	//-- Delete an image (automatically removes it from all playlists)
	"DELETE FROM Images WHERE image_id = ?;",

	// Delete from playlist
	"DELETE FROM Playlist_Images WHERE playlist_id = ? AND image_id = ?;",

	"DELETE FROM Playlist_Images "
	"WHERE playlist_id = (SELECT playlist_id FROM Playlists WHERE name = ?) "
	"AND image_id = (SELECT image_id FROM Images WHERE path = ?);",



};

sqlite3_stmt* sqlstmts[NUM_STMTS];
//sqlite3* db;  //images, playlists, possibly thumbs in the future?


void init_db(const char* db_file, sqlite3** db)
{
	if (sqlite3_open(db_file, db)) {
		SDL_LogCriticalApp("Failed to open %s: %s\n", db_file, sqlite3_errmsg(*db));
		exit(1);
	}
	create_tables(*db);
	prepare_stmts(*db);
}

void shutdown_db(sqlite3* db)
{
	finalize_stmts();
	sqlite3_close(db);
}

void create_tables(sqlite3* db)
{
	char* errmsg = NULL;
	if (sqlite3_exec(db, sql[CREATE_TABLES], NULL, NULL, &errmsg)) {
		SDL_LogCriticalApp("Failed to create table: %s\n", errmsg);
		sqlite3_free(errmsg);
		exit(1);
	}
}

void prepare_stmts(sqlite3* db)
{
	for (int i=INSERT; i<NUM_STMTS; ++i) {
		if (sqlite3_prepare_v2(db, sql[i], -1, &sqlstmts[i], NULL)) {
			SDL_LogCriticalApp("Failed to prep following statement:\n\"%s\"\nproduced errormsg: %s\n", sql[i], sqlite3_errmsg(db));
			exit(1);
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
	sqlite3_stmt* stmt = sqlstmts[INSERT_PLIST];
	sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
	int rc;
	// TODO decide whether to always check here or always check myself before
	// calling this function
	if ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
		SDL_Log("Failed add playlist %s (%d): %s\n", "%s", rc, sqlite3_errmsg(db));
		return FALSE;
	}
	sqlite3_reset(stmt);
	return TRUE;
}

int delete_playlist(const char* name)
{
	sqlite3_stmt* stmt = sqlstmts[DEL_PLIST_NAME];
	sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
	sqlite3_step(stmt);
	sqlite3_reset(stmt);

	return TRUE;
}

int rename_playlist(const char* new_name, const char* old_name)
{
	sqlite3_stmt* stmt = sqlstmts[RENAME_PLIST];
	sqlite3_bind_text(stmt, 1, new_name, -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, old_name, -1, SQLITE_TRANSIENT);
	sqlite3_step(stmt);
	sqlite3_reset(stmt);

	return TRUE;
}

int get_sql_playlists(void)
{
	cvec_clear_str(&g->playlists);
	sqlite3_stmt* stmt = sqlstmts[SELECT_ALL_PLISTS];
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		const char *s = (const char *)sqlite3_column_text(stmt, 0);
		cvec_push_str(&g->playlists, s);
	}
	sqlite3_reset(stmt);
	return TRUE;
}



// TODO use SELECT_ALL_IN_PLIST_NAME,
int load_sql_playlist_name(cvector_file* files, const char* name)
{
	int id;
	sqlite3_stmt* stmt = sqlstmts[GET_PLIST_ID];
	//sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		id = sqlite3_column_int(stmt, 0);
	} else {
		// check for error instead of SQLITE_DONE?
		sqlite3_reset(stmt);
		return FALSE;
	}

	sqlite3_reset(stmt);

	load_sql_playlist_id(files, id);
	return TRUE;
}

// No URLs in database so if stat fails it was deleted or renamed
// Only regular files in database so if stat succeeds we can assume it's a regular file
// not a directory or link
void load_sql_playlist_id(cvector_file* files, int id)
{
	file f = {0};
	struct stat file_stat;

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
			f.modified = 0

			sep = strrchr(f.path, PATH_SEPARATOR); // TODO test on windows but I think I normalize
			f.name = (sep) ? sep+1 : f.path;

			f.name = f.path;
			strncpy(f.size_str, "unknown", SIZE_STR_BUF);
			strncpy(f.mod_str, "unknown", MOD_STR_BUF);
			cvec_push_file(&g->files, &f);
		} else {
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
	sqlite3_reset(stmt);
}

int get_image_id(const char *path)
{
    sqlite3_stmt *stmt = sqlstmts[GET_IMG_ID];
    int image_id = -1;
    int rc;

    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        image_id = sqlite3_column_int(stmt, 0);
    } else {
        fprintf(stderr, "Unexpected error: Image '%s' not found (should exist).\n", path);
    }

    sqlite3_reset(stmt);
    return image_id;
}

int do_sql_save(int removing)
{
	int idx;
	int img_id;
	int cur_plist_id = g->cur_plist_id;
	char* playlist = g->cur_playlist;
	

	if (removing) {
		sqlite3_stmt* stmt = sqlstmts[DEL_FROM_PLIST];
		for (int i=0; i<g->n_imgs; i++) {
		}
	} else {
		SDL_Log("%s already in %s\n", g->img_focus->fullpath, playlist);

		for (int i=0; i<g->n_imgs; i++) {
			idx = g->img[i].index;
			if (IS_VIEW_RESULTS()) {
				idx = g->search_results.a[idx];
			}
			sqlite3_stmt* stmt = sqlstmts[INSERT_INTO_PLIST];
			sqlite3_bind_text(stmt, 1, cur_plist_id);

			//if ((img_id = get_image_id(g->files.a[idx].path)) < 0) {
			if ((img_id = get_image_id(g->img[i].fullpath)) < 0) {
				assert(img_id >= 0 && "This should never happen");
			}

			sqlite3_bind_text(stmt, 2, img_id);

			int rc = sqlite3_step(stmt);
			if (rc == SQLITE_DONE) {
				SDL_Log("saving %s\n", g->img[i].fullpath);
			} else if (rc == SQLITE_CONSTRAINT) {
				SDL_Log("%s already in %s\n", g->img[i].fullpath, playlist);
			} else {
			}

		}
	}

	return TRUE;
}

// Do this or do it as we scan sources and stat() files?
int add_cur_files_to_db()
{
	sqlite3_exec(ctx->db, "BEGIN TRANSACTION", NULL, NULL, NULL);
	for (int i=0; i<g->files.size; i++) {
	}

	sqlite3_exec(ctx->db, "COMMIT", NULL, NULL, NULL);
}


