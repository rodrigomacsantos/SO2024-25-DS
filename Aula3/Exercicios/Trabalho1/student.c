#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define MAX_BUF 1024

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Uso: %s <nome_do_pipe> <pedido>\n", argv[0]);
        exit(1);
    }

    const char *pipe_name = argv[1];
    const char *pedido = argv[2];

    int fd = open(pipe_name, O_WRONLY);
    if (fd == -1)
    {
        fprintf(stderr, "Erro ao abrir o pipe %s: %s\n", pipe_name, strerror(errno));
        exit(1);
    }

    char buffer[MAX_BUF];
    snprintf(buffer, sizeof(buffer), "Student %s: %s", argv[2], pedido);

    ssize_t bytes_written = write(fd, buffer, strlen(buffer));
    if (bytes_written == -1)
    {
        fprintf(stderr, "Erro ao escrever no pipe: %s\n", strerror(errno));
        close(fd);
        exit(1);
    }

    printf("Pedido enviado com sucesso: %s\n", buffer);

    close(fd);
    return 0;
}