#ifndef FILETABLE_H
#define FILETABLE_H
#pragma once
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
//#include<assert.h>
#define DEVICE_NUMBER	5
#define BLOCK_SIZE	4096  //define in fsc
#define DISK_SIZE	20000 //change to 8GB or 2000 000 blocks in disk
#define FILE_DATA	"FileSizesDA.csv"
#define File_number	500

//#include "FSC.h"
struct disk{
	int disk_num;
	int current_offset;

};

typedef struct arg_struct{
    int  sno;
    int i;

}arg_struct;
/*
typedef struct ioreq_event{
	int flags;
	int blkno;
	int fileid;

}ioreq_event;
*/
typedef struct arg_struct_read{
	int  sno;
	int i;
	char *buffer;

}arg_struct_read;
struct file_entry{
	int fileid;
	int filesize;
	int start_disk;
	int start_offset;
};
extern char file_table_writeData[BLOCK_SIZE];
extern pthread_mutex_t mutex;
extern int total_files;
void raid_create(int device_number);
int is_parity(int disk_num,int stripe_number);
void ini_file_table();
int total_file_num(FILE *file_data);
int file_table_create();
int allocate_disk_filetable();
//file_entry * find(int fileid);
int requestStripeNm(int file_name,int block_num);
void write_back(int stripenum,char *p,int i);
file_entry * find(int fileid);
void initial_ptr();
void initial_block();
void initial_file_entry();
int getblocknum(int fileid,int block_num);
//void create_full_stripe_read_threads(cacheStripe newStripe1);
void *threadWork(void *arg);
void *readInThread(void *argument);
void writeBack(int sno);
int getParitybnum(int stripe_num);
#endif
