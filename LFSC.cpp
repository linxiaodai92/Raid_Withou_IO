#include <stdio.h>
#include <stdlib.h>
#include <math.h>               
#include <assert.h>  
#include<string.h>
#include <pthread.h>
#include "LFSC.h"
#include "filetable.h"
#include <semaphore.h>
pthread_mutex_t mutex_lfsc;
pthread_mutex_t mutex_writeback = {1};
int threadcount = 0;
pthread_cond_t condc, condp;
sem_t slot,item;
int main()
{
	int blockno, stripeno, operation;
	initiatizeBlockCache();
	initializeQueueParam();
	initializeHashTable();
	initializeJobQueueParam();
	initializeWriteBackQueueParam();
	sem_init(&slot,0,1);
	sem_init(&item,0,1);
//	pthread_mutex_init(&mutex_writeback,NULL);	
	/****** Creation of files in Disks ****************/
	char line[200];
	printf("\n1. Stripe Level\n2. Block Level ");
	scanf("%d",&policy);
	if(policy==1)
	{
		printf("\n Choice of parity\n 1. Maintaining parity inside cache \n 2. Not maintaining parity inside cache ");
		scanf("%d",&paritySelection);
		for(int k=0;k<No_of_disk;k++)
		{
			memset(db_array[k].buff,'1',BLOCK_SIZE);
		}
	}
		
	if(policy==2)
	{
		memset(db.buff,'1',BLOCK_SIZE);
	}
	
	raid_create(No_of_disk);
	ini_file_table();
	file_table_create();
	allocate_disk_filetable();
	int fileid,offset;
	FILE *fp=fopen(TRACE_FILE,"r");
	unsigned int i=0;
	/*********** End of file creation on Disk ***************/
	modThreshold=CACHE_SIZE*THRESHOLD;//
		
	for(i=0;i<MAX_ACCESS;i++)
	{
		fscanf(fp,"%d,%d,%d\n",&operation,&offset,&fileid);
//		printf("\n**************************************************************************************");
//		printf("i: %d operation %d,offset %d fileid %d\n",i,operation,offset,fileid);
		stripeno=requestStripeNm(fileid,offset);
		blockno=getblocknum(fileid,offset);
		paritybno=getParitybnum(stripeno);
//		printf("stripeno= %d operation= %d blockno=%d \n",stripeno,operation,blockno);
		if(operation==0)
		{
			cacheReadRequest(stripeno,blockno);
		}
		else
		{
			memset(writeData,'1',BLOCK_SIZE);
			cacheWriteRequest(stripeno,blockno,writeData);	
			if (modifiedStripe>modThreshold)
			{
				writeBackPolicy();
			}
		}
	//	printf("\n The length of the cache queue is :%d",qp->size);
	//	printf("\nOk for %d",i);
	}	
	printf("\n The length of the cache queue is :%d",qp->size);
	printf("\n The number of cache hit is: %d",cacheHit);
	printf("\n The number of cache miss is: %d\n",(MAX_ACCESS-cacheHit));
	printf("\n The total number of I/o operation: %d",noiop);
	printf("\n The number of read I/O: %d",readiop);
	printf("\n The number of write I/O is: %d",writeiop);
	printf("\n The number of unnecessary writeback is : %d\n",unnecessaryWrite);
	//printf("\n The total number of IO operations: %d\n",noiop);
}

/********* Initialization BlockCache and Queue *************/
void initiatizeBlockCache()
{
	int i,j;
	for(i=0;i<BLOCK_CACHE_SIZE;i++)
	{
		//printf("\nCheck");
		block_cache[i].tag=0;
		memset(block_cache[i].buffer,'0',BLOCK_SIZE);
	}
}

void initializeQueueParam()
{
	qp=(cacheQueueParam *)malloc(sizeof(cacheQueueParam));
	qp->front=NULL;
	qp->tail=NULL;
	qp->size=0;
}

void initializeHashTable()
{
	int i;
	for(i=0;i<CACHE_SIZE;i++)
	{
		hashTable[i]=NULL;
	}
}

void initializeJobQueueParam()
{
	jq=(jobQueueParam *)malloc(sizeof(jobQueueParam));
	jq->front=NULL;
	jq->tail=NULL;
	jq->size=0;
}

void initializeWriteBackQueueParam()
{
	wq=(writeBackQueueParam *)malloc(sizeof(writeBackQueueParam));
	wq->front=NULL;
	wq->tail=NULL;
	wq->size=0;
}

