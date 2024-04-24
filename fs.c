#include "fs.h"

unsigned char* fs;

// void mapfs(int fd){
//   if ((fs= mmap(NULL, FSSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == NULL){
//       perror("mmap failed");
//       exit(EXIT_FAILURE);
//   }
// }



// void unmapfs(){
//   munmap(fs, FSSIZE);
// }

void print_manifest() 
{
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
    for (int i = 0; i < MAX_ENTRIES; ++i) 
    { 
        if (strlen(mf->entries[i].name) > 0) {
            printf("%d.\t\t", i + 1);
            printf("%s\t", mf->entries[i].type == DIR ? "DIR" : "DATA");
            printf("%s\n", mf->entries[i].name);
        } else {
            break; 
        }
    }
    fclose(file);
    // Free allocated memory
    free(mf);
}


char * initialize_manifest()
{
    Manifest *mf = (Manifest *)malloc(sizeof(Manifest));
    if (mf == NULL) {
        perror("Error allocating memory for manifest");
        return NULL;
    }
    // Initialize the directory segment for root directory
    mf->MH.numdirectorysegs = 1;
    mf->MH.numdatasegs = 0;
    mf->MH.numinodes = INODESPERSEG;
    mf->MH.numsegs = 1;

    char* dir_name = (char *)malloc(255);
    sprintf(dir_name, "DIR_%d", mf->MH.numdirectorysegs);
    strcpy(mf->entries[0].name, dir_name);
    mf->entries[0].type = DIR;
    FILE *file = fopen("segments/manifest", "rb+");
    if (!file) {
        perror("Error opening file");
        free(mf); \
        return NULL;
    }
    fseek(file, 0, SEEK_SET);
    fwrite(mf, sizeof(Manifest), 1, file);
    fclose(file);
    free(mf);
    return dir_name;
}


// Update the manifest file for the new segment type
// Return new segment name
char* update_manifest(int type) 
{
    char* seg_name = (char*)malloc(255);
    Manifest* mf = (Manifest*)malloc(sizeof(Manifest));
    FILE* manifest = fopen("segments/manifest", "rb+");
    if (!manifest) {
        perror("Error opening file system file");
        return NULL;
    }
    fread(mf, sizeof(Manifest), 1, manifest);
    mf->MH.numsegs++;
    // printf("Updated no. of segments: %d\n", mf->MH.numsegs );
    if (type == DIR) {
        mf->MH.numdirectorysegs++;
        seg_name = (char*)malloc(255);
        sprintf(seg_name, "DIR_%d", mf->MH.numdirectorysegs);
    } else {
        mf->MH.numdatasegs++;
        seg_name = (char*)malloc(255);
        sprintf(seg_name, "DATA_%d", mf->MH.numdatasegs);
    }

    mf->MH.numinodes += INODESPERSEG;
    mf->entries[mf->MH.numsegs - 1].type = type;
    // printf("Segment name to be added: %s of type: %d\n", seg_name, type);
    strcpy(mf->entries[mf->MH.numsegs - 1].name, seg_name);
    fseek(manifest, 0, SEEK_SET);
    fwrite(mf, sizeof(Manifest), 1, manifest);
    fclose(manifest);
    free(mf);
    return seg_name;
}

// Create a new segment of the specified type 
// Initialize the 0th inode based on the num value as data segment does not need inodes if blocks were only needed 
void create_segment(int type, int num)
{
    char seg_path[256];
    char*seg_name = (char*) malloc(255);

    if(num == 0)
    {
        printf("Initializing Segment\n");
        seg_name = initialize_manifest();
    }
    else
    {
        // printf("Updating manifest\n");
        seg_name = update_manifest(type);
    }
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
    seg->SB.blockid = 0; 
    seg->SB.blocksperseg = BLOCKSPERSEG;
    memset(seg->FBL.bitmap, -1, sizeof(seg->FBL.bitmap));
     // Initialize inodes and blocks 
    for (int i = 0; i < INODESPERSEG; ++i) 
    {
        seg->inodes[i].type = type;
        seg->inodes[i].inuse = 0;
        for (int j = 0; j < BLOCKSPERSEG; j++)
        {
            seg->inodes[i].blocks[j] = -1;
        }
    }

    //For not setting node for files that only need blocks
    if (num != -1)
    {
        seg->inodes[0].inuse = 1;
    }

    FILE *file = fopen(seg_path, "wb+"); 
    if (!file) {
        perror("Error opening file");
        free(seg); 
        return;
    }

    fseek(file, 0, SEEK_SET);
    fwrite(seg, sizeof(Segment), 1, file);
    fclose(file);
    free(seg);
    printf("Segment '%s' created\n", seg_name);
    free(seg_name);
}

void formatfs()
{   
    //Creating directory segment for root directory (inode 0)
    create_segment(DIR, 0);
}



void loadfs()
{

}

//get the segment name from the manifest file based on id
char * get_segname(int seg_id)
{
    Manifest *mf = (Manifest *)malloc(sizeof(Manifest));
    FILE *manifest = fopen("segments/manifest", "rb+");
    if (!manifest) {
        perror("Error opening file system file");
        return NULL;
    }
    fread(mf, sizeof(Manifest), 1, manifest);
    fclose(manifest);
    return mf->entries[seg_id].name;
}

//get the segment id from the manifest file based on segment name
int get_seg_id(char* seg_name)
{
    Manifest *mf = (Manifest *)malloc(sizeof(Manifest));
    FILE *manifest = fopen("segments/manifest", "rb+");
    if (!manifest) {
        perror("Error opening file system file");
        return -1;
    }
    fread(mf, sizeof(Manifest), 1, manifest);
    fclose(manifest);
    for (int i = 0; i < mf->MH.numsegs; i++)
    {
        if(strcmp(mf->entries[i].name, seg_name) == 0)
        {
            return i;
        }
    }
    return -1;
}

