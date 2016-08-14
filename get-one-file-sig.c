#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

static unsigned int BYTES_RECV = 0;
static unsigned int BUFFER_SIZE = 1024;
static unsigned int DISPLAY = 0;

void error(char *msg)
{
    perror(msg);
    exit(0);
}

void sigIntHandler(int sig_num)
{
    printf("\nprocess %d received SIGINT, downloaded %d bytes so far\n", getpid(), BYTES_RECV);
    // exiting with standard exit code for termination
    exit(130);
}

// function to fetch file from sockfd
// display = 1 => display to stdout contents of downloaded file, 0 => not display
void getFile(char* file, int sockfd)
{
    char buffer[BUFFER_SIZE];    
    sprintf(buffer, "get %s", file);

    /* send user message to server */
    int bytes_sent = write(sockfd, buffer, strlen(buffer));
    if (bytes_sent < 0) 
        error("ERROR writing to socket");

    while (1)
    {
        bzero(buffer, BUFFER_SIZE);
        int curr_recv = recv(sockfd, buffer, BUFFER_SIZE, 0);
        
        if (curr_recv < 0) 
            error("ERROR reading from socket");
        else if (curr_recv == 0)
        {
            if (BYTES_RECV > 0)
            {   
                // file download complete
                if (DISPLAY)
                    printf("\n");
            }
            else
            {
                fprintf(stderr, "ERROR file requested by client not found on server\n");
            }
            break;
        }
        else
        {   
            buffer[curr_recv] = '\0';
            if (DISPLAY)
                printf("%s", buffer);
            BYTES_RECV += curr_recv;
        }
    }
    // close the socket
    close(sockfd);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sigIntHandler);

    if (argc != 5) {
        fprintf(stderr, "usage :  %s [file] [host] [port] [display-mode]\n", argv[0]);
        exit(0);
    }

    int port, sockfd, display, yes = 1;
    struct hostent *server;
    char* file;
    struct sockaddr_in serv_addr;

    file = (char*)malloc(strlen(argv[1]));
    strncpy(file, argv[1], strlen(argv[1]));

    port = atoi(argv[3]);
    
    // display mode
    if (strcmp(argv[4], "display") == 0)
        DISPLAY = 1;

    // server    
    server = gethostbyname(argv[2]);
    if (server == NULL)
        error("ERROR no such server");

    /* create socket, get sockfd handle */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (sockfd < 0) 
        error("ERROR opening socket");

    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    bcopy((char*)server->h_addr, (char*)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    /* connect to server */
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
    //printf("Client connected to the server\n");

    getFile(file, sockfd);

    return 0;
}
