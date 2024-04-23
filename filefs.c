#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>

#include <errno.h>

#include <sys/mman.h>

#include "fs.h"

int zerosize(int fd);
void exitusage(char* pname);


int main(int argc, char** argv){
  
  int opt;
  int create = 0;
  int list = 0;
  int add = 0;
  int remove = 0;
  int extract = 0;
  char* toadd = NULL;
  char* toremove = NULL;
  char* toextract = NULL;
  char* fsname = NULL;
  char* fpath = NULL;
  char* copyname = NULL;
  int fd = -1;
  int newfs = 0;
  int filefsname = 0;
  int debug = 0;

  fsname = "segments/manifest";
  


   if (argc < 2) {
        exitusage(argv[0]);
    }

    if (strcmp(argv[1], "-l") == 0) {
        list = 1;
    } else if (strcmp(argv[1], "-a") == 0) {
        add = 1;
        if (argc >= 3) {
            toadd = strdup(argv[2]);
        } else {
            printf("Path for adding required!\n");
            exitusage(argv[0]);
        }
        if (argc >= 5 && strcmp(argv[3], "-f") == 0) {
            fpath = strdup(argv[4]);
        } else {
            printf("File path for adding required!\n");
            exitusage(argv[0]);
        }
    } else if (strcmp(argv[1], "-r") == 0) {
        remove = 1;
        if (argc >= 3) {
            toremove = strdup(argv[2]);
        } else {
            printf("Path for removing required!\n");
            exitusage(argv[0]);
        }
    } else if (strcmp(argv[1], "-e") == 0) {
        extract = 1;
        if (argc >= 3) {
            toextract = strdup(argv[2]);
        } else {
            printf("File path for extraction required!\n");
            exitusage(argv[0]);
        }
    } else if (strcmp(argv[1], "-D") == 0) {
        // printf("Option -D is present\n");
        debug = 1;
    } else {
        exitusage(argv[0]);
    }

    const char* folder = "segments";
    // Create the folder
    if (access(folder, F_OK) != 0) 
    {
      if (mkdir(folder, 0777) != 0) {
          perror("Error creating folder");
          return 1;
      }
    }

  if ((fd = open(fsname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)) == -1)
  {
    perror("open failed");
    exit(EXIT_FAILURE);
  }
  else
  {
    if (zerosize(fd))
    {
      newfs = 1;
    }
    
    if (newfs)
    {
      if (lseek(fd, FSSIZE-1, SEEK_SET) == -1)
      {
        perror("seek failed");
        exit(EXIT_FAILURE);
      }
      else
      {
        if(write(fd, "\0", 1) == -1)
        {
          perror("write failed");
          exit(EXIT_FAILURE);
	      }
      }
    }
  }
  
  if (zerosize(fd))
    {
      newfs = 1;
    }

  
  if (newfs){
    formatfs();
  }

  loadfs();
  
  if (add){
    addfilefs(toadd, fpath);
  }

  if (remove){
    removefilefs(toremove);
  }

  if (extract){
    extractfilefs(toextract);
  }

  if(list){
    lsfs();
  }

if(debug){
    debugfs();
  }
  
  return 0;
}


int zerosize(int fd){
  struct stat stats;
  fstat(fd, &stats);
  if(stats.st_size == 0)
    return 1;
  return 0;
}

void exitusage(char* pname){
  fprintf(stderr, "Usage %s [-l] [-a path] [-e path] [-r path] -f name\n", pname);
  exit(EXIT_FAILURE);
}