/********************  Read Request ************************/
void cacheReadRequest(int stripeno,int blockno)
{
//	printf("\n cacheReadRequest");
	if(qp->front==NULL && qp->tail==NULL)
	{
		printf("\n compulsory miss");
		if(policy=1)
		{
			readIn(stripeno);
			readiop++;
			noiop++;
		}
		else
		{
			//readInBlock(stripeno,blockno);
			readiop++;
			noiop++;
		}
		//noiop++;
		//printf("\nIn Compulsory Miss");
		placing(stripeno,blockno);
		//cacheMiss++;
	}
	else
	{
		//hit
		if(searchCache(stripeno,blockno)==1)
		{
			printf("\n Hit");
			updateCache(stripeno);
			usedUpdateAtHash(stripeno,blockno);
			//cacheHit++;
		}
		else 
		//miss
		{
			if(io_complete==0)
			{
			//	printf("\n Miss");
			if(policy=1)
			{
				readIn(stripeno);
				readiop++;
				noiop++;
			}
			if(policy==2)
			{
				//readInBlock(stripeno,blockno);
				readiop++;
				noiop++;
			}	
			//readInBlock(stripeno,blockno);
			noiop++;
			/*Free space available in cache*/
			if(qp->size<CACHE_SIZE)
			{
			//	printf("\n Miss but cache is not full");
				if(policy==1)
					placing(stripeno,blockno);
				if(policy==2)
				{
					if(searchStripe(stripeno)!=0)
					{
						updateCache(stripeno);
						updateBlockRead(stripeno,blockno);
					}
					else
					{
						placing(stripeno,blockno);
					}
				}
			}
			/*Capacity Miss*/
			else
			{
			//	printf("\nIn Miss and space not available");
				if(policy==1)
				{
					deleteClean();
					placing(stripeno,blockno);
					//cacheMiss++;
				}
			
				if(policy==2)
				{
					if(searchStripe(stripeno)!=0)
					{
						updateCache(stripeno);
						updateBlockRead(stripeno,blockno);
					}
					else
					{
						deleteClean();
						placing(stripeno,blockno);
						//cacheMiss++;	
					}
				}
				
			
			}	
			}	
		}
		
	}
}

void placing(int stripeno, int blockno)
{
	cacheQueueUpdate(stripeno);
	hashUpdate(stripeno,blockno);
}

void cacheQueueUpdate(int stripeno)
{
//	printf("\nIn cacheQueueUpdate");
	/***************Initialize Cache Queue Node***************/
	cacheQueue *newNode= (cacheQueue *)malloc(sizeof(cacheQueue));
	newNode->sno=stripeno;
	newNode->next=NULL;
	newNode->prev=NULL;
	
	/************ Queue front and tail adjustment************/
	if(qp->front==qp->tail && qp->front==NULL)
	{
		qp->front=newNode;
		qp->tail = newNode;
	}
	else if(qp->front==qp->tail)
	{
		qp->front->next=newNode;
		qp->tail=newNode;
		newNode->prev=qp->front;
	}
	else
	{
		qp->tail->next=newNode;
		newNode->prev=qp->tail;
		qp->tail=newNode;
	}
	qp->size++;	
}

