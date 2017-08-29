#include "filetable.h"
/*
d_create(DEVICE_NUMBER);
	ini_file_table();
	file_table_create();
	allocate_disk_filetable();
	find(2);
	find(1);
	find(2);
	printf("request %d\n",requestStripeNm(2,5));
	printf("re %d\n",requestStripeNm(9,3));
	write_back(1,ini_block,1);
	return 0;

}
*/
FILE *ptrs[DEVICE_NUMBER];
int  ini_block[BLOCK_SIZE];
struct file_entry *file_table[File_number];
char file_table_writeData[BLOCK_SIZE];
pthread_mutex_t mutex;

void initial_ptr(){

	int i=0;
	for(i=0;i<DEVICE_NUMBER;i++){
		ptrs[i]=NULL;


	}


}

void initial_block(){

	int i=0;
	for(i=0;i<BLOCK_SIZE;i++)
	{
		ini_block[i]=0;
	}
}

void initial_file_entry()
{
	int i=0;
	for(i=0;i<File_number;i++)
	{

		file_table[i]=NULL;	
	}
}

int total_files=0;
void ini_file_table(){
	char line[201];
	file_entry *fe;

	FILE *file_data=fopen(FILE_DATA,"r");
	if(file_data==NULL){
		printf("couldnot open fileSizeDA file\n");
	}

	total_files=total_file_num(file_data);
//	*file_table[total_files];
//	printf("total file num is %d\n",total_files);

	fclose(file_data);


}
int total_file_num(FILE *file_data){
	int lines=0;
	int ch=0;
	while(!feof(file_data)){
		ch=fgetc(file_data);
		if(ch=='\n')
			lines++;
	}
	return lines+1;
}

int allocate_disk_filetable(){
	int j=0;
	int i;
	disk *disk0=(disk *)malloc(sizeof(disk));
	file_entry *fe=(file_entry *)malloc(sizeof(file_entry));
	disk0->disk_num=0;
	disk0->current_offset=0;
	disk *disk1=(disk *)malloc(sizeof(disk));
	disk *disk2=(disk *)malloc(sizeof(disk));
	disk *disk3=(disk *)malloc(sizeof(disk));
	disk *disk4=(disk *)malloc(sizeof(disk));
	disk1->disk_num=1;
	disk1->current_offset=0;
	disk2->disk_num=2;
	disk2->current_offset=0;
	disk3->disk_num=3;
	disk3->current_offset=0;

	int current_offset=0;
	for(i=0;i<total_files;i++){
        fe=file_table[i];
        int filesize=file_table[i]->filesize;
        if (i == 0) {
            disk0->disk_num = 0;
            fe->start_disk=disk0->disk_num;
            fe->start_offset=disk0->current_offset;
            disk0->current_offset=disk0->current_offset+filesize - 1;
        }
		if (i == 1) {
			disk1->disk_num = 1;
			fe->start_disk=disk1->disk_num;
			fe->start_offset=disk1->current_offset;
			disk1->current_offset=disk1->current_offset+filesize - 1;
		}
		if (i == 2) {
			disk2->disk_num = 2;
			fe->start_disk=disk2->disk_num;
			fe->start_offset=disk2->current_offset;
			disk2->current_offset=disk2->current_offset+filesize - 1;
		}
		if (i == 3) {
			disk3->disk_num = 3;
			fe->start_disk=disk3->disk_num;
			fe->start_offset=disk3->current_offset;
			disk3->current_offset=disk3->current_offset+filesize - 1;
		}
        if ( i!= 0 && i % (DEVICE_NUMBER-1) == 0) {

			disk0->disk_num = 0;
			fe->start_disk=disk0->disk_num;
			fe->start_offset=disk0->current_offset;
			disk0->current_offset=disk0->current_offset+filesize;

        }
        if (i!=1 && i %(DEVICE_NUMBER-1) == 1) {
            disk1->disk_num = 1;
            fe->start_disk=disk1->disk_num;
            fe->start_offset=disk1->current_offset;
			disk1->current_offset=disk1->current_offset+filesize;

        }
        if (i!= 2 && i %(DEVICE_NUMBER -1) == 2){
            disk2->disk_num = 2;
            fe->start_disk=disk2->disk_num;
            fe->start_offset=disk2->current_offset;
			disk2->current_offset=disk2->current_offset+filesize;

        }
        if (i!=3 && i%(DEVICE_NUMBER-1) ==3 ) {
            disk3->disk_num = 3;
            fe->start_disk=disk3->disk_num;
            fe->start_offset=disk3->current_offset;
			disk3->current_offset=disk3->current_offset+filesize;

        }

	}
	for(i=0;i<total_files;i++){
		//printf("fe %d,%d disk %d,offset %d\n",file_table[i]->fileid,file_table[i]->filesize,file_table[i]->start_disk,file_table[i]->start_offset);
		
	}
	free(disk1);
	free(fe);
	free(disk0);
	free(disk2);
	free(disk3);
	return 0;
}
int file_table_create(){
	int fileid;
	int filesize;
	FILE *file_data=fopen(FILE_DATA,"r");
	disk *disk1=(disk *)malloc(sizeof(disk));
	disk1->disk_num=0;
	disk1->current_offset=0;
	char line[20];
	if(file_data==NULL){
		printf("couldnot open fileSizeDA file\n");
	}
	int i=total_files;
	int j=0;

	for(i=0;i<total_files;i++){
		file_entry *fe=(file_entry *)malloc(sizeof(file_entry));	
		fscanf(file_data,"%d,%d",&fileid,&filesize);
	//	printf("%d,%d\n",fileid,filesize);	
		fe->start_disk=disk1->disk_num;
		fe->start_offset=disk1->current_offset;
		fe->fileid=fileid;
		fe->filesize=filesize;
		//printf("id %d size %d start_disknum %d offset %d\n ",fe->fileid,fe->filesize,fe->start_disk,fe->start_offset);
		//allocate_disk_filetable(fe,disk1);
		file_table[i]=fe;
	//	printf("file_table %d,%d\n",file_table[i]->fileid,file_table[i]->filesize);
		//free(fe);

	}
		
	free(disk1);
		fclose(file_data);
	return 0;
}
void raid_create(int device_number){
	char diskname[DEVICE_NUMBER];
	int i;
	int j;

	memset(ini_block,'0',BLOCK_SIZE);
	for(i=0;i<DEVICE_NUMBER;i++){
		//error occur stack smashing detected;
	//	sprintf(diskname,"/mnt/raid5/disk%d/%d.disk",i+1,i);
		sprintf(diskname,"%d.disk",i);
		//printf("diskname is %s\n",diskname);
		ptrs[i]=fopen(diskname,"w+");
		if(ptrs[i]==NULL){
			printf("could not create a disk\n");
			exit(1);

		}
			if(ini_block==NULL){
			printf("initial block is null in disk\n");
			exit(1);
		}
		/*
		for(j=0;j<DISK_SIZE;j++){

			fwrite(ini_block,1,BLOCK_SIZE,ptrs[i]);

		}
		*/
	//	printf("finish created disk\n");
		rewind(ptrs[i]);
	//	fclose(ptrs[i]);
	}
	

}

