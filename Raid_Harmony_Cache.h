#include <stdio.h>
#include <stdlib.h>
//#include<conio.h>
#include<string.h>
#include "filetable.h"
#define BLOCK_SIZE 4096
#define BLOCK_CACHE_SIZE 5000
#define No_of_disk 5
#define CACHE_SIZE 100
#define SPURT_SIZE 2//
//WHAT IS MAX_CACHE_SIZE
#define MAX_CACHE_SIZE	5000
#define WRITE_BACK_PERCENT 0.5
#define TRACE_FILE "TraceBlocksDA.csv"
#define THRESHOLD 0.5
#define Del_Threshold 0.1
#define MAX_ACCESS 56335

typedef struct dataBuffer
{
	char buff[BLOCK_SIZE];
}dataBuffer;

typedef struct block
{
	int tag;
	char buffer[BLOCK_SIZE];
}block;

typedef struct blk
{
	int modify;
	int used;
	block *bptr;
}blk;

typedef struct cacheStripe
{
	int sno;
	int dirty;
	blk block_array[No_of_disk];
	struct cacheStripe *prev;
	struct cacheStripe *next;
}cacheStripe;

typedef struct cacheQueue{
    int sno;
    struct cacheQueue *next;
    struct cacheQueue *prev;
}cacheQueue;

typedef struct cacheQueueParam{
    cacheQueue *front;
    cacheQueue *tail;
    int size; 
}cacheQueueParam;

int tag[BLOCK_CACHE_SIZE];
block block_cache[BLOCK_CACHE_SIZE];
cacheQueueParam *qp;//qp->size=0;
cacheStripe *hashTable[CACHE_SIZE];
char writeData[BLOCK_SIZE];
int policy;
int paritySelection=0;
int cacheMiss=0;
int cacheHit=0;
int modifiedStripe=0;
int usedBlock=0;
int unnecessaryWrite=0;
int modThreshold;
int noiop;
dataBuffer db;
dataBuffer db_array[No_of_disk];

void initiatizeBlockCache();
void initializeQueueParam();
void initializeHashTable();
void cacheReadRequest(int stripeno,int blockno);
void placing(int stripeno, int blockno);
void cacheQueueUpdate(int stripeno);
void hashUpdate(int stripeno,int blockno);
int getBlockCacheIndex(int k);
int getBlkCacheIndex();
int searchCache(int stripeno, int blockno);
int searchStripe(int stripeno);
void updateCache(int stripeno);
void updateBlockRead(int stripeno,int blockno);
void updateBlockWrite(int stripeno,int blockno);
void denqueue();
void usedUpdateAtHash(int stripeno,int blockno);
void deleteClean();
int delligibleStripe(int stripeno);
void deleteLRUCleanElement(int sno);
void restoreBlocks(cacheStripe *s);
void cacheWriteRequest(int stripeno, int blockno,char writedata[]);
void placingwrite(int stripeno, int blockno);
void hashUpdateWrite(int stripeno, int blockno);
void hashUpdateWriteHit(int stripeno, int blockno);
void writeBackPolicy();
void readIn(int);
void readInBlock(int stripeno,int blockno);





