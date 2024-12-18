#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

// Definições de constantes para configuração do sistema
#define PIPE_SUPPORT "/tmp/suporte"     // Caminho do pipe principal de comunicação
#define PIPE_ADMIN "/tmp/admin"         // Caminho do pipe de comunicação administrativa
#define MAX_DISCIPLINES 10               // Número máximo de disciplinas
#define MAX_HORARIOS 5                  // Número máximo de horários por disciplina
#define MAX_STUDENTS 256                 // Número máximo de alunos
#define INSCRICOES_POR_ALUNO 5           // Limite de inscrições por aluno
#define BUFFER_SIZE 1024                 // Tamanho do buffer para leitura de mensagens

// Estrutura para representar um horário de disciplina
typedef struct
{
    int alunos_inscritos[MAX_STUDENTS];  // Array para armazenar IDs dos alunos inscritos
    int num_inscritos;                   // Número atual de alunos inscritos
    int limite_vagas;                    // Máximo de vagas permitidas no horário
} Horario;

// Estrutura para representar uma disciplina
typedef struct
{
    Horario horarios[MAX_HORARIOS];      // Array de horários disponíveis
} Disciplina;

// Variáveis globais para gerenciamento do sistema
Disciplina disciplinas[MAX_DISCIPLINES];                 // Array de todas as disciplinas
pthread_mutex_t trincos_disciplinas[MAX_DISCIPLINES];    // Mutex para sincronização de acesso às disciplinas
pthread_mutex_t trinco_geral = PTHREAD_MUTEX_INITIALIZER; // Mutex global para operações críticas

// Variáveis de controle de execução
int continuar_execucao = 1;                              // Flag para manter o processamento ativo
int inscricoes_alunos[MAX_STUDENTS] = {0};               // Contador de inscrições por aluno

// Protótipos de funções
int le_pipe(int fd, char *msg_in, int bsize);             // Leitura de pipe
int inscrever_aluno(int id_aluno, int disciplina, int horario_desejado);  // Inscrição de aluno
void consultar_horarios(int num_aluno, const char *pipe_resposta);  // Consulta de horários
void gravar_em_arquivo(const char *nome_arquivo, const char *pipe_resposta);  // Gravação em arquivo

// Função para leitura segura de pipe
// Lê bytes individuais para garantir leitura completa da mensagem
int le_pipe(int fd, char *msg_in, int bsize)
{
    // Validações de segurança dos parâmetros
    if (fd < 0 || msg_in == NULL || bsize <= 0)
        return -1;

    int len = 0;
    char byte;

    // Leitura byte a byte até encontrar terminador ou atingir limite
    while (len < bsize - 1)
    {
        int result = read(fd, &byte, 1);
        if (result == 1)
        {
            // Encontrou terminador de string
            if (byte == '\0')
            {
                msg_in[len] = '\0';
                return len;
            }
            msg_in[len++] = byte;
        }
        else if (result == 0)
        {
            break;  // Fim do pipe
        }
        else
        {
            return -1;  // Erro de leitura
        }
    }

    msg_in[len] = '\0';
    return len;
}

