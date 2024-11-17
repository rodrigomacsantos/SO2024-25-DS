#!/bin/bash

PIPE_SUPPORT="/tmp/suporte"
LOCKFILE="/tmp/suporte_desk.lock"

trap 'rm -f $PIPE_SUPPORT /tmp/student_* $LOCKFILE; pkill -f support_agent; exit' INT TERM EXIT

if [ $# -ne 4 ]; then
    echo "Usage: $0 <NALUN> <NDISCIP> <NLUG> <NSTUD>"
    exit 1
fi

# Mecanismo de bloqueio
if [ -f $LOCKFILE ]; then
    echo "Another instance of the script is running. Exiting."
    exit 1
fi
touch $LOCKFILE

NALUN=$1
NDISCIP=$2
NLUG=$3
NSTUD=$4

# Remover pipes existentes
echo "Removendo pipes antigos..."
rm -f /tmp/suporte /tmp/student_*
if [ -p $PIPE_SUPPORT ]; then
    echo "Removendo pipe principal existente: $PIPE_SUPPORT"
    rm -f $PIPE_SUPPORT
fi

for i in $(seq 1 $NSTUD); do
    STUDENT_PIPE="/tmp/student_$i"
    if [ -p $STUDENT_PIPE ]; then
        echo "Removendo pipe do estudante existente: $STUDENT_PIPE"
        rm -f $STUDENT_PIPE
    fi
done

# Criar pipe do suporte
echo "Criando pipe principal ($PIPE_SUPPORT)..."
if [ ! -p $PIPE_SUPPORT ]; then
    mkfifo $PIPE_SUPPORT || { echo "Erro ao criar $PIPE_SUPPORT"; exit 1; }
    echo "Pipe principal criado: $PIPE_SUPPORT"
fi


# Iniciar o agente de suporte
echo "Iniciando o agente de suporte..."
./suporte_agente $NALUN &
if [ $? -ne 0 ]; then
    echo "Erro ao iniciar support_agent"
    exit 1
fi

# Iniciar os estudantes
students_per_process=$((NALUN / NSTUD))
for i in $(seq 1 $NSTUD); do
    STUDENT_PIPE="/tmp/student_$i"
    echo "Criando pipe do estudante ($STUDENT_PIPE)..."
    if [ ! -p $STUDENT_PIPE ]; then
        mkfifo $STUDENT_PIPE
    else
        echo "Pipe do estudante já existe: $STUDENT_PIPE"
    fi

    initial_student=$(( (i - 1) * students_per_process ))
    echo "Iniciando estudante $i: inicial=$initial_student, total=$students_per_process"
    
    # Passando os argumentos corretos ao ./student
    ./student $i $initial_student $students_per_process &
    echo "Estudante $i iniciado com sucesso."
done



# Aguardar finalização
wait

# Limpar pipes e remover lockfile
echo "Limpando pipes..."
if [ -p $PIPE_SUPPORT ]; then
    rm -f $PIPE_SUPPORT
    echo "Pipe principal removido: $PIPE_SUPPORT"
fi

for i in $(seq 1 $NSTUD); do
    STUDENT_PIPE="/tmp/student_$i"
    if [ -p $STUDENT_PIPE ]; then
        rm -f $STUDENT_PIPE
        echo "Pipe do estudante removido: $STUDENT_PIPE"
    fi
done

rm -f $LOCKFILE
echo "Execução finalizada."