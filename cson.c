#include <stdio.h>
#include <stdint.h> 
#include <stdlib.h>
#include <string.h>

#define BUFFER_DEFAULT_SIZE 64

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
    int nbOfElement;
    JsonValue * listeOfValue;
}JsonArray;

typedef struct JsonObject{
    int nbOfElement;
    JsonPair * listeOfPair;
}JsonObject;

typedef struct JsonRoot{
    JsonValue root;
}JsonRoot;

JsonType getType(char* json_str,size_t position);
long getSize(FILE* file);
int loadJson(char** dest,FILE* file);
int whitespaceCleaner(char** str_json,long* size);

JsonValue parseOBJ(char* json_str, size_t* position);
JsonValue parseARRAY(char* json_str, size_t* position);
JsonValue parseSTRING(char* json_str,size_t* position);
JsonValue parseNUMBER(char* json_str,size_t* position);
JsonValue parseBOOL(char* json_str,size_t* position);
JsonValue parseNULL(char* json_str,size_t* position);
JsonValue parseDECIMAL(char* json_str, size_t* position);


JsonValue parseValue(char* json_str,size_t* position);

size_t _strlen(char* str);
int _strcmp(char* str1,char* str2);
int _strchr(char* str,char c);
int _strcpybxy(char **dest, char *src, int x, int y);
int _strchrxt(char * str,char x,int avoid);
int _strcpy(char** dest,char* src);

int stringToInt(char* str,int* res);
int stringToDouble(char* str, double* res);


void debug();

int main(int argc, char *argv[]){
    JsonValue test;

    char* json = "\"Hello, world! \"inter string\"";
    size_t pos = 0;

    test = parseSTRING(json,&pos);

    printf("%s\n",test.value.string);

    return 0;
}

JsonValue parseOBJ(char* json_str, size_t* position){
    size_t size = _strlen(json_str);
    size_t capacity = 10;

    JsonValue obj_value;
    obj_value.type = JSON_OBJECT;
    obj_value.value.object = (JsonObject){0};
    obj_value.value.object.listeOfPair = (JsonPair*)calloc(10,sizeof(JsonPair));

    JsonType targeted_type;
    JsonPair buffeur_pair;
    JsonValue buffeur_value;

    int NumberOfElement = 0;
    for(size_t index = *position+1;index<size;index++){
        if(json_str[index] == '}'){
            break;
        }else if(json_str[index] == ','){
            continue;
        }

        if(capacity >= NumberOfElement){
            capacity *= 2;
            obj_value.value.object.listeOfPair = (JsonPair*)realloc(
                obj_value.value.object.listeOfPair, 
                capacity * sizeof(JsonPair)
            );
        }

        buffeur_value = parseSTRING(json_str,&index);
        _strcpy(buffeur_pair.key,buffeur_value.value.string);

        index ++;

        targeted_type = getType(json_str,index);
        switch(targeted_type){
            case JSON_NUMBER:
                buffeur_pair.value = parseNUMBER(json_str,&index);
                obj_value.value.object.listeOfPair[NumberOfElement] = buffeur_pair;
                NumberOfElement++;
            break;
            case JSON_BOOL:
                buffeur_pair.value = parseBOOL(json_str,&index);
                obj_value.value.object.listeOfPair[NumberOfElement] = buffeur_pair;
                NumberOfElement++;
            break;
            case JSON_DECIMAL:
                buffeur_pair.value = parseDECIMAL(json_str,&index);
                obj_value.value.object.listeOfPair[NumberOfElement] = buffeur_pair;
                NumberOfElement++;
            break;
            case JSON_ERROR:
                _strcpy(obj_value.value.object.listeOfPair[NumberOfElement].key,"ERROR");
                obj_value.value.object.listeOfPair[NumberOfElement].value.value.integer = 0;
                obj_value.value.object.listeOfPair[NumberOfElement].value.type = JSON_ERROR;
                NumberOfElement++;
            break;
            case JSON_NULL:
                buffeur_pair.value = parseNULL(json_str,&index);
                obj_value.value.object.listeOfPair[NumberOfElement] = buffeur_pair;
                NumberOfElement++;
            break;
            case JSON_STRING:
                buffeur_pair.value = parseSTRING(json_str,&index);
                obj_value.value.object.listeOfPair[NumberOfElement] = buffeur_pair;
                NumberOfElement++;
            break;
            case JSON_ARRAY:
                buffeur_pair.value = parseARRAY(json_str,&index);
                obj_value.value.object.listeOfPair[NumberOfElement] = buffeur_pair;
                NumberOfElement++;
            break;
            case JSON_OBJECT:
                buffeur_pair.value = parseOBJ(json_str,&index);
                obj_value.value.object.listeOfPair[NumberOfElement] = buffeur_pair;
                NumberOfElement++;
            break;
        }
    }
    
    *position = index + 1;
    obj_value.object.nbOfElement = NumberOfElement;
    free(buffeur_value.value.string);
    return obj_value;
}

