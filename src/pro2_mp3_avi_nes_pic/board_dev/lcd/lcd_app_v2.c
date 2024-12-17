#include "lcd_app_v2.h"
#include "font.h"

/*
 * @brief: 在指定位置画一个指定大小的圆
 * @param: (x0, y0)为圆心 r为半径  color为圆的颜色
 */
void lcd_draw_circle(uint32_t x0, uint32_t y0, uint32_t r, uint32_t color)
{
	int a,b;
	int di;
	a=0;b=r;
	di=3-(r<<1);             //判断下个点位置的标志
	while(a<=b)
	{
		lcd_draw_point(x0-b,y0-a,color);             //3
		lcd_draw_point(x0+b,y0-a,color);             //0
		lcd_draw_point(x0-a,y0+b,color);             //1
		lcd_draw_point(x0-b,y0-a,color);             //7
		lcd_draw_point(x0-a,y0-b,color);             //2
		lcd_draw_point(x0+b,y0+a,color);             //4
		lcd_draw_point(x0+a,y0-b,color);             //5
		lcd_draw_point(x0+a,y0+b,color);             //6
		lcd_draw_point(x0-b,y0+a,color);
		a++;
		//使用Bresenham算法画圆
		if(di<0)di +=4*a+6;
		else
		{
			di+=10+4*(a-b);
			b--;
		}
		lcd_draw_point(x0+a,y0+b,color);
	}
}

/*
 * @brief: 画直线
 * @param: x1,y1:起点坐标  x2,y2:终点坐标  color:直线的颜色
 */
