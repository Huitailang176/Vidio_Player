#ifndef  __NES_EMULATOR_H_
#define  __NES_EMULATOR_H_


int  nes_emulator_load_game(char *fname);
void nes_emulator_cycle(void);
void nes_emulator_stop(void);

int nes_emulator_exec(char *fname);


#endif


