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
    for (int i = 0; i < MAX_ENTRIES; ++i) 
    { // Loop through entries
        if (strlen(mf->entries[i].name) > 0) {
            printf("%d.\t\t", i + 1);
            printf("%s\t", mf->entries[i].type == DIR ? "DIR" : "DATA");
            printf("%s\n", mf->entries[i].name);
        } else {
            break; // Exit loop if name is empty
        }
    }
    fclose(file);
    // Free allocated memory
    free(mf);
}


char * initialize_manifest()
{
    // Dynamically allocate memory for the manifest
    printf("Initializing the manifest\n");
    Manifest *mf = (Manifest *)malloc(sizeof(Manifest));
    if (mf == NULL) {
        perror("Error allocating memory for manifest");
        return NULL;
    }
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
        free(mf); // Free allocated memory before returning
        return NULL;
    }
    fseek(file, 0, SEEK_SET);
    fwrite(mf, sizeof(Manifest), 1, file);
    fclose(file);
    // memcpy(fs, &mf, sizeof(Manifest));
    free(mf);
    return dir_name;
}

// char* update_manifest(int type)
// {
//     char * seg_name;
//     Manifest *mf = (Manifest *)malloc(sizeof(Manifest));
//     FILE *manifest = fopen("segments/manifest", "rb+");
//     if (!manifest) {
//         perror("Error opening file system file");
//         return NULL;
//     }
//     fread(mf, sizeof(Manifest), 1, manifest);
//     mf->MH.numsegs++;
//     if(type == DIR)
//     {
//         mf->MH.numdirectorysegs++;
//         seg_name = (char *)malloc(255);
//         sprintf(seg_name, "DIR_%d", mf->MH.numdirectorysegs);
//     }
//     else
//     {
//         mf->MH.numdatasegs++;
//         seg_name = (char *)malloc(255);
//         sprintf(seg_name, "DATA_%d", mf->MH.numdatasegs);
//     }
    
//     mf->MH.numinodes += INODESPERSEG;
//     mf->entries[mf->MH.numsegs - 1].type = type;
//     printf("Segment name to be added: %s of type: %d\n", seg_name, type);

//     strcpy(mf->entries[(mf->MH.numsegs) - 1].name, seg_name);
//     fseek(manifest, 0, SEEK_SET);
//     fwrite(mf, sizeof(Manifest), 1, manifest);
//     fclose(manifest);
//     free(mf);
//     return seg_name;
// }

char* update_manifest(int type) {
    char* seg_name = (char*)malloc(255);
    Manifest* mf = (Manifest*)malloc(sizeof(Manifest));
    FILE* manifest = fopen("segments/manifest", "rb+");
    if (!manifest) {
        perror("Error opening file system file");
        return NULL;
    }
    fread(mf, sizeof(Manifest), 1, manifest);
    mf->MH.numsegs++;
    printf("Updated no. of segments: %d\n", mf->MH.numsegs );
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
    printf("Segment name to be added: %s of type: %d\n", seg_name, type);

    // Copy seg_name into the appropriate manifest entry's name
    strcpy(mf->entries[mf->MH.numsegs - 1].name, seg_name);

    fseek(manifest, 0, SEEK_SET);
    fwrite(mf, sizeof(Manifest), 1, manifest);
    fclose(manifest);
    free(mf);
    return seg_name;
}


