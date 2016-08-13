#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

int BUFFER_SIZE = 1024;

void error(char *msg)
{
    perror(msg);
    exit(1);
}

// a seperate function, which is executed by a thread
// whose job is to reap children of this process
void reapChildren()
{
    // wait for any child
    pid_t killpid;
    while (1)
    {
	    while ((killpid = wait(NULL)) > 0)
	    {
	        printf("A child process %d terminated\n", killpid);
	    }
        sleep(5);
	}
}

// function which server calls to serve the requsted file to the client
void serveFile(int sock)
{
	char buffer[BUFFER_SIZE];
	bzero(buffer, BUFFER_SIZE); 
    
	/* Read file requesst from client */
    int bytes_read = read(sock, buffer, sizeof(buffer));
    if (bytes_read < 0)
    	error("ERROR reading from socket");
    
    /* extract filename */
    char* filename = (char*)malloc(strlen(buffer) - 3);
    strncpy(filename, buffer + 4, strlen(buffer) - 3);

    /* Open the requested file */
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)	// handle this in client
    	error("ERROR file not found");

    
    /* Send requested file */
    printf("Sending file %s to client\n", filename);

    while (1)
    {
    	int bytes_read = fread(buffer, sizeof(char), sizeof(buffer), fp);
    	if (bytes_read > 0)
    	{
    		int bytes_sent = send(sock, buffer, bytes_read, 0);
    		if (bytes_sent < bytes_read) 
    			error("ERROR writing to socket");
    	}
    	if (bytes_read == 0)
    	{
    		printf("File %s successfully sent to client\n",filename);
    		fclose(fp);
    		break;
    	}
    	if (bytes_read < 0)
    	{
    		error("ERROR reading from file");
    	}

        sleep(1);
    }
    close(sock);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, clilen, yes = 1;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in serv_addr, cli_addr;
    pid_t pid, killpid;
    if (argc < 2 || argc > 2) {
        printf(stderr,"usage :  %s [port]\n", argv[0]);
        exit(1);
    }

    // thread which reaps children
    pthread_t tid;
    pthread_create(&tid, NULL, reapChildren, NULL);

    /* create socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (sockfd <= 0) 
        error("ERROR opening socket\n");

    /* fill in port number to listen on. IP address can be anything (INADDR_ANY) */

    bzero((char*)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    /* bind socket to this port number on this machine */

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) 
    error("ERROR on binding\n");
    
    /* listen for incoming connection requests */
    listen(sockfd, 100);
    clilen = sizeof(cli_addr);

	while (1)
	{
        /* accept a new request, create a newsockfd */
	    newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
	    if (newsockfd < 0) 
	        error("ERROR on accept\n");
	    printf("New client connected\n");

	    pid = fork();

	    if (pid < 0)
	    {
	    	error("ERROR could not fork new process\n");
	    }
	    if (pid == 0)
	    {  
            // close the current socket (inherited from the parent)
	    	close(sockfd);
            // serve file on the newsocket created for the client connection
	    	serveFile(newsockfd);
	    }
	    else
	    {
	    	while ((killpid = waitpid(-1, NULL, WNOHANG)) > 0)
	    	{
	    		printf("A child process %d terminated\n", killpid);
	    	}
            // the parent doesn't need the new socket, hence closed
	    	close(newsockfd);
	    }
	}
    // join reaper thread
    pthread_join(tid, NULL);
    return 0; 
}
