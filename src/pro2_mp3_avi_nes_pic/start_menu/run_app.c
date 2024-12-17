#include "run_app.h"
#include "fs_app_v2.h"

#include "audio_player.h"
#include "video_player.h"
#include "nes_emulator.h"
#include "pic_display.h"


int run_file_with_app(char *fname, int ftype)
{
	int ret;

	switch(ftype)
	{
		case MP3_FILE:
		case WAV_FILE:
//		case AAC_FILE:        //Not support AAC
			ret = audio_player_exec(fname);
			break;

		case AVI_FILE:
			ret = video_player_exec(fname);
			break;

		case NES_FILE:
			ret = nes_emulator_exec(fname);
			break;

		case BMP_FILE:
		case JPG_FILE:
			ret = picture_display(fname);
			break;

		default:
			ret = -1;		
	}

	return ret;
}


