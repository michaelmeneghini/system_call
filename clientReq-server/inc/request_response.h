
#include <sys/types.h>

struct Request {
  char user_id[50];   // Identificativo dell'utente
  char service[50];   // Servizio richiesto dall'utente
  pid_t pid;          // Necessario per la gestione di più fifo
};

struct Response {
  long key;          // Chiave generata dal server
};
