#include <stdio.h>
#include <stdint.h> 
#include <stdlib.h>

#define BUFFER_DEFAULT_SIZE 64

#define DEBUG_VALUE 0

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

// Initialisée la transcription Text -> JsonValue
JsonValue initCson(char* path);

// Utils
JsonType getType(char* json_str,size_t position);
long getSize(FILE* file);
int loadJson(char** dest,FILE* file);
int whitespaceCleaner(char** str_json,long* size);
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
int stringToInt(char* str,int* res);
int stringToDouble(char* str, double* res);

void debug(char* str);

int main(int argc, char *argv[]) {
    // Créer un objet source ALLOUÉ
    JsonObject* srcObj = (JsonObject*)malloc(sizeof(JsonObject));
    srcObj->nbOfElement = 0;
    srcObj->listeOfPair = (JsonPair*)calloc(10, sizeof(JsonPair));
    
    JsonValue val1;
    val1.type = JSON_STRING;
    val1.value.string = NULL;
    _strcpy(&val1.value.string, "Hello World");
    int state = addToObject(srcObj, "greeting", val1, 0);
    if(state != 0) printf("Erreur ajout val1\n");
    
    JsonValue val2;
    val2.type = JSON_NUMBER;
    val2.value.integer = 42;
    state = addToObject(srcObj, "answer", val2, 1);
    if(state != 0) printf("Erreur ajout val2\n");
    
    JsonValue val3;
    val3.type = JSON_BOOL;
    val3.value.integer = 1;
    state = addToObject(srcObj, "active", val3, 2);
    if(state != 0) printf("Erreur ajout val3\n");
    
    printf("===================== OBJET SOURCE =====================\n");
    int depth = 0;
    printfObject(&depth, *srcObj);
    
    // Créer un objet destination ALLOUÉ
    printf("===================== COPIE OBJET =====================\n");
    JsonObject* destObj = (JsonObject*)malloc(sizeof(JsonObject));
    destObj->nbOfElement = 0;
    destObj->listeOfPair = (JsonPair*)calloc(10, sizeof(JsonPair));
    
    state = cpyObject(destObj, srcObj);
    if(state == 1){
        printf("Erreur lors de la copie objet\n");
    }
    
    depth = 0;
    printfObject(&depth, *destObj);
    
    // Tester avec un array
    printf("\n===================== ARRAY SOURCE =====================\n");
    JsonArray* srcAry = (JsonArray*)malloc(sizeof(JsonArray));
    srcAry->nbOfElement = 0;
    srcAry->listeOfValue = (JsonValue*)calloc(10, sizeof(JsonValue));
    
    JsonValue aryVal1;
    aryVal1.type = JSON_NUMBER;
    aryVal1.value.integer = 10;
    state = addToArray(srcAry, aryVal1, 0);
    if(state != 0) printf("Erreur ajout aryVal1\n");
    
    JsonValue aryVal2;
    aryVal2.type = JSON_STRING;
    aryVal2.value.string = NULL;
    _strcpy(&aryVal2.value.string, "test");
    state = addToArray(srcAry, aryVal2, 1);
    if(state != 0) printf("Erreur ajout aryVal2\n");
    
    depth = 0;
    printfArray(&depth, *srcAry);
    printf("\n");
    
    // Copier l'array
    printf("===================== COPIE ARRAY =====================\n");
    JsonArray* destAry = (JsonArray*)malloc(sizeof(JsonArray));
    destAry->nbOfElement = 0;
    destAry->listeOfValue = NULL;
    
    state = cpyArray(destAry, srcAry);
    if(state == 1){
        printf("Erreur lors de la copie array\n");
    }
    
    depth = 0;
    printfArray(&depth, *destAry);
    printf("\n");
    
    // Libérer
    printf("===================== LIBERATION =====================\n");
    freeObject(srcObj);
    freeObject(destObj);
    freeArray(srcAry);
    freeArray(destAry);
    printf("Tout libéré !\n");
    
    return 0;
}

