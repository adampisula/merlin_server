#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sqlite3.h> 

void error(const char *msg) {
    perror(msg);
    exit(0);
}

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
    char * errmsg = (char*) malloc(sizeof(char) * 512);
    
    rc = sqlite3_open(path, &retdb);

    if(rc) {
        sprintf(errmsg, "Dang it! There was a problem with database - '%s'\n", sqlite3_errmsg(retdb));
        error(errmsg);
    }

    return retdb;
}

void executeSQL(char* command, sqlite3 *db) {
    char *zErrMsg = 0;
    int rc = sqlite3_exec(db, command, callback, 0, &zErrMsg);
    char *errmsg = (char*) malloc(sizeof(char) * 512);
   
    if( rc != SQLITE_OK ) {
        sprintf(errmsg, "Oh shoot! There was an error while executing SQL - '%s'\n", zErrMsg);
        sqlite3_free(zErrMsg);
        error(errmsg);
    }
}

int main(int argc, char *argv[]) {
    int sockfd, portno = atoi(argv[2]), n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    bool ipPreferred = false;
    char buffer[256];

    if (argc < 2)
       error("You didn't pass a valid device :/\n");

    if(strcmp(separate(argv[1], '/', 0), "") == 0)
        ipPreferred = true;

    //IP PREFERRED
    if(!ipPreferred)
        server = gethostbyname(separate(argv[1], '/', 0));

    if (server == NULL)
        error("Such host doesn't exist, dude :(\n");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
        error("Ouch! We couldn't open that socket.\n");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("Dang it, there was a trouble while connecting with specified address.\n");

    while(n > 0) {
        bzero(buffer, 256);
        fgets(buffer, 255, stdin);

        n = write(sockfd,buffer,strlen(buffer));

        if (n < 0)
            error("We couldn't write to socket :c\n");

        bzero(buffer, 256);
        n = read(sockfd, buffer, 255);

        if (n < 0)
            error("We couldn't read from socket :c\n");

        printf("\t%s\n", buffer);
    }

    close(sockfd);
    return 0;
}
