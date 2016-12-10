#include "de.h"

#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>
#include <libswresample/swresample.h>

#include <stdio.h>
#include <stdlib.h>

#define INBUF_SIZE 4096

static DeContext *
de_context_new (AVFormatContext *fmt_ctx,
                int stream_id,
                struct SwsContext *sws_ctx)
{
    DeContext *context;

    context = malloc (1 * sizeof(DeContext));
    context->fmt_ctx = fmt_ctx;
    context->stream_id = stream_id;
    context->sws_ctx = sws_ctx;

    return context;
}

void
de_context_free (DeContext *context) {
    avformat_close_input (&context->fmt_ctx);
    free (context);
}

static int
find_video_stream (AVFormatContext *fmt_ctx) {
    int i;
    int stream_id;

    for(i = 0; i < fmt_ctx->nb_streams; i++) {
        if(fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            stream_id = i;
            break;
        }
    }

    return stream_id;
}

DeContext *
de_context_create (const char *infile)
{
    AVFormatContext *fmt_ctx = NULL;
    int stream_id = -1;
    AVStream *stream = NULL;
    AVCodec *decoder = NULL;
    AVDictionary *options = NULL;
    int fail = 0;
    DeContext *context;
    struct SwsContext *sws_ctx;

    /* register all formats and codecs */
    av_register_all ();

    /* Open input file, and allocate format context */
    if (avformat_open_input (&fmt_ctx, infile, NULL, NULL) < 0) {
        fprintf (stderr, "Could not open source file %s\n", infile);
        fail = 1;
        goto out;
    }
    printf("[LOG] Input file opened\n");

    /* Retrieve stream nformation */
    if (avformat_find_stream_info (fmt_ctx, NULL) < 0) {
        fprintf (stderr, "Could not find stream information\n");
        fail = 1;
        goto out;
    }
    printf("[LOG] Got stream info\n");

    av_dump_format (fmt_ctx, 0, infile, 0);

    /* Find video stream */
    stream_id = find_video_stream (fmt_ctx);
    if (stream_id == -1) {
        printf("Could not find stream\n");
        fail = 1;
        goto out;
    }
    printf("[LOG] Got video stream, id = %d\n", stream_id);

    stream = fmt_ctx->streams[stream_id];

    /* Find decoder for stream */
    decoder = avcodec_find_decoder (stream->codec->codec_id);
    if (!decoder) {
        fprintf(stderr, "Failed to find codec\n");
        fail = 1;
        goto out;
    }
    printf("[LOG] Found codec\n");

    if (avcodec_open2(stream->codec, decoder, &options) < 0) {
        fprintf(stderr, "Failed to open codec\n");
        fail = 1;
        goto out;
    }
    printf("[LOG] Opened codec\n");

    sws_ctx = sws_getContext(stream->codec->width,
                             stream->codec->height,
                             stream->codec->pix_fmt,
                             stream->codec->width,
                             stream->codec->height,
                             AV_PIX_FMT_RGB24,
                             SWS_BICUBIC,
                             NULL,
                             NULL,
                             NULL);

out:
    if (fail) {
        avformat_close_input(&fmt_ctx);
        return NULL;
    }

    context = de_context_new (fmt_ctx, stream_id, sws_ctx);

    return context;
}

AVFrame *
de_context_get_next_frame (DeContext *context, int *got_frame) {
    AVPacket pkt;
    AVCodecContext *codec_ctx;
    AVFrame *rgb_frame = NULL;
    AVFrame *yuv_frame;
    int num_bytes;
    int ret;
    uint8_t *buffer;

    codec_ctx = context->fmt_ctx->streams[context->stream_id]->codec;

    /* Init packet and let av_read_frame fill it */
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    /* Allocate frames */
    yuv_frame = av_frame_alloc();
    rgb_frame = av_frame_alloc();

    // Determine required buffer size and allocate buffer
    num_bytes = avpicture_get_size(AV_PIX_FMT_RGB24, codec_ctx->width, codec_ctx->height);
    buffer = (uint8_t *)av_malloc(num_bytes*sizeof(uint8_t));

    avpicture_fill((AVPicture *)rgb_frame, buffer, AV_PIX_FMT_RGB24,
                   codec_ctx->width, codec_ctx->height);

    if (av_read_frame (context->fmt_ctx, &pkt) >= 0) {
        printf("[LOG] Got packet\n");
        if (pkt.stream_index == context->stream_id) {
            ret = avcodec_decode_video2 (codec_ctx,
                                         yuv_frame,
                                         got_frame,
                                         &pkt);
            if (ret < 0) {
                fprintf(stderr, "Error decoding video frame (%s)\n", av_err2str(ret));
                return NULL;
            }

            if (*got_frame) {
                printf("[LOG] Got frame\n");
                /* Convert to RGB */
                sws_scale (context->sws_ctx,
                           (uint8_t const * const *)yuv_frame->data,
                           yuv_frame->linesize,
                           0,
                           codec_ctx->height,
                           rgb_frame->data,
                           rgb_frame->linesize);

                rgb_frame->width = yuv_frame->width;
                rgb_frame->height = yuv_frame->height;

                printf("[LOG] Frame converted\n");
                return rgb_frame;
            }
        }
    } else {
        *got_frame = -1;
    }

    return NULL;
}
