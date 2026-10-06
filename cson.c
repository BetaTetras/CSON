#include <stdio.h>
#include <stdint.h> 
#include <stdlib.h>

#define BUFFER_DEFAULT_SIZE 64
#define DEBUG_VALUE 0
#define JSON_MAX_DEPTH 512  // Profondeur max d'imbrication ({ et [) acceptée par la validation

// Liste de type que peux prendre un JSON
typedef enum {
    JSON_NULL,          // NULL
    JSON_BOOL,          // Integer (0 ou 1)
    JSON_NUMBER,        // Integer
    JSON_STRING,        // char*
    JSON_ARRAY,         // JsonArray
    JSON_OBJECT,        // JsonObject
    JSON_DECIMAL,       // double
    JSON_EXPONENTIAL,   // char*
    JSON_ERROR          // erreur
} JsonType;

// Pré-déclaration
typedef struct JsonArray JsonArray;
typedef struct JsonObject JsonObject;

// Représentation d'une valeur JSON qui peut étre l'une de ces valeur -> Paire valeur type
typedef struct JsonValue{
    union {
        long long int integer; // ou Binaire
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

// Initialisée la transcription Text -> JsonValue
JsonValue initCson(char* path);
JsonValue initJsonValue(JsonType type);

// Utils
JsonType getType(char* json_str,size_t position);
long getSize(FILE* file);
int loadJson(char** dest,FILE* file);
int whitespaceCleaner(char* src,char** dest);
int isEscaped(char* str, size_t position) ;

// parsing
JsonValue parseOBJ(char* json_str, size_t* position);
JsonValue parseARRAY(char* json_str, size_t* position);
JsonValue parseSTRING(char* json_str,size_t* position);
JsonValue parseNUMBER(char* json_str,size_t* position);
JsonValue parseBOOL(char* json_str,size_t* position);
JsonValue parseNULL(char* json_str,size_t* position);
JsonValue parseDECIMAL(char* json_str, size_t* position);
JsonValue parseEXPONENTIAL(char* json_str,size_t* position);
JsonValue parseValue(char* json_str,size_t* position);

// validation (0 = valide, 1 = invalide)
int validateJson(char* json_str, size_t size, size_t* errorPosition);
int validateValue(char* json_str, size_t size, size_t* pos, int depth);
int validateObject(char* json_str, size_t size, size_t* pos, int depth);
int validateArray(char* json_str, size_t size, size_t* pos, int depth);
int validateString(char* json_str, size_t size, size_t* pos);
int validateNumber(char* json_str, size_t size, size_t* pos);
int validateLiteral(char* json_str, size_t size, size_t* pos, char* literal);
void skipWhitespace(char* json_str, size_t size, size_t* pos);
int isDigitAt(char* json_str, size_t size, size_t pos);

// modification
int addToArray(JsonArray* ary, JsonValue value,size_t index);
int addToObject(JsonObject* obj, char* key, JsonValue value,size_t index);
int removeToObject(JsonObject* obj, size_t index);
int removeToArray(JsonArray* ary, size_t index);
int modifyObject(JsonObject* obj,JsonPair paire,size_t index);
int modifyArray(JsonArray* ary,JsonValue value,size_t index);

//gets
JsonValue* getValueFromObject(JsonObject* obj,char* key);
int getIndexFromObject(JsonObject* obj,char* key);

// frees
void freeValue(JsonValue value);
void freeObject(JsonObject *obj);
void freeArray(JsonArray *ary);

//cpy
int cpyValue(JsonValue* dest,JsonValue* src);
int cpyObject(JsonObject* dest,JsonObject* src);
int cpyArray(JsonArray* dest,JsonArray* src);

// printf
void printfXtab(int x);
void printfValue(JsonValue value);
void printfArray(int* depth, JsonArray ary);
void printfObject(int* depth, JsonObject obj);

// fprintf
int fprintfValue(FILE* file,JsonValue value);
void fprintfObject(FILE* file, int* depth, JsonObject obj);
void fprintfArray(FILE* file, int* depth, JsonArray ary);

// _string.h
size_t _strlen(char* str);
int _strcmp(char* str1,char* str2);
int _strchr(char* str,char c);
int _strcpybxy(char **dest, char *src, int x, int y);
int _strchrxt(char * str,char x,int avoid);
int _strcpy(char** dest,char* src);

// conversion to str to x
int stringToInt(char* str,long long int* res);
int stringToDouble(char* str, double* res);

void debug(char* str);

JsonValue initCson(char* path){
    JsonValue json = initJsonValue(JSON_ERROR);

    FILE* file = fopen(path, "rb");
    if(file == NULL){
        printf("Error : impossible to open JSON file\n");
        return json;
    }

    char* json_str = NULL;
    if(loadJson(&json_str, file) != 0) {
        printf("Error : impossible to load the JSON file (to big?)\n");
        fclose(file);
        free(json_str);
        return json;
    }
    fclose(file);

    size_t size = _strlen(json_str);
    if(size == (size_t)-1){
        printf("Error : a problem as occure during the calculation of the size\n");
        return json;
    }

    size_t errorPosition = 0;
    if(validateJson(json_str, size, &errorPosition) != 0){
        size_t line = 1;
        size_t column = 1;
        for(size_t i = 0; i < errorPosition; i++){
            if(json_str[i] == '\n'){
                line++;
                column = 1;
            }else{
                column++;
            }
        }
        printf("Error : invalid JSON at line %zu, column %zu\n", line, column);
        free(json_str);
        return json;
    }

    whitespaceCleaner(json_str, &json_str);

    size_t pos = 0;
    json = parseValue(json_str, &pos);

    free(json_str);
    return json;
}

JsonValue initJsonValue(JsonType type){
    JsonValue v = {0};
    v.type = type;
    return v;
}

//////////////////////////////////////////// printf function ////////////////////////////////////////////

void printfXtab(int x){
    for(int i=0;i<x;i++){
        printf("  ");
    }
}

void printfValue(JsonValue value){
    int depth = 0;
    switch(value.type){
        case JSON_ARRAY:
            printfArray(&depth,*value.value.array);
        break;
        case JSON_BOOL:{
            if(value.value.integer == 1){
                printf("true");
            }else if(value.value.integer == 0){
                printf("false");
            }else{
                printf("Boolean error");
            }
            break;
        }
        
        case JSON_DECIMAL:
            printf("%f",value.value.decimal);
        break;
        case JSON_ERROR:
            printf("ERROR\n");
        break;
        case JSON_EXPONENTIAL:
            printf("%s",value.value.string);
        break;
        case JSON_NULL:
            printf("null");
        break;
        case JSON_NUMBER:
            printf("%lld",value.value.integer);
        break;
        case JSON_OBJECT:
            printfObject(&depth,*value.value.object);
        break;
        case JSON_STRING:
            if(value.value.string == NULL) printf("null");
            else printf("\"%s\"", value.value.string);
        break;
        default:
            printf("error print");
    }
    return;
}

void printfArray(int* depth,JsonArray ary){
    size_t numberOfElement = (size_t)ary.nbOfElement;
    printf("[");
    if(ary.nbOfElement == 0) {
        printf("]");
        return;
    }
    for(size_t i=0;i<numberOfElement;i++){
        switch(ary.listeOfValue[i].type){
            case JSON_OBJECT:
                printfObject(depth,*ary.listeOfValue[i].value.object);
            break;
            case JSON_ARRAY:
                printfArray(depth,*ary.listeOfValue[i].value.array);
            break;
            case JSON_BOOL:{
                if(ary.listeOfValue[i].value.integer == 1){
                    printf("true");
                }else if(ary.listeOfValue[i].value.integer == 0){
                    printf("false");
                }else{
                    printf("Boolean error");
                }
            }
            break;
            case JSON_DECIMAL:
                printf("%f",ary.listeOfValue[i].value.decimal);
            break;
            case JSON_ERROR:
                printf("ERROR");
            break;
            case JSON_EXPONENTIAL:{
                if(ary.listeOfValue[i].value.string == NULL){
                    printf("null");
                }else{
                    printf("%s",ary.listeOfValue[i].value.string);
                }
            }
            break;
            case JSON_NULL:
                printf("null");
            break;
            case JSON_NUMBER:
                printf("%lld",ary.listeOfValue[i].value.integer);
            break;
            case JSON_STRING:{
                if(ary.listeOfValue[i].value.string == NULL){
                    printf("null");
                }else{
                    printf("\"%s\"",ary.listeOfValue[i].value.string);
                }
            }
            break;
            default:
                printf("Error print");
        }
        if(i < ary.nbOfElement - 1) {
            printf(",");
        }
    }
    printf("]");
}

void printfObject(int* depth,JsonObject obj){
    size_t numberOfOBJ = (size_t)obj.nbOfElement;
    printf("{\n");

    if(obj.nbOfElement == 0) {
        printfXtab(*depth);
        printf("}");
        return;
    }
    (*depth)++;
    for(size_t i=0;i<numberOfOBJ;i++){
        
        printfXtab(*depth);
        printf("\"%s\": ", obj.listeOfPair[i].key);
        switch(obj.listeOfPair[i].value.type){
            case JSON_OBJECT:
                printfObject(depth, *obj.listeOfPair[i].value.value.object);
            break;
            case JSON_ARRAY:
                printfArray(depth, *obj.listeOfPair[i].value.value.array);
            break;
            case JSON_BOOL:{
                if(obj.listeOfPair[i].value.value.integer == 1){
                    printf("true");
                }else if(obj.listeOfPair[i].value.value.integer == 0){
                    printf("false");
                }else{
                    printf("Boolean error");
                }
            }
            break;
            case JSON_DECIMAL:
                printf("%f",obj.listeOfPair[i].value.value.decimal);
            break;
            case JSON_ERROR:
                printf("ERROR");
            break;
            case JSON_EXPONENTIAL:{
                if(obj.listeOfPair[i].value.value.string == NULL){
                    printf("null");
                }else{
                    printf("%s",obj.listeOfPair[i].value.value.string);
                }
            }
            break;
            case JSON_NULL:
                printf("null");
            break;
            case JSON_NUMBER:
                printf("%lld",obj.listeOfPair[i].value.value.integer);
            break;
            case JSON_STRING:{
                if(obj.listeOfPair[i].value.value.string == NULL){
                    printf("null");
                }else{
                    printf("\"%s\"",obj.listeOfPair[i].value.value.string);
                }
            }
            break;
            default:
                printf("Error reading");
        }
        if(i < obj.nbOfElement - 1) {
            printf(",");
        }
        printf("\n");
    }
    (*depth)--;

    printfXtab(*depth);
    printf("}\n");

}

//////////////////////////////////////////// fprintf function ////////////////////////////////////////////

void fprintfXtab(FILE* file, int x){
    for(int i=0;i<x;i++){
        fprintf(file,"  ");
    }
}

int fprintfValue(FILE* file, JsonValue value){
    int depth = 0;
    switch(value.type){
        case JSON_ARRAY:
            fprintfArray(file,&depth,*value.value.array);
        break;
        case JSON_BOOL:{
            if(value.value.integer == 1){
                fprintf(file,"true");
            }else if(value.value.integer == 0){
                fprintf(file,"false");
            }else{
                fprintf(file,"Boolean error");
            }
            break;
        }
        
        case JSON_DECIMAL:
            fprintf(file,"%f",value.value.decimal);
        break;
        case JSON_ERROR:
            fprintf(file,"ERROR");
        break;
        case JSON_EXPONENTIAL:
            fprintf(file,"%s",value.value.string);
        break;
        case JSON_NULL:
            fprintf(file,"null");
        break;
        case JSON_NUMBER:
            fprintf(file,"%lld",value.value.integer);
        break;
        case JSON_OBJECT:
            fprintfObject(file,&depth,*value.value.object);
        break;
        case JSON_STRING:{
            if(value.value.string == NULL){
                fprintf(file,"null");
            }else{
                fprintf(file,"\"%s\"",value.value.string);
            }
            break;
        }
        default:
            fprintf(file,"error print");
    }
    return 0;
}

void fprintfObject(FILE* file, int* depth, JsonObject obj){
    size_t numberOfOBJ = (size_t)obj.nbOfElement;
    fprintf(file,"{\n");

    if(obj.nbOfElement == 0) {
        fprintfXtab(file,*depth);
        fprintf(file,"}");
        return;
    }
    (*depth)++;
    for(size_t i=0;i<numberOfOBJ;i++){
        
        fprintfXtab(file,*depth);
        fprintf(file,"\"%s\": ", obj.listeOfPair[i].key);
        switch(obj.listeOfPair[i].value.type){
            case JSON_OBJECT:
                fprintfObject(file, depth, *obj.listeOfPair[i].value.value.object);
            break;
            case JSON_ARRAY:
                fprintfArray(file, depth, *obj.listeOfPair[i].value.value.array);
            break;
            case JSON_BOOL:{
                if(obj.listeOfPair[i].value.value.integer == 1){
                    fprintf(file,"true");
                }else if(obj.listeOfPair[i].value.value.integer == 0){
                    fprintf(file,"false");
                }else{
                    fprintf(file,"Boolean error");
                }
            }
            break;
            case JSON_DECIMAL:
                fprintf(file,"%f",obj.listeOfPair[i].value.value.decimal);
            break;
            case JSON_ERROR:
                fprintf(file,"ERROR");
            break;
            case JSON_EXPONENTIAL:{
                if(obj.listeOfPair[i].value.value.string == NULL){
                    fprintf(file,"null");
                }else{
                    fprintf(file,"%s",obj.listeOfPair[i].value.value.string);
                }
            }
            break;
            case JSON_NULL:
                fprintf(file,"null");
            break;
            case JSON_NUMBER:
                fprintf(file,"%lld",obj.listeOfPair[i].value.value.integer);
            break;
            case JSON_STRING:{
                if(obj.listeOfPair[i].value.value.string == NULL){
                    fprintf(file,"null");
                }else{
                    fprintf(file,"\"%s\"",obj.listeOfPair[i].value.value.string);
                }
            }
            break;
            default:
                fprintf(file,"Error reading");
        }
        if(i < obj.nbOfElement - 1) {
            fprintf(file,",");
        }
        fprintf(file,"\n");
    }
    (*depth)--;

    fprintfXtab(file,*depth);
    fprintf(file,"}\n");
}

void fprintfArray(FILE* file, int* depth, JsonArray ary){
    size_t numberOfElement = (size_t)ary.nbOfElement;
    fprintf(file,"[");
    if(ary.nbOfElement == 0) {
        fprintf(file,"]");
        return;
    }
    for(size_t i=0;i<numberOfElement;i++){
        switch(ary.listeOfValue[i].type){
            case JSON_OBJECT:
                fprintfObject(file,depth,*ary.listeOfValue[i].value.object);
            break;
            case JSON_ARRAY:
                fprintfArray(file,depth,*ary.listeOfValue[i].value.array);
            break;
            case JSON_BOOL:{
                if(ary.listeOfValue[i].value.integer == 1){
                    fprintf(file,"true");
                }else if(ary.listeOfValue[i].value.integer == 0){
                    fprintf(file,"false");
                }else{
                    fprintf(file,"Boolean error");
                }
            }
            break;
            case JSON_DECIMAL:
                fprintf(file,"%f",ary.listeOfValue[i].value.decimal);
            break;
            case JSON_ERROR:
                fprintf(file,"ERROR");
            break;
            case JSON_EXPONENTIAL:{
                if(ary.listeOfValue[i].value.string == NULL){
                    fprintf(file,"null");
                }else{
                    fprintf(file,"%s",ary.listeOfValue[i].value.string);
                }
            }
            break;
            case JSON_NULL:
                fprintf(file,"null");
            break;
            case JSON_NUMBER:
                fprintf(file,"%lld",ary.listeOfValue[i].value.integer);
            break;
            case JSON_STRING:{
                if(ary.listeOfValue[i].value.string == NULL){
                    fprintf(file,"null");
                }else{
                    fprintf(file,"\"%s\"",ary.listeOfValue[i].value.string);
                }
            }
            break;
            default:
                fprintf(file,"Error print");
        }
        if(i < ary.nbOfElement - 1) {
            fprintf(file,",");
        }
    }
    fprintf(file,"]");
}

//////////////////////////////////////////// new function ////////////////////////////////////////////

int newJsonObject(JsonValue* dest){
    if(dest == NULL){
        return 1;
    }
    *dest = initJsonValue(JSON_OBJECT);
    dest->value.object =(JsonObject*)malloc(sizeof(JsonObject));
    if(dest->value.object == NULL){
        dest->type = JSON_ERROR;
        return 1;
    }
    dest->value.object->nbOfElement = 0;
    dest->value.object->listeOfPair = NULL;

    return 0;
}

int newJsonArray(JsonValue* dest){
    if(dest == NULL){
        return 1;
    }
    *dest = initJsonValue(JSON_ARRAY);
    dest->value.array = (JsonArray*)malloc(sizeof(JsonArray));
    if(dest->value.array == NULL){
        dest->type = JSON_ERROR;
        return 1;
    }
    dest->value.array->nbOfElement = 0;
    dest->value.array->listeOfValue = NULL;

    return 0;
}

int newJsonString(JsonValue* dest,char* str){
    if(dest == NULL){
        return 1;
    }
    
    *dest = initJsonValue(JSON_STRING);
    dest->value.string = NULL;
    int state;
    
    state = _strcpy(&dest->value.string,str);
    if(state){
        dest->type = JSON_ERROR;
        return 1;
    }
    
    return 0;
}

int newJsonNull(JsonValue* dest){
    if(dest == NULL){
        return 1;
    }
    
    *dest = initJsonValue(JSON_NULL);
    dest->value.integer = 0;
    
    return 0;
}

int newJsonBool(JsonValue* dest,int _bool){
    if(dest == NULL){
        return 1;
    }
    if(_bool > 1 || _bool < 0){
        return 1;
    }
    
    *dest = initJsonValue(JSON_BOOL);
    dest->value.integer = _bool;
    
    return 0;
}

int newJsonNumber(JsonValue* dest,long long int _int){
    if(dest == NULL){
        return 1;
    }
    
    *dest = initJsonValue(JSON_NUMBER);
    dest->value.integer = _int;
    
    return 0;
}

int newJsonDecimal(JsonValue* dest,double _dec){
    if(dest == NULL){
        return 1;
    }
    
    *dest = initJsonValue(JSON_DECIMAL);
    dest->value.decimal = _dec;
    
    return 0;
}

int newJsonExponencial(JsonValue* dest,char* exp){
    if(dest == NULL){
        return 1;
    }
    
    *dest = initJsonValue(JSON_EXPONENTIAL);
    dest->value.string = NULL;
    int state;
    
    state = _strcpy(&dest->value.string,exp);
    if(state){
        dest->type = JSON_ERROR;
        return 1;
    }
    
    return 0;
}



//////////////////////////////////////////// add function ////////////////////////////////////////////

int addToObject(JsonObject* obj, char* key, JsonValue value,size_t index){
    if(obj == NULL || key == NULL || obj->nbOfElement<index){
        return 1;
    }

    if(getIndexFromObject(obj,key) != -1){
        return 1;
    }

    char* cpy_key = NULL;
    if(_strcpy(&cpy_key, key)){
        return 1;
    }
    JsonValue cpy_value ;
    if(cpyValue(&cpy_value, &value)){
        free(cpy_key);
        return 1;
    }

    JsonPair* newListe = (JsonPair*)realloc(obj->listeOfPair, (obj->nbOfElement + 1) * sizeof(JsonPair));
    if(newListe == NULL){
        free(cpy_key);
        freeValue(cpy_value);
        return 1;
    }
    obj->listeOfPair = newListe;

    for(size_t i = obj->nbOfElement; i > index; i--){
        newListe[i] = newListe[i-1];
    }

    newListe[index].key = cpy_key;
    newListe[index].value = cpy_value;
    obj->nbOfElement++;

    return 0;
}

int addToArray(JsonArray* ary, JsonValue value, size_t index){
    if(ary == NULL || ary->nbOfElement < index){
        return 1;
    }

    JsonValue cpy_value;
    if(cpyValue(&cpy_value, &value)){
        return 1;
    }

    JsonValue* newListe = (JsonValue*)realloc(ary->listeOfValue, (ary->nbOfElement + 1) * sizeof(JsonValue));
    if(newListe == NULL){ 
        freeValue(cpy_value);
        return 1;
    }
    ary->listeOfValue = newListe;

    for(size_t i = ary->nbOfElement; i > index; i--){
        newListe[i] = newListe[i-1];
    }
    newListe[index] = cpy_value;
    ary->nbOfElement++;

    return 0;
}

//////////////////////////////////////////// remove function ////////////////////////////////////////////

int removeToObject(JsonObject* obj, size_t index){
    if(obj == NULL || index >= obj->nbOfElement){
        return 1;
    }

    free(obj->listeOfPair[index].key);
    freeValue(obj->listeOfPair[index].value);

    for(size_t i = index; i < obj->nbOfElement - 1; i++){
        obj->listeOfPair[i] = obj->listeOfPair[i + 1];
    }

    obj->nbOfElement--;

    if(obj->nbOfElement == 0){
        free(obj->listeOfPair);
        obj->listeOfPair = NULL;
    }else{
        JsonPair* newListe = (JsonPair*)realloc(obj->listeOfPair, obj->nbOfElement * sizeof(JsonPair));
        if(newListe == NULL){
            return 1;
        }
        obj->listeOfPair = newListe;
    }

    return 0;
}

int removeToArray(JsonArray* ary, size_t index){
    if(ary == NULL || index >= ary->nbOfElement){
        return 1;
    }

    freeValue(ary->listeOfValue[index]);

    for(size_t i = index; i < ary->nbOfElement - 1; i++){
        ary->listeOfValue[i] = ary->listeOfValue[i + 1];
    }

    ary->nbOfElement--;

    if(ary->nbOfElement == 0){
        free(ary->listeOfValue);
        ary->listeOfValue = NULL;
    }else{
        JsonValue* newListe = (JsonValue*)realloc(ary->listeOfValue, ary->nbOfElement * sizeof(JsonValue));
        if(newListe == NULL){
            return 1;
        }
        ary->listeOfValue = newListe;
    }

    return 0;
}

//////////////////////////////////////////// modify function ////////////////////////////////////////////

int modifyObject(JsonObject* obj,JsonPair paire,size_t index){
    if(obj == NULL || index >= obj->nbOfElement){
        return 1;
    }
    size_t paireLen = _strlen(paire.key);
    if(paireLen == (size_t)-1){
        return 1;
    }

    JsonPair newPair;
    newPair.key = NULL
    if(_strcpy(&newPair.key,paire.key)){
        return 1;
    }
    if(cpyValue(&newPair.value,&paire.value)){
        free(newPair.key);
        return 1;
    }

    free(obj->listeOfPair[index].key);
    freeValue(obj->listeOfPair[index].value);
    obj->listeOfPair[index] = newPair;

    return 0;
}

int modifyArray(JsonArray* ary, JsonValue value, size_t index){
    if(ary == NULL || index >= (size_t)ary->nbOfElement){
        return 1;
    }

    JsonValue newValue;
    if(cpyValue(&newValue, &value)){
        return 1;
    }

    freeValue(ary->listeOfValue[index]);
    ary->listeOfValue[index] = newValue;

    return 0;
}
//////////////////////////////////////////// get function ////////////////////////////////////////////

JsonValue* getValueFromObject(JsonObject* obj,char* key){
    for(size_t i=0;i<obj->nbOfElement;i++){
        if(_strcmp(obj->listeOfPair[i].key,key) == 0){
            return &obj->listeOfPair[i].value;
        }
    }

    return NULL;
}

int getIndexFromObject(JsonObject* obj,char* key){
    for(size_t i=0;i<obj->nbOfElement;i++){
        if(_strcmp(obj->listeOfPair[i].key,key) == 0){
            return (int)i;
        }
    }

    return -1;
}

//////////////////////////////////////////// validation function ////////////////////////////////////////////
// Vérifie que le texte respecte la grammaire JSON (RFC 8259) avant le parsing.
// Les fonctions parseXXX supposent un JSON valide : c'est cette étape qui les protège
// des lectures hors limites et des valeurs mal formées.
// En cas d'erreur, *pos reste sur le caractère fautif.

int validateJson(char* json_str, size_t size, size_t* errorPosition){
    size_t pos = 0;
    int state = 1;

    if(json_str != NULL){
        skipWhitespace(json_str, size, &pos);
        state = validateValue(json_str, size, &pos, 0);
        if(state == 0){
            skipWhitespace(json_str, size, &pos);
            // Il ne doit rien rester après la valeur racine ("{"a":1}xyz" est refusé)
            if(pos != size){
                state = 1;
            }
        }
    }

    if(errorPosition != NULL){
        *errorPosition = pos;
    }
    return state;
}

int validateValue(char* json_str, size_t size, size_t* pos, int depth){
    if(*pos >= size){
        return 1;
    }
    switch(json_str[*pos]){
        case '{':
            return validateObject(json_str, size, pos, depth + 1);
        case '[':
            return validateArray(json_str, size, pos, depth + 1);
        case '"':
            return validateString(json_str, size, pos);
        case 't':
            return validateLiteral(json_str, size, pos, "true");
        case 'f':
            return validateLiteral(json_str, size, pos, "false");
        case 'n':
            return validateLiteral(json_str, size, pos, "null");
        default:
            return validateNumber(json_str, size, pos);
    }
}

int validateObject(char* json_str, size_t size, size_t* pos, int depth){
    if(depth > JSON_MAX_DEPTH){
        return 1;
    }
    (*pos)++; // '{'
    skipWhitespace(json_str, size, pos);
    if(*pos < size && json_str[*pos] == '}'){
        (*pos)++;
        return 0;
    }

    while(*pos < size){
        // clé
        if(json_str[*pos] != '"' || validateString(json_str, size, pos)){
            return 1;
        }
        skipWhitespace(json_str, size, pos);
        if(*pos >= size || json_str[*pos] != ':'){
            return 1;
        }
        (*pos)++;
        skipWhitespace(json_str, size, pos);

        // valeur
        if(validateValue(json_str, size, pos, depth)){
            return 1;
        }
        skipWhitespace(json_str, size, pos);
        if(*pos >= size){
            return 1;
        }
        if(json_str[*pos] == '}'){
            (*pos)++;
            return 0;
        }
        if(json_str[*pos] != ','){
            return 1;
        }
        (*pos)++;
        // Après une virgule il faut une nouvelle clé : "{"a":1,}" est refusé
        skipWhitespace(json_str, size, pos);
    }
    return 1; // pas de '}' fermante
}

int validateArray(char* json_str, size_t size, size_t* pos, int depth){
    if(depth > JSON_MAX_DEPTH){
        return 1;
    }
    (*pos)++; // '['
    skipWhitespace(json_str, size, pos);
    if(*pos < size && json_str[*pos] == ']'){
        (*pos)++;
        return 0;
    }

    while(*pos < size){
        if(validateValue(json_str, size, pos, depth)){
            return 1;
        }
        skipWhitespace(json_str, size, pos);
        if(*pos >= size){
            return 1;
        }
        if(json_str[*pos] == ']'){
            (*pos)++;
            return 0;
        }
        if(json_str[*pos] != ','){
            return 1;
        }
        (*pos)++;
        // Après une virgule il faut une nouvelle valeur : "[1,]" est refusé
        skipWhitespace(json_str, size, pos);
    }
    return 1; // pas de ']' fermant
}

int validateString(char* json_str, size_t size, size_t* pos){
    (*pos)++; // '"' ouvrant

    while(*pos < size){
        unsigned char c = (unsigned char)json_str[*pos];
        if(c == '"'){
            (*pos)++;
            return 0;
        }
        if(c < 0x20){
            // Tab, retour à la ligne... doivent être échappés (\t, \n) dans une string JSON
            return 1;
        }
        if(c == '\\'){
            (*pos)++;
            if(*pos >= size){
                return 1;
            }
            switch(json_str[*pos]){
                case '"': case '\\': case '/':
                case 'b': case 'f': case 'n': case 'r': case 't':
                break;
                case 'u':
                    // \uXXXX : exactement 4 chiffres hexadécimaux
                    for(int i = 0; i < 4; i++){
                        (*pos)++;
                        if(*pos >= size){
                            return 1;
                        }
                        char h = json_str[*pos];
                        if(!((h >= '0' && h <= '9') || (h >= 'a' && h <= 'f') || (h >= 'A' && h <= 'F'))){
                            return 1;
                        }
                    }
                break;
                default:
                    return 1;
            }
        }
        (*pos)++;
    }
    return 1; // pas de '"' fermant
}

int validateNumber(char* json_str, size_t size, size_t* pos){
    // [-] partieEntière [.chiffres] [e|E [+|-] chiffres]
    if(*pos < size && json_str[*pos] == '-'){
        (*pos)++;
    }

    // Partie entière : "0" seul, ou 1-9 suivi de chiffres ("01" est refusé)
    if(!isDigitAt(json_str, size, *pos)){
        return 1;
    }
    if(json_str[*pos] == '0'){
        (*pos)++;
    }else{
        while(isDigitAt(json_str, size, *pos)){
            (*pos)++;
        }
    }

    // Partie décimale : au moins un chiffre après le '.'
    if(*pos < size && json_str[*pos] == '.'){
        (*pos)++;
        if(!isDigitAt(json_str, size, *pos)){
            return 1;
        }
        while(isDigitAt(json_str, size, *pos)){
            (*pos)++;
        }
    }

    // Exposant : un seul, signe optionnel, au moins un chiffre
    if(*pos < size && (json_str[*pos] == 'e' || json_str[*pos] == 'E')){
        (*pos)++;
        if(*pos < size && (json_str[*pos] == '+' || json_str[*pos] == '-')){
            (*pos)++;
        }
        if(!isDigitAt(json_str, size, *pos)){
            return 1;
        }
        while(isDigitAt(json_str, size, *pos)){
            (*pos)++;
        }
    }

    // Ce qui suit (',' '}' ']' ou fin) est vérifié par l'appelant : "1e5e3" ou "1.2.3" sont refusés
    return 0;
}

int validateLiteral(char* json_str, size_t size, size_t* pos, char* literal){
    for(size_t i = 0; literal[i] != '\0'; i++){
        if(*pos >= size || json_str[*pos] != literal[i]){
            return 1;
        }
        (*pos)++;
    }
    return 0;
}

void skipWhitespace(char* json_str, size_t size, size_t* pos){
    while(*pos < size && (json_str[*pos] == ' ' || json_str[*pos] == '\n' ||
                          json_str[*pos] == '\t' || json_str[*pos] == '\r')){
        (*pos)++;
    }
}

int isDigitAt(char* json_str, size_t size, size_t pos){
    return pos < size && json_str[pos] >= '0' && json_str[pos] <= '9';
}

//////////////////////////////////////////// parse function ////////////////////////////////////////////

JsonValue parseOBJ(char* json_str, size_t* position){
    debug("parseOBJ");
    JsonValue obj_value = initJsonValue(JSON_ERROR);

    size_t capacity = 10;
    obj_value.type = JSON_OBJECT;
    obj_value.value.object = (JsonObject*)malloc(sizeof(JsonObject));
    if(obj_value.value.object == NULL){
        obj_value = initJsonValue(JSON_ERROR);
        return obj_value;
    }
    obj_value.value.object->nbOfElement = 0;
    obj_value.value.object->listeOfPair = (JsonPair*)calloc(10, sizeof(JsonPair));
    if(obj_value.value.object->listeOfPair == NULL){
        free(obj_value.value.object);
        obj_value = initJsonValue(JSON_ERROR);
        return obj_value;
    }

    JsonType targeted_type;
    JsonPair buffeur_pair;
    JsonValue buffeur_value;
    buffeur_value.value.string = NULL;

    int NumberOfElement = 0;
    size_t index;
    for(index = *position+1; json_str[index] != '\0';){
        if(json_str[index] == '}'){
            break;
        }else if(json_str[index] == ','){
            index++;
            continue;
        }

        if(capacity <= NumberOfElement){
            capacity *= 2;
            JsonPair* tmp = (JsonPair*)realloc(obj_value.value.object->listeOfPair, capacity * sizeof(JsonPair));
            if(tmp == NULL){
                obj_value.value.object->nbOfElement = NumberOfElement;
                freeObject(obj_value.value.object);
                obj_value = initJsonValue(JSON_ERROR);
                return obj_value;
            }
            obj_value.value.object->listeOfPair = tmp;
        }

        size_t before = index;
        buffeur_value = parseSTRING(json_str, &index);
        // La clé n'est pas une string valide -> on abandonne l'objet
        if(index == before){
            obj_value.value.object->nbOfElement = NumberOfElement;
            freeObject(obj_value.value.object);
            obj_value = initJsonValue(JSON_ERROR);
            return obj_value;
        }
        buffeur_pair.key = NULL;
        _strcpy(&buffeur_pair.key, buffeur_value.value.string);
        freeValue(buffeur_value);
        index++;

        before = index;
        targeted_type = getType(json_str, index);
        switch(targeted_type){
            case JSON_NUMBER:
                buffeur_pair.value = parseNUMBER(json_str, &index);
                obj_value.value.object->listeOfPair[NumberOfElement] = buffeur_pair;
                NumberOfElement++;
            break;
            case JSON_BOOL:
                buffeur_pair.value = parseBOOL(json_str, &index);
                obj_value.value.object->listeOfPair[NumberOfElement] = buffeur_pair;
                NumberOfElement++;
            break;
            case JSON_DECIMAL:
                buffeur_pair.value = parseDECIMAL(json_str, &index);
                obj_value.value.object->listeOfPair[NumberOfElement] = buffeur_pair;
                NumberOfElement++;
            break;
            case JSON_ERROR: {
                // Avancer jusqu'au prochain délimiteur
                while(json_str[index] != '\0' && json_str[index] != ',' && json_str[index] != '}' && json_str[index] != ']') {
                    index++;
                }
                
                JsonValue errorValue = initJsonValue(JSON_ERROR);
                
                buffeur_pair.value = errorValue;
                obj_value.value.object->listeOfPair[NumberOfElement] = buffeur_pair;
                NumberOfElement++;
                
                // Avancer après la virgule si présente
                if(json_str[index] == ',') {
                    index++;
                }
                break;
            }
            case JSON_NULL:
                buffeur_pair.value = parseNULL(json_str, &index);
                obj_value.value.object->listeOfPair[NumberOfElement] = buffeur_pair;
                NumberOfElement++;
            break;
            case JSON_STRING:
                buffeur_pair.value = parseSTRING(json_str, &index);
                obj_value.value.object->listeOfPair[NumberOfElement] = buffeur_pair;
                NumberOfElement++;
            break;
            case JSON_ARRAY:
                buffeur_pair.value = parseARRAY(json_str, &index);
                obj_value.value.object->listeOfPair[NumberOfElement] = buffeur_pair;
                NumberOfElement++;
            break;
            case JSON_OBJECT:
                buffeur_pair.value = parseOBJ(json_str, &index);
                obj_value.value.object->listeOfPair[NumberOfElement] = buffeur_pair;
                NumberOfElement++;
            break;
            case JSON_EXPONENTIAL:
                buffeur_pair.value = parseEXPONENTIAL(json_str, &index);  // ← index avance ici
                obj_value.value.object->listeOfPair[NumberOfElement] = buffeur_pair;
                NumberOfElement++;
            break;
        }

        // Le parseur de la valeur n'a pas avancé -> on abandonne l'objet
        if(index == before){
            obj_value.value.object->nbOfElement = NumberOfElement;
            freeObject(obj_value.value.object);
            obj_value = initJsonValue(JSON_ERROR);
            return obj_value;
        }
    }
    
    *position = index + 1;
    obj_value.value.object->nbOfElement = NumberOfElement;
    return obj_value;
}

JsonValue parseARRAY(char* json_str, size_t* position){
    debug("parseARRAY");
    JsonValue ary_value = initJsonValue(JSON_ERROR);

    size_t capacity = 10;
    ary_value.type = JSON_ARRAY;
    ary_value.value.array = (JsonArray*)malloc(sizeof(JsonArray));
    if(ary_value.value.array == NULL){
        ary_value = initJsonValue(JSON_ERROR);
        return ary_value;
    }
    ary_value.value.array->nbOfElement = 0;
    ary_value.value.array->listeOfValue = (JsonValue*)calloc(capacity, sizeof(JsonValue));
    if(ary_value.value.array->listeOfValue == NULL){
        free(ary_value.value.array);
        ary_value = initJsonValue(JSON_ERROR);
        return ary_value;
    }

    
    JsonType targeted_type;
    JsonValue buffeur;

    int NumberOfElement = 0;
    size_t index;
    for( index = *position+1; json_str[index] != '\0';){
        if(json_str[index] == ']'){
            break;
        }else if(json_str[index] == ','){
            index++;
            continue;
        }

        if(capacity <= NumberOfElement){
            capacity *= 2;
            JsonValue* tmp = (JsonValue*)realloc(ary_value.value.array->listeOfValue, capacity * sizeof(JsonValue));
            if(tmp == NULL){
                ary_value.value.array->nbOfElement = NumberOfElement;
                freeArray(ary_value.value.array);
                ary_value = initJsonValue(JSON_ERROR);
                return ary_value;
            }
            ary_value.value.array->listeOfValue = tmp;
        }

        size_t before = index;
        targeted_type = getType(json_str, index);
        switch(targeted_type){
            case JSON_NUMBER:
                buffeur = parseNUMBER(json_str, &index);
                ary_value.value.array->listeOfValue[NumberOfElement] = buffeur;
                NumberOfElement++;
                
            break;
            case JSON_BOOL:
                buffeur = parseBOOL(json_str, &index);
                ary_value.value.array->listeOfValue[NumberOfElement] = buffeur;
                NumberOfElement++;
            break;
            case JSON_DECIMAL:
                buffeur = parseDECIMAL(json_str, &index);
                ary_value.value.array->listeOfValue[NumberOfElement] = buffeur;
                NumberOfElement++;
            break;
            case JSON_ERROR:{
                while(json_str[index] != '\0' && json_str[index] != ',' && json_str[index] != '}' && json_str[index] != ']') {
                    index++;
                }

                buffeur = initJsonValue(JSON_ERROR);

                ary_value.value.array->listeOfValue[NumberOfElement] = buffeur;
                NumberOfElement++;

                if(json_str[index] == ',') {
                    index++;
                }
                break;
            }
            break;
            case JSON_NULL:
                buffeur = parseNULL(json_str, &index);
                ary_value.value.array->listeOfValue[NumberOfElement] = buffeur;
                NumberOfElement++;
            break;
            case JSON_STRING:
                buffeur = parseSTRING(json_str, &index);
                ary_value.value.array->listeOfValue[NumberOfElement] = buffeur;
                NumberOfElement++;
            break;
            case JSON_ARRAY:
                buffeur = parseARRAY(json_str, &index);
                ary_value.value.array->listeOfValue[NumberOfElement] = buffeur;
                NumberOfElement++;
            break;
            case JSON_OBJECT:
                buffeur = parseOBJ(json_str, &index);
                ary_value.value.array->listeOfValue[NumberOfElement] = buffeur;
                NumberOfElement++;
            break;
            case JSON_EXPONENTIAL:
                buffeur = parseEXPONENTIAL(json_str, &index);  // ← index avance ici
                ary_value.value.array->listeOfValue[NumberOfElement] = buffeur;
                NumberOfElement++;
            break;
        }

        // Le parseur n'a pas avancé -> sans ça on boucle à l'infini, on abandonne le tableau
        if(index == before){
            ary_value.value.array->nbOfElement = NumberOfElement;
            freeArray(ary_value.value.array);
            ary_value = initJsonValue(JSON_ERROR);
            return ary_value;
        }
    }

    *position = index + 1;
    ary_value.value.array->nbOfElement = NumberOfElement;
    return ary_value;
}

JsonValue parseSTRING(char* json_str,size_t* position){
    debug("parseSTRING");
    JsonValue str_value = initJsonValue(JSON_ERROR);
    str_value.type = JSON_STRING;
    str_value.value.string = NULL;

    size_t start = *position;

    if(json_str[start] != '"') {
        str_value.type = JSON_ERROR;
        return str_value;
    }

    size_t end = start;

    for(size_t index = start + 1; json_str[index] != '\0'; index++){
        if(json_str[index] == '"' && !isEscaped(json_str, index)){
            end = index;
            break;
        }
    }

    if(end == start){
        str_value.type = JSON_ERROR;
        return str_value;
    }

    if(end == start + 1) {
        _strcpy(&str_value.value.string, "");
    }else if(end > start) {
        _strcpybxy(&str_value.value.string, json_str, (int)start+1, (int)end-1);
    }

    *position = end + 1;
    return str_value;
}

JsonValue parseEXPONENTIAL(char* json_str,size_t* position){
    debug("parseEXPONENTIAL");
    JsonValue exp_value = initJsonValue(JSON_ERROR);

    exp_value.type = JSON_EXPONENTIAL;
    exp_value.value.string = (char*)calloc(64,sizeof(char));
    if(exp_value.value.string == NULL){
        exp_value = initJsonValue(JSON_ERROR);
        return exp_value;
    }
    int numberOfDigit = 0;
    int numberOfDot = 0;

    for(size_t i = *position;json_str[i] != '\0';i++){
        if(numberOfDot > 1 || numberOfDigit >= 63){
            free(exp_value.value.string);
            exp_value = initJsonValue(JSON_ERROR);
            return exp_value;
        }
        if((json_str[i] >= '0' && json_str[i] <= '9') || json_str[i] == '.' || json_str[i] == '-' || json_str[i] == '+'){
            if(json_str[i] == '-' || json_str[i] == '+' ){
                // Un signe n'est valide qu'au tout début ('-' uniquement) ou juste après le 'e'/'E'
                int isStart = (numberOfDigit == 0 && json_str[i] == '-');
                int isAfterExp = (numberOfDigit > 0 && (json_str[i-1] == 'e' || json_str[i-1] == 'E'));
                if(!isStart && !isAfterExp){
                    free(exp_value.value.string);
                    exp_value = initJsonValue(JSON_ERROR);
                    return exp_value;
                }
            }else if(json_str[i] == '.'){
                numberOfDot ++;
            }
            exp_value.value.string[numberOfDigit] = json_str[i];
            numberOfDigit++;
        }
        else if(json_str[i] == 'e' || json_str[i] == 'E'){
            if(i > 0 && (json_str[i-1] >= '0' && json_str[i-1] <= '9')){
                if((json_str[i+1] >= '0' && json_str[i+1] <= '9') ||
                                json_str[i+1] == '-' || json_str[i+1] == '+'){
                    exp_value.value.string[numberOfDigit] = json_str[i];
                    numberOfDigit++;
                }else{
                    free(exp_value.value.string);
                    exp_value = initJsonValue(JSON_ERROR);
                    return exp_value;
                }
            }else{
                free(exp_value.value.string);
                exp_value = initJsonValue(JSON_ERROR);
                return exp_value;
            }
        }
        else if(json_str[i] == ']' || json_str[i] == '}' || json_str[i] == ','){
            break;
        }
        else{
            break;
        }
    }
    exp_value.value.string[numberOfDigit] = '\0';

    *position = *position + numberOfDigit;  
    return exp_value;
}

JsonValue parseNUMBER(char* json_str,size_t* position){
    debug("parseNUMBER");
    JsonValue nbr_value = initJsonValue(JSON_ERROR);

    char* number_str = (char*)calloc(64, sizeof(char));
    if(number_str == NULL){
        nbr_value = initJsonValue(JSON_ERROR);
        return nbr_value;
    }
    long long int number_int;
    size_t numberOfDigit = 0;
    int state;

    
    nbr_value.type = JSON_NUMBER;

    for(size_t index=*position;json_str[index] != '\0';index++){
        if((json_str[index] >= 48 && json_str[index] <= 57) || json_str[index] == '-'){
            if(numberOfDigit >= 63){
                nbr_value = initJsonValue(JSON_ERROR);
                free(number_str);
                return nbr_value;
            }
            numberOfDigit ++;
            number_str[numberOfDigit-1] = json_str[index];
        }else{
            break;
        }
    }
    number_str[numberOfDigit] = '\0';
    state = stringToInt(number_str,&number_int);
    if(state == 1){
        // Trop long pour un long long -> on le garde en double (valeur approchée)
        double number_double;
        if(stringToDouble(number_str, &number_double)){
            nbr_value = initJsonValue(JSON_ERROR);
            free(number_str);
            return nbr_value;
        }
        nbr_value.type = JSON_DECIMAL;
        nbr_value.value.decimal = number_double;
    }else{
        nbr_value.value.integer = number_int;
    }

    *position = *position + numberOfDigit;  
    free(number_str);
    return nbr_value;
}

JsonValue parseBOOL(char* json_str, size_t* position){
    debug("parseBOOL");
    JsonValue boo_value = initJsonValue(JSON_ERROR);

    if(json_str[*position] == '\0'){
        return boo_value;
    }

    // indexEnd = premier délimiteur, ou le '\0' final s'il n'y en a pas
    size_t indexEnd = *position;
    while(json_str[indexEnd] != '\0' && json_str[indexEnd] != ',' && json_str[indexEnd] != '}' && json_str[indexEnd] != ']'){
        indexEnd++;
    }

    char* buffer = (char*)calloc(indexEnd - (*position) + 1, sizeof(char));
    if(buffer == NULL){
        return boo_value;
    }
    
    int state = _strcpybxy(&buffer, json_str, (int)*position, (int)(indexEnd - 1));
    if(state == 1){
        free(buffer);
        *position = indexEnd;
        return boo_value;
    }
    
    if(_strcmp(buffer, "true") == 0){
        boo_value.type = JSON_BOOL;
        boo_value.value.integer = 1;
    }else if(_strcmp(buffer, "false") == 0){
        boo_value.type = JSON_BOOL;
        boo_value.value.integer = 0;
    }
    
    free(buffer);
    *position = indexEnd;
    return boo_value;
}

JsonValue parseNULL(char* json_str,size_t* position){
    JsonValue null_value = initJsonValue(JSON_ERROR);
    int state;

    // indexEnd = premier délimiteur, ou le '\0' final s'il n'y en a pas
    size_t indexEnd = *position;
    while(json_str[indexEnd] != '\0' && json_str[indexEnd] != ',' && json_str[indexEnd] != '}' && json_str[indexEnd] != ']'){
        indexEnd++;
    }
    if(*position == indexEnd){
        return null_value;
    }

    char* buffer =(char*)calloc(indexEnd-(*position)+1,sizeof(char));
    if(buffer == NULL){
        return null_value;
    }
    state = _strcpybxy(&buffer,json_str,(int)*position,indexEnd-1);
    if(state == 1){
        *position = indexEnd;
        free(buffer);
        return null_value;
    }
    if(_strcmp(buffer,"null") == 0){
        null_value.type = JSON_NULL;
        null_value.value.integer = 0;
    }
    *position = indexEnd;
    free(buffer);
    return null_value;
}

JsonValue parseDECIMAL(char* json_str, size_t* position){
    debug("parseDECIMAL");
    JsonValue dec_value = initJsonValue(JSON_ERROR);

    char* decimal_str = (char*)calloc(64, sizeof(char));
    if(decimal_str == NULL){
        dec_value = initJsonValue(JSON_ERROR);
        return dec_value;
    }
    double decimal_double;
    size_t numberOfDigit = 0;
    
    int state;

    dec_value.type = JSON_DECIMAL;

    for(size_t index = *position; json_str[index] != '\0'; index++){
        if(numberOfDigit >= 63){
            dec_value = initJsonValue(JSON_ERROR);
            free(decimal_str);
            return dec_value;
        }
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
        dec_value = initJsonValue(JSON_ERROR);
        free(decimal_str);
        return dec_value;
    }
    
    dec_value.value.decimal = decimal_double;
    *position = *position + numberOfDigit;  
    free(decimal_str);
    return dec_value;
}

JsonValue parseValue(char* json_str, size_t* position) {
    debug("parseValue");
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
        case JSON_EXPONENTIAL:
            return parseEXPONENTIAL(json_str, position);
        default: {
            JsonValue error_value = initJsonValue(JSON_ERROR);
            return error_value;
        }
    }
}
//////////////////////////////////////////// free function ////////////////////////////////////////////
void freeValue(JsonValue value){
    switch(value.type){
        case JSON_BOOL:
        case JSON_DECIMAL:
        case JSON_NULL:
        case JSON_NUMBER:
        case JSON_ERROR:
        break;
        case JSON_STRING:
        case JSON_EXPONENTIAL:{
            free(value.value.string);
        }
        break;
        case JSON_OBJECT:{
            freeObject(value.value.object);
        }
        break;
        case JSON_ARRAY:{
            freeArray(value.value.array);
        }
        break;
    }
    debug("free value\n");
}

void freeObject(JsonObject *obj){
    if(obj == NULL){
        return;
    }
    for(int i=0;i<obj->nbOfElement;i++){
        free(obj->listeOfPair[i].key);
        switch(obj->listeOfPair[i].value.type){
            case JSON_BOOL:
            case JSON_DECIMAL:
            case JSON_NULL:
            case JSON_NUMBER:
            case JSON_ERROR:
            break;
            case JSON_STRING:
            case JSON_EXPONENTIAL:{
                free(obj->listeOfPair[i].value.value.string);
            }
            break;
            case JSON_OBJECT:{
                freeObject(obj->listeOfPair[i].value.value.object);
            }
            break;
            case JSON_ARRAY:{
                freeArray(obj->listeOfPair[i].value.value.array);
            }
            break;
        }
    }
    free(obj->listeOfPair);
    free(obj);
    debug("free obj\n");
}

void freeArray(JsonArray *ary){
    if(ary == NULL){
        return;
    }
    for(int i = 0; i < ary->nbOfElement; i++){
        switch(ary->listeOfValue[i].type){
            case JSON_BOOL:
            case JSON_DECIMAL:
            case JSON_NULL:
            case JSON_NUMBER:
            case JSON_ERROR:
            break;
            case JSON_STRING:
            case JSON_EXPONENTIAL:{
                free(ary->listeOfValue[i].value.string);
            }
            break;
            case JSON_OBJECT:{
                freeObject(ary->listeOfValue[i].value.object);
            }
            break;
            case JSON_ARRAY:{
                freeArray(ary->listeOfValue[i].value.array);
            }
            break;
        }
    }
    free(ary->listeOfValue);
    free(ary);
    debug("free ary\n");
}

//////////////////////////////////////////// cpy function ////////////////////////////////////////////

int cpyValue(JsonValue* dest,JsonValue* src){
    if(dest == NULL ||src == NULL){
        printf("Erreur : copy of a array impossible -> dest or src is null");
        return 1;
    }
    int state;

    *dest = initJsonValue(src->type);   // les 8 octets de value à zéro
    switch(dest->type){
        case JSON_NULL :
            dest->value.integer = 0;
        break;
        case JSON_BOOL :
            if(src->value.integer == 1){
                dest->value.integer = 1;
            }else if(src->value.integer == 0){
                dest->value.integer = 0;
            }else{
                dest->type = JSON_ERROR;
                dest->value.integer = 0;
            }
        break;
        case JSON_NUMBER:
            dest->value.integer = src->value.integer;
        break;
        case JSON_STRING :
            dest->value.string = NULL;
            state = _strcpy(&dest->value.string,src->value.string);
            if(state){
                printf("Error : copy of value (String) had a problem...\n");
                free(dest->value.string);
                return 1;
            }
        break;
        case JSON_ARRAY :
            dest->value.array = (JsonArray*)malloc(sizeof(JsonArray));
            if(dest->value.array == NULL){
                printf("Error : malloc error during the copy of a JSON_ARRAY Allocation...\n");
                return 1;
            }
            dest->value.array->nbOfElement = 0;
            dest->value.array->listeOfValue = NULL;

            state = cpyArray(dest->value.array,src->value.array);
            if(state){
                printf("Error : copy of value (Array) had a problem...\n");
                free(dest->value.array);
                *dest = initJsonValue(JSON_ERROR);
                return 1;
            }
        break;
        case JSON_OBJECT :
            dest->value.object = (JsonObject*)malloc(sizeof(JsonObject));
            if(dest->value.object == NULL){
                printf("Error : malloc error during the copy of a JSON_OBJECT Allocation...\n");
                return 1;
            }
            dest->value.object->nbOfElement = 0;
            dest->value.object->listeOfPair = NULL;

            state = cpyObject(dest->value.object,src->value.object);
            if(state){
                printf("Error : copy of value (Object) had a problem...\n");
                free(dest->value.object);
                *dest = initJsonValue(JSON_ERROR);
                return 1;
            }
        break;
        case JSON_DECIMAL :
            dest->value.decimal = src->value.decimal;
        break;
        case JSON_EXPONENTIAL :
            dest->value.string = NULL;
            state = _strcpy(&dest->value.string,src->value.string);
            if(state){
                printf("Error : copy of value (Exponential) had a problem...\n");
                free(dest->value.string);
                return 1;
            }
        break;
        case JSON_ERROR :
            dest->value.integer = 0;
        break;
        default :
            printf("Error : unknow type...\n");
    }
    return 0;
}

int cpyObject(JsonObject* dest,JsonObject* src){
    if(dest == NULL ||src == NULL){
        printf("Erreur : copy of a array impossible -> dest or src is null");
        return 1;
    }
    if(dest == src){
        return 0;
    }
    int state;

    if(dest->listeOfPair == NULL){
        dest->listeOfPair = (JsonPair*)calloc(src->nbOfElement, sizeof(JsonPair));
        if(dest->listeOfPair == NULL){
            printf("Erreur : something went wrong during the allocation for an object");
            return 1;
        }
    }else{
        if(dest->listeOfPair != NULL){
            for(int i = 0; i < dest->nbOfElement; i++){
                free(dest->listeOfPair[i].key);
                freeValue(dest->listeOfPair[i].value);
            }
            free(dest->listeOfPair);
            dest->listeOfPair = NULL;
        }
        dest->listeOfPair = (JsonPair*)calloc(src->nbOfElement, sizeof(JsonPair));
        if(dest->listeOfPair == NULL){
            return 1;
        }
    }

    dest->nbOfElement = src->nbOfElement;
    for(int i=0;i<dest->nbOfElement;i++){
        state = _strcpy(&dest->listeOfPair[i].key,src->listeOfPair[i].key);
        if(state){
            printf("Error : copy of key from the object %s had a problem...\n",src->listeOfPair[i].key);
            for(int j = 0; j < i; j++){
                free(dest->listeOfPair[j].key);
                freeValue(dest->listeOfPair[j].value);
                
            }
            free(dest->listeOfPair);
            dest->listeOfPair = NULL;
            dest->nbOfElement = 0;
            return 1;
        }

        state = cpyValue(&dest->listeOfPair[i].value,&src->listeOfPair[i].value);
        if(state){
            printf("Error : copy of the value from the object %s had a problem...\n",src->listeOfPair[i].key);
            free(dest->listeOfPair[i].key);
            for(int j = 0; j < i; j++){
                free(dest->listeOfPair[j].key);
                freeValue(dest->listeOfPair[j].value);
                dest->nbOfElement = 0;
            }
            free(dest->listeOfPair);
            dest->listeOfPair = NULL;
            dest->nbOfElement = 0;
            return 1;
        }
    }
    return 0;
}

int cpyArray(JsonArray* dest,JsonArray* src){
    if(dest == NULL ||src == NULL){
        printf("Erreur : copy of a array impossible -> dest or src is null");
        return 1;
    }
    if(dest == src){
        return 0;
    }
    int state;

    // dest contient déjà des valeurs -> on les libère, sa liste peut être plus petite que src
    if(dest->listeOfValue != NULL){
        for(int i = 0; i < dest->nbOfElement; i++){
            freeValue(dest->listeOfValue[i]);
        }
        free(dest->listeOfValue);
        dest->listeOfValue = NULL;
        dest->nbOfElement = 0;
    }

    dest->listeOfValue = (JsonValue*)calloc(src->nbOfElement,sizeof(JsonValue));
    if(dest->listeOfValue == NULL){
        printf("Erreur : something went wrong during the allocation for a array");
        return 1;
    }

    dest->nbOfElement = src->nbOfElement;
    for(int i=0; i < src->nbOfElement; i++){
        state = cpyValue(&dest->listeOfValue[i],&src->listeOfValue[i]);
        if(state){
            printf("Erreur : copy of an array had a error...");
            for(int j = 0; j < i; j++){
                freeValue(dest->listeOfValue[j]);
            }
            // On laisse dest vide et cohérent (sinon double free au prochain freeArray)
            free(dest->listeOfValue);
            dest->listeOfValue = NULL;
            dest->nbOfElement = 0;
            return 1;
        }
    }

    return 0;
}

//////////////////////////////////////////// treatment function ////////////////////////////////////////////

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

// Retire les espaces hors des strings en une seule passe.
// dest peut pointer sur src (nettoyage sur place) : indexWrite <= i, on n'écrase jamais un char pas encore lu.
// Si *dest est NULL, un buffer de _strlen(src)+1 octets est alloué (le résultat n'est jamais plus long que src).
int whitespaceCleaner(char* src,char** dest){
    size_t indexWrite = 0;
    int insideString = 0;

    if(src == NULL || dest == NULL){
        return 1;
    }
    if(*dest == NULL){
        *dest = (char*)malloc(_strlen(src) + 1);
        if(*dest == NULL){
            return 1;
        }
    }

    for(size_t i=0;src[i] != '\0';i++){
        char c = src[i];

        if(insideString){
            // Dans une string on garde tout, espaces compris
            (*dest)[indexWrite] = c;
            indexWrite++;
            if(c == '\\' && src[i + 1] != '\0'){
                // Séquence d'échappement (\" \\ \n ...) : on recopie aussi le char suivant,
                // sinon un \" serait pris pour la fin de la string
                i++;
                (*dest)[indexWrite] = src[i];
                indexWrite++;
            }else if(c == '"'){
                insideString = 0;
            }
            continue;
        }

        if(c == ' ' || c == '\n' || c == '\t' || c == '\r'){
            continue;
        }
        if(c == '"'){
            insideString = 1;
        }
        (*dest)[indexWrite] = c;
        indexWrite++;
    }

    (*dest)[indexWrite] = '\0';
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
                case '-':
        case '0' ... '9' : {
            size_t index = position;
            int boolEXP = 0;
            int boolDEC = 0;

            if(json_str[index] == '-'){
                index++;
            }
            while(json_str[index] != '\0' &&
                ((json_str[index] >= '0' && json_str[index] <= '9') ||
                json_str[index] == '.' ||
                json_str[index] == 'e' || json_str[index] == 'E' ||
                json_str[index] == '+' || json_str[index] == '-')) {

                if(json_str[index] == '.'){
                    boolDEC = 1;
                }
                if(json_str[index] == 'e' || json_str[index] == 'E'){
                    boolEXP = 1;
                }
                index++;
            }
            if(boolEXP){
                return JSON_EXPONENTIAL;
            }else if(boolDEC){
                return JSON_DECIMAL;
            }else{
                return JSON_NUMBER;
            }
        }
        break;
        default:
            return JSON_ERROR;
    }
}

