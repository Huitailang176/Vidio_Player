#ifndef  __MEDIA_TIMER_V2_H_
#define  __MEDIA_TIMER_V2_H_

//Media common timer
void media_timer_start(int beat_freq, int beat_count, void (*beat_callback)(void));
void media_timer_stop(void); 

#endif

