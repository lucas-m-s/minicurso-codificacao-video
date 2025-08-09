/*
 * Exemplo de programa para desmonstrar o uso da biblioteca libavformat,
 * abrindo um arquivo de vídeo e imprimindo seus metadados
 * 
 * Esse código é uma adptação de https://ffmpeg.org/doxygen/4.4/metadata_8c-example.html
 * 
 * Como compilar: gcc open_info.c -o open_info -lavformat
 * Como executar (exemplo): ./open_info videos/entrada.mp4
 */

// Seção de inclusão de bibliotecas
#include <stdio.h>

#include <libavformat/avformat.h>

// Seção de variáveis
const char *input_filename;
AVFormatContext *fmt_ctx = NULL;
 
int main (int argc, char **argv)
{
    int ret;

    // Seção de validação dos parâmetros passados pela linha de comando
    if (argc != 2) {
        printf("Entrada inválida\n");
        return 1;
    }
    input_filename = argv[1];
 
    // Seção de configurações iniciais
    ret = avformat_open_input(&fmt_ctx, input_filename, NULL, NULL);
    if (ret != 0){
        printf("Falha ao abrir o stream da entrada\n");
        return ret;
    }

    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0) {
        printf("Não é possível encontrar informações da stream\n");
        return ret;
    }

    // Seção de execução de tarefas principais
    av_dump_format(fmt_ctx, 0, input_filename, 0);
 
    // Seção de encerramento
    avformat_close_input(&fmt_ctx);

    return 0;
}