/*Copyright (c) [2019] [Seek Thermal, Inc.]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The Software may only be used in combination with Seek cores/products.

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// gcc src/seekware-video.c -o seekware-video -lavcodec  -lavutil -lswscale -lseekware

/* Things to add:
  -Command line options for giving:
    - Bitrate -> bit_rate - default now 400000
    - Codec type -> codec_name - default now "libx264"
    - Output filename -> filename - default now test.mpeg

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * Project:     Seek Thermal SDK Demo
 * Purpose:     Demonstrates how to record video to encoded video file
 * Author:      Seek Thermal, Inc.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <math.h>

#include <seekware/seekware.h>

// FFMPEG
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>


#define NUM_CAMS           5
#define SCALE_FACTOR       2
#define DRAW_RETICLE       true

typedef struct display_lut_t {
    const char * name;
    sw_display_lut value;
}display_lut_t;

static display_lut_t lut_names[] = {
    { "white",  SW_LUT_WHITE    },
    { "black",  SW_LUT_BLACK    },
    { "iron",   SW_LUT_IRON     },
    { "cool",   SW_LUT_COOL     },
    { "amber",  SW_LUT_AMBER    },
    { "indigo", SW_LUT_INDIGO   },
    { "tyrian", SW_LUT_TYRIAN   },
    { "glory",  SW_LUT_GLORY    },
    { "envy",   SW_LUT_ENVY     }
};

bool exit_requested = false;
sw_display_lut current_lut = SW_LUT_TYRIAN;

static void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt,
                   FILE *outfile)
{
    int ret;

    /* send the frame to the encoder */
    if (frame)
        printf("Send frame %ld\n", frame->pts);

    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending a frame for encoding\n");
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during encoding\n");
            exit(1);
        }

        printf("Write packet %ld (size=%5d)\n", pkt->pts, pkt->size);
        fwrite(pkt->data, 1, pkt->size, outfile);
        av_packet_unref(pkt);
        fflush(stderr);
    }
}

static inline void print_time(void) {
	static time_t t = 0;
	static struct tm* timeptr = NULL;
	time(&t);
    timeptr = localtime(&t);
    printf("\n%s", asctime(timeptr));
}

static inline void print_fw_info(psw camera) {
    sw_retcode status;
    int therm_ver;

    printf("Model Number:%s\n",camera->modelNumber);
    printf("SerialNumber: %s\n", camera->serialNumber);
    printf("Manufacture Date: %s\n", camera->manufactureDate);

    printf("Firmware Version: %u.%u.%u.%u\n",
            camera->fw_version_major,
			camera->fw_version_minor,
            camera->fw_build_major,
			camera->fw_build_minor);

	status = Seekware_GetSettingEx(camera, SETTING_THERMOGRAPHY_VERSION, &therm_ver, sizeof(therm_ver));
	if (status != SW_RETCODE_NONE) {
		fprintf(stderr, "Error: Seek GetSetting returned %i\n", status);
	}
    printf("Thermography Version: %i\n", therm_ver);

	sw_sdk_info sdk_info;
	Seekware_GetSdkInfo(NULL, &sdk_info);
	printf("Image Processing Version: %u.%u.%u.%u\n",
		sdk_info.lib_version_major,
		sdk_info.lib_version_minor,
		sdk_info.lib_build_major,
		sdk_info.lib_build_minor);

	printf("\n");
    fflush(stdout);
}
static void signal_callback(int signum) {
    printf("\nExit requested!\n");
    exit_requested  = true;
}

static void help(const char * name) {
    printf(
        "Usage: %s [option]...\n"
        "Valid options are:\n"
        "   -h | --help                             Prints help.\n"
        "   -lut | --lut <l>                        Sets the given LUT for drawing a color image. The following values can be set (default is \"black\"):\n"

        "                         ", name
    );

    for (int j = 0; lut_names[j].name; ++j) {
        if (j) {
            printf(", ");
        }
        printf("%s", lut_names[j].name);
    }
    printf("\n");
}

