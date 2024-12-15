#!/bin/bash

PIPE_SUPPORT="/tmp/suporte"
PIPE_ADMIN="/tmp/admin"
LOCKFILE="/tmp/suporte_desk.lock"

trap 'rm -f $PIPE_SUPPORT /tmp/student_* $PIPE_ADMIN $LOCKFILE; pkill -f support_agent; exit' INT TERM EXIT

if [ $# -ne 4 ]; then
    echo "Uso: $0 <NALUN> <NDISCIP> <NLUG> <NSTUD>"
    exit 1
fi

NALUN=$1
NDISCIP=$2
NLUG=$3
NSTUD=$4

echo "Removendo pipes antigos..."
if [ -e "$PIPE_SUPPORT" ]; then
    rm -f "$PIPE_SUPPORT"
    echo "Pipe principal existente removido: $PIPE_SUPPORT"
fi

if [ -e "$PIPE_ADMIN" ]; then
    rm -f "$PIPE_ADMIN"
    echo "Pipe admin existente removido: $PIPE_ADMIN"
fi

for i in $(seq 0 $((NSTUD-1))); do
    if [ -e "/tmp/student_$i" ]; then
        rm -f "/tmp/student_$i"
        echo "Pipe do estudante removido: /tmp/student_$i"
    fi
done

echo "Criando pipe principal ($PIPE_SUPPORT)..."
mkfifo "$PIPE_SUPPORT" || { echo "Erro ao criar $PIPE_SUPPORT"; exit 1; }
chmod 0666 "$PIPE_SUPPORT"

echo "Criando pipe admin ($PIPE_ADMIN)..."
if [ ! -p "$PIPE_ADMIN" ]; then
    mkfifo "$PIPE_ADMIN" || { echo "Erro ao criar $PIPE_ADMIN"; exit 1; }
fi
chmod 0666 "$PIPE_ADMIN"


echo "Iniciando o agente de suporte..."
./support_agent $NALUN &
sleep 1

# Determinar número de alunos por processo
total_processes=5
students_per_process=$((NSTUD / total_processes))
if [ $((NSTUD % total_processes)) -ne 0 ]; then
    students_per_process=$((students_per_process + 1))
fi

# Lançar processos para cada grupo de alunos
for process_id in $(seq 0 $((total_processes-1))); do
    start_student=$((process_id * students_per_process))
    if [ $start_student -lt $NSTUD ]; then
        echo "Iniciando processo de alunos $process_id: inicial=$start_student, total=$students_per_process"
        ./student $process_id $start_student $students_per_process &
        sleep 0.1
    fi
done

# Aguardar todos os processos terminarem
wait

echo "Limpando pipes..."
rm -f "$PIPE_SUPPORT" "$PIPE_ADMIN"
for i in $(seq 0 $((NSTUD-1))); do
    rm -f "/tmp/student_$i"
done

rm -f "$LOCKFILE"
echo "Execução finalizada."