int is_parity(int disk_num,int stripe_number){
	int is_parity=0;
//	printf("disk number is %d, stripe_number is %d\n",disk_num,stripe_number);
	if(((stripe_number%DEVICE_NUMBER)+disk_num)==DEVICE_NUMBER-1){
			is_parity=1;
		//	printf("parity\n");
	}else{

		is_parity=0;

	}

	return is_parity;



}
int getParitybnum(int stripe_num) {
	for(int i = 0; i < DEVICE_NUMBER; i++) {
		if (((stripe_num % DEVICE_NUMBER) + i )== DEVICE_NUMBER -1 ){
			return i;
		}
	}
}

int requestStripeNm(int file_name,int block_num){
	
	int stripe_num;
//	int current_disk_num;
	int offset=block_num;
	file_entry *fe;
	fe=find(file_name);
	stripe_num=fe->start_offset+offset;


	return stripe_num;
}
file_entry *find(int fileid){
	//file_current *new;
	int i=0;
//	int j=0;
//	for(j=0;i<total_files+1;i++){

//		printf("fe %d,%d\n",file_table[i]->fileid,file_table[i]->filesize);
		

//	}

	while(file_table[i]->fileid!=fileid&&i<501){
		
		if(file_table[i]==NULL){

			printf("file_table[i] is NULLi,can't find file\n");
				exit(0);
		}else{
			i++;

		}
		
	//	printf("i: %d %d %d\n",i,file_table[i]->fileid,fileid);
		

	}
//	printf("find %d %d %d %d\n",file_table[i]->fileid,file_table[i]->filesize,file_table[i]->start_disk,file_table[i]->start_offset);
	return file_table[i];


}

//ioreq_event *iotrace_validate_get_ioreq_event(FILE *origfile,ioreq_event *new){

//	char line[201];
int getblocknum(int fileid,int block_num){
	file_entry *fe;
	fe=find(fileid);
	int current_disk_num=fe->start_disk;
	int stripeno=fe->start_offset;

	return current_disk_num;

}
/*
void write_back(int stripenum,char *p,int i){

		fseek(ptrs[i],stripenum*BLOCK_SIZE,SEEK_SET);		
		fwrite(p,1,BLOCK_SIZE,ptrs[i]);



	
}
 */
void *threadWork(void *argument)
{

    //memset(file_table_writeData,'1',BLOCK_SIZE);
    arg_struct *args = (arg_struct *) argument;

    fseek(ptrs[args->i],args->sno*BLOCK_SIZE,SEEK_SET);
    //fread(file_table_writeData,1,BLOCK_SIZE,ptrs[args->i]);
    fwrite(file_table_writeData,1,BLOCK_SIZE,ptrs[args->i]);
   // printf("done\n");
    pthread_exit(0);
}
void *readInThread(void *argument){
    pthread_mutex_init(&mutex,NULL);
    pthread_mutex_lock(&mutex);
	arg_struct_read *args = (arg_struct_read *) argument;
	fseek(ptrs[args->i],args->sno*BLOCK_SIZE,SEEK_SET);
	fwrite(args->buffer,BLOCK_SIZE,1,ptrs[args->i]);
    pthread_mutex_unlock(&mutex);
	pthread_exit(0);
}

/*
void writeBack(int sno)
{

	int ret;
	int i;
	pthread_t thread1,thread2,thread3,thread4,thread0;
	arg_struct args1,args2,args3,args4,args0;
	memset(file_table_writeData,'1',BLOCK_SIZE);
	pthread_mutex_init(&mutex,NULL);
	pthread_mutex_lock(&mutex);
	args0.sno=sno;
	args0.i=0;
	args2.sno=sno;
	args2.i=2;
	args3.sno=sno;
	args3.i=3;
	args4.sno=sno;
	args4.i=4;
	args1.sno=sno;
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
	pthread_mutex_unlock(&mutex);
	//   printf("after mutex\n");
//	pthread_exit(NULL);
}
*/

