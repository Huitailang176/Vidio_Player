#include "myfat.h"
#include "rtc_v2.h"
#include "my_printf.h"
#include "libc.h"
#include "time.h"
#include "uart.h"
#include "sdio_sdcard_v5.h"
#include "spi_flash_v4.h"

/* 磁盘id-当前使用的分区号 */
#if _MULTI_PARTITION
PARTITION VolToPart[_VOLUMES] =
{
	{0, 1},     //磁盘0的第1个分区作为格式化(挂载)为fatfs文件系统
	{1, 1},     //磁盘1的第1个分区作为格式化(挂载)为fatfs文件系统
};
#endif


#if !_FS_READONLY && !_FS_NORTC

//User defined function to give a current time to fatfs module      */
//31-25: Year(0-127 org.1980), 24-21: Month(1-12), 20-16: Day(1-31) */
//15-11: Hour(0-23), 10-5: Minute(0-59), 4-0: Second(0-29 *2) */
/* RTC function */
DWORD get_fattime (void)	/* 返回UTC时间, 即零时区 */
{
	DWORD time = 0;
	DWORD tmp = 0;

	int t[6];
	get_time(t, sizeof(t));

	tmp = t[5] - 1980;       //Year
	time |= (tmp<<25) & 0xFE000000;

	tmp = t[4];             //month
	time |= (tmp<<21) & 0x01E00000;

	tmp = t[3];             //date
	time |= (tmp<<16) & 0x001F0000;

	tmp = t[2];             //hour
	time |= (tmp<<11) & 0x0000F800;

	tmp = t[1];             //min
	time |= (tmp<<5) & 0x000007E0;

	tmp = t[1]/2;           //second
	time |= tmp & 0x0000001F;

	return time;
}
#endif

//get_fattime()的反函数   fat文件的时间戳采用的是UTC时间, 显示时转化为北京时间(UTC+8)
int fattime2rtc(DWORD fattime, int *t, int len)
{
	if(len < 6*sizeof(int))
		return -1;

	t[5] = ((fattime&0xFE000000)>>25) + 1980;  //year
	t[4] = (fattime&0x01E00000) >> 21;         //month
	t[3] = (fattime&0x001F0000) >> 16;         //date

	t[2] = (fattime&0x0000F800) >> 11;         //hour
	t[1] = (fattime&0x000007E0) >> 5;          //minute
	t[0] = (fattime&0x0000001F) >> 0;          //second

	//由于文件的修改时间戳采用的是UTC时间(即零时区), 需要转换为北京时间
//	utc2bj(t, len);

	return 0;
}

//====================================================================================================
FATFS sys_fs[_VOLUMES];
DWORD drv0_partition[4] = {100, 0, 0, 0};   /* 磁盘0的分区情况: 主分区1占磁盘100%, 其余3个分区未使用 */
DWORD drv1_partition[4] = {100, 0, 0, 0};   /* 磁盘1的分区情况: 主分区1占磁盘100%, 其余3个分区未使用 */

extern int uni_oem_data_check(void);   //在cc936.c文件中

/*
 * @breif: 挂载磁盘并初始化文件系统
 */
int fatfs_init(void)
{
	int ret = FR_OK;
	
	ret = uni_oem_data_check();
	if(0 != ret) {
		my_printf("\n\rSPI Flash Data check fail\n\r");
		return ret;
	}

	ret = f_mount(&sys_fs[0], "0:", 1);  //挂载磁盘0
	if(ret == FR_OK)   //挂载磁盘上的文件系统成功
	{	my_printf("\n\rMount disk0 fatfs success ^_^\n\r");	}
	else               //挂载磁盘上的文件系统失败
	{
		my_printf("\n\rMount disk0 fatfs fail. Now, parting the disk and formating fatfs, ret=%d\n\r", ret);

		uint8_t work_buff[_MAX_SS];
		ret = f_fdisk(0, drv0_partition, work_buff, 1024*256/_MAX_SS);  //给磁盘0创建MBR分区
		if(ret != FR_OK) {
			my_printf("Disk0 create patition fail, ret=%d\n\r", ret);
			return ret;
		}

		ret = f_mkfs("0:", FM_ANY, 0, work_buff, sizeof(work_buff));
		if(ret != FR_OK) {
			my_printf("Disk0 create fatfs fail, ret=%d\n\r", ret);
			return ret;
		}

		ret = f_setlabel ("0:SF_DISK");	/* Set volume label */
		if(ret != FR_OK) {
			my_printf("Disk0 set label fail, ret=%d\n\r", ret);
			return ret;
		}
		my_printf("Disk0 formated fatfs success ^_^\n\r");
	}

	return ret;
}

/*
 * @breif: 挂载SD卡
 */
