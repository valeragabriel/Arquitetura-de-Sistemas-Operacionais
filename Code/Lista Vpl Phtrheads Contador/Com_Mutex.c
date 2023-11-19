#include <stdio.h>
#include <pthread.h>

#define NUM_THREADS 128
#define INCREMENTS_PER_THREAD 1000

volatile int counter = 0; // Contador global
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex inicializado

void *increment_counter(void *thread_id) {
    for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
        pthread_mutex_lock(&mutex); // Bloqueia o mutex antes de acessar o contador
        counter++; // Incrementa o contador
        pthread_mutex_unlock(&mutex); // Desbloqueia o mutex após a atualização
    }
    
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, increment_counter, NULL) != 0) {
            perror("Erro ao criar thread");
            return 1;
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Valor final do contador com mutex: %d\n", counter);

    // Destroi o mutex
    pthread_mutex_destroy(&mutex);

    return 0;
}
