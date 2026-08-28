#include <stdio.h>
#include <stdint.h> 
#include <stdlib.h>
#include <string.h>

#define BUFFER_DEFAULT_SIZE 1024

int _strchrxt(char* str,char x,int avoid);
int _strcpybxy(char** dest,char* src,int x,int y);

// Liste de type que peux prendre un JSOn
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

// Représentation d'une valeur JSOn qui peut 
typedef struct JsonValue{
    union {
        int integer; // ou Binaire
        double decimal;
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





// String reserch x time
int _strchrxt(char* str,char x,int avoid){
    int xTime = 0;
    for(int i=0;i<(int)strlen(str);i++){
        if(str[i] == x && avoid == xTime){
            return i;
        }
        if(str[i] == x) {
            xTime++;
        }
    }   
    return -1;
}

// String copy between x and y
int _strcpybxy(char** dest,char* src,int x,int y){
    if(x<0 || y>(int)strlen(src)+1){
        return 1;
    }
    if(y<x){
        return 1;
    }

    // dest = (char*)realloc(dest,(y-x+1)*sizeof(char));
    if(*dest == NULL){
        *dest = (char*)malloc((y-x+1));
    }else{
        *dest = (char*)realloc(*dest,(y-x+1));    
    }

    int index = 0;
    int lenSrc = strlen(src);
    for(int i=0;i<lenSrc;i++){
        if(i>=x && i<=y){
            (*dest)[index]=src[i];
            index++;
        }
    }
    (*dest)[index] = '\0';
    return 0;
}

int StringToInt(char* str){
    int strlenght = strlen(str);
    int neg = 0;
    int finaleNumber;
    if(str[0] == '-'){
        neg = 1;
    }
    for(i=0+neg;i<strlenght;i++){

    }

}