void print_directory_entries(int dir_inode, int indent)
{
    // printf("Printing directory entries\n");
    Segment *seg= (Segment *)malloc(sizeof(Segment));
    int segment_id = dir_inode / 10;
    int inode_index = dir_inode % 10;
    char filepath[255];
    char* seg_name = (char *)malloc(255);
    seg_name = get_segname(segment_id);
    snprintf(filepath, sizeof(filepath), "segments/%s", seg_name);
    // printf("Segment name: %s\n", filepath);
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        perror("Error opening file system file");
        return;
    }
    fseek(file, 0, SEEK_SET);
    fread(seg, sizeof(Segment), 1, file);
    fclose(file);
    inode *ind = (inode *)malloc(sizeof(inode));
    *ind = seg->inodes[inode_index];
    // printf("For Inode: %d\n", dir_inode);
    for (int i = 0; i < BLOCKSPERSEG; i++)
    {
        // printf("Block vaues = %d\n", ind->blocks[i]);
        if(ind->blocks[i] != -1)
        {
            // printf("Block %d\n", ind->blocks[i]);
            DirectoryEntry *entry_ptr = (DirectoryEntry *)malloc(sizeof(DirectoryEntry));
            memcpy(entry_ptr, &(seg->blocks[ind->blocks[i]]), sizeof(DirectoryEntry));
            // printf("Entry name: %s inuse = %d, inode = %d \n", entry_ptr->name, entry_ptr->inuse, entry_ptr->inodenum);
            if(entry_ptr -> inuse == 1)
            {
                for (int j = 0; j < indent; j++)
                {
                    printf("-");
                }
                if(entry_ptr->inuse == 1)
                {
                printf("| ");
                printf("%s\t", entry_ptr->name);
                printf("%d\n", entry_ptr->inodenum);
                // printf("%s\n", entry_ptr->type == DIR ? "Dir" : "Data");
                // printf("%d\n", entry_ptr->inuse);
                if(entry_ptr->type == DIR)
                {
                    print_directory_entries(entry_ptr->inodenum, indent + 1);
                }
                }
            }
        }
    }
    free(ind);
    free(seg);
}

//Print all entries in the hash table
void print_hash_table()
{
    hash_table *ht = (hash_table *)malloc(sizeof(hash_table));
    FILE *file = fopen("segments/hash_table", "rb");
    if (!file) {
        perror("Error opening file system file");
        return;
    }
    fread(ht, sizeof(hash_table), 1, file);
    fclose(file);
    printf("Hash Table Contents:\n");
    printf("Block ID\tReferenced\tHash\n");
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (ht->entries[i].block_id != -1) {
            printf("%d\t\t", ht->entries[i].block_id);
            printf("%d\t\t", ht->entries[i].referenced);
            for (int j = 0; j < HASH_SIZE; j++) {
                printf("%02x", ht->entries[i].hash[j]);
            }
            printf("\n");
        }
    }
    // free(ht);

}

void lsfs()
{       
    print_directory_entries(0, 0);
    // print_manifest();
    // print_hash_table();
}

int extract_number(const char* str) 
{
    // Find the position of the underscore character
    const char* underscore_pos = strchr(str, '_');
    if (!underscore_pos) 
    {
        return -1;
    }

    // Convert the substring after the underscore to an integer
    int number = atoi(underscore_pos + 1);
    return number;
}

//Mark the inode as used or unused for the given inode number
void mark_inode(int inode_num, int use)
{
    Segment *seg= (Segment *)malloc(sizeof(Segment));
    int segment_id = inode_num / 10;
    int inode_index = inode_num % 10;
    char filepath[255];
    char* seg_name = (char *)malloc(255);
    seg_name = get_segname(segment_id);
    snprintf(filepath, sizeof(filepath), "segments/%s", seg_name);
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        perror("Error opening file system file");
        return;
    }
    fseek(file, 0, SEEK_SET);
    fread(seg, sizeof(Segment), 1, file);
    fclose(file);
    seg->inodes[inode_index].inuse = use;
    FILE *file2 = fopen(filepath, "rb+");
    if (!file2) {
        perror("Error opening file system file");
        return;
    }
    fwrite(seg, sizeof(Segment), 1, file2);
    fclose(file2);
    free(seg);
    // printf("Inode %d marked as %d\n", inode_num, use);
}

