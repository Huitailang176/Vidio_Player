#ifndef  __USB_APP_H_
#define  __USB_APP_H_

#include "usb_core.h"
#include "usbd_core.h"
#include "usb_conf.h"

#include <stdint.h>


#define  BULK_MAX_PACKET_SIZE  0x40	 //USB�������ݰ��Ĵ�С 64�ֽ�
#define  DISK_BLOCK_SIZE       512   //���̿��С 512�ֽ� 

#define  USB_MAS_DISK_READ     0x01
#define  USB_MAS_DISK_WRITE    0x02
#define  USB_MAS_COPY_DATA     0x04

#define  IN_BUFF_SIZE          (DISK_BLOCK_SIZE*4)     //IN_BUFF_SIZE <= OUT_BUFF_SIZE
#define  OUT_BUFF_SIZE         (DISK_BLOCK_SIZE*16)    //��MSC_MEDIA_PACKET����һ��

#define  USB_IN_BUFF_EMPTY    0
#define  USB_IN_BUFF_LOADED   1
#define  USB_OUT_BUFF_EMPTY   0
#define  USB_OUT_BUFF_FULL    1

#define  USB_IN_TRANS_IDLE     0
#define  USB_IN_TRANS_ONGOING  1
#define  USB_OUT_TRANS_IDLE    0
#define  USB_OUT_TRANS_ONGOING 1

#define  OPT_PREPARE  0
#define  OPT_ONGOING  1


typedef struct usb_disk_opt_s
{
	uint32_t opt_mark;   //��д��־
	uint32_t lun;        //����id
	
	uint8_t  usb_out_buff[OUT_BUFF_SIZE];   //OUT��������һ�����̿�Ϳ�ʼд����,
	uint64_t write_addr;                     //��Ҫ�ȵ�buff����д����̺��Valid���ն˵�, ����OUT�������һ��buff�Ϳ� 
	uint32_t write_len;
	uint32_t write_trans_cnt;                //Write10 Cmdд���������������� write_trans_cnt==write_trans_size ˵�������д����
	uint32_t write_trans_size;               //Write10 CmdҪд���̵�������

	uint8_t  usb_in_buff[IN_BUFF_SIZE];      //IN�������˫����, �һ��������̿�. ����Ϊ����߶����̵��ٶ�
 	uint64_t read_addr;
	uint32_t read_len;
	uint32_t read_trans_cnt;               //Read10 Cmd������������������ read_trans_cnt==read_trans_size ˵������˶�����
	uint32_t read_trans_size;              //Read10 CmdҪ�����̵�������

    uint8_t  usb_in_buff_load_state;  //USB deviceҪ�ϴ��������Ƿ��Ѿ����ص�����
	uint8_t  usb_out_buff_full_state; //USB device���յ��������Ƿ��Ѿ�д������

	uint8_t  usb_in_trans_state;  //���ݴ�RAM->USB����״̬(USB IN������״̬) 
	uint8_t  usb_out_trans_state; //���ݴ�USB->RAM����״̬(USB OUT������״̬)
	
	uint32_t usb_in_read_disk_speed;   //IN����������ٶ�
	uint32_t usb_out_write_disk_speed; //OUT����д�����ٶ�
}usb_disk_opt_t;


extern uint8_t Max_Lun;
extern USB_OTG_CORE_HANDLE USB_OTG_dev;

void USB_device_init(void);
void usb_read_write_disk(void);
void usb_mas_read_memory(int opt_flag, uint8_t lun, uint32_t Memory_Offset, uint32_t Transfer_Length);
void usb_mas_write_memory(int opt_flag, uint8_t lun, uint32_t Memory_Offset, uint32_t Transfer_Length);

void usb_1s_hook(void);


#endif