JsonValue parseARRAY(char* json_str, size_t* position){
    size_t size = _strlen(json_str);
    size_t capacity = 10;

    JsonValue ary_value;
    ary_value.type = JSON_ARRAY;
    ary_value.value.array = (JsonArray){0};
    ary_value.value.array.listeOfValue = (JsonValue*)calloc(capacity, sizeof(JsonValue));

    
    JsonType targeted_type;
    JsonValue buffeur;

    int NumberOfElement = 0;
    for(size_t index = *position+1;index<size;index++){
        if(json_str[index] == ']'){
            break;
        }else if(json_str[index] == ','){
            continue;
        }

        if(capacity >= NumberOfElement){
            capacity *= 2;
            ary_value.value.array.listeOfValue = (JsonValue*)realloc(
                ary_value.value.array.listeOfValue, 
                capacity * sizeof(JsonValue)
            );
        }

        targeted_type = getType(json_str,index);
        switch(targeted_type){
            case JSON_NUMBER:
                buffeur = parseNUMBER(json_str,&index);
                ary_value.value.array.listeOfValue[NumberOfElement] = buffeur;
                NumberOfElement++;
            break;
            case JSON_BOOL:
                buffeur = parseBOOL(json_str,&index);
                ary_value.value.array.listeOfValue[NumberOfElement] = buffeur;
                NumberOfElement++;
            break;
            case JSON_DECIMAL:
                buffeur = parseDECIMAL(json_str,&index);
                ary_value.value.array.listeOfValue[NumberOfElement] = buffeur;
                NumberOfElement++;
            break;
            case JSON_ERROR:
                ary_value.value.array.listeOfValue[NumberOfElement].value.integer = 0;
                ary_value.value.array.listeOfValue[NumberOfElement].type = JSON_ERROR;
                NumberOfElement++;
            break;
            case JSON_NULL:
                buffeur = parseNULL(json_str,&index);
                ary_value.value.array.listeOfValue[NumberOfElement] = buffeur;
                NumberOfElement++;
            break;
            case JSON_STRING:
                buffeur = parseSTRING(json_str,&index);
                ary_value.value.array.listeOfValue[NumberOfElement] = buffeur;
                NumberOfElement++;
            break;
            case JSON_ARRAY:
                buffeur = parseARRAY(json_str,&index);
                ary_value.value.array.listeOfValue[NumberOfElement] = buffeur;
                NumberOfElement++;
            break;
            case JSON_OBJECT:
                buffeur = parseOBJ(json_str,&index);
                ary_value.value.array.listeOfValue[NumberOfElement] = buffeur;
                NumberOfElement++;
            break;
        }

    }

    *position = index + 1;
    ary_value.value.array.nbOfElement = NumberOfElement;
    return ary_value;
}

JsonValue parseSTRING(char* json_str,size_t* position){
    size_t size = _strlen(json_str);
    JsonValue str_value;
    str_value.type = JSON_STRING;

    size_t start = *position;
    size_t end = 0;
    for(size_t index = *position+1;index<size;index++){
        if(json_str[index] == '"'){
            if(json_str[index-1] == '\\'){
                continue;
            }else{
                end = index;
                break;
            }
        }
    }

    _strcpybxy(&str_value.value.string,json_str,(int)start+1,(int)end-1);
    *position = end + 1;
    return str_value;
}

