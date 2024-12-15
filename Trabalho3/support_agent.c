#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define PIPE_SUPPORT "/tmp/suporte"
#define PIPE_ADMIN "/tmp/admin"
#define MAX_DISCIPLINES 10
#define MAX_HORARIOS 5
#define MAX_STUDENTS 256
#define INSCRICOES_POR_ALUNO 5
#define BUFFER_SIZE 1024

typedef struct
{
    int alunos_inscritos[MAX_STUDENTS];
    int num_inscritos;
} Horario;

typedef struct
{
    Horario horarios[MAX_HORARIOS];
} Disciplina;

Disciplina disciplinas[MAX_DISCIPLINES];
pthread_mutex_t trincos_disciplinas[MAX_DISCIPLINES];
pthread_mutex_t trinco_geral = PTHREAD_MUTEX_INITIALIZER;

int continuar_execucao = 1;
int inscricoes_alunos[MAX_STUDENTS] = {0}; // Controle de inscrições por aluno

int le_pipe(int fd, char *msg_in, int bsize);
int inscrever_aluno(int id_aluno, int disciplina);
void consultar_horarios(int num_aluno, const char *pipe_resposta);
void gravar_em_arquivo(const char *nome_arquivo, const char *pipe_resposta);

int le_pipe(int fd, char *msg_in, int bsize)
{
    if (fd < 0 || msg_in == NULL || bsize <= 0)
        return -1;

    int len = 0;
    char byte;

    while (len < bsize - 1)
    {
        int result = read(fd, &byte, 1);
        if (result == 1)
        {
            if (byte == '\0')
            {
                msg_in[len] = '\0';
                return len;
            }
            msg_in[len++] = byte;
        }
        else if (result == 0)
        {
            break;
        }
        else
        {
            return -1;
        }
    }

    msg_in[len] = '\0';
    return len;
}

int inscrever_aluno(int id_aluno, int disciplina)
{
    pthread_mutex_lock(&trincos_disciplinas[disciplina]);

    // Verificar se o aluno já está inscrito
    for (int h = 0; h < MAX_HORARIOS; h++)
    {
        for (int i = 0; i < disciplinas[disciplina].horarios[h].num_inscritos; i++)
        {
            if (disciplinas[disciplina].horarios[h].alunos_inscritos[i] == id_aluno)
            {
                pthread_mutex_unlock(&trincos_disciplinas[disciplina]);
                return h; // Já inscrito neste horário
            }
        }
    }

    // Inscrever o aluno no próximo horário disponível
    for (int h = 0; h < MAX_HORARIOS; h++)
    {
        if (disciplinas[disciplina].horarios[h].num_inscritos < MAX_STUDENTS)
        {
            int idx = disciplinas[disciplina].horarios[h].num_inscritos;
            disciplinas[disciplina].horarios[h].alunos_inscritos[idx] = id_aluno;
            disciplinas[disciplina].horarios[h].num_inscritos++;
            pthread_mutex_unlock(&trincos_disciplinas[disciplina]);
            return h;
        }
    }

    pthread_mutex_unlock(&trincos_disciplinas[disciplina]);
    return -1; // Sem vagas
}

void consultar_horarios(int num_aluno, const char *pipe_resposta)
{
    char resposta[BUFFER_SIZE];
    snprintf(resposta, sizeof(resposta), "Aluno %d: ", num_aluno);

    for (int d = 0; d < MAX_DISCIPLINES; d++)
    {
        pthread_mutex_lock(&trincos_disciplinas[d]);
        for (int h = 0; h < MAX_HORARIOS; h++)
        {
            for (int i = 0; i < disciplinas[d].horarios[h].num_inscritos; i++)
            {
                if (disciplinas[d].horarios[h].alunos_inscritos[i] == num_aluno)
                {
                    char temp[32];
                    snprintf(temp, sizeof(temp), "%d/%d, ", d, h);
                    strncat(resposta, temp, sizeof(resposta) - strlen(resposta) - 1);
                }
            }
        }
        pthread_mutex_unlock(&trincos_disciplinas[d]);
    }

    printf("[agente_suporte] Tentando abrir pipe de resposta: %s\n", pipe_resposta);
    int fd_resp = open(pipe_resposta, O_WRONLY);
    if (fd_resp != -1)
    {
        write(fd_resp, resposta, strlen(resposta) + 1);
        close(fd_resp);
    }
    else
    {
        perror("[agente_suporte] Erro ao abrir pipe de resposta para enviar horários");
    }
}

