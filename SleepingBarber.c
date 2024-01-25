#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define CLIENTES 10
#define SILLAS 5


int id; //Variable que identifica a los clientes.
int pos; // Posición en la queue (sillas) de los clientes.
int sig; // Cliente que sigue en la queue.
int turno[SILLAS]; // Queue (sillas).
int barbero = 1; //Disponibilidad del barbero.

pthread_cond_t cv; //Condición variable usada para que los clientes esperen.
pthread_mutex_t mutex; //Mutex general.
sem_t sem_clientes; //Semáforo que indica que el barbero tiene clientes.
sem_t sem_cortando; //Semáforo que indica que se puede entrar a la función cortando.
sem_t sem_pagando; //Semáforo que indica que se puede entrar a la función pagando.
sem_t sem_barberia; //Semáforo que indica si hay lugar en la barberia.

//Funciones encargadas de cortar y pagar.
void cortando(){
    printf("Estoy cortando\n");
    sleep(1);
    return;
}

void me_cortan(int i){
    printf("Me estan cortando, cliente %d\n", i);
    sleep(1);
    return;
}

void pagando(int i){
    printf("Estoy pagando, cliente %d\n", i);
    sleep(1);
    return;
}

void me_pagan(){
    printf("Me estan pagando\n");
    sleep(1);
    return;
}

/*
        El barbero si no tiene ningun cliente, espera con su variable "barbero" en 1 (disponible).
    De lo contrario, indica que va a empezar a cortar. Cuando quiera cobrar,
    deberá esperar a que el cliente levante el semáforo para comenzar a pagar.
    Esto asegura que estas funciones se hagan de manera concurrente.
        Una vez que le pagan al barbero, el barbero toma el mutex y da el siguiente
    turno y pone a barbero en 1. Además, despierta a los demás clientes para que pueda pasar
    el siguiente en  la fila.  
*/
void* barbero_func(){
    while(1){
		//Espera a que entre un cliente para despertarse
        sem_wait(&sem_clientes); 
        sem_post(&sem_cortando); //Con el cliente manejan estos semaforos para realizar
                                 //las funciones de manera concurrente.
        cortando();
        sem_wait(&sem_pagando);
        me_pagan();
        pthread_mutex_lock(&mutex);
        sig = (sig + 1) % SILLAS; //Da el proximo turno.
        barbero = 1; //Avisa que ya está disponible.
        pthread_mutex_unlock(&mutex);
        pthread_cond_broadcast(&cv); //Levanta a los que tenia esperando.
    }
}

/*
        Cuando entra un cliente, se fija si hay lugar. Si no hay, suelta el mutex y se vá (retorna).
    Si hay lugar, se sienta en la posición del arreglo que le corresponde (marcado por la variable pos).
    Una vez hecho esto, checkea si su turno es el siguiente o si el barbero está disponible.
    Si alguna de estas no se cumple, se pone a esperar.
        Cuando es su turno, marca al barbero como ocupado así el proximo cliente que 
    entra (se sentará en su lugar) es obligado a esperar a su turno.
    Además de esto, avisa que hay un lugar en la barberia y despierta al barbero.
        Luego suelta el mutex y ejecuta concurrentemente las funciones de cortar y pagar con el barbero. 
*/
void* clientes_func(void* arg){
    int num = arg - NULL;
    
    //Lockean el mutex ya que van a entrar a la barberia.
    pthread_mutex_lock(&mutex);
    
    if (sem_trywait(&sem_barberia)){
        printf("Me voy\n");
        pthread_mutex_unlock(&mutex);
        return NULL;
    }
    
    
    //Se le dá un turno.
    printf("Entra cliente: %d, se sienta en: %d\n", num, pos);
    turno[pos] = num;
    pos = (pos + 1) % SILLAS;

    while(turno[sig] != num || barbero == 0){ //Si no es el turno de el o el barbero esta ocupado, espera.
        pthread_cond_wait(&cv, &mutex); 
    }

    barbero = 0; //Ocupa al barbero ya que se va a cortar.
    sem_post(&sem_barberia); //Ademas avisa que hay un lugar disponible.
    sem_post(&sem_clientes); //Le avisa al barbero que tiene un cliente.
    printf("Se levantó: %d\n", num);
    pthread_mutex_unlock(&mutex);
    
    
    sem_wait(&sem_cortando);
    me_cortan(num);
    sem_post(&sem_pagando);
    pagando(num);
    printf("Me cortaron y pagué\n");
 
    return NULL;
}

/*
        Esta funcion permite simular la entrada de infinitos clientes solo con 5 hilos.
    Cada hilo que entra, calcula un id que será su número de cliente y ejecuta la función 
    que permite "entrar a la barberia".
*/
void* maneja_proceso(void* arg){
    while(1){
        int num_actual = 0;
        sleep(1);
        pthread_mutex_lock(&mutex);
        id = (id + 1) % 10000;
        num_actual = id;
        pthread_mutex_unlock(&mutex);
        clientes_func(num_actual + (void*) 0); 
    }
}

int main(){
    pthread_t barbero, thread[CLIENTES];
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cv, NULL);
    sem_init(&sem_clientes, 0, 0);
    sem_init(&sem_cortando, 0, 0);
    sem_init(&sem_pagando,0,0);
    sem_init(&sem_barberia, 0, 5);
    int i;
    pthread_create(&barbero, NULL, barbero_func, NULL);
    for (i = 0; i < CLIENTES; i++)
        pthread_create(&thread[i], NULL, maneja_proceso,NULL);
    
    pthread_join(barbero, NULL); 
}
