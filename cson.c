#include <stdio.h>
#include <stdint.h> 
#include <stdlib.h>
#include <string.h>

#define BUFFER_DEFAULT_SIZE 1024

long getSize(FILE* file);
int loadJson(char** dest,FILE* file);
int whitespaceCleaner(char** str_json,long* size);

// Liste de type que peux prendre un JSON
typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT,
    JSON_DECIMAL
} JsonType;

// Pré-déclaration
typedef struct JsonArray JsonArray;
typedef struct JsonObject JsonObject;

// Représentation d'une valeur JSON qui peut étre l'une de ces valeur -> Paire valeur type
typedef struct JsonValue{
    union {
        int integer; // ou Binaire
        double decimal; // JSON ne fais pas la diff entre integer et decimal mais la je suis C
        char* string;
        JsonArray* array;   // On file des pointeur pour des soucis de place,
        JsonObject* object; // vue qu'un pointeur c'est toujour 8 octet
    } value;
    JsonType type;
}JsonValue;

typedef struct JsonPair{
    char* key;
    JsonValue value;
}JsonPair;

typedef struct JsonArray{
    int nbOfElemet;
    JsonValue * listeOfValue;
}JsonArray;

typedef struct JsonObject{
    int nbOfElemet;
    JsonPair * listeOfPair;
}JsonObject;

typedef struct JsonRoot{
    JsonValue root;
}JsonRoot;

int main(void){
    char* str = (char*)calloc(46,sizeof(char));
    strcpy(str,"{  \"name\"  : \n \"John\"  ,  \"age\"  :  30  }");

    long size = 46;
    whitespaceCleaner(&str,&size);
    printf("%s\n",str);

    return 0;
}

int loadJson(char** dest,FILE* file){
    // Détermine la longeur du JSON
    long size = getSize(file);
    if(size == -1){
        return 1;
    }

    // Si l'allocation de la dest existe alors realloc sinon calloc (Par sécuritée)
    if(*dest == NULL){
        *dest = (char*)calloc((size+1),sizeof(char));
        if((*dest) == NULL){
            return 1;
        }
    }else{
        *dest = realloc(*dest,(size+1)*sizeof(char));
        if((*dest) == NULL){
            return 1;
        }
    }

    // On lit tout le fichier et le décale dans dest
    long read = fread(*dest,1,size,file);
    if(read < size){
        return 1;
    }
    *(dest) [size] = '\0';
    return 0;
}

int whitespaceCleaner(char** str_json,long* size){
    int whitespaceCleanedCount = 0;
    int index = 0;
    int IsInsideQuote = 0;

    for(long i = 0 ;i<(*size);i++){
        char c = (*str_json)[i];

        // Vue que l'on ne veux pas modifiée l'intérieur des string on vérifie si c'est pas
        // string qu'on essaye de netoyée
        if(c == '"'){
            if(IsInsideQuote == 1){
                if((*str_json)[i-1] == '\\'){
                    continue;
                }else{
                    IsInsideQuote = 0;
                }
            }else{
                IsInsideQuote = 1;
            }
        }
        // Si le char se trouve entre '"' alors il est dans une string donc on saute
        if(IsInsideQuote == 1){
            continue;
        }
        index = i;
        //Si on trouveun whitespace
        if(c == '\n' || c == '\t' || c == '\r' || c == ' '){
            //on décale tout les char pour suppr de whitespace
            while(index<*size-1){
                (*str_json)[index] = (*str_json)[index+1];
                index++;
            }
            // On compte le nombre de char suppr pour modifiée la taille du fichier
            whitespaceCleanedCount ++;
            //On résychronise i
            i--;
        }
    }

    // On modifie la taille
    *size = *size - whitespaceCleanedCount;
    return 0;
}

long getSize(FILE* file){
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    return size;
}