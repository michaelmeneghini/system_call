#include "../inc/request_response.h"

char* toServerFIFO = "./FIFOSERVER";       // Client -> FIFOSERVER (Request)  -> Server
char* baseClientFIFO = "./FIFOCLIENT.";

// struttura che verrà salvata nella memoria condivisa

struct Entry {
    char user_id[50];               // Username
    long key;                       // Chiave generata dal server
    long timestamp;                 // Timestamp dell'istante in cui la richiesta viene scritta nella memoria condivisa
};