JsonValue parseNUMBER(char* json_str,size_t* position){
    char* number_str = (char*)calloc(64, sizeof(char));
    int number_int;
    size_t numberOfDigit = 0;
    size_t size = _strlen(json_str);
    int state;

    JsonValue nbr_value;
    nbr_value.type = JSON_NUMBER;

    for(size_t index=*position;index<size;index++){
        if((json_str[index] >= 48 && json_str[index] <= 57) || json_str[index] == '-'){
            numberOfDigit ++;
            number_str[numberOfDigit-1] = json_str[index];
        }else{
            break;
        }
    }
    number_str[numberOfDigit] = '\0';
    state = stringToInt(number_str,&number_int);
    if(state == 1){
        nbr_value.type = JSON_ERROR;
        nbr_value.value.integer = 0;
        return nbr_value;
    }
    nbr_value.value.integer = number_int;
    *position = *position + numberOfDigit;  
    free(number_str);
    return nbr_value;
}

JsonValue parseBOOL(char* json_str,size_t* position){
    JsonValue boo_value;
    boo_value.type = JSON_BOOL;

    if(json_str[*position] == 't'){
        boo_value.value.integer = 1;
        *position = *position + 4;
    }else if(json_str[*position] == 'f'){
        boo_value.value.integer = 0;
        *position = *position + 5;
    }else{
        boo_value.type = JSON_ERROR;
        boo_value.value.integer = 0;
    }

    return boo_value;
}

JsonValue parseNULL(char* json_str,size_t* position){
    JsonValue null_value;
    null_value.type = JSON_NULL;

    if(json_str[*position] == 'n'){
        null_value.value.integer = 0;
        *position = *position + 4;
    }else{
        null_value.type = JSON_ERROR;
    }

    return null_value;
}

JsonValue parseDECIMAL(char* json_str, size_t* position){
    char* decimal_str = (char*)calloc(64, sizeof(char));
    double decimal_double;
    size_t numberOfDigit = 0;
    size_t size = _strlen(json_str);
    int state;

    JsonValue dec_value;
    dec_value.type = JSON_DECIMAL;

    for(size_t index = *position; index < size; index++){
        if((json_str[index] >= 48 && json_str[index] <= 57) || 
           json_str[index] == '-' || json_str[index] == '.'){
            numberOfDigit++;
            decimal_str[numberOfDigit-1] = json_str[index];
        }else{
            break;
        }
    }
    decimal_str[numberOfDigit] = '\0';
    
    state = stringToDouble(decimal_str, &decimal_double);
    if(state == 1){
        dec_value.type = JSON_ERROR;
        dec_value.value.decimal = 0.0;
        free(decimal_str);
        return dec_value;
    }
    
    dec_value.value.decimal = decimal_double;
    *position = *position + numberOfDigit;  
    free(decimal_str);
    return dec_value;
}

JsonValue parseValue(char* json_str, size_t* position) {
    JsonType type = getType(json_str, *position);
    
    switch(type) {
        case JSON_STRING:
            return parseSTRING(json_str, position);
        case JSON_NUMBER:
            return parseNUMBER(json_str, position);
        case JSON_BOOL:
            return parseBOOL(json_str, position);
        case JSON_NULL:
            return parseNULL(json_str, position);
        case JSON_DECIMAL:
            return parseDECIMAL(json_str, position);
        case JSON_ARRAY:
            return parseARRAY(json_str, position);
        case JSON_OBJECT:
            return parseOBJ(json_str, position);
        default: {
            JsonValue error_value;
            error_value.type = JSON_ERROR;
            error_value.value.integer = 0;
            return error_value;
        }
    }
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

JsonType getType(char* json_str,size_t position){
    if(json_str == NULL){
        return JSON_ERROR;
    }
    switch(json_str[position]){
        case 't':
        case 'f':
            return JSON_BOOL;
        break;
        case '-':
            if(_strchr(json_str,'.') > 0){
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
            if(_strchr(&json_str[position], '.') > 0){
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

int _strcpy(char** dest,char* src){
    size_t size = _strlen(src) + 1;
    if (*dest == NULL) {
        *dest = malloc(size);
    } else {
        *dest = realloc(*dest, size);
    }
    if (*dest == NULL) {
        return 1;
    }

    for(size_t i = 0;i<size;i++){
        (*dest)[i] = src[i];
    }
    (*dest)[size-1] = '\0';
    return 0;
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

int stringToDouble(char* str, double* res){
    if(str == NULL){
        return 1;
    }
    
    char* endptr;
    *res = strtod(str, &endptr);
    
    // Vérifier si la conversion a échoué
    if(endptr == str){
        return 1;
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