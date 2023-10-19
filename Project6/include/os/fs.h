
#ifndef INCLUDE_FS_H_
#define INCLUDE_FS_H_

#include <context.h>

#include <type.h>
#include <os/list.h>
#include <os/mm.h>
#include <type.h>
#include <context.h>

void check_fs();
int do_mkfs();
int do_statfs();
int do_cd(char* directories);
int do_mkdir(char* directory);
int do_rmdir(char* directory);
int do_ls(char* directories);
int do_ln(char* dir,char* src);
int do_ls_l(char* filename);
int do_rm(char* filename);
int do_lseek(int fd,int offset,int whence);
int do_touch(char* filename);
int do_cat(char* filename);
int do_fopen(char* filename,int mode);
int do_fwrite(int fd, char* context, int length);
int do_fread(int fd, char *buff, int length);
int do_close(int fd);
#endif