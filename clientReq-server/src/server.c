#include <stdlib.h>
#include <stdio.h>

#include "../inc/miscellaneous.h"
#include "../../semaphore.c"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/sem.h>

#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <string.h>

#include <errno.h>

#define SHM_MAX_ENTRIES 10

// VARIABILE CHE RAPPRESENTA IL SEMAFORO
int semid;
int shmid;
struct Entry* shared_memory;


void print_shm(){

    struct Entry *current_shm = shared_memory;
    printf("\n------------------------------------------------------------------------------------------\n");
    int i;
    for(i=0; i<SHM_MAX_ENTRIES; i++){
        printf("USER: %s  KEY: %ld   TIMESTAMP: %ld\n", current_shm[i].user_id, current_shm[i].key, current_shm[i].timestamp );
        printf("\n------------------------------------------------------------------------------------------\n");
    }

}

//TODO:
// Key hanlder

// Funzione che gestisce l'arrivo del segnale SIGTERM
void server_handler( int sig ){

  printf("\nTHANK YOU, SEE YOU NEXT TIME.\n");

  //La rimuovo
  if(unlink(toServerFIFO) == -1){
      printf("UNLINK FAILED");
  }

  //Detatch memoria condivisa
  if(shmdt(shared_memory) == -1){
      //printf("ERROR DURING SHARED MEMORY DETATCH");
      perror("ERROR DURING SHARED MEMORY DETATCH\n");
  }
  printf("> DETATCH COMPLETED\n");

  //elimino il segmento di memoria condivisa
  if(shmctl(shmid, IPC_RMID, NULL) == -1){
      printf("ERROR DURING SHARED MEMORY ELIMINATION");
  }
  printf("> SHARED MEMORY DELETED\n");

  // Rimuovo il semaforo
  if( semctl(semid, 1, IPC_RMID, NULL) == -1){
      printf("ERROR DURING SEMAPHORE ELIMINATION");
  }
  printf("> SEMAPHORE REMOVED\n");

  // TODO:
  // MANDA SIGTERM A KEY MANAGER

  exit(0);
}


// Funzione che genera la chiave di accesso ai servizi a partire dalla richiesta
// Per evitare possibili duplicati della chiave utilizzo timestamp in millisecondi
// Formato della chiave:
//  TIMESTAMP_RICHIESTA|CODICE_SERVIZIO (0 - stampa, 1 - salva, 2- invia)
// ES: 15589958555670 (timestamp: 1558995855567 - servizio: 0 (stampa))
long key_generator(struct Request *request){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long timestamp = (long) tv.tv_usec;
    timestamp /= 1000;
    timestamp += (tv.tv_sec * 1000);

    if( strcmp(request->service, "Stampa") == 0){
        return (timestamp*10) + 0;
    } else if (strcmp(request->service, "Salva") == 0){
        return (timestamp*10) + 1;
    } else {
        return (timestamp*10) + 2;
    };

}

void shm_update(struct Request *request, struct Response response, int saved_entries){

    // Preparo la entry da scrivere in memoria
    struct Entry entry;
    sprintf(entry.user_id, "%s", request->user_id);
    entry.key = response.key;
    entry.timestamp = (long) time(NULL);

    //
    struct Entry *current_shm = shared_memory;

    // Scrivo quando trovo la prima entry vuota / non valida
    current_shm[saved_entries-1] = entry;


}

void send_response(struct Request *request, int entries){

    //path per la risposta
    char toClientFIFO[20];
    sprintf(toClientFIFO, "%s%d", baseClientFIFO, request->pid);

    printf("\n> OPENING FIFO %s \n", toClientFIFO);
    //apro la FIFO
    int clientFIFO = open(toClientFIFO, O_WRONLY | S_IWUSR);
    if (clientFIFO == -1){
        err_exit("OPEN FAILED\n");
    }

    printf("> REQUEST FROM: %s\n", request->user_id);
    printf("> SERVICE REQUESTED: %s\n", request->service);

    //risposta
    struct Response response;
    response.key = key_generator(request);

    //la scrivo nella FIFO in uscita
    printf("> SENDING A RESPONSE\n");
    if(write(clientFIFO, &response, sizeof(struct Response)) != sizeof(struct Response)){
        printf("ERROR IN SENDING THE RESPONSE");

    }
    // chiudo la FIFOCLIENT
    if (close(clientFIFO) != 0){
        err_exit("CLOSE FAILED ");
    }

    //P -> blocco
    //semOp(semid, 0, 1);
    //shm_update( request, response, entries);
    //print_shm();
    //V -> sblocco
  //  semOp(semid, 0, -1);

}

