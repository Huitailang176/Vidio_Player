#ifndef  __BMP_CODEC_H_
#define  __BMP_CODEC_H_

#include <stdint.h>

/* 本文件实现bmp图片的编解码
 * 需要的资源: Fatfs文件系统, 用来读/写图片文件
 *             LCD显示器, 用来显示像素/获取像素
 *             内存管理, 提供malloc free函数
 */


//====================================================================================================
/*
 * bmp位图文件由四个部分组成:
 * 位图文件头(bitmap file header)、位图信息头(bitmap information header)、彩色表(color table)、
 * 位图数据 : 1. 行像素row的存储字节数 4字节对齐--无论在编码还是解码中,这个知识点都太重要
 *            2. 图片的坐标原点在图片的左下角, 数据扫描方向为: 从左往右, 从下往上
 */

//位图文件头(bitmap file header)
typedef struct bitmapFileHeader_s
{
	uint16_t bfType;       //文件标识, 只识别'BM', 用来识别BMP位图类型
	uint32_t bfSize;       //整个文件的大小
	uint32_t bfReserved;   //保留
	uint32_t bfOffBits;    //从文件开始到位图数据开始(bitmap data)之间的偏移量
}__attribute__((packed)) bitmapFileHeader_t;

//位图信息头(bitmap information header)
#define  BI_RGB        0      //没有压缩, RGB-555
#define  BI_RLE8       1      //每个像素8bit的RLE压缩编码, 压缩格式由2字节组成
#define  BI_RLE4       2      //每个像素4bit的RLE压缩编码, 压缩格式由2字节组成
#define  BI_BITFIELDS  3      //每个像素的比特由指定的掩码决定
//关于RLE(Run Length Encoding) 行程编码, 可以参考:https://www.cnblogs.com/hwl1023/p/5129696.html

typedef struct bitmapInfoHeader_s
{
	uint32_t biSize;          //bitmapInfoHeader_te结构体的大小
	uint32_t biWidth;         //图像的宽度, 以像素为单位
	uint32_t biHeight;        //图像的高度, 以像素为单位
	uint16_t biPlanes;        //为目标设备说明位面数, 其值将总是被设置为1
	uint16_t biBitCount;      //每个像素的比特数, 其值为1, 4, 8, 16, 24, 32
	uint32_t biCompression;   //取值为BI_RGB, BI_RLE8, BI_RLE4, BI_BITFIELDS
	uint32_t biSizeImage;     //图像的大小, 以字节为单位
	uint32_t biXPelsPerMeter; //说明水平分辨率, 单位: 像素/米
	uint32_t biYPelsPerMeter; //说明垂直分辨率, 单位: 像素/米
	uint32_t biClrUsed;       //彩色表的索引数,即彩色表的个数
	uint32_t biClrImportant;  //对图像显示有重要影响的颜色索引的数目, 为0表示都重要
}__attribute__((packed)) bitmapInfoHeader_t;

//彩色表(color table) 实际上是调色板,不一定使用
typedef struct RGBQuad_s
{
	uint8_t rgbBlue;
	uint8_t rgbGreen;
	uint8_t rgbRed;
	uint8_t rgbReserved;
}__attribute__((packed)) RGBQuad_t;


//bmp图片的数据结构
typedef struct bmp_s
{
	bitmapFileHeader_t bitmapFileHeader; //位图文件头(bitmap file header)
	bitmapInfoHeader_t bitmapInfoHeader; //位图信息头(bitmap information header)
	RGBQuad_t *RGBQuad;                  //彩色表(color table) 实际上是调色板, 使用情况见下面知识点
}__attribute__((packed)) bmp_t;

/*必看bmp图片格式知识点：https://blog.csdn.net/qq445803843/article/details/46476473， 摘要如下:
bitmapInfoHeader.biBitCount && bitmapInfoHeader.biClrUsed && bitmapInfoHeader.biCompression 这三者决定调色板(RGBQuad)的使用情况
(1) bitmapInfoHeader.biBitCount=1 <=> bitmapInfoHeader.biClrUsed=2 <=> 使用2个调色板, 有RGBQuad[2]
(2) bitmapInfoHeader.biBitCount=4 <=> bitmapInfoHeader.biClrUsed=16 <=> 使用16个调色板, 有RGBQuad[16]
(3) bitmapInfoHeader.biBitCount=8 <=> bitmapInfoHeader.biClrUsed=16 <=> 使用16个调色板, 有RGBQuad[16]
(4) bitmapInfoHeader.biBitCount=16, 若bitmapInfoHeader.biCompression=BI_RGB; 则不使用调色板
	                                若bitmapInfoHeader.biCompression=BI_BITFIELDS; 使用调色板
									若bitmapInfoHeader.biCompression=BI_RLE8, BI_RLE4;行程编码-暂时不研究(bmp很少使用此格式)
(5) bitmapInfoHeader.biBitCount=24, RGB-888 一律不适用调色板
(6) bitmapInfoHeader.biBitCount=32, 若bitmapInfoHeader.biCompression=BI_RGB; 则不使用调色板
                                    若bitmapInfoHeader.biCompression=BI_BITFIELDS;使用Alpha Blending算法(ICON图标)
									若bitmapInfoHeader.biCompression=BI_RLE8, BI_RLE4;行程编码-暂时不研究(bmp很少使用此格式)
*/


int bmp_decode(const char *bmp_name);
int lcd_bmp_encode(const char *bmp_name, uint32_t sx, uint32_t sy, uint32_t ex, uint32_t ey);

#endif


