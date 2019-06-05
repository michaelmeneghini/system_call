#include <stdlib.h>
#include <stdio.h>

#include "../../miscellaneous.h"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include <fcntl.h>
#include <time.h>
#include <signal.h>

// VARIABILE CHE RAPPRESENTA IL SEMAFORO
int semid;
int shmid;
struct Entry* shared_memory;


void print_shm(char * color){
    printf("%s ------------------------------------------------------------- %s\n", color, ANSI_COLOR_RESET );
    struct Entry *current_shm = shared_memory;
    int i;
    for(i=0; i<SHM_MAX_ENTRIES; i++){
        printf("%s USER: %s | KEY: %ld | TIMESTAMP: %ld %s\n",color, current_shm[i].user_id, current_shm[i].key, current_shm[i].timestamp, ANSI_COLOR_RESET );
    }
    printf("%s ------------------------------------------------------------- %s\n", color, ANSI_COLOR_RESET );
}

// Funzione che gestisce l'arrivo del segnale SIGTERM per il processo figlio key manager
void key_manager_handler(int sig){
    printf(" %s \n\n[KEY MANAGER] terminated. \n\n %s", ANSI_COLOR_YELLOW, ANSI_COLOR_RESET);
    exit(0);
}

// Funzione che gestisce l'arrivo del segnale SIGTERM per il processo server
void server_handler( int sig ){

  // Mando un sigterm a key manager
  if(signal(SIGTERM, key_manager_handler) == SIG_ERR){
      printf("ERROR IN KEY MANAGER EXIT");
  }

  printf("\nTHANK YOU, SEE YOU NEXT TIME.\n");

  //La rimuovo
  if(unlink(toServerFIFO) == -1){
      printf("UNLINK FAILED");
  }

  //Detatch memoria condivisa
  if(shmdt(shared_memory) == -1){
      printf("ERROR DURING SHARED MEMORY DETATCH");
  } else {
      printf("> DETATCH COMPLETED\n");
  }

  //elimino il segmento di memoria condivisa
  if(shmctl(shmid, IPC_RMID, NULL) == -1){
      printf("ERROR DURING SHARED MEMORY ELIMINATION");
  } else {
      printf("> SHARED MEMORY DELETED\n");
  }

  // Rimuovo il semaforo
  if( semctl(semid, 1, IPC_RMID, NULL) == -1){
      printf("ERROR DURING SEMAPHORE ELIMINATION");
  } else {
      printf("> SEMAPHORE REMOVED\n");
  }

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

void shm_update(struct Request *request, struct Response response){

    // Preparo la entry da scrivere in memoria
    struct Entry entry;
    sprintf(entry.user_id, "%s", request->user_id);
    entry.key = response.key;
    entry.timestamp = (long) time(NULL);

    struct Entry* current_shm = shared_memory;

    //Scrivo nella prima cella vuota che trovo
    //Controllo che il timestamp sia inizializzato a 0
    int i;
    for(i=0; i<SHM_MAX_ENTRIES; i++){
        if( current_shm[i].timestamp == 0){
                current_shm[i] = entry;
                break;
        }
    }

}

void send_response(struct Request *request){

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
    semOp(semid, 0, 1);
    shm_update( request, response);
    print_shm(ANSI_COLOR_GREEN  );
    //V -> sblocco
    semOp(semid, 0, -1);

    printf("Sem:%d getpid:%d getncnt:%d getzcnt:%d\n",
    semctl(semid, 0, GETPID, NULL),
    semctl(semid, 0, GETNCNT, NULL),
    semctl(semid, 0, GETZCNT, NULL));

}

int main (int argc, char *argv[]) {

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

    //---------KEY MANAGER-----------//

    pid_t km = fork();

    if(km == -1){
        err_exit("ERROR DURING KEY MANAGER INITIALIZATION");
    }

    // KEY MANAGERù
    if(km == 0){

        printf("%s \n[KEY MANAGER]\n %s", ANSI_COLOR_YELLOW, ANSI_COLOR_RESET);

        // Setto un signal handler esclusivamente per Key Manager
        if (signal(SIGTERM, key_manager_handler) == SIG_ERR){
            err_exit(" Error in signal management");
        }

        while(1){
            sleep(30);
            //P --> blocco
            semOp(semid, 0, 1);
            // COntrollo sulla memoria condivisa
            // Se trovo Entry non valide, le "azzero" e le rendo disponibili ad essere riempite da nuove richieste
            struct Entry* current_shm = shared_memory;
            int i;
            for(i=0; i<SHM_MAX_ENTRIES; i++){
                // Se l'entry è vuota non ha senso far qualcosa
                if( current_shm[i].timestamp != 0){
                    // Se la differenza di timestamp dell'istante corrente e il timestamp dell'entry >= 300 elimino l'entry
                    // 300 secondi -> 5 minuti
                    if( (long)time(NULL) - current_shm[i].timestamp >= 300){
                        sprintf(current_shm[i].user_id, "%s", "");
                        current_shm[i].key = 0;
                        current_shm[i].timestamp = 0;
                        // Stampo la nuova situazione della memoria
                        print_shm(ANSI_COLOR_YELLOW);
                    }
                }
            }
            //V --> sblocco
            semOp(semid, 0, -1);
        }
    }

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
            send_response(&request);
        }

        if(close(serverFIFO) == -1){
              err_exit("CLOSE FAILED");
        }
    }
    //------------------------------//
}
