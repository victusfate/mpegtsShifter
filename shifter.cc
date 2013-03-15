#include <cstdlib>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/avutil.h>
}
using namespace std;

static AVStream * addStream(AVFormatContext *pFormatCtx, AVStream *pInStream) {
    AVCodecContext  *pInCodecCtx    = NULL;
    AVCodecContext  *pOutCodecCtx   = NULL;
    AVCodec         *pCodec         = NULL;
    AVStream        *pOutStream     = NULL;
    AVDictionary    *pOptionsDict   = NULL;

    // cout << "addStream 0" << endl;

    // Get a pointer to the codec context for the video stream
    pInCodecCtx = pInStream->codec;
    pCodec      = avcodec_find_decoder(pInCodecCtx->codec_id);


    // cout << "addStream 1" << endl;

    if (pCodec == NULL) {
        cout << "Unsupported codec (" << pInCodecCtx->codec_name << ")" << endl;
        exit(1);
    }

    // Open codec
    if (avcodec_open2(pInCodecCtx, pCodec, &pOptionsDict)<0) {
        cout << "Unable to open codec (" << pInCodecCtx->codec_name << ")" << endl;
        exit(1);
    }

    // cout << "addStream 2" << endl;


    pOutStream = avformat_new_stream(pFormatCtx, pCodec);
    if (!pOutStream) {
        cout << "Could not allocate stream" << endl;
        exit(1);
    }

    // cout << "addStream 3" << endl;


    pOutCodecCtx = pOutStream->codec;

    // surprised there's not a codec context copy function
    pOutCodecCtx->codec_id          = pInCodecCtx->codec_id;
    pOutCodecCtx->codec_type        = pInCodecCtx->codec_type;
    pOutCodecCtx->codec_tag         = pInCodecCtx->codec_tag;
    pOutCodecCtx->bit_rate          = pInCodecCtx->bit_rate;
    pOutCodecCtx->extradata         = pInCodecCtx->extradata;
    pOutCodecCtx->extradata_size    = pInCodecCtx->extradata_size;

    // cout << "addStream 4" << endl;


    if(av_q2d(pInCodecCtx->time_base) * pInCodecCtx->ticks_per_frame > av_q2d(pInStream->time_base) && av_q2d(pInStream->time_base) < 1.0/1000) {
        pOutCodecCtx->time_base = pInCodecCtx->time_base;
        pOutCodecCtx->time_base.num *= pInCodecCtx->ticks_per_frame;
    }
    else {
        pOutCodecCtx->time_base = pInStream->time_base;
    }

    // cout << "addStream 5" << endl;


    switch (pInCodecCtx->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
            pOutCodecCtx->channel_layout = pInCodecCtx->channel_layout;
            pOutCodecCtx->sample_rate = pInCodecCtx->sample_rate;
            pOutCodecCtx->channels = pInCodecCtx->channels;
            pOutCodecCtx->frame_size = pInCodecCtx->frame_size;
            if ((pInCodecCtx->block_align == 1 && pInCodecCtx->codec_id == CODEC_ID_MP3) || pInCodecCtx->codec_id == CODEC_ID_AC3) {
                pOutCodecCtx->block_align = 0;
            }
            else {
                pOutCodecCtx->block_align = pInCodecCtx->block_align;
            }
            break;
        case AVMEDIA_TYPE_VIDEO:
            pOutCodecCtx->pix_fmt = pInCodecCtx->pix_fmt;
            pOutCodecCtx->width = pInCodecCtx->width;
            pOutCodecCtx->height = pInCodecCtx->height;
            pOutCodecCtx->has_b_frames = pInCodecCtx->has_b_frames;

            if (pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
                pOutCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
            }
            break;
    default:
        break;
    }

    return pOutStream;
}