void hashUpdate(int stripeno, int blockno)
{
//	printf("\nIn hashUpdate");
	int hashIndex=stripeno%CACHE_SIZE;
	cacheStripe *newStripe=(cacheStripe *)malloc(sizeof(cacheStripe));
	if(newStripe!=NULL)
	{	
		newStripe->sno=stripeno;//Set stripeno
		newStripe->dirty=0;//Set dirtybit
		//if(paritySelection==1)
		//	memset(newStripe->partialparity,'0',BLOCK_SIZE);//Set partial Parity
		int i=0;
		if(policy==1)
		{
			for(i=0;i<No_of_disk;i++)//Set Stripe block 
			{
				//printf("\n\t\t i=%d",i);
				int block_cache_index=0;
			//	printf("\n The tag count: %d",tagCount);
				block_cache_index=getBlockCacheIndex(i);
			//	printf("\n\t\t %d th block_cache_index=%d",i,block_cache_index);
				newStripe->block_array[i].modify=0;
				if(i==blockno)
				{
						newStripe->block_array[i].used=1;
						usedBlock++;
				}
				else
					newStripe->block_array[i].used=0;
			//	printf("\nblock cache index: %p",&block_cache[block_cache_index]);
				newStripe->block_array[i].bptr=&block_cache[block_cache_index];
				
			//	printf("\n i=%d",i);
				//printf("\n check");
			//	printf("\n %dth block address: %p",i,newStripe->block_array[i].bptr);
		//	printf("\n tag:%d",newStripe->block_array[i].bptr->tag);
		//	block_cache[block_cache_index].tag=1;
			}
		}
		if(policy==2)
		{
			int block_cache_index=getBlkCacheIndex();
			for(i=0;i<No_of_disk;i++)//Set Stripe block 
			{
				//printf("\ni=%d",i);
				newStripe->block_array[i].modify=0;
				newStripe->block_array[i].used=0;
				newStripe->block_array[i].bptr=NULL;
			}
			newStripe->block_array[blockno].bptr=&block_cache[block_cache_index];
			newStripe->block_array[blockno].used=1;
			usedBlock++;
			
			//for printing block pointer content
			for(i=0;i<No_of_disk;i++)//Set Stripe block 
			{
			//	printf("\ni=%d",i);
			//	printf("\n The %d th block pointer : %p",i,newStripe->block_array[i].bptr);
			}
					
		 }
			//newStripe->block_array[i].bptr=&block_cache[block_cache_index];
			//printf("\n i=%d",i);
			//printf("\n %dth block address: %p",i,newStripe->block_array[i].bptr);
			//	printf("\n tag:%d",newStripe->block_array[i].bptr->tag);
			//	block_cache[block_cache_index].tag=1

		  newStripe->prev=NULL;
		  newStripe->next=NULL;
		}

	cacheStripe *t=hashTable[hashIndex];
	if(t==NULL)
	{
		hashTable[hashIndex]=newStripe;
	}
	else
	{
		newStripe->next=t;
		newStripe->prev=NULL;
		t->prev=newStripe;
		t=newStripe;
		hashTable[hashIndex]=t;
	}
}

int getBlockCacheIndex(int k) 
{
//	printf("\ncalling getBlockCacheIndex");	
	int i,x=0;
	for(i=0;i<BLOCK_CACHE_SIZE;i++)
	{
		if(block_cache[i].tag==0)
		{
		//	printf("\n Before-----> block_cache[%d].tag==%d",i,block_cache[i].tag );
			strcpy(block_cache[i].buffer,db_array[k].buff);
	//		printf("\n buff %s ",db_array[k].buff);
		//	printf("\n tag %d",block_cache[i].tag);

			block_cache[i].tag=1;
			tagCount++;
		//	printf("\n After------> block_cache[%d].tag==%d",i,block_cache[i].tag );
	//		printf("\n buff %d ",tagCount);
	//		printf("\n buff %s",block_cache[i].buffer);

			x=i;
			break;
		}
	}
	return x;
}

int getBlkCacheIndex()
{
	//printf("\ncalling getBlockCacheIndex");	
	int i,x;
	for(i=0;i<BLOCK_CACHE_SIZE;i++)
	{
		if(block_cache[i].tag==0)
		{
			strcpy(block_cache[i].buffer,db.buff);
			block_cache[i].tag=1;
			x=i;
			break;
		}
	}
	return x;
}

int searchCache(int stripeno, int blockno)
{
//	printf("\nsearch in Cache");	
	int found=0;
	int count=0;
	int hashIndex=stripeno%CACHE_SIZE;
	//cacheQueue *temp=qp->front;
	cacheStripe *t=hashTable[hashIndex];
	//printf("\n In search for stripe in cache Queue---------");
	while(t!=NULL)
	{
		count++;
		//printf("\n%d th element in Queue:%d\t",count,temp->sno);
		if(t->sno==stripeno && t->block_array[blockno].bptr!=NULL)
		{
			found=1;
		//	printf("\nSearch successful at %d",count);	
			cacheHit++;
			break;
		}
		else
		{
			t=t->next;	
		}
		
		//printf("Count: %d",count);
	}
	return found;
}

int searchStripe(int stripeno)
{
	int found=0;
	cacheQueue *q=qp->front;
	while(q!=NULL)
	{
		found++;
		if(q->sno==stripeno)
		{
			found=1;
			break;	
		}
		else
			q=q->next;	
	}
	return found;
}

void updateCache(int stripeno)
{
//	printf("\nupdateCache");
	if(qp->front==qp->tail)
	{
	//	printf("\n Updating Null list");
		return;
	}
    else if(qp->front->sno == stripeno)
	{
	//	printf("\n The front element");
		denqueue();
	}
    else if(qp->tail->sno == stripeno)
	{
	//	printf("\n The tail element");
	    return;
    }
     else
	{
		cacheQueue *q = qp->front->next;
        while(q!=NULL)
        {
        	if(q->sno==stripeno)
        	{
        		q->prev->next=q->next;
        		q->next->prev=q->prev;
        		q->prev=qp->tail;
        		qp->tail->next=q;
        		qp->tail=q;
        		q->next=NULL;
        		break;
			}
			else
			{
				q=q->next;
			}
		}
    }
   // printCacheQueue();
}

