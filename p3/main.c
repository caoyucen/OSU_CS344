//
//  main.c
//  p3
//
//  Created by Yucen Cao on 11/17/19.
//  Copyright © 2019 Yucen Cao. All rights reserved.
//

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

int bg_switch = 1;

void exit_status(int status){
    if(WIFEXITED(status)){
        // terminates normally
        printf("exit value %d\n", WEXITSTATUS(status));
    }
    else{
        printf("terminated by signal %d\n", status);
    }
}


void add_bgPid(pid_t* bg_Pid, pid_t spawnPid){
    int i = 0;
    while(bg_Pid[i]){
        i++;
    }
    bg_Pid[i] = spawnPid;
}

void remove_bgPid(pid_t* bg_Pid){
    //printf("come to remove\n");
    int i = 0;
    while(bg_Pid[i]){
        if(bg_Pid[i+1]){
            bg_Pid[i] = bg_Pid[i+1];
        }
        i++;
    }
    bg_Pid[i - 1] = 0;
}

/* Catch SIGINT, note that signal handlers in parent will be inherited by child process when using fork(), hence, we need to overwrite SIGINT handler action in
    case child process requires different handling behavior
*/
void catchSIGINT(int signal) {
    const char* msg_prefix = "Terminated by signal 2\n";
    if (signal == SIGINT) {
        write(1, msg_prefix, 24);
    }
}

/* */
void catchSIGTSTP(int signal) {
    if (bg_switch == 1) {
        bg_switch = 0;
        write(1, "\nEntering foreground-only mode (& is now ignored)", 49);
    } else {
        bg_switch = 1;
        write(1, "\nExiting foreground-only mode", 29);
    }
}

int main() {
    char*   input_file = NULL;
    char*   output_file = NULL;
    int     fileDescriptor_input = -1;
    int     fileDescriptor_outputput = -1;
    int     fg_process;
    char    input[2048];
    char*   arguments[512];   //2048/512, so I choose the big number 50, which is bigger than it
    int     mark;
    char*   token;
    pid_t   spawnPid = -1;
    pid_t   bg_Pid[50];           // non completed pid array in the back ground
    int     status = 0;
    //int     exit_flag = 0;
    struct sigaction SIGINT_action = {0};
//    struct sigaction SIGUSR2_action = {0};
    struct sigaction SIGTSTP_action = {0};
    
    
    memset(bg_Pid, 0, 50);
    
    SIGINT_action.sa_handler = catchSIGINT;
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = SA_RESTART;
    sigaction(SIGINT, &SIGINT_action, NULL);

    /* adding handler for SIGTSTP */
    SIGTSTP_action.sa_handler = catchSIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
    //signal(SIGTSTP, SIG_IGN);
    // signal(SIGCHLD, SIG_IGN); // this is not desired, we still want to resume the waiting
//
//    SIGUSR2_action.sa_handler = catchSIGUSR2;
//    sigfillset(&SIGUSR2_action.sa_mask);
//    SIGUSR2_action.sa_flags = 0;
//    ignore_action.sa_handler = SIG_IGN;
    
    while(1){
        fg_process = 1;                     //initialize it to 1(foreground process), 0 for background process
        
        fflush(stdout);
        printf(": ");
        fflush(stdout);
        
        if(fgets(input, 2048, stdin) == NULL){
            return (0);
        }
        
        mark = 0;
        token = strtok(input, " \n");
        while(token != NULL){
             //printf( "%s\n", token );
            if(strcmp(token, "<") == 0){
                token = strtok(NULL, " \n");
                input_file = (char*) malloc(100*sizeof(char));
                sprintf(input_file, "%s", token);
                token = strtok(NULL, " \n");
            }
            else if(strcmp(token, ">") == 0){
                token = strtok(NULL, " \n");
                output_file = (char*) malloc(100*sizeof(char));
                sprintf(output_file, "%s", token);
                token = strtok(NULL, " \n");
            }

            else{
                arguments[mark] = (char*) malloc(50*sizeof(char));
                if(strcmp(token, "$$") == 0){
                    pid_t shell_pid = getpid();
                    sprintf(arguments[mark], "%d", shell_pid);
                }
                else{
                    sprintf(arguments[mark], "%s", token);
                }
                token = strtok(NULL, " \n");
                mark++;
            }
        }
        if ((mark > 0)
            && (strcmp(arguments[mark - 1], "&") == 0)){
            //& is the last word , the command is to be executed in the background
            if (bg_switch == 1) {
                fg_process = 0; //mark it to the background process
            }
            arguments[mark - 1] = NULL;         //set the last one to null
        }
        else{
            arguments[mark] = NULL;//the last one to null
        }
        
        
        
        if( arguments[0] != NULL && arguments[0][0] != '#'){        //if input is not NULL or the  not begin with # character
            // the input is "exit"
            //printf("argument[0] = %s, mark=%d\n", arguments[0], mark);
            if(strcmp(arguments[0], "exit") == 0){
                exit(0);
            }
            // the input begins with "cd"
            else if(strcmp(arguments[0], "cd") == 0){
                if ( arguments[1] == NULL){
                    // cd to home dir
                    chdir(getenv("HOME"));
                }
                else{
                    // cd to the following path
                    chdir(arguments[1]);
                }
            }
            // the input is "status"
            else if(strcmp(arguments[0], "status") == 0){
                // need to do here 写status如何如何 调用 status 的函数？
                exit_status(status);
            }
            else{
                spawnPid = fork();
                switch (spawnPid) {
                    case -1: {
                        perror("Hull Breach!\n");
                        status = 1;
                        break;
                    }
                    case 0: {
                        // first handle the singel 需要写singel
                        if (fg_process == 1) {
                            //printf("hello world\n");
                            SIGINT_action.sa_handler = SIG_DFL;
                            //sigaction(SIGTSTP, &SIGTSTP_action, NULL);
                            signal(SIGTSTP, SIG_IGN);
                        } else {
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
                            //in the background process, the user did not specify some other file to take standard input from
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
                        
                        
                        if(execvp(arguments[0], arguments)){
                            printf("%s: no such file or directory\n", arguments[0]);
                            status = 1;
                            fflush(stdout);
                            _Exit(1);
                        }
                        break;
                    }
                    
                    default: {
                        //为什么 fork default 是什么？
                        if(fg_process == 1){
                            // in the foreground process, wait child process terminates
                            
                            waitpid(spawnPid, &status, 0);
                        }
                        else {   //为什么这里会有两遍？？
                            //background process, will not wait
                            printf("background pid is %d\n", spawnPid);
                            add_bgPid(bg_Pid, spawnPid);
                            break;
                        }
                    //    break;
                    }
                }
            }
            
        }

        
        // free and reset the space
        int i;
        for(i = 0; arguments[i] != NULL; i++){
            free(arguments[i]);
        }
        free(input_file);
        input_file = NULL;
        free(output_file);
        output_file = NULL;
        
        //printf("end \n");
        
        
        //printf("bg_Pid[0] : %d\n", (int)bg_Pid[0]);
        if(bg_Pid[0]){
            //still has back ground process running
            pid_t temp = waitpid(bg_Pid[0], &status, WNOHANG);
            if(temp){
                // the back ground process ends
                printf("background pid %d is done: ", (int)temp);
                //handle the singal !!!??????!!!!!????????
                exit_status(status);
                remove_bgPid(bg_Pid);
                //printf("after remove bg_Pid[0] : %d\n", (int)bg_Pid[0]);
            }
            
        }
        
        

    }
    
    return 0;
}
