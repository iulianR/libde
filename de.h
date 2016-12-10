#include <stdint.h>

#include <libavformat/avformat.h>

typedef struct {
    AVFormatContext *fmt_ctx;
    int stream_id;
    struct SwsContext *sws_ctx;
} DeContext;

void        de_context_free             (DeContext *context);
DeContext  *de_context_create           (const char *infile);
AVFrame    *de_context_get_next_frame   (DeContext *context, int *got_frame);
