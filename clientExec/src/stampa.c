#include <stdlib.h>
#include <stdio.h>



int main (int argc, char *argv[]) {

    printf("\n --------------------\n");
    printf (" |SERVIZIO DI STAMPA|");
    printf("\n --------------------\n\n");

    int i;
    for(i=0; i<argc; i++){
        printf("%s\n", argv[i]);
    }

    printf("\n --------------------\n");

    return 0;
}
