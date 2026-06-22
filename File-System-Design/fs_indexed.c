/**
 * 
 * Course: SFWRENG 3SH3, Winter 2026 C01
 * Assignment 4: File Block Allocation Simulator
 * 
 * File Name: fs_indexed.c
 * Purpose: (implementation of file system functions)
 * 
 * implement the Indexed Allocation Scheme for a simulated flat file system using C. This scheme uses an index block to store pointers to data
blocks, enabling efficient random access
 * 
 * 
 *
 * Run code: < gcc main.c fs_indexed.c -o fs_indexed && ./fs_indexed >
 */

#include "fs_indexed.h"

// global variable for the volume control block
VCB vcb;

// initialize the file system
int initFS(){
    vcb.total_blocks = TOTAL_BLOCKS;
    vcb.free_blocks = TOTAL_BLOCKS;
    vcb.block_size = BLOCK_SIZE;
    vcb.free_block_list = NULL;
    vcb.num_files = 0;

    // mark all file slots as unoccupied
    for (int i = 0; i < MAX_FILES; i++) {
        vcb.occupied[i] = 0;
    }
    vcb.next_FIB_ID = 0;

    // build free block list in ASCENDING order: 0 -> 1 -> ... -> 63
    Node* tail = NULL;

    for (int i = 0; i < TOTAL_BLOCKS; i++) {
        Node* new_node = (Node*)malloc(sizeof(Node));
        // fails to allocate memory for the new node => clean up and exit
        if (new_node == NULL) {
            // clean up => free all previously allocated nodes
            Node* current = vcb.free_block_list;
            while (current != NULL) {
                Node* temp = current;
                current = current->next;
                free(temp);
            }
            // reset and exit with error message
            vcb.free_block_list = NULL;
            printf("[ERROR] Failed to initialize the volume control block.\n");
            return -1;
        }
        new_node->block_num = i;
        new_node->next = NULL;

        if (vcb.free_block_list == NULL) {
            // first node
            vcb.free_block_list = new_node;
            tail = new_node;
        } else {
            // append to tail
            tail->next = new_node;
            tail = new_node;
        }
    }

    printf("Filesystem initialized with %d blocks of %d bytes each.\n",
           vcb.total_blocks, vcb.block_size);
    return 0;
}

// creates a file in file system using index allocation scheme
int createFile(const char* filename, int size){
    int blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // check if file can be created (# of files does not exceed max)
    if (vcb.num_files >= MAX_FILES || size < 0 || (blocks_needed + 1) > vcb.free_blocks) {
        printf("[ERROR] Failed to create file '%s' of size %d bytes. Not enough space or max file limit reached.\n", filename, size);
        return -1;
    }

    // find free FIB slot
    int fib_index = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (vcb.occupied[i] == 0) {
            fib_index = i;
            break;
        }
    }

    if (fib_index == -1){
        printf("[ERROR] Failed to create file '%s'. No free FIB slot available.\n", filename);
        return -1;
    } 

    // allocate index block
    int index_block = allocateFreeBlock();
    if (index_block == -1) {
        printf("[ERROR] Failed to create file '%s'. No free blocks available for index block.\n", filename);
        return -1;
    }

    // allocate data blocks
    int data_blocks[blocks_needed];
    for (int i = 0; i < blocks_needed; i++) {
        int b = allocateFreeBlock();
        if (b == -1) {
            printf("[ERROR] Failed to create file '%s'. Not enough free blocks available for data blocks.\n", filename);
            return -1;
        }
        data_blocks[i] = b;
    }

    // store data block numbers inside index block
    int* index_data = (int*) vcb.disk[index_block].data;
    for (int i = 0; i < blocks_needed; i++) {
        index_data[i] = data_blocks[i];
    }

    // fill FIB
    vcb.file_table[fib_index].FIB_ID = getFileInformationBlockID(); // helper function to get FIB ID
    strcpy(vcb.file_table[fib_index].filename, filename);
    vcb.file_table[fib_index].file_size = size;
    vcb.file_table[fib_index].block_count = blocks_needed;
    vcb.file_table[fib_index].index_block = index_block;

    vcb.occupied[fib_index] = 1;
    vcb.num_files++;
    printf("File '%s' created with %d data blocks + 1 index block.\n", filename, blocks_needed);

    return 0;
}

// deletes a file with the given file name
int deleteFile(const char* filename){
    int fib_index = -1;

    // find file
    for (int i = 0; i < MAX_FILES; i++) {
        if (vcb.occupied[i] && strcmp(vcb.file_table[i].filename, filename) == 0) {
            fib_index = i;
            break;
        }
    }

    // file not found
    if (fib_index == -1){
        printf("[ERROR] Failed to delete file '%s'. File not found.\n", filename);
        return -1;
    }

    int index_block = vcb.file_table[fib_index].index_block;
    int block_count = vcb.file_table[fib_index].block_count;

    // get pointer to index block data
    int* index_data = (int*) vcb.disk[index_block].data;

    // free data blocks
    for (int i = 0; i < block_count; i++) {
        returnFreeBlock(index_data[i]);
    }

    // free index block
    returnFreeBlock(index_block);

    // clear FIB
    vcb.occupied[fib_index] = 0;
    vcb.num_files--;

    printf("File '%s' deleted.\n", filename);

    return 0;
}

// lists all the files in the flat file system
void listFiles() {
    printf("Root Directory Listing (%d files): \n", vcb.num_files);

    // Temporary array to store pointers to occupied FIBs
    FIB* files[MAX_FILES];
    int count = 0;

    for (int i = 0; i < MAX_FILES; i++) {
        if (vcb.occupied[i]) {
            files[count++] = &vcb.file_table[i];
        }
    }

    // sort files by FIB_ID to ensure consistent listing order
    for (int i = 0; i < count-1; i++) {
        for (int j = 0; j < count-1-i; j++) {
            if (files[j]->FIB_ID > files[j+1]->FIB_ID) {
                FIB* temp = files[j];
                files[j] = files[j+1];
                files[j+1] = temp;
            }
        }
    }

    // Print in sorted order
    for (int i = 0; i < count; i++) {
        printf("  %-10s |   %d bytes |  %d data blocks | FIBID=%d\n",
               files[i]->filename,
               files[i]->file_size,
               files[i]->block_count,
               files[i]->FIB_ID);
    }
}

// adds a freed block to the tail of the free block list
void returnFreeBlock(int block_num){
    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->block_num = block_num;
    new_node->next = NULL;

    // if list is empty
    if (vcb.free_block_list == NULL) {
        vcb.free_block_list = new_node;
    } else {
        // traverse to tail
        Node* curr = vcb.free_block_list;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = new_node;
    }

    vcb.free_blocks++;
}

// removes a free block from the head of the free block list and returns it
int allocateFreeBlock(){
    if (vcb.free_block_list == NULL) {
        return -1; // all blocks occupied
    }
    Node* temp = vcb.free_block_list;
    int block_num = temp->block_num; 
    vcb.free_block_list = temp->next; // progress to next free block

    free(temp); // free the allocated node
    vcb.free_blocks--; // decrease free block count
    return block_num;
}

// allocation FIB ID to new file
int getFileInformationBlockID(){
    return vcb.next_FIB_ID++;
}

// displays all free block numbers and the total count of free block
void printFreeBlocks(){
    printf("Free Blocks (%d): ", vcb.free_blocks);
    Node* curr = vcb.free_block_list;
    while (curr != NULL) {
        printf("[%d] -> ", curr->block_num);
        curr = curr->next;
    }
    printf("NULL\n");
}

