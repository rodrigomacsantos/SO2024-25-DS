#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// Definições de constantes
#define PIPE_ADMIN "/tmp/admin"     // Caminho do pipe administrativo
#define BUFFER_SIZE 1024            // Tamanho do buffer para mensagens

// Função para limpar buffer de entrada
// Remove caracteres residuais após leitura de input
void limpar_buffer()
{
    int c;
    // Consome caracteres até encontrar nova linha ou fim de arquivo
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

int main()
{
    // Preparar caminho para pipe de resposta
    char pipe_resposta[256] = "/tmp/admin_resp";

    // Remover pipe de resposta existente
    unlink(pipe_resposta);

    // Criar novo pipe de resposta
    if (mkfifo(pipe_resposta, 0666) == -1)
    {
        perror("[admin] Erro ao criar pipe de resposta");
        exit(1);
    }

    // Abrir pipe admin para escrita
    int fd_admin = open(PIPE_ADMIN, O_WRONLY);
    if (fd_admin == -1)
    {
        perror("[admin] Erro ao abrir pipe principal");
        unlink(pipe_resposta);
        exit(1);
    }
    printf("[admin] Pipe admin aberto com sucesso para escrita.\n");

    // Loop principal do menu administrativo
    while (1)
    {
        // Exibir opções do menu
        printf("\nMenu Admin:\n");
        printf("1. Consultar horários de um aluno\n");
        printf("2. Gravar em arquivo\n");
        printf("3. Terminar o agente\n");
        printf("0. Sair (somente admin)\n");
        printf("Escolha: ");

        // Ler opção do usuário
        int opcao;
        if (scanf("%d", &opcao) != 1)
        {
            printf("Opção inválida!\n");
            limpar_buffer();
            continue;
        }

        // Limpar buffer após leitura
        limpar_buffer();

        // Buffer para preparar mensagens
        char mensagem[BUFFER_SIZE];

        // Processar opção selecionada
        switch (opcao)
        {
        case 1: // Consultar horários de aluno
        {
            // Solicitar número do aluno
            printf("Número do aluno: ");
            int num_aluno;
            if (scanf("%d", &num_aluno) != 1)
            {
                printf("Número de aluno inválido!\n");
                limpar_buffer();
                break;
            }
            limpar_buffer();

            // Preparar mensagem para o agente de suporte
            snprintf(mensagem, BUFFER_SIZE, "1,%d,%s", num_aluno, pipe_resposta);
            write(fd_admin, mensagem, strlen(mensagem) + 1);

            // Abrir pipe de resposta e ler resultado
            int fd_resp = open(pipe_resposta, O_RDONLY);
            char resposta[BUFFER_SIZE] = {0};
            read(fd_resp, resposta, BUFFER_SIZE - 1);
            close(fd_resp);

            // Imprimir horários do aluno
            printf("Resposta: %s\n", resposta);
            break;
        }
        case 2: // Gravar em arquivo
        {
            // Solicitar nome do arquivo
            printf("Nome do arquivo para gravação: ");
            char nome_arquivo[256];
            if (scanf("%255s", nome_arquivo) != 1)
            {
                printf("Nome de arquivo inválido!\n");
                limpar_buffer();
                break;
            }
            limpar_buffer();

            // Preparar mensagem para o agente de suporte
            snprintf(mensagem, BUFFER_SIZE, "2,%s,%s", nome_arquivo, pipe_resposta);
            write(fd_admin, mensagem, strlen(mensagem) + 1);

            // Abrir pipe de resposta e verificar resultado
            int fd_resp = open(pipe_resposta, O_RDONLY);
            int resultado;
            read(fd_resp, &resultado, sizeof(resultado));
            close(fd_resp);

            // Apresentar resultado da operação
            if (resultado == -1)
            {
                printf("[admin] Erro ao gravar no arquivo %s\n", nome_arquivo);
            }
            else
            {
                printf("[admin] Arquivo %s gravado com sucesso.\n", nome_arquivo);
            }
            break;
        }
        case 3: // Terminar agente de suporte
        {
            printf("[admin] Enviando comando para terminar o agente...\n");

            // Preparar mensagem de término
            snprintf(mensagem, BUFFER_SIZE, "3,%s", pipe_resposta);
            write(fd_admin, mensagem, strlen(mensagem) + 1);

            // Abrir pipe de resposta e verificar confirmação
            int fd_resp = open(pipe_resposta, O_RDONLY);
            if (fd_resp == -1)
            {
                perror("[admin] Erro ao abrir pipe de resposta");
                break;
            }

            // Ler resposta
            char resposta[BUFFER_SIZE] = {0};
            read(fd_resp, resposta, BUFFER_SIZE - 1);
            close(fd_resp);

            // Verificar se recebeu confirmação
            if (strcmp(resposta, "Ok") == 0)
            {
                printf("[admin] Support_agent finalizado com sucesso. Admin continua ativo.\n");
            }
            else
            {
                printf("[admin] Erro ao finalizar o support_agent.\n");
            }
            break;
        }

        case 0: // Sair do programa admin
            printf("[admin] Encerrando admin...\n");
            unlink(pipe_resposta); // Remove o pipe de resposta
            close(fd_admin);        // Fecha o pipe admin
            exit(0);

        default:
            printf("Opção inválida! Escolha novamente.\n");
            break;
        }
    }

    // Remover pipe de resposta (nunca alcançado no código atual)
    unlink(pipe_resposta);
    return 0;
}