#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <vector>
#include <fcntl.h>
#include <cstring>

using namespace std;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void* thread_print(void* arg) {
    FILE* file = fdopen(*(int*)arg, "r");

    pthread_mutex_lock(&lock);
    cout << "----" << pthread_self() << endl;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file)) {
        cout << buffer;
    }
    cout << "----" << pthread_self() << endl;
    fflush(stdout);
    pthread_mutex_unlock(&lock);
    return NULL;
}

struct CommandLine {
    string command;
    string input;
    string option;
    string redirection;
    string backgroundjob;
    string filename;
    int numarg;
};

int main() {

    //-----Parsing-----

    ifstream inputFile("commands.txt");
    ofstream outputFile("parse.txt");
    string line;
    string word;
    vector<CommandLine> alldata; // all command lines are stored here

    while (getline(inputFile, line)) {
        istringstream iss(line);
        vector<string> commandinfo;
        while (iss >> word) {
            commandinfo.push_back(word);
        }
        CommandLine x;
        x.numarg = commandinfo.size();

        x.command = commandinfo[0];
        if (commandinfo[1][0] == '-') {
            x.option = commandinfo[1];
            x.input = "";
        }
        else {
            x.input = commandinfo[1];
            if (commandinfo[2][0] == '-') {
                x.option = commandinfo[2];
            }
            else {
                x.option = "";
            }
        }
         x.redirection = "-";
         x.filename = "";
        for (int i = 0; i < commandinfo.size(); i++) {
            if (commandinfo[i] == "<" || commandinfo[i] == ">") {
                x.redirection = commandinfo[i];
                x.filename = commandinfo[i + 1];
                // filename and redirection are not counted as arg
                x.numarg -= 2;
            }
        }
        if (commandinfo[commandinfo.size() - 1] == "&") {
            x.backgroundjob = "y";
            x.numarg -= 1; // background is not counted as arg
        }
        else {
            x.backgroundjob = "n";
        }
        alldata.push_back(x);
    }
    inputFile.close();

    //-----Printing-----

    for (CommandLine elem : alldata) {
        outputFile << "----------" << endl;
        outputFile << "Command: " << elem.command << endl;
        outputFile << "Inputs: " << elem.input << endl;
        outputFile << "Options: " << elem.option << endl;
        outputFile << "Redirection: " << elem.redirection << endl;
        outputFile << "Background Job: " << elem.backgroundjob << endl;
        outputFile << "----------" << endl;
    }

    //---Executing Commands---

    vector<pthread_t> Threads;
    vector<pid_t> Processes; // pid_t for process IDs

    for (CommandLine elem : alldata) {

        if (elem.command == "wait") {
            for (pthread_t thread : Threads) {
                pthread_join(thread, NULL);
            }
            for (pid_t process : Processes) {
                int status;
                waitpid(process, &status, 0);
            }
        }

        else if (elem.redirection == ">") {
            int rc = fork();

            if (rc == 0) {
                // Child Process
                close(STDOUT_FILENO);
                int a = open(elem.filename.c_str(), O_CREAT | O_WRONLY, 0644);

                char* myargs[elem.numarg + 1];
                myargs[0] = strdup(elem.command.c_str());
                if (elem.input != "") {
                    myargs[1] = strdup(elem.input.c_str());
                    if (elem.option != "") {
                        myargs[2] = strdup(elem.option.c_str());
                    }
                }
                else if (elem.option != "") {
                    myargs[1] = strdup(elem.option.c_str());
                }
                myargs[elem.numarg] = NULL;
                execvp(myargs[0], myargs);
                fflush(stdout);
                exit(1);
            }
            // Parent Process
            if (elem.backgroundjob == "n") {
                int status;
                waitpid(rc, &status, 0);
            } else {
                Processes.push_back(rc);
            }
        }

        else {
            int *fd = (int*)malloc(sizeof(int)*2);
            pipe(fd);
            int rc = fork();

            if (rc == 0) {
                // Child Process
                if (elem.redirection == "<") {
                    close(STDIN_FILENO);
                    int a = open(elem.filename.c_str(), O_RDONLY);
                }
                dup2(fd[1], STDOUT_FILENO); // Parent will read from fd[0]

                char* myargs[elem.numarg + 1]; // +1 -> last element NULL
                myargs[0] = strdup(elem.command.c_str());
                if (elem.input != "") {
                    myargs[1] = strdup(elem.input.c_str());
                    if (elem.option != "") {
                        myargs[2] = strdup(elem.option.c_str());
                    }
                }
                else if (elem.option != "") {
                    myargs[1] = strdup(elem.option.c_str());
                }
                myargs[elem.numarg] = NULL;
                execvp(myargs[0], myargs);
                exit(1);
            }
            // Parent Process
            close(fd[1]); // Close unused write end in the parent
            pthread_t thread1;
            pthread_create(&thread1, NULL, thread_print, &fd[0]);
            Threads.push_back(thread1);

            if (elem.backgroundjob == "n") {
                int status;
                waitpid(rc, &status, 0);
            } else {
                Processes.push_back(rc);
            }
        }
    }

    // Wait for remaining threads and processes outside the loop
    for (pthread_t thread : Threads) {
        pthread_join(thread, NULL);
    }

    for (pid_t process : Processes) {
        int status;
        waitpid(process, &status, 0);
    }

    return 0;
}

