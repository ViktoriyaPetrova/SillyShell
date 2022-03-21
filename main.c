//Viktoriya Petrova
//CS333
//Lab3

//This program creates a Unix shell that is capable of executing commands in the background
//and in the foreground. It is also capable of handling the built it command "exit" which
//causes the program to terminate. The program will wait for all children processes to be
//done executing prior to terminating. It cannot handle any built in commands other than "exit".

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>


//This function gets the input from the user and allocates it into dynamic memory
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

//This function counts the number of words in the user input so that the
//proper amount of dynamic memory can me allocated for the silly_argv array.
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

//This function parses the user input into the silly_argv array. It also
//counts the number of silly_argv indices as it's parsing and returns it
//to main. Lastly, it makes sure that the last index of the array is set
//to NULL before returning.
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

//Signal handler for the SIGCHLD signal. This function
//is used by the background processes. It reaps any child
//process that has exited -1.
void sigchld_handler(int signum){
    int status;
    pid_t pid;

    while((pid = waitpid(-1, &status, WNOHANG)) > 0) {}
}

//This function waits for all background processes to finish
//when "exit" is called in the parent process. If there are running
//background processes this function will pause the parent execution
//until the children exit and are reaped properly.
void exit_wait(){
    int status;
    pid_t pid;

    printf("Waiting on background processes to finish...\n");
    while((pid = waitpid(-1, &status, 0)) > 0) {}
    printf("Done.\nGoodbye!\n");
}

//This function executes commands in the foreground. The parent
//process waits until the child is done executing the command before
//continuing.
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

//This function executes commands in the background. The parent
//process does not wait for the child to be done executing before
//continuing.
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

//This function is responsible for setting up the signal handler
//that reaps children which execute background processes. It also contains
//the call for "fork" to create all children processes needed. Lastly,
//it checks all commands for the "&" to see if they should be run in the
//foreground or background. Afterwards it calls the appropriate functions
//to execute the commands.
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
    bool flag = false;               //Did the user call "exit" in the parent process?
    int index;                       //Aids in dynamic memory allocation for silly_argv
    int silly_argc;                  //Size of silly_argv array
    char **silly_argv;               //Parsed command line input
    char *input;                     //Command line input

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