//Create Directory inside a directory and return new_inode
int create_directory(const char* name, int old_inode)
{

   FILE *manifest = fopen("segments/manifest", "rb+");
    if (!manifest) 
    {
        perror("Error opening file system file");
        return -1;
    }

    Segment *seg= (Segment *)malloc(sizeof(Segment));
    Manifest *mf = (Manifest *)malloc(sizeof(Manifest));
    char* path = (char *)malloc(1000);
    fread(mf, sizeof(Manifest), 1, manifest);
    // printf("Number of segments: %d\n", mf->MH.numsegs);
    fclose(manifest);
    int i;
    int j;
    int new_inode = -1;
    int found_unused_inode = 0;
    int segment_id = old_inode / 10;
    int inode_index = old_inode % 10;
    char* seg_name = (char *)malloc(255);
    for (i = 0; i < mf->MH.numsegs; i++)
    {
        // printf("Segment %d\n", i);
        seg_name = get_segname(i);
        char* path2 = (char *)malloc(255);
        sprintf(path2, "segments/%s", seg_name);
        // printf("Segment name: %s\n", path2);
        FILE *f = fopen(path2, "rb");
        if (!f) 
        {
            perror("Error opening DIR__ file");
            return -1;
        }
        fread(seg, sizeof(Segment), 1, f);
        if(seg->type == DATA)
        {
            continue;
        }
        for( j = 0; j < INODESPERSEG; j++)
        {
            // printf("inode %d in use  with value = %d\n ", j, seg->inodes[j].inuse); 
            if (seg->inodes[j].inuse == 0)
            {
                // printf("Found unused inode at segment %d, inode %d\n", i, j);
                found_unused_inode = 1;
                break;
            }
        }
        fclose(f);
        if(found_unused_inode)
        {
            break;
        }
    } 
    char* seg_nam2 = (char *)malloc(255);

    if(found_unused_inode)
    {
        new_inode = i * 10 + j;
        mark_inode(new_inode, 1);

    }
    else
    {
        create_segment(DIR, 1);
        manifest = fopen("segments/manifest", "rb+");
        fread(mf, sizeof(Manifest), 1, manifest);
        fclose(manifest);
        // printf("Num of segments: %d\n", mf->MH.numsegs);
        int seg_id2 = mf->MH.numsegs - 1;
        seg_nam2 = get_segname(seg_id2);
        new_inode = (mf->MH.numsegs - 1) * 10;
        i = mf->MH.numsegs - 1;
        j = 0;  
        // printf("i = %d, j = %d\n", i, j);          
    }
        char seg_name2[255];
        char path_2[255];
        seg_nam2 = get_segname(segment_id);
        sprintf(seg_name2, "segments/%s", seg_nam2);
        // printf("Opeing Segment name: %s for entry.\n", seg_name2);
        FILE *file2 = fopen(seg_name2, "rb+");
        if (!file2) 
        {
            perror("Error opening DIR_ file");
            return -1;
        }
        fread(seg, sizeof(Segment), 1, file2);
        fclose(file2);
        //adding an entry in the directory in the parent directory
        DirectoryEntry *new_entry = (DirectoryEntry *)malloc(sizeof(DirectoryEntry));
        strncpy(new_entry->name, name, sizeof(new_entry->name));
        new_entry->type = DIR; 
        new_entry->inodenum = new_inode; 
        new_entry->inuse = 1;

        //Finding free block and marking it as used
        int found_free_block = 0;
        int k;
        for (k = 0; k < BLOCKSPERSEG; k++)
        {
            if(seg->FBL.bitmap[k] == -1)
            {
                // printf("Found free block at %d\n", k);
                seg->FBL.bitmap[k] = 0;
                found_free_block = 1;
                break;
            }
        }

        //FOR MAX BLOCK EXCEEDING for directory
        if(!found_free_block)
        {
            printf("No free block found\n");
            return -1;
        }


        //Flagging the block used for that inode
        int x =0;
        for (int x = 0; x < BLOCKSPERSEG; x++)
        {
            if(seg->inodes[inode_index].blocks[x] == -1)
            {
                seg->inodes[inode_index].blocks[x] = k;
                break;
            }
        }
        seg->inodes[inode_index].size = x * sizeof(DirectoryEntry);
        memcpy(seg->blocks[k].data, new_entry, sizeof(DirectoryEntry));
        
        char *seg_name3 = get_segname(segment_id);
        sprintf(path, "segments/%s", seg_name3);
        // printf("Saving directory entry to segment name: %s\n", path);
        FILE *file3 = fopen(path, "rb+");
        if (!file3) 
        {
            perror("Error opening DIR_ file");
            return -1;
        }
        fwrite(seg, sizeof(Segment), 1, file3);
        free(new_entry);
        fclose(file3);
    
    free(seg);
    free(mf);
    return new_inode;
}

//Get the elements of the path
char** parse_path(const char* path, int* num_elements) 
{
    char* token = (char *)malloc(255);
    char* path_copy = (char *)malloc(255);
    path_copy = strdup(path);  
    *num_elements = 0;
    char** elements = (char**)malloc(20 * sizeof(char*));

    // Get the first token
    token = strtok(path_copy, "/");
    
    while (token != NULL)
    {
        // Store the token in the elements array
        elements[*num_elements] = strdup(token);
        (*num_elements)++;
        token = strtok(NULL, "/");
    }
    free(path_copy);
    return elements;
}


//Loop over Direcotry Entries to find inode number
int search_inode_in_directory_entry(int current_inode, char* name)
{
    // printf("Searching in inode %d in for %s \n", current_inode, name);
    Segment *seg= (Segment *)malloc(sizeof(Segment));
    int segment_no = current_inode / 10;
    int inode_index = current_inode % 10;
    char filepath[255];
    char* seg_name = (char *)malloc(255);
    seg_name = get_segname(segment_no);
    snprintf(filepath, sizeof(filepath), "segments/%s", seg_name);
    FILE *file = fopen(filepath, "rb");
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

    for (int i = 0; i < BLOCKSPERSEG; i++)
    {
        if(ind->blocks[i] != -1)
        {
            DirectoryEntry *entry_ptr = (DirectoryEntry *)malloc(sizeof(DirectoryEntry));
            memcpy(entry_ptr, &(seg->blocks[ind->blocks[i]]), sizeof(DirectoryEntry));
            if(strcmp(entry_ptr->name, name) == 0)
            {
                if (entry_ptr->inuse == 1)
                {
                    // printf("%s found in inode %d\n", name, current_inode);
                    new_inode = entry_ptr->inodenum;
                    break;
                }
            }
        }
    }
    // printf("%s not found in inode %d\n", name, current_inode);
    free(ind);
    free(seg);
    return new_inode;
}

//Initializing hash table to implement Venti
void initialize_hash_table()
{
    hash_table *ht = (hash_table *)malloc(sizeof(hash_table));
    for (int i = 0; i < HASH_TABLE_SIZE; i++)
    {
        ht->entries[i].block_id = -1;
        ht->entries[i].referenced = -1;
    }
    FILE *file = fopen("segments/hash_table", "wb+");
    if (!file) {
        perror("Error opening file system file");
        return;
    }
    fwrite(ht, sizeof(hash_table), 1, file);
    fclose(file);
    // free(ht);
}

