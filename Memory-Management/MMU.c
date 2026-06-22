/**
 * 
 * Course: SFWRENG 3SH3, Winter 2026 C01
 * Assignment 3: Memory Management in OS
 * 
 * File Name: MMU.c
 * Purpose: To simulate a Memory Management Unit (MMU) which translates logical 
 * addresses into physical addresses using a page table and Translation Lookaside 
 * Buffer (TLB), with a FIFO page replacement policy. 
 * 
 * 
 * Run code: < gcc MMU.c -o MMU && ./MMU >
 *
 */

// include header files
#include <stdio.h>
#include <string.h> //For memcpy function
#include <unistd.h> 
#include <sys/mman.h> //For mmap() function
#include <fcntl.h> //For file descriptors
#include <stdlib.h> //For file descriptors

/*
LAS = 2^16 = 65536
PAS = 2^15 = 32768
*/

#define TLB_SIZE 16
#define PAGE_SIZE 256  // page size = 256 bytes = 2^8 
#define OFFSET_BITS 8  // number of bits for offset = 8 bits (since page size is 256 bytes)
#define OFFSET_MASK 0xFF  // mask for offset (2^8 - 1 => 0xFF)
#define NUM_PAGES 256  // number of pages = 2^16/2^8 = 2^8 = 256
#define NUM_FRAMES 128  // number of frames = 2^15/2^8 = 2^7 = 128
#define BUFFER_SIZE 10

// function prototypes
int search_TLB(int page_num);
void TLB_Add(int page_num, int frame_num);
void TLB_Update(int old_page, int new_page, int frame_num);

// create struct for TLB
struct TLBentry {
    int page_num;
    int frame_num;
};

// global variables
struct TLBentry *TLB[TLB_SIZE]; // TLB with 16 entries
int page_table[NUM_PAGES]; // page table with 256 entries
int tlb_index = 0; // index for TLB replacement

// physical memory (FIFO)
signed char physical_memory[NUM_FRAMES][PAGE_SIZE];
int frame_queue[NUM_FRAMES]; // maps each frame to the page stored in it
int next_frame = 0;
int total_frames_used = 0;

// statistics
int tlb_hits = 0;
int page_faults = 0;
int total_addresses = 0;

// search the TLB for an entry corresponding to a page number
int search_TLB(int page_num) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (TLB[i]->page_num == page_num) {
            return TLB[i]->frame_num; // TLB hit
        }
    }
    return -1; // TLB miss
}

// add an entry to the TLB using FIFO policy
void TLB_Add(int page_num, int frame_num) {
    TLB[tlb_index]->page_num = page_num; // add page number to TLB
    TLB[tlb_index]->frame_num = frame_num; // add frame number to TLB
    tlb_index = (tlb_index + 1) % TLB_SIZE; // update index (circular) -> if TLB is full, replace oldest entry
}

// // update the TLB by replacing old_page with new_page in the same slot
void TLB_Update(int old_page, int new_page, int frame_num) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (TLB[i]->page_num == old_page) {
            TLB[i]->page_num = new_page;
            TLB[i]->frame_num = frame_num; // update frame number in TLB
            return; 
        }
    }
}

int main() {

    // open file
    FILE *fptr = fopen("addresses.txt", "r");

    // open backing store
    int backing_store_fd = open("BACKING_STORE.bin", O_RDONLY);

    // mmap backing store
    signed char *backing_store = mmap(
        0,
        NUM_PAGES * PAGE_SIZE,
        PROT_READ,
        MAP_PRIVATE,
        backing_store_fd,
        0
    );

    // initialize TLB
    for (int i = 0; i < TLB_SIZE; i++) {
        TLB[i] = (struct TLBentry*) malloc(sizeof(struct TLBentry));
        TLB[i]->page_num = -1; // initialize to not in memory
        TLB[i]->frame_num = -1; // initialize to not in memory
    }

    // initialize page table
    for (int i = 0; i < NUM_PAGES; i++) {
        page_table[i] = -1; // initialize to not in memory
    }

    char buff[BUFFER_SIZE];

    // read file line by line
    while (fgets(buff, BUFFER_SIZE, fptr) != NULL) {
        int LA = atoi(buff);// logical address
        total_addresses++;

        // MMU – Address Translation: compute page number and offset using bitwise operators
        int page_num = LA >> OFFSET_BITS; // extract page number (upper 8 bits)
        int page_offset = LA & OFFSET_MASK; // extract offset (lower 8 bits)

        // MMU – look up page number from the logical address in TLB
        int frame_num = search_TLB(page_num); // TLB hit: get frame number from TLB

        // TLB hit
        if (frame_num != -1) {
            tlb_hits++;
        } 
        else {// TLB miss: check page table for page number to get frame number
            // check page table if TLB miss 
            frame_num = page_table[page_num];

            // PAGE FAULT
            if (frame_num == -1) {
                page_faults++; 
 
                // free frame available 
                if (total_frames_used < NUM_FRAMES) { 
                    frame_num = next_frame; 
                    total_frames_used++; 
                }  
                else { 
                    // FIFO replacement 
                    frame_num = next_frame; 

                    int old_page = frame_queue[frame_num]; 

                    page_table[old_page] = -1;

                    // replace old page entry in TLB with new page in same slot
                    TLB_Update(old_page, page_num, frame_num);
                }

                // load page from backing store
                memcpy(
                    physical_memory[frame_num],
                    backing_store + (page_num * PAGE_SIZE),
                    PAGE_SIZE
                );

                // update structures
                page_table[page_num] = frame_num;
                frame_queue[frame_num] = page_num;

                // only add if it was NOT replaced in TLB
                int found = 0;
                for (int i = 0; i < TLB_SIZE; i++) {
                    if (TLB[i]->page_num == page_num) {
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    TLB_Add(page_num, frame_num);
                }

                next_frame = (next_frame + 1) % NUM_FRAMES; 
            } 
            else {
                // page table hit -> update TLB
                TLB_Add(page_num, frame_num);
            }
        }

        int PA = (frame_num << OFFSET_BITS) | page_offset; // compute physical address (frame * page_size + offset)

        signed char value = physical_memory[frame_num][page_offset];

        printf("Virtual address: %d Physical address = %d Value= %d\n",
               LA, PA, value);
    }

    // print statistics
    printf("Total addresses = %d\n", total_addresses);
    printf("Page_faults = %d\n", page_faults);
    printf("TLB Hits = %d\n", tlb_hits);

    // cleanup
    for (int i = 0; i < TLB_SIZE; i++) {
        free(TLB[i]);
    }

    munmap(backing_store, NUM_PAGES * PAGE_SIZE);
    close(backing_store_fd);
    fclose(fptr);

    return 0;
}
