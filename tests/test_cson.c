// Tests de CSON, uniquement à travers l'API publique (cson.h).
// Lancement : make test        (ou make valgrind pour vérifier aussi la mémoire)
// Le programme renvoie 0 si tous les tests passent, 1 sinon.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <stdint.h>
#include "cson.h"

#define FICHIER_ENTREE "tests/tmp_entree.json"
#define FICHIER_SORTIE "tests/tmp_sortie.json"
#define FICHIER_SORTIE2 "tests/tmp_sortie2.json"

static int g_ok = 0;
static int g_ko = 0;

#define CHECK(condition, nom) do{ \
    if(condition){ g_ok++; } \
    else{ g_ko++; printf("  ECHEC : %s (ligne %d)\n", nom, __LINE__); } \
}while(0)

// Écrit n octets dans le fichier d'entrée puis le lit avec initCson
static JsonValue lireN(const char* contenu, size_t n){
    FILE* f = fopen(FICHIER_ENTREE, "wb");
    if(f == NULL){
        printf("Impossible de créer %s\n", FICHIER_ENTREE);
        exit(1);
    }
    fwrite(contenu, 1, n, f);
    fclose(f);
    return initCson(FICHIER_ENTREE);
}
#define LIRE(texte) lireN(texte, sizeof(texte) - 1)

