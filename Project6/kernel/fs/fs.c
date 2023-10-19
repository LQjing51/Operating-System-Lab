#include <os/list.h>
#include <os/mm.h>
#include <pgtable.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/fs.h>
#include <os/time.h>
#include <os/irq.h>
#include <screen.h>
#include <stdio.h>
#include <assert.h>
#include <csr.h>
#include <type.h>
#include <os/string.h>
//sd display
#define SD_FS_BEGIN 1048576
#define SUPERBLOCK 1048576//0
#define INODE_MAP 1048577//1
#define SECTOR_MAP 1048578//2~257
#define INODE 1048834//258~385
#define DATA 1048962//386~
//kernel sd buffer
#define BUFFERNUM 100
//superblock display
#define MAGIC_OFFSET 0
#define NUM_SECTOR_OFFSET 1
#define START_SECTOR_OFFFSET 2
#define INODE_MAP_OFFSET 3
#define SECTOR_MAP_OFFSET 4
#define INODE_OFFSET 5
#define DATA_OFFSET 6
#define INODE_ENTRY_SIZE 7
#define DIR_ENTRY_SIZE 8

//file describer
#define FDNUM 10
#define O_RDONLY 1 /* read only open */
#define O_WRONLY 2 /* write only open */
#define O_RDWR 3 /* read/write open */

typedef struct{
    int inode;
    int mode;
    u32 pos;
}fd_t;
fd_t fdbuffer[FDNUM];

typedef struct{
    char name[25];
    int size;
    int type;//0:file 1:directory
    int hard_link;
    int direct_pointer[7];
    int indirect_pointer[3];
    int second_pointer[2];
    int third_pointer;
}inode_t;
typedef struct{
    char name[27];
    u8 type;
    int inode;
}dir_entry_t;
//the inode number of current directory
int cur_dir=0;//root

int x;
void my_srand(unsigned seed)
{
    x = seed;
}
int myrand()
{
    long long tmp = 0x5deece66dll * x + 0xbll;
    x = tmp & 0x7fffffff;
    return x;
}

int sdbuffer_id[BUFFERNUM];
char sdbuffer[BUFFERNUM][512];
u64 sdbuffer_last_ref_time[BUFFERNUM];

char zero_buffer[512];

int load_sd(int id){
    int i=0;
    for(i=0;i<BUFFERNUM;i++){
        if(sdbuffer_id[i]==id){
            sdbuffer_last_ref_time[i] = get_ticks();
            return i;
        }
    }
    for(i=0;i<BUFFERNUM;i++){
        if(!sdbuffer_id[i]){
            break;
        }
    }
    if(i<BUFFERNUM){
        sbi_sd_read(kva2pa(sdbuffer[i]),1,id);
        sdbuffer_id[i] = id;
        sdbuffer_last_ref_time[i] = get_ticks();
        return i;
    }else{
        int replace = 0;
        int min_time = sdbuffer_last_ref_time[0];
        for (i = 1; i < BUFFERNUM; i++)
            if (sdbuffer_last_ref_time[i] < min_time) {
                min_time = sdbuffer_last_ref_time[i];
                replace = i;
            }
        sbi_sd_read(kva2pa(sdbuffer[replace]),1,id);
        sdbuffer_id[replace] = id;
        sdbuffer_last_ref_time[replace] = get_ticks();
        return replace;
    }
}
int store_sd(int buffer_index){
    sbi_sd_write(kva2pa(sdbuffer[buffer_index]), 1,sdbuffer_id[buffer_index]);
    sdbuffer_last_ref_time[buffer_index] = get_ticks();
    return 1;
}
int used_data_sector(){
    int i,j;
    int sum = 0 ;
    int sector_map_index;
    for(i=0;i<256;i++){
        sector_map_index = load_sd(SECTOR_MAP + i);
        for(j=0;j<512;j++){
            u8 byte = sdbuffer[sector_map_index][j];
            int k = 0;
            u8 pw = 1;
            while(k<8){
                if(byte & pw){
                    sum++;
                }
                pw <<= 1; 
                k++;
            }
        }
    }
    return sum;
}
int used_inode(){
    int i;
    int sum=0;
    int inode_map_index = load_sd(INODE_MAP);
    for(i=0;i<512;i++){
        if(sdbuffer[inode_map_index][i]==1){
            sum++;
        }
    }
    return sum;
}
int search_for_sector(){
    int i,j;
    int sector_map_index;
    for(i=0;i<256;i++){
        sector_map_index = load_sd(SECTOR_MAP + i);
        for(j=0;j<512;j++){
            u8 byte = sdbuffer[sector_map_index][j];
            if(byte == 255){
                break;
            }else{
                int k = 0;
                u8 pw = 1;
                while (byte & pw) {
                    pw <<= 1; k++;
                }
                sdbuffer[sector_map_index][j] |= pw;
                store_sd(sector_map_index);
                int sector_num = (i*512+j)*8+k;
                sbi_sd_write(kva2pa(zero_buffer), 1, sector_num + DATA);
                return sector_num;
            }
        
        }

    }
}
int set_entry(int buffer_dir_num,dir_entry_t* dir_entry){
    int i;
    for(i=0;i<512;i+=32){
        if(sdbuffer[buffer_dir_num][i]==0){
            kmemcpy(&sdbuffer[buffer_dir_num][i],dir_entry,32);
            store_sd(buffer_dir_num);
            return 1;
        }
    }
    return 0;
}
void set_dir_entry(int dir_inode_num,dir_entry_t* dir_entry){
    // int dir_index = load_sd(sector_number + DATA);
    int inode_index;
    inode_index = load_sd(INODE + (dir_inode_num >> 2));
    inode_t dir_inode;
    kmemcpy(&dir_inode,&sdbuffer[inode_index][(dir_inode_num & 0x3) << 7],sizeof(inode_t));

    int i;
    int count=0;
    int buffer_dir_num;
    int ret;
    buffer_dir_num = load_sd(dir_inode.direct_pointer[count++] + DATA);
    ret = set_entry(buffer_dir_num,dir_entry);
    if(!ret){
        buffer_dir_num = load_sd(dir_inode.direct_pointer[count++] + DATA);
        ret = set_entry(buffer_dir_num,dir_entry);
    }
}