int mount_sd(void)
{
	int ret = FR_OK;

	if(_VOLUMES == 1) {
		my_printf("System don't support SD disk!!\n\r");
		return 1;
	}
	
//	sdio_controller_init();    //no need
	ret = sd_init();           //检测SD卡是否存在
	if( ret ) {
		return 2;
	}

	ret = f_mount(&sys_fs[1], "1:", 1);  //挂载磁盘1
	if(ret == FR_OK)   //挂载磁盘上的文件系统成功
	{	my_printf("\n\rMount disk1 fatfs success ^_^\n\r");	}
	else               //挂载磁盘上的文件系统失败
	{
		my_printf("\n\rMount disk1 fatfs fail. Now, parting the disk and formating fatfs, ret=%d\n\r", ret);

		uint8_t work_buff[_MAX_SS];
		ret = f_fdisk(1, drv1_partition, work_buff, 1024*256/_MAX_SS);  //给磁盘1创建MBR分区
		if(ret != FR_OK) {
			my_printf("Disk1 create patition fail, ret=%d\n\r", ret);
			return ret;
		}

		ret = f_mkfs("1:", FM_ANY, 0, work_buff, sizeof(work_buff));
		if(ret != FR_OK) {
			my_printf("Disk1 create fatfs fail, ret=%d\n\r", ret);
			return ret;
		}

		ret = f_setlabel ("1:SD_DISK");	/* Set volume label */
		if(ret != FR_OK) {
			my_printf("Disk1 set label fail, ret=%d\n\r", ret);
			return ret;
		}
		my_printf("Disk1 formated fatfs success ^_^\n\r");
	}

	return 0;	
}

/*
 * @breif: 卸载SD卡
 */
int unmount_sd(void)
{
	if(_VOLUMES == 1) {
		my_printf("System don't support SD disk!!\n\r");
		return 0;
	}

	int ret = 0;
	
	if(sys_fs[1].fs_type == 0) {
		my_printf("SD not mounted, can't unmount!!\n\r");
		return 0;
	}

	ret = f_chdrive("0:/");  //切换为磁盘0
	if(ret == FR_OK) {
		SD_PowerOFF();   //关闭SDIO的CLK输出
		u_memset(&sys_fs[1], 0x00, sizeof(sys_fs[1]));
	}
	
	if( ret ) {
		my_printf("unmount sd fail, ret=%d\n\r", ret);
	}
	
	return 0;
}

/*
 * @breif: 改变磁盘驱动器(挂载SD才有使用意义)
 */
int change_drv(int n)
{
	int ret = 0;
	if(_VOLUMES == 1)
	{
		my_printf("The system only mount 1 disk, can't change the disk drive");
		ret = 1;
	}
	else
	{
		if(sys_fs[1].fs_type == 0) {
			my_printf("SD not mounted, can't change drive!\n\r");
			return 2;
		}

		if(n == 0)
			ret = f_chdrive("0:/");  //切换为磁盘0 SPI Flash
		else if(n == 1)
			ret = f_chdrive("1:/");  //切换为磁盘1 SD Card
		else
			ret = -1;

		if(ret > 0) {
			my_printf("change disk drive fail, ret=%d\n\r", ret);
		}
	}

	return ret;	
}

//=============================================
/*
 * @brief: 获取当前工作路径
 */
int get_cur_dir(char *buff, int len)
{
	int ret = 0;
	ret = f_getcwd (buff, len);
	return ret;
}

/*
 * @brief: 创建目录
 * @param: mode 是否递归创建
 * 				0-不递归
 * 				1-递归创建,相当于 -p 参数
 */
int make_dir(const char *dir, int mode)
{
	int ret = 0;
	char buff[256];
	int i, cnt;
	FILINFO fno;

	if(mode == 0)
	{
		ret = f_mkdir(dir);
	}
	else
	{
		if(u_strlen(dir)+1 > sizeof(buff))
			return -1;

		cnt = u_strlen(dir);
		for(i=0; i<cnt; i++)
		{
			if(dir[i]=='/' && i>0 && dir[i-1]!='.') {
				u_memcpy(buff, dir, i);
				buff[i] = '\0';           //字符串结尾
				ret = f_stat(buff, &fno); //Check existance of a file or sub-directory
				if(ret == FR_NO_FILE)
					ret = f_mkdir(buff);
				if( ret )
					break;
			}
		}
		if(ret == FR_OK) {
			ret = f_mkdir(dir);
		}
	}

	return ret;
}

