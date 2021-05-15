#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <wait.h>
#include <sys/types.h>
void exec();
void execOR();
void execPP();
void getPath();
void setPipes();
//generous constant sizes
#define MAX_INPUT_LINE 100
#define MAX_PATH_LENGTH 500

int main(int argc, char *argv[]) {
    char input[MAX_INPUT_LINE];
    
    //obtaining prompt line
    FILE* promptfile  = fopen(".sh360rc", "r");
    //generous buffer size
    char promptLine[50];  
    fgets(promptLine, MAX_INPUT_LINE, promptfile);
    char *p = promptLine;
    p[strlen(p) - 1] = 0;
    
    //loop prompting for input, method taken from appendix a. Use of strtok was 
    //borrowed from appendix e.
    while(1) {
        fprintf(stdout, "%s", promptLine);  
        fflush(stdout);
        fgets(input, MAX_INPUT_LINE, stdin);
        
        if (input[strlen(input) - 1] == '\n') 
            input[strlen(input) - 1] = '\0';
        
        if (strcmp(input, "exit") == 0) 
            exit(0);

        if (strcmp(input, "") == 0) 
            continue;
        
        char* tok;        
        char *args[12];      
        int nargs = 0;
        tok = strtok(input, " ");
        
        //tokenize input, storing each token in args
        while (tok != NULL) {
            args[nargs] = tok;
            nargs++;
            if(nargs == 11)
                break;
            tok = strtok(NULL, " ");
        }

        //supporting up to 10 arguments, interpreted as tokens. 10 rather then 7 to support the second example for PP in the writeup.
        if(nargs > 10){
            fprintf(stderr, "maximum number of arguments is 10\n");
            continue;
        }

        //work is split for PP, OR and executing a single process
        if(strcmp(args[0], "PP") == 0)
          execPP(args, nargs);
        else 
        if(strcmp(args[0], "OR") == 0)
          execOR(args, nargs);
        else
          exec(args, nargs);
  }
}


/*
getPath: given the name of a command this function iterates through the paths contained
in .sh360rc. If the command may be found and is executable at the end of a path it is append onto 
the path and stored in the line buffer. If the command may not be found "-1" is 
placed into the line buffer.

line: buffer to store the path in
args: collection of input arguments
argindex: index where the name of executable is located in args

Usage of the stat function to determine file permissions is from
https://codeforwin.org/2018/03/c-program-find-file-properties-using-stat-function.html#:~:text=stat()%20function%20in%20C&text=stat()%20function%20is%20used,is%20defined%20in%20sys%2Fstat.
*/
void getPath(char* line, char *args[10], int argindex){
     FILE* pathFile = fopen(".sh360rc", "r");
     fgets(line, MAX_PATH_LENGTH, pathFile);             
     

     /*
     loop gets each path in .sh360rc and appends the command onto
     the end, checking if it is executable.
     */
     while (fgets(line, MAX_PATH_LENGTH, pathFile)) {
         for(size_t i = 0; i < MAX_PATH_LENGTH; i++){
             if (line[i] == '\n' || line[i] == 0){      
                 line[i] = '/';                          
                 int j = 0;
                 for(; j < strlen(args[argindex]); j++)
                     line[i + j + 1] = args[argindex][j];
                 line[i + j + 1] = 0;
                 break;
              }
         }
         struct stat stats;
         if (stat(line, &stats) == 0 && stats.st_mode & X_OK){
             //the current path in line has been found to be executable
             return;
         }
     }
    
    //if we never found the command, return -1.
    line[0] = '-';        
    line[1] = '1';
    line[2]= 0;
}

/*
exec: function to execute a single command with no PP or OR.
args is the collection of tokenized arguments from the input and nargs is the number of them
method of branching into a child process was taken from appendix b
*/
void exec(char *args[10], int nargs){
    char line[MAX_PATH_LENGTH];   
    getPath(line, args, 0);
    if(strcmp(line, "-1") == 0){
        fprintf(stderr, "unable to locate binary for command\n");
        return;
    }
    int pid, status; 
    char *envp[] = {0};
    args[nargs] = 0;             
    pid = fork();   
    if(pid < 0){
        fprintf(stderr, "fork failed\n");
    }       
    if(pid == 0)
        execve(line, args, envp);   
    waitpid(pid, &status, 0);
}


