# simplePlayer
a simple ffplay-style video player
ffmpeg解封装解码，SDL渲染音视频

一、解决音频无法正常播放的问题：
解决方法1、audio_dev = SDL_OpenAudio(&wanted_spec, NULL) < 0，其中obtained SDL_AudioSpec 需要为NULL
          详见audio.cpp/open_audio_playing.cpp
解决方法2、参考ffplay中打开音频设备的方法
          详见audio.cpp/open_audio_playing_ffplay.cpp

二、是否按照原视频分辨率播放，设置于player.h, OriginalSize
若不按照原分辨率播放，对播放窗口进行按原视频比例进行缩放

三、准确seeking 见video.cpp与audio.cpp  FineSeek
