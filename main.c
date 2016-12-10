#include "de.h"

int count = 0;

void ppm_save(AVFrame *frame)
{
  FILE *file;
  int y;
  char filename[16];

  printf("[LOG] Saving frame\n");
  sprintf(filename, "frame%d.ppm", count++);
  file = fopen(filename, "wb");
  if (file == NULL)
    return;

  /* Write header */
  fprintf(file, "P6\n%d %d\n255\n", frame->width, frame->height);

  /* Write data */
  for(y = 0; y < frame->height; y++)
    fwrite (frame->data[0] + y * frame->linesize[0], 1, frame->width * 3, file);

  fclose(file);
}

int main(int argc, char **argv)
{
    DeContext *context;
    AVFrame *frame = NULL;
    int got_frame;

    context = de_context_create ("test.mpg");

    do {
        frame = de_context_get_next_frame (context, &got_frame);
        if (got_frame == -1)
            break;

        if (got_frame)
            ppm_save(frame);
    } while (1);

    de_context_free (context);
    return 0;
}