void updateBlockRead(int stripeno,int blockno)
{
	int hashIndex=stripeno%CACHE_SIZE;
	cacheStripe *t=hashTable[hashIndex];
	while(t!=NULL)
	{
		if(t->sno==stripeno)
		{
			int block_cache_index=getBlkCacheIndex();
			t->block_array[blockno].bptr=&block_cache[block_cache_index];
			t->block_array[blockno].used=1;
			break;
		}
		else
		{
			t=t->next;
		}
	}
}

void denqueue()
{
//	printf("\n denqueue");
	cacheQueue *r  = qp->front;
    qp->front = qp->front->next;//Update front to next element
   	qp->tail->next=r;//Append the requested node to the tail of the queue
   	r->prev=qp->tail;
   	r->next=NULL;
   	qp->front->prev=NULL;
   	qp->tail=r;
   	//printCacheQueue();
}

void usedUpdateAtHash(int stripeno,int blockno)
{
//	printf("\n usedUpdateAtHash");
	int hashIndex=stripeno%CACHE_SIZE;
	cacheStripe *s=hashTable[hashIndex];
	while(s!=NULL)
	{
		if(s->sno==stripeno)
		{
			if(s->block_array[blockno].used!=1)
			{
				s->block_array[blockno].used=1;	
				usedBlock++;
			}
				break;
		}
		else
		{
			s=s->next;
		}
	}
}

void deleteClean()
{
//	printf("\n deleteClean");
	int sno;
	cacheQueue *td=qp->front;
//	printf("\n\t The number of modified Stripes: %d",modifiedStripe);
//	printf("\n\tThe number of used blocks: %d",usedBlock);
	while(td!=NULL)
	{
		sno=td->sno;
		if(delligibleStripe(sno)==1)
		{
//			printf("/n\t\tThe deleted stripeno:%d",sno);
//			printf("\n call deleteLRUCleanElement for stripe no: %d",sno);
			deleteLRUCleanElement(sno);
			break;
		}
		else
		{
			td=td->next;
		}
	}
	//If the least recently used stripe is clean delete it immediately
	//Otherwise, Writeback spurt size of modified stripe and put them to clean status and delete the least recently one
	
//	printf("\n\tThe number of modified Stripes: %d",modifiedStripe);
//	printf("\n\tThe number of used blocks: %d",usedBlock);
//	printf("\n The number of unnecessary writes: %d",unnecessaryWrite);
	qp->size--;
}

int delligibleStripe(int stripeno)
{
	int i;
	//printf("\n In delligibleStripe for stripe: %d,",stripeno);
	int elligible=0;
	int hashIndex=stripeno%CACHE_SIZE;
	cacheStripe *t=hashTable[hashIndex];
	//cacheStripe *s;
	while(t!=NULL)
	{
		//printf("\n In delligibleStripe	In while current stripe: %d",t->sno);
		if(t->sno==stripeno && t->dirty==0)
		{
			///printf("\n Got the delete elligible Stripe found: %d,",t->sno);
			//printf("\n The prev node in hashtable: %p",t->prev);
			//printf("\nThe next node in hashtable: %p",t->next);
		//	printf("\n LRU element is clean");
			elligible=1;
			if(t->prev!=NULL && t->next==NULL)
			{
		//		printf("\t case 1");
				t->prev->next=NULL;
			}
			else if(t->prev==NULL && t->next!=NULL)
			{
		//		printf("\t case 2");
				hashTable[hashIndex]=t->next;
			}
			else if(t->prev!=NULL && t->next!=NULL)
			{
			//	printf("\t case 3");
					//printf("\n IF");
					t->prev->next=t->next;
					t->next->prev=t->prev;
			}
			else
			{
			//	printf("\t case 4");
					hashTable[hashIndex]=NULL;
			}
			//printf("\n Before restore Block");
			restoreBlocks(t);
			break;
		}
		else
		{
			//printf("\n In delligibleStripe	In while In else");
			t=t->next;
		}
		//printf("\n Elligible = %d",elligible);
	}
	//free(s);
	return elligible;
}

