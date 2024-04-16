#include "fs.h"

unsigned char* fs;

void mapfs(int fd){
  if ((fs= mmap(NULL, FSSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == NULL){
      perror("mmap failed");
      exit(EXIT_FAILURE);
  }
}


void unmapfs(){
  munmap(fs, FSSIZE);
}

void print_manifest() {
    // Dynamically allocate memory for the manifest
    FILE *file = fopen("segments/manifest", "rb");
    if (!file) {
        perror("Error opening file");
        return;
    }
    Manifest *mf = (Manifest *)malloc(sizeof(Manifest));
    if (mf == NULL) {
        perror("Error allocating memory for manifest");
        return;
    }
    fread(mf, sizeof(Manifest), 1, file);
    printf("Manifest Contents:\n");
    printf("Number of Directory Segments: %d\n", mf->MH.numdirectorysegs);
    printf("Number of Data Segments: %d\n", mf->MH.numdatasegs);
    printf("Number of Inodes: %d\n", mf->MH.numinodes);
    printf("Number of Segments: %d\n", mf->MH.numsegs);
    printf("Entries\t\tType\tName\n");
    for (int i = 0; i < 1; ++i) 
    { // Loop through entries
        if (strlen(mf->entries[i].name) > 0) {
            printf("%d.\t\t", i + 1);
            printf("%s\t", mf->entries[i].type == DIR ? "DIR" : "DATA");
            printf("%s\t", mf->entries[i].name);
        } else {
            break; // Exit loop if name is empty
        }
    }
    fclose(file);
    // Free allocated memory
    free(mf);
}


void initialize_manifest()
{
    // Dynamically allocate memory for the manifest
    
    Manifest *mf = (Manifest *)malloc(sizeof(Manifest));
    if (mf == NULL) {
        perror("Error allocating memory for manifest");
        return;
    }
    mf->MH.numdirectorysegs = 1;
    mf->MH.numdatasegs = 0;
    mf->MH.numinodes = INODESPERSEG;
    mf->MH.numsegs = 1;
    char dir_name[255]; 
    sprintf(dir_name, "DIR_%d", mf->MH.numdirectorysegs);
    strcpy(mf->entries[0].name, dir_name);
    mf->entries[0].type = DIR;
    FILE *file = fopen("segments/manifest", "rb+");
    if (!file) {
        perror("Error opening file");
        free(mf); // Free allocated memory before returning
        return;
    }
    fseek(file, 0, SEEK_SET);
    fwrite(mf, sizeof(Manifest), 1, file);
    fclose(file);
    // memcpy(fs, &mf, sizeof(Manifest));
    free(mf);
}

void create_segment(char*seg_name, int type, int inode)
{
    char seg_path[256];
    snprintf(seg_path, sizeof(seg_path), "%s/%s", "segments", seg_name);
    Segment *seg= (Segment *)malloc(sizeof(Segment));
    if (seg== NULL) {
        perror("Error allocating memory for file system");
        return;
    }
    seg->type = type;
    seg->SB.blocksize = BLOCK_SIZE;
    seg->SB.segsize = SEGMENT_SIZE;
    seg->SB.inodesperseg = INODESPERSEG;
    seg->SB.blockid = 0; // Assuming block id starts from 0
    seg->SB.blocksperseg = BLOCKSPERSEG;
    memset(seg->FBL.bitmap, -1, sizeof(seg->FBL.bitmap));
     // Initialize inodes and blocks (if needed)
    for (int i = 0; i < INODESPERSEG; ++i) 
    {
        seg->inodes[i].type = type;
        seg->inodes[i].inuse = 0;
        for (int j = 0; j < (BLOCK_SIZE/4)-4; j++)
        {
            seg->inodes[i].blocks[j] = -1;
        }
    }
    if (inode != -1)
    {
        seg->inodes[inode].inuse = 1;
    }

    // Write the file system to file "EXFS"
    FILE *file = fopen(seg_path, "wb+"); 
    if (!file) {
        perror("Error opening file");
        free(seg); // Free allocated memory before returning
        return;
    }
    // Write the file system to file
    fseek(file, 0, SEEK_SET);
    fwrite(seg, sizeof(Segment), 1, file);
    // Close the file
    fclose(file);
    // Free allocated memoxry
    free(seg);
    printf("Segment '%s' created\n", seg_name);
}

void formatfs()
{   
    // Initialize the file system
    printf("Formatting the file system\n");
    initialize_manifest();
    print_manifest();
    create_segment("DIR_1", DIR, 0);
    print_manifest();
    // Segment *seg= (Segment *)malloc(sizeof(Segment));
    // if (seg== NULL) {
    //     perror("Error allocating memory for file system");
    //     return;
    // }

    // seg->type = DIR; // Directory segment type
    // seg->SB.blocksize = BLOCK_SIZE;
    // seg->SB.segsize = SEGMENT_SIZE;
    // seg->SB.inodesperseg = INODESPERSEG;
    // seg->SB.blockid = 0; // Assuming block id starts from 0
    // seg->SB.blocksperseg = BLOCKSPERSEG;
    // memset(seg->FBL.bitmap, -1, sizeof(seg->FBL.bitmap));
    //  // Initialize inodes and blocks (if needed)
    // for (int i = 0; i < INODESPERSEG; ++i) {
    //     seg->inodes[i].type = DIR;
    //     seg->inodes[i].inuse = 0;
    //     for (int j = 0; j < (BLOCK_SIZE/4)-4; j++)
    //     {
    //         seg->inodes[i].blocks[j] = -1;
    //     }
    // }
    // //For root directory
    // seg->inodes[0].inuse = 1;

    // // Write the file system to file "EXFS"
    // FILE *file = fopen("EXFS", "wb"); // Changed mode to "wb" for writing in binary mode
    // if (!file) {
    //     perror("Error opening file");
    //     free(seg); // Free allocated memory before returning
    //     return;
    // }
    // // Write the file system to file
    // fseek(file, 0, SEEK_SET);
    // fseek(file, sizeof(Manifest), SEEK_SET);
    // fwrite(seg, sizeof(Segment), 1, file);
    // // Close the file
    // fclose(file);
    // // Free allocated memoxry
    // free(seg);
    // print_manifest();
}



void loadfs()
{

}


void print_root_directory() 
{
    // Open the file system file
    FILE *file = fopen("segments/DIR_1", "rb+");
    if (!file) {
        perror("Error opening file system file");
        return;
    }
    printf("Root Directory:\n");
    // Read the file system from file
    Segment *seg= (Segment *)malloc(sizeof(Segment));
    fread(seg, sizeof(Segment), 1, file);

    // Close the file system file
    fclose(file);

    // Check if the root directory is valid
    if (seg->type != DIR) {
        printf("Error: Root directory not found\n");
        return;
    }

    // Iterate over root directory entries and print them
    printf("Name\t\tInode Number\tType\n");
    DirectoryEntry *entry = (DirectoryEntry *)malloc(sizeof(DirectoryEntry));
    for (int i = 0; i < ((BLOCK_SIZE/4)-4); i++) 
    {
        // printf("inode %d\n", seg->inodes[0].blocks[i]);
        if (seg->inodes[0].blocks[i] != -1) 
        {
            printf("Found entry in block %d\n", seg->inodes[0].blocks[i]);
            memcpy(entry, seg->blocks[seg->inodes[0].blocks[i]].data, sizeof(DirectoryEntry));
            printf("%s\t\t%d\t\t%s\n", entry->name, entry->inodenum, entry->type == DIR ? "Dir" : "Data");
        }
    }
    free(seg);
    free(entry);
}


void lsfs()
{   
    // print_root_directory();
    
}

int find_free_inode(Segment *fs) {
    for (int i = 0; i < INODESPERSEG; ++i)
     {
        if(fs->type == DATA)
        {
            continue;
        }
        if (!fs->inodes[i].inuse) {
            return i;
        }
    }
    return -1;
}



// void create_entry_dir(const char* name, int *dirinodenum) 
// {
//     printf("Creating entry in directory %s\n", name);
//     // if (dir.num_entries >= MAX_ENTRIES_PER_BLOCK) {
//     //     printf("Error: Directory is full\n");
//     //     return -1;
//     // }
    
//     FILE *file = fopen("EXFS", "rb+");
//     if (!file) {
//         perror("Error opening file system file");
//         return;
//     }
//     Manifest mf;
//     fread(&mf, sizeof(Manifest), 1, file);
//     Segment fs;
//     int i;
//     int j;
//     int new_inode = -1;
//     int found_unused_inode = 0;
//     for (i = 0; i < mf.MH.numsegs; i++)
//     {
//         fseek(file, 0, SEEK_SET);
//         fseek(file, sizeof(Manifest) + i * sizeof(Segment), SEEK_SET);
//         fread(&fs, sizeof(Segment), 1, file);
//         if(fs.type == DATA)
//         {
//             continue;
//         }
//         for( j = 0; j < INODESPERSEG; j++)
//         {
//             if (fs.inodes[j].inuse == 0)
//             {
//                 found_unused_inode = 1;
//                 break;
//             }
//         }
//         if(found_unused_inode)
//         {
//             break;
//         }
//     } 
//     if(!found_unused_inode)
//     {
//         new_inode = mf.MH.numsegs * 10 + 0;
//         Segment fs;
//         fs.type = DIR; // Directory segment type
//         fs.SB.blocksize = BLOCK_SIZE;
//         fs.SB.segsize = SEGMENT_SIZE;
//         fs.SB.inodesperseg = INODESPERSEG;
//         fs.SB.blockid = 0; // Assuming block id starts from 0
//         fs.SB.blocksperseg = BLOCKSPERSEG;
//         memset(fs.FBL.bitmap, 0, sizeof(fs.FBL.bitmap)); // All blocks are initially free

//         // Initialize inodes and blocks (if needed)
//         for (int i = 0; i < INODESPERSEG; ++i) {
//             fs.inodes[i].type = DIR;
//             fs.inodes[i].inuse = 0;
//         }
//             // Initialize directory entry for root directory
//         Directory new_entry;
//         strncpy(new_entry.name, name, sizeof(new_entry.name));
//         // new_entry.type = 1; // Assuming directory type
//         new_entry.inodenum = new_inode; // Assuming root inode number is 0
//         new_entry.inuse = 1; // Mark as in use
//         new_entry.num_entries = 0;
//         fs.inodes[0].inuse = 1;
//         fs.inodes[0].type = DIR;
//         fs.inodes[0].size = sizeof(Directory);
//         memcpy(fs.blocks[0].data, &new_entry, sizeof(Directory));

//         // Write the file system to file
//         fseek(file, sizeof(Manifest) + mf.MH.numsegs * sizeof(Segment), SEEK_SET);
//         mf.entries[mf.MH.numsegs].type = DIR;
//         mf.MH.numinodes += 10;
//         mf.MH.numsegs++;
//         mf.MH.numdirectorysegs++;
//         char dir_name[255]; 
//         sprintf(dir_name, "Dir_%d", mf.MH.numdirectorysegs);
//         strcpy(mf.entries[0].name, dir_name);
//         fwrite(&mf, sizeof(Manifest), 1, file);
//         fwrite(&fs, sizeof(Segment), 1, file);
//         // Close the file
//         fclose(file);

//     }
//     else{
//         printf("Found unused inode at segment %d, inode %d\n", i, j);
//         new_inode = i * 10 + j;
//         Directory entry;
//         strncpy(entry.name, name, 255);
//         entry.inodenum = new_inode;
//         entry.inuse = 1;
//         entry.num_entries = 0;
//         fs.inodes[j].inuse = 1;
//         fs.inodes[j].type = DIR;
//         fs.inodes[j].size = sizeof(Directory);
//         int x = j * sizeof(Directory);
//         //freeblockslistlookup
//         memcpy(fs.blocks[x].data, &entry, sizeof(DirectoryEntry));
//         fclose(file);
//     }
//         //new Entry in the directory
//         FILE *file2 = fopen("EXFS", "rb+");
//         if (!file2) {
//             perror("Error opening file2 system file2");
//             return;
//         }

//         //Searching segment using inode number
//         Segment old_fs;
//         int segno = *dirinodenum / 10;
//         fseek(file2, sizeof(Manifest) + segno * sizeof(Segment), SEEK_SET);
//         fread(&old_fs, sizeof(Segment), 1, file2);
//         int k = *dirinodenum % 10;
//         Directory entries;
//         // Calculate the byte offset of the i-th directory entry within the block
//         size_t entry_offset = k * sizeof(Directory);

//         // Obtain a pointer to the i-th directory entry within the block
//         Directory *entry_ptr = (Directory *)(old_fs.blocks[0].data + entry_offset);

//         // Now you can access the i-th directory entry using entry_ptr
//         // For example, you can access the name of the entry like this:
//         printf("Name of the i-th directory entry: %s\n", entry_ptr->name);
//         // memcpy(&entries, old_fs.blocks[0].data , sizeof(Directory) * i);
//         DirectoryEntry new_entry2;
//         strncpy(new_entry2.name, name, sizeof(new_entry2.name));
//         new_entry2.type = DIR; // Assuming directory type is provided
//         new_entry2.inodenum = (mf.MH.numsegs - 1) * 10 + 0; 
//         new_entry2.inuse = 1;
//         entry_ptr->entries[entry_ptr->num_entries] = new_entry2;
//         entry_ptr->num_entries++;
//         // Write the file system to file
//         fseek(file2, sizeof(Manifest) + segno * sizeof(Segment), SEEK_SET);
//         fwrite(&old_fs, sizeof(Segment), 1, file2);
//         // Close the file
//         fclose(file2);
//         printf("Directory '%s' added to the root directory\n", name);
//         *dirinodenum = new_inode;
// }

int create_entry(const char* name, int old_inode)
{

   FILE *file = fopen("EXFS", "rb+");
    if (!file) 
    {
        perror("Error opening file system file");
        return -1;
    }
    printf("Creating entry in directory of %s inode %d\n", name, old_inode);
    Segment *seg= (Segment *)malloc(sizeof(Segment));
    Manifest *mf = (Manifest *)malloc(sizeof(Manifest));
    fread(mf, sizeof(Manifest), 1, file);
    printf("Number of segments: %d\n", mf->MH.numsegs);
    int i;
    int j;
    int new_inode = -1;
    int found_unused_inode = 0;
    int segment_no = old_inode / 10;
    int inode_index = old_inode % 10;
    for (i = 0; i < mf->MH.numsegs; i++)
    {
        printf("Segment %d\n", i);
        fseek(file, 0, SEEK_SET);
        fseek(file, sizeof(Manifest) + i * sizeof(Segment), SEEK_SET);
        fread(seg, sizeof(Segment), 1, file);
        if(seg->type == DATA)
        {
            continue;
        }
        for( j = 0; j < INODESPERSEG; j++)
        {
            printf("inode use: %d\n", seg->inodes[j].inuse);
            if (seg->inodes[j].inuse == 0)
            {
                found_unused_inode = 1;
                break;
            }
        }
        if(found_unused_inode)
        {
            break;
        }
    } 
    if(found_unused_inode)
    {
        printf("Found unused inode at segment %d, inode %d\n", i, j);
        new_inode = i * 10 + j;
        seg->inodes[j].inuse = 1;
        seg->inodes[j].type = DIR;
        seg->inodes[j].size = sizeof(Directory);
        fseek(file, 0, SEEK_SET);
        fseek(file, sizeof(Manifest) + i * sizeof(Segment), SEEK_SET);
        fwrite(seg, sizeof(Segment), 1, file);

        //adding an entry in the directory
        DirectoryEntry *new_entry = (DirectoryEntry *)malloc(sizeof(DirectoryEntry));
        strncpy(new_entry->name, name, sizeof(new_entry->name));
        new_entry->type = DIR; 
        new_entry->inodenum = new_inode; 
        new_entry->inuse = 1;
        seg->inodes[inode_index].inuse = 1;
        seg->inodes[inode_index].type = DIR;
        seg->inodes[inode_index].size = sizeof(DirectoryEntry);

        //Finding free block and marking it as used
        int k;
        for (k = 0; k < ((SEGMENT_SIZE/BLOCK_SIZE)/8); k++)
        {
            if(seg->FBL.bitmap[k] != -1)
            {
                seg->FBL.bitmap[k] = 0;
                break;
            }
        }

        for (int x = 0; x < (BLOCK_SIZE/4)-4; x++)
        {
            if(seg->inodes[inode_index].blocks[x] == -1)
            {
                seg->inodes[inode_index].blocks[x] = k;
                break;
            }
        }
        // seg->inodes[inode_index].blocks[0] = k;
        memccpy(seg->blocks[k].data, new_entry, sizeof(DirectoryEntry), 1);
        fseek(file, 0, SEEK_SET);
        fseek(file, sizeof(Manifest) + segment_no * sizeof(Segment), SEEK_SET);
        fwrite(seg, sizeof(Segment), 1, file);
        free(new_entry);
    }
    else
    {
        printf("No free inode found\n");
    }
    fclose(file);
    free(seg);
    free(mf);
    return new_inode;
}

char** parse_path(const char* path, int* num_elements) {
    char* token;
    char* path_copy = strdup(path);  // Create a copy of the path to avoid modifying the original string

    // Initialize the number of elements to 0
    *num_elements = 0;

    // Allocate memory for the array of elements
    char** elements = (char**)malloc(20 * sizeof(char*));

    // Get the first token
    token = strtok(path_copy, "/");
    
    // Keep retrieving tokens until no more tokens are available
    while (token != NULL) {
        // Store the token in the elements array
        elements[*num_elements] = strdup(token);
        
        // Increment the number of elements
        (*num_elements)++;
        
        // Get the next token
        token = strtok(NULL, "/");
    }

    // Free the memory allocated for the copy of the path
    free(path_copy);

    return elements;
}

void add_entry_to_root_directory(const char* name, int inodenum) 
{
    // Open the file system file
    FILE *file = fopen("EXFS", "rb+");
    if (!file) 
    {
        perror("Error opening file system file");
        return;
    }

    // Read the file system from file
    Segment fs;
    fseek(file, sizeof(Manifest), SEEK_SET);
    fread(&fs, sizeof(Segment), 1, file);


    // Check if the root directory is valid

    if (fs.type != DIR) {
        printf("Error: Root directory not found\n");
        return;
    }

    Directory root_dir;
    memcpy(&root_dir, fs.blocks[0].data, sizeof(Directory));

    // Check if the root directory is full
    if (root_dir.num_entries >= MAX_ENTRIES_PER_BLOCK) {
        printf("Error: Root directory is full\n");
        return;
    }

    // Create a new directory entry
    DirectoryEntry new_entry;
    strncpy(new_entry.name, name, sizeof(new_entry.name));
    new_entry.type = DIR; // Assuming directory type
    new_entry.inodenum = inodenum; // Assuming inode number for the file
    new_entry.inuse = 1; // Mark as in use

    // Add the new entry to the root directory
    root_dir.entries[root_dir.num_entries] = new_entry;
    root_dir.num_entries++;
    
    // Write the updated root directory back to the file system
    memcpy(fs.blocks[0].data, &root_dir, sizeof(Directory));
    fseek(file, sizeof(Manifest), SEEK_SET);
    fwrite(&fs, sizeof(Segment), 1, file);

    // Close the file system file
    fclose(file);
    printf("Entry '%s' added to the root directory\n", name);
  
}

int search_inode_in_directory_entry(int current_inode, char* name)
{
    Segment *seg= (Segment *)malloc(sizeof(Segment));
    int segment_no = current_inode / 10;
    int inode_index = current_inode % 10;
    char filepath[255];
    snprintf(filepath, sizeof(filepath), "DIR_%d", segment_no);
    FILE *file = fopen(filepath, "rb+");
    if (!file) {
        perror("Error opening file system file");
        return -1;
    }
    fseek(file, 0, SEEK_SET);
    fread(seg, sizeof(Segment), 1, file);
    fclose(file);
    int new_inode = -1;

    inode *ind = (inode *)malloc(sizeof(inode));
    *ind = seg->inodes[inode_index];

    for (int i; i < (BLOCK_SIZE/4)-4; i++)
    {
        if(ind->blocks[i] != -1)
        {
            DirectoryEntry *entry_ptr = (DirectoryEntry *)malloc(sizeof(DirectoryEntry));
            memcpy(entry_ptr, &(seg->blocks[ind->blocks[i]]), sizeof(DirectoryEntry));
            if(strcmp(entry_ptr->name, name) == 0)
            {
                new_inode = entry_ptr->inodenum;
                break;
            }
        }
    }
    free(ind);
    free(seg);
    return new_inode;
}

void addfilefs(char* fspath, char *fpath) 
{
    int num_elements;
    char** dir_name = parse_path(fspath, &num_elements);

    // printf("Number of elements: %d\n", num_elements);
    // printf("Path elements:\n");
    // for (int i = 0; i < num_elements; i++) {
    //     printf("%d: %s\n", i, dir_name[i]);
    //     // add_entry_to_root_directory(dir_name[i], i+1);
    // }

    int current_inode = 0;
    int new_inode = -1;
    FILE *file = fopen("segments/DIR_1", "rb+");
    if (!file) {
        perror("Error opening file system file");
        return;
    }

    fclose(file);
    Segment *seg= (Segment *)malloc(sizeof(Segment));
    // Directory entry;
    fread(&seg, sizeof(Segment), 1, file);

    // Directory current_dir;

    // memcpy(&current_dir, fs->blocks[0].data , sizeof(Directory));
    for(int l = 0; l < num_elements; l++)
    {
        new_inode = search_inode_in_directory_entry(current_inode, dir_name[l]);
        if (new_inode == -1) 
        {
            printf("Directory '%s' not found\n", dir_name[l]);
            new_inode = create_entry(dir_name[l], current_inode);
        }
        current_inode = new_inode;
        
    }

    free(seg);
    free(dir_name);
}
  





void removefilefs(char* fname)
{

}


void extractfilefs(char* fname)
{}

