#ifndef  __ICO_DECODE_H_
#define  __ICO_DECODE_H_

#include <stdint.h>


//6 Bytes
typedef struct icon_header_s
{
	uint16_t reserved; //Reserved (must be 0)
	uint16_t type;     //Resource Type (1 for icons)
	uint16_t count;    //How many images?
}__attribute__((packed)) icon_header_t;

//16 Bytes
typedef struct icon_pic_info_s
{
	uint8_t width;  //Width, in pixels, of the image
	uint8_t height; //Height, in pixels, of the image
	uint8_t color_count; // Number of colors in image (0 if >=8bpp)
	uint8_t reserved1;
	uint32_t reserved2;
	uint32_t bytes_in_res;  //How many bytes in this resource?
	uint32_t image_offset;  //Where in the file is this image?
}__attribute__((packed)) icon_pic_info_t;

//40 Bytes
typedef struct bmp_header_s
{
	uint32_t this_struct_size;         //结构体长度
	uint32_t width;                    //图像宽度
	uint32_t height;                   //图像高度(XOR图高度+AND图高度)
	uint16_t not_clear;                //位面板数? 不清除这2字节有何作用
	uint16_t bits_of_per_pix;          //每个像素所占的位数
	uint32_t compress;                 //像素数据的压缩类型
	uint32_t pix_data_len;             //像素数据的长度
	uint8_t  reserved[16];             //16个0
}__attribute__((packed)) bmp_header_t;


//4 buyes
typedef struct color_palette_s
{
	uint8_t b;
	uint8_t g;
	uint8_t r;
	uint8_t reserved;
}__attribute__((packed)) color_palette_t;



int icon_decode(const char *fname, int lcd_x, int lcd_y);


#endif