//Looking for unused inode in the segment
int find_unused_inode(int type)
{
    Segment *seg= (Segment *)malloc(sizeof(Segment));
    Manifest *mf = (Manifest *)malloc(sizeof(Manifest));
    FILE *manifest = fopen("segments/manifest", "rb+");
    if (!manifest) {
        perror("Error opening file system file");
        return -1;
    }
    fread(mf, sizeof(Manifest), 1, manifest);
    fclose(manifest);
    if(type == DATA && mf->MH.numdatasegs == 0)
    {
        create_segment(DATA, -1);
        initialize_hash_table();
        ///
        FILE *manifest2 = fopen("segments/manifest", "rb+");
        if (!manifest2) {
            perror("Error opening file system file");
            return -1;
        }
        fread(mf, sizeof(Manifest), 1, manifest2);
        fclose(manifest2);
        ///
    }

    int i;
    int j;
    int new_inode = -1;
    int found_unused_inode = 0;
    char* seg_name = (char *)malloc(255);

    for (i = 0; i < mf->MH.numsegs; i++)
    {
        seg_name = get_segname(i);
        char* path = (char *)malloc(255);
        sprintf(path, "segments/%s", seg_name);
        FILE *file = fopen(path, "rb");
        if (!file) 
        {
            perror("Error opening DIR__ file");
            return -1;
        }
        fread(seg, sizeof(Segment), 1, file);
        if(seg->type != type)
        {
            continue;
        }
        for( j = 0; j < INODESPERSEG; j++)
        {
            if (seg->inodes[j].inuse == 0)
            {
                // printf("Found unused inode at segment %d, inode %d\n", i, j);
                found_unused_inode = 1;
                break;
            }
        }
        fclose(file);
        if(found_unused_inode)
        {
            break;
        }
    } 
    if(found_unused_inode)
    {
        new_inode = i * 10 + j;
        return new_inode;
    }
    return -1;
}
    
//Adding directory into the parent directory inode
void add_directory_entry_in_parent(int parent_inode, int child_inode, char* name, int type)
{
    Segment *seg= (Segment *)malloc(sizeof(Segment));

    int segment_id = parent_inode / 10;
    int inode_index = parent_inode % 10;

    char filepath[255];

    char* seg_name = (char *)malloc(255);
    seg_name = get_segname(segment_id);
    snprintf(filepath, sizeof(filepath), "segments/%s", seg_name);

    FILE *file = fopen(filepath, "rb");
    if (!file) {
        perror("Error opening file system file");
        return;
    }
    fseek(file, 0, SEEK_SET);
    fread(seg, sizeof(Segment), 1, file);
    fclose(file);

    inode *ind = (inode *)malloc(sizeof(inode));
    *ind = seg->inodes[inode_index];

    //Constructing Directory Entry
    DirectoryEntry *new_entry = (DirectoryEntry *)malloc(sizeof(DirectoryEntry));
    strncpy(new_entry->name, name, sizeof(new_entry->name));
    new_entry->type = type; 
    new_entry->inodenum = child_inode; 
    new_entry->inuse = 1;

    //Finding free block and marking it as used
    int found_free_block = 0;
    int k;
    for (k = 0; k < BLOCKSPERSEG; k++)
    {
        if(seg->FBL.bitmap[k] == -1)
        {
            // printf("Found free block at %d\n", k);
            seg->FBL.bitmap[k] = 0;
            found_free_block = 1;
            break;
        }
    }
    //FOR MAX BLOCK EXCEEDING
    if(!found_free_block)
    {
        printf("No free block found\n");
        return;
    }

    //Flagging the block used for that inode
    int x = 0;
    for (int x = 0; x < BLOCKSPERSEG; x++)
    {
        if(seg->inodes[inode_index].blocks[x] == -1)
        {
            seg->inodes[inode_index].blocks[x] = k;
            break;
        }
    }

    seg->inodes[inode_index].size = x * sizeof(DirectoryEntry);
    // printf("\n");
    // printf("inode for new entry: %d\n", new_entry->inodenum);
    // seg->inodes[inode_index].blocks[0] = k;
    memcpy(seg->blocks[k].data, new_entry, sizeof(DirectoryEntry));

    char segname3[255];
    char filepath3[255];
    strcpy(segname3, get_segname(segment_id));
    sprintf(filepath3, "segments/%s", segname3);
    // printf("Saving directory entry to segment name: %s\n", filepath3);

    FILE *file3 = fopen(filepath3, "rb+");
    if (!file3) 
    {
        perror("Error opening DIR_ file");
        return ;
    }
    fwrite(seg, sizeof(Segment), 1, file3);

    fclose(file3);
    free(new_entry);
    free(ind);
    free(seg);

}

//Get empty blocks for the given number of blocks and type
void get_empty_blocks(int num_blocks, int* blocks, int type)
{
    Segment *seg= (Segment *)malloc(sizeof(Segment));
    Manifest *mf = (Manifest *)malloc(sizeof(Manifest));
    FILE *manifest = fopen("segments/manifest", "rb+");
    if (!manifest) {
        perror("Error opening file system file");
        return;
    }
    fread(mf, sizeof(Manifest), 1, manifest);
    fclose(manifest);

    int i;
    int j;
    char* seg_name = (char *)malloc(255);

    freeblocklist *fbl = (freeblocklist *)malloc(sizeof(freeblocklist));
    int count = 0;
    for (i = 0; i < mf->MH.numsegs; i++)
    {
        seg_name = get_segname(i);
        char* path = (char *)malloc(255);
        sprintf(path, "segments/%s", seg_name);
        FILE *file = fopen(path, "rb");
        if (!file) 
        {
            perror("Error opening DIR_ file");
            return;
        }
        fread(seg, sizeof(Segment), 1, file);
        fbl = &seg->FBL;
        fclose(file);

        if(seg->type != type)
        {
            continue;
        }
        for( j = 0; j < BLOCKSPERSEG; j++)
        {
            if (fbl->bitmap[j] == -1)
            {
                blocks[count] = i * 10000 + j;
                // printf("Found free block at segment %d, block %d with block index %d\n", i, j, blocks[count-1]);
                count++;
                if(count == num_blocks)
                {
                    return;
                }
            }
        }

    }
    if(count < num_blocks)
    {
    create_segment(type, -1);
    FILE *manifest2 = fopen("segments/manifest", "rb+");
    if (!manifest2) {
        perror("Error opening file system file");
        return;
    }
    fread(mf, sizeof(Manifest), 1, manifest2);
    fclose(manifest2);
    int seg_id2 = mf->MH.numsegs - 1;
    int remaining_blocks = num_blocks - count;
    int found_all_blocks = 0;
    while (!found_all_blocks)
    {
        for(int i = 0; i < BLOCKSPERSEG; i++)
        {
            blocks[count] = seg_id2 * 10000 + i;
            count++;
            if (count == num_blocks)
            {
                found_all_blocks = 1;
                break;
            }
        }
        if (count == num_blocks)
        {
            found_all_blocks = 1;
        }
        else
        {
            create_segment(type, -1); //Dont initialze inodes for data segment
            FILE *manifest3 = fopen("segments/manifest", "rb+");
            if (!manifest3) {
                perror("Error opening file system file");
                return;
            }
            fread(mf, sizeof(Manifest), 1, manifest3);
            fclose(manifest3);
            seg_id2 = mf->MH.numsegs - 1;
        }
    }
    
    }
}


