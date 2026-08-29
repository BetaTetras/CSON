#include <stdio.h>
#include <stdint.h> 
#include <stdlib.h>
#include <string.h>

#define BUFFER_DEFAULT_SIZE 1024

#define DEBUG_VALUE 0

// Liste de type que peux prendre un JSON
typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT,
    JSON_DECIMAL,
    JSON_ERROR
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

JsonType getType(char* json);
long getSize(FILE* file);
int loadJson(char** dest,FILE* file);
int whitespaceCleaner(char** str_json,long* size);

size_t _strlen(char* str);
int _strcmp(char* str1,char* str2);
int _strchr(char* str,char c);
int _strcpybxy(char **dest, char *src, int x, int y);
int _strchrxt(char * str,char x,int avoid);
int stringToInt(char* str,int* res);

void debug();

int main(int argc, char *argv[]){
    if(argc != 2){
        printf("Usage: %s <path_to_json>\n", argv[0]);
        return 1;
    }
    int state; // Pour vérifiée le res d'une fonction (0 ou 1)
    char* path = argv[1];
    char* json_str = NULL;
    FILE* json_file = fopen(path,"r");
    if(json_file == NULL){
        printf("Error occure during the opening process of the json file, check the path:\n %s\n",path);
        return 1;
    }
    long size = getSize(json_file);
    

    state = loadJson(&json_str,json_file);
    printf("Debug");
    fclose(json_file);
    if(state == 1){
        printf("Error occure during the loading of the json file on RAM (File to big?)\n");
        return 1;
    }
    state = whitespaceCleaner(&json_str,&size);

    JsonRoot json_root;
    json_root.root.type = getType(json_str);
    switch(json_root.root.type){
        case JSON_NULL:
            json_root.root.value = NULL;
        break;
        case JSON_BOOL:
            if(_strcmp(json_str,"true") == 0){
                json_root.root.value.integer = 1;
            }else if(_strcmp(json_str,"false") == 0){
                json_root.root.value.integer = 0;
            }else{
                json_root.root.value = NULL;
                json_root.root.type = JSON_ERROR;
            }
        break;
        case JSON_NUMBER:
            int tmp_int = stringToInt(json_str,&state);
            if(state == 1){
                json_root.root.value = NULL;
                json_root.root.type = JSON_ERROR;
                break;
            }
            json_root.root.value.integer = tmp_int;
        break;
        case JSON_STRING:
            int x = _strchr(json_str,'"');
            int y = _strchrxt(json_str,'"',1);
            if(x == -1 || y == -1){
                json_root.root.type = JSON_ERROR;
                json_root.root.value = NULL;
                break;
            }
            char* tmp_str;
            state = _strcpybxy(&tmp_str,json_str,x,y);
            if(state == 1){
                json_root.root.value = NULL;
                json_root.root.type = JSON_ERROR;
                break;
            }
            json_root.root.type.string = tmp_str;
        break;
        case JSON_ARRAY:
            printf("JSON_ARRAY\n");
        break;
        case JSON_OBJECT:
            // A faire -> récursion
        break;
        case JSON_DECIMAL:
            printf("JSON_DECIMAL\n");
        break;
        case JSON_ERROR:
            printf("JSON_ERROR\n");
        break;
    }

    return 0;
}



int loadJson(char** dest,FILE* file){
    // Détermine la longeur du JSON
    long size = getSize(file); //Rewind -> Retour au debut
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
    (*dest)[size] = '\0';
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

JsonType getType(char* json){
    if(json == NULL){
        return JSON_ERROR;
    }
    switch(json[0]){
        case 't':
        case 'f':
            return JSON_BOOL;
        break;
        case '-':
            if(_strchr(json,'.') > 0){
                return JSON_DECIMAL;
            }else{
                return JSON_NUMBER;
            }
        break;
        case 'n':
            return JSON_NULL;
        break;
        case '"':
            return JSON_STRING;
        break;
        case '[':
            return JSON_ARRAY;
        break;
        case '{':
            return JSON_OBJECT;
        break;
        default:
            if(_strchr(json,'.') > 0){
                return JSON_DECIMAL;
            }else{
                return JSON_NUMBER;
            }
        break;
    }
    return JSON_ERROR;
}

long getSize(FILE* file){
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    return size;
}

////////////////////////////////////////////////////

size_t _strlen(char* str){
    size_t i =0;
    while(str[i] != '\0'){
        i++;
    }
    return i;
}

int _strcmp(char* str1,char* str2){
    size_t size_1 = _strlen(str1);
    size_t size_2 = _strlen(str2);
    if(size_1 != size_2){
        return 1;
    }
    for(size_t i=0;i<size_1;i++){
        if(str1[i] != str2[i]){
            return 1;
        }
    }
    return 0;
}

int _strchr(char* str,char c){
    size_t size = _strlen(str);
    for(size_t i = 0;i<size;i++){
        if(str[i] == c){
            return (int)i;
        }
    }
    return -1;
}

// String copy between x and y
int _strcpybxy(char **dest, char *src, int x, int y){
    int lenSrc = strlen(src);
    if (x < 0 || y >= lenSrc || y < x) {
        return 1;
    }

    size_t size = y - x + 2;
    if (*dest == NULL) {
        *dest = malloc(size);
    } else {
        *dest = realloc(*dest, size);
    }
    if (*dest == NULL) {
        return 1;
    }

    int index = 0;
    for (int i = x; i <= y; i++) {
        (*dest)[index] = src[i];
        index++;
    }

    (*dest)[index] = '\0';
    return 0;
}

// String reserch x time
int _strchrxt(char * str,char x,int avoid){
    int xTime = 0;
    int size = (int)_strlen(str);
    for(int i=0;i<size;i++){
        if(str[i] == x && avoid == xTime){
            return i;
        }
        if(str[i] == x) {
            xTime++;
        }
    }   
    return -1;
}

int stringToInt(char* str,int* res){
    if(str == NULL){
        return 1;
    }
    int result = 0;
    int boolNeg = 0;
    int i = 0;
    int len = (int)_strlen(str);
    if (len == 0) {
        return 1;
    }

    if(str[len-1] == '\n'){
        len--;
    } 
    if(str[0] == '-') {
        boolNeg = 1;
        i++;
    }

    for(; i < len; i++){
        if(str[i] < '0' || str[i] > '9'){
            return 1; // caractère invalide
        }
        result = result * 10 + (str[i] - '0');
    }
    if(boolNeg == 1){
        *res = -result;
    }else{
        *res = result;
    }
    return 0;
}

////////////////////////////////////////////////////

void debug(){
    if(DEBUG_VALUE == 1){
        printf("\033[38;2;255;0;0m");
        printf("DEBUG!\n");
        printf("\033[0m");
    }
}