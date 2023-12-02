#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

int main() {
    dup2(STDOUT_FILENO , STDOUT_FILENO);//for solving an issue
    printf("I'm SHELL process, with PID: %d - Main command is: man grep | grep \"-w\" -A 7 -w \n", (int)getpid());
    int fd[2];
    pipe(fd);
    int rc = fork();

    if (rc < 0) {
        // Fork Failed
        fprintf(stderr, "Fork failed!\n");
        exit(1);
    } else if (rc == 0) {
    //Child 1
    
        printf("I am MAN process, with pid: %d - My command is: man grep\n", (int)getpid());
     
    	
        close(fd[0]); // close read
        dup2(fd[1], STDOUT_FILENO);

        char* myargs[3];
        myargs[0] = "man";
        myargs[1] = "grep";
        myargs[2] = NULL;
        execvp(myargs[0], myargs);
        fprintf(stderr, "execvp failed\n");
        exit(1);
    }
    
    int fd2[2];
    pipe(fd2);
    
    int rc2 = fork();
    
    if (rc2 < 0) {
        // Fork Failed
        fprintf(stderr, "Fork failed!\n");
        exit(1);
  
    } else if (rc2 == 0) {
        // Child 2
        printf("I'm GREP process, with PID: %d - My command is: grep \"-w\" -A 7 -w\n", (int)getpid());
        close(fd[1]); // close write
        dup2(fd[0], STDIN_FILENO);
        dup2(fd2[1], STDOUT_FILENO);

        char* myargs[6];
        myargs[0] = "grep";
        myargs[1] = "\\-w";
        myargs[2] = "-A";
        myargs[3] = "7";
        myargs[4] = "-w";
        myargs[5] = NULL;
        execvp(myargs[0], myargs);
        fprintf(stderr, "execvp failed\n");
        exit(1);
    }
    
     wait(NULL);
    close(fd[0]);
    close(fd[1]);
    close(fd2[1]);
    
    dup2(STDOUT_FILENO , STDOUT_FILENO); //for solving an issue
    printf("I'm SHELL process, with PID: %d - execution is completed, you can find the results in output.txt \n", (int)getpid());
    
      // Read the content of the pipe into 'buff'
    char buff[1000];
    read(fd2[0], buff, sizeof(buff));

    // Open the output file for writing
    int x = open("output.txt", O_CREAT | O_WRONLY, 0644);

    // Redirect stdout to the 'output.txt' file
    dup2(x, STDOUT_FILENO);

    // Print the content of 'buff' to 'output.txt'
    printf("%s", buff);
    return 0;
}