static int parse_cmdline(int argc, char **argv) {
    for (int i=1; i<argc; ++i) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            help(argv[0]);
            return 0;
        }
        else if (!strcmp(argv[i], "-lut") || !strcmp(argv[i], "--lut")) {
            char* lut_name = argv[++i];
            bool found = false;
            for (int j = 0; lut_names[j].name; ++j) {
                if (!strcasecmp(lut_name, lut_names[j].name)) {
                    current_lut = lut_names[j].value;
                    found = true;
                    break;
                }
            }
            if (!found) {
                fprintf(stderr, "ERROR: Unknown parameter \"%s\" for \"-lut\"\n", lut_name);
                help(argv[0]);
                return 1;
            }
        }
        else {
            fprintf(stderr, "Unknown parameter: \"%s\"\n", argv[i]);
            help(argv[0]);
            return 1;
        }
    }
    return 1;
}


int main(int argc, char ** argv) {

	size_t frame_count = 0;
	size_t frame_pixels = 0;
  uint32_t* image_buffer = NULL;

  bool camera_running = false;
	psw camera = NULL;
	psw camera_list[NUM_CAMS] = {0};
  sw_retcode status = SW_RETCODE_NONE;

  const char *filename, *codec_name;
  const AVCodec *codec;
  AVCodecContext *c= NULL;
  int ret;
  FILE *f;
  AVFrame *frame;
  AVPacket *pkt;
//  uint8_t endcode[] = { 0, 0, 1, 0xb7 };

  // Set default output & codec, these could be command line parameters
  filename = "test.mpeg";
  codec_name = "libx264";


  signal(SIGINT, signal_callback);
  signal(SIGTERM, signal_callback);

  printf("seekware-video: Seek Thermal Video recording\n\n");

	sw_sdk_info sdk_info;
	Seekware_GetSdkInfo(NULL, &sdk_info);
	printf("SDK Version: %u.%u\n\n", sdk_info.sdk_version_major, sdk_info.sdk_version_minor);

/* * * * * * * * * * * * * Find Seek Cameras * * * * * * * * * * * * * * */

    int num_cameras_found = 0;
    Seekware_Find(camera_list, NUM_CAMS, &num_cameras_found);
    if (num_cameras_found == 0) {
        printf("Cannot find any cameras.\n");
        goto cleanup;
    }

/* * * * * * * * * * * * * Initialize Seek SDK * * * * * * * * * * * * * * */

    //Open the first Seek Camera found by Seekware_Find
    for(int i = 0; i < num_cameras_found; ++i){
        camera = camera_list[i];
        status = Seekware_Open(camera);
        if (status == SW_RETCODE_OPENEX) {
            continue;
        }
        if(status != SW_RETCODE_NONE){
            fprintf(stderr, "Could not open camera (%d)\n", status);
        }
        camera_running = true;
        break;
    }

    if(status != SW_RETCODE_NONE){
        fprintf(stderr, "Could not open camera (%d)\n", status);
        goto cleanup;
    }

	frame_pixels = (size_t)camera->frame_cols * (size_t)camera->frame_rows;

    printf("::Seek Camera Info::\n");
    print_fw_info(camera);

    // Set the default display lut value
    current_lut = SW_LUT_TYRIAN_NEW;

    // Parse the command line to additional settings
    if (parse_cmdline(argc, argv) == 0) {
		goto cleanup;
    }

    if(Seekware_SetSettingEx(camera, SETTING_ACTIVE_LUT, &current_lut, sizeof(current_lut)) != SW_RETCODE_NONE) {
        fprintf(stderr, "Invalid LUT index\n");
		goto cleanup;
    }

    /* * * * * * * * * * * * * Initialise video encoding * * * * * * * * * * * * * * */

    // Register codecs
    // This is deprecated on FFMPEG 4.x ->
    avcodec_register_all();

    // Set loggin level to suitable AV_LOG_FATAL - Something went wrong and recovery is not possible.
    // Other levels:
    // AV_LOG_WARNING - Something somehow does not look correct.
    // AV_LOG_INFO - Standard information.
    // AV_LOG_QUIET - Print no output.
    av_log_set_level(AV_LOG_FATAL);

    /* find the video encoder */
    codec = avcodec_find_encoder_by_name(codec_name);
    if (!codec) {
        fprintf(stderr, "Codec '%s' not found\n", codec_name);
        goto cleanup;
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        goto cleanup;
    }

    pkt = av_packet_alloc();
    if (!pkt)
        goto cleanup;

    /* put sample parameters */
    c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    c->width = camera->frame_cols;
    c->height = camera->frame_rows;
    /* frames per second */
    c->time_base = (AVRational){1, 9};
    c->framerate = (AVRational){9, 1};

    // FFMPEG says to use 2 for H.264
    c->ticks_per_frame = 2;

    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    c->gop_size = 9;
    c->max_b_frames = 1;
    c->pix_fmt = AV_PIX_FMT_YUV420P;

    if (codec->id == AV_CODEC_ID_H264)
        av_opt_set(c->priv_data, "preset", "slow", 0);

    /* open it */
    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open codec: %s\n", av_err2str(ret));
        goto cleanup;
    }

    f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        goto cleanup;
    }

    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        goto cleanup;
    }
    frame->format = c->pix_fmt;
    frame->width  = c->width;
    frame->height = c->height;

    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate the video frame data\n");
        goto cleanup;
    }

    // Use libswscale to convert from ARGB buffer to AV_PIX_FMT_YUV420P
    // Allocate swscale AVCodecContext
    struct SwsContext *sws_scaler_ctx = sws_getContext(
      frame->width, frame->height, AV_PIX_FMT_RGB32,
      frame->width, frame->height, frame->format,
      SWS_FAST_BILINEAR, NULL, NULL, NULL
    );

    if (!sws_scaler_ctx){
      printf("Couldn't allocate swscale context\n");
      exit(1);
    }

    // RGB data is on one plane mixed BGRABGRA but swscale needs the data planes
    image_buffer = (uint32_t*) malloc(frame_pixels * sizeof(uint32_t));
    const uint8_t* source[4] = {(uint8_t*)image_buffer, 0 ,0 ,0};
    const int source_linesize[4]= {frame->width*4, 0, 0, 0};