// Função para inscrever aluno em disciplina
// Suporta horário preferido e alternativo
int inscrever_aluno(int id_aluno, int disciplina, int horario_desejado)
{
    // Bloquear acesso concorrente à disciplina
    pthread_mutex_lock(&trincos_disciplinas[disciplina]);

    // Verificar se aluno já está inscrito na disciplina
    for (int h = 0; h < MAX_HORARIOS; h++)
    {
        for (int i = 0; i < disciplinas[disciplina].horarios[h].num_inscritos; i++)
        {
            if (disciplinas[disciplina].horarios[h].alunos_inscritos[i] == id_aluno)
            {
                pthread_mutex_unlock(&trincos_disciplinas[disciplina]);
                return h; // Retorna horário já inscrito
            }
        }
    }

    // Tentar primeiro o horário desejado
    if (horario_desejado >= 0 && horario_desejado < MAX_HORARIOS)
    {
        // Verificar limite de vagas no horário
        if (disciplinas[disciplina].horarios[horario_desejado].num_inscritos < 
            disciplinas[disciplina].horarios[horario_desejado].limite_vagas)
        {
            // Adicionar aluno no horário desejado
            int idx = disciplinas[disciplina].horarios[horario_desejado].num_inscritos;
            disciplinas[disciplina].horarios[horario_desejado].alunos_inscritos[idx] = id_aluno;
            disciplinas[disciplina].horarios[horario_desejado].num_inscritos++;
            pthread_mutex_unlock(&trincos_disciplinas[disciplina]);
            return horario_desejado;
        }
    }

    // Buscar outro horário disponível
    for (int h = 0; h < MAX_HORARIOS; h++)
    {
        // Verificar se há vagas no horário
        if (disciplinas[disciplina].horarios[h].num_inscritos < 
            disciplinas[disciplina].horarios[h].limite_vagas)
        {
            // Adicionar aluno no primeiro horário disponível
            int idx = disciplinas[disciplina].horarios[h].num_inscritos;
            disciplinas[disciplina].horarios[h].alunos_inscritos[idx] = id_aluno;
            disciplinas[disciplina].horarios[h].num_inscritos++;
            pthread_mutex_unlock(&trincos_disciplinas[disciplina]);
            return h;
        }
    }

    // Nenhum horário disponível
    pthread_mutex_unlock(&trincos_disciplinas[disciplina]);
    return -1;
}

