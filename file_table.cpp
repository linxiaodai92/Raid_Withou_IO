//
// Created by lynn on 8/29/17.
//

#include "file_table.h"
#include <iostream>



retval RAID_5_file_table::raid_create(const int device_number) {
    char diskname[device_number];
    //initialize block
    Block* block = new Block();
    raid_file_table = new File_table(totalNumofFile);
    block->initial_block();
    for (int i = 0; i < device_number; i++){
        sprintf(diskname,"%d.disk",i);
        raid_disk[i] = new Disk(i,0);
        raid_disk[i]->file_pointer = fopen(diskname,"w+");
       // printf("create %d disk\n",diskname);
        if (raid_disk[i]->file_pointer == NULL) {
            printf("error, could not allocate disk\n");
            exit(1);
        }
        if (ini_block == NULL) {
            printf("error, initial block is null\n");
            exit(1);
        }
        rewind(raid_disk[i]->file_pointer);
    }
   // printf("raid disk is %d\n",ra)
    return SUCCESS;
}

int File_table::is_parity(int disk_num, int stripe_number) {
    int is_parity=0;
    //printf("disk number is %d, stripe_number is %d\n",disk_num,stripe_number);
    if(((stripe_number%device_number)+disk_num)==device_number-1){
        is_parity=1;
        //printf("parity\n");
    }else{
        is_parity=0;
    }
    return is_parity;
}

//need to be tested
int RAID_5_file_table::get_paritynum(int stripe_num) {
    for(int i = 0; i < device_number; i++) {
        if (((stripe_num % device_number) + i )== device_number -1 ){
            return i;
        }
    }
}
//need to be tested
int RAID_5_file_table::requestStripeNm(int file_name, int block_num) {
    int stripe_num = 0;
    int offset = block_num;
    File_entry* file_entry = NULL;
    file_entry = raid_file_table->find(file_name);
    stripe_num = file_entry->get_start_offset() + offset/(device_number - 1);
    return stripe_num;
}

//need to be tested
int RAID_5_file_table::getblocknum(int fileid, int block_num) {
    File_entry* file_entry = NULL;
    file_entry = raid_file_table->find(fileid);
    int current_disk_num = file_entry->get_start_disk();
    int stripeno = file_entry->get_start_offset();
    while (block_num > 0) {
        if(raid_file_table->is_parity(current_disk_num, stripeno) == 1) {

        }else {
            block_num--;
        }
        current_disk_num = current_disk_num + 1;
        if (current_disk_num > 4) {
            stripeno = stripeno + 1;
            current_disk_num = current_disk_num %device_number;
        }
    }
    return current_disk_num;
}

retval RAID_5_file_table::initailize_raid_state_with_file_table() {
    if (raid_create(device_number) < 0){
        return FAILED;
    };
    if (raid_file_table->ini_file_table() < 0) {
        return FAILED;
    };
    if(raid_file_table->file_table_create() < 0){
        printf("hello\n");
        return FAILED;
    };
    printf("file_created\n");
    if(raid_file_table->allocate_disk_filetable() < 0){
        return FAILED;
    };
    return SUCCESS;
}

retval Block::initial_block() {
    for (int j = 0; j < block_size; j++){
        ini_block[j] = 0;
    }
    return SUCCESS;
}

int File_table::total_file_num_inFileData(FILE *file_data) {
    int lines = 0;
    int charnum = 0;
    while(!feof(file_data)){
        charnum = fgetc(file_data);
        if (charnum == '\n') {
            lines++;
        }
    }
    return lines;
}

retval File_table::ini_file_table() {
    FILE *file_data = fopen(FILE_DATA_FILE,"r");
    if (file_data == NULL) {
        printf("error, could not open file data file\n ");
        exit(1);
    }
    set_file_number(total_file_num_inFileData(file_data));
    int actual_file_number2 = get_file_number();
    for (int i = 0; i < actual_file_number2; i++) {
        file_table[i] = NULL;
        printf("created a file_table[%d]\n", i);
    }
    printf("file num is %d\n", actual_file_number2);
    fclose(file_data);
    return SUCCESS;
}
int File_table::get_file_number() {
    return this->actual_file_number2;
}

void File_table::set_file_number(int file_number) {
    this->actual_file_number2 = file_number;
}
retval File_table::file_table_create() {
    int fileid = 0;
    int filesize = 0;
    FILE *file_data_file = fopen(FILE_DATA_FILE,"r");
    if (file_data_file == NULL) {
        printf("could not open filesizeDA file\n");
        exit(1);
    }
    //File_entry* file_entry = NULL;
    printf("file opened\n");
    for(int j = 0; j < actual_file_number2; j++){
        fscanf(file_data_file,"%d, %d", &fileid, &filesize);
        File_entry* file_entry = new File_entry(fileid, filesize, 0, 0);
        if (file_entry == NULL) {
            printf("obejct created wrong\n");
            exit(0);
        }
        printf("total num of file is %d\n",totalNumofFile);
        file_entry->set_fileid(fileid);
        file_entry->set_filesize(filesize);
        printf("fileid %d, filesize %d\n", fileid, filesize);
        file_table[j] = file_entry;
        file_entry->get_filesize();
        printf("file entry is %s\n" , file_entry);
        printf("filetable[%d] is %s and fileentry fileid is %d, filesize is %d\n", j,file_table[j], file_entry->get_fileid(),file_entry->get_filesize());
    }
    fclose(file_data_file);
    return SUCCESS;
}

