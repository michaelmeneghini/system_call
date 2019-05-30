#include <stdlib.h>
#include <stdio.h>
#include "../inc/request_response.h"
#include "../../err_exit.c"
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <string.h>

char* toServerFIFO = "/FIFOSERVER";       // Client -> FIFOSERVER (Request)  -> Server
char* baseClientFIFO = "/FIFOCLIENT.";

int serverFIFO;
//TODO:
// Key hanlder

// Funzione che gestisce l'arrivo del segnale SIGTERM
void quit( int sig ){

  printf("THANK YOU, SEE YOU NEXT TIME.");

  //Chiudo la fifo
  if(close(serverFIFO) == -1){
      err_exit("CLOSE FAILED");
  }
  //La rimuovo
  if(unlink(toServerFIFO) == -1){
      err_exit("UNLINK FAILED");
  }
  exit(0);
}


// Funzione che genera la chiave di accesso ai servizi a partire dalla richiesta
// Formato della chiave:
//  TIMESTAMP_RICHIESTA|CODICE_SERVIZIO (0 - stampa, 1 - salva, 2- invia)
// ES: 15589958550 (timestamp: 1558995855 - servizio: 0 (stampa))
long key_generator(struct Request *request){
    long timestamp = (long) time(NULL)*10;

    if( strcmp(request->service, "Stampa") == 0){
        return timestamp + 0;
    } else if (strcmp(request->service, "Salva") == 0){
        return timestamp + 1;
    } else {
        return timestamp + 2;
    };

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

}

void safe_unlink(){
    unlink(toServerFIFO);
    printf("FIFO UNLINKED %s\n", toServerFIFO);

}

int main (int argc, char *argv[]) {
    safe_unlink();

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
    if( signal(SIGTERM, quit) == SIG_ERR){
        err_exit("Errore nel gestore dei segnali");
    }
    //-------------------------------//

    //------REQUEST RESPONSE--------//
    while(1){
    printf("\n> WAITING A CLIENT .. \n");
    serverFIFO = open(toServerFIFO, O_RDONLY | S_IRUSR);
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
        send_response(&request);
    }


    }
    //------------------------------//
}
