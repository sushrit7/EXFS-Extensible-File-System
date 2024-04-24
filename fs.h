#ifndef __FS_H__
#define __FS_H__
#include <sys/mman.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include <openssl/sha.h>


#define FSSIZE 10000000

extern unsigned char* fs;
#define SEGMENT_SIZE 5 * 1024 * 1024 // 5MB
#define BLOCK_SIZE 4096
#define MAX_FILENAME_LEN 256
#define MAX_ENTRIES_PER_BLOCK 1
#define INODESPERSEG 10
#define BLOCKSPERSEG 1020
#define MAX_ENTRIES 50
#define HASH_SIZE SHA256_DIGEST_LENGTH
#define HASH_TABLE_SIZE 100000


enum EntryType 
{
    DIR = 0,
    DATA = 1
};

typedef struct Manifestheader{
  int numdirectorysegs;
  int numdatasegs;
  int numinodes;
  int numsegs;
}Manifestheader;

typedef struct Manifestentry{
  char name[256];
  int type;
}Manifestentry;

typedef struct Manifest{
  Manifestheader MH;
  Manifestentry entries[MAX_ENTRIES];
}Manifest;

typedef struct Superblock{
  int blocksize;
  int segsize;
  int inodesperseg;
  int blockid;
  int blocksperseg;
}Superblock;

typedef struct freeblocklist{
  char bitmap[BLOCKSPERSEG];
}freeblocklist;

typedef struct inode{
  int inuse;
  int type;
  int size;
  int blocks[BLOCKSPERSEG];
  int indirect;
  int dindirect;
  int tindirect;
}inode;

typedef struct block{
  char data[BLOCK_SIZE];
}block;

typedef struct DirectoryEntry{
  char name[256];
  int type;
  int inodenum;
  int inuse;
}DirectoryEntry;

typedef struct Segment{
  int type;
  Superblock SB;
  freeblocklist FBL;
  inode inodes[INODESPERSEG];
  block blocks[BLOCKSPERSEG];
}Segment;

///EXTRA CREDIT///
typedef struct {
    int block_id;
    unsigned char hash[HASH_SIZE];
    int referenced;
} hash_entries;

typedef struct {
    // hashheader header;
    hash_entries entries[HASH_TABLE_SIZE];
} hash_table;

void initialize_and_write_file_system();
void mapfs(int fd);
void unmapfs();
void formatfs();
void loadfs();
void lsfs();
void addfilefs(char* fname, char *fpath);
void removefilefs(char* fname);
void extractfilefs(char* fname);
void debugfs();

//Print options:
void print_directory_entries(int dir_inode, int indent);
void print_entries();
void print_manifest();
void print_hash_table();


//Manifest
char * initialize_manifest();
char* update_manifest(int type);

//Segment
void create_segment(int type, int num);
char * get_segname(int seg_id);
int get_seg_id(char* seg_name);

//Inode
void mark_inode(int inode_num, int use);
void mark_unused_inode_in_directory_entry(int dir_inode, int file_inode);
int find_unused_inode(int type);
void update_inode_with_blocks(int inode_num, int* blocks, int num_blocks, int file_size);
void get_blocks_from_inode(int file_inode, int* blocks);


//Directory Entires
int create_directory(const char* name, int old_inode);
int search_inode_in_directory_entry(int current_inode, char* name);
void add_directory_entry_in_parent(int parent_inode, int child_inode, char* name, int type);
int get_inode_to_last_directory(char* fpath, int create_new);
void mark_unused_inode_in_directory_entry(int dir_inode, int file_inode);

//Files
void add_new_file(int dir_inode, char* fpath);
void write_file_to_fs(FILE* source_file, int file_size, int* blocks); 
int get_file_size(int file_inode);


//Blocks
void get_empty_blocks(int num_blocks, int* blocks, int type);
void write_file_to_block(char* buffer, int block, int bytes_to_read);

//Venti
void initialize_hash_table();
int venti(unsigned char* buffer_hash, int bytes_to_read, int* block);
void calculate_hash(const unsigned char* buffer, size_t buffer_size, unsigned char* hash_result);
void add_to_hash_table(unsigned char* buffer_hash, int block);
void set_FBL_free(int block);
void decrement_refrenced_count(int file_inode);

//Helper
char** parse_path(const char* path, int* num_elements);
void split_path(const char *path, char *dir, char *file); 
int extract_number(const char* str);
void debug_directory_entries(int dir_inode, int indent);
#endif