File_entry* File_table::find(int fileid) {
    int i = 0;
    while((file_table[i]->get_fileid()) != fileid && i < actual_file_number2 ) {
        if(file_table[i] == NULL) {
            printf("file_table[%d] is NULL, can't find file in filetable\n", i);
            exit(0);
        } else {
            i++;
        }
    }
    printf("file_table[%d] fileid is %d \n", i, file_table[i]->get_fileid() );
    return file_table[i];
}

retval File_table::allocate_disk_filetable() {
        int j = 0;
        File_entry* fe = NULL;
        int current_offset=0;
        for(int i=0;i<actual_file_number2;i++){
            fe=file_table[i];

            if(is_parity(raid_disk[0]->get_disk_num(),raid_disk[0]->get_current_offset()) == 1) {
                    printf("parity\n");
                    raid_disk[0]->set_disk_num((raid_disk[0]->get_disk_num() + 1) % device_number);
                    j++;
                    if(j == 5){
                        j = 0;
                        raid_disk[0]->set_current_offset(raid_disk[0]->get_current_offset() + 1);
                    }
            }
            file_table[i]->set_start_disk(raid_disk[0]->get_disk_num());
            file_table[i]->set_start_offset(raid_disk[0]->get_current_offset());
            int file_size=file_table[i]->get_filesize();
            printf("i: %d, file_size %d\n",i,file_size);
            while(file_size > 0) {
                if (is_parity(raid_disk[0]->get_disk_num(),raid_disk[0]->get_current_offset())== 1){
                    raid_disk[0]->set_disk_num((raid_disk[0]->get_disk_num() + 1) % device_number);
                    j++;
                    if(j==5) {
                        j = 0;
                        raid_disk[0]->set_current_offset(raid_disk[0]->get_current_offset() + 1);
                    }
                } else {
                    raid_disk[0]->set_disk_num((raid_disk[0]->get_disk_num() + 1) % device_number);
                    j++;
                    if(j == 5) {
                        j = 0;
                        raid_disk[0]->set_current_offset(raid_disk[0]->get_current_offset() + 1);
                    }
                    file_size--;
                }
            }
        }
        printf("in filetable created \n");
        for(int i=0;i<actual_file_number2;i++){
        printf("fe %d,start disk %d,fileszie %d,start offset %d\n",i,file_table[i]->get_start_disk(),file_table[i]->get_filesize(),file_table[i]->get_start_disk(),file_table[i]->get_start_offset());
        }
    return SUCCESS;
}
/*
File_table* File_table::file_table_pInstance = 0;
File_table* File_table::Instance() {
    if(file_table_pInstance == 0) {
        file_table_pInstance = new File_table(totalNumofFile);
    }
    return file_table_pInstance;
}*/

File_table::File_table(int file_number) {
    this->actual_file_number2 = file_number;
}

File_entry::File_entry( int fileid,  int filesize,  int start_disk,
                        int start_offset) {
    this->fileid = fileid;
    this->filesize = filesize;
    this->start_disk = start_disk;
    this->start_offset = start_offset;
    printf("hello in  file_ entry \n" );
}

void File_entry::set_start_disk(int start_disk) {
    this->start_disk = start_disk;
}

void File_entry::set_fileid(int fileid) {
    this->fileid = fileid;
}

void File_entry::set_filesize(int file_size) {
    this->filesize =file_size;
}

void File_entry::set_start_offset(int start_offset) {
    this->start_offset= start_offset;
}

int File_entry::get_filesize() {
    return this->filesize;
}

int File_entry::get_start_offset() {
    return this->start_offset;
}

int File_entry::get_start_disk() {
    return this->start_disk;
}

int File_entry::get_fileid() {
    return this->fileid;
}

void Disk::set_disk_num(int diskNum) {
    this->disk_num = diskNum;
}

Disk::Disk(int disk_num, int current_offset) {
    this->disk_num = disk_num;
    this->current_offset = current_offset;
}

void Disk::set_current_offset(int offset) {
    this->current_offset = offset;
}

void Disk::set_file_pointer(FILE *file) {
    this->file_pointer = file;
}

int Disk::get_disk_num() {
    return this->disk_num;
}

int Disk::get_current_offset() {
    return this->current_offset;
}

retval RAID_5_file_table::unitTest() {
    initailize_raid_state_with_file_table();
    File_entry* file_entry1 = raid_file_table->find(1);
    file_entry1 = raid_file_table->find(2);
    file_entry1 = raid_file_table->find(3);
    printf("find file id:%d start_disk %d  file_start_offset %d\n", file_entry1->get_fileid(),file_entry1->get_start_disk(),file_entry1->get_start_offset() );
    for(int i = 0; i <raid_file_table->actual_file_number2; i++) {
        printf("rai_file table i is %d, fileid is  %d  start disk %d file_size is %d start_offset %d \n", i, file_table[i]->get_fileid(),file_table[i]->get_start_disk(),file_table[i]->get_filesize(),file_table[i]->get_start_offset());
    }
    printf("stripe number for 1 is %d\n",requestStripeNm(1,4));
    printf("stripe number for 2 is %d\n",requestStripeNm(2,4));
}