int create_inode(char* name,int size,int type,int father_dir){
    int i;
    int inode_map_index = load_sd(INODE_MAP);
    for(i=0;i<512;i++){
        if(sdbuffer[inode_map_index][i]==0){
            sdbuffer[inode_map_index][i]=1;//used;
            int inode_index = load_sd(INODE + (i >> 2));
            inode_t myinode;
            memset(&myinode, 0, sizeof(inode_t));
            kstrcpy(myinode.name, name);
            myinode.type=type;
            myinode.size=size;
            myinode.hard_link = 1;
            if(myinode.size>0){//when create_inode size==1(dir) or 0(file)
                myinode.direct_pointer[0] = search_for_sector();
            }
            kmemcpy(&sdbuffer[inode_index][(i & 0x3) << 7],&myinode,sizeof(inode_t));
            store_sd(inode_index);
            store_sd(inode_map_index);
            if(myinode.type == 1){
                dir_entry_t mydir_entry;
                kstrcpy(mydir_entry.name, ".");
                mydir_entry.inode = i;
                mydir_entry.type = 1;
                set_dir_entry(i,&mydir_entry);
            
            
                kstrcpy(mydir_entry.name, "..");
                mydir_entry.inode = father_dir;
                mydir_entry.type = 1;
                set_dir_entry(i,&mydir_entry);
            }

            return i;//inode
        }
    }
    return 0;
}
//release inode and it's sector space
void release_inode(int inode){
    int inode_map_index = load_sd(INODE_MAP);
    assert(sdbuffer[inode_map_index][inode]==1);
    sdbuffer[inode_map_index][inode]=0;//clear inode used byte
    store_sd(inode_map_index);

}

void check_fs(){
    int index;
    index = load_sd(SUPERBLOCK);//0x20000000=512MB
    int* magic_num = (int*)(sdbuffer[index]);
    int* begin = magic_num;
    if(*magic_num == 0x54745464){
        prints("[FS] filesystem has been inialized!\n");
    }else{
        do_mkfs();
    }
}
int do_mkfs(){
    int index;
    index = load_sd(SUPERBLOCK);//0x20000000=512MB
    int* magic_num = (int*)(sdbuffer[index]);
    int* begin = magic_num;
    prints("[FS] Start Initialize filesystem!\n");
    prints("[FS] Setting superblock...\n");
    *magic_num = 0x54745464;
    prints("     magic:0x%x\n",*magic_num);
    int* num_sector = begin + NUM_SECTOR_OFFSET;
    *num_sector = 1048576;
    int* start_sector = begin + START_SECTOR_OFFFSET;
    *start_sector = 1048576;
    prints("     num_sector:0x%x, start_sector:0x%x\n",*num_sector,*start_sector);
    int* inode_map_offset = begin + INODE_MAP_OFFSET;
    *inode_map_offset = 1;
    prints("     inode_map_offset:0x%x (1)\n",*inode_map_offset);
    int* sector_map_offset = begin + SECTOR_MAP_OFFSET;
    *sector_map_offset = 2;//256 sector's sector map
    prints("     sector_map_offset:0x%x (256)\n",*sector_map_offset);
    int* inode_offset = begin + INODE_OFFSET;
    *inode_offset = 258;//128 sector's inode
    prints("     inode_offset:0x%x (128)\n",*inode_offset);
    int* data_offset = begin + DATA_OFFSET;
    *data_offset = 386;
    prints("     data_offset:0x%x (1048191)\n",*data_offset);
    int* inode_entry_size = begin + INODE_ENTRY_SIZE;
    *inode_entry_size = 128;
    int* dir_entry_size = begin + DIR_ENTRY_SIZE;
    *dir_entry_size = 32;
    prints("     inode_entry_size:%dB, dir_entry_size:%dB\n",*inode_entry_size,*dir_entry_size);
    store_sd(index);

    prints("[FS] Clear inode_map...\n");
    int inode_map_index = load_sd(INODE_MAP);
    memset(sdbuffer[inode_map_index], 0, 512);
    store_sd(inode_map_index);
    
    prints("[FS] Clear sector_map...\n");
    int i;
    int sector_map_index;
    for(i=0;i<256;i++){
        sector_map_index = load_sd(SECTOR_MAP + i);
        memset(sdbuffer[sector_map_index], 0, 512);
        store_sd(sector_map_index);
    }
    prints("[FS] Setting root inode...\n");
    cur_dir = 0;
    create_inode("/",1,1,0);//type=dir father_dir=0
    prints("[FS] Initialize filesystem finished!\n");
    return 1;
}