// Função para consultar horários de um aluno
void consultar_horarios(int num_aluno, const char *pipe_resposta)
{
    // Preparar buffer de resposta
    char resposta[BUFFER_SIZE];
    snprintf(resposta, sizeof(resposta), "Aluno %d: ", num_aluno);

    // Percorrer todas as disciplinas
    for (int d = 0; d < MAX_DISCIPLINES; d++)
    {
        // Bloquear acesso à disciplina para leitura thread-safe
        pthread_mutex_lock(&trincos_disciplinas[d]);
        
        // Verificar todos os horários
        for (int h = 0; h < MAX_HORARIOS; h++)
        {
            for (int i = 0; i < disciplinas[d].horarios[h].num_inscritos; i++)
            {
                // Adicionar à resposta se aluno encontrado
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

    // Enviar resposta via pipe
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

// Função para gravar informações de inscrições em arquivo
void gravar_em_arquivo(const char *nome_arquivo, const char *pipe_resposta)
{
    // Abrir arquivo para gravação
    FILE *arquivo = fopen(nome_arquivo, "w");
    if (!arquivo)
    {
        // Tratar erro de abertura de arquivo
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

    // Escrever cabeçalho do CSV
    fprintf(arquivo, "#aluno,d0,d1,d2,d3,d4,d5,d6,d7,d8,d9\n");

    // Percorrer todos os alunos
    for (int aluno = 0; aluno < MAX_STUDENTS; aluno++)
    {
        fprintf(arquivo, "%d", aluno);

        // Verificar inscrições em cada disciplina
        for (int d = 0; d < MAX_DISCIPLINES; d++)
        {
            int horario = -1;
            pthread_mutex_lock(&trincos_disciplinas[d]);
            
            // Encontrar horário do aluno
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

            // Marcar presença no horário
            fprintf(arquivo, ",%s", horario >= 0 ? "h" : "");
        }
        fprintf(arquivo, "\n");
    }

    fclose(arquivo);

    // Confirmar gravação
    int fd_resp = open(pipe_resposta, O_WRONLY);
    if (fd_resp != -1)
    {
        int sucesso = 1;
        write(fd_resp, &sucesso, sizeof(sucesso));
        close(fd_resp);
    }
}

// Função para processar pedido de inscrição de aluno
void *processar_pedido_aluno(void *arg)
{
    char *mensagem = (char *)arg;
    int id_aluno, disciplina, horario_desejado = -1;
    char pipe_aluno[256];

    // Parse da mensagem com horário desejado
    if (sscanf(mensagem, "%d %d %d %s", &id_aluno, &disciplina, &horario_desejado, pipe_aluno) != 4)
    {
        // Fallback para formato antigo sem horário
        if (sscanf(mensagem, "%d %d %s", &id_aluno, &disciplina, pipe_aluno) != 3)
        {
            free(mensagem);
            return NULL;
        }
        horario_desejado = -1;
    }

    // Verificar limite de inscrições do aluno
    if (inscricoes_alunos[id_aluno] >= INSCRICOES_POR_ALUNO)
    {
        free(mensagem);
        return NULL;
    }

    // Tentar inscrever aluno
    int horario = inscrever_aluno(id_aluno, disciplina, horario_desejado);

    // Processar resultado da inscrição
    if (horario >= 0)
    {
        inscricoes_alunos[id_aluno]++;
        printf("[agente_suporte] Aluno %d inscrito na disciplina %d, horário %d (%d/%d)\n",
               id_aluno, disciplina, horario, inscricoes_alunos[id_aluno], INSCRICOES_POR_ALUNO);
    }

    // Responder ao aluno via pipe
    int fd_aluno = open(pipe_aluno, O_WRONLY);
    if (fd_aluno != -1)
    {
        write(fd_aluno, &horario, sizeof(horario));
        close(fd_aluno);
    }

    free(mensagem);
    return NULL;
}
// Função para processar pedidos administrativos
void *processar_pedidos_admin(void *arg)
{
    // Iniciar thread administrativa
    printf("[agente_suporte] Thread admin iniciada.\n");

    // Abrir pipe administrativo para leitura
    int fd_admin = open(PIPE_ADMIN, O_RDONLY);
    if (fd_admin == -1)
    {
        perror("[agente_suporte] Erro ao abrir pipe admin");
        return NULL;
    }

    // Buffer para armazenar mensagens recebidas
    char buffer[BUFFER_SIZE];

    // Loop de processamento de pedidos administrativos
    while (continuar_execucao)
    {
        // Limpar buffer
        memset(buffer, 0, sizeof(buffer));

        // Ler mensagem do pipe
        int len = le_pipe(fd_admin, buffer, sizeof(buffer));

        // Processar mensagem se houver conteúdo
        if (len > 0)
        {
            // Log da mensagem recebida
            printf("[agente_suporte] Mensagem recebida do admin: %s\n", buffer);

            // Criar cópia do buffer para não modificar original
            char buffer_copy[BUFFER_SIZE];
            strncpy(buffer_copy, buffer, BUFFER_SIZE);

            // Extrair código de operação
            char *comando_str = strtok(buffer_copy, ",");
            if (!comando_str)
            {
                printf("[agente_suporte] Formato de mensagem inválido\n");
                continue;
            }

            // Converter código de operação
            int codigo_op = atoi(comando_str);

            // Extrair próximo token
            char *next_token = strtok(NULL, ",");
            if (!next_token)
            {
                printf("[agente_suporte] Faltam parâmetros na mensagem\n");
                continue;
            }

            // Processar diferentes tipos de comandos
            switch (codigo_op)
            {
            case 1: // Consulta de horários
            case 2: // Gravação em arquivo
            {
                // Requer um terceiro token (pipe de resposta)
                char *pipe_resposta = strtok(NULL, ",");
                if (!pipe_resposta)
                {
                    printf("[agente_suporte] Pipe de resposta não fornecido\n");
                    continue;
                }

                if (codigo_op == 1)
                {
                    // Consultar horários de um aluno
                    int num_aluno = atoi(next_token);
                    printf("[agente_suporte] Consultando horários do aluno %d, pipe resposta: %s\n",
                           num_aluno, pipe_resposta);
                    consultar_horarios(num_aluno, pipe_resposta);
                }
                else // codigo_op == 2
                {
                    // Gravar informações em arquivo
                    printf("[agente_suporte] Gravando em arquivo %s, pipe resposta: %s\n",
                           next_token, pipe_resposta);
                    gravar_em_arquivo(next_token, pipe_resposta);
                }
            }
            break;

            case 3: // Comando de término
            {
                // next_token é o pipe de resposta
                char *pipe_resposta = next_token;
                printf("[agente_suporte] Recebido comando de término, pipe resposta: %s\n",
                       pipe_resposta);

                // Sinalizar parada de execução com mutex
                pthread_mutex_lock(&trinco_geral);
                continuar_execucao = 0;
                pthread_mutex_unlock(&trinco_geral);

                // Responder ao admin
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

    // Fechar pipe administrativo
    close(fd_admin);
    return NULL;
}

// Função principal do agente de suporte
int main(int argc, char *argv[])
{
    int limite_vagas = MAX_STUDENTS;  // Valor padrão de vagas

    // Validar número de argumentos
    if (argc < 2 || argc > 3)
    {
        fprintf(stderr, "Uso: %s <num_alunos> [limite_vagas_por_horario]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Converter número de alunos
    int num_alunos = atoi(argv[1]);
    
    // Definir limite de vagas se fornecido
    if (argc == 3)
    {
        limite_vagas = atoi(argv[2]);
        if (limite_vagas <= 0)
        {
            fprintf(stderr, "Limite de vagas deve ser positivo\n");
            exit(EXIT_FAILURE);
        }
    }

    // Informar configuração inicial
    printf("[agente_suporte] Iniciado com %d alunos, limite %d vagas por horário.\n", 
           num_alunos, limite_vagas);

    // Inicializar disciplinas e mutexes
    for (int d = 0; d < MAX_DISCIPLINES; d++)
    {
        // Inicializar mutex para cada disciplina
        pthread_mutex_init(&trincos_disciplinas[d], NULL);
        
        // Configurar horários
        for (int h = 0; h < MAX_HORARIOS; h++)
        {
            disciplinas[d].horarios[h].num_inscritos = 0;
            disciplinas[d].horarios[h].limite_vagas = limite_vagas;
        }
    }

    // Abrir pipe principal de suporte
    int fd_support = open(PIPE_SUPPORT, O_RDONLY);
    if (fd_support == -1)
    {
        perror("[agente_suporte] Erro ao abrir pipe principal");
        exit(EXIT_FAILURE);
    }

    // Abrir pipe admin em modo não bloqueante
    int fd_admin = open(PIPE_ADMIN, O_RDONLY | O_NONBLOCK);
    if (fd_admin == -1)
    {
        perror("[agente_suporte] Erro ao abrir pipe admin");
        close(fd_support);
        exit(EXIT_FAILURE);
    }

    // Criar thread para processar pedidos administrativos
    pthread_t thread_admin;
    pthread_create(&thread_admin, NULL, processar_pedidos_admin, NULL);

    // Preparar para processar pedidos de alunos
    char mensagem[BUFFER_SIZE];
    pthread_t threads[MAX_STUDENTS];
    int thread_count = 0;

    // Loop principal de processamento
    while (continuar_execucao)
    {
        // Limpar buffer de mensagem
        memset(mensagem, 0, BUFFER_SIZE);

        // Ler mensagens do pipe de suporte
        int len = le_pipe(fd_support, mensagem, BUFFER_SIZE);
        if (len > 0)
        {
            // Criar thread para processar pedido do aluno
            pthread_create(&threads[thread_count++], NULL, processar_pedido_aluno, strdup(mensagem));

            // Controlar número de threads
            if (thread_count >= MAX_STUDENTS)
            {
                // Aguardar conclusão de todas as threads
                for (int i = 0; i < thread_count; i++)
                {
                    pthread_join(threads[i], NULL);
                }
                thread_count = 0;
            }
        }
    }

    // Fechar pipe admin
    close(fd_admin);

    // Aguardar thread administrativa finalizar
    pthread_join(thread_admin, NULL);

    // Aguardar todas as threads de alunos finalizarem
    for (int i = 0; i < thread_count; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // Destruir mutexes
    for (int d = 0; d < MAX_DISCIPLINES; d++)
    {
        pthread_mutex_destroy(&trincos_disciplinas[d]);
    }

    // Fechar pipe principal
    close(fd_support);
    printf("[agente_suporte] Finalizado.\n");
    return 0;
}