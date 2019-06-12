#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <fcntl.h>

#include <sys/msg.h>

#include "../../err_exit.c"

struct Message{
	long type;
	char content[200];
};

int main (int argc, char *argv[]) {
    
    printf("\n --------------------\n");
    printf (" |SERVIZIO DI INVIO|");
    printf("\n --------------------\n\n");

   // il primo argomento è per requisito la key per la msg queue
   key_t msg_key = (key_t) atoi(argv[0]);

   // creo la msg queue o ne prendo il riferimento se già presente
   int msgqid = msgget( msg_key, IPC_CREAT | S_IRUSR | S_IWUSR);
   if( msgqid == -1 ){
   		err_exit(" msgget FAILED");
   		printf("\n --------------------\n\n");
   }

   struct Message msg;
   msg.type = 1;

   char * content;
   //Creo il contenuto del mio messaggio, prendendo i vari argomenti
   int i;
   for(i = 1; i< argc; i++){
   		strcat(content, argv[i]);
   		strcat(content, " ");
   }

   memcpy(msg.content, content, strlen(content)+1);

   size_t msg_size = sizeof( struct Message ) - sizeof(long);

   if( msgsnd(msgqid, &msg, msg_size, 0)  == -1){
   		err_exit(" msgsnd FAILED!");
   		printf("\n --------------------\n\n");
   }
   // rimuovo la coda 
   if ( msgctl(msgqid, IPC_RMID, NULL) == -1){
   		err_exit(" msgctl FAILED!");
   		printf("\n --------------------\n\n");
   }

   printf("\n --------------------\n\n");

    return 0;
}
