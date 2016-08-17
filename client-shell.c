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
static bool FGWasRunning = false;

static char* getflExec;


void error(char *msg)
{
    perror(msg);
    exit(1);
}

void sigIntHandlerPseudo(int sig_num)
{
    // wait for all childs with same process group ID i.e all foreground process
    while (waitpid(0, NULL, 0) > 0) {}

    // if a fg process was not even running, then we need a new Hello> prompt
    if (!FGWasRunning)
        printf("\nHello>");
    fflush(stdout);
}

void sigIntHandlerKill(int sig_num)
{   
    exit(0);
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

void cd(char** tokens)
{
    if (chdir(tokens[1]) != 0)
    {
        char errorMsg[MAX_TOKEN_SIZE + 20];
        sprintf(errorMsg, "bash: cd: %s", tokens[1]);
        perror(errorMsg);
    }
}

void getfl(char* filename, char* displayMode)
{   
    char* sIP = strdup(serverIP.c_str());
    char* sPort = strdup(serverPort.c_str());
    char* arguments[] = {getflExec, filename, sIP, sPort, displayMode, NULL};
    if (execvp(arguments[0], arguments))
        perror("ERROR getfile");
}

void getsq(char** tokens)
{   
    signal(SIGINT, sigIntHandlerKill);
    // fork one, exec one, reap one
    for (int i = 1; tokens[i] != NULL; i++)
    {
        pid_t pid = fork();
        if (pid < 0) 
        {
            perror("ERROR forking child process failed");
            return;
        }
        else if (pid == 0)               
            getfl(tokens[i], "nodisplay");
        else
            waitpid(pid, NULL, 0);
    }
    exit(0);
}

void getpl(char** tokens)
{   
    signal(SIGINT, sigIntHandlerKill);
    int i;
    // fork the processes simultaneously
    for (i = 1; tokens[i] != NULL; i++)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("ERROR forking child process failed");
            return;
        }
        else if (pid == 0)         
            getfl(tokens[i], "nodisplay");
    }
    // wait for each process it spawned
    while (waitpid(0, NULL, 0) > 0) {}

    exit(0);
}

void getflRedirection(char* downloadFile, char* outputFile)
{
    int fileFD = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 644);
    if (fileFD < 0)
    {
        error("ERROR can't open output file");
    }
    else
    {
        // make STDOUT fd become a copy of fileFD
        dup2(fileFD, STDOUT_FILENO);
        // we can close fileFD as we have made a copy
        close(fileFD);
        getfl(downloadFile, "display");
    }
}

void getflPipe(char* downloadFile, char** tokens)
{
    int newFD[2];
    // creates new file descriptors, pointing to a pipe inode
    // newFD[0] for reading while newFD[1] for writing
    pipe(newFD);

    pid_t childpid = fork();

    if (childpid < 0)
    {
        perror("ERROR forking child process failed");
    }
    // the child process will exec the get-one-file executable
    else if (childpid == 0)
    {   
        // close the reading FD
        close(newFD[0]);
        // write to writing FD instead of STDOUT
        dup2(newFD[1], STDOUT_FILENO);
        // we can close writing FD as we have made a copy 
        close(newFD[1]);
        getfl(downloadFile, "display");
    }
    // parent executes the right side command
    else 
    {
        // close the writing FD
        close(newFD[1]);
        // read from reading FD instead of STDIN
        dup2(newFD[0], STDIN_FILENO);
        // we can close reading FD as we have made a copy 
        close(newFD[0]);
        if (execvp(*tokens, tokens) < 0)
        {
            fprintf(stderr, "%s: Command not found\n", tokens[0]);
            exit(0);
        }
    }
}

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

void setServer(char** tokens)
{
    if (tokens[1] == NULL || tokens[2] == NULL || tokens[3] != NULL)
        fprintf(stderr, "usage: server [server-ip] [server-port]\n");
    else
    {
        serverIP = string(tokens[1]);
        serverPort = string(tokens[2]);
    }
}

void FGProcess(char** tokens)
{
    string ftoken(tokens[0]);

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
    
    if (ftoken == "getbg")
    {
        if (tokens[1] == NULL || tokens[2] != NULL)
            fprintf(stderr, "usage: getbg [filename]\n");
        else
            getfl(tokens[1], "nodisplay");
    }
    exit(0);
}

void shellProcess(char** tokens)
{   
    string ftoken(*tokens);

    if (ftoken == "server")
    {
        setServer(tokens);
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
        MTX.lock();
        while (!BGprocs.empty())
        {   
            // -1 specifies `any` child, so even though gpid is different
            // the bgprocs will be reaped, as they are children of the shell
            pid_t killpid = waitpid(*BGprocs.begin(), NULL, 0);
            BGprocs.erase(killpid);
        }
        MTX.unlock();
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

    getflExec = realpath("./get-one-file-sig", NULL);
    if (getflExec == NULL)
        perror("ERROR getting get-one-file-sig executable");
    // thread which reaps children
    pthread_create(&reaperT, NULL, reapChildren, NULL);

    while (true)
    {           
        FGWasRunning = false;
        printf("Hello>");
        fflush(stdout);
        bzero(line, MAX_INPUT_SIZE);
        fgets(line, MAX_INPUT_SIZE, stdin);
        FGWasRunning = true;
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
