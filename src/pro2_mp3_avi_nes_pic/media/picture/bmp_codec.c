#include "bmp_codec.h"
#include "ff.h"
#include "my_malloc.h"
#include "my_printf.h"
#include "libc.h"
#include "lcd_app_v2.h"


/*
 * 函数参数: src_color-背景颜色  dst_color-目标颜色  alpha-目标颜色所占比重[0, 255]
 * 返回值:   按照alpha混合后的颜色(LCD 待显示的颜色)
 */
static inline int alpha_blending(int src_color, int dst_color, int alpha)
{	 
	/*
	 * 计算公式 ret= dst*alp + src*(1-alp), 其中alp∈[0.0, 1.0]
	 * 由于计算机浮点运算速度远小于整数运算速度,所以进行如下处理
	 * ret = dst_color*alpha/255 + src_color * (0xFF - alpha)/255
	 *     = (dst_color - src_color)*alpha>>8 + src_color
	 */
	return (((dst_color-src_color)*alpha)/255 + src_color);
//	return (dst_color*alpha/255 + src_color * (0xFF - alpha)/255);
}

/*
 * @brief: bmp图片解码(支持RGB565 RGB888 ARGB8888)
 * @param: bmp_name-bmp图片的文件名, 比如pic1.bmp  /picture/pic1.bmp等
 * @reval: 0-图片解码成功,  非0-图片解码失败
 */
