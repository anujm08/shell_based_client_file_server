#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string>
#include <set>
#include <pthread.h>
#include <fcntl.h>
#include <mutex>

using namespace std;

static unsigned int MAX_INPUT_SIZE = 1024;
static unsigned int MAX_TOKEN_SIZE = 64;
static unsigned int MAX_NUM_TOKENS = 64;
static unsigned int REAP_TIME = 4;

static string serverIP = "";
static string serverPort = "";
static set<string> FGcommands = {"getfl", "getsq", "getpl"};
static set<string> BGcommands = {"getbg"};

static set<pid_t> BGprocs;
static pthread_t reaperT;
static mutex MTX;
static bool FGProcessRunning = false;

#include "functions.h"

void* reapChildren(void* x)
{
    while (true)
    {
        pid_t killpid = wait(NULL);
        if (killpid > 0)
        {   
            MTX.lock();
            if (BGprocs.find(killpid) != BGprocs.end())
            {
                printf("\nbackground download process %d terminated\nHello>", killpid);
                fflush(stdout);
                BGprocs.erase(killpid);
            }
            else
            {   
                // reaped a FGproc
            }
            MTX.unlock();
        }
        sleep(REAP_TIME);
    }
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
    string ftoken(tokens[0]);

    // TODO : move usage inside the functions?
    if (ftoken == "getfl")
    {
        if (tokens[1] == NULL)
        {
            fprintf(stderr, "usage: getfl [filename]\n");
        }
        else if (tokens[2] == NULL)
        {
            getfl(tokens[1], "display");
        }
        else if (strcmp(tokens[2], ">") == 0)
        {
            if(tokens[3] == NULL || tokens[4] != NULL)
                fprintf(stderr, "usage: getfl [filename] > [outputfile]\n");
            else
                getflRedirection(tokens[1], tokens[3]);
        }
        else if (strcmp(tokens[2], "|") == 0) 
        {
            if(tokens[3] == NULL)
                fprintf(stderr, "usage: getfl [filename] > command\n");
            else
                getflPipe(tokens[1], &tokens[3]);
        }
        else
        {
            fprintf(stderr, "usage: getfl [filename]\n");
        }
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
        if (tokens[1] == NULL)
            fprintf(stderr, "usage: getpl [file1] [file2] ...\n");
        else
            getpl(tokens);
    }
    exit(0);
}

void BGProcess(char** tokens)
{
    string ftoken(tokens[0]);

    // set the gpid of bg process to be different from the parent
    // this way, sigint recvd by the shell, won't be signalled to 
    // this process too
    if (setpgid(0, 0) != 0)
        error("ERROR gpid could not be set");
    
    // TODO : move usage inside the functions?
    if (ftoken == "getbg")
    {
        if (tokens[1] == NULL || tokens[2] != NULL)
            fprintf(stderr, "usage: getbg [filename]\n");
        else
            getfl(tokens[1], "nodisplay");
    }
    exit(0);
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

    // it's the 'exit' command
    else if (ftoken == "exit")
    {   
        // send SIGINT to all BGprocs
        for (auto pid : BGprocs)
            if(kill(pid, SIGINT) != 0)
                printf("couldn't send signal to process");
        
        // reap the BGProcs
        /*MTX.lock();
        while (!BGprocs.empty())
        {
            pid_t killpid = waitpid(-1, NULL, 0);
            BGprocs.erase(killpid);
        }
        MTX.unlock();*/
        // wait until all childs are reaped
        while (1)
        {
            pid_t killpid = wait(NULL);
            if (errno == ECHILD)
                break;
        }
        exit(0);
    }

    // it's a foreground command
    else if (FGcommands.find(ftoken) != FGcommands.end())
    {   
        if (serverIP == "" || serverPort == "")
            fprintf(stderr, "ERROR server info not set\n");
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
                // sometimes, the reaper thread might reap this pid
                // process, and this will return immediately
                waitpid(pid, NULL, 0);
        }
    }

    // it's a background command
    else if (BGcommands.find(ftoken) != BGcommands.end())
    {   
        if (serverIP == "" || serverPort == "")
            fprintf(stderr, "ERROR server info not set\n");
        else
        {
            pid_t pid;
            pid = fork();

            if (pid < 0)
                perror("ERROR forking child process failed");
            else if (pid == 0)
                // child does the BG server process
                BGProcess(tokens);
            else {
                // shell doesn't wait for forked child
                MTX.lock();
                BGprocs.insert(pid);
                MTX.unlock();
            }
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
                fprintf(stderr, "%s: Command not found\n", tokens[0]);
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
    signal(SIGINT, sigIntHandlerPseudo);
    char line[MAX_INPUT_SIZE];          
    char **tokens;

    // thread which reaps children
    pthread_create(&reaperT, NULL, reapChildren, NULL);

    while (true)
    {           
        FGProcessRunning = false;
        printf("Hello>");

        bzero(line, MAX_INPUT_SIZE);
        fgets(line, MAX_INPUT_SIZE, stdin);
        FGProcessRunning = true;
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
