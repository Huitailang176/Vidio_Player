#ifndef  __VIDEO_APP_H_
#define  __VIDEO_APP_H_

#include <stdint.h>

int video_open_file(char *fname);
int video_decode_stream(void);
int video_close(void);

void video_1s_hook(void);

int video_seek(int pos);
int video_get_pos(void);
int video_set_volume(int volume);
int video_get_volume(void);
int video_pause(void);
int video_resume(void);


#endif