int main(int argc, char **argv)
{
    string input;
    string outputPrefix;

    AVInputFormat *pInFormat = NULL;
    AVOutputFormat *pOutFormat = NULL;
    AVFormatContext *pInFormatCtx = NULL;
    AVFormatContext *pOutFormatCtx = NULL;
    AVStream *pVideoStream = NULL;
    AVStream *pAudioStream = NULL;
    AVCodec *pCodec = NULL;
    int videoIndex;
    int audioIndex;
    int decodeDone;
    int ret;
    unsigned int i;

    if (argc != 4) {
        cout << "Usage: " << argv[0] << " <input MPEG-TS file> <offset time> <output MPEG-TS file>" << endl;
        exit(1);
    }

    // cout << "start" << endl;

    av_register_all();

    input = argv[1];
    if (!strcmp(input.c_str(), "-")) {
        input = "pipe:";
    }
    double offsetTime = atof(argv[2]);
    unsigned int tsShift = offsetTime * 90000; // h264 defined sample rate is 90khz
    string outFile = argv[3];

    // cout << "0" << endl;

    pInFormat = av_find_input_format("mpegts");
    if (!pInFormat) {
        cout << "Could not find MPEG-TS demuxer" << endl;
        exit(1);
    }
    // cout << "0.5" << endl;

    ret = avformat_open_input(&pInFormatCtx, input.c_str(), pInFormat, NULL);
    if (ret != 0) {
        cout << "Could not open input file, make sure it is an mpegts file: " << ret << endl;
        exit(1);
    }
    // cout << "0.6" << endl;

    if (avformat_find_stream_info(pInFormatCtx, NULL) < 0) {
        cout << "Could not read stream information" << endl;
        exit(1);
    }
    // cout << "0.7" << endl;
    av_dump_format(pInFormatCtx, 0, argv[1], 0);
    // cout << "0.75" << endl;

    pOutFormat = av_guess_format("mpegts", NULL, NULL);
    if (!pOutFormat) {
        cout << "Could not find MPEG-TS muxer" << endl;
        exit(1);
    }
    // cout << "0.8" << endl;

    pOutFormatCtx = avformat_alloc_context();
    if (!pOutFormatCtx) {
        cout << "Could not allocated output context" << endl;
        exit(1);
    }
    // cout << "0.9" << endl;


    pOutFormatCtx->oformat = pOutFormat;

    videoIndex = -1;
    audioIndex = -1;

    for (i = 0; i < pInFormatCtx->nb_streams && (videoIndex < 0 || audioIndex < 0); i++) {
        // cout << "checking streams for video, current stream type = " << pInFormatCtx->streams[i]->codec->codec_type;
        // cout << " AVMEDIA_TYPE_VIDEO = " << AVMEDIA_TYPE_VIDEO << " AVMEDIA_TYPE_AUDIO = " <<  AVMEDIA_TYPE_AUDIO << endl;

        switch (pInFormatCtx->streams[i]->codec->codec_type) {
            case AVMEDIA_TYPE_VIDEO:
                videoIndex = i;
                pInFormatCtx->streams[i]->discard = AVDISCARD_NONE;
                pVideoStream = addStream(pOutFormatCtx, pInFormatCtx->streams[i]);
                break;
            case AVMEDIA_TYPE_AUDIO:
                audioIndex = i;
                pInFormatCtx->streams[i]->discard = AVDISCARD_NONE;
                pAudioStream = addStream(pOutFormatCtx, pInFormatCtx->streams[i]);
                break;
            default:
                pInFormatCtx->streams[i]->discard = AVDISCARD_ALL;
                break;
        }
    }
    // cout << "0.95" << endl;
    
    pCodec = avcodec_find_decoder(pVideoStream->codec->codec_id);
    if (!pCodec) {
        cout << "Could not find video decoder, key frames will not be honored" << endl;
    }

    // cout << "0.97" << endl;

    if (avcodec_open2(pVideoStream->codec, pCodec, NULL) < 0) {
        cout << "Could not open video decoder, key frames will not be honored" << endl;
    }

    // cout << "0.98" << endl;

    // stringstream outStream;
    // outStream << outputPrefix << std::setfill('0') << std::setw(3) << outputIndex << ".ts";
    // outputIndex++;
    // outStream >> outFile;

    // cout << "0.99" << endl;

    if (avio_open(&pOutFormatCtx->pb, outFile.c_str(), AVIO_FLAG_WRITE) < 0) {
        cout << "Could not open " << outFile << endl;
        exit(1);
    }

    // cout << "0.991" << endl;

    if (avformat_write_header(pOutFormatCtx,NULL)) {
        cout << "Could not write mpegts header to first output file" << endl;
        exit(1);
    }

    // cout << "0.992" << endl;

    do {
        // double segmentTime;
        AVPacket packet;

        decodeDone = av_read_frame(pInFormatCtx, &packet);
        if (decodeDone < 0) {
            break;
        }

        if (av_dup_packet(&packet) < 0) {
            cout << "Could not duplicate packet" << endl;
            av_free_packet(&packet);
            break;
        }

        // cout << "before packet pts dts " << packet.pts << " " << packet.dts;
        packet.pts += tsShift;
        packet.dts += tsShift;
        // cout << " after packet pts dts " << packet.pts << " " << packet.dts << endl;


        ret = av_interleaved_write_frame(pOutFormatCtx, &packet);
        if (ret < 0) {
            cout << "Warning: Could not write frame of stream" << endl;
        }
        else if (ret > 0) {
            cout <<  "End of stream requested" << endl;
            av_free_packet(&packet);
            break;
        }

        av_free_packet(&packet);

    } while (!decodeDone);

    av_write_trailer(pOutFormatCtx);

    avcodec_close(pVideoStream->codec);

    for(i = 0; i < pOutFormatCtx->nb_streams; i++) {
        av_freep(&pOutFormatCtx->streams[i]->codec);
        av_freep(&pOutFormatCtx->streams[i]);
    }

    avio_close(pOutFormatCtx->pb);
    av_free(pOutFormatCtx);


    return 0;
}
