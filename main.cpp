#include "player.h"

/* Prevents SDL from overriding main() */
#undef main

int main(int argc, char *argv[])
{
    //if (argc != 2)
    //{
    //    printf("Please provide a movie file, usage: \n");
    //    printf("./ffplayer ring.mp4\n");
    //    return -1;
    //}
    // D:\Project\FFmpeg\file    
    // audio_video_sync_test.mp4  Iron_Man-Trailer_HD.mp4   tesst.mp4
    const char* file = "D:/Project/C++/player/files/IronMan.mp4";
    printf("Try playing %s ...\n", file);
    player_running(file);

    return 0;
}
