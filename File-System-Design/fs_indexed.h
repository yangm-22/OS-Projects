/**
 * 
 * 
 * Course: SFWRENG 3SH3, Winter 2026 C01
 * Assignment 4: File Block Allocation Simulator
 * 
 * File Name: fs_indexed.h
 * Purpose: header file with structure definitions and function prototypes to implement the Indexed Allocation Scheme for a simulated flat file system using C. This scheme uses an index block to store pointers to data
blocks, enabling efficient random access
 * 
 * Run code: N/A (this is a header file, not meant to be run on its own)
 *
 */

#ifndef LAB4_H
#define LAB4_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// constants
#define BLOCK_SIZE 1024 // 1KB
#define TOTAL_BLOCKS 64
#define MAX_FILES 10

// disk block
typedef struct Block {
    char data[BLOCK_SIZE];
    int block_num;
} Block;

// File Information Block
typedef struct FIB {
    int FIB_ID;
    char filename[20];
    int file_size;
    int block_count;
    int index_block;
} FIB;

// linked list node for free blocks
typedef struct Node {
    int block_num;
    struct Node* next;
} Node;

// Volume Control Block
typedef struct VCB {
    int total_blocks;
    int free_blocks;
    int block_size;
    Node* free_block_list;
    Block disk[TOTAL_BLOCKS];
    FIB file_table[MAX_FILES];
    int num_files;
    int occupied[MAX_FILES];
    int next_FIB_ID;
} VCB;

extern VCB vcb;

// operation function prototypes
int initFS();
int createFile(const char* filename, int size);
int deleteFile(const char* filename);
void listFiles();

// helper function prototypes
void returnFreeBlock(int block_num);
int allocateFreeBlock();
int getFileInformationBlockID();
void printFreeBlocks();

#endif