void deleteLRUCleanElement(int sno)
{
	cacheQueue *t=qp->front;
//	printf("\n called deleteLRUCleanElement for stripe no: %d",sno);
	
	//int hashIndex=sno%CACHE_SIZE;
	//cacheStripe *th=hashTable[hashIndex];
	while(t!=NULL)
	{
	//	printf("\n The current stripe number is %d",t->sno);
		if(t->sno==sno)
		{
			if(t->prev==NULL && t->next!=NULL)
			{
				t->next->prev=NULL;
				qp->front=t->next;
			}
			else if(t->prev!=NULL && t->next!=NULL)
			{
				t->prev->next=t->next;
				t->next->prev=t->prev;
			}
			else if(t->prev!=NULL && t->next==NULL)
			{
				t->prev->next==NULL;
				qp->tail==t->prev;
			}
			break;
			//free(t);
		}
		else
		{
			t=t->next;
		}
	}
	
	
		
		
	/*while(th!=NULL)
	{
		if(th->sno==sno)
		{
			if(th->prev==NULL)
			{
				th=th->next;
				th->prev=NULL;
			}
			else if(th->prev!=NULL && th->next!=NULL)
			{
				th->prev->next=th->next;
				th->next->prev=th->prev;
			}
			else if(th->prev!=NULL && th->next==NULL)
			{
				th->prev->next=NULL;
			}
		}
		else
		{
			th=th->next;
		}
	}*/
	
	//qp->front=qp->front->next;
	//qp->front->prev=NULL;
}
void restoreBlocks(cacheStripe *s)
{
//	printf("\n restore blocks");
	int i;
	for(i=0;i<5;i++)
	{
		//printf("\n i=%d",i);
		//printf("\n bptr: %p",s->block_array[i].bptr);
		//printf("\n tag: %d", s->block_array[i].bptr->tag);
		if(s->block_array[i].used==1)
			usedBlock--;
		if(s->block_array[i].bptr!=NULL)
		{
			 s->block_array[i].bptr->tag=0;	
			 tagCount--;
			 s->block_array[i].bptr=NULL;
		}
		   
		//printf("\n after loop");
	}
	//printf("\nfinish restoreBlocks");
}

/********************  Write Request ************************/

void cacheWriteRequest(int stripeno, int blockno,char writedata[])
{
//	printf("\n cacheWriteRequest");
	char c='w';
	//what is write data 
	//Compulsory Miss
	if(qp->front==NULL && qp->tail==NULL)
	{
		//readIn(stripeno,blockno);
		readiop++;
		noiop++;
		placingwrite(stripeno,blockno);
		cacheMiss++;
	}
	else
	{
	//	Hit
		
		if(searchCache(stripeno,blockno)==1)
		{
		//	printf("\n Hit");
			updateCache(stripeno);
			hashUpdateWriteHit(stripeno,blockno);
			//cacheHit++;
		}
		else 
	//	miss
		{
			if(io_complete==0)
			{
		//	printf("\n Miss but cache is not full");
			if(qp->size<CACHE_SIZE)
			{
			//	printf("\n Miss but cache is not full");
				if(policy==1)
				{
					//readIn(stripeno,blockno);
					readiop++;
					noiop++;
					placingwrite(stripeno,blockno);
					
				}
					
				if(policy==2)
				{
					if(searchStripe(stripeno)!=0)
					{
						updateCache(stripeno);
						updateBlockWrite(stripeno,blockno);
					}
					else
					{
							//readInBlock(stripeno,blockno);
						readiop++;
						noiop++;
						placingwrite(stripeno,blockno);
					
					}	
				}
			}
			else
			{
			//	printf("\n Miss but cache is full");
				if(policy==1)
				{
					//readIn(stripeno,blockno);
					deleteClean();
					readiop++;
					noiop++;
					placingwrite(stripeno,blockno);
					//cacheMiss++;
				}
			
				if(policy==2)
				{
					if(searchStripe(stripeno)!=0)
					{
						updateCache(stripeno);
						updateBlockWrite(stripeno,blockno);
					}
					else
					{
						deleteClean();
						readiop++;
						noiop++;
						placingwrite(stripeno,blockno);
						//cacheMiss++;	
					}
				}			
			}
			//cacheMiss++;
			}
		}
	}	
}

void placingwrite(int stripeno, int blockno)
{
//	printf("\n placingwrite");
	cacheQueueUpdate(stripeno);/*As same as Cache Read*/
	hashUpdateWrite(stripeno, blockno);
}