int do_statfs(){
    int index;
    int used_sector = used_data_sector();
    int used_in = used_inode();
    index = load_sd(SUPERBLOCK);//0x20000000=512MB
    int* magic_num = (int*)(sdbuffer[index]);
    int* begin = magic_num;
    int* start_sector = begin + START_SECTOR_OFFFSET;
    int* inode_map_offset = begin + INODE_MAP_OFFSET;
    int* sector_map_offset = begin + SECTOR_MAP_OFFSET;
    int* inode_offset = begin + INODE_OFFSET;
    int* data_offset = begin + DATA_OFFSET;
    int* inode_entry_size = begin + INODE_ENTRY_SIZE;
    int* dir_entry_size = begin + DIR_ENTRY_SIZE;
    prints("magic:0x%x\n",*magic_num);
    prints("used sector:%d/1048191, start sector:%d\n",used_sector,*start_sector);
    prints("inode map offset:%d, occupied sector:1, used:%d/512\n",*inode_map_offset,used_in);
    prints("sector map offset:%d, occupied sector:256\n",*sector_map_offset);
    prints("inode offset:%d, occupied sector:128\n",*inode_offset);
    prints("data offset:%d, occupied sector:1048191\n",*data_offset);
    prints("inode entry size:%dB, dir entry size:%dB\n",*inode_entry_size,*dir_entry_size);
    return 1;
}
int find_cur_dir(){
    int cur_inode_index;
    int cur_dir_index;
    cur_inode_index = load_sd(INODE + (cur_dir >> 2));
    inode_t myinode;
    int sector_num;
    kmemcpy(&myinode,&sdbuffer[cur_inode_index][(cur_dir & 0x3) << 7],sizeof(inode_t));
    sector_num = myinode.direct_pointer[0]; 
    cur_dir_index = load_sd(DATA+sector_num);
    return cur_dir_index;
}
void cat_context(int sd_num){
    int sd_index;
    int i;
    sd_index = load_sd(sd_num + DATA);
    for(i=0;i<512;i++){
        if(sdbuffer[sd_index][i]!=0){
            prints("%c",sdbuffer[sd_index][i]);
        }
    }
}
int do_cd(char* directories){
    //parse the path
    char dir[10][20];
    int index=0;
    int i=0;
    int j=0;
    while(directories[index] != '\0'){
        for(;directories[index]!='/' && directories[index]!='\0';index++){
            dir[i][j++] = directories[index];
        }
        dir[i][j] = '\0';
        if(directories[index] == '/'){
            index++;
            i++;
            j=0;
        }
    }
    int init_dir = cur_dir;
    //cd one by one
    int cur_dir_index;
    int type;
    dir_entry_t mydir_entry;
    int m;

    inode_t myinode;
    int cur_inode_index;
    for(j=0;j<=i;j++){//path depth
        cur_dir_index = find_cur_dir();
        //ensure that the directory name has existed
        for(m=0;m<512;m+=32){
            if(sdbuffer[cur_dir_index][m]!=0){
                kmemcpy(&mydir_entry,&sdbuffer[cur_dir_index][m],32);
                cur_inode_index = load_sd(INODE + (mydir_entry.inode >> 2));
                kmemcpy(&myinode,&sdbuffer[cur_inode_index][(mydir_entry.inode & 0x3) << 7],sizeof(inode_t));
                if(!kstrcmp(mydir_entry.name,dir[j]) && myinode.type==1){
                    break;
                }
            }
        }
        if(m==512){
            cur_dir = init_dir;
            return 0;
        }
        cur_dir = mydir_entry.inode;
    }
    return 1;
}
int do_mkdir(char* directory){
    int cur_dir_index;
    cur_dir_index = find_cur_dir();
    //ensure that the directory name has not been used
    int i;
    dir_entry_t mydir_entry;
    for(i=0;i<512;i+=32){
        if(sdbuffer[cur_dir_index][i]!=0){
            kmemcpy(&mydir_entry,&sdbuffer[cur_dir_index][i],32);
            if(!kstrcmp(mydir_entry.name,directory)){
                return 0;
            }
        }
    }
    int new_inode;
    new_inode = create_inode(directory,1,1,cur_dir);
    //set new entry in father dir
    dir_entry_t new_dir;
    kstrcpy(new_dir.name, directory);
    new_dir.inode = new_inode;
    new_dir.type = 1;
    set_dir_entry(cur_dir,&new_dir);
    
    return 1;
}
int do_rmdir(char* directory){
    int cur_dir_index;
    cur_dir_index = find_cur_dir();
    //ensure that the directory name has existed
    int i;
    dir_entry_t mydir_entry;
    for(i=0;i<512;i+=32){
        if(sdbuffer[cur_dir_index][i]!=0){
            kmemcpy(&mydir_entry,&sdbuffer[cur_dir_index][i],32);
            if(!kstrcmp(mydir_entry.name,directory)){
                sdbuffer[cur_dir_index][i] = 0;//delete this entry
                store_sd(cur_dir_index);
                break;
            }
        }
    }
    if(i==512) return 0;
    release_inode(mydir_entry.inode);
    return 1;
}
int do_ls(char* directories){
    //parse the path
    char dir[10][20];
    int index=0;
    int i=0;
    int j=0;
    if (!(*directories)) {
        i=-1;
    } else {
        while(directories[index] != '\0'){
            for(;directories[index]!='/' && directories[index]!='\0';index++){
                dir[i][j++] = directories[index];
            }
            dir[i][j] = '\0';
            if(directories[index] == '/'){
                index++;
                i++;
                j=0;
            }
        }
    }
    int init_dir = cur_dir;
    //cd one by one
    int cur_dir_index;
    dir_entry_t mydir_entry;
    int m;
    for(j=0;j<=i;j++){//path depth
        cur_dir_index = find_cur_dir();
        //ensure that the directory name has existed
        for(m=0;m<512;m+=32){
            if(sdbuffer[cur_dir_index][m]!=0){
                kmemcpy(&mydir_entry,&sdbuffer[cur_dir_index][m],32);
                if(!kstrcmp(mydir_entry.name,dir[j])){
                    break;
                }
            }
        }
        if(m==512){
            cur_dir = init_dir;
            return 0;
        }
        cur_dir = mydir_entry.inode;
    }
    //ls
    cur_dir_index = find_cur_dir();
    for(m=0;m<512;m+=32){
        if(sdbuffer[cur_dir_index][m]!=0){
            kmemcpy(&mydir_entry,&sdbuffer[cur_dir_index][m],32);
            prints("%s ",mydir_entry.name);
        }
    }
    prints("\n");
    cur_dir = init_dir;
    return 1;
}

