#include <stdio.h>
#include "cifrado.h" //IMPORTACION DE LA LIBRERIA CIFRADO.h

void contrasenacif(const char *entrada, char *salida) {
    unsigned long hashear = 5381;
    int c;

    while ((c = *entrada++))
        hashear = ((hashear << 5) + hashear) + c; // hash * 33 + c

    sprintf(salida, "%lu", hashear);
}