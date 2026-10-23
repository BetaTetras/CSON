#include <stdio.h>
#include <stdint.h> 
#include <stdlib.h>

#include "cson.h"

#define BUFFER_DEFAULT_SIZE 64
#define DEBUG_VALUE 0
#define JSON_MAX_DEPTH 512  // Profondeur max d'imbrication ({ et [) acceptée par la validation
#define HEXA_TAB "0123456789abcdef"

// Les types et les fonctions publiques sont dans cson.h.
// Tout ce qui suit est interne : "static" les rend invisibles en dehors de ce fichier.

// Utils
static JsonType getType(char* json_str,size_t position);
static long getSize(FILE* file);
static int loadJson(char** dest,FILE* file);
static int whitespaceCleaner(char* src,char** dest);
static int isEscaped(char* str, size_t position);
static char witchEscapeChar(char c);
static char witchCharEscape(char c);
static int getPosInHEXA_TAB(char c);
static size_t findKey(JsonObject* obj, char* storedKey);

// parsing (suppose un texte déjà validé par validateJson)
static JsonValue parseOBJ(char* json_str, size_t* position);
static JsonValue parseARRAY(char* json_str, size_t* position);
static JsonValue parseSTRING(char* json_str,size_t* position);
static JsonValue parseNUMBER(char* json_str,size_t* position);
static JsonValue parseBOOL(char* json_str,size_t* position);
static JsonValue parseNULL(char* json_str,size_t* position);
static JsonValue parseDECIMAL(char* json_str, size_t* position);
static JsonValue parseEXPONENTIAL(char* json_str,size_t* position);
static JsonValue parseValue(char* json_str,size_t* position);

// validation (0 = valide, 1 = invalide)
static int validateJson(char* json_str, size_t size, size_t* errorPosition);
static int validateValue(char* json_str, size_t size, size_t* pos, int depth);
static int validateObject(char* json_str, size_t size, size_t* pos, int depth);
static int validateArray(char* json_str, size_t size, size_t* pos, int depth);
static int validateString(char* json_str, size_t size, size_t* pos);
static int validateUtf8Char(char* json_str, size_t size, size_t* pos);
static int validateNumber(char* json_str, size_t size, size_t* pos);
static int validateLiteral(char* json_str, size_t size, size_t* pos, char* literal);
static void skipWhitespace(char* json_str, size_t size, size_t* pos);
static int isDigitAt(char* json_str, size_t size, size_t pos);

// fprintf
static void fprintfXtab(FILE* file, int x);
static char localeDecimalPoint(void);
static void fprintfDouble(FILE* file, double d);
static void fprintfObject(FILE* file, int* depth, JsonObject obj);
static void fprintfArray(FILE* file, int* depth, JsonArray ary);

// _string.h
static size_t _strlen(char* str);
static int _strcmp(char* str1,char* str2);
static int _strcpybxy(char **dest, char *src, size_t x, size_t y);
static int _strcpy(char** dest,char* src);
static int _strtod(char* str,double *res);
static int _strecp(char** dest,char* src);

// conversion to str to x
static int _strtoi(char* str,long long int* res);

static void debug(char* str);

JsonValue initCson(char* path){
    JsonValue json = initJsonValue(JSON_ERROR);
    int state;

    FILE* file = fopen(path, "rb");
    if(file == NULL){
        fprintf(stderr, "Error : unable to open the JSON file\n");
        json.value.string = "cannot open file";
        return json;
    }
    // Sans tampon : sinon fread passe par un bloc interne de stdio, que fclose libère sans l'effacer
    // (tout le coffre y resterait en clair). À faire avant toute lecture du fichier.
    setvbuf(file, NULL, _IONBF, 0);

    char* json_str = NULL;
    state = loadJson(&json_str, file);
    if(state == 1) {
        fprintf(stderr, "Error : unable to load the JSON file (too big?)\n");
        json.value.string = "cannot load file";
        fclose(file);
        free(json_str);
        return json;
    }else if(state == 2){
        fprintf(stderr, "Error : the JSON file contains NUL byte(s)\n");
        json.value.string = "NUL byte in file";
        fclose(file);
        free(json_str);
        return json;
    }
    fclose(file);
    
    size_t size = _strlen(json_str);
    if(size == (size_t)-1){
        fprintf(stderr, "Error : a problem occurred while computing the size\n");
        json.value.string = "cannot compute size";
        return json;
    }
    // taille de tout le texte : whitespaceCleaner le raccourcit sur place, mais les octets d'après restent dans le bloc
    size_t fileSize = size;

    char* start;
    if((unsigned char)json_str[0] == 0xEF && (unsigned char)json_str[1] == 0xBB && (unsigned char)json_str[2] == 0xBF){
        start = json_str + 3;
    }else{
        start = json_str;
    }
    size = _strlen(start);   // sans les 3 octets du BOM s'il y en avait un

    size_t errorPosition = 0;
    if(validateJson(start, size, &errorPosition) != 0){
        size_t line = 1;
        size_t column = 1;
        for(size_t i = 0; i < errorPosition; i++){
            if(start[i] == '\n'){
                line++;
                column = 1;
            }else{
                column++;
            }
        }
        fprintf(stderr, "Error : invalid JSON at line %zu, column %zu\n", line, column);
        json.value.string = "invalid JSON";
        stringToZero(json_str, fileSize);
        free(json_str);
        return json;
    }

    whitespaceCleaner(start, &start);

    size_t pos = 0;
    json = parseValue(start, &pos);

    stringToZero(json_str, fileSize);   // tout le coffre est dans ce texte : on l'efface avant de rendre le bloc
    free(json_str);   // start n'est qu'une façon de lire ce bloc : un seul free
    return json;
}

JsonValue initJsonValue(JsonType type){
    JsonValue v = {0};
    v.type = type;
    return v;
}

//////////////////////////////////////////// printf function ////////////////////////////////////////////


// Affiche la valeur à l'écran : exactement ce que fprintfValue écrirait dans un fichier
void printfValue(JsonValue value){
    fprintfValue(stdout, value);
}



//////////////////////////////////////////// fprintf function ////////////////////////////////////////////

static void fprintfXtab(FILE* file, int x){
    for(int i=0;i<x;i++){
        fprintf(file,"  ");
    }
}

// Séparateur décimal de la locale actuelle : '.' par défaut, ',' si le programme a fait setlocale(LC_ALL, "fr_FR...")
// On le demande à snprintf : il l'utilise pour écrire 0.5
static char localeDecimalPoint(void){
    char buffer[8];
    snprintf(buffer, sizeof(buffer), "%.1f", 0.5);
    return buffer[1];
}

static void fprintfDouble(FILE* file, double d){
    char buffer[BUFFER_DEFAULT_SIZE];
    if(d-d != 0.0){
        fprintf(file, "\"Error : NaN or infinity is not allowed\"");
        return;
    }
    char point = localeDecimalPoint();
    double check;
    int found = 0;
    for(int precision = 1; precision <= 60 && !found; precision++){
        int len = snprintf(buffer, sizeof(buffer), "%.*f", precision, d);
        if(len >= (int)sizeof(buffer)){
            break;   // trop long : ajouter des chiffres ne fera que l'allonger
        }
        // snprintf suit la locale (',' en français) : le JSON et _strtod veulent un '.'
        for(int i = 0; buffer[i] != '\0'; i++){
            if(buffer[i] == point){
                buffer[i] = '.';
            }
        }
        if(_strtod(buffer, &check) == 0 && check == d){
            found = 1;
        }
    }
    if(!found){
        // Trop précis pour _strtod (0.1 + 0.2, 1e300...) : 17 chiffres, sera relu en texte
        snprintf(buffer, sizeof(buffer), "%.17g", d);
        for(int i = 0; buffer[i] != '\0'; i++){
            if(buffer[i] == point){
                buffer[i] = '.';
            }
        }
    }
    fprintf(file, "%s", buffer);
}