//Write the buffer to the block
void write_file_to_block(char* buffer, int block, int bytes_to_read) 
{
    // printf("Writing to block %d\n", block);
    Segment *seg = (Segment *)malloc(sizeof(Segment));
    int segment_id = block / 10000;
    int block_index = block % 10000;
    // printf("Segment id: %d, Block index: %d\n", segment_id, block_index);
    char filepath[255];
    char* seg_name = (char *)malloc(255);
    seg_name = get_segname(segment_id); 
    // printf("Segment name: %s\n", seg_name);
    snprintf(filepath, sizeof(filepath), "segments/%s", seg_name);
    // free(seg_name); // Free allocated memory for seg_name

    FILE *file = fopen(filepath, "rb+");
    if (!file) {
        perror("Error opening file system file");
        // free(seg); 
        return;
    }
    fseek(file, 0, SEEK_SET);
    fread(seg, sizeof(Segment), 1, file);
    fclose(file);

    memcpy(seg->blocks[block_index].data, buffer, bytes_to_read);
    seg->FBL.bitmap[block_index] = 0;

    FILE *file2 = fopen(filepath, "rb+");
    if (!file2) {
        perror("Error opening file system file");
        // free(seg); 
        return;
    }
    fseek(file2, 0, SEEK_SET);
    fwrite(seg, sizeof(Segment), 1, file2);
    fclose(file2);

    // free(seg); 
}

/// VENTI ///

// Venti function to check if the hash is already present in the hash table
int venti(unsigned char* buffer_hash, int bytes_to_read, int* block) 
{
    hash_table *ht = (hash_table *)malloc(sizeof(hash_table));
    if (!ht) {
        perror("Error allocating memory for hash table");
        return -1;
    }

    // Opening the hash table file for reading and writing
    FILE *file = fopen("segments/hash_table", "rb+");
    if (!file) {
        perror("Error opening hash table file");
        free(ht); 
        return -1;
    }
    fread(ht, sizeof(hash_table), 1, file);
    fclose(file);

    int found_hash = 0;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        // Compare hash values byte by byte
        if (memcmp(ht->entries[i].hash, buffer_hash, HASH_SIZE) == 0) {
            *block = ht->entries[i].block_id;
            ht->entries[i].referenced++;
            printf("Reusing block %d. ", *block);
            found_hash = 1;
            FILE *file2 = fopen("segments/hash_table", "rb+");
            if (!file2) {
                perror("Error opening hash table file for writing");
                free(ht); // Free allocated memory
                return -1;
            }
            // printf("Referenced: %d\n", ht->entries[i].referenced);
            // Write the updated hash table back to the file
            fwrite(ht, sizeof(hash_table), 1, file2);
            fclose(file2);
            break;
        }
    }
    return found_hash;
}

//Calculate the hash of the buffer
void calculate_hash(const unsigned char* buffer, size_t buffer_size, unsigned char* hash_result) 
{
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, buffer, buffer_size);
    SHA256_Final(hash_result, &ctx);
}

//Add the hash to the hash table
void add_to_hash_table(unsigned char* buffer_hash, int block)
{
    hash_table *ht = (hash_table *)malloc(sizeof(hash_table));
    FILE *file = fopen("segments/hash_table", "rb+");
    if (!file) {
        perror("Error opening file system file");
        return;
    }
    fread(ht, sizeof(hash_table), 1, file);
    fclose(file);
    int found_hash = 0;
    for (int i = 0; i < HASH_TABLE_SIZE; i++)
    {
        if(ht->entries[i].block_id == -1)
        {
            ht->entries[i].block_id = block;
            ht->entries[i].referenced = 1;
            memcpy(ht->entries[i].hash, buffer_hash, HASH_SIZE);
            found_hash = 1;
            break;
        }
    }
    if (!found_hash)
    {
        printf("Oops! Ran out of space in the hash table.\n");
    }

    FILE *file2 = fopen("segments/hash_table", "rb+");
    if (!file2) {
        perror("Error opening file system file");
        return;
    }
    fwrite(ht, sizeof(hash_table), 1, file2);
    fclose(file2);
}

//Write the file to the file system
void write_file_to_fs(FILE* source_file, int file_size, int* blocks) 
{
    int buffer_size = BLOCK_SIZE; 
    char* buffer = malloc(buffer_size * sizeof(char));

    int remaining_bytes = file_size;
    // int block_index = 0;
    int block_array_index = 0; 
    while (remaining_bytes > 0) {
        // Read data from the source file into the buffer
        int bytes_to_read = (remaining_bytes > buffer_size) ? buffer_size : remaining_bytes;
        fread(buffer, sizeof(char), bytes_to_read, source_file);

        // Calculate the hash of the buffer
        unsigned char buffer_hash[HASH_SIZE];
        calculate_hash(buffer, bytes_to_read, buffer_hash);

        // Checking if the hash is already present in the hash table
        int found_hash = venti(buffer_hash, bytes_to_read, &blocks[block_array_index]);
        if (!found_hash)
        {
            // Add the hash to the hash table if not in the hash table
            add_to_hash_table(buffer_hash, blocks[block_array_index]);
            write_file_to_block(buffer, blocks[block_array_index], bytes_to_read);
        }
        remaining_bytes -= bytes_to_read;
        block_array_index++;
    }

    // free(buffer);
}