int do_ln(char* dst,char* src){
    char src_path[10][20];
    char dst_path[10][20];
    int index=0;
    int i=0;
    int j=0;
    while(src[index] != '\0'){
        for(;src[index]!='/' && src[index]!='\0';index++){
            src_path[i][j++] = src[index];
        }
        src_path[i][j] = '\0';
        if(src[index] == '/'){
            index++;
            i++;
            j=0;
        }
    }
    int src_depth = i;
    index=0;
    i=0;
    j=0;
    while(dst[index] != '\0'){
        for(;dst[index]!='/' && dst[index]!='\0';index++){
            dst_path[i][j++] = dst[index];
        }
        dst_path[i][j] = '\0';
        if(dst[index] == '/'){
            index++;
            i++;
            j=0;
        }
    }
    int dst_depth = i;
    int init_dir = cur_dir;
    int m;

    //get the dst file's inode number
    int cur_dir_index;
    dir_entry_t mydir_entry;
    for(j=0;j<=dst_depth;j++){//path depth
        cur_dir_index = find_cur_dir();
        //ensure that the directory name has existed
        for(m=0;m<512;m+=32){
            if(sdbuffer[cur_dir_index][m]!=0){
                kmemcpy(&mydir_entry,&sdbuffer[cur_dir_index][m],32);
                if(!kstrcmp(mydir_entry.name,dst_path[j])){
                    break;
                }
            }
        }
        if(m==512){
            cur_dir = init_dir;
            return 0;
        }
        cur_dir = mydir_entry.inode;
    }

    //recover the cur_dir 
    cur_dir = init_dir;

    int dst_inode = mydir_entry.inode;
  
    //create a new file(src_file) link to the dst_inode
    //find the src(new) file dir
    for(j=0;j<src_depth;j++){//path depth
        cur_dir_index = find_cur_dir();
        //ensure that the directory name has existed
        for(m=0;m<512;m+=32){
            if(sdbuffer[cur_dir_index][m]!=0){
                kmemcpy(&mydir_entry,&sdbuffer[cur_dir_index][m],32);
                if(!kstrcmp(mydir_entry.name,src_path[j])){
                    break;
                }
            }
        }
        if(m==512){
            cur_dir = init_dir;
            return 0;
        }
        cur_dir = mydir_entry.inode;
    }
    //set up the new file
    cur_dir_index = find_cur_dir();
    //ensure that the file name has not been used
    for(m=0;m<512;m+=32){
        if(sdbuffer[cur_dir_index][m]!=0){
            kmemcpy(&mydir_entry,&sdbuffer[cur_dir_index][m],32);
            if(!kstrcmp(mydir_entry.name,src_path[src_depth])){      
                return 0;
            }
        }
    }
    //set new entry in src_file dir(using dst inode)
    dir_entry_t new_dir;
    kstrcpy(new_dir.name, src_path[src_depth]);
    new_dir.inode = dst_inode;
    new_dir.type = 0;//file
    set_dir_entry(cur_dir,&new_dir);
    
    //updata the dst file's hard_link number
    int cur_inode_index;
    cur_inode_index = load_sd(INODE + (dst_inode >> 2));
    inode_t * myinode = (inode_t*)&sdbuffer[cur_inode_index][(dst_inode & 0x3) << 7];
    myinode->hard_link++;
    store_sd(cur_inode_index);

    //recover the dir
    cur_dir = init_dir;
    return 1;
}
//print detail information of files
int do_ls_l(char* directories){
    //parse the path
    char dir[10][20];
    int index=0;
    int i=0;
    int j=0;
    if (!(*directories)) {
        i=-1;
    } else {
        while(directories[index] != '\0'){
            for(;directories[index]!='/' && directories[index]!='\0';index++){
                dir[i][j++] = directories[index];
            }
            dir[i][j] = '\0';
            if(directories[index] == '/'){
                index++;
                i++;
                j=0;
            }
        }
    }
    int init_dir = cur_dir;
    //cd one by one
    int cur_dir_index;
    dir_entry_t mydir_entry;
    int m;
    for(j=0;j<=i;j++){//path depth
        cur_dir_index = find_cur_dir();
        //ensure that the directory name has existed
        for(m=0;m<512;m+=32){
            if(sdbuffer[cur_dir_index][m]!=0){
                kmemcpy(&mydir_entry,&sdbuffer[cur_dir_index][m],32);
                if(!kstrcmp(mydir_entry.name,dir[j])){
                    break;
                }
            }
        }
        if(m==512){
            cur_dir = init_dir;
            return 0;
        }
        cur_dir = mydir_entry.inode;
    }
    //ls
    int cur_inode_index;
    cur_dir_index = find_cur_dir();
    for(m=0;m<512;m+=32){
        if(sdbuffer[cur_dir_index][m]!=0){
            kmemcpy(&mydir_entry,&sdbuffer[cur_dir_index][m],32);
            if(mydir_entry.type == 0){//file
                prints("%s ",mydir_entry.name);
                cur_inode_index = load_sd(INODE + (mydir_entry.inode >> 2));
                inode_t * myinode = (inode_t*)&sdbuffer[cur_inode_index][(mydir_entry.inode & 0x3) << 7];
                prints("inode number: %d  ", mydir_entry.inode);
                prints("hard_link: %d  ",myinode->hard_link);
                prints("size: %d\n",myinode->size*512);
            }
        }
    }
    cur_dir = init_dir;
    return 1;
}
int do_rm(char* filename){
    int cur_dir_index;
    cur_dir_index = find_cur_dir();
    //ensure that the file name has existed
    int i;
    dir_entry_t mydir_entry;
    for(i=0;i<512;i+=32){
        if(sdbuffer[cur_dir_index][i]!=0){
            kmemcpy(&mydir_entry,&sdbuffer[cur_dir_index][i],32);
            if(!kstrcmp(mydir_entry.name,filename)){
                sdbuffer[cur_dir_index][i] = 0;//delete this entry
                store_sd(cur_dir_index);
                break;
            }
        }
    }
    if(i==512){
        return 0;
    }
    //release the inode
    int cur_inode_index;
    cur_inode_index = load_sd(INODE + (mydir_entry.inode >> 2));
    inode_t * myinode = (inode_t*)&sdbuffer[cur_inode_index][(mydir_entry.inode & 0x3) << 7];
    myinode->hard_link--;
    store_sd(cur_inode_index);
    //if hard_link = 0,release inode
    if(myinode->hard_link==0){
        release_inode(mydir_entry.inode);
    }
    return 1;
}

