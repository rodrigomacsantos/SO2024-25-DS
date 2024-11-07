#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define N_DISCIPLINAS 3
#define N_SECRETARIOS 10
#define N_INSC_PER_PRODUCER 5


long disciplinas[N_DISCIPLINAS];
pthread_mutex_t mutexxx[N_DISCIPLINAS];
//Este função faz as inscrições de um numero de alunos de cada vez
void inscrever_cadeiras (void) {

    for(int i = 0; i < N_DISCIPLINAS; i++) {
        pthread_mutex_lock(&mutexxx[i]);
        disciplinas[i] +=1;
        sleep(1);
        pthread_mutex_unlock(&mutexxx[i]);

    }
}

// *data Um ponteiro genérico para o numero de inscrições a fazer
//Na função main os producers passam a ser threads. Para isso, alterem o ciclo for que chama o
//producer para criar threads que chamam o producer.
void *producer(void *data) {
    int *ptr = (int *) data;
    int no_inscricoes = *(ptr);

    for (int i = 0; i < no_inscricoes; i++) {
        inscrever_cadeiras();
    }
    return NULL;
}


int main(int argc, char const *argv[])
{
    int insc_per_producer = N_INSC_PER_PRODUCER;

    struct timespec start_time, end_time;

    pthread_t th[N_SECRETARIOS];
    for (int i = 0; i < N_DISCIPLINAS; i++) 
        pthread_mutex_init(&mutexxx[i], NULL);

    if (argc>=2) {
        sscanf (argv[1], "%d", &insc_per_producer);
        if (insc_per_producer <= 0 || insc_per_producer > N_INSC_PER_PRODUCER) {
            insc_per_producer = N_INSC_PER_PRODUCER;
        }

    }

    // Marcando o tempo de início (tempo de parede)
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    //Na função main os producers passam a ser threads. Para isso, alterem o ciclo for que chama o
    //producer para criar threads que chamam o producer.
    for (int i = 0; i < N_SECRETARIOS; i++) {

        pthread_create(&th[i], NULL, producer, &insc_per_producer);

        //producer(&insc_per_producer);
    }

    for (int i = 0; i < N_SECRETARIOS;i++) {
        pthread_join(th[i], NULL);
    }

    // Marcando o tempo de fim (tempo de parede)
    clock_gettime(CLOCK_MONOTONIC, &end_time);

    for (int i = 0; i < N_DISCIPLINAS; i++) {

        printf("Total inscritos na disciplina %ld: %ld \n", i, disciplinas[i]);
    }

    // Calculando o tempo total de execução
    double time_taken = (end_time.tv_sec - start_time.tv_sec) +
    (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    printf("O programa demorou %f segundos a executar.\n", time_taken);

    return 0;
}