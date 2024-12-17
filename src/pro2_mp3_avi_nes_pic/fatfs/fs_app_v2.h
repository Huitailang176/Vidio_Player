#ifndef  __FS_APP_V2_H_
#define  __FS_APP_V2_H_

//路径
#define  DIRECTORY  0

//音频
#define  MP3_FILE   0x10
#define  WAV_FILE   0x11
#define  AAC_FILE   0x12

//视频
#define  AVI_FILE   0x20
#define  MP4_FILE   0x21
#define  FLV_FILE   0x22

//NES游戏
#define  NES_FILE   0x30

//图片
#define  JPG_FILE   0x40
#define  BMP_FILE   0x41

//未知文件类型
#define  UNKNOW_FILE  0xFF

typedef struct fil_obj_s
{
	char fname[68];      //文件名
	int  ftype;	         //文件类型
	int  size;           //大小
}fil_obj_t;

//单向非循环链表
typedef struct dir_list_s
{
	fil_obj_t fil_obj;
	struct dir_list_s *next;
}dir_list_t;


int  get_file_obj_type(const char *fname);
int  fname_str_compare(const char *fname1, const char *fname2);

dir_list_t *dir_list_create(const char *path);
int  dir_traverse_obj(dir_list_t *dir_list);
int  dir_get_obj_num(dir_list_t *dir_list);
int  dir_read_obj_info(dir_list_t *dir_list, int skip, fil_obj_t *out, int num);
void dir_list_destroy(dir_list_t *dir_list);
void dir_list_output(const char *path, void (*out_func)(void *, int));
int  dir_test_expect_obj(const char *path, int n);

#define  ORDER_SEQ  0
#define  ORDER_REV  1
int  dir_check_list(const char *path, int mode, int order);
int  dir_check_list_child(const char *path, int order);


//==================================================================
#define  DIRECT_DIR  0
#define  SUB_DIR     1
typedef struct dir_obj_s
{
	dir_list_t *dir_list;   //目录item链表

	int item_idx;           //目录item索引
	int item_cnt;           //目录item个数

	char dir_name[64];      //路径名, 比如 /, /sys/font, /music等
	int  dir_type;          //路径类型 0-直接目录(如/music) 1-子目录(比如/music/ZhouJielun)
}dir_obj_t;


dir_obj_t *dir_open(const char *dir_path);
void dir_close(dir_obj_t *pdir);
void dir_back(char *dir_name);
void dir_increase(char *dir_name, char *subdir);
int dir_direct_judge(char *dir_name);


#endif



