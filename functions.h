
void error(char *msg)
{
    perror(msg);
    exit(1);
}

void sigIntHandlerPseudo(int sig_num)
{
    printf("\nHello>");
    fflush(stdout);
}

void sigIntHandlerKill(int sig_num)
{   
    exit(0);
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
    char* arguments[] = {"./get-one-file-sig", filename, sIP, sPort, displayMode, NULL};
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
    int fd = open(outputFile, O_RDWR | O_CREAT | O_TRUNC);
    if (fd < 0 )
    {
        error("Can't open output file");
    }
    else
    {
        dup2(fd, fileno(stdout));
        getfl(downloadFile, "display");
        close(fd);
    }
}

void getflPipe(char* downloadFile, char** tokens)
{
    int fd[2];
    pipe(fd);

    pid_t childpid = fork();

    if (childpid < 0)
    {
        perror("ERROR forking child process failed");
    }
    else if (childpid == 0)
    {
        close(fd[0]);
        dup2(fd[1], fileno(stdout));
        close(fd[1]);
        getfl(downloadFile, "display");
    }
    else 
    {
        close(fd[1]);
        dup2(fd[0], fileno(stdin));
        close(fd[0]);
        if (execvp(*tokens, tokens) < 0)
        {
            fprintf(stderr, "%s: Command not found\n", tokens[0]);
            exit(0);
        }
        waitpid(childpid, NULL, 0);
    }
}