void create_segment(int type, int num)
{
    char seg_path[256];
    char*seg_name = (char*) malloc(255);

    if(num == 0)
    {
        printf("Initializing segment\n");
        seg_name = initialize_manifest();
    }
    else
    {
        printf("Updating manifest\n");
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
    //For not setting node for files that only need blocks
    if (num != -1)
    {
        seg->inodes[0].inuse = 1;
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
    free(seg_name);
    // print_manifest();
}

void formatfs()
{   
    // Initialize the file system
    printf("Formatting the file system\n");
    // initialize_manifest();
    create_segment(DIR, 0);
    // print_manifest();
}



void loadfs()
{

}

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
    for (int i = 0; i < ((BLOCK_SIZE/4)-4); i++)
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


void lsfs()
{       
    // print_manifest();
    print_directory_entries(0, 0);
}

int extract_number(const char* str) {
    // Find the position of the underscore character
    const char* underscore_pos = strchr(str, '_');
    if (!underscore_pos) {
        // Return an error value if underscore is not found
        return -1;
    }

    // Convert the substring after the underscore to an integer
    int number = atoi(underscore_pos + 1);
    return number;
}

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
    printf("Inode %d marked as %d\n", inode_num, use);
}

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
    printf("Number of segments: %d\n", mf->MH.numsegs);
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
        printf("Segment %d\n", i);
        seg_name = get_segname(i);
        char* path2 = (char *)malloc(255);
        sprintf(path2, "segments/%s", seg_name);
        printf("Segment name: %s\n", path2);
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
            printf("inode %d in use  with value = %d\n ", j, seg->inodes[j].inuse); 
            if (seg->inodes[j].inuse == 0)
            {
                printf("Found unused inode at segment %d, inode %d\n", i, j);
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
        ///Creating a new segment
        // seg_nam2 = update_manifest(DIR);
        printf("Creating new segment\n");
        create_segment(DIR, 1);
        manifest = fopen("segments/manifest", "rb+");
        fread(mf, sizeof(Manifest), 1, manifest);
        fclose(manifest);
        printf("Num of segments: %d\n", mf->MH.numsegs);
        int seg_id2 = mf->MH.numsegs - 1;
        seg_nam2 = get_segname(seg_id2);
        new_inode = (mf->MH.numsegs - 1) * 10;
        i = mf->MH.numsegs - 1;
        j = 0;  
        printf("i = %d, j = %d\n", i, j);          
    }
        char seg_name2[255];
        char path_2[255];
        seg_nam2 = get_segname(segment_id);
        sprintf(seg_name2, "segments/%s", seg_nam2);
        printf("Opeing Segment name: %s for entry.\n", seg_name2);
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
        for (k = 0; k < ((SEGMENT_SIZE/BLOCK_SIZE)/8); k++)
        {
            if(seg->FBL.bitmap[k] == -1)
            {
                printf("Found free block at %d\n", k);
                seg->FBL.bitmap[k] = 0;
                found_free_block = 1;
                break;
            }
        }
        //FOR MAX BLOCK EXCEEDING
        if(!found_free_block)
        {
            printf("No free block found\n");
            return -1;
        }


        //Flagging the block used for that inode
        // for (int x = 0; x < BLOCKSPERSEG; x++)
        int x =0;
        for (int x = 0; x < (BLOCK_SIZE/4)-4; x++)
        {
            if(seg->inodes[inode_index].blocks[x] == -1)
            {
                seg->inodes[inode_index].blocks[x] = k;
                break;
            }
        }
        seg->inodes[inode_index].size = x * sizeof(DirectoryEntry);

        printf("\n");
        printf("inode for new entry: %d\n", new_entry->inodenum);

        // seg->inodes[inode_index].blocks[0] = k;
        memcpy(seg->blocks[k].data, new_entry, sizeof(DirectoryEntry));
        
        char *seg_name3 = get_segname(segment_id);
        sprintf(path, "segments/%s", seg_name3);
        printf("Saving directory entry to segment name: %s\n", path);
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

char** parse_path(const char* path, int* num_elements) {
    char* token = (char *)malloc(255);
    char* path_copy = (char *)malloc(255);
    path_copy = strdup(path);  // Create a copy of the path to avoid modifying the original string

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


int search_inode_in_directory_entry(int current_inode, char* name)
{
    printf("Searching in inode %d in for %s \n", current_inode, name);
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

    for (int i = 0; i < (BLOCK_SIZE/4)-4; i++)
    {
        if(ind->blocks[i] != -1)
        {
            DirectoryEntry *entry_ptr = (DirectoryEntry *)malloc(sizeof(DirectoryEntry));
            memcpy(entry_ptr, &(seg->blocks[ind->blocks[i]]), sizeof(DirectoryEntry));
            if(strcmp(entry_ptr->name, name) == 0)
            {
                printf("Found search entry in block %d with name: %s and inode %d\n", ind->blocks[i], entry_ptr->name, entry_ptr->inodenum);
                // printf("Found search entry in block %d with name: %s and inode %d\n", ind->blocks[i], entry_ptr->name, entry_ptr->inodenum);
                new_inode = entry_ptr->inodenum;
                break;
            }
        }
    }
    printf("%s not found in inode %d\n", name, current_inode);
    free(ind);
    free(seg);
    return new_inode;
}

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
    if(type == DIR && mf->MH.numdatasegs == 0)
    {
        create_segment(DATA, 1);
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
                printf("Found unused inode at segment %d, inode %d\n", i, j);
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
    for (k = 0; k < ((SEGMENT_SIZE/BLOCK_SIZE)/8); k++)
    {
        if(seg->FBL.bitmap[k] == -1)
        {
            printf("Found free block at %d\n", k);
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
    for (int x = 0; x < (BLOCK_SIZE/4)-4; x++)
    {
        if(seg->inodes[inode_index].blocks[x] == -1)
        {
            seg->inodes[inode_index].blocks[x] = k;
            break;
        }
    }

    seg->inodes[inode_index].size = x * sizeof(DirectoryEntry);
    printf("\n");
    printf("inode for new entry: %d\n", new_entry->inodenum);
    // seg->inodes[inode_index].blocks[0] = k;
    memcpy(seg->blocks[k].data, new_entry, sizeof(DirectoryEntry));

    char segname3[255];
    char filepath3[255];
    strcpy(segname3, get_segname(segment_id));
    sprintf(filepath3, "segments/%s", segname3);
    printf("Saving directory entry to segment name: %s\n", filepath3);

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

void add_new_file(int dir_inode, char* fpath) {
    // Step 1: Finding free inode for data
    int free_inode = find_unused_inode(DATA);
    if(free_inode == -1) {
        create_segment(DATA, 1);
        free_inode = find_unused_inode(DATA);
    }
    printf("Free inode: %d\n", free_inode);
    mark_inode(free_inode, 1);

    // Step 2: Open the source file
    FILE *source_file = fopen(fpath, "rb");
    if (!source_file) {
        perror("Error opening source file");
        return;
    }

    // Step 3: Write the file in the blocks of inode
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

    // Step 4: Calculate the number of blocks needed
    fseek(source_file, 0, SEEK_END);
    int file_size = ftell(source_file);
    int num_blocks = file_size / BLOCK_SIZE;
    if (file_size % BLOCK_SIZE != 0) 
    {
        num_blocks++;
    }
    printf("Number of blocks needed: %d\n", num_blocks);


    //Add directory entry for the file in the parent directory
    int num_elements;
    char** dir_name = parse_path(fpath, &num_elements);
    char fname[255];
    strcpy(fname, dir_name[num_elements - 1]);
    printf("File name: %s\n", fname);
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

    //Free the allocated memory
    free(ind1);
    free(seg1);
    // free(seg_name);
    free(dir_name);
    fclose(source_file);

    // 
}

int get_inode_to_last_directory(char* fpath, int create_new)
{
    int num_elements;
    char** dir_name = parse_path(fpath, &num_elements);
    int current_inode = 0;
    int new_inode = -1;
    for (int i = 0; i < num_elements; i++) 
    {
        new_inode = search_inode_in_directory_entry(current_inode, dir_name[i]);
        if (new_inode == -1) 
        {
            // printf("Directory '%s' not found\n", dir_name[i]);
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

    int current_inode = get_inode_to_last_directory(fpath, 1);
    add_new_file(current_inode, fpath);
    // FILE *file = fopen("segments/DIR_1", "rb");
    // if (!file) {
    //     perror("Error opening file system file");
    //     return;
    // }

    // memcpy(&current_dir, fs->blocks[0].data , sizeof(Directory));
    // for(int l = 0; l < num_elements; l++)
    // {
      
    //     new_inode = search_inode_in_directory_entry(current_inode, dir_name[l]);
    //     if (new_inode == -1 ) 
    //     {
    //         printf("Directory '%s' not found\n", dir_name[l]);
    //         new_inode = create_directory(dir_name[l], current_inode);
    //     }
    //     current_inode = new_inode;
    //     printf("Current inode: %d\n", current_inode);
        
    // }

    //Add the file to the directory in the current directory
    

    // free(dir_name);
}
  


void split_path(const char *path, char *dir, char *file) {
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

    for (int i = 0; i < (BLOCK_SIZE/4)-4; i++)
    {
        if(ind->blocks[i] != -1)
        {
            DirectoryEntry *entry_ptr = (DirectoryEntry *)malloc(sizeof(DirectoryEntry));
            memcpy(entry_ptr, &(seg->blocks[ind->blocks[i]]), sizeof(DirectoryEntry));
            if(entry_ptr->inodenum == file_inode)
            {
                printf("Found search entry in block %d with name: %s and inode %d\n", ind->blocks[i], entry_ptr->name, entry_ptr->inodenum);
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

void removefilefs(char* fname)
{
    char dir[256];
    char file[256];

    split_path(fname, dir, file);

    printf("Directory: %s\n", dir);
    printf("File: %s\n", file);
    int dir_inode = get_inode_to_last_directory(dir, 0);
    if (dir_inode == -1) 
    {
        printf("Directory not found\n");
        return;
    }
    int file_inode = search_inode_in_directory_entry(dir_inode, file);
    if (file_inode == -1) 
    {
        printf("File not found\n");
        return;
    }
    mark_unused_inode_in_directory_entry(dir_inode, file_inode);
    mark_inode(file_inode, 0);

}


void extractfilefs(char* fname)
{}