long getSize(FILE* file){
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    return size;
}

int isEscaped(char* str, size_t position) {
    size_t count = 0;

    while (position > 0 && str[position - 1] == '\\') {
        count++;
        position--;
    }

    return count % 2 != 0;
}

//////////////////////////////////////////// treatment function ////////////////////////////////////////////

size_t _strlen(char* str){
    if(str == NULL){
        return -1;
    }
    size_t i =0;
    while(str[i] != '\0'){
        i++;
    }
    return i;
}

int _strcmp(char* str1,char* str2){
    if(str1 == NULL){
        if(str2 == NULL){
            return 0;
        }else{
            return 1;
        }
    }
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

int _strchr(char* str, char c){
    if(str == NULL){
        return -1;
    }
    size_t len = _strlen(str);
    for(size_t i = 0; i < len; i++){
        if(str[i] == c){
            return (int)i;
        }
    }
    return -1;
}

// String copy between x and y
int _strcpybxy(char **dest, char *src, int x, int y){
    if(src == NULL){
        return 1;
    }

    if (x < 0 || y < x) {
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
    if(str == NULL){
        return -1;
    }
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
    if(src == NULL){
        return 1;
    }

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

int stringToInt(char* str,long long int* res){
    if(str == NULL){
        return 1;
    }
    long long int result = 0;
    int boolNeg = 0;
    int i = 0;
    int len = (int)_strlen(str);
    if(len == 0) {
        return 1;
    }

    if(str[len-1] == '\n'){
        len--;
    } 
    if(str[0] == '-') {
        boolNeg = 1;
        i++;
    }
    if(len - i >= 19){
        return 1;
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

void debug(char* str){

    if(DEBUG_VALUE == 1){
        if(str == NULL){
            printf("\033[38;2;255;0;0m");
            printf("DEBUG!\n");
            printf("\033[0m");
        }else{
            printf("\033[38;2;255;0;0m");
            printf("DEBUG : %s\n",str);
            printf("\033[0m");
        }

    }
}