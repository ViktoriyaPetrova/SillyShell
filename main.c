#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>

char* get_input(){
    char src[4096];

    printf("SILLY>");
    if(fgets(src, 4096, stdin) == NULL){
        fprintf(stderr, "ERROR: Input\n");
        exit(1);
    }
    if((strlen(src) > 0) && (src[strlen(src) - 1] == '\n'))
        src[strlen(src) - 1] = '\0';

    char *input = (char *) malloc(strlen(src) + 1);
    if(input == NULL){
        fprintf(stderr, "ERROR: No memory.\n");
        exit(1);
    }

    strcpy(input,src);
    return input;
}

int word_count(char *input){
    int wc = 0;
    bool letter = false;

    for(int i = 0; input[i] != '\0'; i++){
        if(input[i] == ' ')
            letter = false;
        else{
            if(letter == false)
                wc++;
            letter = true;
        }
    }
    return wc;
}

int get_args(char *input, char **silly_argv){
    char *token;
    int i = 0;

    token = strtok(input, " ");
    while(token != NULL){
        silly_argv[i] = token;
        i++;
        token = strtok(NULL, " ");
    }
    silly_argv[i] = NULL;
    return i;
}

void sigchld_handler(int signum){
    int status;
    pid_t pid;

    while((pid = waitpid(-1, &status, WNOHANG)) > 0) {}
}

void exit_wait(){
    int status;
    pid_t pid;

    printf("Waiting on background processes to finish...\n");
    while((pid = waitpid(-1, &status, 0)) > 0) {}
    printf("Done.\nGoodbye!\n");
}

void run_foreground(pid_t child_pid, char **silly_argv){
    int status;

    if(child_pid == 0){
        if(execvp(silly_argv[0], silly_argv) < 0){
            fprintf(stderr, "ERROR: Exec failed\n");
            exit(1);
        }
    }
    else{
        if((child_pid = waitpid(child_pid, &status, 0)) < 0){
            fprintf(stderr, "ERROR: Waitpid failed\n");
            exit(1);
        }
    }
}

void run_background(pid_t child_pid, char **silly_argv){
    int status;

    if(child_pid == 0){
        if(execvp(silly_argv[0], silly_argv) < 0){
            fprintf(stderr, "ERROR: Exec failed\n");
            exit(1);
        }
    }
    else{
        if((child_pid = waitpid(child_pid, &status, WNOHANG)) < 0){
            fprintf(stderr, "ERROR: Waitpid failed\n");
            exit(1);
        }
    }
}

void create_fork(int silly_argc, char **silly_argv){

    //Signal Handling
    struct sigaction new_action, old_action;
    new_action.sa_handler = sigchld_handler;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = SA_RESTART;

    sigaction(SIGCHLD, NULL, &old_action);
    if(old_action.sa_handler != SIG_IGN)
        sigaction(SIGCHLD, &new_action, NULL);

    //Child Fork
    pid_t child_pid = fork();
    if(child_pid < 0){
        fprintf(stderr, "ERROR: Fork failed\n");
        exit(1);
    }

    //Execute in the background or foreground
    if(strcmp(silly_argv[silly_argc - 1], "&") == 0){
        silly_argv[silly_argc - 1] = NULL;
        run_background(child_pid, silly_argv);
    }
    else
        run_foreground(child_pid, silly_argv);
}

int main(int argc, char *argv[]){
    bool flag = false;
    int index;
    int silly_argc;
    char **silly_argv;
    char *input;

    do{
        input = get_input();
        index = word_count(input);

        silly_argv = malloc(sizeof(char *) * (index + 1));
        if(silly_argv == NULL){
            fprintf(stderr, "ERROR: No memory.\n");
            exit(1);
        }

        silly_argc = get_args(input, silly_argv);

        if(strcmp(silly_argv[0], "exit") == 0)
            flag = true;

        if(flag == false)
            create_fork(silly_argc, silly_argv);

        free(silly_argv);
        free(input);
    }while(flag == false);

    exit_wait();

    exit(0);
}
