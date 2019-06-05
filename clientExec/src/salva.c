#include <stdlib.h>
#include <stdio.h>

#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "../../err_exit.c"

int main (int argc, char *argv[]) {

  int j;
  for(j=0; j<argc; j++){
      printf("%s\n", argv[j]);
  }


  printf("\n----------------------------------------\n");
  printf("Servizio SALVA: \n");

  // File descriptor del file che andrò a creare o su cui andrò a scrivere
  char file_name[50];
  sprintf(file_name, "%s", argv[0]);
  int fd = open(file_name, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  if( fd == -1 ){
      err_exit("ERROR: open FAILED\n");
  } else {
      printf("Apertura o creazione file completata .. \n");
  }

  char buffer[1000];
  int i;
  size_t size = 0;
  for( i=1; i< argc; i++){
      size += strlen(argv[i])+1;
  }

  ssize_t bW = 0;
  for(i=1;i <argc;i++){
      bW += write(fd, argv[i], strlen(argv[i])+1);
  }

  if( bW != size ){
      err_exit("ERROR: write FAILED");
  } else {
      printf("Salvataggio su file completato ..");
  }

  printf("\n----------------------------------------\n");

}
