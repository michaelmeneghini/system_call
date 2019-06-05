#include "clientReq-server/inc/request_response.h"

#include <string.h>
#include "err_exit.h"
#include "semaphore.c"

#define SHM_MAX_ENTRIES 10

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

char* toServerFIFO = "/FIFOSERVER";       // Client -> FIFOSERVER (Request)  -> Server
char* baseClientFIFO = "/FIFOCLIENT.";

// struttura che verrà salvata nella memoria condivisa

struct Entry {
    char user_id[50];               // Username
    long key;                       // Chiave generata dal server
    long timestamp;                 // Timestamp dell'istante in cui la richiesta viene scritta nella memoria condivisa
};
