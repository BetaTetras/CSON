// Exemple d'utilisation de CSON : lire test.json, lire des valeurs, en ajouter une,
// afficher le résultat et l'enregistrer dans sortie.json.
// Compilation : make exemple   puis   ./exemple

#include <stdio.h>
#include "cson.h"

int main(void){
    // 1. Lire le fichier
    JsonValue racine = initCson("test.json");
    if(racine.type == JSON_ERROR){
        printf("Lecture impossible : %s\n", racine.value.string);
        return 1;
    }
    if(racine.type != JSON_OBJECT){
        printf("test.json devrait contenir un objet\n");
        freeValue(racine);
        return 1;
    }
    JsonObject* obj = racine.value.object;

    // 2. Lire des valeurs : toujours vérifier le type avant de lire le champ de value
    JsonValue* texte = getValueFromObject(obj, "test_1_string");
    if(texte != NULL && texte->type == JSON_STRING){
        printf("test_1_string = %s\n", texte->value.string);
    }
    JsonValue* entier = getValueFromObject(obj, "test_2_int");
    if(entier != NULL && entier->type == JSON_NUMBER){
        printf("test_2_int    = %lld\n", entier->value.integer);
    }
    JsonValue* personne = getValueFromObject(obj, "test_5_object");
    if(personne != NULL && personne->type == JSON_OBJECT){
        JsonValue* age = getValueFromObject(personne->value.object, "Age");
        if(age != NULL && age->type == JSON_NUMBER){
            printf("Age           = %lld\n", age->value.integer);
        }
    }

    // 3. Ajouter une valeur à la fin de l'objet.
    //    addToObject COPIE la valeur : on libère donc la nôtre juste après.
    JsonValue ajout;
    if(newJsonString(&ajout, "ajout de l'exemple") == 0){
        if(addToObject(obj, "test_7_ajout", ajout, obj->nbOfElement) != 0){
            printf("Ajout impossible\n");
        }
        freeValue(ajout);
    }

    // 4. Afficher, puis enregistrer
    printfValue(racine);
    printf("\n");

    FILE* sortie = fopen("sortie.json", "w");
    if(sortie == NULL || fprintfValue(sortie, racine) != 0){
        printf("Enregistrement impossible\n");
    }else{
        printf("Enregistré dans sortie.json\n");
    }
    if(sortie != NULL){
        fclose(sortie);
    }

    // 5. Tout libérer
    freeValue(racine);
    return 0;
}