//使用递归算法来删除一个路径下所有的文件, 然后再删除该目录
static int clear_dir(char *path);
static int clear_dir(char *path)     //递归算法若递归的次数很多, 将消耗大量栈内存, 注意栈的溢出
{
    int res;      //这些变量全都在栈内存中, 每递归一次都要在栈中开辟相应的空间,
    DIR dir;      //return回溯时释放栈空间
    UINT i, j;
    FILINFO fno;

    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if (fno.fattrib & AM_DIR) {                    /* It is a directory */
                i = u_strlen(path);
                u_sprintf(&path[i], "/%s", fno.fname);
                res = clear_dir(path);                    /* Enter the directory */
                if (res != FR_OK) break;
                path[i] = 0;
            } else {                                       /* It is a file. */
            	j = u_strlen(path);
            	u_sprintf(&path[j], "/%s", fno.fname);
//                my_printf("del fil: %s \n\r", path);   //debug
                res = f_unlink(path);      //删除一个文件
                path[j] = 0;
                if(res )
                	break;
            }
        }
        res = f_closedir(&dir);
        if(res == FR_OK) {
//        	my_printf("del dir: %s \n\r", path);    //debug
        	res = f_unlink(path);          //删除一个空目录
        }
    }

    return res;    //正常情况下从此处回溯
}
/*
 * @brief: Remove a file or sub-directory
 * @param: mode 是否递归删除
 * 			0-非递归
 * 			1-递归删除, 相当于 -r 参数
 */
int remove_dir(const char *path, int mode)
{
	int ret = 0;
	FILINFO fno;

	ret = f_stat(path, &fno); //Check existance of a file or sub-directory
	if(ret == FR_NO_FILE)
		return ret;

	if(ret == FR_OK)
	{
		if(mode == 0) {
			ret = f_unlink(path);      //此函数只能清除一个文件或者一个空目录
		}
		else {
			char path_buff[256];
			int  len = u_strlen(path);
			if(len > sizeof(path_buff)/2)    //path_buff必须有一定的余量, 因为递归遍历时还要往path_buff后面添加字符串
				return -1;
			u_memcpy(path_buff, path, len+1);
			ret = clear_dir(path_buff);      //采用递归算法,会消耗大量的栈
		}
	}
	return ret;
}

//检测一个路径是否存在 0-存在  其他-不存在
//static int is_dir_exist(const char *dir)
int is_dir_exist(const char *dir)
{
	FILINFO fno;
	int ret;

	ret = f_stat(dir, &fno);
	if(ret==0 && (fno.fattrib&AM_DIR))
		return 0;
	return ret;
}

//检测一个文件是否存在 0-存在  其他-不存在
//static int is_fil_exist(const char *fil)
int is_fil_exist(const char *fil)
{
	FILINFO fno;
	int ret;

	ret = f_stat(fil, &fno);
	if(ret==0 && !(fno.fattrib&AM_DIR))
		return 0;
	return ret;
}

/*
 * @brief: 显示磁盘空闲情况
 */
int disk_free(int unit)
{
	int i, cnt, res = 0;
	char path[16];
    FATFS *fs;
    DWORD fre_clust, fre_sect, tot_sect;

	if(_VOLUMES>=2 && sys_fs[1].fs_type!=0)  //判断SD磁盘是否已经挂载
		cnt = _VOLUMES;
	else
		cnt = 1;
	
	my_printf("%-5s %10s %10s %10s %10s\n\r", "disk", "total", "used", "free", "use%");
	for(i=0; i<cnt; i++)
	{
		u_snprintf(path, sizeof(path), "%d:", i);

		/* Get volume information and free clusters of drive 1 */
		res = f_getfree(path, &fre_clust, &fs);
		if (res) return(res);

		/* Get total sectors and free sectors */
		tot_sect = (fs->n_fatent - 2) * fs->csize;
		fre_sect = fre_clust * fs->csize;
		
//		my_printf("disk %d cluster size: %d\n\r", i, fs->csize);  //一个cluster有几个sector
			
		/* Print the free space (assuming 512 bytes/sector) */
		if(unit == 'K')
			my_printf("%-5d %9uK %9uK %9uK %9d%%\n\r", i, tot_sect>>1, (tot_sect-fre_sect)>>1, fre_sect>>1, 100*(tot_sect-fre_sect)/tot_sect);
		else
			my_printf("%-5d %9uM %9uM %9uM %9d%%\n\r", i, tot_sect>>11, (tot_sect-fre_sect)>>11, fre_sect>>11, 100*(tot_sect-fre_sect)/tot_sect);	
	}
	
	return res;
}

/** @brief 随机定位文件的读写指针
  * @param fil    -文件指针
  *        percent-读写指针的百分比位置 0-定位到音频文件开头的地方  2024-定位到音频20.24%处, 10000-定位到音频文件的末尾
  * @reval 0=成功 负数=失败
  */
int relocate_file_wr_ptr(FIL *fil, int percent)
{
	if(percent>10000 || percent<0)
		return -1;

	uint32_t new_ptr = (uint64_t)f_size(fil) * percent / 10000;
	new_ptr &= 0xFFFFFFF0;  //一律16字节对齐

	f_lseek(fil, new_ptr);

	return 0;	
}

/** @brief 获取文件读写指针的百分比位置
  * @param fil -文件指针
  * @reval 负数-失败
  *        非负数-读写指针的位置,以百分数的100倍表示, 比如5000(50%)表文件的一半
  */
int get_file_wr_ptr(FIL *fil)
{
	//尽量不用float浮点数
	int ret = (uint64_t)10000 * fil->fptr / f_size(fil);

	return ret;	
}