//Update the inode with the blocks and file size
void update_inode_with_blocks(int inode_num, int* blocks, int num_blocks, int file_size)
{
    Segment *seg= (Segment *)malloc(sizeof(Segment));
    int segment_id = inode_num / 10;
    int inode_index = inode_num % 10;
    char filepath[255];
    char* seg_name = (char *)malloc(255);
    seg_name = get_segname(segment_id);
    snprintf(filepath, sizeof(filepath), "segments/%s", seg_name);
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        perror("Error opening file system file");
        return;
    }
    fseek(file, 0, SEEK_SET);
    fread(seg, sizeof(Segment), 1, file);
    fclose(file);
    seg->inodes[inode_index].size = file_size;
    for (int i = 0; i < num_blocks; i++)
    {
        seg->inodes[inode_index].blocks[i] = blocks[i];
    }
    FILE *file2 = fopen(filepath, "rb+");
    if (!file2) {
        perror("Error opening file system file");
        return;
    }
    fwrite(seg, sizeof(Segment), 1, file2);
    fclose(file2);
    // free(seg);
}

//Add new file to the file system using the given path
void add_new_file(int dir_inode, char* fpath) 
{
    //Finding free inode for data
    int free_inode = find_unused_inode(DATA);
    if(free_inode == -1) {
        create_segment(DATA, 1);
        free_inode = find_unused_inode(DATA);
    }
    // printf("Free inode: %d\n", free_inode);
    mark_inode(free_inode, 1);
    FILE *source_file = fopen(fpath, "rb");
    if (!source_file) {
        perror("Error opening source file");
        return;
    }

    //Writing the file in the blocks of inode
    Segment *seg1 = (Segment *)malloc(sizeof(Segment));
    int segment_id = free_inode / 10;
    int inode_index = free_inode % 10;
    char filepath[255];
    char* seg_name = (char *)malloc(255);
    seg_name = get_segname(segment_id);
    snprintf(filepath, sizeof(filepath), "segments/%s", seg_name);
    FILE *file = fopen(filepath, "rb+");
    if (!file) {
        perror("Error opening file system file");
        return;
    }
    fseek(file, 0, SEEK_SET);
    fread(seg1, sizeof(Segment), 1, file);
    fclose(file);
    inode *ind1 = (inode *)malloc(sizeof(inode));
    *ind1 = seg1->inodes[inode_index];

    //Adding directory entry for the file in the parent directory
    int num_elements;
    char** dir_name = parse_path(fpath, &num_elements);
    char fname[255];
    strcpy(fname, dir_name[num_elements - 1]);
    // printf("File name: %s\n", fname);
    int exist = search_inode_in_directory_entry(dir_inode, fname);
    if (exist != -1) 
    {
        printf("File already exists in directory.\n");
        return;
    }
    else
    {
        add_directory_entry_in_parent(dir_inode, free_inode, fname, DATA);
    }

    //Calculating the number of blocks needed
    fseek(source_file, 0, SEEK_END);
    long file_size = ftell(source_file);
    fseek(source_file, 0, SEEK_SET);
    int num_blocks = file_size / BLOCK_SIZE;
    if (file_size % BLOCK_SIZE != 0) 
    {
        num_blocks++;
    }

    // printf("Number of blocks needed: %d\n", num_blocks);
    int* blocks = malloc(num_blocks * sizeof(int));

    //Getting empty blocks for the file
    get_empty_blocks(num_blocks, blocks, DATA);

  
    // printf("Found Blocks: ");
    // for (int i = 0; i < num_blocks; i++) {
    //     printf("%d ", blocks[i]);
    // }

    //Writing the file data to the blocks
    write_file_to_fs(source_file, file_size, blocks);

    //Updating the inode with the blocks
    update_inode_with_blocks(free_inode, blocks, num_blocks, file_size);
    free(ind1);
    free(seg1);
    // free(seg_name);
    free(dir_name);
    fclose(source_file);
    free(blocks);
}

//Get the inode number to the last directory in the path
//Create new if we are adding to it 
int get_inode_to_last_directory(char* fpath, int create_new)
{
    int num_elements;
    char** dir_name = parse_path(fpath, &num_elements);
    // for (int i = 0; i < num_elements; i++) {
    //     printf("Element %d: %s\n", i, dir_name[i]);
    // }
    int current_inode = 0;
    int new_inode = -1;
    for (int i = 0; i < num_elements; i++) 
    {
        new_inode = search_inode_in_directory_entry(current_inode, dir_name[i]);
        if (new_inode == -1) 
        {
            if(create_new == 0)
            {
                return -1;
            }
            new_inode = create_directory(dir_name[i], current_inode);
        }
        current_inode = new_inode;
    }
    return current_inode;
}

//Add file to the file system
void addfilefs(char* fspath, char *fpath) 
{
    int current_inode = get_inode_to_last_directory(fspath, 1);
    add_new_file(current_inode, fpath);
   
}
  

//Split the path into directory and file for exfs
void split_path(const char *path, char *dir, char *file) 
{
    // Find the last occurrence of '/'
    const char *last_slash = strrchr(path, '/');

    if (last_slash != NULL) {
        // Copy directory part
        strncpy(dir, path, last_slash - path);
        dir[last_slash - path] = '\0'; // Null-terminate the string

        // Copy file part
        strcpy(file, last_slash + 1);
    } else {
        // No directory, copy the entire path as file
        strcpy(dir, ".");
        strcpy(file, path);
    }
}

