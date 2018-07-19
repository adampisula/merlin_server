#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
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

    for(int j = i - word_size; j < i; j++)
        snprintf(ret + strlen(ret), sizeof(ret - strlen(ret)), "%c", str[j]);

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
    exit(0);
}

int main(int argc, char *argv[]) {
    int sockfd, portno = atoi(argv[2]), n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    if (argc < 2) {
       fprintf(stderr, "No hostname given.\n");
       exit(0);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
        error("ERROR opening socket");

    server = gethostbyname(separate(argv[1], '/', 0));

    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    while(n > 0) {
        bzero(buffer, 256);
        fgets(buffer, 255, stdin);

        n = write(sockfd,buffer,strlen(buffer));

        if (n < 0)
            error("ERROR writing to socket");

        bzero(buffer, 256);
        n = read(sockfd, buffer, 255);

        if (n < 0)
            error("ERROR reading from socket");

        printf("\t%s\n", buffer);
    }

    close(sockfd);
    return 0;
}
