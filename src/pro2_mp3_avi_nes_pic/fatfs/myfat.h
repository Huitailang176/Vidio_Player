#ifndef  __MYFAT_H_
#define  __MYFAT_H_


#include "ff.h"

int fatfs_init(void);

int mount_sd(void);
int unmount_sd(void);
int change_drv(int n);

//===========================================
int get_cur_dir(char *buff, int len);
int make_dir(const char *dir, int mode);
int remove_dir(const char *path, int mode);
int is_dir_exist(const char *dir);
int is_fil_exist(const char *fil);
int disk_free(int unit);

int relocate_file_wr_ptr(FIL *fil, int percent);
int get_file_wr_ptr(FIL *fil);


#endif



