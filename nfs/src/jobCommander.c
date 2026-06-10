#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int main(int argc, char *argv[]) {
    if (argc < 4) {                      //Elenxoume gia orthi xrhsh parametrwvn
        printf("Not enough arguments");
        exit(EXIT_FAILURE);
    }

    const char *serverName = argv[1];   //Apothikeuoume to onoma tou server
    int portNum = atoi(argv[2]);        //Apothikeuoume ton arithmo ths thuras

    char command[1024] = {0};          //Pinakas pou tha apothikeuousoume thn parametro command
    for (int i = 3; i < argc; i++) {
        strcat(command, argv[i]);
        if (i < argc - 1) strcat(command, " ");
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);    //Anoigoume to socket

    struct hostent *server = gethostbyname(serverName);  //Dhmiourgoume to struct hostent gia ton server me bash to onoma tou

    struct sockaddr_in server_address;                    //Dhmiourgoume kai to stuct sockaddr pali gia ton server
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(portNum);
    memcpy(&server_address.sin_addr.s_addr, server->h_addr_list[0], server->h_length);


    if (connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {  //Kanoume aithma sundeshs
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (send(sockfd, command, strlen(command), 0) < 0) {            //Stelnoume to command
        perror("send");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    char message[32000] = {0};                               //Lambanoume kai tupwnoume to mhnuma pou phrame apo ton server
    int bytes = recv(sockfd, message, 5064, 0);
    if (bytes < 0) {
        perror("recv");
    } else {
        printf("%s\n", message);
    }

    close(sockfd);                                         //Kleinoume to socket
    return 0;
}
