#include <stdlib.h>
#include <stdio.h>
#include "../inc/request_response.h"
#include "../../err_exit.c"
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

// TEST FUNCTIONS -> to remove after

void print_request(struct Request request){
  printf("USER ID: %s\n", request.user_id);
  printf("SERVICE : %s\n", request.service);
  printf("PID: %d\n", request.pid);
}


char* toServerFIFO = "/FIFOSERVER";       // Client -> FIFOSERVER (Request)  -> Server
char* baseClientFIFO = "/FIFOCLIENT.";    // Server -> FIFOCLIENT (Response) -> Client

int main (int argc, char *argv[]) {

    struct Request request;

    //------ DISPLAY MENU ------//
    printf("Benvenuto nel gestore servizi, sono disponibili i seguenti servizi:\n- Stampa\n- Salva\n- Invia \n\n");
    printf("Inserisci le seguenti informazioni: \nClient ID: ");
    scanf(" %s", request.user_id);
    char temp_service[50];

    // L'utente deve inserire correttamente il nome del servizio!
    do{
        printf("Servizio desiderato: ");
        scanf(" %s", temp_service);
    } while(strcmp(temp_service, "Stampa") != 0 &&
            strcmp(temp_service, "Salva")  != 0 &&
            strcmp(temp_service, "Invia")  != 0);
    sprintf(request.service, "%s", temp_service);
    request.pid = getpid();
    print_request(request);
    //--------------------------//

    // Creo il path effettivo della FIFO in entrata
    // avrà il formato: FIFOCLIENT.(pid)
    char toClientFIFO[20];
    sprintf(toClientFIFO, "%s%d", baseClientFIFO, getpid());
    //printf("%s", toClientFIFO);

    // FIFO IN ENTRATA
    if(mkfifo(toClientFIFO, S_IRUSR | S_IWUSR) == -1){
          err_exit("Errore durante la creazione di FIFOCLIENT");
    }
    printf("%s created.", toClientFIFO);

    // Richiesta al server
    int serverFIFO = open(toServerFIFO, O_WRONLY | S_IWUSR);
    if( serverFIFO == -1){
        err_exit("Operazione open fallita");
    }


    if( write(serverFIFO, &request, sizeof(struct Request)) != sizeof(struct Request)){
        err_exit("Errore durante la richiesta");
    }

    // Apro la FIFO in entrata e mi preparo a ricevere
    int clientFIFO = open(toClientFIFO, O_RDONLY | S_IRUSR);
    if( clientFIFO == -1 ){
        err_exit("Operazione open in entrata fallita");
    }

     // Da fixxare ed utilizzare un semfaroro
    struct Response response;
    int bR = -1 ;
    bR = read(clientFIFO, &response, sizeof(struct Response));
    if ( bR == -1){
        err_exit("Errore nella ricezione della risposta, riprovare.");
    } else if ( bR != sizeof(struct Response)){
        err_exit("Errore nella ricezione della risposta, riprovare.");
    }

    printf("\nCHIAVE: %ld", response.key);
    printf("\n");

    //Chiudo le FIFO
    if(close(serverFIFO) != 0 || close(clientFIFO) != 0){
        err_exit("Errore durante la chiusura delle FIFO");
    }

    //Rimuovo la FIFO da fs
    if (unlink(toClientFIFO) != 0){
      err_exit("Operazione di unlink fallita");
    }


    return 0;

}