/*
execOR: function to handle the setup of an OR command.
args is the collection of tokenized arguments from the input and nargs is the number of them
method of branching into a child process was taken from appendix b, writing to a file was taken from appendix c
*/
void execOR(char *args[10], int nargs){
    //checking for proper form of input here
    //if flag is -1 we have seen 0 or greater than 2 -> tokens. It is the index of the -> otherwise.
    int flag = -1;
    for(int i = 0; i < nargs; i++){
      if(strcmp(args[i], "->") == 0){
        if(flag != -1){
            flag = -1;
            break;
        }
        flag = i;
      }
    }
    // -> should not be the first arugment after OR, it should not be the last, and we check that it is the second last.
    if(flag == -1 || strcmp(args[1], "->") == 0 || strcmp(args[nargs - 2], "->") != 0 || strcmp(args[nargs - 1], "->") == 0){
        fprintf(stderr, "improper input form for OR\n");
        return;
    }
    
    //input okay, check if the command is executable
    char line[MAX_PATH_LENGTH];    
    getPath(line, args, 1);
    if(strcmp(line, "-1") == 0){
        fprintf(stderr, "unable to locate binary for command\n");
        return;
    }
    
    //set up for process creation
    int pid, status; 
    char *envp[] = {0};
    pid = fork();
    if(pid  < 0){
        fprintf(stderr, "fork failed\n");
    }     
    if(pid == 0){
        int fd = open(args[nargs - 1], O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
        if (fd == -1) {
            fprintf(stderr, "cannot open given file for writing\n");
            exit(1);
        }
        //redirect output
        dup2(fd, 1);
        dup2(fd, 2); 
        //new arg buffer for the command, we remove the leading OR and trailing "->" along with the file name
        char *args2[10];         
        int k = 0;
        int nargs2 = nargs - 3;
        
        for(; k < nargs2; k++)  
            args2[k] = args[k + 1];
        
        args2[nargs2] = 0;
        execve(line, args2, envp);  
    }

    waitpid(pid, &status, 0);
}


/*
execPP: function to handle the setup of an PP command.
args is the collection of tokenized arguments from the input and nargs is the number of them
*/
void execPP(char *args[10], int nargs){
    /*
    these store the indicies of the -> tokens. under the assignment specs with 7 arguments a valid input
    may have at most 2 -> symbols. The OR arg + 3 commands + 2 -> = 6 tokens leaving only room for another -> with no trailing command to be piped to. 
    */
    int pip1 = -1;
    int pip2 = -1; 
   
    for(int i = 1; i < nargs; i++){
        if(strcmp(args[i], "->") == 0){
            if(pip1 == -1)
                pip1 = i;
        else if(pip2 == -1)
                 pip2 = i;
             else{
                //found more that 2 -> tokens
                fprintf(stderr, "improper use of PP, maximum of two -> is supported\n");;
                return;
            }
        }
    }
   
   //we check that no pipes are adjacent, must have found at least one pipe, the command may not end on a pipe, and the command may not begin on a pipe;
    if(abs(pip1 - pip2) == 1 || (pip1 == -1 && pip2 == -1) || pip1 == (nargs - 1) || pip2 == (nargs - 1) || pip1 == 1 || pip2 == 1) {
        printf("improper input form for PP\n");
        return;
     }
    
    //new structures to store the arguments for each command
    char *args1[10]; 
    char *args2[10]; 
    char *args3[10]; 
    int i = 0, nargs1 = 0, nargs2 = 0, nargs3 = 0;
    
    //store all tokens from PP to the first pipe (exclusive) in args1, with a count kept in nargs1
    for(i = 1; i < pip1; i++){
        args1[i - 1] = args[i];
        nargs1 = nargs1 + 1;
    }
    //args should be terminated with 0 for execve
    args1[nargs1] = 0;
    
    
    /*
    grabbing args for the second command. If we have 2 -> tokens, want to grab args from after the first -> up until the second,
    so pip1 until pip2. If there is no second pipe, then all remaning args should be for the second command, so we 
    go until nargs.
    */
    int n = pip2;
    if(pip2 == -1)
        n = nargs;
    
    for(i = pip1 + 1; i < n; i++){
        args2[i - (pip1 + 1)] = args[i];
        nargs2++;
    }
    args2[nargs2] = 0;
    
    //if we have a second pipe, and thus a third command we populate the args for it.
    if(pip2 != -1){
       for(i = pip2 + 1; i < nargs; i++){
           args3[i - (pip2 + 1)] = args[i]; 
           nargs3++;
       }
       args3[nargs3] = 0;
    }
    
    //buffers for the path to the binary of each command.
    char line1[MAX_PATH_LENGTH];
    char line2[MAX_PATH_LENGTH];
    char line3[MAX_PATH_LENGTH];
    //must have at least 2 commands to pipe
    getPath(line1, args1, 0);
    getPath(line2, args2, 0);
    
    //if we have a third command
    if(pip2 != -1){
        getPath(line3, args3, 0);
        if(strcmp(&line3[0], "-1") == 0){
            printf("unable to locate binary for command 3\n");
            return;
        }
    }
   
    if(strcmp(&line2[0], "-1") == 0){
        printf("unable to locate binary for command 2\n");
        return;
   }

   if(strcmp(&line1[0], "-1") == 0){
     printf("unable to locate binary for command 1\n");
            return;

   }

   //pass off to helper function to set up the pipes
   int numpipes = 1;
   if(pip2 != -1)
        numpipes = 2;
   setPipes(args1, args2, args3, &line1, &line2, &line3, numpipes);
  }


/*
setPipes: Helper function to split up the work of parsing a PP command and setting up the pipes/processes.
The first three parameters are the arguments for each command respectively, followed by the path to the binary for each command. 
The last command indicates if we have 1 or 2 pipes, and thus 2 or 3 commands.
method for setting up the pipes was taken from appendix d.
*/
void setPipes(char *args1[10], char *args2[10], char *args3[10], char *line1, char *line2, char *line3, int numpipes){
    char *envp[] = {0};
    int status;
    int pid1, pid2, pid3;
    int fd1[2];
    int fd2[2];
    
    pipe(fd1);
    pipe(fd2);

   //set up file descriptors and pass necessary data to execve for the first command
   if((pid1 = fork()) == 0) {
       
        dup2(fd1[1], 1);
        close(fd1[0]);
        close(fd2[0]);
        close(fd2[1]);
        execve(line1, args1, envp);
     }
        if(pid1 < 0){
           fprintf(stderr, "fork failed\n");
           return;
       }   
    //set up file descriptors and pass necessary data to execve for the second command
    if((pid2 = fork()) == 0) {
     
        dup2(fd1[0], 0);
        close(fd1[1]);
        close(fd2[0]);
        //if we have a third command, then set up the output of the second command to the second pipe
        if(numpipes == 2){
            dup2(fd2[1], 1);
            close(fd2[0]);
        }
        execve(line2, args2, envp);
      }
          if(pid2 < 0){
            fprintf(stderr, "fork failed\n");
            return;
        }    
    //if we have a third command, then set it to read the output of the second pipe
    if(numpipes == 2){
        if ((pid3 = fork()) == 0) {
        
            dup2(fd2[0], 0);
            close(fd2[1]);
            close(fd1[0]);
            close(fd1[1]);
            execve(line3, args3, envp);
        }
    }
        if(pid3 < 0){
                fprintf(stderr, "fork failed\n");
                return;
            }
    //close all fd for parent 
    close(fd1[0]);
    close(fd1[1]);
    close(fd2[0]);
    close(fd2[1]);
    
    //wait for all children
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);
    if(numpipes == 2)
        waitpid(pid3, &status, 0);
}
