#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h> // Para mkfifo

#define PIPE_SUPPORT "/tmp/suporte"

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <student_id> <initial_student> <num_students>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int student_id = atoi(argv[1]);
    int initial_student = atoi(argv[2]);
    int num_students = atoi(argv[3]);

    printf("[student %d] Pipe principal definido como: %s\n", student_id, PIPE_SUPPORT);

    printf("[student %d] Iniciado. Aluno inicial: %d, Total de alunos: %d\n", student_id, initial_student, num_students);

    char student_pipe[256];
    snprintf(student_pipe, sizeof(student_pipe), "/tmp/student_%d", student_id);

    // Criar o named pipe do estudante
    if (access(student_pipe, F_OK) == -1)
    {
        if (mkfifo(student_pipe, 0666) == -1)
        {
            perror("[student] mkfifo");
            exit(EXIT_FAILURE);
        }
        printf("[student %d] Pipe criado: %s\n", student_id, student_pipe);
    }
    else
    {
        printf("[student %d] Pipe jÃ¡ existe: %s\n", student_id, student_pipe);
    }
    // Enviar pedido ao suporte
    int fd_support = open(PIPE_SUPPORT, O_WRONLY);
    if (fd_support == -1)
    {
        perror("[student] open support pipe");
        unlink(student_pipe);
        exit(EXIT_FAILURE);
    }
    printf("[student %d] Conectado ao pipe principal: %s\n", student_id, PIPE_SUPPORT);

    char request[512];
    snprintf(request, sizeof(request), "%d %d %s", initial_student, num_students, student_pipe);
    write(fd_support, request, strlen(request) + 1);
    printf("[student %d] Pedido enviado: %s\n", student_id, request);
    close(fd_support);

    // Receber resposta
    int fd_student = open(student_pipe, O_RDONLY);
    if (fd_student == -1)
    {
        perror("[student] open student pipe");
        unlink(student_pipe);
        exit(EXIT_FAILURE);
    }

    int students_registered;
    read(fd_student, &students_registered, sizeof(students_registered));
    close(fd_student);
    printf("[student %d] Resposta recebida: %d alunos inscritos\n", student_id, students_registered);

    unlink(student_pipe);
    printf("[student %d] Pipe removido: %s\n", student_id, student_pipe);
    return 0;
}