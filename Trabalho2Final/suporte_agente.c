#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PIPE_SUPPORT "/tmp/suporte"
#define MAX_DISCIPLINES 10
#define MAX_HORARIOS 5

typedef struct
{
    int vagas;
} Horario;

typedef struct
{
    Horario horarios[MAX_HORARIOS];
} Disciplina;

Disciplina disciplinas[MAX_DISCIPLINES];
pthread_mutex_t lock;

void *handle_request(void *arg)
{
    char *message = (char *)arg;

    int initial_student, num_students;
    char response_pipe[256];
    sscanf(message, "%d %d %s", &initial_student, &num_students, response_pipe);
    printf("[support_agent] Pedido recebido: %s\n", message);

    pthread_mutex_lock(&lock);
    int students_registered = 0;

    // Alocar alunos a horários
    int remaining_students = num_students;

    for (int d = 0; d < MAX_DISCIPLINES && remaining_students > 0; d++)
    {
        for (int h = 0; h < MAX_HORARIOS && remaining_students > 0; h++)
        {
            if (disciplinas[d].horarios[h].vagas > 0)
            {
                int allocated = disciplinas[d].horarios[h].vagas >= remaining_students ? remaining_students : disciplinas[d].horarios[h].vagas;
                disciplinas[d].horarios[h].vagas -= allocated;
                remaining_students -= allocated;

                printf("[support_agent] Alocando %d alunos na disciplina %d, horário %d. Vagas restantes: %d\n",
                       allocated, d, h, disciplinas[d].horarios[h].vagas);
            }
        }
    }

    students_registered = num_students - remaining_students;
    printf("[support_agent] Total de alunos alocados: %d\n", students_registered);

    pthread_mutex_unlock(&lock);

    printf("[support_agent] Alunos alocados: %d. Enviando resposta para: %s\n", students_registered, response_pipe);

    // Enviar resposta
    int fd_response = open(response_pipe, O_WRONLY);
    write(fd_response, &students_registered, sizeof(students_registered));
    close(fd_response);

    printf("[support_agent] Resposta enviada para %s\n", response_pipe);
    free(message);
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <num_students>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int num_students = atoi(argv[1]);

    printf("[support_agent] Iniciado. Total de alunos: %d\n", num_students);

    // Inicializar disciplinas e horários
    pthread_mutex_init(&lock, NULL);
    for (int d = 0; d < MAX_DISCIPLINES; d++)
    {
        for (int h = 0; h < MAX_HORARIOS; h++)
        {
            disciplinas[d].horarios[h].vagas = 1; // Exemplo: 10 vagas por horário
        }
    }

    // Criar o named pipe do suporte
    if (access(PIPE_SUPPORT, F_OK) == -1)
    {
        if (mkfifo(PIPE_SUPPORT, 0666) == -1)
        {
            perror("[support_agent] mkfifo");
            exit(EXIT_FAILURE);
        }
        printf("[support_agent] Pipe principal criado: %s\n", PIPE_SUPPORT);
    }
    else
    {
        printf("[support_agent] Pipe principal já existe: %s\n", PIPE_SUPPORT);
    }
    // Processar pedidos
    while (num_students > 0)
    {
        int fd_support = open(PIPE_SUPPORT, O_RDONLY);
        if (fd_support == -1)
        {
            perror("[suporte_agente] open suporte pipe");
            continue;
        }

        char *message = malloc(256);
        read(fd_support, message, 256);
        close(fd_support);

        pthread_t thread;
        pthread_create(&thread, NULL, handle_request, message);
        pthread_detach(thread);

        num_students -= 1; // Exemplo simples, decrementa por pedido processado
    }

    unlink(PIPE_SUPPORT);
    pthread_mutex_destroy(&lock);
    printf("[support_agent] Pipe principal removido: %s\n", PIPE_SUPPORT);
    return 0;
}