void hashUpdateWrite(int stripeno, int blockno)
{
	
//	printf("\n hashUpdateWrite");
//	printf("\n blockno %d, paritybno %d",blockno,paritybno);
	int hashIndex=stripeno%CACHE_SIZE;
	cacheStripe *newStripe=(cacheStripe *)malloc(sizeof(cacheStripe));
	if(newStripe!=NULL)
	{
		newStripe->sno=stripeno;//Set stripeno
		newStripe->dirty=1;//Set dirtybit
		//memset(newStripe->partialparity,'0',BLOCK_SIZE);//Set partial Parity
		int i=0;
		if(policy==1)
		{
			for(i=0;i<No_of_disk;i++)//Set Stripe block 
			{
				int block_cache_index=getBlockCacheIndex(i);
				
				newStripe->block_array[i].bptr=&block_cache[block_cache_index];
				block_cache[block_cache_index].tag=1;
				if(i!=blockno)
				{
					newStripe->block_array[i].modify=0;
					newStripe->block_array[i].used=0;
				}	
			//	printf("\n blockno: i=%d",i);
			//	printf("\n %dth block address: %p",i,newStripe->block_array[i].bptr);
			}
			
					//if(paritySelection==1)
			updatParity(newStripe->block_array[paritybno].bptr->buffer,newStripe->block_array[blockno].bptr->buffer,BLOCK_SIZE);
			memcpy(newStripe->block_array[blockno].bptr->buffer,writeData,BLOCK_SIZE);
			newStripe->block_array[blockno].modify=1;
			newStripe->block_array[blockno].used=1;
			if(paritySelection==1)
				updatParity(newStripe->block_array[paritybno].bptr->buffer,newStripe->block_array[blockno].bptr->buffer,BLOCK_SIZE);
					//if(paritySelection==1)
					//updatePartialParity(newStripe->partialparity,newStripe->block_array[i].bptr->buffer,BLOCK_SIZE);
			usedBlock++;
			

		}
		if(policy==2)
		{
			int block_cache_index=getBlkCacheIndex();
			memcpy(block_cache[block_cache_index].buffer,writeData,BLOCK_SIZE);
			for(i=0;i<No_of_disk;i++)//Set Stripe block 
			{
				newStripe->block_array[i].bptr=NULL;
				newStripe->block_array[i].modify=0;
				newStripe->block_array[i].used=0;
			}
			newStripe->block_array[blockno].bptr=&block_cache[block_cache_index];
			newStripe->block_array[i].modify=1;
			newStripe->block_array[i].used=1;
			usedBlock++;		
			for(i=0;i<No_of_disk;i++)//Set Stripe block 
			{
			//	printf("\ni=%d",i);
			//	printf("\n The %d th block pointer : %p",i,newStripe->block_array[i].bptr);
			}				
		}
		
		newStripe->prev=NULL;
		newStripe->next=NULL;
	}
	cacheStripe *t=hashTable[hashIndex];
	if(t==NULL)
	{
		hashTable[hashIndex]=newStripe;
		newStripe->next=NULL;
		newStripe->prev=NULL;
	}
	else
	{
		newStripe->next=t;
		newStripe->prev=NULL;
		t->prev=newStripe;
		t=newStripe;
		hashTable[hashIndex]=newStripe;
	}
	modifiedStripe++;
}

void updateBlockWrite(int stripeno,int blockno)
{
	int hashIndex=stripeno%CACHE_SIZE;
	cacheStripe *s=hashTable[hashIndex];
	while(s!=NULL)
	{
		if(s->sno==stripeno)
		{
			int block_cache_index=getBlkCacheIndex();
			s->block_array[blockno].bptr=&block_cache[block_cache_index];
			memcpy(s->block_array[blockno].bptr,writeData,BLOCK_SIZE);
			s->block_array[blockno].used=1;
			s->block_array[blockno].modify=1;
			break;
		}
		else
		{
			s=s->next;
		}
	}
}

void hashUpdateWriteHit(int stripeno, int blockno)
{
//	printf("\n hashUpdateWriteHit");
	int hashIndex=stripeno%CACHE_SIZE;
	cacheStripe *s=hashTable[hashIndex];
	while(s!=NULL)
	{
		if(s->sno==stripeno)
		{
			if(s->dirty!=1)
			{
				s->dirty=1;
				modifiedStripe++;
			}
			updatParity(s->block_array[paritybno].bptr->buffer,s->block_array[blockno].bptr->buffer,BLOCK_SIZE);
			memcpy(s->block_array[blockno].bptr->buffer,writeData,BLOCK_SIZE);
			
			if(s->block_array[blockno].modify!=1)
			{
				if(paritySelection==1)
				updatParity(s->block_array[paritybno].bptr->buffer,s->block_array[blockno].bptr->buffer,BLOCK_SIZE);
				s->block_array[blockno].modify=1;
			}
				
			if(s->block_array[blockno].used!=1)
			{
				s->block_array[blockno].used=1;
				usedBlock++;
			}
			break;
		}
		else
		{
			s=s->next;
		}
	}
}

