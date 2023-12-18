#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <cstdlib>
#include <pthread.h>
#include <semaphore.h>

using namespace std;

pthread_barrier_t barrier;
pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER; //After selecting 4 passangers process should be blocked untill the previous valid combination is printed completely
sem_t semA, semB;
int waitingA = 0; // Number of people waiting for car in team A
int waitingB = 0; // Number of people waiting for car in team B
int CarID = 0;
int PassangerCount = 0; // For detecting the captain, this will be divided by 4

void* thread_function(void* param) {
    string TeamName = *(string*)param;
    bool isCaptain = false;

    cout << "Thread ID: " << pthread_self() << ", Team: " << TeamName << ", I am looking for a car" << endl;
    pthread_mutex_lock(&lock1);
	
    if (TeamName == "A") {
        waitingA++;
    } else if (TeamName == "B") {
        waitingB++;
    }

    //Wake 4 people waiting for car with a valid combination
    if (waitingA >= 2 && waitingB >= 2) {
    	pthread_mutex_lock(&lock2); //Wait the previous 4 to wake new 4
        sem_post(&semA);
        sem_post(&semA);
        sem_post(&semB);
        sem_post(&semB);
        waitingA -= 2;
        waitingB -= 2;
    } else if (waitingA >= 4) {
    	pthread_mutex_lock(&lock2); //Wait the previous 4 to wake new 4
        sem_post(&semA);
        sem_post(&semA);
        sem_post(&semA);
        sem_post(&semA);
        waitingA -= 4;
    } else if (waitingB >= 4) {
    	pthread_mutex_lock(&lock2); //Wait the previous 4 to wake new 4
        sem_post(&semB);
        sem_post(&semB);
        sem_post(&semB);
        sem_post(&semB);
        waitingB -= 4;
    }
    //Everybody waits according to their team	
    pthread_mutex_unlock(&lock1);

    if (TeamName == "A") {
        sem_wait(&semA);
    } else if (TeamName == "B") {
        sem_wait(&semB);
    }
    //Passangers with valid comination get their seat
    pthread_barrier_wait(&barrier);
    pthread_mutex_lock(&lock1);
    PassangerCount++;
    cout << "Thread ID: " << pthread_self() << ", Team: " << TeamName << ", I have found a spot in a car" << endl;
    //There must be exactly 1 driver for 4 passanger (driver is also a passanger) 
    if (PassangerCount % 4 == 0) {
        cout << "Thread ID: " << pthread_self() << ", Team: " << TeamName
             << ", I am the captain and driving the car with ID " << CarID << endl;
        CarID++;
        pthread_mutex_unlock(&lock2); //Process is completed we can wake new 4
    }

    pthread_mutex_unlock(&lock1);
    return NULL;
}

int main(int argc, char* argv[]) {
    int NumFansTeamA = atoi(argv[1]);
    int NumFansTeamB = atoi(argv[2]);
    int NumFansTotal = NumFansTeamA + NumFansTeamB;

    // Check validity of the arguments
    if (NumFansTeamA % 2 != 0 || NumFansTeamB % 2 != 0 || NumFansTotal % 4 != 0) {
        cout << "The main terminates";
        return 1;
    }

    string teamA = "A";
    string teamB = "B";
    vector<pthread_t> Threads;
    pthread_barrier_init(&barrier, NULL, 4);

    for (int i = 0; i < NumFansTotal; i++) {
        pthread_t thread1;
        if (i < NumFansTeamA) {
            pthread_create(&thread1, NULL, thread_function, &teamA);
        } else {
            pthread_create(&thread1, NULL, thread_function, &teamB);
        }
        Threads.push_back(thread1); //Store thread IDs in the vector for later use
    }

    // Wait for threads to finish
    for (pthread_t threadID : Threads) {
        pthread_join(threadID, NULL);
    }
    cout << "The main terminates";
    return 0;
}