void lcd_draw_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color)
{
	uint32_t t;
	int xerr=0,yerr=0,delta_x,delta_y,distance;
	int incx,incy,uRow,uCol;

	delta_x=x2-x1; //计算坐标增量
	delta_y=y2-y1;
	uRow=x1;
	uCol=y1;
	if(delta_x>0)incx=1; //设置单步方向
	else if(delta_x==0)incx=0;//垂直线
	else {incx=-1;delta_x=-delta_x;}
	if(delta_y>0)incy=1;
	else if(delta_y==0)incy=0;//水平线
	else{incy=-1;delta_y=-delta_y;}
	if( delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴
	else distance=delta_y;
	for(t=0;t<=distance+1;t++ )//画线输出
	{
		lcd_draw_point(uRow,uCol,color);//画点
		xerr+=delta_x ;
		yerr+=delta_y ;
		if(xerr>distance)
		{
			xerr-=distance;
			uRow+=incx;
		}
		if(yerr>distance)
		{
			yerr-=distance;
			uCol+=incy;
		}
	}
}

/*
 * @brief: 画矩形
 * @param: (x1,y1),(x2,y2):矩形的对角坐标
 */
void lcd_draw_rectangle(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color)
{
	lcd_draw_line(x1,y1,x2,y1,color);
	lcd_draw_line(x1,y1,x1,y2,color);
	lcd_draw_line(x1,y2,x2,y2,color);
	lcd_draw_line(x2,y1,x2,y2,color);
}

//=====================================================================================
/*
 * @brief: 在指定位置显示一个字符    后续优化: 每次刷一行或者一列点,而不是一个一个的描点(已经完成优化)
 * @param: x,y: 起始坐标
 *         num: 要显示的字符 A B 1  ? : " "- > 等 从此函数可以推知字符的取模方式: 从上往下,从左到右
 *         size: 字体大小 12 or 16 or 24
 *         mode: 0-每次显示字体前都把字体区域的背景刷为屏幕背景(lcd_cur_sel->backcolor)
 *               1-刷为lcd_read_point(uint32_t x, uint32_t y)颜色值 (在图片上显示文字)
 *
 */
void lcd_show_char(uint32_t x, uint32_t y, int num, int size, int mode, uint32_t color)
{
    int temp, t1, t;
    uint16_t color_buff[24*12];           // 576 Byte
	int csize = (size/8+((size%8)?1:0))*(size/2);  //得到字体一个字符对应点阵集所占的字节数
	uint32_t y0 = y, x0=x;

	if(x >= lcd_dev.xres)	return;		                            //超区域了

	num = num-' ';	//得到偏移后的值
	if(size == 12) {
		uint16_t (*font12)[6] = (uint16_t (*)[])color_buff;
		for(t=0; t<csize; t++)
		{
			temp=asc2_1206[num][t]; 	 	//调用1206字体
			for(t1=0; t1<8; t1++)
			{
				if(temp&0x80)
					font12[y-y0][x-x0] = color24to16(color); //复杂的访存地址定位工作交给编译器
				else if(mode==0)
					font12[y-y0][x-x0] = color24to16(lcd_dev.backcolor);
				else if(mode==1)
					font12[y-y0][x-x0] = color24to16(lcd_read_point(x, y));
				temp <<= 1;
				y++;
				if((y-y0) == size)
				{
					y=y0;
					x++;
					if(x>=lcd_dev.xres && t+1<csize)	return;	//超区域了
					break;
				}
			}
		}
		lcd_colorfill_area_16b(x0, y0, x0+12/2-1, y0+12-1, color_buff);
	}
	else if(size == 16) {
		uint16_t (*font16)[8] = (uint16_t (*)[])color_buff;
		for(t=0; t<csize; t++)
		{
			temp=asc2_1608[num][t]; 	 	//调用1608字体
			for(t1=0; t1<8; t1++)
			{
				if(temp&0x80)
					font16[y-y0][x-x0] = color24to16(color);
				else if(mode==0)
					font16[y-y0][x-x0] = color24to16(lcd_dev.backcolor);
				else if(mode==1)
					font16[y-y0][x-x0] = color24to16(lcd_read_point(x, y));
				temp <<= 1;
				y++;
				if((y-y0) == size)
				{
					y=y0;
					x++;
					if(x>=lcd_dev.xres && t+1<csize)	return;	//超区域了
					break;
				}
			}
		}
		lcd_colorfill_area_16b(x0, y0, x0+16/2-1, y0+16-1, color_buff);
	}
	else {
		uint16_t (*font24)[12] = (uint16_t (*)[])color_buff;
		for(t=0; t<csize; t++)
		{
			temp=asc2_2412[num][t]; 	 	//调用2412字体
			for(t1=0; t1<8; t1++)
			{
				if(temp&0x80)
					font24[y-y0][x-x0] = color24to16(color);
				else if(mode==0)
					font24[y-y0][x-x0] = color24to16(lcd_dev.backcolor);
				else if(mode==1)
					font24[y-y0][x-x0] = color24to16(lcd_read_point(x, y));
				temp <<= 1;
				y++;
				if((y-y0) == size)
				{
					y=y0;
					x++;
					if(x>=lcd_dev.xres && t+1<csize)	return;	//超区域了
					break;
				}
			}
		}
		lcd_colorfill_area_16b(x0, y0, x0+24/2-1, y0+24-1, color_buff);
	}
}

/*
 * @brief: 显示字符串          显示区域 [x,x+width-1] [y,y+height-1]
 * @param: x,y:起点坐标
 *         width,height:区域大小
 *         size:字体大小
 *         p:字符串的存储地址
 */
//void lcd_show_string(uint32_t x, uint32_t y, uint32_t width, uint32_t height, int size, char *p, int mode, uint32_t color)
//{
//	uint32_t x0 = x;
//	width += x;
//	height += y;
//    while((*p<='~') && (*p>=' '))	//判断是不是非法字符!
//    {
//        if(x > width-size/2){x=x0;y+=size;}
//        if(y>=height)break;//退出
//        lcd_show_char(x, y, *p, size, mode, color);
//        x+=size/2;
//        p++;
//    }
//}


//校准时显示十字叉
void lcd_show_cross(uint32_t x, uint32_t y, uint32_t r, uint32_t color)
{
	lcd_draw_line(x-r, y, x+r, y, color);
	lcd_draw_line(x, y-r, x, y+r, color);
	lcd_draw_circle(x, y, r, color);
}


//====================================================================================
//显示汉字(需要文件系统的支持)
#include "ff.h"

#define  FONT12_FON  "0:/sys/font/GBK12.FON"
#define  FONT16_FON  "0:/sys/font/GBK16.FON"
#define  FONT24_FON  "0:/sys/font/GBK24.FON"

//code 字符指针开始
//从字库中查找出字模
//code 字符串的开始地址,GBK码
//mat  数据存放地址 (size/8+((size%8)?1:0))*(size) bytes大小	
//size:字体大小

/** @brief 获取字模矩阵
  * @param font-汉字内码, 由2字节组成 第一字节0X81~0XFE; 第二个字节分为两部分 一是0X40~0X7E 二是0X80~0XFE
  *       fil -文件句柄
  *       out_buff-字模输出内存
  *       len-字模矩阵的大小 单位-字节
  * @reval 0-字模查找成功 非0-字模查找失败
  */
static int get_matrix(uint8_t *font, FIL *fil, uint8_t *out_buff, int len)
{
	uint8_t qh,ql;					  
	uint32_t foffset, rd_len;
	int i, ret;	 
	qh = *font;
	ql= *(++font);
	if(qh<0x81||ql<0x40||ql==0xff||qh==0xff)//非常用汉字
	{   		    
	    for(i=0;i<len;i++)
			*out_buff++ = 0x00;      //填充满格
	    return -1;
	}          
	if(ql<0x7f)
		ql-=0x40;//注意!
	else
		ql-=0x41;
	qh-=0x81;
	
	foffset = ((uint32_t)190*qh+ql)*len;	  //得到字库中的字节偏移量
	ret = f_lseek(fil, foffset);
	if(ret != FR_OK)
	{
	    for(i=0;i<len;i++)
			*out_buff++ = 0x00;      //填充满格
	    return -2;	
	}
	ret = f_read(fil, out_buff, len, &rd_len);
	if(ret!=FR_OK || rd_len!=len)
	{
	    for(i=0;i<len;i++)
			*out_buff++ = 0x00;      //填充满格
	    return -3;		
	}
	
	return 0;
}

static int show_font12(uint32_t x, uint32_t y, uint8_t *matrix_buff, int mode, uint32_t color)
{
	int t, t1;
	uint16_t font12[12][12];      //注意栈要足够大, 否则会溢出 
	uint8_t  temp;
	uint32_t y0 = y, x0=x;
	uint16_t dot_color  = color24to16(color);
	uint16_t back_color = color24to16(lcd_dev.backcolor);
	for(t=0; t<2*12; t++)
	{
		temp=matrix_buff[t];
		for(t1=0; t1<8; t1++)
		{
			if(temp&0x80)
				font12[y-y0][x-x0] = dot_color; //复杂的访存地址定位工作交给编译器
			else if(mode==0)
				font12[y-y0][x-x0] = back_color;
			else if(mode==1)
				font12[y-y0][x-x0] = color24to16(lcd_read_point(x, y));
			temp <<= 1;
			y++;
			if((y-y0) == 12)
			{
				y=y0;
				x++;
				if(x>=lcd_dev.xres && t+1<2*12)	
					return -1;	//超区域了
				break;
			}
		}
	}
	lcd_colorfill_area_16b(x0, y0, x0+12-1, y0+12-1, font12[0]);
	return 0;
}

static int show_font16(uint32_t x, uint32_t y, uint8_t *matrix_buff, int mode, uint32_t color)
{
	int t, t1;
	uint16_t font16[16][16];      //注意栈要足够大, 否则会溢出 
	uint8_t  temp;
	uint32_t y0 = y, x0=x;
	uint16_t dot_color  = color24to16(color);
	uint16_t back_color = color24to16(lcd_dev.backcolor);
	for(t=0; t<2*16; t++)
	{
		temp=matrix_buff[t];
		for(t1=0; t1<8; t1++)
		{
			if(temp&0x80)
				font16[y-y0][x-x0] = dot_color;   //复杂的访存地址定位工作交给编译器
			else if(mode==0)
				font16[y-y0][x-x0] = back_color;
			else if(mode==1)
				font16[y-y0][x-x0] = color24to16(lcd_read_point(x, y));
			temp <<= 1;
			y++;
			if((y-y0) == 16)
			{
				y=y0;
				x++;
				if(x>=lcd_dev.xres && t+1<2*16)
					return -1;	//超区域了
				break;
			}
		}
	}
	lcd_colorfill_area_16b(x0, y0, x0+16-1, y0+16-1, font16[0]);
	return 0;
}

static int show_font24(uint32_t x, uint32_t y, uint8_t *matrix_buff, int mode, uint32_t color)
{
	int t, t1;
	uint16_t font24[24][24];      //注意栈要足够大, 否则会溢出 
	uint8_t  temp;
	uint32_t y0 = y, x0=x;
	uint16_t dot_color  = color24to16(color);
	uint16_t back_color = color24to16(lcd_dev.backcolor);
	for(t=0; t<3*24; t++)
	{
		temp=matrix_buff[t];
		for(t1=0; t1<8; t1++)
		{
			if(temp&0x80)
				font24[y-y0][x-x0] = dot_color; //复杂的访存地址定位工作交给编译器
			else if(mode==0)
				font24[y-y0][x-x0] = back_color;
			else if(mode==1)
				font24[y-y0][x-x0] = color24to16(lcd_read_point(x, y));
			temp <<= 1;
			y++;
			if((y-y0) == 24)
			{
				y=y0;
				x++;
				if(x>=lcd_dev.xres && t+1<2*24)	
					return -1;	//超区域了
				break;
			}
		}
	}
	lcd_colorfill_area_16b(x0, y0, x0+24-1, y0+24-1, font24[0]);
	return 0;
}

void lcd_show_font(uint32_t x, uint32_t y, uint8_t *font, int size, FIL *fil, int mode, uint32_t color)
{
	uint8_t matrix_buff[3*24];     //最大字模
	int ret, matrix_size;
	
	matrix_size = (size/8+((size%8)?1:0))*size;  //得到字体一个字符对应点阵集所占的字节数
	ret = get_matrix(font, fil, matrix_buff, matrix_size);
	if(ret != 0)
		return;

	if(size == 12)
		ret = show_font12(x, y, matrix_buff, mode, color);
	else if(size == 16)
		ret = show_font16(x, y, matrix_buff, mode, color);
	else if(size == 24)
		ret = show_font24(x, y, matrix_buff, mode, color);
	else
		return;
}

/*
 * @brief: 显示字符串          显示区域 [x,x+width-1] [y,y+height-1]
 * @param: x,y:起点坐标
 *         width,height:区域大小
 *         size:字体大小
 *         p:字符串的存储地址
 */
void lcd_show_string(uint32_t x, uint32_t y, uint32_t width, uint32_t height, int size, char *p, int mode, uint32_t color)
{
	FIL fil;
	int exist_hanzi;
	const char *fname;
	uint8_t *str;
	uint32_t x0 = x;
	width += x;
	height += y;
	
	str = (uint8_t *)p;
	exist_hanzi = 0;
	while(*str != 0)      //先判断待显示的字符串中是否有汉字
	{
		if(*str > 0x80) {
			exist_hanzi = 1;
			break;
		}
		str++;
	}
	
	if( exist_hanzi )
	{
		if(size == 12)
			fname = FONT12_FON;
		else if(size == 16)
			fname = FONT16_FON;
		else
			fname = FONT24_FON;
		if(f_open(&fil, fname, FA_READ) != FR_OK)
			return;
	}
	
	str = (uint8_t *)p;
	while(*str != 0)
	{
		if(*str < 0x80)
		{	//字符
//			if((*p<='~') && (*p>=' '))	//判断是不是非法字符!
//			{
			if(x > width-size/2){x=x0;y+=size;}
			if(y>=height)break;   //退出
			lcd_show_char(x, y, *str, size, mode, color);
			x+=size/2;
			str++;
//			}
		}
		else
		{	//汉字
			if(x > width-size){x=x0;y+=size;}
			if(y>=height)break;   //退出
			lcd_show_font(x, y, str, size, &fil, mode, color);
			x += size;
			str += 2;
		}
	}
	
	if( exist_hanzi )
	{
		f_close(&fil);
	}
}


/** @breif 提取字符串的字模信息
 ** @param str-字符串
 **        matrix_window-字模输出缓冲区
 **        width        -缓存区的宽度
 **        height       -缓存区的高度
 */
void fetch_str_matrix(char *str, void *matrix_window, int width, int height)
{
	int i, t, matrix, asc_idx;
	int x_oft, y_oft, xt;
	int csize = (height/8+((height%8)?1:0))*(height/2);    //得到字体一个字符对应点阵集所占的字节数
	uint8_t *arr2 = matrix_window;

	matrix = width*height;
	for(i=0; i<matrix; i++)
		arr2[i] = 0;

	/* 根据info加工fb */
	x_oft = 0;
	while((*str<='~') && (*str>=' '))
	{
		/* 跳过' ' */
		if(*str == ' ')
		{
			str++;
			x_oft += height/2;
			if(x_oft >= width)
				break;
			continue;
		}
		
		/* 在fb中显示一个字符 */
		asc_idx = *str - ' ';
		y_oft = 0;
		xt = x_oft;
		for(i=0; i<csize; i++)
		{
			if(height == 12)
				matrix = asc2_1206[asc_idx][i];
			else if(height == 16)
				matrix = asc2_1608[asc_idx][i];
			else
				matrix = asc2_2412[asc_idx][i];

			for(t=0; t<8; t++)
			{
				if(matrix&0x80)
					arr2[y_oft*width+xt] = 1;

				matrix <<= 1;
				y_oft++;
				if(y_oft == height)
				{
					y_oft = 0;
					xt++;
					break;
				}
			}
		}

		/* 获取下一个字符 */
		str++;
		x_oft += height/2;
		if(x_oft >= width)
			break;
	}
}

//=======================================================================================
/** @breif 显示进度条
 ** @param bar-进度条属性指针
 **        value-进度值[0, 10000], 100-1.00%  2024-20.24%  10000-100.00%
 **        draw_undue-是否绘制未进行部分
 */
void lcd_draw_progress_bar(const progress_bar_t *bar, int value, int draw_undue)
{
	int offset = value * (bar->ex - bar->sx + 1) / 10000;

	int pass_ex = bar->sx + offset - 1;
	int undu_sx = pass_ex + 1;

	if(value == 0)
	{	//此时pass_ex小于bar->sx, 不用画pass
		lcd_fill_area(bar->sx, bar->sy, bar->ex, bar->ey, bar->undue_color);
	}
	else if(value >= 10000)
	{	//此时undu_sx大于bar->ex, 不用画undue
		lcd_fill_area(bar->sx, bar->sy, bar->ex, bar->ey, bar->pass_color);
	}
	else
	{
		lcd_fill_area(bar->sx, bar->sy, pass_ex, bar->ey, bar->pass_color);
		if( draw_undue )
			lcd_fill_area(undu_sx, bar->sy, bar->ex, bar->ey, bar->undue_color);
	}
}