int bmp_decode(const char *bmp_name)
{
	int  ret = 0;
	FIL fil;
	bitmapFileHeader_t bitmapFileHeader;   //位图文件头(bitmap file header)
	bitmapInfoHeader_t bitmapInfoHeader;   //位图信息头(bitmap information header)
	uint8_t *file_buff;
	uint16_t *color_buff;  //for LCD-RGB565
	uint32_t rd_len;
	uint32_t rgb_mask_table[3];            //RGB565时有用
	
	ret= f_open(&fil, bmp_name, FA_READ);
	if( ret ) {
		return ret;
	}

	f_rewind(&fil);
	ret = f_read(&fil, &bitmapFileHeader, sizeof(bitmapFileHeader), &rd_len);
	if(ret!=FR_OK || rd_len!=sizeof(bitmapFileHeader)) {
		f_close(&fil);
		return ret;
	}
	ret = f_read(&fil, &bitmapInfoHeader, sizeof(bitmapInfoHeader), &rd_len);
	if(ret!=FR_OK || rd_len!=sizeof(bitmapInfoHeader)) {
		f_close(&fil);
		return ret;
	}
	
	//debug
//	my_printf("bitmapFileHeader.bfSize = %d\n\r", bitmapFileHeader.bfSize);
//	my_printf("bitmapFileHeader.bfOffBits = %d\n\r", bitmapFileHeader.bfOffBits);
//	my_printf("bitmapInfoHeader.biSize = %d\n\r", bitmapInfoHeader.biSize);
//	my_printf("bitmapInfoHeader.biWidth = %d\n\r", bitmapInfoHeader.biWidth);
//	my_printf("bitmapInfoHeader.biHeight = %d\n\r", bitmapInfoHeader.biHeight);
//	my_printf("bitmapInfoHeader.biPlanes = %d\n\r", bitmapInfoHeader.biPlanes);
//	my_printf("bitmapInfoHeader.biBitCount = %d\n\r", bitmapInfoHeader.biBitCount);
//	my_printf("bitmapInfoHeader.biCompression = %d\n\r", bitmapInfoHeader.biCompression);
//	my_printf("bitmapInfoHeader.biSizeImage = %d\n\r", bitmapInfoHeader.biSizeImage);
//	my_printf("bitmapInfoHeader.biXPelsPerMeter = %d\n\r", bitmapInfoHeader.biXPelsPerMeter);
//	my_printf("bitmapInfoHeader.biYPelsPerMeter = %d\n\r", bitmapInfoHeader.biYPelsPerMeter);
//	my_printf("bitmapInfoHeader.biClrUsed = %d\n\r", bitmapInfoHeader.biClrUsed);
//	my_printf("bitmapInfoHeader.biClrImportant = %d\n\r", bitmapInfoHeader.biClrImportant);
	
	if(bitmapFileHeader.bfType != (((uint16_t)'M'<<8)|'B'))
	{
		f_close(&fil);                      //非BMP格式
		return -1;		
	}
	
	if(bitmapInfoHeader.biBitCount == 16)
	{
		if(bitmapInfoHeader.biCompression != BI_BITFIELDS) {
			f_close(&fil);                 //非RGB565格式
			return -2;
		} else if(bitmapFileHeader.bfOffBits-sizeof(bitmapInfoHeader_t)-sizeof(bitmapFileHeader_t) != sizeof(rgb_mask_table)) {
			f_close(&fil);              //没有RGB掩码表      
			return -3;
		}
		else {                          //读取RGB掩码表
			ret = f_read(&fil, rgb_mask_table, sizeof(rgb_mask_table), &rd_len);
			if(ret!=FR_OK || rd_len!=sizeof(rgb_mask_table)) {
				f_close(&fil);
				return -4;
			}
			if(rgb_mask_table[0]!=0x00F800 || rgb_mask_table[1]!=0x0007E0 || rgb_mask_table[2]!=0x00001F) {
				f_close(&fil);
				return -5;
			}
		}	
	}
	if(bitmapInfoHeader.biBitCount==24 || bitmapInfoHeader.biBitCount==32)
	{
		if(bitmapInfoHeader.biCompression != BI_RGB) {
			f_close(&fil);                  //RGB888与ARGB8888不支持RGB数据有压缩
			return -6;
		}
	}		

	/* 读写指针定位到RGB数据区 */
	ret = f_lseek(&fil, bitmapFileHeader.bfOffBits); 
	if( ret ) {
		f_close(&fil);
		return ret;
	}
	
	int height, width, num;
	uint8_t  *pdat;
	uint32_t row_pixel_bytes;   //每一行row(line)像素所占存储空间4字节对齐(是4的整数倍)
	uint32_t disp_lcd_sx, disp_lcd_ex, disp_lcd_sy, disp_lcd_ey;

	//LCD居中显示, 空白部分清为黑色
	if(bitmapInfoHeader.biHeight < lcd_dev.yres)
	{
		disp_lcd_sy = 0;
		disp_lcd_ey = (lcd_dev.yres - bitmapInfoHeader.biHeight) / 2;
		disp_lcd_sx = 0;
		disp_lcd_ex = lcd_dev.xres - 1;
		lcd_fill_area(disp_lcd_sx, disp_lcd_sy, disp_lcd_ex, disp_lcd_ey, LCD_BLACK);

		disp_lcd_sy = disp_lcd_ey + bitmapInfoHeader.biHeight;
		disp_lcd_ey = lcd_dev.yres - 1;
		lcd_fill_area(disp_lcd_sx, disp_lcd_sy, disp_lcd_ex, disp_lcd_ey, LCD_BLACK);

		disp_lcd_sy = lcd_dev.yres - (lcd_dev.yres-bitmapInfoHeader.biHeight)/2 - 1;
	}
	else
	{
		disp_lcd_sy = lcd_dev.yres - 1;
	}

	num = disp_lcd_sy;			//下面的if中使用了disp_lcd_sy变量, 必须先保存一下
	if(bitmapInfoHeader.biWidth < lcd_dev.xres)
	{
		disp_lcd_sx = 0;
		disp_lcd_ex = (lcd_dev.xres - bitmapInfoHeader.biWidth) / 2;
		disp_lcd_sy = 0;
		disp_lcd_ey = lcd_dev.yres - 1;
		lcd_fill_area(disp_lcd_sx, disp_lcd_sy, disp_lcd_ex, disp_lcd_ey, LCD_BLACK);

		disp_lcd_sx = disp_lcd_ex + bitmapInfoHeader.biWidth;
		disp_lcd_ex = lcd_dev.xres - 1;
		lcd_fill_area(disp_lcd_sx, disp_lcd_sy, disp_lcd_ex, disp_lcd_ey, LCD_BLACK);

		disp_lcd_sx = (lcd_dev.xres - bitmapInfoHeader.biWidth) / 2;
		disp_lcd_ex = disp_lcd_sx + bitmapInfoHeader.biWidth - 1;
	}
	else
	{
		disp_lcd_sx = 0;
		disp_lcd_ex = lcd_dev.xres - 1;
	}
	disp_lcd_sy = num;

	/* 开辟2个缓存并读取RGB数据, 然后显示 */
	if(bitmapInfoHeader.biBitCount == 16)
	{	
		row_pixel_bytes = bitmapInfoHeader.biWidth * 2;
		if(row_pixel_bytes & (0x4-1))
			row_pixel_bytes += (-row_pixel_bytes)%0x04;  //向4字节对齐
		
		file_buff  = mem_malloc(row_pixel_bytes);
		color_buff = mem_malloc(bitmapInfoHeader.biWidth * 2);  //for LCD-RGB565
		if(file_buff==NULL || color_buff==NULL) {
			mem_free(file_buff);
			mem_free(color_buff);
			f_close(&fil);
			return -7;
		}

		uint32_t register color16;
		//RGB565与RGB888的转化: http://blog.sina.com.cn/s/blog_64fb9b920102wxss.html
		for(height=0; height<bitmapInfoHeader.biHeight; height++)
		{
			ret = f_read(&fil, file_buff, row_pixel_bytes, &rd_len);
			if(ret!=FR_OK || row_pixel_bytes!=rd_len) {
				ret = -8;
				break;
			}

			pdat = file_buff;
			num = bitmapInfoHeader.biWidth;

			if(bitmapInfoHeader.biWidth > lcd_dev.xres) {
				//图像宽度超过LCD水平分辨率时, 提取图像中间像素
				pdat += 2 * ((bitmapInfoHeader.biWidth-lcd_dev.xres)/2);
				num = lcd_dev.xres;
			}

			for(width=0; width<num; width++) {
				color16 = ((uint32_t)pdat[1]<<8) | pdat[0];
				color_buff[width] = color16;  //RGB565
				pdat += 2;
			}

			lcd_colorfill_area_16b(disp_lcd_sx, disp_lcd_sy, disp_lcd_ex, disp_lcd_sy, color_buff);
			if(disp_lcd_sy == 0)
				break;
			disp_lcd_sy--;
		}
	}
	else if(bitmapInfoHeader.biBitCount == 24)
	{
		row_pixel_bytes = bitmapInfoHeader.biWidth * 3;
		if(row_pixel_bytes & (0x4-1))
			row_pixel_bytes += (-row_pixel_bytes)%0x04;  //向4字节对齐

		file_buff  = mem_malloc(row_pixel_bytes);
		color_buff = mem_malloc(bitmapInfoHeader.biWidth * 2);  //for LCD-RGB565
		if(file_buff==NULL || color_buff==NULL) {
			mem_free(file_buff);
			mem_free(color_buff);
			f_close(&fil);
			return -7;
		}

		for(height=0; height<bitmapInfoHeader.biHeight; height++)
		{
			ret = f_read(&fil, file_buff, row_pixel_bytes, &rd_len);
			if(ret!=FR_OK || row_pixel_bytes!=rd_len) {
				ret = -8;
				break;
			}

			pdat = file_buff;
			num = bitmapInfoHeader.biWidth;

			if(bitmapInfoHeader.biWidth > lcd_dev.xres) {
				//图像宽度超过LCD水平分辨率时, 提取图像中间像素
				pdat += 3 * ((bitmapInfoHeader.biWidth-lcd_dev.xres)/2);
				num = lcd_dev.xres;
			}

			for(width=0; width<num; width++) {
				color_buff[width] = ((uint16_t)(pdat[2]&0xF8)<<8) | ((uint16_t)(pdat[1]&0xFC)<<3) | ((uint16_t)(pdat[0]&0xF8)>>3);  //RGB565
				pdat += 3;
			}

			lcd_colorfill_area_16b(disp_lcd_sx, disp_lcd_sy, disp_lcd_ex, disp_lcd_sy, color_buff);		
			if(disp_lcd_sy == 0)
				break;
			disp_lcd_sy--;
		}
	}
	else if(bitmapInfoHeader.biBitCount == 32)
	{
		row_pixel_bytes = bitmapInfoHeader.biWidth * 4;
		if(row_pixel_bytes & (0x4-1))
			row_pixel_bytes += (-row_pixel_bytes)%0x04;  //向4字节对齐

		file_buff  = mem_malloc(row_pixel_bytes);
		color_buff = mem_malloc(bitmapInfoHeader.biWidth * 2);  //for LCD-RGB565
		if(file_buff==NULL || color_buff==NULL) {
			mem_free(file_buff);
			mem_free(color_buff);
			f_close(&fil);
			return -7;
		}
		
		int alpha;
		uint8_t lcd[4];
		for(height=0; height<bitmapInfoHeader.biHeight; height++)
		{
			ret = f_read(&fil, file_buff, row_pixel_bytes, &rd_len);
			if(ret!=FR_OK || row_pixel_bytes!=rd_len) {
				ret = -8;
				break;
			}

			pdat = file_buff;
			num = bitmapInfoHeader.biWidth;

			if(bitmapInfoHeader.biWidth > lcd_dev.xres) {
				//图像宽度超过LCD水平分辨率时, 提取图像中间像素
				pdat += 4 * ((bitmapInfoHeader.biWidth-lcd_dev.xres)/2);
				num = lcd_dev.xres;
			}

			for(width=0; width<num; width++) {
				alpha = pdat[3];
				if(alpha == 0xFF) {
					color_buff[width] = ((uint16_t)(pdat[2]&0xF8)<<8) | ((uint16_t)(pdat[1]&0xFC)<<3) | ((uint16_t)(pdat[0]&0xF8)>>3);  //RGB565
				} else {
//					color_buff[width] = lcd_read_point_16b(disp_lcd_orgn.x+width, disp_lcd_orgn.y + bitmapInfoHeader.biHeight - height-1);
					color_buff[width] = color24to16(lcd_dev.backcolor);   //SPI LCD无法读取像素, 故注释掉上面
					if(alpha != 0x00)
					{   //计算2个像素的混合度(3个通道分别计算)
						lcd[2] = (color_buff[width] & 0xF800) >> 8;
						lcd[1] = (color_buff[width] & 0x07E0) >> 3;
						lcd[0] = (color_buff[width] & 0x001F) << 3;
						
						pdat[2] = alpha_blending(lcd[2], pdat[2], alpha);
						pdat[1] = alpha_blending(lcd[1], pdat[1], alpha);
						pdat[0] = alpha_blending(lcd[0], pdat[0], alpha);
						color_buff[width] = ((uint16_t)(pdat[2]&0xF8)<<8) | ((uint16_t)(pdat[1]&0xFC)<<3) | ((uint16_t)(pdat[0]&0xF8)>>3);  //混合后的颜色 RGB565
					}
				}
				pdat += 4;
			}

			lcd_colorfill_area_16b(disp_lcd_sx, disp_lcd_sy, disp_lcd_ex, disp_lcd_sy, color_buff);
			if(disp_lcd_sy == 0)
				break;
			disp_lcd_sy--;
		}
	}
	else
	{   //不支持的像素比特数
		f_close(&fil);
		return -9;
	}

	mem_free(file_buff);
	mem_free(color_buff);
	f_close(&fil);
	return ret;
}