int fprintfValue(FILE* file, JsonValue value){
    if(file == NULL){
        return 1;
    }
    int depth = 0;
    switch(value.type){
        case JSON_ARRAY:
            if(value.value.array == NULL){
                fprintf(file,"null");   // tableau jamais alloué (initJsonValue seul) : rien à lire
            }else{
                fprintfArray(file,&depth,*value.value.array);
            }
        break;
        case JSON_BOOL:{
            if(value.value.integer == 1){
                fprintf(file,"true");
            }else if(value.value.integer == 0){
                fprintf(file,"false");
            }else{
                fprintf(file,"\"Boolean error\"");
            }
            break;
        }
        
        case JSON_DECIMAL:
            fprintfDouble(file,value.value.decimal);
        break;
        case JSON_ERROR:
            if(value.value.string == NULL){
                fprintf(file,"\"error...\"");
            }else{
                fprintf(file, "\"error : %s\"", value.value.string);
            }
        break;
        case JSON_EXPONENTIAL:
            if(value.value.string == NULL){
                fprintf(file,"null");
            }else{
                fprintf(file,"%s",value.value.string);
            }
        break;

        case JSON_NULL:
            fprintf(file,"null");
        break;
        case JSON_NUMBER:
            fprintf(file,"%lld",value.value.integer);
        break;
        case JSON_OBJECT:
            if(value.value.object == NULL){
                fprintf(file,"null");
            }else{
                fprintfObject(file,&depth,*value.value.object);
            }
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
            fprintf(file,"\"error : unknown type\"");
    }

    // fprintf écrit d'abord dans un tampon en mémoire : fflush l'envoie maintenant (disque plein -> erreur ici),
    // et ferror dit si une des écritures précédentes a échoué
    if(fflush(file) != 0 || ferror(file)){
        return 1;
    }
    return 0;
}

static void fprintfObject(FILE* file, int* depth, JsonObject obj){
    size_t numberOfOBJ = (size_t)obj.nbOfElement;
    // Objet vide : "{}" sur une seule ligne, comme le tableau vide "[]"
    if(obj.nbOfElement == 0) {
        fprintf(file,"{}");
        return;
    }
    fprintf(file,"{\n");
    (*depth)++;
    for(size_t i=0;i<numberOfOBJ;i++){
        
        fprintfXtab(file,*depth);
        fprintf(file,"\"%s\": ", obj.listeOfPair[i].key);
        switch(obj.listeOfPair[i].value.type){
            case JSON_OBJECT:
                if(obj.listeOfPair[i].value.value.object == NULL){
                    fprintf(file,"null");
                }else{
                    fprintfObject(file, depth, *obj.listeOfPair[i].value.value.object);
                }
            break;
            case JSON_ARRAY:
                if(obj.listeOfPair[i].value.value.array == NULL){
                    fprintf(file,"null");
                }else{
                    fprintfArray(file, depth, *obj.listeOfPair[i].value.value.array);
                }
            break;
            case JSON_BOOL:{
                if(obj.listeOfPair[i].value.value.integer == 1){
                    fprintf(file,"true");
                }else if(obj.listeOfPair[i].value.value.integer == 0){
                    fprintf(file,"false");
                }else{
                    fprintf(file,"\"Boolean error\"");
                }
            }
            break;
            case JSON_DECIMAL:
                fprintfDouble(file,obj.listeOfPair[i].value.value.decimal);
            break;
            case JSON_ERROR:
                if(obj.listeOfPair[i].value.value.string == NULL){
                    fprintf(file,"\"error...\"");
                }else{
                    fprintf(file, "\"error : %s\"", obj.listeOfPair[i].value.value.string);
                }
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
                fprintf(file,"\"error : unknown type\"");
        }
        if(i < obj.nbOfElement - 1) {
            fprintf(file,",");
        }
        fprintf(file,"\n");
    }
    (*depth)--;

    fprintfXtab(file,*depth);
    fprintf(file,"}");   // pas de \n : c'est le parent qui écrit la ',' puis le retour à la ligne
}

