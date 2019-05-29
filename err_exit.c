#include "err_exit.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

void err_exit(const char *msg) {
    perror(msg);
    exit(1);
}