/********************  Write Request ************************/

void writeBackPolicy()
{
//	printf("\n writeBackPolicy");
//	printf("\n%d %lf",modifiedStripe,Del_Threshold);
	int nofwriteback=(int)modifiedStripe*Del_Threshold;
	int count=0;
	int i;
	int modifiedBlockCount=0;
	int mcount=No_of_disk-1;
//	printf("\n The total no_of_writeback should be: %d",nofwriteback);
//	printf("\n mcount=%d, count=%d",mcount,count);
	int cond=(mcount>0)||(count<nofwriteback);
//	printf("condition: %d",cond);
	while(mcount>0 && nofwriteback>0)
	{
//		printf("\n while");
//		printf("\n writeback for modified stripe with: %d blocks",mcount);
		cacheQueue *td=qp->front;
//		printf("\n before cachequeue while");
		while(td!=NULL)
		{
			int sno=td->sno;
			//printf("\n Cache queue Stripeno: %d",td->sno);
			int hashIndex=sno%CACHE_SIZE;
			cacheStripe *t=hashTable[hashIndex];
			while(t!=NULL)
			{
				//printf("\n stripenumber in hashtable: %d",t->sno);
				if(t->sno==sno && t->dirty==1)
				{
					modifiedBlockCount=0;
					//printf("\nThe stripe no %d got writeback",sno);
					for(i=0;i<No_of_disk;i++)
					{
						if(t->block_array[i].bptr!=NULL && t->block_array[i].modify==1)
						{
							modifiedBlockCount++;
						}
					}
					if(modifiedBlockCount==mcount)
					{
						populateWriteBackQueue(sno);
						nofwriteback--;
						wq->size++;
					}	

				}
				t=t->next;
				
			}
			td=td->next;
		}
		mcount--;
	}
	initiateWriteBack();
//	printf("\n Actual no of write back: %d",count);
}

void populateWriteBackQueue(int stripeno)
{
	writeBackQueueNode *wbNode= (writeBackQueueNode *)malloc(sizeof(writeBackQueueNode));
	wbNode->sno=stripeno;
	wbNode->next=NULL;
	wbNode->prev=NULL;
	if(wq->front==wq->tail && wq->front==NULL)
	{
		wq->front=wbNode;
		wq->tail =wbNode;
	}
	else if(wq->front==wq->tail)
	{
		wq->front->next=wbNode;
		wq->tail=wbNode;
		wbNode->prev=wq->front;
	}
	else
	{
		wq->tail->next=wbNode;
		wbNode->prev=wq->tail;
		wq->tail=wbNode;
	}
	wq->size++;
}

void initiateWriteBack()
{
//	printf("\nInitiate WriteBAck");
	writeBackQueueNode *j=wq->front;
	while(j!=NULL)	
	{
		int sno=j->sno;
//		printf("\n The current size of write back queue:%d", wq->size);
//		printf("\n Writeback needed for %d stripe",sno);
		int hashIndex=sno%CACHE_SIZE;
		cacheStripe *s=hashTable[hashIndex];
		while(s!=NULL)
		{
	//		printf("\n The current stripe is %d:",s->sno);
			if(s->sno==sno)
			{
				//compute Parity
						readIn(sno);
				pthread_t writeback_thread;
				int err;
				noiop++;
				s->dirty=0;
             		        writeiop++;
				noiop++;
				if(paritySelection==2){
					computeParity(sno);
				}
			//	pthread_mutex_lock(&mutex_writeback);
					sem_wait(&item);
					pthread_mutex_lock(&mutex_writeback);
					err = pthread_create(&writeback_thread, NULL, writeback,&sno);
		//			printf("hello\n");
					pthread_mutex_unlock(&mutex_writeback);
					sem_post(&slot);

				pthread_join(writeback_thread,NULL);
				if(err!=0)
				{
					 printf("\n Thread cant be created successfully\n");
				}
				else
				{
					 printf("\n Thread created successfully\n");
				}
				wq->size--;
				modifiedStripe--;
				int i;
				for(i=0;i<No_of_disk;i++)
				{
					if(s->block_array[i].modify!=1)
					{
						unnecessaryWrite++;
					}
					s->block_array[i].used=0;
					s->block_array[i].modify=0;
					
				}
				break;
			}
			else
			{
				s=s->next;
			}
		}
		j=j->next;
		if(j!=NULL)
			j->prev=NULL;
		wq->front=j;
			
	}
}