/* * * * * * * * * * * * * Imaging Loop * * * * * * * * * * * * * * */

    print_time(); printf(" Start! \n");
    printf("Capturing %d x %d picture = %ld pixels\n", camera->frame_cols, camera->frame_rows, frame_pixels);

    do{

		//Get data from the camera
    status =  Seekware_GetDisplayImage(camera, image_buffer, (uint32_t)frame_pixels);

		//Check for errors
        if(camera_running){
            if(status == SW_RETCODE_NOFRAME){
                print_time(); printf(" Seek Camera Timeout ...\n");
            }
            if(status == SW_RETCODE_DISCONNECTED){
                print_time(); printf(" Seek Camera Disconnected ...\n");
            }
            if(status != SW_RETCODE_NONE){
                print_time(); printf(" Seek Camera Error : %u ...\n", status);
                break;
            }
        }

        ret = av_frame_make_writable(frame);
        if (ret < 0)
            exit(1);

        // Use Swscale to convert colorspace
            sws_scale(sws_scaler_ctx,
                      source, source_linesize, 0, frame->height,
                      frame->data, frame->linesize);


        frame->pts = frame_count;

        /* encode the image */
        encode(c, frame, pkt, f);

        ++frame_count;


	} while (!exit_requested);

/* * * * * * * * * * * * * Cleanup * * * * * * * * * * * * * * */
cleanup:

  printf("Exiting...\n");

  // somehow make sure the encode finishes, busy loop ?

	if(camera != NULL){
		Seekware_Close(camera);
	}
    if(image_buffer != NULL){
        free(image_buffer);
    }

  // Free the scaler context
  if (sws_scaler_ctx)
    sws_freeContext(sws_scaler_ctx);

  // Close output file
  if (f){
    fclose(f);
  }

  avcodec_free_context(&c);
  av_frame_free(&frame);
  av_packet_free(&pkt);

  return 0;
}

/* * * * * * * * * * * * * End - of - File * * * * * * * * * * * * * * */
