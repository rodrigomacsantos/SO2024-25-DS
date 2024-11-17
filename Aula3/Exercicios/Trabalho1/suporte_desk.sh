#!/bin/bash

#Remove pipes antigos, se existirem
rm -f /tmp/suporte
for ((i=1; i<=$4; i++)); do
    rm -f /tmp/student$i
done

#Verifica se os argumentos necessários foram fornecidos
if [ "$#" -ne 4 ]; then
    echo "Uso: $0 <NALUN> <NDISCIP> <NLUG> <NSTUD>" >&2
    exit 1
fi

Argumentos
NALUN=$1
NDISCIP=$2
NLUG=$3
NSTUD=$4

#Número de horários por disciplina (pode ajustar se necessário)
NHOR_PER_DISC=4
#Calcula o número máximo de horários
NHOR=$((NDISCIP * NHOR_PER_DISC))

#Cria o named pipe comum
FIFO_NAME="/tmp/suporte"
if [ ! -e "$FIFO_NAME" ]; then
    mkfifo "$FIFO_NAME"
fi

#Executa o suport_agent em background, passando NALUN como argumento
./suporte_agente "$NALUN" &
SUPORT_AGENT_PID=$!

#Executa os processos student em background
for ((i=1; i<=NSTUD; i++)); do
    # Calcula o número de aluno inicial e o número de alunos a inscrever para cada student
    ALUNO_INICIAL=$(( (NALUN / NSTUD) * (i - 1) ))
    NUM_ALUNOS=$(( NALUN / NSTUD ))

#Cria o named pipe específico para este student
    STUDENT_FIFO="/tmp/student$i"
    mkfifo "$STUDENTFIFO"

    # Executa o student em background
    ./student "$i" "$ALUNO_INICIAL" "$NUM_ALUNOS" &
    STUDENT_PIDS+=($!)
done

#Aguarda a finalização de todos os processos student e do suport_agent
wait $SUPORT_AGENT_PID
for pid in "${STUDENT_PIDS[@]}"; do
    wait "$pid"
done

#Remove os named pipes
rm "$FIFO_NAME"
for ((i=1; i<=NSTUD; i++)); do
    rm "/tmp/student$i"
done

echo "Suport Desk: Execução completa e limpeza realizada."