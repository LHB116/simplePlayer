#ifndef __VIDEO_H__
#define __VIDEO_H__

#include "player.h"

int open_video(player_stat_t *is);
void video_refresh(void* opaque, double* remaining_time);


#endif

