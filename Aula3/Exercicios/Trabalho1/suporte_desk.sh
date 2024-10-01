#!/bin/bash

# Função para exibir o uso correto do script
usage() {
    echo "Uso: $0 <numero_de_students> <nome_do_pipe>"
    exit 1
}

# Verificação dos argumentos de entrada
if [ "$#" -ne 2 ]; then
    usage
fi

num_students=$1
pipe_name=$2

echo "Iniciando suporte_desk.sh"

# Verificar se o executável student existe
if [ ! -x "./student" ]; then
    echo "Erro: O executável './student' não foi encontrado ou não tem permissão de execução."
    echo "Por favor, compile o programa student.c e verifique as permissões."
    exit 1
fi

echo "Executável 'student' encontrado."

# Criar o named pipe apenas se ainda não existir
if [ ! -p "$pipe_name" ]; then
    echo "Criando named pipe $pipe_name"
    if mkfifo "$pipe_name"; then
        echo "Named pipe criado com sucesso."
    else
        echo "Erro ao criar named pipe. Verifique as permissões."
        exit 1
    fi
else
    echo "O named pipe $pipe_name já existe."
fi

# Executar o número especificado de students em background
echo "Executando $num_students instâncias de student em background"
for i in $(seq 1 $num_students)
do
    echo "Iniciando student $i"
    ./student "$pipe_name" "$i" &
    if [ $? -ne 0 ]; then
        echo "Erro ao executar student $i"
    else
        echo "Student $i iniciado com sucesso"
    fi
done

# Iniciar suporte_agente.sh em background
echo "Iniciando suporte_agente.sh em background"
./suporte_agente.sh "$pipe_name" &
if [ $? -ne 0 ]; then
    echo "Erro ao iniciar suporte_agente.sh"
else
    echo "suporte_agente.sh iniciado com sucesso"
fi

# Aguardar 1 segundo
echo "Aguardando 1 segundo"
sleep 1

# Enviar 'quit' para o named pipe
echo "Enviando 'quit' para o named pipe"
if echo "quit" > "$pipe_name"; then
    echo "'quit' enviado com sucesso"
else
    echo "Erro ao enviar 'quit' para o named pipe"
fi

# Aguardar todos os processos terminarem
echo "Aguardando todos os processos terminarem"
wait
echo "Todos os processos terminaram"

# Remover o named pipe
echo "Removendo o named pipe"
if rm "$pipe_name"; then
    echo "Named pipe removido com sucesso"
else
    echo "Erro ao remover o named pipe"
fi

echo "suporte_desk.sh concluído"