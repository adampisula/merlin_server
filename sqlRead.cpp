#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h> 

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
   int i;
   for(i = 0; i<argc; i++) {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

sqlite3* openDB(char* path) {
    sqlite3* retdb;
    int rc;
    
    rc = sqlite3_open(path, &retdb);

    if(rc) {
        printf("Dang it! There was a problem with database - '%s\n'", sqlite3_errmsg(retdb));
        return(0);
    }

    return retdb;
}

void executeSQL(char* command, sqlite3 *db) {
    char *zErrMsg = 0;
    int rc = sqlite3_exec(db, command, callback, 0, &zErrMsg);
   
    if( rc != SQLITE_OK ) {
        printf("Oh shoot! There was an error while executing SQL - '%s'\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
}

int main(int argc, char* argv[]) {
    sqlite3* db = openDB("tmp/local.db");
    executeSQL("CREATE TABLE IF NOT EXISTS `queue` (`id`	INTEGER PRIMARY KEY AUTOINCREMENT, `from_name`	TEXT, `from_ip`	TEXT, `to_name`	TEXT, `to_ip`	TEXT, `value`	TEXT, `time`	TEXT);", db);

    executeSQL("SELECT * FROM `queue`", db);

    sqlite3_close(db);

    return 0;
}
