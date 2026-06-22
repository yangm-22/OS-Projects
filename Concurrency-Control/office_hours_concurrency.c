/**
 * 
 * Course: SFWRENG 3SH3, Winter 2026 C01
 * Assignment 2: The Sleeping Teaching Assistant
 * 
 * File Name: office_hours_concurrency.c
 * Purpose: Using POSIX threads, mutex locks, and semaphores, implemented a solution that 
coordinates the activities of the TA and the students
 * 
 * 
 * Run code: < gcc office_hours_concurrency.c -o ta -pthread && ./ta >
 *
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <stdbool.h>
#include <unistd.h>

// define constants
#define CHAIRS 3
#define NUM_STUDENTS 5 // change to test
#define MAX_VISITS 2 // max num of visits per student to avoid running indefinitely 

// function prototypes
void *TA(void *param); 
void *students(void *param); 
void rand_sleep(int num);

// global variables
int waiting_students = 0; // number of students waiting for TA
pthread_mutex_t mutex; // to protect waiting_students variable
sem_t ta_sem; // to call student into office 
sem_t student_sem; // to wake up TA when student arrives/waiting
sem_t done; // after TA done helping student
bool ta_sleeping = true; //keep track if the ta is sleeping

// generate random sleep time for students and TA
void rand_sleep(int num) {
    int time = rand() % num + 1;
    sleep(time);
}

void *TA(void *param){
    while (true){
        pthread_mutex_lock(&mutex);
        // Check if TA should sleep
        if (waiting_students == 0){
            ta_sleeping = true;
            printf("\nTA is sleeping...");
        }
        pthread_mutex_unlock(&mutex);

        sem_wait(&student_sem); // TA sleeps while waiting for students

        pthread_mutex_lock(&mutex); // acquire mutex lock()
        ta_sleeping = false; 
        
        if (waiting_students > 0){
            waiting_students--; // one student is being helped
            printf("\nTA helping student. Waiting students = %d ", waiting_students);
            sem_post(&ta_sem); // calls in waiting student to office  

            pthread_mutex_unlock(&mutex); // release mutex lock

            rand_sleep(3); // helping student...
            printf("\nTA DONE helping student. Waiting students = %d ", waiting_students);
            sem_post(&done); // signal that TA is done helping student
        }
        else {
            pthread_mutex_unlock(&mutex); // release mutex lock
        }
    }
}

void *students(void *param){
    int student_id = *(int*)param;
    int visits = 0; // track number of times student has been helped by TA

    while (visits < MAX_VISITS){
        printf("\nStudent %d is doing assignments...", student_id);
        rand_sleep(4); // student doing programming...

        pthread_mutex_lock(&mutex); // acquire mutex
        printf("\nStudent %d showed up for office hours.", student_id);

        // check if chairs available
        if (waiting_students < CHAIRS){
            
            waiting_students++;
            printf("\nStudent %d sits in a chair in the hallway. Waiting students = %d ", student_id, waiting_students);
            
            //check if ta is sleeping
            if (ta_sleeping){
                printf("\nStudent %d wakes up the TA!", student_id);
                ta_sleeping = false;
            }

            sem_post(&student_sem); // wake up TA bc student needs help
            pthread_mutex_unlock(&mutex); // release mutex
            
            sem_wait(&ta_sem); // wait until TA free
            printf("\nStudent %d being helped...", student_id);
            
            sem_wait(&done); // wait for TA to finish helping
            printf("\nStudent %d DONE being helped.", student_id); 
            visits++;
        }
        else{
            printf("\nStudent %d leaves (no chairs available). Waiting students = %d ", student_id, waiting_students);
            pthread_mutex_unlock(&mutex); // release mutex
        }
    }
    printf("\nStudent %d leaving office hours.", student_id);
    pthread_exit(NULL);
}

// main
int main(int argc, char *argv[]){
    // track student ids
    int student_ids[NUM_STUDENTS];

    // initialize threads
    pthread_t student_threads[NUM_STUDENTS];
    pthread_t ta_thread;
    pthread_attr_t attr; // set thread attributes
    pthread_attr_init(&attr); // set default attributes of threads

    // initialize semaphores
    sem_init(&ta_sem, 0, 0); // TA starts sleeping
    sem_init(&student_sem, 0, 0); // no students waiting at the beginning
    sem_init(&done, 0, 0); // TA is not done helping

    // initialize mutex
    pthread_mutex_init(&mutex, NULL);

    printf("--- Office Hours Started ---\n");

    // create threads for TA 
    if (pthread_create(&ta_thread, &attr, TA, NULL) != 0){
        printf("\nError in creating thread for TA.");
        return -1;
    }

    // create threads for students
    for (int i=0 ; i<NUM_STUDENTS ; i++){
        student_ids[i] = i+1; // track student ids
        if (pthread_create(&student_threads[i], &attr, students, &student_ids[i]) != 0){
            printf("\nError in creating thread for students.");
            return -1;
        }
    }

    // join threads and wait for all to finish
    for (int i=0 ; i<NUM_STUDENTS ; i++){
        pthread_join(student_threads[i], NULL);
    }

    // destroy threads, mutex, and semaphores to exit 
    pthread_cancel(ta_thread);
    printf("\n--- Office Hours Ended ---\n");
    pthread_mutex_destroy(&mutex);
    sem_destroy(&student_sem);
    sem_destroy(&ta_sem);
    sem_destroy(&done);
    
    return 0;
}