JsonValue initCson(char* path){
    JsonValue json;
    json.type = JSON_ERROR; 
    json.value.integer = 0;

    FILE* file = fopen(path, "r");
    if(file == NULL){
        printf("Error : impossible to open JSON file\n");
        return json;
    }

    char* json_str = NULL;
    if(loadJson(&json_str, file) != 0) {
        printf("Error : impossible to load the JSON file (to big?)\n");
        fclose(file);
        return json;
    }
    fclose(file);

    long size = _strlen(json_str);
    whitespaceCleaner(&json_str, &size);

    size_t pos = 0;
    json = parseValue(json_str, &pos);

    free(json_str);
    return json;
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
            printf("%d",value.value.integer);
        break;
        case JSON_OBJECT:
            printfObject(&depth,*value.value.object);
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
                printf("%d",ary.listeOfValue[i].value.integer);
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
                printf("%d",obj.listeOfPair[i].value.value.integer);
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
            fprintf(file,"%d",value.value.integer);
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
                fprintf(file,"%d",obj.listeOfPair[i].value.value.integer);
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
    fprintf(file,"}");
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
                fprintf(file,"%d",ary.listeOfValue[i].value.integer);
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

//////////////////////////////////////////// add function ////////////////////////////////////////////

int addToObject(JsonObject* obj, char* key, JsonValue value,size_t index){
    if(obj == NULL || key == NULL || obj->nbOfElement<index){
        return 1;
    }

    if(getIndexFromObject(obj,key) != -1){
        return 1;
    }

    JsonPair* newListe = (JsonPair*)realloc(obj->listeOfPair, (obj->nbOfElement + 1) * sizeof(JsonPair));
    if(newListe == NULL){
        return 1;
    }
    obj->listeOfPair = newListe;

    for(size_t i = obj->nbOfElement; i > index; i--){
        newListe[i] = newListe[i-1];
    }

    newListe[index].key = NULL;
    if((_strcpy(&newListe[index].key,key))){
        return 1;
    }
    newListe[index].value = value;
    obj->nbOfElement++;

    return 0;
}

int addToArray(JsonArray* ary, JsonValue value,size_t index){
    if(ary == NULL || ary->nbOfElement < index){
        return 1;
    }

    JsonValue* newListe = (JsonValue*)realloc(ary->listeOfValue, (ary->nbOfElement + 1) * sizeof(JsonValue));
    if(newListe == NULL){
        return 1;
    }
    ary->listeOfValue = newListe;

    for(size_t i = ary->nbOfElement; i > index; i--){
        newListe[i] = newListe[i-1];
    }
    newListe[index] = value;
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
    JsonPair* newListe = (JsonPair*)realloc(obj->listeOfPair, obj->nbOfElement * sizeof(JsonPair));
    if(newListe == NULL && obj->nbOfElement > 0){
        return 1;
    }
    obj->listeOfPair = newListe;

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
    JsonValue* newListe = (JsonValue*)realloc(ary->listeOfValue, ary->nbOfElement * sizeof(JsonValue));
    if(newListe == NULL && ary->nbOfElement > 0){
        return 1;
    }
    ary->listeOfValue = newListe;

    return 0;
}

//////////////////////////////////////////// modify function ////////////////////////////////////////////

int modifyObject(JsonObject* obj,JsonPair paire,size_t index){
    if(obj == NULL || index >= obj->nbOfElement){
        return 1;
    }

    free(obj->listeOfPair[index].key);
    freeValue(obj->listeOfPair[index].value);
    obj->listeOfPair[index] = paire;

    return 0;
}

int modifyArray(JsonArray* ary,JsonValue value,size_t index){
    if(ary == NULL || index >= ary->nbOfElement){
        return 1;
    }

    freeValue(ary->listeOfValue[index]);
    ary->listeOfValue[index] = value;

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

//////////////////////////////////////////// parse function ////////////////////////////////////////////

JsonValue parseOBJ(char* json_str, size_t* position){
    debug("parseOBJ");
    size_t size = _strlen(json_str);
    JsonValue obj_value;
    if(size == (size_t)-1){
        obj_value.type = JSON_ERROR;
        return obj_value;
    }

    size_t capacity = 10;
    obj_value.type = JSON_OBJECT;
    obj_value.value.object = (JsonObject*)malloc(sizeof(JsonObject));
    obj_value.value.object->nbOfElement = 0;
    obj_value.value.object->listeOfPair = (JsonPair*)calloc(10, sizeof(JsonPair));

    JsonType targeted_type;
    JsonPair buffeur_pair;
    JsonValue buffeur_value;
    buffeur_value.value.string = NULL;

    int NumberOfElement = 0;
    size_t index;
    for(index = *position+1; index<size;){
        if(json_str[index] == '}'){
            break;
        }else if(json_str[index] == ','){
            index++;
            continue;
        }

        if(capacity <= NumberOfElement){
            capacity *= 2;
            obj_value.value.object->listeOfPair = (JsonPair*)realloc(
                obj_value.value.object->listeOfPair, 
                capacity * sizeof(JsonPair)
            );
            if(obj_value.value.object->listeOfPair == NULL){
                free(obj_value.value.object->listeOfPair);
                obj_value.type = JSON_ERROR;
                return obj_value;
            }
        }

        buffeur_value = parseSTRING(json_str, &index);
        buffeur_pair.key = NULL;
        _strcpy(&buffeur_pair.key, buffeur_value.value.string);

        index++;

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
                while(index < size && json_str[index] != ',' && json_str[index] != '}' && json_str[index] != ']') {
                    index++;
                }
                
                // Créer la clé "error"
                buffeur_pair.key = NULL;
                _strcpy(&buffeur_pair.key, "error");
                
                // Créer la valeur d'erreur
                JsonValue errorValue;
                errorValue.type = JSON_ERROR;
                errorValue.value.string = NULL;
                _strcpy(&errorValue.value.string, "INVALID_VALUE");  // ← Allocate dynamiquement
                
                buffeur_pair.value = errorValue;
                obj_value.value.object->listeOfPair[NumberOfElement] = buffeur_pair;
                NumberOfElement++;
                
                // Avancer après la virgule si présente
                if(index < size && json_str[index] == ',') {
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
    }
    
    *position = index + 1;
    obj_value.value.object->nbOfElement = NumberOfElement;
    return obj_value;
}

JsonValue parseARRAY(char* json_str, size_t* position){
    debug("parseARRAY");
    size_t size = _strlen(json_str);
    JsonValue ary_value;
    if(size == (size_t)-1){
        ary_value.type = JSON_ERROR;
        return ary_value;
    }

    size_t capacity = 10;
    ary_value.type = JSON_ARRAY;
    ary_value.value.array = (JsonArray*)malloc(sizeof(JsonArray));
    ary_value.value.array->nbOfElement = 0;
    ary_value.value.array->listeOfValue = (JsonValue*)calloc(capacity, sizeof(JsonValue));

    
    JsonType targeted_type;
    JsonValue buffeur;

    int NumberOfElement = 0;
    size_t index;
    for( index = *position+1; index<size;){
        if(json_str[index] == ']'){
            break;
        }else if(json_str[index] == ','){
            index++;
            continue;
        }

        if(capacity <= NumberOfElement){
            capacity *= 2;
            ary_value.value.array->listeOfValue = (JsonValue*)realloc(
                ary_value.value.array->listeOfValue, 
                capacity * sizeof(JsonValue)
            );
            if(ary_value.value.array->listeOfValue == NULL){
                free(ary_value.value.array->listeOfValue);
                ary_value.type = JSON_ERROR;
                return ary_value;
            }
        }

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
                // Avancer jusqu'au prochain délimiteur
                while(index < size && json_str[index] != ',' && json_str[index] != '}' && json_str[index] != ']') {
                    index++;
                }

                buffeur.value.string = NULL;
                _strcpy(&buffeur.value.string,"INVALID_VALUE");
                buffeur.type = JSON_ERROR;

                ary_value.value.array->listeOfValue[NumberOfElement] = buffeur;
                NumberOfElement++;

                // Avancer après la virgule si présente
                if(index < size && json_str[index] == ',') {
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

    }

    *position = index + 1;
    ary_value.value.array->nbOfElement = NumberOfElement;
    return ary_value;
}

JsonValue parseSTRING(char* json_str,size_t* position){
    debug("parseSTRING");
    size_t size = _strlen(json_str);
    JsonValue str_value;
    if(size == (size_t)-1){
        str_value.type = JSON_ERROR;
        return str_value;
    }
    str_value.type = JSON_STRING;
    str_value.value.string = NULL;

    size_t start = *position;

    if(start >= size || json_str[start] != '"') {
        str_value.type = JSON_ERROR;
        return str_value;
    }

    size_t end = start;

    for(size_t index = start + 1; index < size; index++){
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
    size_t size = _strlen(json_str);
    JsonValue exp_value;
    if(size == (size_t)-1){
        exp_value.type = JSON_ERROR;
        return exp_value;
    }

    exp_value.type = JSON_EXPONENTIAL;
    exp_value.value.string = (char*)calloc(64,sizeof(char));
    int numberOfDigit = 0;
    int numberOfDot = 0;
    int NumberOfOperator = 0;
    
    for(size_t i = *position;i<size;i++){
        if(numberOfDot > 1 || NumberOfOperator > 1 ){
            exp_value.type = JSON_ERROR;
            return exp_value;
        }
        if((json_str[i] >= '0' && json_str[i] <= '9') || json_str[i] == '.' || json_str[i] == '-' || json_str[i] == '+'){
            if(json_str[i] == '-' || json_str[i] == '+' ){
                NumberOfOperator++;
            }else if(json_str[i] == '.'){
                numberOfDot ++;
            }
            exp_value.value.string[numberOfDigit] = json_str[i];
            numberOfDigit++;
        }
        else if(json_str[i] == 'e' || json_str[i] == 'E'){
            if(i > 0 && (json_str[i-1] >= '0' && json_str[i-1] <= '9')){
                if(i+1 < size && ((json_str[i+1] >= '0' && json_str[i+1] <= '9') ||
                                json_str[i+1] == '-' || json_str[i+1] == '+')){
                    exp_value.value.string[numberOfDigit] = json_str[i];
                    numberOfDigit++;
                }else{
                    exp_value.type = JSON_ERROR;
                    free(exp_value.value.string);
                    return exp_value;
                }
            }else{
                exp_value.type = JSON_ERROR;
                free(exp_value.value.string);
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
    size_t size = _strlen(json_str);
    JsonValue nbr_value;
    if(size == (size_t)-1){
        nbr_value.type = JSON_ERROR;
        return nbr_value;
    }

    char* number_str = (char*)calloc(64, sizeof(char));
    int number_int;
    size_t numberOfDigit = 0;
    int state;

    
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
    debug("parseBOOL");
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
    debug("parseDECIMAL");
    size_t size = _strlen(json_str);
    JsonValue dec_value;
    if(size == (size_t)-1){
        dec_value.type = JSON_ERROR;
        return dec_value;
    }

    char* decimal_str = (char*)calloc(64, sizeof(char));
    double decimal_double;
    size_t numberOfDigit = 0;
    
    int state;

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
            JsonValue error_value;
            error_value.type = JSON_ERROR;
            error_value.value.integer = 0;
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
        break;
        case JSON_ERROR:
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
            break;
            case JSON_ERROR:
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
            break;
            case JSON_ERROR:
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

    dest->type = src->type;
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
                return 1;
            }
        break;
        case JSON_ARRAY :
            state = cpyArray(dest->value.array,src->value.array);
            if(state){
                printf("Error : copy of value (Array) had a problem...\n");
                return 1;
            }
        break;
        case JSON_OBJECT :
            state = cpyObject(dest->value.object,src->value.object);
            if(state){
                printf("Error : copy of value (Object) had a problem...\n");
                return 1;
            }
        break;
        case JSON_DECIMAL :
            dest->value.decimal = src->value.decimal;
        break;
        case JSON_EXPONENTIAL :
            state = _strcpy(&dest->value.string,src->value.string);
            if(state){
                printf("Error : copy of value (Exponential) had a problem...\n");
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
    int state;

    if(dest->listeOfPair == NULL){
        dest->listeOfPair = (JsonPair*)calloc(src->nbOfElement, sizeof(JsonPair));
        if(dest->listeOfPair == NULL){
            printf("Erreur : something went wrong during the allocation for an object");
            return 1;
        }
    }

    dest->nbOfElement = src->nbOfElement;
    for(int i=0;i<dest->nbOfElement;i++){
        state = _strcpy(&dest->listeOfPair[i].key,src->listeOfPair[i].key);
        if(state){
            printf("Error : copy of key from the object %s had a problem...\n",src->listeOfPair[i].key);
            return 1;
        }

        state = cpyValue(&dest->listeOfPair[i].value,&src->listeOfPair[i].value);
        if(state){
            printf("Error : copy of the value from the object %s had a problem...\n",src->listeOfPair[i].key);
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
    int state;

    if(dest->listeOfValue == NULL){
        dest->listeOfValue = (JsonValue*)calloc(src->nbOfElement,sizeof(JsonValue));
        if(dest->listeOfValue == NULL){
            printf("Erreur : something went wrong during the allocation for a array");
            return 1;
        }
    }

    dest->nbOfElement = src->nbOfElement;
    for(int i=0; i < src->nbOfElement; i++){
        state = cpyValue(&dest->listeOfValue[i],&src->listeOfValue[i]);
        if(state){
            printf("Erreur : copy of an array had a error...");
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

int whitespaceCleaner(char** str_json, long* size){
    int whitespaceCleanedCount = 0;
    int IsInsideQuote = 0;

    for(long i = 0; i < *size; i++){
        char c = (*str_json)[i];

        if(c == '"' && !isEscaped(*str_json, i)){
            IsInsideQuote = !IsInsideQuote;
        }

        if(IsInsideQuote){
            continue;
        }

        if(c == '\n' || c == '\t' || c == '\r' || c == ' '){
            for(long j = i; j < *size; j++){
                (*str_json)[j] = (*str_json)[j + 1];
            }

            (*size)--;
            (*str_json)[*size] = '\0';

            whitespaceCleanedCount++;
            i--;
        }
    }

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
            size_t size = _strlen(json_str);
            if(size == (size_t)-1){
                return JSON_ERROR;
            }
            int boolEXP = 0;
            int boolDEC = 0;

            if(json_str[index] == '-'){
                index++;
            }
            while(index < size && 
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

int _strchr(char* str,char c){
    if(str == NULL){
        return -1;
    }
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
    if(src == NULL){
        return 1;
    }
    int lenSrc = _strlen(src);
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