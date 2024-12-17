#ifndef  __DIR_LIST_H_
#define  __DIR_LIST_H_

//加载dir目录列表界面
int dir_list_form_load(char *dir, char *title);

//音乐/游戏等app结束后, 重新返回dir列表界面
void dir_list_form_reload(void);

//音乐/图片等app界面下, 根据mode自动运行list item
int dir_list_item_auto_run(int mode, char **fname_out);

//获取当前选中的item文件名
int dir_list_get_cur_item(char **fname_out);


#endif


