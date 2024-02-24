#ifndef __PLAYER_H__
#define __PLAYER_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavdevice/avdevice.h>

#include <libavformat/avformat.h>

#include <libavcodec/avcodec.h>
#include "libavcodec/avfft.h"

#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

#include <libavutil/audio_fifo.h>
#include <libavutil/fifo.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include "libavutil/opt.h"
#include "libavutil/dict.h"
#include "libavutil/parseutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/avassert.h"
#include <libavutil/audio_fifo.h>
#include "libavutil/time.h"
#include "libavutil/avstring.h"
#include "libavutil/eval.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include <libavutil/frame.h>

#include <SDL.h>
#include <SDL_thread.h>


#ifdef __cplusplus
}
#endif

// 播放器按照接近该宽高比例进行播放，实际播放宽高按照原视频分辨率进行等比例缩放
#define WINDOW_W 1280
#define WINDOW_H 720

// 是否使用视频原始分辨率播放
#define OriginalSize 0

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

/* polls for possible required screen refresh at least this often, should be less than 1/fps */
#define REFRESH_RATE 0.01

#define SDL_AUDIO_BUFFER_SIZE 1024
#define MAX_AUDIO_FRAME_SIZE 192000

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_FRAMES 25

/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30

#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))


#define FF_QUIT_EVENT    (SDL_USEREVENT + 2)

typedef struct {
    double pts;                     // 当前帧(待播放)显示时间戳，播放后，当前帧变成上一帧
    double pts_drift;               // 当前帧显示时间戳与当前系统时钟时间的差值
    double last_updated;            // 当前时钟(如视频时钟)最后一次更新时间，也可称当前时钟时间
    double speed;                   // 时钟速度控制，用于控制播放速度
    int serial;                     // 播放序列，所谓播放序列就是一段连续的播放动作，一个seek操作会启动一段新的播放序列
    int paused;                     // 暂停标志
    int *queue_serial;              // 指向packet_serial
}   play_clock_t;

typedef struct {
    int freq;
    int channels;
    int64_t channel_layout;
    enum AVSampleFormat fmt;
    int frame_size;
    int bytes_per_sec;
}   audio_param_t;

typedef struct {
    SDL_Window *window; 
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_Rect rect;
}   sdl_video_t;

typedef struct packet_queue_t {
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;                 // 队列中packet的数量
    int size;                       // 队列所占内存空间大小
    int64_t duration;               // 队列中所有packet总的播放时长
    int abort_request;
    int serial;                     // 播放序列，所谓播放序列就是一段连续的播放动作，一个seek操作会启动一段新的播放序列
    SDL_mutex *mutex;
    SDL_cond *cond;
}   packet_queue_t;

/* Common struct for handling all types of decoded data and allocated render buffers. */
typedef struct {
    AVFrame *frame;
    int serial;
    double pts;           /* presentation timestamp for the frame */
    double duration;      /* estimated duration of the frame */
    int64_t pos;                    // frame对应的packet在输入文件中的地址偏移
    int width;
    int height;
    int format;
    AVRational sar;
    int uploaded;
    int flip_v;
}   frame_t;

typedef struct {
    frame_t queue[FRAME_QUEUE_SIZE];
    int rindex;                     // 读索引。待播放时读取此帧进行播放，播放后此帧成为上一帧
    int windex;                     // 写索引
    int size;                       // 总帧数
    int max_size;                   // 队列可存储最大帧数
    int keep_last;
    int rindex_shown;               // 当前是否有帧在显示
    SDL_mutex *mutex;
    SDL_cond *cond;
    packet_queue_t *pktq;           // 指向对应的packet_queue
}   frame_queue_t;

typedef struct {
    char *filename;
    AVFormatContext *p_fmt_ctx;
    AVStream *p_audio_stream;
    AVStream *p_video_stream;
    AVCodecContext *p_acodec_ctx;
    AVCodecContext *p_vcodec_ctx;

    int audio_idx;
    int video_idx;
    sdl_video_t sdl_video;

    play_clock_t audio_clk;                   // 音频时钟
    play_clock_t video_clk;                   // 视频时钟
    double frame_timer;

    packet_queue_t audio_pkt_queue;
    packet_queue_t video_pkt_queue;
    frame_queue_t audio_frm_queue;
    frame_queue_t video_frm_queue;

    struct SwsContext *img_convert_ctx;
    struct SwrContext *audio_swr_ctx;
    AVFrame *p_frm_yuv;

    audio_param_t audio_param_src;
    audio_param_t audio_param_tgt;
    int audio_hw_buf_size;              // SDL音频缓冲区大小(单位字节)
    uint8_t *p_audio_frm;               // 指向待播放的一帧音频数据，指向的数据区将被拷入SDL音频缓冲区。若经过重采样则指向audio_frm_rwr，否则指向frame中的音频
    uint8_t *audio_frm_rwr;             // 音频重采样的输出缓冲区
    unsigned int audio_frm_size;        // 待播放的一帧音频数据(audio_buf指向)的大小
    unsigned int audio_frm_rwr_size;    // 申请到的音频缓冲区audio_frm_rwr的实际尺寸
    int audio_cp_index;                 // 当前音频帧中已拷入SDL音频缓冲区的位置索引(指向第一个待拷贝字节)
    int audio_write_buf_size;           // 当前音频帧中尚未拷入SDL音频缓冲区的数据量，audio_frm_size = audio_cp_index + audio_write_buf_size
    double audio_clock;
    int audio_clock_serial;
    
    int abort_request;
    int paused;
    int step;

    SDL_cond *continue_read_thread;
    SDL_Thread *read_tid;           // demux解复用线程

    // seeking 相关
    int seek_req;                     // 请求seeking标志
    int seek_flags;                   // seeking标志
    int64_t seek_pos;                 // seeking的字节偏移
    int64_t seek_rel;                 // seeking的相对位置

    // FineSeek
    double seek_time;        // 精确的跳转点时间
    int seek_flag_audio;      // 音频seek标志位
    int seek_flag_video;      //  视频seek标志位
    int seek_flag_subtitle;   //  字幕seek标志位
    // FineSeek

}   player_stat_t;

int player_running(const char *p_input_file);
double get_clock(play_clock_t *c);
void set_clock_at(play_clock_t *c, double pts, int serial, double time);
void set_clock(play_clock_t *c, double pts, int serial);

#endif