static void fprintfArray(FILE* file, int* depth, JsonArray ary){
    size_t numberOfElement = (size_t)ary.nbOfElement;
    fprintf(file,"[");
    if(ary.nbOfElement == 0) {
        fprintf(file,"]");
        return;
    }
    for(size_t i=0;i<numberOfElement;i++){
        switch(ary.listeOfValue[i].type){
            case JSON_OBJECT:
                if(ary.listeOfValue[i].value.object == NULL){
                    fprintf(file,"null");
                }else{
                    fprintfObject(file,depth,*ary.listeOfValue[i].value.object);
                }
            break;
            case JSON_ARRAY:
                if(ary.listeOfValue[i].value.array == NULL){
                    fprintf(file,"null");
                }else{
                    fprintfArray(file,depth,*ary.listeOfValue[i].value.array);
                }
            break;
            case JSON_BOOL:{
                if(ary.listeOfValue[i].value.integer == 1){
                    fprintf(file,"true");
                }else if(ary.listeOfValue[i].value.integer == 0){
                    fprintf(file,"false");
                }else{
                    fprintf(file,"\"Boolean error\"");
                }
            }
            break;
            case JSON_DECIMAL:
                fprintfDouble(file,ary.listeOfValue[i].value.decimal);
            break;
            case JSON_ERROR:
                if(ary.listeOfValue[i].value.string == NULL){
                    fprintf(file,"\"error...\"");
                }else{
                    fprintf(file, "\"error : %s\"", ary.listeOfValue[i].value.string);
                }
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
                fprintf(file,"\"error : unknown type\"");
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
        fprintf(stderr, "Error : cannot create the value (dest is NULL)\n");
        return 1;
    }
    *dest = initJsonValue(JSON_OBJECT);
    dest->value.object =(JsonObject*)malloc(sizeof(JsonObject));
    if(dest->value.object == NULL){
        fprintf(stderr, "Error : allocation failed while creating an object\n");
        dest->type = JSON_ERROR;
        dest->value.string = "out of memory";
        return 1;
    }
    dest->value.object->nbOfElement = 0;
    dest->value.object->listeOfPair = NULL;

    return 0;
}

int newJsonArray(JsonValue* dest){
    if(dest == NULL){
        fprintf(stderr, "Error : cannot create the value (dest is NULL)\n");
        return 1;
    }
    *dest = initJsonValue(JSON_ARRAY);
    dest->value.array = (JsonArray*)malloc(sizeof(JsonArray));
    if(dest->value.array == NULL){
        fprintf(stderr, "Error : allocation failed while creating an array\n");
        dest->type = JSON_ERROR;
        dest->value.string = "out of memory";
        return 1;
    }
    dest->value.array->nbOfElement = 0;
    dest->value.array->listeOfValue = NULL;

    return 0;
}

int newJsonString(JsonValue* dest,char* str){
    if(dest == NULL){
        fprintf(stderr, "Error : cannot create the value (dest is NULL)\n");
        return 1;
    }
    
    *dest = initJsonValue(JSON_STRING);
    dest->value.string = NULL;
    int state;
    
    state = _strecp(&dest->value.string,str);
    if(state){
        fprintf(stderr, "Error : cannot create the string (str is NULL or out of memory)\n");
        dest->type = JSON_ERROR;
        dest->value.string = "out of memory";
        return 1;
    }
    
    return 0;
}

int newJsonNull(JsonValue* dest){
    if(dest == NULL){
        fprintf(stderr, "Error : cannot create the value (dest is NULL)\n");
        return 1;
    }
    
    *dest = initJsonValue(JSON_NULL);
    dest->value.integer = 0;
    
    return 0;
}

int newJsonBool(JsonValue* dest,int _bool){
    if(dest == NULL){
        fprintf(stderr, "Error : cannot create the value (dest is NULL)\n");
        return 1;
    }
    if(_bool > 1 || _bool < 0){
        fprintf(stderr, "Error : a boolean must be 0 or 1 (got %d)\n", _bool);
        *dest = initJsonValue(JSON_ERROR);
        dest->value.string = "an error occurred in the initialisation of a boolean value (!0 or 1)"; 
        return 1;
    }
    
    *dest = initJsonValue(JSON_BOOL);
    dest->value.integer = _bool;
    
    return 0;
}

int newJsonNumber(JsonValue* dest,long long int _int){
    if(dest == NULL){
        fprintf(stderr, "Error : cannot create the value (dest is NULL)\n");
        return 1;
    }
    
    *dest = initJsonValue(JSON_NUMBER);
    dest->value.integer = _int;
    
    return 0;
}

int newJsonDecimal(JsonValue* dest,double _dec){
    if(dest == NULL){
        fprintf(stderr, "Error : cannot create the value (dest is NULL)\n");
        return 1;
    }

    if(_dec - _dec != 0.0){
        fprintf(stderr, "Error : NaN or infinity is not allowed in JSON\n");
        *dest = initJsonValue(JSON_ERROR);
        dest->value.string = "NaN or infinity is not allowed";
        return 1;
    }
    
    *dest = initJsonValue(JSON_DECIMAL);
    dest->value.decimal = _dec;
    
    return 0;
}

int newJsonExponencial(JsonValue* dest,char* exp){
    if(dest == NULL){
        fprintf(stderr, "Error : cannot create the value (dest is NULL)\n");
        return 1;
    }
    if(exp == NULL){
        fprintf(stderr, "Error : cannot create the number (text is NULL)\n");
        *dest = initJsonValue(JSON_ERROR);
        dest->value.string = "number text is NULL";
        return 1;
    }

    // Le texte est écrit tel quel (sans guillemets) : il doit être un nombre JSON valide, et rien d'autre.
    // validateNumber s'arrête à la fin du nombre : pos doit donc arriver au bout ("12abc" s'arrête sur 'a')
    size_t len = _strlen(exp);
    size_t pos = 0;
    if(validateNumber(exp, len, &pos) != 0 || pos != len){
        fprintf(stderr, "Error : cannot create the number (invalid JSON number)\n");
        *dest = initJsonValue(JSON_ERROR);
        dest->value.string = "invalid number";
        return 1;
    }

    *dest = initJsonValue(JSON_EXPONENTIAL);
    dest->value.string = NULL;
    int state;
    
    state = _strcpy(&dest->value.string,exp);
    if(state){
        fprintf(stderr, "Error : allocation failed while creating a number\n");
        dest->type = JSON_ERROR;
        dest->value.string = "out of memory";
        return 1;
    }
    
    return 0;
}



//////////////////////////////////////////// add function ////////////////////////////////////////////

int addToObject(JsonObject* obj, char* key, JsonValue value,size_t index){
    if(obj == NULL || key == NULL || obj->nbOfElement<index){
        fprintf(stderr, "Error : cannot add to the object (obj or key is NULL, or index out of range)\n");
        return 1;
    }

    char* cpy_key = NULL;
    if(_strecp(&cpy_key, key)){
        fprintf(stderr, "Error : allocation failed while copying the key\n");
        return 1;
    }

    if(findKey(obj,cpy_key) != (size_t)-1){
        fprintf(stderr, "Error : cannot add to the object (the key already exists)\n");
        stringToZero(cpy_key, _strlen(cpy_key));
        free(cpy_key);
        return 1;
    }

    JsonValue cpy_value ;
    if(cpyValue(&cpy_value, &value)){
        fprintf(stderr, "Error : cannot add to the object (copy of the value failed)\n");
        stringToZero(cpy_key, _strlen(cpy_key));
        free(cpy_key);
        return 1;
    }

    JsonPair* newListe = (JsonPair*)realloc(obj->listeOfPair, (obj->nbOfElement + 1) * sizeof(JsonPair));
    if(newListe == NULL){
        fprintf(stderr, "Error : allocation failed while adding to the object\n");
        stringToZero(cpy_key, _strlen(cpy_key));
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
        fprintf(stderr, "Error : cannot add to the array (ary is NULL or index out of range)\n");
        return 1;
    }

    JsonValue cpy_value;
    if(cpyValue(&cpy_value, &value)){
        fprintf(stderr, "Error : cannot add to the array (copy of the value failed)\n");
        return 1;
    }

    JsonValue* newListe = (JsonValue*)realloc(ary->listeOfValue, (ary->nbOfElement + 1) * sizeof(JsonValue));
    if(newListe == NULL){
        fprintf(stderr, "Error : allocation failed while adding to the array\n");
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
        fprintf(stderr, "Error : cannot remove from the object (obj is NULL or index out of range)\n");
        return 1;
    }

    stringToZero(obj->listeOfPair[index].key, _strlen(obj->listeOfPair[index].key));
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
        if(newListe != NULL){
            obj->listeOfPair = newListe;
        }
    }

    return 0;
}

int removeToArray(JsonArray* ary, size_t index){
    if(ary == NULL || index >= ary->nbOfElement){
        fprintf(stderr, "Error : cannot remove from the array (ary is NULL or index out of range)\n");
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
        if(newListe != NULL){
            ary->listeOfValue = newListe;
        }
    }

    return 0;
}

//////////////////////////////////////////// modify function ////////////////////////////////////////////

int modifyObject(JsonObject* obj,JsonPair paire,size_t index){
    if(obj == NULL || index >= obj->nbOfElement){
        fprintf(stderr, "Error : cannot modify the object (obj is NULL or index out of range)\n");
        return 1;
    }

    JsonPair newPair;
    newPair.key = NULL;
    if(_strecp(&newPair.key,paire.key)){
        fprintf(stderr, "Error : cannot modify the object (key is NULL or out of memory)\n");
        return 1;
    }

    size_t isFound = findKey(obj,newPair.key);
    if(isFound != (size_t)-1 && isFound != index){
        fprintf(stderr, "Error : cannot modify the object (the key already exists at another index)\n");
        stringToZero(newPair.key, _strlen(newPair.key));
        free(newPair.key);
        return 1;
    }

    if(cpyValue(&newPair.value,&paire.value)){
        fprintf(stderr, "Error : cannot modify the object (copy of the value failed)\n");
        stringToZero(newPair.key, _strlen(newPair.key));
        free(newPair.key);
        return 1;
    }

    stringToZero(obj->listeOfPair[index].key, _strlen(obj->listeOfPair[index].key));
    free(obj->listeOfPair[index].key);
    freeValue(obj->listeOfPair[index].value);
    obj->listeOfPair[index] = newPair;

    return 0;
}

