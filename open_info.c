/*
 * Exemplo de programa para desmonstrar o uso da biblioteca libavformat,
 * abrindo um arquivo de vídeo e imprimindo seus metadados
 * 
 * Esse código é uma adptação de https://ffmpeg.org/doxygen/4.4/metadata_8c-example.html
 * 
 * Como compilar: gcc open_info.c -o open_info -lavformat -lavutil
 * Como executar: ./open_info caminho/para/arquivo/video
 */
 
#include <stdio.h>

#include <libavformat/avformat.h>
 
int main (int argc, char **argv)
{
    // Seção de variáveis
    AVFormatContext *fmt_ctx = NULL;
    const char *input_file = NULL;
    int ret;
 
    // Seção de validação dos parâmetros passados pela linha de comando
    if (argc != 2) {
        printf("Entrada inválida\n"
               "Uso: %s caminho/para/arquivo/video\n", argv[0]);
        return 1;
    }

    input_file = argv[1];
 
    // Seção de configurações iniciais
    ret = avformat_open_input(&fmt_ctx, input_file, NULL, NULL);
    if (ret != 0){
        printf("Falha ao abrir o stream da entrada\n"
               "Erro %d: %s\n", ret, av_err2str(ret));
        return ret;
    }

    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0) {
        printf("Não é possível encontrar informações da stream\n"
               "Erro %d: %s\n", ret, av_err2str(ret));
        return ret;
    }

    // Seção de execução de tarefas principais
    av_dump_format(fmt_ctx, 0, input_file, 0);
 
    // Seção de encerramento
    avformat_close_input(&fmt_ctx);

    return 0;
}