#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <string.h>
#include <cstring>
#include <unistd.h>
#include <sqlite3.h>
#include <microhttpd.h>
#ifndef _WIN32
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif

#define PORT 51000

char ip[128];
char name[128];
char role[128];

int portno = 51001;
int max_devices = 128;

void error(const char *msg) {
    perror(msg);
    exit(1);
}

bool port_available(int portno) {
    int sockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    bool available = true;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
        error("Ouch! We've had a trouble opening the socket.\n");

    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        available = false;

    close(sockfd);

    return available;
}

static int save_get_data (void *cls, enum MHD_ValueKind kind, const char *key, const char *value) {
    if(strcmp(key, "ip") == 0)
        strcpy(ip, value);

    else if(strcmp(key, "name") == 0)
        strcpy(name, value);

    else if(strcmp(key, "role") == 0)
        strcpy(role, value);

    return MHD_YES;
}

static int on_client_connect (void *cls, const struct sockaddr *addr, socklen_t addrlen) {
    //struct sockaddr_in *sin = (struct sockaddr_in *) addr;
    //char* ip = inet_ntoa(sin->sin_addr);
    //printf("%s\n", ip);

    return MHD_YES;
}

static int answer_to_connection (void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls) {
    bzero(ip, 128);
    bzero(name, 128);
    bzero(role, 128);

    MHD_get_connection_values (connection, MHD_GET_ARGUMENT_KIND, save_get_data, NULL);    

    char *page = (char*) malloc(sizeof(char) * 512);
    char port_string[7];

    bzero(port_string, 7);

    int i = portno;

    while(i < portno + max_devices && !port_available(i))
        i++;

    snprintf(port_string, 7, "%d", i);

    strcpy(page, port_string);

    //OPEN EITHER CLIENT OR SERVER
    char* clientCmd = (char*) malloc(sizeof(char) * 256);
    char* serverCmd = (char*) malloc(sizeof(char) * 256);

    sprintf(clientCmd, "xfce4-terminal -e \"/root/Documents/Projects/merlin_server/runclient.sh %s/%s/%s %s\"", name, name, ip, role);
    sprintf(serverCmd, "xfce4-terminal -e \"/root/Documents/Projects/merlin_server/runserver.sh %s/%s/%s %s\"", name, ip, role, port_string);

    if(strcmp(role, "CLIENT") == 0) {
        popen(serverCmd, "r");
        printf("Opened server on port %s.\n", port_string);
    }

    else if(strcmp(role, "SERVER") == 0) {
        popen(clientCmd, "r");
        printf("Opened client on port %s.\n", port_string);
    }

    struct MHD_Response *response;
    int ret;

    response = MHD_create_response_from_buffer (strlen (page), (void*) page, MHD_RESPMEM_PERSISTENT);

    ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    MHD_destroy_response (response);

    return ret;
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

int main(int argc, char *argv[]) {
    struct MHD_Daemon *daemon;

    daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, PORT, on_client_connect, NULL, &answer_to_connection, NULL, MHD_OPTION_END);
    
    if(daemon == NULL)
        error("Web server doesn't seem to work :(");

    char command[256];
    bzero(command, 256);

    while(strcmp(command, "exit") != 0)
        scanf("%s", command);

    return 0;
}
