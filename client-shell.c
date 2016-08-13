#include  <stdio.h>
#include  <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

void error(char *msg)
{
	perror(msg);
	exit(1);
}

char **tokenize(char *line)
{
	char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
	char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
	int i, tokenIndex = 0, tokenNo = 0;

	for(i =0; i < strlen(line); i++)
	{
		char readChar = line[i];

		if (readChar == ' ' || readChar == '\n' || readChar == '\t')
		{
			token[tokenIndex] = '\0';
			if (tokenIndex != 0)
			{
				tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
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

void runProcess(char** tokens)
{
	if(strcmp(*tokens,"cd") == 0)
	{
		if(tokens[1] == NULL || tokens[2] != NULL)
			fprintf(stderr,"usage: cd [directory]");
		else if(chdir(tokens[1]) != 0)
		{
			char errorMsg[MAX_TOKEN_SIZE + 20];
			sprintf(errorMsg,"bash: cd: %s", tokens[1]);
			perror(errorMsg);
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

		//do whatever you want with the commands, here we just print them

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
		for(i=0;tokens[i]!=NULL;i++)
		{
			free(tokens[i]);
		}
		free(tokens);

	}	
	return 0;
}

				
