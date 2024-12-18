#!/bin/bash

# Definir caminhos para pipes
PIPE_SUPPORT="/tmp/suporte"
PIPE_ADMIN="/tmp/admin"

# Validar número de argumentos
if [ $# -ne 4 ]; then
    echo "Uso: $0 <NALUN> <NDISCIP> <NLUG> <NSTUD>"
    exit 1
fi

# Capturar parâmetros
NALUN=$1     # Número de alunos
NDISCIP=$2   # Número de disciplinas
NLUG=$3      # Número de lugares/vagas
NSTUD=$4     # Número de processos de estudantes

# Remover pipes antigos
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

# Criar pipe principal
echo "Criando pipe principal ($PIPE_SUPPORT)..."
mkfifo "$PIPE_SUPPORT" || { echo "Erro ao criar $PIPE_SUPPORT"; exit 1; }
chmod 0666 "$PIPE_SUPPORT"

# Criar pipe admin
echo "Criando pipe admin ($PIPE_ADMIN)..."
if [ ! -p "$PIPE_ADMIN" ]; then
    mkfifo "$PIPE_ADMIN" || { echo "Erro ao criar $PIPE_ADMIN"; exit 1; }
fi
chmod 0666 "$PIPE_ADMIN"

# Iniciar agente de suporte
echo "Iniciando o agente de suporte..."
./support_agent $NALUN $NLUG & 
sleep 1

# Calcular distribuição de alunos por processo
total_processes=$NSTUD
students_per_process=$((NALUN / total_processes))
remainder=$((NALUN % total_processes))

# Lançar processos de estudantes sequencialmente
for process_id in $(seq 0 $((total_processes-1))); do
    start_student=$((process_id * students_per_process + (process_id < remainder ? process_id : remainder)))
    
    # Ajustar o número de alunos para este processo
    if [ $process_id -lt $remainder ]; then
        num_students=$((students_per_process + 1))
    else
        num_students=$students_per_process
    fi

    if [ $num_students -gt 0 ]; then
        echo "Iniciando processo de alunos $process_id: inicial=$start_student, total=$num_students"
        ./student $process_id $start_student $num_students
        sleep 0.1
    fi
done

# Aguardar conclusão de todos os processos
wait

# Limpeza final
echo "Limpando pipes..."
rm -f "$PIPE_SUPPORT" "$PIPE_ADMIN"
for i in $(seq 0 $((NALUN-1))); do
    rm -f "/tmp/student_$i"
done

echo "Execução finalizada."
