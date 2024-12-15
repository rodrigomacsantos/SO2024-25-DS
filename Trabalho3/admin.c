#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PIPE_ADMIN "/tmp/admin"
#define BUFFER_SIZE 1024

void limpar_buffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

int main()
{
    char pipe_resposta[256] = "/tmp/admin_resp";

    // Criar pipe de resposta
    unlink(pipe_resposta);
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

    while (1)
    {
        printf("\nMenu Admin:\n");
        printf("1. Consultar horários de um aluno\n");
        printf("2. Gravar em arquivo\n");
        printf("3. Terminar o agente\n");
        printf("0. Sair (somente admin)\n");
        printf("Escolha: ");

        int opcao;
        if (scanf("%d", &opcao) != 1)
        {
            printf("Opção inválida!\n");
            limpar_buffer();
            continue;
        }

        limpar_buffer();

        char mensagem[BUFFER_SIZE];
        switch (opcao)
        {
        case 1:
        {
            printf("Número do aluno: ");
            int num_aluno;
            if (scanf("%d", &num_aluno) != 1)
            {
                printf("Número de aluno inválido!\n");
                limpar_buffer();
                break;
            }
            limpar_buffer();

            snprintf(mensagem, BUFFER_SIZE, "1,%d,%s", num_aluno, pipe_resposta);
            write(fd_admin, mensagem, strlen(mensagem) + 1);

            int fd_resp = open(pipe_resposta, O_RDONLY);
            char resposta[BUFFER_SIZE] = {0};
            read(fd_resp, resposta, BUFFER_SIZE - 1);
            close(fd_resp);

            printf("Resposta: %s\n", resposta);
            break;
        }
        case 2:
        {
            printf("Nome do arquivo para gravação: ");
            char nome_arquivo[256];
            if (scanf("%255s", nome_arquivo) != 1)
            {
                printf("Nome de arquivo inválido!\n");
                limpar_buffer();
                break;
            }
            limpar_buffer();

            snprintf(mensagem, BUFFER_SIZE, "2,%s,%s", nome_arquivo, pipe_resposta);
            write(fd_admin, mensagem, strlen(mensagem) + 1);

            int fd_resp = open(pipe_resposta, O_RDONLY);
            int resultado;
            read(fd_resp, &resultado, sizeof(resultado));
            close(fd_resp);

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
        case 3:
        {
            printf("[admin] Enviando comando para terminar o agente...\n");

            // Enviar comando para finalizar o agente
            snprintf(mensagem, BUFFER_SIZE, "3,%s", pipe_resposta);
            write(fd_admin, mensagem, strlen(mensagem) + 1);

            // Ler resposta do agente
            int fd_resp = open(pipe_resposta, O_RDONLY);
            if (fd_resp == -1)
            {
                perror("[admin] Erro ao abrir pipe de resposta");
                break;
            }

            char resposta[BUFFER_SIZE] = {0};
            read(fd_resp, resposta, BUFFER_SIZE - 1);
            close(fd_resp);

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

        case 0:
            printf("[admin] Encerrando admin...\n");
            unlink(pipe_resposta); // Remove o pipe de resposta
            close(fd_admin);       // Fecha o pipe admin
            exit(0);

        default:
            printf("Opção inválida! Escolha novamente.\n");
            break;
        }
    }

    unlink(pipe_resposta);
    return 0;
}
