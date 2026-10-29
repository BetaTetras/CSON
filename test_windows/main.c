// Test de CSON sous Windows (marche aussi sous Linux).
// Verifie les fonctions principales et affiche OK / ECHEC pour chaque test.
// Lancer le programme depuis ce dossier : il lit test.json et ecrit (puis supprime) sortie_test.json.
// Les messages "Error : ..." sur la console sont normaux : certains tests provoquent des erreurs volontairement.

#define _CRT_SECURE_NO_WARNINGS   // Visual Studio : ne pas avertir pour fopen / sprintf
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#include "cson.h"

static int g_ok = 0;
static int g_ko = 0;

static void verifier(int condition, const char* nom){
    if(condition){
        g_ok++;
        printf("  OK     %s\n", nom);
    }else{
        g_ko++;
        printf("  ECHEC  %s\n", nom);
    }
}

// Lit tout un fichier (en binaire) dans un bloc alloue, termine par '\0'
static char* lireFichier(const char* chemin){
    FILE* f = fopen(chemin, "rb");
    if(f == NULL){
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long taille = ftell(f);
    rewind(f);
    char* texte = (char*)malloc((size_t)taille + 1);
    if(texte != NULL){
        size_t lu = fread(texte, 1, (size_t)taille, f);
        texte[lu] = '\0';
    }
    fclose(f);
    return texte;
}

static int estNombre(JsonValue* v, long long attendu){
    return v != NULL && v->type == JSON_NUMBER && v->value.integer == attendu;
}

// Decode une string stockee et la compare au texte attendu
static int decodeEn(JsonValue* v, const char* attendu){
    char* clair = NULL;
    int ok = v != NULL && v->type == JSON_STRING && unescapeString(&clair, v->value.string) == 0 && strcmp(clair, attendu) == 0;
    if(clair != NULL){
        stringToZero(clair, strlen(clair));
        free(clair);
    }
    return ok;
}

int main(void){
    printf("=== CSON : tests\n");

    ////////////////////////////////////////////////////////////////////////
    printf("\n[1] Lecture de test.json (initCsonFromFile)\n");
    JsonValue racine = initCsonFromFile("test.json");
    verifier(racine.type == JSON_OBJECT, "test.json lu");
    if(racine.type != JSON_OBJECT){
        printf("Arret : test.json introuvable ou invalide (lancer le programme depuis le dossier test_windows)\n");
        return 1;
    }
    JsonObject* obj = racine.value.object;
    JsonValue* v;

    v = getValueFromObject(obj, "nom");
    verifier(v != NULL && v->type == JSON_STRING && strcmp(v->value.string, "Gregoire") == 0, "string simple");
    verifier(estNombre(getValueFromObject(obj, "age"), 25), "entier 25");
    verifier(estNombre(getValueFromObject(obj, "grand"), 9223372036854775807LL), "plus grand long long");
    verifier(estNombre(getValueFromObject(obj, "petit"), -9223372036854775807LL - 1), "plus petit long long");
    v = getValueFromObject(obj, "taille");
    verifier(v != NULL && v->type == JSON_DECIMAL && v->value.decimal == 1.78, "decimal 1.78");
    v = getValueFromObject(obj, "distance");
    verifier(v != NULL && v->type == JSON_EXPONENTIAL && strcmp(v->value.string, "1.5e8") == 0, "1.5e8 garde en texte");
    v = getValueFromObject(obj, "actif");
    verifier(v != NULL && v->type == JSON_BOOL && v->value.integer == 1, "true");
    v = getValueFromObject(obj, "fax");
    verifier(v != NULL && v->type == JSON_NULL, "null");
    v = getValueFromObject(obj, "notes");
    verifier(v != NULL && v->type == JSON_ARRAY && v->value.array->nbOfElement == 6, "tableau de 6 valeurs");
    v = getValueFromObject(obj, "adresse");
    verifier(v != NULL && v->type == JSON_OBJECT && estNombre(getValueFromObject(v->value.object, "code"), 75000), "objet imbrique");
    verifier(getValueFromObject(obj, "absente") == NULL, "cle absente -> NULL");

    ////////////////////////////////////////////////////////////////////////
    printf("\n[2] Unicode et echappements (unescapeString)\n");
    const char* cafe = "caf\xC3\xA9 \xE2\x82\xAC \xF0\x9F\x98\x80";   // "cafe EUR emoji" en UTF-8
    verifier(decodeEn(getValueFromObject(obj, "mdp"), cafe), "\\u00e9 \\u20ac \\ud83d\\ude00 decodes en UTF-8");
    verifier(decodeEn(getValueFromObject(obj, "mdpDirect"), cafe), "UTF-8 ecrit directement dans le fichier");
    verifier(decodeEn(getValueFromObject(obj, "texte"), "guillemet \" et retour\n\xC3\xA0 la ligne"), "\\\" et \\n decodes");

    ////////////////////////////////////////////////////////////////////////
    printf("\n[3] Ecriture (fprintfValue / sprintfValue)\n");
    FILE* f = fopen("sortie_test.json", "wb");   // "wb" : pas de \r ajoute par Windows
    if(f != NULL){
        setvbuf(f, NULL, _IONBF, 0);
    }
    verifier(f != NULL && fprintfValue(f, racine) == 0, "fprintfValue dans sortie_test.json");
    if(f != NULL){
        fclose(f);
    }
    char* texteFichier = lireFichier("sortie_test.json");
    char* texte = NULL;
    verifier(sprintfValue(&texte, racine) == 0, "sprintfValue");
    verifier(texte != NULL && texteFichier != NULL && strcmp(texte, texteFichier) == 0, "sprintfValue donne le meme texte que fprintfValue");
    verifier(texte != NULL && strstr(texte, "9223372036854775807") != NULL && strstr(texte, "-9223372036854775808") != NULL, "grands entiers bien ecrits (%lld)");
    verifier(texte != NULL && strstr(texte, "\"dixieme\": 0.1,") != NULL, "0.1 ecrit 0.1");
    JsonValue relu = initCsonFromFile("sortie_test.json");
    char* texte2 = NULL;
    verifier(relu.type == JSON_OBJECT && sprintfValue(&texte2, relu) == 0 && strcmp(texte, texte2) == 0, "fichier ecrit puis relu : identique");
    char* deja = "x";
    verifier(sprintfValue(&deja, racine) == 1, "sprintfValue refuse un *dest deja rempli");
    if(texte != NULL){ stringToZero(texte, strlen(texte)); free(texte); }
    if(texte2 != NULL){ stringToZero(texte2, strlen(texte2)); free(texte2); }
    free(texteFichier);
    freeValue(relu);
    remove("sortie_test.json");

    ////////////////////////////////////////////////////////////////////////
    printf("\n[4] Lecture depuis la memoire (initCson) et erreurs\n");
    char enMemoire[] = "\xEF\xBB\xBF{ \"site\" : \"banque\", \"pin\" : 1234 }";   // commence par un BOM
    JsonValue m = initCson(enMemoire);
    verifier(m.type == JSON_OBJECT && estNombre(getValueFromObject(m.value.object, "pin"), 1234), "texte en memoire, avec BOM");
    freeValue(m);
    char faux[] = "{\"a\": 1,}";
    verifier(initCson(faux).type == JSON_ERROR, "JSON invalide refuse");
    char doublon[] = "{\"a\": {\"b\": 1, \"b\": 2}}";
    verifier(initCson(doublon).type == JSON_ERROR, "cle en double refusee (objet imbrique)");
    verifier(initCsonFromFile("fichier_qui_n_existe_pas.json").type == JSON_ERROR, "fichier absent refuse");
    verifier(initCson(NULL).type == JSON_ERROR, "texte NULL refuse");

    ////////////////////////////////////////////////////////////////////////
    printf("\n[5] Modification d'un objet\n");
    JsonValue o;
    JsonValue un;
    JsonValue s;
    newJsonObject(&o);
    newJsonNumber(&un, 1);
    newJsonString(&s, "p\"ss\\w0rd");
    verifier(addToObject(o.value.object, "site", un, 0) == 0, "ajout de site");
    verifier(addToObject(o.value.object, "mdp", s, 1) == 0, "ajout de mdp");
    verifier(addToObject(o.value.object, "site", un, 0) == 1, "ajout d'une cle deja presente refuse");
    JsonPair paire;
    paire.key = "login";
    paire.value = s;
    verifier(modifyObject(o.value.object, paire, 0) == 0, "site remplace par login");
    verifier(getValueFromObject(o.value.object, "site") == NULL && getValueFromObject(o.value.object, "login") != NULL, "recherche apres modification");
    verifier(removeToObject(o.value.object, 1) == 0 && getValueFromObject(o.value.object, "mdp") == NULL, "suppression de mdp");
    verifier(decodeEn(getValueFromObject(o.value.object, "login"), "p\"ss\\w0rd"), "echappement automatique puis decodage");
    JsonValue copie;
    verifier(cpyValue(&copie, &o) == 0 && getValueFromObject(copie.value.object, "login") != NULL, "copie complete, recherche dans la copie");
    freeValue(copie);
    freeValue(o);
    freeValue(s);

    ////////////////////////////////////////////////////////////////////////
    printf("\n[6] Locale francaise (separateur decimal)\n");
    const char* noms[] = {"French_France.1252", "fr-FR", "fr_FR.UTF-8", "fr_FR.utf8", "French"};
    const char* trouvee = NULL;
    for(int i = 0; i < 5 && trouvee == NULL; i++){
        if(setlocale(LC_NUMERIC, noms[i]) != NULL){
            trouvee = noms[i];
        }
    }
    if(trouvee != NULL){
        printf("  (locale %s)\n", trouvee);
        JsonValue dec;
        newJsonDecimal(&dec, 1.5);
        char* t = NULL;
        verifier(sprintfValue(&t, dec) == 0 && strcmp(t, "1.5") == 0, "1.5 ecrit avec un point, meme en francais");
        free(t);
        char lu[] = "[2.25]";
        JsonValue l = initCson(lu);
        verifier(l.type == JSON_ARRAY && l.value.array->listeOfValue[0].type == JSON_DECIMAL && l.value.array->listeOfValue[0].value.decimal == 2.25, "2.25 relu, meme en francais");
        freeValue(l);
        setlocale(LC_NUMERIC, "C");
    }else{
        printf("  (aucune locale francaise trouvee : test ignore)\n");
    }

    ////////////////////////////////////////////////////////////////////////
    printf("\n[7] Vitesse (40 000 cles dans un meme objet)\n");
    JsonValue gros;
    newJsonObject(&gros);
    char cle[32];
    clock_t t0 = clock();
    for(int i = 0; i < 40000; i++){
        sprintf(cle, "site%d", i);
        addToObject(gros.value.object, cle, un, gros.value.object->nbOfElement);
    }
    double tAjout = (double)(clock() - t0) / CLOCKS_PER_SEC;
    verifier(gros.value.object->nbOfElement == 40000, "40 000 addToObject");
    char* grosTexte = NULL;
    sprintfValue(&grosTexte, gros);
    t0 = clock();
    JsonValue grosRelu = initCson(grosTexte);
    double tLecture = (double)(clock() - t0) / CLOCKS_PER_SEC;
    verifier(grosRelu.type == JSON_OBJECT && grosRelu.value.object->nbOfElement == 40000, "objet de 40 000 cles relu");
    printf("         40 000 addToObject     : %.3f s\n", tAjout);
    printf("         lecture des 40 000 cles : %.3f s\n", tLecture);
    free(grosTexte);
    freeValue(grosRelu);
    freeValue(gros);

    freeValue(racine);
    printf("\n=== %d tests reussis, %d echoues\n", g_ok, g_ko);
    return g_ko != 0;
}
