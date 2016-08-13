#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

char* serverIP;
char* serverPort;

void error(char *msg)
{
	perror(msg);
	exit(1);
}

char **tokenize(char *line)
{
	char **tokens = (char **) malloc(MAX_NUM_TOKENS * sizeof(char *));
	char *token = (char *) malloc(MAX_TOKEN_SIZE * sizeof(char));
	int i, tokenIndex = 0, tokenNo = 0;

	for (i = 0; i < strlen(line); i++)
	{
		char readChar = line[i];

		if (readChar == ' ' || readChar == '\n' || readChar == '\t')
		{
			token[tokenIndex] = '\0';
			if (tokenIndex != 0)
			{
				tokens[tokenNo] = (char*) malloc(MAX_TOKEN_SIZE*sizeof(char));
				strcpy(tokens[tokenNo++], token);
				tokenIndex = 0; 
			}
		} 
		else
		{
			token[tokenIndex++] = readChar;
		}
	}
 
	free(token);
	tokens[tokenNo] = NULL ;
	return tokens;
}

void getfl(char* filename, char* displayMode)
{
	char *arguments[] = {"./get-one-file-sig", filename, serverIP, serverPort, displayMode, NULL};
	if (execvp(arguments[0], arguments))
		perror("Error : getfl");
}

void runProcess(char** tokens)
{
	if (strcmp(*tokens, "cd") == 0)
	{
		// check if number of arguments is 2
		if (tokens[1] == NULL || tokens[2] != NULL)
		{
			fprintf(stderr, "usage: cd [directory]\n");
		}
		else if (chdir(tokens[1]) != 0)
		{
			char errorMsg[MAX_TOKEN_SIZE + 20];
			sprintf(errorMsg, "bash: cd: %s\n", tokens[1]);
			perror(errorMsg);
		}
	}
	else if (strcmp(*tokens, "getfl") == 0)
	{
		if (tokens[1] == NULL || tokens[2] != NULL)
			fprintf(stderr, "usage: getfl [filename]\n");
		else if (serverIP == NULL || serverPort == NULL)
			fprintf(stderr, "error: server info unavailable, use server command to set server IP and port\n");
		else
			getfl(tokens[1], "display");

	}
	else if (strcmp(*tokens, "getsq") == 0)
	{
		if (tokens[1] == NULL)
		{
			fprintf(stderr, "usage: getsq [file1] [file2] ...\n");
		}
		else if (serverIP == NULL || serverPort == NULL)
		{
			fprintf(stderr, "error: server info unavailable, use server command to set server IP and port\n");
		}
		else
		{
			int i;
			for (i = 1; tokens[i+1] != NULL; i++)
			{
				pid_t pid = fork();
				if (pid < 0)
				{
					perror("ERROR: forking child process failed");
					return;
				}
				if (pid == 0)				
				{
					getfl(tokens[i], "nodisplay");
				}
				else
				{
					waitpid(pid, NULL, 0);
				}
			}
			getfl(tokens[i], "nodisplay");
		}
	}
	else if (strcmp(*tokens, "getpl") == 0)
	{
		// TODO : Can use group IDs??
		if (tokens[1] == NULL)
		{
			fprintf(stderr, "usage: getpl [file1] [file2] ...\n");
		}
		else if (serverIP == NULL || serverPort == NULL)
		{
			fprintf(stderr, "error: server info unavailable, use server command to set server IP and port\n");
		}
		else
		{
			int i;
			for (i = 1; tokens[i] != NULL; i++)
			{
				pid_t pid = fork();
				if (pid < 0)
				{
					perror("ERROR: forking child process failed");
					return;
				}
				if (pid == 0)				
				{
					getfl(tokens[i], "display");
				}
			}
			while (i > 1)
			{
				waitpid(0, NULL, 0);
				i--;
			}
			exit(0);
		}
	}
	else if (execvp(*tokens, tokens) < 0)
	{
		fprintf(stderr, "%s: Command not found\n", *tokens);
	}
}



int  main(void)
{
	char  line[MAX_INPUT_SIZE];            
	char  **tokens;              
	int i;

	while (1)
	{           
	   
		printf("Hello>");

		bzero(line, MAX_INPUT_SIZE);
		fgets(line, sizeof(line), stdin);           
		line[strlen(line)] = '\n'; //terminate with new line
		tokens = tokenize(line);

		//detect empty command
		if (tokens[0] == NULL)
			continue;

		if (strcmp(*tokens, "server") == 0)
		{
			if (tokens[1] == NULL || tokens[2] == NULL || tokens[3] != NULL)
				fprintf(stderr,"usage: server [server-ip] [server-port]\n");
			else
			{
				if (serverIP != NULL)
					free(serverIP);

				serverIP = (char*) malloc((strlen(tokens[1])) * sizeof(char));
				strncpy(serverIP, tokens[1], strlen(tokens[1]) + 1);

				if (serverPort == NULL)
					free(serverPort);

				serverPort = (char*) malloc((strlen(tokens[2])) * sizeof(char));
				strncpy(serverPort, tokens[2], strlen(tokens[2]) + 1);
			}
			continue;
		}
		// to assign default server ip and port 
		// TO BE REMOVED
		if (strcmp(*tokens, "ser") == 0)
		{
			if (serverIP != NULL)
				free(serverIP);

			serverIP = (char*) malloc((strlen("127.0.0.1")) * sizeof(char));
			strncpy(serverIP, "127.0.0.1", strlen("127.0.0.1") + 1);

			if (serverPort == NULL)
				free(serverPort);

			serverPort = (char*) malloc((strlen("5001")) * sizeof(char));
			strncpy(serverPort, "5001", strlen("5001") + 1);
			continue;
		}

		pid_t pid;
		pid = fork();

		if (pid < 0)
		{
			perror("ERROR: forking child process failed");
			continue;
		}
		else if (pid == 0)
		{
			runProcess(tokens);
		}
		else 
		{
			waitpid(pid, NULL, 0);
		}

		// Freeing the allocated memory	
		for (i = 0; tokens[i] != NULL; i++)
		{
			free(tokens[i]);
		}
		free(tokens);

	}	
	return 0;
}

				
