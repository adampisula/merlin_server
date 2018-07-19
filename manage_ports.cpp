#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sqlite3.h>
#include <map>

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
        printf("Ouch! We've had a trouble opening the socket.\n");

    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        available = false;

    close(sockfd);

    return available;
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
    //PROGRAM
    int sockfd, newsockfd;
    int portno = 51717, ports_to_connect = 16;
    struct sockaddr_in serv_addr, cli_addr;
    int devices_limit = 128;
    socklen_t clilen;
    char buffer[256];
    int n;
    int bind_result;

    while(true) {
    	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	    if (sockfd < 0)
            printf("Ouch! We've had a trouble opening the socket.\n");

        int enable = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
            printf("¡Caramba! We couldn't reuse the socket. That's a shame.\n");

        printf("%s\n", "Socket created. As it was planned.");

        bzero((char *) &serv_addr, sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        
        printf("%s\n", "Socket configured.");

        for(int i = portno; i < portno + ports_to_connect; i++) {
            serv_addr.sin_port = htons(i);

            printf("Binding on %i... ", i);

            if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) >= 0)
                continue;
        }

        printf("%s\n", "Bounded ;)\nListening...");

        listen(sockfd, 5);

        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

        if (setsockopt(newsockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
            printf("¡Caramba! We couldn't reuse the socket. That's a shame.\n");

        bzero(buffer, 256);
        n = read(newsockfd, buffer, 255);

        if(n < 0) {
            close(newsockfd);
            close(sockfd);

            continue;
        }

        char *device_hostname = separate(buffer, '/', 0);
        char *device_ip = separate(buffer, '/', 1);
        char *device_role = separate(buffer, '/', 2);

        printf("\tDevice: %s\n", buffer);
        printf("\tRole: %s\n", device_role);

        char port_string[7];

        char command[256];

        if(strcmp(device_role, "CLIENT") == 0) {
            strcpy(command, "./server ");
            strcat(command, buffer);
        }

        else if(strcmp(device_role, "SERVER") == 0) {
            strcpy(command, "./client ");
            strcat(command, buffer);
        }

        for(int i = portno + ports_to_connect + 1; i < portno + ports_to_connect + devices_limit + 1; i++) {
            if(port_available(i)) {
                snprintf(port_string, 7, "%d", i);

                printf("Port: %s\n", port_string);
                strcat(command, " ");
                strcat(command, port_string);
                strcat(command, " &");

                printf("\tCommand: %s\n", command);

                bzero(buffer, 256);
                n = write(newsockfd, port_string, 18);

                if (n < 0)
                    printf("¡Dios mío! We weren't able to write to socket :(\n");

                popen(command, "r");

                break;
            }
        }

        close(sockfd);
        close(newsockfd);
        printf("%s\n", "-----");
	usleep(750 * 1000);
    }

    return 0;
}
