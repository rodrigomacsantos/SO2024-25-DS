#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#define BSIZE 80
#define NOMEFIFO "/tmp/support_pipe"

int main(int argc, char const *argv[])
{
    char const *nome_pipe;
    int fd;
    char buf[BSIZE];

    if (argc == 3) {
        nome_pipe = argv[2];
    } else {
        nome_pipe = NOMEFIFO;
    }
    if ((fd = open (nome_pipe, O_WRONLY)) < 0) {
        perror ("open");
        exit(1);
    }
    strcpy (buf, argv[1]);
    strcat (buf, "\n");
    //printf ("student: %s, buf=%s", argv[1], buf);
    write (fd, buf, strlen(buf)+1);
    return 0;
}



