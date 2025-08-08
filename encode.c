/*
 * Exemplo de programa para demosntrar o uso da biblioteca libavcodec,
 * codificando um arquivo de vídeo bruto e salvando como um arquivo de vídeo codificado.
 * Este programa foi implementado para codificar com o codec H.264, ou MPEG2, ou MJPEG.
 * 
 * Esse código é uma adptação de https://ffmpeg.org/doxygen/4.4/encode_video_8c-example.html
 *
 * Como compilar: gcc encode.c -o encode -lavcodec -lavutil
 * Como executar (exemplo): ./encode videos/entrada.yuv videos/saida.h264 1280 720 30 4000 yuv420p libx264
 * Como reproduzir vídeo codificado (exemplo): ffplay videos/saida.h264
 */
 
#include <stdio.h>
#include <stdlib.h>
 
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
#include <libavutil/imgutils.h>

const char *output_filename, *input_filename;
const char *pix_fmt_name, *codec_name;
int width, height, fps, bitrate;

FILE *output_file, *input_file;

const AVCodec *codec;
AVCodecContext *codec_ctx;
AVPacket *pkt;
AVFrame *frame;
enum AVPixelFormat pix_fmt;
uint8_t *image_data[4];
int image_linesize[4];
int image_bufsize;
long int pts = 0;
uint8_t endcode_mpeg[] = {0, 0, 1, 0xb7};
 
void encode(AVCodecContext *enc_ctx, AVFrame *input_frame, AVPacket *output_pkt)
{
    int ret;
 
    ret = avcodec_send_frame(enc_ctx, input_frame);
    if (ret < 0) {
        printf("Erro ao enviar um quadro para codificação\n");
        exit(1);
    }
 
    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, output_pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            printf("Erro durante a codificação\n");
            exit(1);
        }
 
        fwrite(output_pkt->data, 1, output_pkt->size, output_file);
        av_packet_unref(output_pkt);
    }
}
 
int main(int argc, char **argv)
{
    int ret;
 
    if (argc != 9) {
        printf("Entrada inválida\n");
        return 1;
    }
    input_filename  = argv[1];
    output_filename = argv[2];
    width           = atoi(argv[3]);
    height          = atoi(argv[4]);
    fps             = atoi(argv[5]);
    bitrate         = atoi(argv[6]) * 1000; // bitrate dado em Kb/s na entrada
    pix_fmt_name    = argv[7];
    codec_name      = argv[8];

    input_file = fopen(input_filename, "rb");
    if (input_file == NULL) {
        printf("Não foi possível abrir %s\n", input_filename);
        return 1;
    }

    output_file = fopen(output_filename, "wb");
    if (output_file == NULL) {
        printf("Não foi possível abrir %s\n", output_filename);
        return 1;
    }
 
    codec = avcodec_find_encoder_by_name(codec_name);
    if (codec == NULL) {
        printf("Codec não encontrado\n");
        return 1;
    }
 
    codec_ctx = avcodec_alloc_context3(codec);
    if (codec_ctx == NULL) {
        printf("Não foi possível alocar o contexto do codec de vídeo\n");
        return 1;
    }

    pix_fmt = av_get_pix_fmt(pix_fmt_name);
    if(pix_fmt == AV_PIX_FMT_NONE){
        printf("Não foi possível encontrar formato de pixel\n");
        return 1;
    }
 
    codec_ctx->bit_rate  = bitrate; // bitrate em b/s
    codec_ctx->width     = width;
    codec_ctx->height    = height;
    codec_ctx->time_base = (AVRational){1, fps};
    codec_ctx->framerate = (AVRational){fps, 1};
    codec_ctx->pix_fmt   = pix_fmt;
 
    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    codec_ctx->gop_size     = 10;
    if(codec->id != AV_CODEC_ID_MJPEG)
        codec_ctx->max_b_frames = 1;
 
    ret = avcodec_open2(codec_ctx, codec, NULL);
    if (ret < 0) {
        printf("Não foi possível abrir o codec\n");
        return ret;
    }

    pkt = av_packet_alloc();
    if (pkt == NULL){
        printf("Não foi possível alocar pacote de vídeo\n");
        return 1;
    }
 
    frame = av_frame_alloc();
    if (frame == NULL) {
        printf("Não foi possível alocar quadro de vídeo\n");
        return 1;
    }
    frame->format = pix_fmt;
    frame->width  = width;
    frame->height = height;
 
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        printf("Não foi possível alocar os dados do quadro de vídeo\n");
        return 1;
    }

    /* allocate image where the decoded image will be put */
    ret = av_image_alloc(image_data, image_linesize, width, height, pix_fmt, 1);
    if (ret < 0) {
        printf("Não foi possível alocar o buffer de vídeo decodificado\n");
        return ret;
    }
    image_bufsize = ret;

    while(feof(input_file) == 0){
        ret = fread(image_data[0], image_bufsize, 1, input_file);
        if (ret <= 0)
            break;
        
        av_image_copy(frame->data, frame->linesize,
                      (const uint8_t **)(image_data), image_linesize,
                      pix_fmt, width, height);

        frame->pts = pts;
        pts++;
        
        encode(codec_ctx, frame, pkt);
    }
 
    /* flush the encoder */
    encode(codec_ctx, NULL, pkt);
 
    /* add sequence end code to have a real MPEG file */
    if (codec->id == AV_CODEC_ID_MPEG1VIDEO || codec->id == AV_CODEC_ID_MPEG2VIDEO)
        fwrite(endcode_mpeg, 1, sizeof(endcode_mpeg), output_file);

    fclose(output_file);
    fclose(input_file);
 
    avcodec_free_context(&codec_ctx);
    av_frame_free(&frame);
    av_packet_free(&pkt);
 
    return 0;
}