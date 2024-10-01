#!/bin/bash

# Função para exibir o uso correto do script
usage() {
    echo "Uso: $0 <nome_do_pipe>"
    exit 1
}

# Verificação dos argumentos de entrada
if [ "$#" -ne 1 ]; then
    echo "Erro: Número incorreto de argumentos."
    usage
fi

pipe_name=$1

echo "Iniciando suporte_agente.sh"

# Verificar se o named pipe existe
max_attempts=30
attempts=0
while [ ! -p "$pipe_name" ] && [ $attempts -lt $max_attempts ]; do
    echo "Aguardando a criação do named pipe $pipe_name..."
    sleep 1
    ((attempts++))
done

if [ ! -p "$pipe_name" ]; then
    echo "O named pipe $pipe_name não foi criado após $max_attempts segundos."
    exit 1
fi

echo "Named pipe $pipe_name encontrado."

while true; do
    # Ler do named pipe
    if read -r pedido < "$pipe_name"; then
        if [ "$pedido" = "quit" ]; then
            echo "Encerrando o suporte_agente."
            break
        fi

        echo "Tratando pedido: $pedido"

        # Espera aleatória entre 1 e 5 segundos
        sleep_time=$((RANDOM % 5 + 1))
        echo "Tempo de processamento: $sleep_time segundos"
        sleep $sleep_time
    fi

    
done

echo "suporte_agente.sh concluído"