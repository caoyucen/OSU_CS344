/*
 Program name: Program 2 - adventure
 Author: YUCEN CAO - CS344 Fall 2019
 ONID: caoyuc
 Date: Nov 20, 2019
 Description: In this assignment you will write your own shell, called smallsh.  This will work like the bash shell you are used to using, prompting for a command line and running commands, but it will not have many of the special features of the bash shell.Your shell will allow for the redirection of standard input and standard output and it will support both foreground and background processes (controllable by the command line and by receiving signals).The shell will support three built in commands: exit, cd, and status. It will also support comments, which are lines beginning with the # character.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>

int bg_switch = 1;   //foreground process only switch, 0 for only foreground mode



/* Description: show the exit ststus */
void exit_status(int status){
    if(WIFEXITED(status)){
        /* terminates normally */
        printf("exit value %d\n", WEXITSTATUS(status));
    }
    else{
        printf("terminated by signal %d\n", status);
    }
}



/* Description: add the background procese pid to the array*/
/* Arguments: background process pid array, childprocess pid*/
/* Pre-conditions:the child process is a background process */
/* conditions: the spawnPid is in the array */
void add_bgPid(pid_t* bg_Pid, pid_t spawnPid){
    int i = 0;
    while(bg_Pid[i]){
        i++;
    }
    bg_Pid[i] = spawnPid;
}



/* Description: when background process is done, so remove it from the head of the array*/
/* Arguments: background process pid array*/
/* Pre-conditions: background process is done*/
/* Post-conditions:  next background process is now the head*/
void remove_bgPid(pid_t* bg_Pid){
    int i = 0;
    while(bg_Pid[i]){
        if(bg_Pid[i+1]){
            bg_Pid[i] = bg_Pid[i+1];
        }
        i++;
    }
    bg_Pid[i - 1] = 0;
}



/* Description: Catch SIGINT, note that signal handlers in parent will be inherited by child process when using fork(), hence, we need to overwrite SIGINT handler action in
case child process requires different handling behavior*/
void catchSIGINT(int signal) {
    const char* msg_prefix = "Terminated by signal 2\n";
    if (signal == SIGINT) {
        write(1, msg_prefix, 24);
    }
}



/* Description: */
/* Arguments: */
/* Pre-conditions: */
/* Post-conditions:  */
void catchSIGTSTP(int signal) {
    if (bg_switch == 1) {
        bg_switch = 0;
        write(1, "\nEntering foreground-only mode (& is now ignored)\n", 50);
    } else {
        bg_switch = 1;
        write(1, "\nExiting foreground-only mode\n", 30);
    }
}



/* Description: check input has $$ or not, if has it, change $$ with the shell pid */
/* Arguments: the input which is already stored in the array*/
/* Pre-conditions: input is not NULL or #*/
/* Post-conditions:  if does not has $$, nothing change; if has, change $$ to shell pid */
void check_expand_DS(char* oneArgument){
    int i = 0;
    int first = 0;
    int mark = 0;
    int find = 0;
    char *temp;
    temp = (char*) malloc(50*sizeof(char));
    
    /*check if it has $$ in it */
    while(oneArgument[i]){
        if(oneArgument[i] == '$'){
            if(first == 1){
                find = 1;
                break;
            }
            if(first == 0){
                first = 1;
                mark = i;
            }
        }
        else if(first == 1){
            first = 0;
            mark = 0;
        }
        i++;
    }
    /* has the $$, change it to shell pid */
    if(find == 1){
        /*get the pid*/
        pid_t shell_pid = getpid();
        /*expand the $$*/
        if(oneArgument[mark + 2]){
            /* when it has something after the $$ */
            temp = strcpy(temp, (oneArgument + mark + 2));
            sprintf((oneArgument + mark), "%d", (int)shell_pid);
            strcat(oneArgument, temp);
        }
        else{
            /* nothing after the $$ */
            sprintf((oneArgument + mark), "%d", (int)shell_pid);
        }
    }
}


