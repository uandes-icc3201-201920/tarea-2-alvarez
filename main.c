/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int page_fault_counter = 0;
int pager_swaps = 0;
int read_counter = 0;
int write_counter = 0;

int *page_counter = 0;
int *frame_counter = 0;

char *run_method;

char *virtmem;
char *physmem;

int *fifo_entries = NULL;
int first_in = 0;

int *frame_table = NULL;
int *frame_table_bit = NULL;

struct disk *disk;


void RAND_method(struct page_table *page_table, int page, int frame, int bits){
	srand(0); 
	int random_frame = rand() % frame_counter;
	int frame_available	= -1;
	for (int i = 0;i <= frame_counter; i++){/
		if (frame_table_bit[i] == 0){
			frame_available = i;
			break;
		}
	}
	if (frame_available == -1){
		bits = PROT_READ;
		
		write_counter++;
		disk_write(disk, frame_table[random_frame], &physmem[random_frame*PAGE_SIZE]);
		
		page_table_set_entry(page_table, frame_table[random_frame], random_frame, 0); 
		
		read_counter++;
		disk_read(disk, page, &physmem[random_frame*PAGE_SIZE]);
		
		frame_table[random_frame] = page;
		frame_table_bit[random_frame] = bits;

		page_table_set_entry(page_table, page, random_frame, bits);
	
	}
	else{
		read_counter++;
		bits = PROT_READ;

		disk_read(disk, page, &physmem[frame_available*PAGE_SIZE]);
		frame_table[frame] = page;
		frame_table_bit[frame] = bits;

		page_table_set_entry(page_table, page, frame_available, bits);
		
	}
}


void FIFO_method(struct page_table *page_table, int page, int frame, int bits){
	int frame_available	= -1;
	for (int i = 0;i <= frame_counter; i++){/
		if (frame_table_bit[i] == 0){
			frame_available = i;
			break;
		}
	}
	if (frame_available == -1){
		write_counter++;
		disk_write(disk, frame_table[first_in], &physmem[first_in*PAGE_SIZE]);
		
		page_table_set_entry(page_table, frame_table[first_in], first_in, 0);

		read_counter++;
		disk_read(disk, page, &physmem[first_in*PAGE_SIZE]);
		frame_table[frame] = page;
		frame_table_bit[frame] = bits;

		page_table_set_entry(page_table, page, first_in, bits);

		if(first_in == frame_counter){
			first_in = 0;
		}
		else{
			first_in++;
		}
	}
	else{
		read_counter++;
		bits = PROT_READ;

		disk_read(disk, page, &physmem[frame_available*PAGE_SIZE]);
		frame_table[frame] = page;
		frame_table_bit[frame] = bits;

		page_table_set_entry(page_table, page, frame_available, bits);
		
	}
}



void page_fault_handler( struct page_table *pt, int page )
{
	printf("page fault on page #%d\n",page);

	page_fault_counter++;
	int frame,bits;
	page_table_get_entry(pt, page, &frame, &bits);

	if(bits == 0){
		page_fault_counter++;
	}
	
	if (!strcmp(run_method,"fifo")){
		FIFO_method(pt,page,frame,bits);
	}
	else if (!strcmp(run_method,"rand")){
		RAND_method(pt,page,frame,bits);
	}
	else{
		printf("Policy not found.");
		exit(1);
	}
}

int main( int argc, char *argv[] )
{
	if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <lru|fifo> <access pattern>\n");
		return 1;
	}

	int npages = atoi(argv[1]);
	int nframes = atoi(argv[2]);
	const char *program = argv[4];

	struct disk *disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}


	struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}

	char *virtmem = page_table_get_virtmem(pt);

	char *physmem = page_table_get_physmem(pt);

	if(!strcmp(program,"pattern1")) {
		access_pattern1(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"pattern2")) {
		access_pattern2(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"pattern3")) {
		access_pattern3(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[3]);

	}

	page_table_delete(pt);
	disk_close(disk);

	return 0;
}
