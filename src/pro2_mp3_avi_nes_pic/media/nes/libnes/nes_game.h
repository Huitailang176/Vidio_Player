#ifndef  __NES_GAME_H_
#define  __NES_GAME_H_

#include <stdint.h>

//libnes_game.lib提供的函数接口

extern int  nes_game_load(char *fname, uint32_t load_flash_addr, uint32_t load_max_size);
extern int  nes_game_run(void);
extern void nes_game_1s_hook(void);
extern void nes_game_stop(void);

extern int  nes_game_get_volume(void);
extern int  nes_game_set_volume(int volume);


#endif