int do_touch(char* filename){
    int m;
    int cur_dir_index;
    cur_dir_index = find_cur_dir();
    dir_entry_t mydir_entry;
    //ensure that the directory name has not been used
    for(m=0;m<512;m+=32){
        if(sdbuffer[cur_dir_index][m]!=0){
            kmemcpy(&mydir_entry,&sdbuffer[cur_dir_index][m],32);
            if(!kstrcmp(mydir_entry.name,filename)){
                return 0;
            }
        }
    }
    int new_inode;
    new_inode = create_inode(filename,0,0,0);
    //set new entry in father dir
    dir_entry_t new_dir;
    kstrcpy(new_dir.name, filename);
    new_dir.inode = new_inode;
    new_dir.type = 0;//file
    set_dir_entry(cur_dir,&new_dir);

    return 1;
}
int do_cat(char* filename){
    int m;
    int cur_dir_index;
    cur_dir_index = find_cur_dir();
    dir_entry_t mydir_entry;
    for(m=0;m<512;m+=32){
        if(sdbuffer[cur_dir_index][m]!=0){
            kmemcpy(&mydir_entry,&sdbuffer[cur_dir_index][m],32);
            if(!kstrcmp(mydir_entry.name,filename)){
                break;
            }
        }
    }
    if (m == 512) return 0;
    int cur_inode_index;
    cur_inode_index = load_sd(INODE + (mydir_entry.inode >> 2));
    inode_t * myinode = (inode_t*)&sdbuffer[cur_inode_index][(mydir_entry.inode & 0x3) << 7];
    int sd_num;
    int sd_index;
    int sd_indir_index;
    int sd_second_index;
    int *sd_indir;
    int *sd_second;
    int *sd_third;
    int i,j,k,t;
    //direct pointer
    for(i=0;i<7;i++){
        sd_num = myinode->direct_pointer[i];
        if(sd_num!=0){
            cat_context(sd_num);
        }
    }
    //indirect pointer
    for(i=0;i<3;i++){
        sd_num = myinode->indirect_pointer[i];
        if(sd_num!=0){
            sd_index = load_sd(sd_num + DATA);
            for(j=0;j<512;j+=4){
                sd_indir = (int*)&sdbuffer[sd_index][j];
                if(*sd_indir!=0){
                    cat_context(*sd_indir);
                }
            }
        }
    }
    //second pointer
    for(i=0;i<2;i++){
        sd_num = myinode->second_pointer[i];
        if(sd_num!=0){
            sd_index = load_sd(sd_num + DATA);
            for(j=0;j<512;j+=4){
                sd_indir = (int*)&sdbuffer[sd_index][j];
                if(*sd_indir!=0){
                    sd_indir_index = load_sd(*sd_indir + DATA);
                    for(k=0;k<512;k+=4){
                        sd_second = (int*)&sdbuffer[sd_indir_index][k];
                        if(*sd_second!=0){
                            cat_context(*sd_second);
                        }
                    }
                }
            }
        }
    }
    //third pointer
    sd_num = myinode->third_pointer;
    if(sd_num!=0){
        sd_index = load_sd(sd_num + DATA);
        for(j=0;j<512;j+=4){
            sd_indir = (int*)&sdbuffer[sd_index][j];
            if(*sd_indir!=0){
                sd_indir_index = load_sd(*sd_indir + DATA);
                for(k=0;k<512;k+=4){
                    sd_second = (int*)&sdbuffer[sd_indir_index][k];
                    if(*sd_second!=0){
                        sd_second_index = load_sd(*sd_second + DATA);
                        for(t=0;t<512;t+=4){
                            sd_third = (int*)&sdbuffer[sd_second_index][t];
                            if(*sd_third){
                                cat_context(*sd_third);
                            }
                        }
                    }
                }
            }
        }
    }

    return 1;
}
int do_lseek(int fd,int offset,int whence){
    //seek_set
    if(whence == 0){
        fdbuffer[fd].pos = offset;
    }else if(whence == 1){
        fdbuffer[fd].pos += offset;
    }else if(whence == 2){
        int cur_inode_index;
        cur_inode_index = load_sd(INODE + (fdbuffer[fd].inode >> 2));
        inode_t * myinode = (inode_t*)&sdbuffer[cur_inode_index][(fdbuffer[fd].inode & 0x3) << 7];
        fdbuffer[fd].pos = myinode->size*512 + offset; 
    }
    return 1;
}
int do_fopen(char* filename,int mode){
    //search for the file
    int m;
    int cur_dir_index;
    cur_dir_index = find_cur_dir();
    dir_entry_t mydir_entry;
    for(m=0;m<512;m+=32){
        if(sdbuffer[cur_dir_index][m]!=0){
            kmemcpy(&mydir_entry,&sdbuffer[cur_dir_index][m],32);
            if(!kstrcmp(mydir_entry.name,filename)){
                break;
            }
        }
    }
    if (m == 512) return -1;
    int i;
    for(i=0;i<FDNUM;i++){
        if(fdbuffer[i].inode==0){
            break;
        }
    }
    fdbuffer[i].inode = mydir_entry.inode;
    fdbuffer[i].mode = mode;
    fdbuffer[i].pos = 0;
    return i;
}
void write_context(int begin,int sd_num,char* context,int length){
    int sd_index;
    sd_index = load_sd(DATA+sd_num);
    memcpy(&sdbuffer[sd_index][begin],context,length);
    store_sd(sd_index);
}
void read_context(int begin,int sd_num,char* buff,int length){
    int sd_index;
    sd_index = load_sd(DATA+sd_num);
    memcpy(buff,&sdbuffer[sd_index][begin],length);
}
int do_fwrite(int fd, char* context, int length){
    if(fdbuffer[fd].mode == O_RDONLY){
        return 0;
    }
    int sd_count = (fdbuffer[fd].pos)/512;
    int tmp_length = length;
    int tmp_sd = sd_count;
    int write_byte;

    int sd_indir_index;
    int sd_second_index;
    int sd_third_index;
    int i,j,k;
    int* tmp;
    //dir:7 sectors
    //indir:3*128 sectors
    //second:2*128*128 sectors
    //third:128*128*128 sectors
    int sd_num;
    while(tmp_length>0){
        int cur_inode_index = load_sd(INODE + (fdbuffer[fd].inode >> 2));
        inode_t * myinode = (inode_t*)&sdbuffer[cur_inode_index][(fdbuffer[fd].inode & 0x3) << 7];
        if(tmp_sd<7){
            sd_num = myinode->direct_pointer[tmp_sd];
            if(sd_num == 0){
                myinode->direct_pointer[tmp_sd] = search_for_sector();
                myinode->size++;
                store_sd(cur_inode_index);
                sd_num = myinode->direct_pointer[tmp_sd];
            }
            write_byte = (tmp_length >(512-(fdbuffer[fd].pos)%512)) ? (512-(fdbuffer[fd].pos)%512) : tmp_length;
            write_context((fdbuffer[fd].pos)%512,sd_num,context,write_byte);
            tmp_length -= write_byte;
            context += write_byte;
            fdbuffer[fd].pos += write_byte;
            tmp_sd++;
        }else if(tmp_sd<(3*128+7)){
            int ip_num = (tmp_sd-7)/128;
            sd_num = myinode->indirect_pointer[ip_num];
            if(sd_num == 0){
                myinode->indirect_pointer[ip_num] = search_for_sector();
                myinode->size++;
                store_sd(cur_inode_index);
                sd_num = myinode->indirect_pointer[ip_num];
            }
            int num_entry = tmp_sd - 7 - ip_num * 128;
            sd_indir_index = load_sd(DATA+sd_num);
            tmp = (int *) sdbuffer[sd_indir_index] + num_entry;
            if(*tmp==0){
                *tmp = search_for_sector();
                store_sd(sd_indir_index);
                myinode->size++;
                store_sd(cur_inode_index);
            }
            write_byte = (tmp_length >(512-(fdbuffer[fd].pos)%512)) ? (512-(fdbuffer[fd].pos)%512) : tmp_length;
            write_context((fdbuffer[fd].pos)%512,*tmp,context,write_byte);
            tmp_length -= write_byte;
            context += write_byte;
            fdbuffer[fd].pos += write_byte;
            tmp_sd++;
        }else if(tmp_sd<(2*128*128+3*128+7)){
            int sp_num = (tmp_sd-7-3*128) / (128*128);
            sd_num = myinode->second_pointer[sp_num];
            if(sd_num == 0){
                myinode->second_pointer[sp_num] = search_for_sector();
                store_sd(cur_inode_index);
                sd_num = myinode->second_pointer[sp_num];
                myinode->size++;
                store_sd(cur_inode_index);
            }
            int num_entry = (tmp_sd - 7 - 3*128 - sp_num*128*128) / 128;
            sd_indir_index = load_sd(DATA+sd_num);
            tmp = (int *) sdbuffer[sd_indir_index] + num_entry;
            if(*tmp==0){
                *tmp = search_for_sector();
                store_sd(sd_indir_index);
                myinode->size++;
                store_sd(cur_inode_index);
            }
            int second_entry = (tmp_sd - 7 - 3*128 - sp_num*128*128) % 128;
            sd_second_index = load_sd(DATA+*tmp);
            tmp = (int *) sdbuffer[sd_second_index] + second_entry;
            if(*tmp==0){
                *tmp = search_for_sector();
                store_sd(sd_second_index);
                myinode->size++;
                store_sd(cur_inode_index);
            }

            write_byte = (tmp_length >(512-(fdbuffer[fd].pos)%512)) ? (512-(fdbuffer[fd].pos)%512) : tmp_length;
            write_context((fdbuffer[fd].pos)%512,*tmp,context,write_byte);
            tmp_length -= write_byte;
            context += write_byte;
            fdbuffer[fd].pos += write_byte;
            tmp_sd++;
        }else if(tmp_sd<(128*128*128+2*128*128+3*128+7)){
            
            sd_num = myinode->third_pointer;
            if(sd_num == 0){
                myinode->third_pointer = search_for_sector();
                store_sd(cur_inode_index);
                sd_num = myinode->third_pointer;
                myinode->size++;
                store_sd(cur_inode_index);
            }
            int num_entry = (tmp_sd - 7 - 3*128 - 2*128*128) / (128*128);
            sd_indir_index = load_sd(DATA+sd_num);
            tmp = (int *) sdbuffer[sd_indir_index] + num_entry;
            if(*tmp==0){
                *tmp = search_for_sector();
                store_sd(sd_indir_index);
                myinode->size++;
                store_sd(cur_inode_index);
            }
            int second_entry = (tmp_sd - 7 - 3*128 - 2*128*128 - num_entry*128*128) / 128;
            sd_second_index = load_sd(DATA+*tmp);
            tmp = (int *) sdbuffer[sd_second_index] + second_entry;
            if(*tmp==0){
                *tmp = search_for_sector();
                store_sd(sd_second_index);
                myinode->size++;
                store_sd(cur_inode_index);
            }
            int third_entry = (tmp_sd - 7 - 3*128 - 2*128*128 - num_entry*128*128) % 128;
            sd_third_index = load_sd(DATA+*tmp);
            tmp = (int *) sdbuffer[sd_third_index] + third_entry;
            if(*tmp==0){
                *tmp = search_for_sector();
                store_sd(sd_third_index);
                myinode->size++;
                store_sd(cur_inode_index);
            }
            write_byte = (tmp_length >(512-(fdbuffer[fd].pos)%512)) ? (512-(fdbuffer[fd].pos)%512) : tmp_length;
            write_context((fdbuffer[fd].pos)%512,*tmp,context,write_byte);
            tmp_length -= write_byte;
            context += write_byte;
            fdbuffer[fd].pos += write_byte;
            tmp_sd++;
        }
    }
    return 1;
}
int do_fread(int fd, char *buff, int length){
    if(fdbuffer[fd].mode == O_WRONLY){
        return 0;
    }
    int sd_count = (fdbuffer[fd].pos)/512;
    int tmp_length = length;
    int tmp_sd = sd_count;
    int read_byte;

    int sd_indir_index;
    int sd_second_index;
    int sd_third_index;
    int i,j,k;
    //dir:7 sectors
    //indir:3*128 sectors
    //second:2*128*128 sectors
    //third:128*128*128 sectors
    int sd_num;
    while(tmp_length>0){
        int cur_inode_index = load_sd(INODE + (fdbuffer[fd].inode >> 2));
        inode_t * myinode = (inode_t*)&sdbuffer[cur_inode_index][(fdbuffer[fd].inode & 0x3) << 7];
        if(tmp_sd<7){
            sd_num = myinode->direct_pointer[tmp_sd];
            read_byte = (tmp_length >(512-(fdbuffer[fd].pos)%512)) ? (512-(fdbuffer[fd].pos)%512) : tmp_length;
            read_context((fdbuffer[fd].pos)%512,sd_num,buff,read_byte);
            tmp_length -= read_byte;
            buff += read_byte;
            fdbuffer[fd].pos += read_byte;
            tmp_sd++;
        }else if(tmp_sd<(3*128+7)){
            int ip_num = (tmp_sd-7)/128;
            sd_num = myinode->indirect_pointer[ip_num];
           
            int num_entry = tmp_sd - 7 - ip_num * 128;
            sd_indir_index = load_sd(DATA+sd_num);
            
            read_byte = (tmp_length >(512-(fdbuffer[fd].pos)%512)) ? (512-(fdbuffer[fd].pos)%512) : tmp_length;
            read_context((fdbuffer[fd].pos)%512,((int*)sdbuffer[sd_indir_index])[num_entry],buff,read_byte);
            tmp_length -= read_byte;
            buff += read_byte;
            fdbuffer[fd].pos += read_byte;
            tmp_sd++;
        }else if(tmp_sd<(2*128*128+3*128+7)){
            int sp_num = (tmp_sd-7-3*128) / (128*128);
            sd_num = myinode->second_pointer[sp_num];
            
            int num_entry = (tmp_sd - 7 - 3*128 - sp_num*128*128) / 128;
            sd_indir_index = load_sd(DATA+sd_num);
            
            int second_entry = (tmp_sd - 7 - 3*128 - sp_num*128*128) % 128;
            sd_second_index = load_sd(DATA+((int*)sdbuffer[sd_indir_index])[num_entry]);

            read_byte = (tmp_length >(512-(fdbuffer[fd].pos)%512)) ? (512-(fdbuffer[fd].pos)%512) : tmp_length;
            read_context((fdbuffer[fd].pos)%512,((int*)sdbuffer[sd_second_index])[second_entry],buff,read_byte);
            tmp_length -= read_byte;
            buff += read_byte;
            fdbuffer[fd].pos += read_byte;
            tmp_sd++;
        }else if(tmp_sd<(128*128*128+2*128*128+3*128+7)){
            
            sd_num = myinode->third_pointer;
            
            int num_entry = (tmp_sd - 7 - 3*128 - 2*128*128) / (128*128);
            sd_indir_index = load_sd(DATA+sd_num);
            
            int second_entry = (tmp_sd - 7 - 3*128 - 2*128*128 - num_entry*128*128) / 128;
            sd_second_index = load_sd(DATA+((int*)sdbuffer[sd_indir_index])[num_entry]);
            
            int third_entry = (tmp_sd - 7 - 3*128 - 2*128*128 - num_entry*128*128) % 128;
            sd_third_index = load_sd(DATA+((int*)sdbuffer[sd_second_index])[second_entry]);
            
            read_byte = (tmp_length >(512-(fdbuffer[fd].pos)%512)) ? (512-(fdbuffer[fd].pos)%512) : tmp_length;
            read_context((fdbuffer[fd].pos)%512,((int*)sdbuffer[sd_third_index])[third_entry],buff,read_byte);
            tmp_length -= read_byte;
            buff += read_byte;
            fdbuffer[fd].pos += read_byte;
            tmp_sd++;
        }
    }

    return 1;
}
int do_close(int fd){
    fdbuffer[fd].inode = 0;
    return 1;
}