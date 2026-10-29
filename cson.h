#ifndef CSON_H
#define CSON_H

// CSON : lecture, modification et écriture de JSON en C (voir README.md).
// Seules les fonctions déclarées ici font partie de la bibliothèque.
// Convention : les fonctions qui renvoient un int renvoient 0 si tout va bien, 1 en cas d'erreur.

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

//////////////////////////////////////////// types ////////////////////////////////////////////

// Liste de type que peux prendre un JSON (et le champ de value à lire pour chacun)
typedef enum {
    JSON_NULL,          // null           (rien à lire)
    JSON_BOOL,          // true / false   -> value.integer (1 ou 0)
    JSON_NUMBER,        // entier         -> value.integer (long long)
    JSON_STRING,        // "texte"        -> value.string  (gardé tel quel, échappements compris)
    JSON_ARRAY,         // [ ... ]        -> value.array
    JSON_OBJECT,        // { ... }        -> value.object
    JSON_DECIMAL,       // décimal        -> value.decimal (double)
    JSON_EXPONENTIAL,   // nombre gardé en texte (exposant, ou trop précis pour un double) -> value.string
    JSON_ERROR          // erreur         -> value.string  (message constant, à ne jamais libérer)
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
    size_t nbOfElement;
    JsonValue * listeOfValue;
}JsonArray;

typedef struct JsonObject{
    size_t nbOfElement;
    JsonPair * listeOfPair;
    char** listeOfSortedKey;
}JsonObject;

//////////////////////////////////////////// lecture ////////////////////////////////////////////

// Lit et valide le fichier JSON `path`. Renvoie la valeur racine (à libérer avec freeValue),
// ou un JSON_ERROR dont value.string explique la cause (un message détaillé est aussi écrit sur stderr).
JsonValue initCsonFromFile(char* path);

// Pareil, mais depuis un texte JSON déjà en mémoire (par exemple un coffre déchiffré).
// Attention : `json` est MODIFIÉ (les espaces hors des strings sont retirés), il doit donc être modifiable :
// un tableau ou un bloc alloué, pas un texte écrit entre guillemets (initCson("{...}") plante).
// initCson ne libère pas `json` et ne l'efface pas : c'est à l'appelant de le faire (stringToZero puis free).
JsonValue initCson(char* json);

// Valeur vide du type demandé, sans allocation. Pour un tableau ou un objet, utiliser plutôt newJsonArray / newJsonObject.
JsonValue initJsonValue(JsonType type);

//////////////////////////////////////////// création ////////////////////////////////////////////
// En cas d'erreur, *dest devient un JSON_ERROR avec un message (sauf si dest est NULL).

int newJsonObject(JsonValue* dest);                    // objet vide {}
int newJsonArray(JsonValue* dest);                     // tableau vide []
int newJsonString(JsonValue* dest, char* str);         // copie de str, échappée au format JSON (" devient \") : relire avec unescapeString
int newJsonNull(JsonValue* dest);
int newJsonBool(JsonValue* dest, int _bool);           // 0 ou 1 uniquement
int newJsonNumber(JsonValue* dest, long long int _int);
int newJsonDecimal(JsonValue* dest, double _dec);      // NaN et infini refusés
int newJsonExponencial(JsonValue* dest, char* exp);    // texte d'un nombre JSON valide ("1e10", "-2.5E-3"...)

//////////////////////////////////////////// modification ////////////////////////////////////////////
// Les valeurs données sont COPIÉES : l'appelant garde (et libère) les siennes.
// Les clés données à addToObject / modifyObject sont échappées comme dans newJsonString.

int addToObject(JsonObject* obj, char* key, JsonValue value, size_t index);  // insère à index (0..nbOfElement), clé déjà présente refusée
int addToArray(JsonArray* ary, JsonValue value, size_t index);               // insère à index (0..nbOfElement)
int removeToObject(JsonObject* obj, size_t index);                           // supprime et libère la paire à index
int removeToArray(JsonArray* ary, size_t index);                             // supprime et libère la valeur à index
int modifyObject(JsonObject* obj, JsonPair paire, size_t index);             // remplace la paire à index (clé déjà présente ailleurs refusée)
int modifyArray(JsonArray* ary, JsonValue value, size_t index);              // remplace la valeur à index

//////////////////////////////////////////// accès ////////////////////////////////////////////

// Pointeur vers la valeur de la clé `key`, ou NULL si elle n'existe pas.
// `key` est le texte normal ("k\"" en C pour la clé k") : elle est échappée comme dans addToObject.
// Attention : ce pointeur n'est plus valable après un add / remove sur le même objet.
JsonValue* getValueFromObject(JsonObject* obj, char* key);

// Index de la clé `key` (texte normal, comme getValueFromObject), ou (size_t)-1 si elle n'existe pas.
size_t getIndexFromObject(JsonObject* obj, char* key);

// Décode une string stockée (forme JSON : \" \\ \n \u0041...) vers le texte réel, par exemple un mot de passe.
// *dest doit valoir NULL : il reçoit une nouvelle chaîne, à effacer avec stringToZero puis libérer avec free.
// Les \uXXXX sont décodés en UTF-8 (é, €, et les emojis écrits en paire \ud83d\ude00).
// Renvoie 1 si src est NULL ou mal formée, si elle contient \u0000, ou une moitié de paire seule (\uD800..\uDFFF).
int unescapeString(char** dest, char* src);

//////////////////////////////////////////// copie ////////////////////////////////////////////
// Copie complète (profonde). dest et src doivent être différents.

int cpyValue(JsonValue* dest, JsonValue* src);
int cpyObject(JsonObject* dest, JsonObject* src);
int cpyArray(JsonArray* dest, JsonArray* src);

//////////////////////////////////////////// écriture ////////////////////////////////////////////

void printfValue(JsonValue value);              // affiche à l'écran (sans retour à la ligne final)
int fprintfValue(FILE* file, JsonValue value);  // écrit dans file. Renvoie 1 si l'écriture a échoué (disque plein...)
// Texte sensible : faire setvbuf(file, NULL, _IONBF, 0) juste après fopen, sinon stdio en garde une copie
// dans un tampon que fclose libère sans l'effacer.

// Même texte que fprintfValue, mais dans un bloc mémoire (pour chiffrer un coffre sans l'écrire en clair sur le disque).
// *dest doit valoir NULL : il reçoit un nouveau texte, alloué en une seule fois à la taille exacte (aucune copie laissée
// en mémoire). À effacer avec stringToZero puis libérer avec free. Renvoie 1 si dest est NULL, *dest pas NULL, ou mémoire pleine.
int sprintfValue(char** dest, JsonValue value);

//////////////////////////////////////////// libération ////////////////////////////////////////////

void freeValue(JsonValue value);   // libère tout ce que contient la valeur (récursivement)
void freeObject(JsonObject *obj);  // libère l'objet et la structure elle-même
void freeArray(JsonArray *ary);    // libère le tableau et la structure elle-même
// Les trois effacent (mettent à 0) les strings et les clés avant de les libérer.

// Met à 0 les `size` premiers octets de str, sans que le compilateur puisse supprimer l'effacement.
// À appeler avant free() sur un texte sensible, par exemple le mot de passe rendu par unescapeString.
void stringToZero(char* str, size_t size);

#ifdef __cplusplus
}
#endif

#endif
