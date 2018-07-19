#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sqlite3.h>

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
   int i;

   for(i = 0; i<argc; i++) {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }

   printf("\n");
   
   return 0;
}

char* separate(char *str, char delimiter, int index) {
    int word_size = 0;
    int delimiters_passed = 0;

    int i;

    for(i = 0; i < strlen(str); i++) {
        if(str[i] == delimiter) {
            delimiters_passed++;

            if(index == delimiters_passed - 1)
                break;

            else
                word_size = -1;
        }

        word_size++;
    }

    char *ret = (char*) malloc(sizeof(char) * word_size);
    bzero(ret, word_size);

    for(int j = i - word_size; j < i; j++) {
        if(!(str[j] == '/' && j == 0))
            snprintf(ret + strlen(ret), sizeof(ret - strlen(ret)), "%c", str[j]);
    }

    return ret;
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

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    sqlite3* db;
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    char* device_name = (char*) malloc(sizeof(char) * 64);
    char* device_ip = (char*) malloc(sizeof(char) * 64);

    bzero(device_name, 64);
    bzero(device_ip, 64);

    if(argc < 2) {
        error("No port specified.");
    }

    if(argc < 3) {
        error("No device specified.");
    }

    portno = atoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
        error("ERROR opening socket");

    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 5);

    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

    if (newsockfd < 0)
        error("ERROR on accept");


    db = openDB("tmp/local.db");
    executeSQL("CREATE TABLE IF NOT EXISTS `queue` (`id`	INTEGER PRIMARY KEY AUTOINCREMENT, `name`	TEXT, `ip`	TEXT, `command`	TEXT);", db);
    
    char sql[256];

    strcpy(device_name, separate(argv[1], '/', 0));
    strcpy(device_ip, separate(argv[1], '/', 1));

    bzero(buffer, 256);
    n = read(newsockfd, buffer, 255);

    while(n > 0) {
        printf("%s", buffer);
        n = write(newsockfd, "RECV", 18);

        bzero(sql, 256);

        strcpy(sql, "INSERT INTO `queue` (`name`, `ip`, `command`) VALUES ('");
        strcat(sql, device_name);
        strcat(sql, "', '");
        strcat(sql, device_ip);
        strcat(sql, "', '");
        strcat(sql, buffer);
        strcat(sql, "');");

        executeSQL(sql, db);
       
        if (n < 0) {
            error("ERROR writing to socket");
            break;
        }

        bzero(buffer, 256);
        n = read(newsockfd, buffer, 255);
    }

    if (n < 0)
        error("ERROR reading from socket");

    close(newsockfd);
    close(sockfd);

    return 0;
}