void gravar_em_arquivo(const char *nome_arquivo, const char *pipe_resposta)
{
    FILE *arquivo = fopen(nome_arquivo, "w");
    if (!arquivo)
    {
        printf("[agente_suporte] Tentando abrir pipe de resposta: %s\n", pipe_resposta);
        int fd_resp = open(pipe_resposta, O_WRONLY);
        if (fd_resp != -1)
        {
            int erro = -1;
            write(fd_resp, &erro, sizeof(erro));
            close(fd_resp);
        }
        perror("[agente_suporte] Erro ao abrir arquivo para gravação");
        return;
    }

    fprintf(arquivo, "#aluno,d0,d1,d2,d3,d4,d5,d6,d7,d8,d9\n");

    for (int aluno = 0; aluno < MAX_STUDENTS; aluno++)
    {
        fprintf(arquivo, "%d", aluno);

        for (int d = 0; d < MAX_DISCIPLINES; d++)
        {
            int horario = -1;
            pthread_mutex_lock(&trincos_disciplinas[d]);
            for (int h = 0; h < MAX_HORARIOS; h++)
            {
                for (int i = 0; i < disciplinas[d].horarios[h].num_inscritos; i++)
                {
                    if (disciplinas[d].horarios[h].alunos_inscritos[i] == aluno)
                    {
                        horario = h;
                        break;
                    }
                }
                if (horario != -1)
                    break;
            }
            pthread_mutex_unlock(&trincos_disciplinas[d]);

            fprintf(arquivo, ",%s", horario >= 0 ? "h" : "");
        }
        fprintf(arquivo, "\n");
    }

    fclose(arquivo);

    int fd_resp = open(pipe_resposta, O_WRONLY);
    if (fd_resp != -1)
    {
        int sucesso = 1;
        write(fd_resp, &sucesso, sizeof(sucesso));
        close(fd_resp);
    }
}

// Função para processar pedidos de alunos
void *processar_pedido_aluno(void *arg)
{
    char *mensagem = (char *)arg;
    int id_aluno, disciplina;
    char pipe_aluno[256];

    sscanf(mensagem, "%d %d %s", &id_aluno, &disciplina, pipe_aluno);

    if (inscricoes_alunos[id_aluno] >= INSCRICOES_POR_ALUNO)
    {
        free(mensagem);
        return NULL; // Aluno já completou suas inscrições
    }

    int horario = inscrever_aluno(id_aluno, disciplina);

    if (horario >= 0)
    {
        inscricoes_alunos[id_aluno]++;
        printf("[agente_suporte] Aluno %d inscrito na disciplina %d, horário %d (%d/%d)\n",
               id_aluno, disciplina, horario, inscricoes_alunos[id_aluno], INSCRICOES_POR_ALUNO);
    }

    // Responder ao aluno
    int fd_aluno = open(pipe_aluno, O_WRONLY);
    if (fd_aluno != -1)
    {
        write(fd_aluno, &horario, sizeof(horario));
        close(fd_aluno);
    }

    free(mensagem);
    return NULL;
}

