#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "../../miscellaneous.h"

#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <fcntl.h>

int main (int argc, char *argv[]) {

    if( argc < 4 ){
        printf("Corretto utilizzo degli argomenti:\n 1: Codice identificativo dell'utente \n 2: Chiave rilasciata dal server \n 3 (in poi): Lista argomenti del servizio scelto\n\n");
        return 0;
    }

    char user_id[50];
    sprintf(user_id, "%s", argv[1]);
    long key = atol(argv[2]);

    // ottengo l'id del semaforo creato dal server
    int semid = semget( sem_key, 1, IPC_CREAT | S_IRUSR | S_IWUSR);
    if( semid == -1 ){
        err_exit(" ERROR: semget FAILED");
    }

    // ottengo l'id del segmento di memoria condivisa creato dal server
    int shmid = shmget( shm_key, sizeof(struct Entry) *SHM_MAX_ENTRIES,  S_IRUSR | S_IWUSR);
    if(shmid == -1){
        err_exit("ERROR: shmget FAILED");
    }

    // effettuo l'attachment di questo processo alla memoria condivisa
    struct Entry* shared_memory = shmat( shmid, NULL, S_IRUSR | S_IWUSR);
    if( shared_memory == (void *) -1 ){
        err_exit("ERROR: shmat FAILED");
    }

    struct Entry* current_shm = shared_memory;
    //P --> blocco
    semOp(semid, 0, 1);

    // cerco la COPPIA user_id / key nella memoria condivisa
    int i;
    for(i=0; i<SHM_MAX_ENTRIES; i++){
          if( strcmp(user_id, current_shm[i].user_id) == 0 && key == current_shm[i].key){
              //rimuovo l'entry
              sprintf(current_shm[i].user_id, "%s", "");
              current_shm[i].key = 0;
              current_shm[i].timestamp = 0;

              // sblocco il semaforo qui visto che ho trovato una corrispondenza in memoria condivisa
              //so che con l'exec il codice verrà cambiato e non avrò altre opportunità di sbloccarlo
              semOp(semid, 0, -1);

              // stessa cosa per il detatch della memoria
              if(shmdt(shared_memory) == -1){
                  printf("ERROR DURING SHARED MEMORY DETATCH");
              } else {
                  printf("\n DETATCH COMPLETED \n");
              }

              // Il mio array di argomenti da passare ai vari exec sarà grande [argc-2] argv[0-1-2] non li devo contare! e mi serve una cella vuota alla fine
              // Per inserirci (char *) NULL
              char * service_arguments[argc-2];
              int j;
              for(j=0; j< argc-2; j++){
                  service_arguments[j] = argv[j+3];
              }
              service_arguments[argc-2] = (char *) NULL;


              // controllo quale servizio devo chiamare osservando l'ultima cifra della key
              //0 = Stampa, 1 = Salva, 2 = Invia
              switch ( key % 10 ) {
                case 0:
                    execv("./stampa", service_arguments);
                    break;
                case 1:
                    execv("./salva", service_arguments);
                    break;
                case 2:
                    printf("Esecuzione servizio Invia.");
                    break;
              }

            }
    }

    //se non trovo corrisponenze, sblocco qui il semaforo
    semOp(semid, 0 , -1);

    printf("\nCombinazione USER_ID - KEY incorretta o assente fra le richieste, riprovare.\n" );

    //V --> sblocco

    // Effettuo il Detatch dalla memoria condivisa
    if(shmdt(shared_memory) == -1){
        printf("ERROR DURING SHARED MEMORY DETATCH");
    } else {
        printf("\n\nDETATCH COMPLETED\n\n");
    }


}
