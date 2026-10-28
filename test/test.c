#include <stdio.h>
#include <stdint.h> 
#include <stdlib.h>

#include "../cson.c"
#include "../cson.h"

int main(){

    JsonValue f = initCsonFromFile("./test.json");
    if(f.type == JSON_ERROR){
        return 1;
    }

    size_t indexChercher = getIndexFromObject(f.value.object,"nom");
    printf("Le nom est = %s\n",f.value.object->listeOfPair[indexChercher].value.value.string);


    return 0;
}