void *processar_pedidos_admin(void *arg)
{
    printf("[agente_suporte] Thread admin iniciada.\n");

    int fd_admin = open(PIPE_ADMIN, O_RDONLY);
    if (fd_admin == -1)
    {
        perror("[agente_suporte] Erro ao abrir pipe admin");
        return NULL;
    }

    char buffer[BUFFER_SIZE];
    while (continuar_execucao)
    {
        memset(buffer, 0, sizeof(buffer));
        int len = le_pipe(fd_admin, buffer, sizeof(buffer));

        if (len > 0)
        {
            printf("[agente_suporte] Mensagem recebida do admin: %s\n", buffer);

            // Criar cópia do buffer para não modificar a string original
            char buffer_copy[BUFFER_SIZE];
            strncpy(buffer_copy, buffer, BUFFER_SIZE);

            // Separar comando primeiro
            char *comando_str = strtok(buffer_copy, ",");
            if (!comando_str)
            {
                printf("[agente_suporte] Formato de mensagem inválido\n");
                continue;
            }

            int codigo_op = atoi(comando_str);

            // Pegar próximo token (pode ser parâmetro ou pipe_resposta)
            char *next_token = strtok(NULL, ",");
            if (!next_token)
            {
                printf("[agente_suporte] Faltam parâmetros na mensagem\n");
                continue;
            }

            switch (codigo_op)
            {
            case 1:
            case 2:
            {
                // Para casos 1 e 2, precisamos do terceiro token (pipe_resposta)
                char *pipe_resposta = strtok(NULL, ",");
                if (!pipe_resposta)
                {
                    printf("[agente_suporte] Pipe de resposta não fornecido\n");
                    continue;
                }

                if (codigo_op == 1)
                {
                    int num_aluno = atoi(next_token);
                    printf("[agente_suporte] Consultando horários do aluno %d, pipe resposta: %s\n",
                           num_aluno, pipe_resposta);
                    consultar_horarios(num_aluno, pipe_resposta);
                }
                else // codigo_op == 2
                {
                    printf("[agente_suporte] Gravando em arquivo %s, pipe resposta: %s\n",
                           next_token, pipe_resposta);
                    gravar_em_arquivo(next_token, pipe_resposta);
                }
            }
            break;

            case 3:
            {
                // Para case 3, next_token já é o pipe_resposta
                char *pipe_resposta = next_token;
                printf("[agente_suporte] Recebido comando de término, pipe resposta: %s\n",
                       pipe_resposta);

                pthread_mutex_lock(&trinco_geral);
                continuar_execucao = 0;
                pthread_mutex_unlock(&trinco_geral);

                int fd_resp = open(pipe_resposta, O_WRONLY);
                if (fd_resp != -1)
                {
                    write(fd_resp, "Ok", 3);
                    close(fd_resp);
                }
                else
                {
                    perror("[agente_suporte] Erro ao abrir pipe de resposta para escrever 'Ok'");
                }
            }
            break;

            default:
                printf("[agente_suporte] Comando inválido do admin.\n");
            }
        }
    }

    close(fd_admin);
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Uso: %s <num_alunos>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int num_alunos = atoi(argv[1]);
    printf("[agente_suporte] Iniciado com %d alunos.\n", num_alunos);

    // Inicializar disciplinas e mutexes
    for (int d = 0; d < MAX_DISCIPLINES; d++)
    {
        pthread_mutex_init(&trincos_disciplinas[d], NULL);
        for (int h = 0; h < MAX_HORARIOS; h++)
        {
            disciplinas[d].horarios[h].num_inscritos = 0;
        }
    }

    // Abrir pipe principal
    int fd_support = open(PIPE_SUPPORT, O_RDONLY);
    if (fd_support == -1)
    {
        perror("[agente_suporte] Erro ao abrir pipe principal");
        exit(EXIT_FAILURE);
    }

    // Abrir o pipe admin em modo leitura (O_RDONLY)
    int fd_admin = open(PIPE_ADMIN, O_RDONLY | O_NONBLOCK);
    if (fd_admin == -1)
    {
        perror("[agente_suporte] Erro ao abrir pipe admin");
        close(fd_support);
        exit(EXIT_FAILURE);
    }

    // Criar thread para processar pedidos do admin
    pthread_t thread_admin;
    pthread_create(&thread_admin, NULL, processar_pedidos_admin, NULL);

    // Loop principal para processar pedidos de alunos
    char mensagem[BUFFER_SIZE];
    pthread_t threads[MAX_STUDENTS];
    int thread_count = 0;

    while (continuar_execucao)
    {
        memset(mensagem, 0, BUFFER_SIZE);

        int len = le_pipe(fd_support, mensagem, BUFFER_SIZE);
        if (len > 0)
        {
            // Criar uma thread para processar o pedido do aluno
            pthread_create(&threads[thread_count++], NULL, processar_pedido_aluno, strdup(mensagem));

            // Garantir que o número de threads não exceda o limite
            if (thread_count >= MAX_STUDENTS)
            {
                for (int i = 0; i < thread_count; i++)
                {
                    pthread_join(threads[i], NULL);
                }
                thread_count = 0; // Reiniciar o contador de threads
            }
        }
    }

    // Fechar o pipe admin após encerrar
    close(fd_admin);

    // Aguardar a thread do admin finalizar
    pthread_join(thread_admin, NULL);

    // Aguardar todas as threads de alunos finalizarem
    for (int i = 0; i < thread_count; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // Limpar mutexes
    for (int d = 0; d < MAX_DISCIPLINES; d++)
    {
        pthread_mutex_destroy(&trincos_disciplinas[d]);
    }

    close(fd_support); // Fechar o pipe principal
    printf("[agente_suporte] Finalizado.\n");
    return 0;
}