//Mark inode as unused in the segment
void mark_unused_inode_in_directory_entry(int dir_inode, int file_inode)
{
    Segment *seg= (Segment *)malloc(sizeof(Segment));
    int segment_id = dir_inode / 10;
    int inode_index = dir_inode % 10;
    char filepath[255];
    char* seg_name = (char *)malloc(255);
    seg_name = get_segname(segment_id);
    snprintf(filepath, sizeof(filepath), "segments/%s", seg_name);
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        perror("Error opening file system file");
        return;
    }
    fseek(file, 0, SEEK_SET);
    fread(seg, sizeof(Segment), 1, file);
    fclose(file);
    inode *ind = (inode *)malloc(sizeof(inode));
    *ind = seg->inodes[inode_index];

    for (int i = 0; i < BLOCKSPERSEG; i++)
    {
        if(ind->blocks[i] != -1)
        {
            DirectoryEntry *entry_ptr = (DirectoryEntry *)malloc(sizeof(DirectoryEntry));
            memcpy(entry_ptr, &(seg->blocks[ind->blocks[i]]), sizeof(DirectoryEntry));
            if(entry_ptr->inodenum == file_inode)
            {
                // printf("Found search entry in block %d with name: %s and inode %d\n", ind->blocks[i], entry_ptr->name, entry_ptr->inodenum);
                entry_ptr->inuse = 0;
                memcpy(&(seg->blocks[ind->blocks[i]]), entry_ptr, sizeof(DirectoryEntry));
                break;
            }
        }
    }
    FILE *file2 = fopen(filepath, "rb+");
    if (!file2) {
        perror("Error opening file system file");
        return;
    }
    fwrite(seg, sizeof(Segment), 1, file2);
    fclose(file2);
    free(ind);
    free(seg);
}

//Setting the block as free in the segment
void set_FBL_free(int block)
{
    Segment *seg = (Segment *)malloc(sizeof(Segment));
    int segment_id = block / 10000;
    int block_index = block % 10000;
    char filepath[255];
    char* seg_name = (char *)malloc(255);
    seg_name = get_segname(segment_id); // Assuming get_segname returns a char*.
    snprintf(filepath, sizeof(filepath), "segments/%s", seg_name);
    FILE *file = fopen(filepath, "rb+");
    if (!file) {
        perror("Error opening file system file");
        // free(seg_name); // Free allocated memory if file opening fails
        return;
    }
    fseek(file, 0, SEEK_SET);
    fread(seg, sizeof(Segment), 1, file);
    fclose(file);

    seg->FBL.bitmap[block_index] = -1;

    FILE *file2 = fopen(filepath, "rb+");
    if (!file2) {
        perror("Error opening file system file");
        // free(seg_name); // Free allocated memory if file opening fails
        return;
    }
    fwrite(seg, sizeof(Segment), 1, file2);
    fclose(file2);
    // free(seg);
}

//Decrement the reference count of the block in the hash table
void decrement_refrenced_count(int file_inode)
{
    int seg_id = file_inode / 10;
    int inode_index = file_inode % 10;
    char filepath[255];
    char *seg_name = (char *)malloc(255);
    seg_name = get_segname(seg_id);
    Segment *seg = (Segment *)malloc(sizeof(Segment));
    snprintf(filepath, sizeof(filepath), "segments/%s", seg_name);
    FILE *file = fopen(filepath, "rb+");
    if (!file) 
    {
        perror("Error opening file system file");
        return;
    }
    fseek(file, 0, SEEK_SET);
    fread(seg, sizeof(Segment), 1, file);
    fclose(file);
    seg->inodes[inode_index].size = 0;

    hash_table *ht = (hash_table *)malloc(sizeof(hash_table));
    FILE *file3 = fopen("segments/hash_table", "rb+");
    if (!file) {
        perror("Error opening file system file");
        return;
    }
    fread(ht, sizeof(hash_table), 1, file3);
    fclose(file3);

    //Decrementing the reference count in the hash table
    int block;
    for (int i = 0; i < BLOCKSPERSEG; i++)
    {
        block = seg->inodes[inode_index].blocks[i];
        if (block != -1)
        {
            for (int j = 0; j < HASH_TABLE_SIZE; j++)
            {
                if(ht->entries[j].block_id == block)
                {
                    ht->entries[j].referenced--;

                    //If the block is not referenced by any inode, mark it as free
                    if(ht->entries[j].referenced == 0)
                    {
                        // printf("for refrenced: %d, we are inside\n", ht->entries[j].referenced);
                        ht->entries[j].block_id = -1;
                        memset(ht->entries[j].hash, 0, HASH_SIZE);
                        ht->entries[j].referenced = -1;
                        set_FBL_free(block);
                        printf("Block %d is now free. ", block);
                    }
                }
            }
        }
    }    

    FILE *file2 = fopen("segments/hash_table", "rb+");
    if (!file2) {
        perror("Error opening file system file");
        return;
    }
    fwrite(ht, sizeof(hash_table), 1, file2);
    fclose(file2);
}

//Remove file from the file system
void removefilefs(char* fname)
{
    char dir[256];
    char file[256];

    split_path(fname, dir, file);

    // printf("Directory: %s\n", dir);
    // printf("File: %s\n", file);
    int dir_inode = get_inode_to_last_directory(dir, 0);
    if (dir_inode == -1) 
    {
        printf("Directory not found\n");
        return;
    }
    int file_inode = search_inode_in_directory_entry(dir_inode, file);
    // printf("File inode: %d\n", file_inode);
    if (file_inode == -1) 
    {
        printf("File not found\n");
        return;
    }

    // mark_inode(file_inode, 0);
    mark_unused_inode_in_directory_entry(dir_inode, file_inode);
    decrement_refrenced_count(file_inode);
}

//Getting block ids from an inode's block array
void get_blocks_from_inode(int file_inode, int* blocks) 
{
    Segment *seg = (Segment *)malloc(sizeof(Segment));
    int segment_id = file_inode / 10;
    int inode_index = file_inode % 10;
    char filepath[255];
    char* seg_name = (char *)malloc(255);
    seg_name = get_segname(segment_id); 
    snprintf(filepath, sizeof(filepath), "segments/%s", seg_name);
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        perror("Error opening file system file");
        free(seg_name); 
        return;
    }
    fseek(file, 0, SEEK_SET);
    fread(seg, sizeof(Segment), 1, file);
    fclose(file);
    
    inode *ind = &(seg->inodes[inode_index]); 

    for (int i = 0; i < BLOCKSPERSEG; i++) 
    {
        blocks[i] = ind->blocks[i];
    }
}


