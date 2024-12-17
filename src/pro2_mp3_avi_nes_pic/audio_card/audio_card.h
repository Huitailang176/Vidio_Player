#ifndef  __AUDIO_CARD_H_
#define  __AUDIO_CARD_H_


//供用户使用的API
int audio_card_init(int volume);
int audio_card_open(int sample_freq, int channels, int sample_bits, void *pkg_buff, int pkg_size);
int audio_card_get_freq(void);
int audio_card_chg_freq(int sample_freq);
int audio_card_chg_volume(int volume);
int audio_card_get_volume(void);
int audio_card_register_feeder(void (*feeding_func)(void));
int audio_card_write(void *pkg_buff, int pkg_size);
int audio_card_close(void);


#endif


