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

#define FSSIZE 10000000

// using namespace std;

extern unsigned char* fs;
#define SEGMENT_SIZE 5 * 1024 * 1024 // 5MB
#define BLOCK_SIZE 4096
#define MAX_FILENAME_LEN 256
#define MAX_ENTRIES_PER_BLOCK 1
#define INODESPERSEG 10
#define BLOCKSPERSEG (SEGMENT_SIZE / BLOCK_SIZE)

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
  Manifestentry entries[50];
}Manifest;

typedef struct Superblock{
  int blocksize;
  int segsize;
  int inodesperseg;
  int blockid;
  int blocksperseg;
}Superblock;

typedef struct freeblocklist{
  char bitmap[(SEGMENT_SIZE/BLOCK_SIZE)/8];
}freeblocklist;

typedef struct inode{
  int inuse;
  int type;
  int size;
  int blocks[(BLOCK_SIZE/4)-4];
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

// typedef struct {
//     char name[255]; // Name of the directory
//     int num_entries; // Number of directory entries
//     DirectoryEntry entries[MAX_ENTRIES_PER_BLOCK]; // Array of directory entries
//     int inodenum; 
//     int inuse;
//     // Add any additional metadata if needed
// } Directory;

typedef struct Segment{
  int type;
  Superblock SB;
  freeblocklist FBL;
  inode inodes[INODESPERSEG];
  block blocks[BLOCKSPERSEG];
}Segment;

void initialize_and_write_file_system();
void mapfs(int fd);
void unmapfs();
void formatfs();
void loadfs();
void lsfs();
void addfilefs(char* fname, char *fpath);
void removefilefs(char* fname);
void extractfilefs(char* fname);

#endif
