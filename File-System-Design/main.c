/**
 * 
 * 
 * Course: SFWRENG 3SH3, Winter 2026 C01
 * Assignment 4: File Block Allocation Simulator
 * 
 * File Name: main.c
 * Purpose:  (driver program to test your implementation)
 * 
 * Implement the Indexed Allocation Scheme for a simulated flat file system using C. This scheme uses an index block to store pointers to data
blocks, enabling efficient random access
 * 
 * 
 * Run code: 
 * compile: < make >
 * run: < make run >
 * clean: < make clean >
 * 
 * or < gcc main.c fs_indexed.c -o fs_indexed && ./fs_indexed >
 *
 */

#include "fs_indexed.h"

int main() {

    // initialize file system
    if (initFS() != 0) {
        return -1; // exit with error (error msg printed in initFS function in fs_indexed.c)
    }

    // create alpha and beta files
    createFile("alpha.txt", 3072);
    createFile("beta.txt", 5120);

    // display output
    printf("\n");
    listFiles();
    printf("\n");
    printFreeBlocks();

    // clean up alpha
    deleteFile("alpha.txt");

    // display output
    printf("\n");
    listFiles();
    printf("\n");
    printFreeBlocks();

    // create files
    createFile("gamma.txt", 4096);
    createFile("delta.txt", 8192);

    // display output
    printf("\n");
    listFiles();
    printf("\n");
    printFreeBlocks();

    // clean up
    deleteFile("beta.txt");
    deleteFile("gamma.txt");
    deleteFile("delta.txt");

    // display output
    printf("\n");
    listFiles();
    printf("\n");
    printFreeBlocks();

    return 0;
}
