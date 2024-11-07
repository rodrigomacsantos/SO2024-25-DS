#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>

#define FIFO_IN "/tmp/suporte"
#define BSIZE 256
#define NDISCIP 5   // Número de disciplinas
#define NHOR 20     // Número total de horários (5 disciplinas * 4 horários cada)
#define NLUG 30     // Número de lugares por horário
#define NALUN 100   // Número total de alunos

typedef struct {
    int disciplina;
    int horario;
    int vagas_disponiveis;
    int vagas_totais;
} HorarioInfo;

HorarioInfo horarios[NDISCIP][NHOR];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Função para processar um único pedido
void processar_pedido(char* msg_in) {
    pthread_mutex_lock(&mutex);
    char msg_out[BSIZE];
    printf("Mensagem recebida: %s\n", msg_in);

    int aluno_inicial, num_alunos;
    char nome_resp[BSIZE];
    sscanf(msg_in, "%d %d %s", &aluno_inicial, &num_alunos, nome_resp);

    int inscritos = 0;
    for (int i = 0; i < num_alunos; i++) {
        int horario_encontrado = 0;
        for (int d = 0; d < NDISCIP && !horario_encontrado; d++) {
            for (int h = 0; h < NHOR && !horario_encontrado; h++) {
                if (horarios[d][h].vagas_disponiveis > 0) {
                    horarios[d][h].vagas_disponiveis--;
                    inscritos++;
                    horario_encontrado = 1;
                    printf("Aluno %d inscrito na disciplina %d, horário %d. Vagas restantes: %d\n",
                           aluno_inicial + i, d, h, horarios[d][h].vagas_disponiveis);
                }
            }
        }
        if (!horario_encontrado) {
            printf("Não há mais horários disponíveis para o aluno %d\n", aluno_inicial + i);
            break;
        }
    }

    snprintf(msg_out, BSIZE, "%d", inscritos);
    int fd_out = open(nome_resp, O_WRONLY);
    if (fd_out == -1) {
        perror("Erro ao abrir o pipe de resposta");
    } else {
        write(fd_out, msg_out, strlen(msg_out) + 1);
        close(fd_out);
    }
    pthread_mutex_unlock(&mutex);
}

int main() {
    for (int i = 0; i < NDISCIP; i++) {
        for (int j = 0; j < NHOR; j++) {
            horarios[i][j].disciplina = i;
            horarios[i][j].horario = j;
            horarios[i][j].vagas_disponiveis = NLUG;
            horarios[i][j].vagas_totais = NLUG;
        }
    }

    if (mkfifo(FIFO_IN, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        return 1;
    }

    int fd = open(FIFO_IN, O_RDONLY);
    if (fd == -1) {
        perror("Erro ao abrir o pipe");
        unlink(FIFO_IN);
        return 1;
    }

    char msg_in[BSIZE];
    while (1) {
        ssize_t bytes_read = read(fd, msg_in, BSIZE);
        if (bytes_read > 0) {
            msg_in[bytes_read] = '\0';
            processar_pedido(msg_in);
        } else if (bytes_read == 0) {
            close(fd);
            fd = open(FIFO_IN, O_RDONLY);  // Reabrir o pipe para a próxima leitura
        } else {
            perror("Erro ao ler do pipe");
        }
    }

    close(fd);
    unlink(FIFO_IN);
    printf("Suport Agent: Encerrado\n");
    return 0;
}