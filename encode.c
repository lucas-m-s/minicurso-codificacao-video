/*
 * Exemplo de programa para demosntrar o uso da biblioteca libavcodec,
 * codificando um arquivo de vídeo bruto e salvando como um arquivo de vídeo codificado
 * 
 * Esse código é uma adptação de https://ffmpeg.org/doxygen/4.4/encode_video_8c-example.html
 *
 * Como compilar: gcc encode.c -o encode -lavcodec -lavutil
 * Como executar (exemplo): ./encode videos/entrada.yuv videos/saida.h264 1280 720 30 yuv420p libx264
 * Como reproduzir vídeo codificado (exemplo): ffplay videos/saida.h264
 */
 
#include <stdio.h>
#include <stdlib.h>
 
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>

const char *output_filename, *input_filename;
const char *pix_fmt_name, *codec_name;
int width, height, fps, frame_size;

FILE *output_file, *input_file;

const AVCodec *codec;
AVCodecContext *codec_ctx;
AVPacket *pkt;
AVFrame *frame;
enum AVPixelFormat pix_fmt;
int pts = 0;
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
 
    if (argc != 8) {
        printf("Entrada inválida\n");
        return 1;
    }
    input_filename = argv[1];
    output_filename = argv[2];
    width = atoi(argv[3]);
    height = atoi(argv[4]);
    fps = atoi(argv[5]);
    pix_fmt_name = argv[6];
    codec_name = argv[7];

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
 
    codec_ctx->bit_rate = 400000;
    codec_ctx->width = width;
    codec_ctx->height = height;
    codec_ctx->time_base = (AVRational){1, fps};
    codec_ctx->framerate = (AVRational){fps, 1};
 
    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    codec_ctx->gop_size = 10;
    codec_ctx->max_b_frames = 1;
    codec_ctx->pix_fmt = pix_fmt;
 
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
    frame_size = width * height;
 
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        printf("Não foi possível alocar os dados do quadro de vídeo\n");
        return 1;
    }

    while(feof(input_file) == 0){
        ret = fread(frame->data[0], frame_size, 1, input_file);
        if (ret <= 0)
            break;

        ret = fread(frame->data[1], frame_size/4, 1, input_file);
        if (ret <= 0)
            break;

        ret = fread(frame->data[2], frame_size/4, 1, input_file);
        if (ret <= 0)
            break;

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