int modifyArray(JsonArray* ary, JsonValue value, size_t index){
    if(ary == NULL || index >= (size_t)ary->nbOfElement){
        fprintf(stderr, "Error : cannot modify the array (ary is NULL or index out of range)\n");
        return 1;
    }

    JsonValue newValue;
    if(cpyValue(&newValue, &value)){
        fprintf(stderr, "Error : cannot modify the array (copy of the value failed)\n");
        return 1;
    }

    freeValue(ary->listeOfValue[index]);
    ary->listeOfValue[index] = newValue;

    return 0;
}
//////////////////////////////////////////// get function ////////////////////////////////////////////

JsonValue* getValueFromObject(JsonObject* obj,char* key){
    size_t index = getIndexFromObject(obj,key);
    if(index == (size_t)-1){
        return NULL;
    }

    return &obj->listeOfPair[index].value;
}

size_t getIndexFromObject(JsonObject* obj,char* key){
    if(obj == NULL){
        return (size_t)-1;
    }

    // les clés sont stockées échappées (k\" pour k") : on échappe la clé cherchée de la même façon
    char* cpy_key = NULL;
    if(_strecp(&cpy_key, key)){
        fprintf(stderr, "Error : cannot search the key (key is NULL or out of memory)\n");
        return (size_t)-1;
    }
    size_t index = findKey(obj,cpy_key);
    stringToZero(cpy_key, _strlen(cpy_key));
    free(cpy_key);

    return index;
}

// Cherche une clé déjà échappée (forme stockée).
// Pour addToObject / modifyObject, qui ont déjà échappé leur clé : passer par getIndexFromObject l'échapperait deux fois.
static size_t findKey(JsonObject* obj, char* storedKey){
    for(size_t i=0;i<obj->nbOfElement;i++){
        if(_strcmp(obj->listeOfPair[i].key,storedKey) == 0){
            return i;
        }
    }

    return (size_t)-1;
}

//////////////////////////////////////////// validation function ////////////////////////////////////////////
// Vérifie que le texte respecte la grammaire JSON (RFC 8259) avant le parsing.
// Les fonctions parseXXX supposent un JSON valide : c'est cette étape qui les protège
// des lectures hors limites et des valeurs mal formées.
// En cas d'erreur, *pos reste sur le caractère fautif.

