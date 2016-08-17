
void error(char *msg)
{
    perror(msg);
    exit(1);
}

void sigIntHandlerPseudo(int sig_num)
{
    // wait for all childs with same process group ID i.e all foreground process
    while (1)
    {
        pid_t killpid = waitpid(0, NULL, 0);
        if (errno == ECHILD)
            break;
    }
    if (!FGProcessRunning)
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
    // TODO : potential memory leak
    char* sIP = strdup(serverIP.c_str());
    char* sPort = strdup(serverPort.c_str());
    char* arguments[] = {getflExec, filename, sIP, sPort, displayMode, NULL};
    if (execvp(arguments[0], arguments))
        perror("ERROR getfile");
}

void getsq(char** tokens)
{   
    signal(SIGINT, sigIntHandlerKill);
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
    for (; i > 1; i--)
        // TODO : waipid(-1, NULL, 0) can be replaced by wait(NULL)?
        waitpid(-1, NULL, 0);

    exit(0);
}

void getflRedirection(char* downloadFile, char* outputFile)
{
    int fileFD = open(outputFile, O_RDWR | O_CREAT | O_TRUNC);
    if (fileFD < 0 )
    {
        error("Can't open output file");
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