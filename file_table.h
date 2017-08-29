//
// Created by lynn on 8/29/17.
//

#ifndef SFVC_FILE_TABLE_H
#define SFVC_FILE_TABLE_H
#include <stdlib.h>
#include <stdio.h>
#define FILE_NUMBER 500;
#define DEVICE_NUMBER 5;
#define BLOCK_SIZE 4096;
#define DISK_SIZE 20000;
#define FILE_DATA_FILE "FileSizesDA.csv"
static const unsigned int block_size = BLOCK_SIZE;
enum retval{
    SUCCESS = 1,
    FAILED = -1,
    FILE_NOT_OPEN = -2
};

class Disk {
public:
    Disk(int disk_num, int current_offset);//constructor
    Disk(){};
    ~Disk(){};//destructor
    void set_disk_num(int diskNum);
    void set_current_offset(int offset);
    void set_file_pointer(FILE *file);
    int  get_disk_num();
    int  get_current_offset();
    FILE *file_pointer;
private:
     int disk_num;
     int current_offset;
};

class Block{
public:

    Block(){};
    ~Block(){};
    retval initial_block();
};

static const int device_number = DEVICE_NUMBER;
static Disk* raid_disk[device_number];
static int ini_block[block_size];
class File_entry{
public:
    File_entry( int fileid,  int filesize,  int start_disk,  int start_offset);
    ~File_entry(){};
    void set_fileid(int fileid);
    void set_filesize(int file_size);
    void set_start_disk(int start_disk);
    void set_start_offset(int start_offset);
    int get_fileid();
    int get_filesize();
    int get_start_disk();
    int get_start_offset();
private:
     int fileid;
     int filesize;
     int start_disk;
     int start_offset;

};
static const int totalNumofFile = FILE_NUMBER;
static File_entry* file_table[totalNumofFile];

class File_table{
public:
    File_table(int file_number);
    retval ini_file_table();
    retval file_table_create();
    retval allocate_disk_filetable();
    int total_file_num_inFileData(FILE *file_data);
    File_entry* find(int fileid);
    int get_file_number();
    void set_file_number(int file_number);
    int actual_file_number2;
    int is_parity(int disk_num, int stripe_number);
private:
    File_table(File_table const&){};
    File_table& operator = (File_table const&){};
};
static File_table* raid_file_table;

class RAID_5_file_table{

public:
    RAID_5_file_table(){};
    ~RAID_5_file_table(){};
    retval raid_create(const int device_number);

    int get_paritynum(int stripe_num);
    int requestStripeNm(int file_name, int block_num);
    int getblocknum(int fileid, int block_num);
    retval  initailize_raid_state_with_file_table();
    retval  unitTest();
};


#endif //SFVC_FILE_TABLE_H