int main() {
    char*   input_file = NULL;              //input file name
    char*   output_file = NULL;             //output file name
    int     fileDescriptor_input = -1;
    int     fileDescriptor_outputput = -1;
    int     fg_process;                     //the mark for foreground or background process
    char    input[2048];
    char*   arguments[512];
    int     mark;                           //the mark for the arguments index
    char*   token;
    pid_t   spawnPid = -1;
    pid_t   bg_Pid[50];           // non completed pid array in the back ground
    int     status = 0;
    struct sigaction SIGINT_action = {0};
    struct sigaction SIGTSTP_action = {0};
    
    
    memset(bg_Pid, 0, 50);
    
    /* adding handler for SIGINT */
    SIGINT_action.sa_handler = catchSIGINT;
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = SA_RESTART;
    sigaction(SIGINT, &SIGINT_action, NULL);

    /* adding handler for SIGTSTP */
    SIGTSTP_action.sa_handler = catchSIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
    
    while(1){
        fg_process = 1;                     //initialize it to 1(foreground process), 0 for background process
        
        fflush(stdout);
        printf(": ");
        fflush(stdout);
        
        //get the user input from the stdin
        if(fgets(input, 2048, stdin) == NULL){
            return (0);
        }
        
        mark = 0;
        token = strtok(input, " \n");
        while(token != NULL){
            if(strcmp(token, "<") == 0){
                token = strtok(NULL, " \n");
                /*next is the input file name, store it*/
                input_file = (char*) malloc(100*sizeof(char));
                sprintf(input_file, "%s", token);
                token = strtok(NULL, " \n");
            }
            else if(strcmp(token, ">") == 0){
                token = strtok(NULL, " \n");
                /*next is the output file name, store it*/
                output_file = (char*) malloc(100*sizeof(char));
                sprintf(output_file, "%s", token);
                token = strtok(NULL, " \n");
            }

            else{
                arguments[mark] = (char*) malloc(50*sizeof(char));
                sprintf(arguments[mark], "%s", token);
                /* check the input has $$ or not, if yes, expand it to shell id*/
                check_expand_DS(arguments[mark]);
                token = strtok(NULL, " \n");
                mark++;
            }
        }
        if ((mark > 0)
            && (strcmp(arguments[mark - 1], "&") == 0)){
            /*if & is the last word , the command is to be executed in the background */
            /* check if it is the foreground process only mode*/
            if (bg_switch == 1) {
                fg_process = 0;                 /* mark it to the background process */
            }
            arguments[mark - 1] = NULL;         /* set the last one to null */
        }
        else{
            arguments[mark] = NULL;             /* the last one to null */
        }
        
        
        
        if( arguments[0] != NULL && arguments[0][0] != '#'){
            /* if input is not NULL or not begin with # character */
            /* Built-in Commands*/
            /* the input is "exit" */
            if(strcmp(arguments[0], "exit") == 0){
                exit(0);
            }
            /* the input begins with "cd" */
            else if(strcmp(arguments[0], "cd") == 0){
                if ( arguments[1] == NULL){
                    /* cd to home dir */
                    chdir(getenv("HOME"));
                }
                else{
                    /* cd to the following path */
                    chdir(arguments[1]);
                }
            }
            /* the input is "status" */
            else if(strcmp(arguments[0], "status") == 0){
                exit_status(status);
            }
            else{
                /* start a child process */
                spawnPid = fork();
                switch (spawnPid) {
                    case -1: {
                        perror("Hull Breach!\n");
                        status = 1;
                        break;
                    }
                    case 0: {
                        if (fg_process == 1) {
                            /* in the foreground process, set sigal*/
                            SIGINT_action.sa_handler = SIG_DFL;
                            signal(SIGTSTP, SIG_IGN);
                        } else {
                            /* in the background process, set sigal*/
                            signal(SIGINT, SIG_IGN);
                        }
                        if(input_file != NULL){
                            fileDescriptor_input = open(input_file, O_RDONLY);
                            if(fileDescriptor_input == -1){
                                printf("cannot open the %s for input\n", input_file);
                                status = 1;
                                _Exit(1); //terminates the children process
                            }
                            if(dup2(fileDescriptor_input, 0) == -1){
                                perror("dup2");
                                status = 1;
                                _Exit(1); //terminates the children process
                            }
                            close(fileDescriptor_input);
                        }
                        else if(fg_process == 0){
                            /*in the background process, Background commands should have their standard input redirected from /dev/null if the user did not specify */
                            fileDescriptor_input = open("/dev/null", O_RDONLY);
                            if(fileDescriptor_input == -1){
                                perror("background not specify open");
                                status = 1;
                                fflush(stdout);
                                _Exit(1);
                            }
                            if(dup2(fileDescriptor_input, 0) == -1){
                                perror("dup2");
                                status = 1;
                                _Exit(1);
                            }
                        }
                        
                        if(output_file != NULL){
                            fileDescriptor_outputput = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0744);
                            if(fileDescriptor_outputput == -1){
                                printf("cannot open the file: %s for reading\n", output_file);
                                status = 1;
                                fflush(stdout);
                                _Exit(1); //terminates the children process
                            }
                            if(dup2(fileDescriptor_outputput, 1) == -1){
                                perror("dup2");
                                status = 1;
                                _Exit(1); //terminates the children process
                            }
                            close(fileDescriptor_outputput);
                        }
                        else if(fg_process == 0){
                            /*in the background process, Background commands should have their standard output redirected from /dev/null if the user did not specify */
                            fileDescriptor_outputput = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0744);
                            if(fileDescriptor_outputput == -1){
                                perror("background not specify open");
                                status = 1;
                                fflush(stdout);
                                _Exit(1); //terminates the children process
                            }
                            if(dup2(fileDescriptor_outputput, 1) == -1){
                                perror("dup2");
                                status = 1;
                                _Exit(1); //terminates the children process
                            }
                        }
                        
                        if(execvp(arguments[0], arguments)){
                            /* Exec failure! */
                            printf("%s: no such file or directory\n", arguments[0]);
                            status = 1;
                            fflush(stdout);
                            _Exit(1);
                        }
                        break;
                    }
                    
                    default: {
                        if(fg_process == 1){
                            /* in the foreground process, wait child process terminates */
                            waitpid(spawnPid, &status, 0);
                        }
                        else {
                            /* background process, will not wait */
                            printf("background pid is %d\n", spawnPid);
                            /* store the background process id to the array bg_Pid*/
                            add_bgPid(bg_Pid, spawnPid);
                            break;
                        }
                    }
                }
            }
            
        }

        
        /* free the space and reset */
        int i;
        for(i = 0; arguments[i] != NULL; i++){
            free(arguments[i]);
        }
        free(input_file);
        input_file = NULL;
        free(output_file);
        output_file = NULL;
        
        
        /* To check the background process is done or not*/
        if(bg_Pid[0]){
            /* still has back ground process running before check*/
            pid_t temp = waitpid(bg_Pid[0], &status, WNOHANG);
            if(temp){
                /* the back ground process ends */
                printf("background pid %d is done: ", (int)temp);
                exit_status(status);
                /* detele the done background process from the head of the array*/
                remove_bgPid(bg_Pid);
            }
            
        }
        
        

    }
    
    return 0;
}
