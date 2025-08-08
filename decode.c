/*
 * Exemplo de programa para demosntrar o uso da biblioteca libavcodec,
 * decodificando um arquivo de vídeo e salvando como um arquivo de vídeo bruto
 * 
 * Esse código é uma adptação de https://ffmpeg.org/doxygen/4.4/decode_video_8c-example.html
 *
 * Como compilar: gcc decode.c -o decode -lavcodec -lavutil
 * Como executar (exemplo): ./decode videos/entrada.h264 videos/saida.yuv 1280 720 yuv420p h264
 * Como reproduzir vídeo decodificado (exemplo): ffplay -f rawvideo -video_size 1280x720 -framerate 30 -pixel_format yuv420p videos/saida.yuv
 */
 
#include <stdio.h>
#include <stdlib.h>
 
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
 
#define INBUF_SIZE 4096

const char *input_filename, *output_filename;
const char *pix_fmt_name, *codec_name;
int width, height;

FILE *input_file, *output_file;

const AVCodec *codec;
AVCodecContext *codec_ctx;
AVCodecParserContext *parser;
AVPacket *pkt;
AVFrame *frame;
enum AVPixelFormat pix_fmt;
uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
uint8_t *data;
size_t data_size;
uint8_t *image_data[4];
int image_linesize[4];
int image_bufsize;
 
void decode(AVCodecContext *dec_ctx, AVFrame *output_frame, AVPacket *input_pkt)
{
    int ret;
 
    ret = avcodec_send_packet(dec_ctx, input_pkt);
    if (ret < 0) {
        printf("Erro ao enviar um pacote para decodificação\n");
        exit(1);
    }
 
    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, output_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            printf("Erro durante a decodificação\n");
            exit(1);
        }
 
        /* copy decoded frame to destination buffer:
        * this is required since rawvideo expects non aligned data */
        av_image_copy(image_data, image_linesize,
                      (const uint8_t **)(output_frame->data), output_frame->linesize,
                      pix_fmt, width, height);
    
        /* write to rawvideo file */
        fwrite(image_data[0], 1, image_bufsize, output_file);
    }
}
 
int main(int argc, char **argv)
{
    int ret;
    
    if (argc != 7) {
        printf("Entrada inválida\n");
        return 1;
    }
    input_filename  = argv[1];
    output_filename = argv[2];
    width           = atoi(argv[3]);
    height          = atoi(argv[4]);
    pix_fmt_name    = argv[5];
    codec_name      = argv[6];

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
 
    codec = avcodec_find_decoder_by_name(codec_name);
    if (codec == NULL) {
        printf("Codec não encontrado\n");
        return 1;
    }
 
    parser = av_parser_init(codec->id);
    if (parser == NULL) {
        printf("Parser não encontrado\n");
        return 1;
    }
 
    codec_ctx = avcodec_alloc_context3(codec);
    if (codec_ctx == NULL) {
        printf("Não foi possível alocar o contexto do codec de vídeo\n");
        return 1;
    }
 
    /* For some codecs, such as msmpeg4 and mpeg4, width and height
       MUST be initialized there because this information is not
       available in the bitstream. */
 
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

    pix_fmt = av_get_pix_fmt(pix_fmt_name);
    if(pix_fmt == AV_PIX_FMT_NONE){
        printf("Não foi possível encontrar formato de pixel\n");
        return 1;
    }

    /* allocate image where the decoded image will be put */
    ret = av_image_alloc(image_data, image_linesize, width, height, pix_fmt, 1);
    if (ret < 0) {
        printf("Não foi possível alocar o buffer de vídeo decodificado\n");
        return ret;
    }
    image_bufsize = ret;
 
    while (feof(input_file) == 0) {
        /* read raw data from the input file */
        data_size = fread(inbuf, 1, INBUF_SIZE, input_file);
        if (data_size == 0)
            break;
 
        /* use the parser to split the data into frames */
        data = inbuf;
        while (data_size > 0) {
            ret = av_parser_parse2(parser, codec_ctx, &pkt->data, &pkt->size,
                                   data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (ret < 0) {
                printf("Erro durante o parsing\n");
                return ret;
            }
            data      += ret;
            data_size -= ret;
 
            if (pkt->size != 0)
                decode(codec_ctx, frame, pkt);
        }
    }
 
    /* flush the decoder */
    decode(codec_ctx, frame, NULL);
 
    fclose(input_file);
    fclose(output_file);
 
    av_parser_close(parser);
    avcodec_free_context(&codec_ctx);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    av_free(image_data[0]);
 
    return 0;
}
