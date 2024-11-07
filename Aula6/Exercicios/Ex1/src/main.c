int le_pipe (int fd, char *msg_in, int bsize)
{
    int j;
    for(j=0; j<bsize; j++) msg_in[j]=0;
    for(j=0; j<bsize; j++) {
        if (read (fd, &msg_in[j], 1) < 0) {
            perror ("le_pipe read");
            exit(1);
        }
        if (msg_in[j] == '\0')
            break;
    }

    return j;
}