//Getting the file sixe from inode
int get_file_size(int file_inode)
{
    Segment *seg = (Segment *)malloc(sizeof(Segment));
    int segment_id = file_inode / 10;
    int inode_index = file_inode % 10;
    char filepath[255];
    char* seg_name = (char *)malloc(255);
    seg_name = get_segname(segment_id); // Assuming get_segname returns a char*.
    snprintf(filepath, sizeof(filepath), "segments/%s", seg_name);
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        perror("Error opening file system file");
        free(seg_name); // Free allocated memory if file opening fails
        return -1;
    }
    fseek(file, 0, SEEK_SET);
    fread(seg, sizeof(Segment), 1, file);
    fclose(file);
    int file_size = seg->inodes[inode_index].size;
    // free(seg);
    return file_size;
}


void extractfilefs(char* fname) 
{
    // printf("Extracting file: %s\n", fname);
    char dir[256];
    char file[256];
    split_path(fname, dir, file);
    // printf("Directory: %s\n", dir);
    // printf("File: %s\n", file);
    
    // Checking if directory exists
    int dir_inode = get_inode_to_last_directory(dir, 0);
    if (dir_inode == -1) {
        printf("Directory not found\n");
        return;
    }
    
    // Checking if file exists
    int file_inode = search_inode_in_directory_entry(dir_inode, file);
    if (file_inode == -1) {
        printf("File not found. Specify a regular file\n");
        return;
    }
    
    // Retrieve file blocks and size
    int *blocks = (int *)malloc(1000 * sizeof(int));
    get_blocks_from_inode(file_inode, blocks);

    // printf("Blocks for file inode %d: ", file_inode);
    // for (int i = 0; i < 1000; i++) {
    //     if (blocks[i] != -1) {
    //         printf("%d ", blocks[i]);
    //     }
    // }
    
    int file_size = get_file_size(file_inode);
    // printf("\nFile size: %d bytes\n", file_size);
    // printf("File contents:\n");
    // printf("File size: %d bytes\n", file_size);

    char buffer[BLOCK_SIZE]; // Buffer to hold file contents
    int remaining_bytes = file_size; // Tracking remaining bytes to read
    for (int i = 0; i < BLOCKSPERSEG && remaining_bytes > 0; i++) 
    {
        if (blocks[i] != -1) {
            Segment *seg = (Segment *)malloc(sizeof(Segment));
            int segment_id = blocks[i] / 10000;
            int block_index = blocks[i] % 10000;
            // printf("Segment id: %d, Block index: %d\n", segment_id, block_index);
            char filepath[255];
            char* seg_name = get_segname(segment_id);
            // printf("Segment name: %s\n", seg_name);
            snprintf(filepath, sizeof(filepath), "segments/%s", seg_name);
            FILE *file = fopen(filepath, "rb");
            if (!file) {
                perror("Error opening file system file");
                return;
            }
            // fseek(file, block_index * BLOCK_SIZE, SEEK_SET);
            // Reading the remaining bytes or up to BLOCK_SIZE, whichever is smaller
            int bytes_to_read = remaining_bytes < BLOCK_SIZE ? remaining_bytes : BLOCK_SIZE;
            fread(seg, sizeof(Segment), 1, file);
            fwrite(seg->blocks[block_index].data, sizeof(char), bytes_to_read, stdout);
            fclose(file);
            //free(seg);
            remaining_bytes -= bytes_to_read;
        }
    }
    // free(blocks); // Free dynamically allocated memory
}


void debug_directory_entries(int dir_inode, int indent)
{
    // printf("Printing directory entries\n");
    Segment *seg= (Segment *)malloc(sizeof(Segment));
    int segment_id = dir_inode / 10;
    int inode_index = dir_inode % 10;
    char filepath[255];
    char* seg_name = (char *)malloc(255);
    seg_name = get_segname(segment_id);
    snprintf(filepath, sizeof(filepath), "segments/%s", seg_name);
    // printf("Segment name: %s\n", filepath);
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        perror("Error opening file system file");
        return;
    }
    fseek(file, 0, SEEK_SET);
    fread(seg, sizeof(Segment), 1, file);
    fclose(file);
    inode *ind = (inode *)malloc(sizeof(inode));
    *ind = seg->inodes[inode_index];
    // printf("For Inode: %d\n", dir_inode);
    for (int i = 0; i < BLOCKSPERSEG; i++)
    {
        // printf("Block vaues = %d\n", ind->blocks[i]);
        if(ind->blocks[i] != -1)
        {
            // printf("Block %d\n", ind->blocks[i]);
            DirectoryEntry *entry_ptr = (DirectoryEntry *)malloc(sizeof(DirectoryEntry));
            memcpy(entry_ptr, &(seg->blocks[ind->blocks[i]]), sizeof(DirectoryEntry));
            if(entry_ptr -> inuse == 1)
            {
                for (int j = 0; j < indent; j++)
                {
                    printf("-");
                }
                if(entry_ptr->inuse == 1)
                {
                    if (entry_ptr->type == DIR)
                    {
                        printf("directory '%s':\t", entry_ptr->name);
                    }
                    else
                    {
                        printf("%s\t", entry_ptr->name);
                    }
                    printf("| ");
                    printf("%d\n", entry_ptr->inodenum);
                    // printf("%s\n", entry_ptr->type == DIR ? "Dir" : "Data");
                    // printf("%d\n", entry_ptr->inuse);
                    if(entry_ptr->type == DIR)
                    {
                        print_directory_entries(entry_ptr->inodenum, indent + 1);
                    }
                }
            }
        }
    }
    // free(ind);
    // free(seg);

}

void debugfs()
{
    printf("directory '/':\n");
    debug_directory_entries(0, 0);

}