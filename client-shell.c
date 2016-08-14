#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string>
#include <set>
using namespace std;

static unsigned int MAX_INPUT_SIZE = 1024;
static unsigned int MAX_TOKEN_SIZE = 64;
static unsigned int MAX_NUM_TOKENS = 64;

static string serverIP = "";
static string serverPort = "";
static set<string> FGcommands = {"getfl", "getsq", "getpl"};

#include "functions.h"

void error(char *msg)
{
    perror(msg);
    exit(1);
}

char **tokenize(char *line)
{
    char **tokens = (char**) malloc(MAX_NUM_TOKENS * sizeof(char *));
    char *token = (char*) malloc(MAX_TOKEN_SIZE * sizeof(char));
    int tokenIndex = 0, tokenNo = 0;

    for (int i = 0; i < strlen(line); i++)
    {
        char readChar = line[i];

        if (readChar == ' ' || readChar == '\n' || readChar == '\t')
        {
            token[tokenIndex] = '\0';
            if (tokenIndex != 0)
            {
                tokens[tokenNo] = (char*) malloc(MAX_TOKEN_SIZE * sizeof(char));
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
    tokens[tokenNo] = NULL;
    return tokens;
}

// TODO : "default" to be removed
void setServer(char** tokens, bool defaultx = false)
{
    if ((tokens[1] == NULL || tokens[2] == NULL || tokens[3] != NULL) and not defaultx)
        fprintf(stderr, "usage: server [server-ip] [server-port]\n");
    else
    {
        if (defaultx) 
            serverIP = "127.0.0.1";
        else
            serverIP = string(tokens[1]);

        if (defaultx) 
            serverPort = "5001";
        else
            serverPort = string(tokens[2]);
    }
}

void FGProcess(char** tokens)
{
    string ftoken(*tokens);

    if (ftoken == "getfl")
    {
        if (tokens[1] == NULL || tokens[2] != NULL)
           fprintf(stderr, "usage: getfl [filename]\n");
        else
            getfl(tokens[1], "display");
    }
    else if (ftoken == "getsq")
    {
        if (tokens[1] == NULL)
            fprintf(stderr, "usage: getsq [file1] [file2] ...\n");
        else
            getsq(tokens);
    }
    else if (ftoken == "getpl")
    {
        // TODO : Can use group IDs??
        if (tokens[1] == NULL)
            fprintf(stderr, "usage: getpl [file1] [file2] ...\n");
        else
            getpl(tokens);
    }
}

// TODO : make the "server settings set" error common
void shellProcess(char** tokens)
{   
    string ftoken(*tokens);
    if (ftoken == "server")
    {
        setServer(tokens);
    }
    // TODO : to be removed
    else if (ftoken == "ser")
    {
        setServer(tokens, true);
    }

    // it's the 'cd' command
    else if (ftoken == "cd")
    {   
        // check if number of arguments is 2
        if (tokens[1] == NULL || tokens[2] != NULL)
            fprintf(stderr, "usage: cd [directory]\n");
        else
            cd(tokens);
    }

    // it's a foreground command
    else if (FGcommands.find(ftoken) != FGcommands.end())
    {   
        if (serverIP == "" || serverPort == "")
        {
            fprintf(stderr, "ERROR server info not set\n");
        }
        else
        {
            pid_t pid;
            pid = fork();

            if (pid < 0)
                perror("ERROR forking child process failed");
            else if (pid == 0)
                // child does the FG server process
                FGProcess(tokens);
            else 
                // shell waits for forked child
                waitpid(pid, NULL, 0);
        }
    }

    // it's a miscellaneous linux command
    else 
    {
        pid_t pid = fork();
            
        if (pid < 0)
        {
            perror("ERROR forking child process failed");
        }
        else if (pid == 0)
        {
            if (execvp(*tokens, tokens) < 0)
            {
                fprintf(stderr, "%s: Command not found\n", *tokens);
                exit(0);
            }
        }
        else 
        {
            waitpid(pid, NULL, 0);
        }
    }
}

int  main(void)
{
    char line[MAX_INPUT_SIZE];          
    char **tokens;

    while (1)
    {           
       
        printf("Hello>");

        bzero(line, MAX_INPUT_SIZE);
        fgets(line, MAX_INPUT_SIZE, stdin);           
        //terminate the string with a new line
        line[strlen(line)] = '\n';
        tokens = tokenize(line);

        //detect empty command
        if (tokens[0] == NULL)
            continue;

        shellProcess(tokens);

        // Freeing the allocated memory 
        for (int i = 0; tokens[i] != NULL; i++)
            free(tokens[i]);
        free(tokens);
    }   
    return 0;
}
