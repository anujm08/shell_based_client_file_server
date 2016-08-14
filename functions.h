
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
    char *arguments[] = {"./get-one-file-sig", filename, sIP, sPort, displayMode, NULL};
    if (execvp(arguments[0], arguments))
        perror("ERROR getfl");
}

void getsq(char** tokens)
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
            getfl(tokens[i], "nodisplay");
        }
        else
        {
            waitpid(pid, NULL, 0);
        }
    }
    exit(0);
}

void getpl(char** tokens)
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
            getfl(tokens[i], "nodisplay");
        }
    }
    while (i > 1)
    {
        // TODO : waipid(-1, NULL, 0) can be replaced by wait(NULL)?
        waitpid(-1, NULL, 0);
        i--;
    }
    exit(0);
}