static int validateJson(char* json_str, size_t size, size_t* errorPosition){
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

static int validateValue(char* json_str, size_t size, size_t* pos, int depth){
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

static int validateObject(char* json_str, size_t size, size_t* pos, int depth){
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

static int validateArray(char* json_str, size_t size, size_t* pos, int depth){
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

static int validateString(char* json_str, size_t size, size_t* pos){
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
        if(c >= 0x80){
            // Caractère non ASCII (é, €, 😀...) : ses octets doivent former de l'UTF-8 valide
            if(validateUtf8Char(json_str, size, pos)){
                return 1;
            }
            continue;   // *pos est déjà après le caractère
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

// Vérifie qu'à *pos commence un caractère UTF-8 valide (RFC 3629), et avance juste après lui.
// 0 = valide, 1 = invalide (*pos reste sur l'octet fautif, pour le message ligne/colonne).
//   1er octet 0x00-0x7F : 1 octet (ASCII)
//   1er octet 0xC2-0xDF : 2 octets      (0xC0 et 0xC1 n'existent pas : ce serait de l'ASCII écrit en trop long)
//   1er octet 0xE0-0xEF : 3 octets
//   1er octet 0xF0-0xF4 : 4 octets      (0xF5-0xFF n'existent pas : au-delà du dernier caractère Unicode)
// Les octets suivants sont toujours entre 0x80 et 0xBF, sauf 4 cas où le 2e octet a des bornes plus serrées.
static int validateUtf8Char(char* json_str, size_t size, size_t* pos){
    unsigned char c = (unsigned char)json_str[*pos];
    size_t length;
    unsigned char min = 0x80;   // bornes du 2e octet
    unsigned char max = 0xBF;

    if(c < 0x80){
        length = 1;
    }else if(c >= 0xC2 && c <= 0xDF){
        length = 2;
    }else if(c >= 0xE0 && c <= 0xEF){
        length = 3;
        if(c == 0xE0){
            min = 0xA0;   // sinon : caractère écrit sur plus d'octets que nécessaire ("overlong")
        }else if(c == 0xED){
            max = 0x9F;   // sinon : U+D800-U+DFFF, réservés aux paires UTF-16, interdits en UTF-8
        }
    }else if(c >= 0xF0 && c <= 0xF4){
        length = 4;
        if(c == 0xF0){
            min = 0x90;   // overlong
        }else if(c == 0xF4){
            max = 0x8F;   // sinon : au-delà de U+10FFFF, le dernier caractère Unicode
        }
    }else{
        return 1;         // 0x80-0xC1 ou 0xF5-0xFF ne peuvent pas commencer un caractère
    }

    if(*pos + length > size){
        return 1;         // caractère coupé par la fin du texte
    }
    for(size_t i = 1; i < length; i++){
        unsigned char next = (unsigned char)json_str[*pos + i];
        unsigned char low = (i == 1) ? min : 0x80;
        unsigned char high = (i == 1) ? max : 0xBF;
        if(next < low || next > high){
            return 1;     // octet de suite manquant ou hors bornes
        }
    }
    *pos += length;
    return 0;
}

static int validateNumber(char* json_str, size_t size, size_t* pos){
    size_t start = *pos;
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

    // Trop long pour les buffers de parseNUMBER / parseDECIMAL / parseEXPONENTIAL
    if(*pos - start > BUFFER_DEFAULT_SIZE - 1){
        *pos = start;   // l'erreur pointera sur le début du nombre
        return 1;
    }

    return 0;
}

static int validateLiteral(char* json_str, size_t size, size_t* pos, char* literal){
    for(size_t i = 0; literal[i] != '\0'; i++){
        if(*pos >= size || json_str[*pos] != literal[i]){
            return 1;
        }
        (*pos)++;
    }
    return 0;
}

static void skipWhitespace(char* json_str, size_t size, size_t* pos){
    while(*pos < size && (json_str[*pos] == ' ' || json_str[*pos] == '\n' ||
                          json_str[*pos] == '\t' || json_str[*pos] == '\r')){
        (*pos)++;
    }
}

static int isDigitAt(char* json_str, size_t size, size_t pos){
    return pos < size && json_str[pos] >= '0' && json_str[pos] <= '9';
}

//////////////////////////////////////////// parse function ////////////////////////////////////////////

static JsonValue parseOBJ(char* json_str, size_t* position){
    debug("parseOBJ");
    JsonValue obj_value = initJsonValue(JSON_ERROR);
    int state;

    size_t capacity = 10;
    obj_value.type = JSON_OBJECT;
    obj_value.value.object = (JsonObject*)malloc(sizeof(JsonObject));
    if(obj_value.value.object == NULL){
        obj_value = initJsonValue(JSON_ERROR);
        obj_value.value.string = "out of memory";
        return obj_value;
    }
    obj_value.value.object->nbOfElement = 0;
    obj_value.value.object->listeOfPair = (JsonPair*)calloc(10, sizeof(JsonPair));
    if(obj_value.value.object->listeOfPair == NULL){
        free(obj_value.value.object);
        obj_value = initJsonValue(JSON_ERROR);
        obj_value.value.string = "out of memory";
        return obj_value;
    }

    JsonType targeted_type;
    JsonPair buffeur_pair;
    JsonValue buffeur_value;
    buffeur_value.value.string = NULL;

    size_t NumberOfElement = 0;
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
                obj_value.value.string = "out of memory";
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
            obj_value.value.string = "object key is not a string";
            return obj_value;
        }
        buffeur_pair.key = NULL;
        state = _strcpy(&buffeur_pair.key, buffeur_value.value.string);
        for(size_t i=0;i<NumberOfElement;i++){
            if(_strcmp(buffeur_pair.key,obj_value.value.object->listeOfPair[i].key) == 0){
                fprintf(stderr, "Error : duplicate key in an object\n");
                stringToZero(buffeur_pair.key, _strlen(buffeur_pair.key));
                free(buffeur_pair.key);
                freeValue(buffeur_value);
                obj_value.value.object->nbOfElement = NumberOfElement;
                freeObject(obj_value.value.object);
                obj_value = initJsonValue(JSON_ERROR);
                obj_value.value.string = "duplicate key";
                return obj_value;
            }
        }
        freeValue(buffeur_value);
        if(state){
            obj_value.value.object->nbOfElement = NumberOfElement;
            freeObject(obj_value.value.object);
            obj_value = initJsonValue(JSON_ERROR);
            obj_value.value.string = "out of memory";
            return obj_value;
        }
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
                errorValue.value.string = "unknown value type";
                
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
            // On garde la vraie cause si l'enfant en a donné une ("duplicate key"...), lue avant le freeObject
            char* cause = "invalid value in object";
            if(buffeur_pair.value.type == JSON_ERROR && buffeur_pair.value.value.string != NULL){
                cause = buffeur_pair.value.value.string;
            }
            obj_value.value.object->nbOfElement = NumberOfElement;
            freeObject(obj_value.value.object);
            obj_value = initJsonValue(JSON_ERROR);
            obj_value.value.string = cause;
            return obj_value;
        }
    }
    
    *position = index + 1;
    obj_value.value.object->nbOfElement = NumberOfElement;
    return obj_value;
}

static JsonValue parseARRAY(char* json_str, size_t* position){
    debug("parseARRAY");
    JsonValue ary_value = initJsonValue(JSON_ERROR);

    size_t capacity = 10;
    ary_value.type = JSON_ARRAY;
    ary_value.value.array = (JsonArray*)malloc(sizeof(JsonArray));
    if(ary_value.value.array == NULL){
        ary_value = initJsonValue(JSON_ERROR);
        ary_value.value.string = "out of memory";
        return ary_value;
    }
    ary_value.value.array->nbOfElement = 0;
    ary_value.value.array->listeOfValue = (JsonValue*)calloc(capacity, sizeof(JsonValue));
    if(ary_value.value.array->listeOfValue == NULL){
        free(ary_value.value.array);
        ary_value = initJsonValue(JSON_ERROR);
        ary_value.value.string = "out of memory";
        return ary_value;
    }

    
    JsonType targeted_type;
    JsonValue buffeur;

    size_t NumberOfElement = 0;
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
                ary_value.value.string = "out of memory";
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
                buffeur.value.string = "unknown value type";

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
            // On garde la vraie cause si l'enfant en a donné une ("duplicate key"...), lue avant le freeArray
            char* cause = "invalid value in array";
            if(buffeur.type == JSON_ERROR && buffeur.value.string != NULL){
                cause = buffeur.value.string;
            }
            ary_value.value.array->nbOfElement = NumberOfElement;
            freeArray(ary_value.value.array);
            ary_value = initJsonValue(JSON_ERROR);
            ary_value.value.string = cause;
            return ary_value;
        }
    }

    *position = index + 1;
    ary_value.value.array->nbOfElement = NumberOfElement;
    return ary_value;
}

static JsonValue parseSTRING(char* json_str,size_t* position){
    debug("parseSTRING");
    JsonValue str_value = initJsonValue(JSON_ERROR);
    str_value.type = JSON_STRING;
    str_value.value.string = NULL;

    size_t start = *position;

    if(json_str[start] != '"') {
        str_value.type = JSON_ERROR;
        str_value.value.string = "string without opening quote";
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
        str_value.value.string = "string without closing quote";
        return str_value;
    }

    if(end == start + 1) {
        if(_strcpy(&str_value.value.string, "")){
            str_value.type = JSON_ERROR;
            str_value.value.string = "out of memory";
            return str_value;
        };
    }else if(end > start) {
        if(_strcpybxy(&str_value.value.string, json_str, start + 1, end - 1)){
            str_value.type = JSON_ERROR;
            str_value.value.string = "out of memory";
            return str_value;
        }
    }

    *position = end + 1;
    return str_value;
}

static JsonValue parseEXPONENTIAL(char* json_str,size_t* position){
    debug("parseEXPONENTIAL");
    JsonValue exp_value = initJsonValue(JSON_ERROR);

    exp_value.type = JSON_EXPONENTIAL;
    exp_value.value.string = (char*)calloc(BUFFER_DEFAULT_SIZE,sizeof(char));
    if(exp_value.value.string == NULL){
        exp_value = initJsonValue(JSON_ERROR);
        exp_value.value.string = "out of memory";
        return exp_value;
    }
    int numberOfDigit = 0;
    int numberOfDot = 0;

    for(size_t i = *position;json_str[i] != '\0';i++){
        char c = json_str[i];
        int isNumberChar = (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '+' || c == 'e' || c == 'E';
        // Buffer plein ET encore un caractère du nombre à ajouter -> trop long (comme dans parseNUMBER)
        if(numberOfDot > 1 || (isNumberChar && numberOfDigit >= BUFFER_DEFAULT_SIZE - 1)){
            free(exp_value.value.string);
            exp_value = initJsonValue(JSON_ERROR);
            exp_value.value.string = "invalid exponent";
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
                    exp_value.value.string = "invalid exponent";
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
                    exp_value.value.string = "invalid exponent";
                    return exp_value;
                }
            }else{
                free(exp_value.value.string);
                exp_value = initJsonValue(JSON_ERROR);
                exp_value.value.string = "invalid exponent";
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

static JsonValue parseNUMBER(char* json_str,size_t* position){
    debug("parseNUMBER");
    JsonValue nbr_value = initJsonValue(JSON_ERROR);

    char* number_str = (char*)calloc(BUFFER_DEFAULT_SIZE, sizeof(char));
    if(number_str == NULL){
        nbr_value = initJsonValue(JSON_ERROR);
        nbr_value.value.string = "out of memory";
        return nbr_value;
    }
    long long int number_int;
    size_t numberOfDigit = 0;
    int state;

    
    nbr_value.type = JSON_NUMBER;

    for(size_t index=*position;json_str[index] != '\0';index++){
        if((json_str[index] >= 48 && json_str[index] <= 57) || json_str[index] == '-'){
            if(numberOfDigit >= BUFFER_DEFAULT_SIZE - 1){
                nbr_value = initJsonValue(JSON_ERROR);
                nbr_value.value.string = "number too long";
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
    state = _strtoi(number_str,&number_int);
    if(state == 1){
        // Trop long pour un long long -> on le garde en double (valeur approchée)
        double number_double;
        if(_strtod(number_str, &number_double)){
            nbr_value.type = JSON_EXPONENTIAL;
            nbr_value.value.string = number_str;   // pas de free : la valeur garde le texte
            *position = *position + numberOfDigit;
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

static JsonValue parseBOOL(char* json_str, size_t* position){
    debug("parseBOOL");
    JsonValue boo_value = initJsonValue(JSON_ERROR);
    boo_value.value.string = "invalid boolean";   // remplacé si on trouve true ou false

    if(json_str[*position] == '\0'){
        return boo_value;
    }

    // indexEnd = premier délimiteur, ou le '\0' final s'il n'y en a pas
    size_t indexEnd = *position;
    while(json_str[indexEnd] != '\0' && json_str[indexEnd] != ',' && json_str[indexEnd] != '}' && json_str[indexEnd] != ']'){
        indexEnd++;
    }
    if(indexEnd == *position){
        return boo_value;   // rien avant le délimiteur (sinon indexEnd - 1 passerait sous 0)
    }

    char* buffer = NULL;   // _strcpybxy alloue elle-même la bonne taille
    
    int state = _strcpybxy(&buffer, json_str, *position, indexEnd - 1);
    if(state == 1){
        boo_value.value.string = "out of memory";
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

static JsonValue parseNULL(char* json_str,size_t* position){
    JsonValue null_value = initJsonValue(JSON_ERROR);
    null_value.value.string = "invalid null";   // remplacé si on trouve null
    int state;

    // indexEnd = premier délimiteur, ou le '\0' final s'il n'y en a pas
    size_t indexEnd = *position;
    while(json_str[indexEnd] != '\0' && json_str[indexEnd] != ',' && json_str[indexEnd] != '}' && json_str[indexEnd] != ']'){
        indexEnd++;
    }
    if(*position == indexEnd){
        return null_value;
    }

    char* buffer = NULL;   // _strcpybxy alloue elle-même la bonne taille
    state = _strcpybxy(&buffer, json_str, *position, indexEnd - 1);
    if(state == 1){
        null_value.value.string = "out of memory";
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

static JsonValue parseDECIMAL(char* json_str, size_t* position){
    debug("parseDECIMAL");
    JsonValue dec_value = initJsonValue(JSON_ERROR);

    char* decimal_str = (char*)calloc(BUFFER_DEFAULT_SIZE, sizeof(char));
    if(decimal_str == NULL){
        dec_value = initJsonValue(JSON_ERROR);
        dec_value.value.string = "out of memory";
        return dec_value;
    }
    double decimal_double;
    size_t numberOfDigit = 0;
    
    int state;

    dec_value.type = JSON_DECIMAL;

    for(size_t index = *position; json_str[index] != '\0'; index++){
        if((json_str[index] >= 48 && json_str[index] <= 57) ||
            json_str[index] == '-' || json_str[index] == '.'){
            // Testé seulement quand il y a un caractère à ajouter, comme dans parseNUMBER
            if(numberOfDigit >= BUFFER_DEFAULT_SIZE - 1){
                dec_value = initJsonValue(JSON_ERROR);
                dec_value.value.string = "number too long";
                free(decimal_str);
                return dec_value;
            }
            numberOfDigit++;
            decimal_str[numberOfDigit-1] = json_str[index];
        }else{
            break;
        }
    }
    decimal_str[numberOfDigit] = '\0';
    
    state = _strtod(decimal_str, &decimal_double);
    if(state == 1){
        dec_value.type = JSON_EXPONENTIAL;
        dec_value.value.string = decimal_str;   // pas de free : la valeur garde le texte
        *position = *position + numberOfDigit;
        return dec_value;
    }
    
    dec_value.value.decimal = decimal_double;
    *position = *position + numberOfDigit;  
    free(decimal_str);
    return dec_value;
}

static JsonValue parseValue(char* json_str, size_t* position) {
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
            error_value.value.string = "unknown value type";
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
            stringToZero(value.value.string, _strlen(value.value.string));
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
    for(size_t i=0;i<obj->nbOfElement;i++){
        stringToZero(obj->listeOfPair[i].key, _strlen(obj->listeOfPair[i].key));
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
                stringToZero(obj->listeOfPair[i].value.value.string, _strlen(obj->listeOfPair[i].value.value.string));
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
    for(size_t i = 0; i < ary->nbOfElement; i++){
        switch(ary->listeOfValue[i].type){
            case JSON_BOOL:
            case JSON_DECIMAL:
            case JSON_NULL:
            case JSON_NUMBER:
            case JSON_ERROR:
            break;
            case JSON_STRING:
            case JSON_EXPONENTIAL:{
                stringToZero(ary->listeOfValue[i].value.string, _strlen(ary->listeOfValue[i].value.string));
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

// volatile : le compilateur n'a pas le droit de supprimer ces écritures,
// même si le bloc est libéré juste après et que personne ne relit les zéros
void stringToZero(char* str,size_t size){
    if(str == NULL){
        return;
    }
    volatile char* p = str;
    for(size_t i=0;i<size;i++){
        p[i] = 0x00;
    }
}

//////////////////////////////////////////// cpy function ////////////////////////////////////////////

int cpyValue(JsonValue* dest,JsonValue* src){
    if(dest == NULL ||src == NULL){
        fprintf(stderr, "Error : cannot copy the value (dest or src is NULL)\n");
        return 1;
    }else if(dest == src){
        fprintf(stderr, "Error : cannot copy the value (dest[%p] and src[%p] are the same pointer)\n", (void*)dest, (void*)src);
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
                dest->value.string = "invalid boolean";
                return 1;
            }
        break;
        case JSON_NUMBER:
            dest->value.integer = src->value.integer;
        break;
        case JSON_STRING :
            dest->value.string = NULL;
            state = _strcpy(&dest->value.string,src->value.string);
            if(state){
                fprintf(stderr, "Error : copy of a string failed\n");
                free(dest->value.string);
                return 1;
            }
        break;
        case JSON_ARRAY :
            dest->value.array = (JsonArray*)malloc(sizeof(JsonArray));
            if(dest->value.array == NULL){
                fprintf(stderr, "Error : allocation failed while copying an array\n");
                *dest = initJsonValue(JSON_ERROR);   // sinon dest resterait un array qui pointe sur NULL
                dest->value.string = "out of memory";
                return 1;
            }
            dest->value.array->nbOfElement = 0;
            dest->value.array->listeOfValue = NULL;

            state = cpyArray(dest->value.array,src->value.array);
            if(state){
                fprintf(stderr, "Error : copy of an array failed\n");
                free(dest->value.array);
                *dest = initJsonValue(JSON_ERROR);
                dest->value.string = "copy failed";
                return 1;
            }
        break;
        case JSON_OBJECT :
            dest->value.object = (JsonObject*)malloc(sizeof(JsonObject));
            if(dest->value.object == NULL){
                fprintf(stderr, "Error : allocation failed while copying an object\n");
                *dest = initJsonValue(JSON_ERROR);   // sinon dest resterait un object qui pointe sur NULL
                dest->value.string = "out of memory";
                return 1;
            }
            dest->value.object->nbOfElement = 0;
            dest->value.object->listeOfPair = NULL;

            state = cpyObject(dest->value.object,src->value.object);
            if(state){
                fprintf(stderr, "Error : copy of an object failed\n");
                free(dest->value.object);
                *dest = initJsonValue(JSON_ERROR);
                dest->value.string = "copy failed";
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
                fprintf(stderr, "Error : copy of an exponential number failed\n");
                free(dest->value.string);
                return 1;
            }
        break;
        case JSON_ERROR :
            dest->value.string = src->value.string;   // message constant : on copie juste le pointeur
        break;
        default :
            fprintf(stderr, "Error : unknown type\n");
    }
    return 0;
}

int cpyObject(JsonObject* dest,JsonObject* src){
    if(dest == NULL ||src == NULL){
        fprintf(stderr, "Error : cannot copy the object (dest or src is NULL)\n");
        return 1;
    }
    if(dest == src){
        fprintf(stderr, "Error : cannot copy the object (dest[%p] and src[%p] are the same pointer)\n", (void*)dest, (void*)src);
        return 1;
    }
    int state;

    if(dest->listeOfPair == NULL){
        dest->listeOfPair = (JsonPair*)calloc(src->nbOfElement, sizeof(JsonPair));
        if(dest->listeOfPair == NULL && src->nbOfElement > 0){
            fprintf(stderr, "Error : allocation failed while copying an object\n");
            return 1;
        }
    }else{
        if(dest->listeOfPair != NULL){
            for(size_t i = 0; i < dest->nbOfElement; i++){
                stringToZero(dest->listeOfPair[i].key, _strlen(dest->listeOfPair[i].key));
                free(dest->listeOfPair[i].key);
                freeValue(dest->listeOfPair[i].value);
            }
            free(dest->listeOfPair);
            dest->listeOfPair = NULL;
        }
        dest->listeOfPair = (JsonPair*)calloc(src->nbOfElement, sizeof(JsonPair));
        if(dest->listeOfPair == NULL && src->nbOfElement > 0){
            return 1;
        }
    }

    dest->nbOfElement = src->nbOfElement;
    for(size_t i=0;i<dest->nbOfElement;i++){
        state = _strcpy(&dest->listeOfPair[i].key,src->listeOfPair[i].key);
        if(state){
            fprintf(stderr, "Error : copy of a key failed\n");
            for(size_t j = 0; j < i; j++){
                stringToZero(dest->listeOfPair[j].key, _strlen(dest->listeOfPair[j].key));
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
            fprintf(stderr, "Error : copy of the value of a key failed\n");
            stringToZero(dest->listeOfPair[i].key, _strlen(dest->listeOfPair[i].key));
            free(dest->listeOfPair[i].key);
            for(size_t j = 0; j < i; j++){
                stringToZero(dest->listeOfPair[j].key, _strlen(dest->listeOfPair[j].key));
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
        fprintf(stderr, "Error : cannot copy the array (dest or src is NULL)\n");
        return 1;
    }
    if(dest == src){
        fprintf(stderr, "Error : cannot copy the array (dest[%p] and src[%p] are the same pointer)\n", (void*)dest, (void*)src);
        return 1;
    }
    int state;

    // dest contient déjà des valeurs -> on les libère, sa liste peut être plus petite que src
    if(dest->listeOfValue != NULL){
        for(size_t i = 0; i < dest->nbOfElement; i++){
            freeValue(dest->listeOfValue[i]);
        }
        free(dest->listeOfValue);
        dest->listeOfValue = NULL;
        dest->nbOfElement = 0;
    }

    dest->listeOfValue = (JsonValue*)calloc(src->nbOfElement,sizeof(JsonValue));
    if(dest->listeOfValue == NULL && src->nbOfElement > 0){
        fprintf(stderr, "Error : allocation failed while copying an array\n");
        return 1;
    }

    dest->nbOfElement = src->nbOfElement;
    for(size_t i=0; i < src->nbOfElement; i++){
        state = cpyValue(&dest->listeOfValue[i],&src->listeOfValue[i]);
        if(state){
            fprintf(stderr, "Error : copy of an array element failed\n");
            for(size_t j = 0; j < i; j++){
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

static int loadJson(char** dest,FILE* file){
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
        char* tmp = (char*)realloc(*dest,(size+1)*sizeof(char));
        if(tmp == NULL){
            return 1;
        }
        *dest = tmp;
    }
    // On lit tout le fichier et le décale dans dest
    long read = fread(*dest,1,size,file);
    if(read < size){
        stringToZero(*dest, (size_t)size);
        free(*dest);
        *dest = NULL;
        return 1;
    }
    (*dest)[size] = '\0';
    if((long)_strlen(*dest) < size){
        stringToZero(*dest, (size_t)size);   // le texte après l'octet NUL peut contenir des mots de passe
        free(*dest);
        *dest = NULL;
        return 2;
    }
    return 0;
}

// Retire les espaces hors des strings en une seule passe.
// dest peut pointer sur src (nettoyage sur place) : indexWrite <= i, on n'écrase jamais un char pas encore lu.
// Si *dest est NULL, un buffer de _strlen(src)+1 octets est alloué (le résultat n'est jamais plus long que src).
static int whitespaceCleaner(char* src,char** dest){
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

static JsonType getType(char* json_str,size_t position){
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
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
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

static long getSize(FILE* file){
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    return size;
}

static int isEscaped(char* str, size_t position) {
    size_t count = 0;

    while (position > 0 && str[position - 1] == '\\') {
        count++;
        position--;
    }

    return count % 2 != 0;
}

//////////////////////////////////////////// treatment function ////////////////////////////////////////////

static size_t _strlen(char* str){
    if(str == NULL){
        return -1;
    }
    size_t i =0;
    while(str[i] != '\0'){
        i++;
    }
    return i;
}

// 0 -> Same | 1 -> Not the same
static int _strcmp(char* str1,char* str2){
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

// String copy between x and y
static int _strcpybxy(char **dest, char *src, size_t x, size_t y){
    if(src == NULL){
        return 1;
    }

    if (y < x) {
        return 1;
    }

    size_t size = y - x + 2;
    // realloc dans tmp : s'il échoue, *dest garde l'ancien bloc (sinon il fuit)
    char* tmp;
    if (*dest == NULL) {
        tmp = malloc(size);
    } else {
        tmp = realloc(*dest, size);
    }
    if (tmp == NULL) {
        return 1;
    }
    *dest = tmp;

    size_t index = 0;
    for (size_t i = x; i <= y; i++) {
        (*dest)[index] = src[i];
        index++;
    }

    (*dest)[index] = '\0';
    return 0;
}

static int _strcpy(char** dest,char* src){
    if(src == NULL){
        return 1;
    }
    size_t size = _strlen(src) + 1;

    // realloc dans tmp : s'il échoue, *dest garde l'ancien bloc (sinon il fuit)
    char* tmp;
    if (*dest == NULL) {
        tmp = malloc(size);
    } else {
        tmp = realloc(*dest, size);
    }
    if (tmp == NULL) {
        return 1;
    }
    *dest = tmp;

    for(size_t i = 0;i<size;i++){
        (*dest)[i] = src[i];
    }
    (*dest)[size-1] = '\0';
    return 0;
}

// string echapement
static int _strecp(char** dest,char* src){
    if(src == NULL || *dest != NULL){
        return 1;
    }

    // 1er passage : la taille finale, pour un seul malloc
    // (un realloc qui déplace le bloc libère l'ancien sans l'effacer : le mot de passe y resterait)
    size_t newSize = 0;
    for(size_t i=0;src[i] != '\0';i++){
        if(src[i] == '\\' || src[i] == '"'){
            newSize +=2;
        }else if(src[i] == '\n' || src[i] == '\r' || src[i] == '\b' || src[i] == '\f' || src[i] == '\t'){
            newSize +=2;
        }else if((unsigned char)src[i] < 0x20){
            newSize +=6;
        }else{
            newSize ++;
        }
    }
    newSize++;  // le '\0' final

    *dest = malloc(newSize);
    if(*dest == NULL){
        return 1;
    }

    size_t read = 0;
    size_t write = 0;
    char c;

    for(read = 0; src[read] != '\0'; read++){
        switch(src[read]){
            case '\\':
            case '"':
                (*dest)[write] = '\\';
                (*dest)[write+1] = src[read];
                write += 2;
            break;
            case '\n':
            case '\r':
            case '\b':
            case '\f':
            case '\t':
                c = witchEscapeChar(src[read]);
                if(c == 'x'){
                    stringToZero(*dest, write);
                    free(*dest);
                    *dest = NULL;
                    return 1;
                }
                (*dest)[write] = '\\';
                (*dest)[write+1] = c;
                write += 2;
            break;
            default:{
                if((unsigned char)src[read] < 0x20){
                    (*dest)[write] = '\\';
                    (*dest)[write+1] = 'u';
                    (*dest)[write+2] = '0';
                    (*dest)[write+3] = '0';
                    (*dest)[write+4] = HEXA_TAB[(src[read]) >> 4];
                    (*dest)[write+5] = HEXA_TAB[(src[read]) & 0xF];
                    write += 6;
                    break;
                }
                (*dest)[write] = src[read];
                write ++;
            break;
            }
        }
    }
    (*dest)[write] = '\0';
    return 0;
}

// string undo echapement (publique : voir cson.h)
int unescapeString(char** dest,char* src){
    if(src == NULL || *dest != NULL){
        return 1;
    }
    size_t size = _strlen(src) + 1;

    // le texte décodé est toujours plus court ou égal : un seul malloc suffit
    *dest = malloc(size);
    if(*dest == NULL){
        return 1;
    }

    size_t read = 0;
    size_t write = 0;
    char c;

    for(read = 0; src[read] != '\0'; read++){
        if(src[read] == '\\'){
            switch(src[read+1]){
                case '\\':
                case '"':
                case '/':
                    (*dest)[write] = src[read+1];
                    write++;
                    read++;     // saute le caractère après '\'
                break;
                case 'n':
                case 'r':
                case 'b':
                case 'f':
                case 't':
                    c = witchCharEscape(src[read+1]);
                    if(c == 'x'){
                        stringToZero(*dest, write);
                        free(*dest);
                        *dest = NULL;
                        return 1;
                    }
                    (*dest)[write] = c;
                    write++;
                    read++;     // saute la lettre
                break;
                case 'u':{
                    int accumulateur = 0;
                    int pos;
                    for(int i=1;i<5;i++){
                        pos = getPosInHEXA_TAB(src[read+1+i]);
                        if(pos == -1){
                            stringToZero(*dest, write);
                            free(*dest);
                            *dest = NULL;
                            return 1;
                        }
                        accumulateur = accumulateur * 16 + pos;
                    }
                    if(accumulateur == 0 || accumulateur >= 0x80){
                        stringToZero(*dest, write);
                        free(*dest);
                        *dest = NULL;
                        return 1;
                    }
                    (*dest)[write] = accumulateur;
                    write++;
                    read += 5;  // saute 'u' + les 4 chiffres
                break;
                }
                default:
                    // lettre inconnue après '\', ou '\' en fin de chaîne ('\0')
                    stringToZero(*dest, write);
                    free(*dest);
                    *dest = NULL;
                    return 1;
            }
        }else{
            (*dest)[write] = src[read];
            write++;
        }
    }
    (*dest)[write] = '\0';
    return 0;
}

static int _strtoi(char* str,long long int* res){
    if(str == NULL){
        return 1;
    }
    long long int result = 0;
    int boolNeg = 0;
    size_t i = 0;
    size_t len = _strlen(str);
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


    // On calcule en NÉGATIF : un long long va jusqu'à -9223372036854775808,
    // mais seulement jusqu'à +9223372036854775807. Le minimum n'a pas d'équivalent positif.
    for(; i < len; i++){
        if(str[i] < '0' || str[i] > '9'){
            return 1; // caractère invalide
        }
        int digit = str[i] - '0';
        if(result < (INT64_MIN + digit) / 10){
            return 1; // result * 10 - digit passerait sous INT64_MIN -> parseNUMBER le gardera autrement
        }
        result = result * 10 - digit;
    }
    if(boolNeg == 1){
        *res = result;
    }else{
        if(result == INT64_MIN){
            return 1; // +9223372036854775808 ne tient pas dans un long long
        }
        *res = -result;
    }
    return 0;
}

static int _strtod(char* str,double *res){
    if(str == NULL){
        return 1;
    }
    size_t sizeOfStr = _strlen(str);
    if(sizeOfStr == (size_t)-1 || sizeOfStr == 0){
        return 1;
    }
    if(sizeOfStr == 1 && str[0] == '-'){
        return 1;
    }
    int boolNeg = 0;
    if(str[0] == '-'){
        boolNeg = 1;
    }

    long long int mantisse = 0;
    int nbrSignificatif = 0;
    int nbrApresVirgule = 0;
    int boolAfterDot = 0;
    for(size_t i = 0;i<sizeOfStr;i++){
        if((str[i] >= '0' && str[i] <= '9') || str[i] == '-' || str[i] == '.'){
            if(str[i] >= '0' && str[i] <= '9'){
                mantisse = mantisse * 10 + (str[i] - '0');
                if(nbrSignificatif > 0 || str[i] != '0'){
                    nbrSignificatif ++;
                    if(nbrSignificatif > 15){
                        return 1;
                    }
                }
            }
            if(boolAfterDot == 1){
                nbrApresVirgule ++;
                if(nbrApresVirgule > 22){
                    return 1;
                }
            }
            if(str[i] == '.'){
                if(boolAfterDot == 1){
                    return 1;
                }else{
                    boolAfterDot = 1;
                }
            }
        }else{
            return 1;
        }
    }
    double power = 1;
    for(int i=0;i<nbrApresVirgule;i++){
        power = power * 10.0;
    }
    *res = (double)mantisse / power;
    if(boolNeg == 1){
        *res = -1.0 * (*res);
    }

    return 0;
}

static char witchEscapeChar(char c){
    switch(c){
        case '\n':
            return 'n';
        break;
        case '\t':
            return 't';
        break;
        case '\r':
            return 'r';
        break;
        case '\f':
            return 'f';
        break;
        case '\b':
            return 'b';
        break;
        default : 
            return 'x';
    }
}

static char witchCharEscape(char c){
    switch(c){
        case 'n':
            return '\n';
        break;
        case 't':
            return '\t';
        break;
        case 'r':
            return '\r';
        break;
        case 'f':
            return '\f';
        break;
        case 'b':
            return '\b';
        break;
        default : 
            return 'x';
    }
}

static int getPosInHEXA_TAB(char c){
    if(c >= '0' && c <= '9'){
        return c - '0';
    }else if(c >= 'a' && c <= 'f'){
        return c - 'a' + 10;
    }else if(c >= 'A' && c <= 'F'){
        return c - 'A' + 10;
    }
    // +10 car dans "0123456789abcdef" il y'a 0[...]9

    return -1;
}

////////////////////////////////////////////////////

static void debug(char* str){

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