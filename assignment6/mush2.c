#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include "mush.h"

void handler(int signum);

int children = 0;

int main(int argc, char *argv[]){
    int i, done, input_fd, output_fd, **pipes;
    char *input, *home;
    FILE *in, *out;
    pid_t pid, child;
    pipeline pipeval;
    struct clstage *stage;

    in = stdin;
    out = stdout;
    home = getenv("HOME");
    while(!feof(stdin)){
        printf("8-P ");
        signal(SIGINT, handler);
        /*create command line*/
        if(((input = readLongString(in)) == NULL) || (!*input)){
            free(input);
            continue;
        }
        if((pipeval = crack_pipeline(input)) == NULL){
            free(input);  
            free_pipeline(pipeval);
            yylex_destroy();
            continue;
        }

        /*special case*/
        if(strcmp("cd", pipeval -> stage -> argv[0]) == 0){
            if(pipeval -> stage -> argc == 1){
                if(chdir(home) == -1){
                    fprintf(stderr, "unable to determine home directory\n");
                }
            }
            else{
                if(chdir(pipeval -> stage -> argv[1]) == -1){
                    fprintf(stderr, "Not a directory\n");
                }
            }
            free(input);
            free_pipeline(pipeval);
            yylex_destroy();
            continue;
        }
        /* create pipeage */
        pipes = (int**)malloc((pipeval -> length + 1) * sizeof(int*));
        if(pipes == NULL){
            fprintf(stderr, "malloc failure\n");
            goto end;
        }
        for(i = 0; i <= pipeval -> length; i++){
            if((pipes[i] = (int*)malloc(2*sizeof(int))) == NULL){   
                fprintf(stderr, "malloc failure\n");
                goto end;
            }
            if(pipe(pipes[i]) == -1){
                fprintf(stderr, "pipe failure\n");
                goto end;
            }
        }
        /* core code */
        for(i = 0; i < pipeval -> length; i++){
            stage = &pipeval -> stage[i];
            
            /* fork */
            children += 1;
            if((pid = fork()) == -1){
                fprintf(stderr, "fork failure\n");
                goto end;
            }     
            else if(pid == 0){
                /*child process*/
                
                /* set up pipes */
                if(i < (pipeval -> length - 1) && i != (pipeval -> length - 1)){
                    if((dup2(pipes[i + 1][1], STDOUT_FILENO)) == -1){
                        fprintf(stderr, "pipe dup2 failure\n");
                        goto end;
                    }
                }
                if(i > 0){
                    if((dup2(pipes[i][0], STDIN_FILENO)) == -1){
                        fprintf(stderr, "pipe dup2 failure\n");
                        goto end;
                    }
                }
                /* close pipes */
                for(i = 0; i <= pipeval -> length - 1; i++){
                    close(pipes[i][0]);
                    close(pipes[i][1]);
                }
                /* restores fd's if needed */
                if(i == (pipeval -> length - 1)){
                    if((dup2(0, STDIN_FILENO)) == -1){
                        fprintf(stderr, "dup2 failure\n");
                        goto end;
                    }
                    if((dup2(1, STDOUT_FILENO)) == -1){
                        fprintf(stderr, "dup2 failure\n");
                        goto end;
                    }   
                }
                /* handle IO redirection < > */
                if(stage -> inname){
                    if((input_fd = open(stage -> inname, O_RDONLY)) == -1){
                        fprintf(stderr, "no such file or directory\n");
                        goto end;
                    }
                    if((dup2(input_fd, fileno(in))) == -1){
                        fprintf(stderr, "input dup2 failure\n");
                        goto end;
                    }
                }
                if(stage -> outname){
                    if((output_fd = open(stage -> outname, 
                    O_WRONLY|O_CREAT|O_TRUNC, 0666)) == -1){
                        fprintf(stderr, "open failure\n");
                        goto end;
                    }
                    if((dup2(output_fd, fileno(out))) == -1){
                        fprintf(stderr, "output dup2 failure\n");
                        goto end;
                    }
                }
                /* execute child */
                execvp(stage -> argv[0], stage -> argv);
                fprintf(stderr, "%s: No such file or directory\n", stage -> argv[0]);
                return 0;
            }
            else{
                /* parent process */
                close(pipes[i][0]);
                close(pipes[i][1]);
                if(i == (pipeval -> length - 1)){
                    close(pipes[i + 1][0]);
                    close(pipes[i + 1][1]);
                }
                if((child = wait(&done)) == -1){
                    goto end;
                }
            }
        }
        end:
        children = 0;
        for(i = 0; i <= pipeval -> length; i++){
            free(pipes[i]);
        }
        free(pipes);
        free(input);
        free_pipeline(pipeval);
        yylex_destroy();
    }
    return 0;
}

void handler(int signum){
    if(children == 0){
       printf("\n8-P ");    
       fflush(stdout);
       return;
    }
    while(children > 0){
        wait(NULL);
        children--;
    }
}