/*
 * @breif: 把整屏LCD像素编码成一张RGB888的bmp图片并存储到文件系统
 * @param: bmp_name-编码后生成图片的文件名
 */
int lcd_bmp_encode(const char *bmp_name, uint32_t sx, uint32_t sy, uint32_t ex, uint32_t ey)
{
	int  ret = 0;
	FIL fil;
	bitmapFileHeader_t bitmapFileHeader;   //位图文件头(bitmap file header)
	bitmapInfoHeader_t bitmapInfoHeader;   //位图信息头(bitmap information header)
	uint8_t *file_buff;
	uint32_t *lcd_buff;
	uint32_t wt_len;
	uint32_t src_line_bytes;   //每一行row(line)像素所占存储空间4字节对齐(是4的整数倍)	
	
	src_line_bytes = (ex - sx + 1) * 3;
	if(src_line_bytes & (0x4-1))
		src_line_bytes += (-src_line_bytes)%0x04;  //向4字节对齐
	
	bitmapInfoHeader.biSize = sizeof(bitmapInfoHeader_t);
	bitmapInfoHeader.biWidth = ex - sx + 1;
	bitmapInfoHeader.biHeight = ey - sy + 1;
	bitmapInfoHeader.biPlanes = 1;      //always 1
	bitmapInfoHeader.biBitCount = 24;
	bitmapInfoHeader.biCompression = BI_RGB;
	bitmapInfoHeader.biSizeImage = src_line_bytes * bitmapInfoHeader.biHeight;
	bitmapInfoHeader.biXPelsPerMeter = 0;
	bitmapInfoHeader.biYPelsPerMeter = 0;
	bitmapInfoHeader.biClrUsed = 0;
	bitmapInfoHeader.biClrImportant = 0;
	
	bitmapFileHeader.bfType = ((uint16_t)'M' << 8) | 'B';
	bitmapFileHeader.bfSize = sizeof(bitmapFileHeader) + sizeof(bitmapInfoHeader) + bitmapInfoHeader.biSizeImage;
	bitmapFileHeader.bfReserved = 0;
	bitmapFileHeader.bfOffBits = sizeof(bitmapFileHeader) + sizeof(bitmapInfoHeader);
	
	ret= f_open(&fil, bmp_name, FA_CREATE_ALWAYS|FA_WRITE);
	if( ret ) {
		return ret;
	}
	
	f_rewind(&fil);
	ret = f_write(&fil, &bitmapFileHeader, sizeof(bitmapFileHeader), &wt_len);
	if(ret!=FR_OK && wt_len!=sizeof(bitmapFileHeader)) {
		f_close(&fil);
		return -1;
	}
	ret = f_write(&fil, &bitmapInfoHeader, sizeof(bitmapInfoHeader), &wt_len);
	if(ret!=FR_OK && wt_len!=sizeof(bitmapInfoHeader)) {
		f_close(&fil);
		return -2;
	}	

	int line, pixel;
	uint8_t  *pdat;
	
	file_buff = mem_malloc(src_line_bytes);
	lcd_buff = mem_malloc((ex-sx+1)*4);
	if(file_buff==NULL || lcd_buff==NULL) {
		mem_free(file_buff);
		mem_free(lcd_buff);
		f_close(&fil);
	}

	lcd_dma_trans_release();                //必须得等DMA释放LCD后, CPU才能操控LCD. 两者不可同时操控LCD
	for(line=ey; line>=(int)sy; line--)     //bmp格式按照从左到右, 从下到上的顺序扫描图片的像素数据进行存储
	{
		lcd_read_area(sx, line, ex, line, lcd_buff, (ex-sx+1)*4);   
		                                                 
		pdat = file_buff;
		for(pixel=0; pixel<bitmapInfoHeader.biWidth; pixel++) {
			pdat[2] = lcd_buff[pixel] >> 16;  //把像素值分解3为个通道
			pdat[1] = lcd_buff[pixel] >> 8;
			pdat[0] = lcd_buff[pixel];
			pdat += 3;
		}		
		
		ret = f_write(&fil, file_buff, src_line_bytes, &wt_len);
		if(ret!=FR_OK && wt_len!=src_line_bytes) {
			my_printf("ret=%d\n\r", ret);
			mem_free(file_buff);
			mem_free(lcd_buff);
			f_close(&fil);
			return -3;
		}
	}	
	
	mem_free(file_buff);
	mem_free(lcd_buff);
	f_close(&fil);
	return 0;
}