void safe_unlink(){
    unlink(toServerFIFO);
    printf("FIFO UNLINKED %s\n", toServerFIFO);

}

int main (int argc, char *argv[]) {
    safe_unlink();

    //--INIT SEMAFORO E MEMORIA CONDIVISA--//
    // sem_key e shm_key sono definite in sempahore.c
    semid = semget( sem_key, 1, IPC_CREAT |  S_IRUSR | S_IWUSR );
    if(semid == -1){
        err_exit("ERROR IN SEMAPHORE CREATION");
    }

    // Inizializzo il semaforo a 0
    union semun arg;
    arg.val = 0;
    if( semctl(semid, 0, SETVAL, arg) == -1){
        err_exit("ERROR IN SEMAPHORE INITIALIZATION");
    }

    // Creo il segmento di memoria convidisa
    shmid = shmget( shm_key, sizeof( struct Entry ) * SHM_MAX_ENTRIES, IPC_CREAT | S_IRUSR | S_IWUSR);
    if ( shmid == -1 ){
        printf("ERROR IN SHARED MEMORY CREATION\n");
    }

    // Effetto l'attachment di questo processo alla memoria condivisa
    shared_memory = (struct Entry *) (unsigned long) shmat( shmid, NULL, 0);
    if (shared_memory == (void *) -1){
        printf("ERROR DURING SHARED MEMORY ATTACHMENT\n");
    }


    //-------------------------------------//

    // Creo la FIFO
    printf("> CREATING FIFO ... \n");
    if( mkfifo(toServerFIFO, S_IRUSR | S_IWUSR) == -1){
        err_exit(" mkfifo è fallita");
    }
    printf("> FIFOSERVER WAS SUCCESFULLY CREATED.\n" );

    //-------GESTIONE SEGNALI-------//
    // Blocco tutti i segnali tranne SIGTERM
    sigset_t mySet;
    if(sigfillset(&mySet) == -1 || sigdelset(&mySet, SIGTERM) == -1 ||
       sigprocmask(SIG_SETMASK, &mySet, NULL)){
          err_exit("Errore durante la gestione dei segnali");
    }
    // Setto il signal handlers
    if( signal(SIGTERM, server_handler) == SIG_ERR){
        err_exit("Errore nel gestore dei segnali");
    }
    //-------------------------------//

    //Variabile necessaria per la scrittura in memoria CONDIVISA
    int entries = 0;

    //------REQUEST RESPONSE--------//
    while(1){
        printf("\n> WAITING A CLIENT .. \n");
        int serverFIFO = open(toServerFIFO, O_RDONLY | S_IRUSR);
        if(serverFIFO == -1){
            err_exit("Open READ-ONLY failed");
        }

        struct Request request;
        int bR = -1;
        printf("\n> WAITING A REQUEST ... \n");


        // Controllo se è presente una risposta
        bR = read(serverFIFO, &request, sizeof(struct Request));
        if(bR == -1){
            printf("THE FIFO HAS SOME PROBLEMS.\n");
            // METTI A 0 LA CHIAVE E RENDILA INVALIDA
        } else if(bR != sizeof(struct Request)){
            printf("THE REQUEST SEEMS INCOMPLETE\n");
            // METTI A 0 LA CHIAVE E RENDILA INVALIDA
        } else {
          //Chiudo la fifo
          if(close(serverFIFO) == -1){
              err_exit("CLOSE FAILED");
          }
            entries++;
            send_response(&request, entries);
        }


    }
    //------------------------------//
}