void *writeback(void *sno) 
{
	//io_complete=1;
	int *x=(int *)sno;
	//printf("\n Writeback done for %d stripeno",*x);
	int ret;
	int i;

	pthread_t thread1,thread2,thread3,thread4,thread0;
	arg_struct args1,args2,args3,args4,args0;
	memset(file_table_writeData,'1',BLOCK_SIZE);
	sem_wait(&slot);
	pthread_mutex_init(&mutex_lfsc,NULL);
	pthread_mutex_lock(&mutex_lfsc);
	args0.sno=*x;
	args0.i=0;
	args2.sno=*x;
	args2.i=2;
	args3.sno=*x;
	args3.i=3;
	args4.sno=*x;
	args4.i=4;
	args1.sno=*x;
	args1.i=1;




	pthread_create(&thread0,NULL,threadWork,(void *)&args0);
	pthread_create(&thread1,NULL,threadWork,(void *)&args1);
	pthread_create(&thread2,NULL,threadWork,(void *)&args2);
	pthread_create(&thread3,NULL,threadWork,(void *)&args3);
	pthread_create(&thread4,NULL,threadWork,(void *)&args4);
	

	pthread_join(thread1,NULL);
	pthread_join(thread2,NULL);
	pthread_join(thread3,NULL);
	pthread_join(thread4,NULL);
	pthread_join(thread0,NULL);
		//  printf("before mutex\n");
	pthread_mutex_unlock(&mutex_lfsc);
    pthread_mutex_unlock(&mutex_writeback);
	sem_post(&item);

}

void readIn(int sno){
	struct timeval stop, start;
	int ret;
	int i;
	//pthread_mutex_t mutex1;
	//cacheStripe *newStripe=(cacheStripe *)malloc(sizeof(cacheStripe));
	pthread_t thread1,thread2,thread3,thread4,thread0;
	arg_struct_read args1,args2,args3,args4,args0;
//	memset(file_table_writeData,'1',BLOCK_SIZE);
//	gettimeofday(&start, NULL);
	pthread_mutex_init(&mutex_lfsc,NULL);
	pthread_mutex_lock(&mutex_lfsc);
//	printf("\nread in \n");
	//args1=malloc(sizeof(arg_struct_read));
	args0.sno=sno;
	args0.i=0;
	args0.buffer=db_array[0].buff;
	args2.sno=sno;
	args2.i=2;
	args0.buffer=db_array[2].buff;
	args3.sno=sno;
	args3.i=3;
	args3.buffer=db_array[3].buff;
	args4.sno=sno;
	args4.i=4;
	args4.buffer=db_array[4].buff;
	args1.sno=sno;
	args1.i=1;
	args1.buffer=db_array[1].buff;

	//timerC++;


	pthread_create(&thread0,NULL,readInThread,(void *)&args0);
	pthread_create(&thread1,NULL,readInThread,(void *)&args1);
	pthread_create(&thread2,NULL,readInThread,(void *)&args2);
	pthread_create(&thread3,NULL,readInThread,(void *)&args3);
	pthread_create(&thread4,NULL,readInThread,(void *)&args4);


	pthread_join(thread1,NULL);
	pthread_join(thread2,NULL);
	pthread_join(thread3,NULL);
	pthread_join(thread4,NULL);
	pthread_join(thread0,NULL);


//	printf("before mutex\n");
	pthread_mutex_unlock(&mutex_lfsc);
	//pthread_mutex_lock(&mutex_writeback);
	pthread_mutex_unlock(&mutex_writeback);
	//printf("after mutex\n");
	//printf("\nread in \n");
	//gettimeofday(&stop, NULL);
	//areadLatency = areadLatency + (stop.tv_usec-start.tv_usec );

//	pthread_exit(NULL);
//	printf("read latency is %lu\n",(areadLatency/timerC));
}

void updatParity(char *str1,char *str2, int len)
{
	int i;
	for(i=0;i<len;i++)
	{
		*str1++ ^= *str2++;
	}
}

void computeParity(int sno)
{
	int hashIndex=sno%CACHE_SIZE;
	cacheStripe *s=hashTable[hashIndex];
	while(s!=NULL)
	{
		if(s->sno==sno)
		{
			int i;
			for(i=0;i<No_of_disk;i++)
				{
					if(s->block_array[i].modify!=1)
					{
						updatParity(s->block_array[paritybno].bptr->buffer,s->block_array[i].bptr->buffer,BLOCK_SIZE);
					}
				}
		}
		else
		{
			s=s->next;
		}
	}
}