// Contenu complet d'un fichier (à libérer avec free)
static char* contenuFichier(const char* path){
    FILE* f = fopen(path, "rb");
    if(f == NULL){
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    rewind(f);
    char* buffer = malloc((size_t)n + 1);
    if(buffer == NULL){
        fclose(f);
        return NULL;
    }
    size_t lu = fread(buffer, 1, (size_t)n, f);
    buffer[lu] = '\0';
    fclose(f);
    return buffer;
}

// Écrit v dans path, renvoie le texte écrit (à libérer avec free)
static char* ecrire(const char* path, JsonValue v){
    FILE* f = fopen(path, "wb");
    if(f == NULL){
        return NULL;
    }
    fprintfValue(f, v);
    fclose(f);
    return contenuFichier(path);
}

// Vrai si v est un JSON_ERROR dont le message vaut `message`
static int estErreur(JsonValue v, const char* message){
    return v.type == JSON_ERROR && v.value.string != NULL && strcmp(v.value.string, message) == 0;
}

static void test_lecture(void){
    printf("lecture\n");
    JsonValue v = LIRE("{\"s\": \"a\\\"b\", \"n\": -12, \"d\": 3.5, \"e\": 1e5, \"t\": true, \"f\": false, \"z\": null,"
                       " \"o\": {\"x\": [1, {}]}, \"vide\": []}");
    CHECK(v.type == JSON_OBJECT, "objet lu");
    if(v.type == JSON_OBJECT){
        JsonObject* o = v.value.object;
        JsonValue* x;
        CHECK(o->nbOfElement == 9, "9 cles");
        x = getValueFromObject(o, "s"); CHECK(x && x->type == JSON_STRING && strcmp(x->value.string, "a\\\"b") == 0, "string gardee telle quelle");
        x = getValueFromObject(o, "n"); CHECK(x && x->type == JSON_NUMBER && x->value.integer == -12, "entier negatif");
        x = getValueFromObject(o, "d"); CHECK(x && x->type == JSON_DECIMAL && x->value.decimal == 3.5, "decimal");
        x = getValueFromObject(o, "e"); CHECK(x && x->type == JSON_EXPONENTIAL && strcmp(x->value.string, "1e5") == 0, "exposant garde en texte");
        x = getValueFromObject(o, "t"); CHECK(x && x->type == JSON_BOOL && x->value.integer == 1, "true");
        x = getValueFromObject(o, "f"); CHECK(x && x->type == JSON_BOOL && x->value.integer == 0, "false");
        x = getValueFromObject(o, "z"); CHECK(x && x->type == JSON_NULL, "null");
        x = getValueFromObject(o, "o"); CHECK(x && x->type == JSON_OBJECT && x->value.object->nbOfElement == 1, "objet imbrique");
        x = getValueFromObject(o, "vide"); CHECK(x && x->type == JSON_ARRAY && x->value.array->nbOfElement == 0, "tableau vide");
        CHECK(getValueFromObject(o, "absente") == NULL, "cle absente -> NULL");
        CHECK(getIndexFromObject(o, "absente") == (size_t)-1, "index d'une cle absente -> (size_t)-1");
    }
    freeValue(v);

    v = LIRE("\xEF\xBB\xBF{\"a\": \"caf\xC3\xA9 \xF0\x9F\x98\x80\"}");
    CHECK(v.type == JSON_OBJECT, "BOM + UTF-8 valide accepte");
    freeValue(v);
}

static void test_fichiers_invalides(void){
    printf("fichiers invalides (les messages \"Error :\" sont normaux)\n");
    JsonValue v;
    v = initCson("tests/ce_fichier_n_existe_pas.json"); CHECK(estErreur(v, "cannot open file"), "fichier absent");
    v = LIRE("{\"a\": 1,}");                   CHECK(estErreur(v, "invalid JSON"), "virgule en trop");
    v = LIRE("[1, 2");                         CHECK(estErreur(v, "invalid JSON"), "tableau non ferme");
    v = LIRE("{\"a\": \"x}");                  CHECK(estErreur(v, "invalid JSON"), "string non fermee");
    v = LIRE("[01]");                          CHECK(estErreur(v, "invalid JSON"), "zero en tete");
    v = LIRE("{\"a\": \"x\xFFy\"}");           CHECK(estErreur(v, "invalid JSON"), "UTF-8 invalide");
    v = LIRE("{\"a\": \"\xED\xA0\x80\"}");     CHECK(estErreur(v, "invalid JSON"), "UTF-8 : surrogate interdit");
    v = LIRE("[1,2]\0[3");                     CHECK(estErreur(v, "NUL byte in file"), "octet NUL");
    v = LIRE("{\"a\": 1, \"a\": 2}");          CHECK(estErreur(v, "duplicate key"), "cle en double");
    v = LIRE("{\"x\": [{\"k\": 1, \"k\": 2}]}"); CHECK(estErreur(v, "duplicate key"), "cle en double imbriquee : vraie cause gardee");
    v = LIRE("[1.0000000000000000000000000000000000000000000000000000000000000001]");
    CHECK(estErreur(v, "invalid JSON"), "nombre de plus de 63 caracteres");
}

static void test_nombres(void){
    printf("nombres\n");
    JsonValue v = LIRE("[1234567890123456789, -9223372036854775808, 9223372036854775808, 0.30000000000000004, 3.14159, -0.5]");
    CHECK(v.type == JSON_ARRAY && v.value.array->nbOfElement == 6, "tableau de 6 nombres");
    if(v.type == JSON_ARRAY && v.value.array->nbOfElement == 6){
        JsonValue* t = v.value.array->listeOfValue;
        CHECK(t[0].type == JSON_NUMBER && t[0].value.integer == 1234567890123456789LL, "entier de 19 chiffres exact");
        CHECK(t[1].type == JSON_NUMBER && t[1].value.integer == INT64_MIN, "plus petit long long");
        CHECK(t[2].type == JSON_EXPONENTIAL && strcmp(t[2].value.string, "9223372036854775808") == 0, "trop grand : garde en texte, sans perte");
        CHECK(t[3].type == JSON_EXPONENTIAL, "plus de 15 chiffres significatifs : garde en texte");
        CHECK(t[4].type == JSON_DECIMAL && t[4].value.decimal == 3.14159, "decimal exact");
        CHECK(t[5].type == JSON_DECIMAL && t[5].value.decimal == -0.5, "decimal negatif");
    }
    freeValue(v);
}

static void test_aller_retour(void){
    printf("ecriture puis relecture\n");
    JsonValue v = LIRE("{\"s\": \"x\\\\y \\\" \\n\", \"n\": [1, -2, 3.25, 1e-7, 6.02e+23, 0.0000001], \"o\": {}, \"t\": [true, false, null]}");
    CHECK(v.type == JSON_OBJECT, "lecture");
    char* premier = ecrire(FICHIER_SORTIE, v);
    freeValue(v);
    JsonValue r = initCson(FICHIER_SORTIE);
    CHECK(r.type == JSON_OBJECT, "le fichier ecrit se relit");
    char* second = ecrire(FICHIER_SORTIE2, r);
    freeValue(r);
    CHECK(premier && second && strcmp(premier, second) == 0, "reecrire donne exactement le meme fichier");
    free(premier);
    free(second);
}

static void test_creation(void){
    printf("creation\n");
    JsonValue v;
    CHECK(newJsonBool(&v, 2) == 1 && v.type == JSON_ERROR, "booleen 2 refuse");
    freeValue(v);
    CHECK(newJsonBool(&v, 1) == 0 && v.type == JSON_BOOL && v.value.integer == 1, "booleen 1");
    double zero = 0.0;
    CHECK(newJsonDecimal(&v, 0.0 / zero) == 1 && v.type == JSON_ERROR, "NaN refuse");
    CHECK(newJsonDecimal(&v, 1.0 / zero) == 1 && v.type == JSON_ERROR, "infini refuse");
    CHECK(newJsonExponencial(&v, "pas un nombre") == 1 && v.type == JSON_ERROR, "faux nombre refuse");
    CHECK(newJsonExponencial(&v, "12abc") == 1, "nombre suivi de texte refuse");
    CHECK(newJsonExponencial(&v, "1e10") == 0 && v.type == JSON_EXPONENTIAL, "exposant accepte");
    freeValue(v);
    CHECK(newJsonString(&v, "texte") == 0 && v.type == JSON_STRING, "string");
    freeValue(v);
    CHECK(newJsonBool(NULL, 1) == 1, "dest NULL refuse");

    JsonValue a = initJsonValue(JSON_ARRAY);   // pointeur NULL : doit s'écrire null, pas planter
    char* texte = ecrire(FICHIER_SORTIE, a);
    CHECK(texte && strcmp(texte, "null") == 0, "tableau jamais alloue -> null");
    free(texte);
}

static void test_modification(void){
    printf("modification\n");
    JsonValue o, n, s;
    newJsonObject(&o);
    newJsonNumber(&n, 1);
    newJsonString(&s, "deux");
    CHECK(addToObject(o.value.object, "a", n, 0) == 0, "ajout de a");
    CHECK(addToObject(o.value.object, "b", s, 1) == 0, "ajout de b");
    CHECK(addToObject(o.value.object, "a", s, 2) == 1, "ajout d'une cle deja presente refuse");
    CHECK(addToObject(o.value.object, "c", s, 9) == 1, "index trop grand refuse");
    CHECK(modifyObject(o.value.object, (JsonPair){"b", n}, 0) == 1, "modify vers une cle presente ailleurs refuse");
    CHECK(modifyObject(o.value.object, (JsonPair){"a", s}, 0) == 0, "modify en gardant la meme cle");
    JsonValue* a = getValueFromObject(o.value.object, "a");
    CHECK(a && a->type == JSON_STRING && strcmp(a->value.string, "deux") == 0, "nouvelle valeur de a");
    CHECK(removeToObject(o.value.object, 0) == 0 && o.value.object->nbOfElement == 1, "suppression");
    CHECK(removeToObject(o.value.object, 5) == 1, "suppression hors limites refusee");

    JsonValue t;
    newJsonArray(&t);
    for(int i = 0; i < 100; i++){
        addToArray(t.value.array, n, t.value.array->nbOfElement);
    }
    CHECK(t.value.array->nbOfElement == 100, "100 ajouts dans un tableau");
    CHECK(modifyArray(t.value.array, s, 50) == 0 && t.value.array->listeOfValue[50].type == JSON_STRING, "modifyArray");
    CHECK(removeToArray(t.value.array, 0) == 0 && t.value.array->nbOfElement == 99, "removeToArray");

    freeValue(t);
    freeValue(o);
    freeValue(n);
    freeValue(s);
}

static void test_copie(void){
    printf("copie\n");
    JsonValue v = LIRE("{\"liste\": [1, \"x\", {\"y\": null}]}");
    JsonValue c;
    CHECK(cpyValue(&c, &v) == 0 && c.type == JSON_OBJECT, "copie complete");
    if(c.type == JSON_OBJECT){
        removeToObject(c.value.object, 0);
        CHECK(v.value.object->nbOfElement == 1, "modifier la copie ne touche pas l'original");
    }
    CHECK(cpyValue(&v, &v) == 1 && v.type == JSON_OBJECT && v.value.object->nbOfElement == 1, "copie sur soi-meme refusee, valeur intacte");
    freeValue(c);
    freeValue(v);
}

static void test_securite(void){
    printf("pointeurs NULL et erreurs d'ecriture\n");
    JsonValue v;
    newJsonNull(&v);
    CHECK(getValueFromObject(NULL, "a") == NULL, "getValueFromObject(NULL)");
    CHECK(getIndexFromObject(NULL, "a") == (size_t)-1, "getIndexFromObject(NULL)");
    CHECK(fprintfValue(NULL, v) == 1, "fprintfValue(NULL)");
    FILE* plein = fopen("/dev/full", "w");   // n'existe que sous Linux
    if(plein != NULL){
        CHECK(fprintfValue(plein, v) == 1, "disque plein detecte");
        fclose(plein);
    }
}

static void test_locale(void){
    printf("locale francaise\n");
    if(setlocale(LC_ALL, "fr_FR.UTF-8") == NULL){
        printf("  (locale fr_FR.UTF-8 absente : test ignore)\n");
        return;
    }
    JsonValue v = LIRE("[1.5, 3.14159]");
    CHECK(v.type == JSON_ARRAY && v.value.array->listeOfValue[0].value.decimal == 1.5, "1.5 bien lu en locale francaise");
    char* texte = ecrire(FICHIER_SORTIE, v);
    CHECK(texte && strcmp(texte, "[1.5,3.14159]") == 0, "ecrit avec un '.' en locale francaise");
    free(texte);
    freeValue(v);
    setlocale(LC_ALL, "C");
}

int main(void){
    test_lecture();
    test_fichiers_invalides();
    test_nombres();
    test_aller_retour();
    test_creation();
    test_modification();
    test_copie();
    test_securite();
    test_locale();

    remove(FICHIER_ENTREE);
    remove(FICHIER_SORTIE);
    remove(FICHIER_SORTIE2);

    printf("\n%d tests reussis, %d echoues\n", g_ok, g_ko);
    return g_ko == 0 ? 0 : 1;
}
