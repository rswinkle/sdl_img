
enum {
	CREATE_TABLES,

	INSERT_IMG,
	INSERT_PLIST,
	INSERT_INTO_PLIST,

	// better names
	SELECT_ALL_PLISTS,
	SELECT_ALL_IN_PLIST,

	DEL_PLIST,
	DEL_IMG,
	DEL_FROM_PLIST,

	NUM_STMTS
};

const char* sql[] = {
	"PRAGMA foreign_keys = ON;"
	"CREATE TABLE IF NOT EXISTS Images ("
		"image_id INTEGER PRIMARY KEY,"
		"path TEXT NOT NULL UNIQUE  -- Full image path, unique to avoid duplicates"
	");"
	"CREATE TABLE IF NOT EXISTS Playlists ("
		"playlist_id INTEGER PRIMARY KEY,"
		"name TEXT NOT NULL"
	");"
	"CREATE TABLE IF NOT EXISTS Playlist_Images ("
		"playlist_id INTEGER,"
		"image_id INTEGER,"
		"PRIMARY KEY (playlist_id, image_id),"
		"FOREIGN KEY (playlist_id) REFERENCES Playlists(playlist_id) ON DELETE CASCADE,"
		"FOREIGN KEY (image_id) REFERENCES Images(image_id) ON DELETE CASCADE"
	");",

	"INSERT INTO Images (path) VALUES (?);",
	"INSERT INTO Playlists (name) VALUES (?);",

	"INSERT INTO Playlist_Images (playlist_id, image_id) VALUES (?, ?);",

	"SELECT name FROM Playlists;",

	//-- Get all image paths in playlist id
	"SELECT i.path"
	"FROM Images i"
	"JOIN Playlist_Images pi ON i.image_id = pi.image_id"
	"WHERE pi.playlist_id = ?;",

	//-- Delete a playlist (automatically removes entries from Playlist_Images due to CASCADE)
	"DELETE FROM Playlists WHERE playlist_id = ?;",

	//-- Delete an image (automatically removes it from all playlists)
	"DELETE FROM Images WHERE image_id = ?;",

};

sqlite3_stmt* sqlstmts[NUM_STMTS];
//sqlite3* db;  //images, playlists, possibly thumbs in the future?


void init_db(const char* db_file, sqlite3** db)
{
	if (sqlite3_open(db_file, db)) {
		SDL_LogCriticalApp("Failed to open %s: %s\n", db_file, sqlite3_errmsg(*db));
		exit(1);
	}
	create_table(*db);
	prepare_stmts(*db);
}

void shutdown_db(sqlite3* db)
{
	finalize_stmts();
	sqlite3_close(db);
}

void create_table(sqlite3* db)
{
	char* errmsg = NULL;
	if (sqlite3_exec(db, sql[CREATE_TABLE], NULL, NULL, &errmsg)) {
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

