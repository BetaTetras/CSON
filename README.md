# 📦 | CSON
![Badge Langage](https://img.shields.io/badge/Langage%20:-C-blue?style=plastic)
![Badge Licence](https://img.shields.io/badge/Licence%20:-MIT-darkblue?style=plastic)
![Badge Version](https://img.shields.io/badge/Version%20:-v2.0-lime?style=plastic)
![Badge OS](https://img.shields.io/badge/OS%20:-LINUX%20%7C%20WINDOWS-blue?style=plastic)
![Badge État](https://img.shields.io/badge/État%20%3A-Stable-green?style=plastic)

**CSON** est une bibliothèque C pour lire, modifier et écrire du JSON

## 📋 | Fonctionnalités de CSON

| **Fonctionnalités**                                                              | **Version** |
|----------------------------------------------------------------------------------|:-----------:|
| [📖] *Lecture*                                                                   |             |
| **Lecture** d'un fichier JSON complet avec `initCsonFromFile()`                  | `v1`        |
| **Lecture** d'un texte JSON déjà en mémoire avec `initCson()` (un coffre déchiffré par exemple) | `v2` |
| **Validation stricte** du JSON (RFC 8259) avant la lecture                       | `v1`        |
| **Message d'erreur** avec la ligne et la colonne du problème                     | `v1`        |
| **Vérification de l'UTF-8** dans les strings et les clés                         | `v1`        |
| **Gestion du BOM** UTF-8 et refus des octets NUL                                 | `v1`        |
| **Refus des clés en double**, même dans les objets imbriqués                     | `v1`        |
| **Lecture rapide des gros objets** : doublons cherchés par tri (40 000 clés en 0,06 s au lieu de 25 s) | `v2` |
| [✏️] *Création & modification*                                                   |             |
| **Création** de valeurs avec `newJsonObject()`, `newJsonString()`...             | `v1`        |
| **Ajout / suppression / modification** avec `addTo...()`, `removeTo...()`, `modify...()` | `v1` |
| **Ajout et recherche rapides** : clés rangées dans une liste triée, recherche par dichotomie (40 000 `addToObject()` en 0,24 s au lieu de 22 s) | `v2` |
| **Recherche** d'une clé avec `getValueFromObject()` / `getIndexFromObject()`     | `v1`        |
| **Copie complète** d'une valeur avec `cpyValue()`                                | `v1`        |
| **Échappement automatique** des strings et des clés (`"` `\` `\n`...), décodage avec `unescapeString()` | `v1.1` |
| **Décodage complet des `\uXXXX`** par `unescapeString()` : accents, `€`, emojis (`\ud83d\ude00`) | `v2` |
| **Effacement** des strings, des clés et du texte du fichier avant libération (`stringToZero()`) | `v1.1` |
| **Effacement des nombres** lus (un code PIN écrit en nombre ne reste plus en mémoire) | `v2`  |
| [💾] *Écriture*                                                                  |             |
| **Affichage** indenté avec `printfValue()`, **écriture** dans un fichier avec `fprintfValue()` | `v1` |
| **Écriture dans un texte en mémoire** avec `sprintfValue()` : même texte que `fprintfValue()`, en un seul bloc, sans copie laissée en mémoire | `v2` |
| **Nombres sans perte** : un décimal écrit puis relu redonne exactement la même valeur | `v1`   |
| **Indépendant de la locale** : toujours un `.` décimal, même en français         | `v1`        |
| **Détection** des erreurs d'écriture (disque plein...)                           | `v1`        |
| [🔩] *Technique*                                                                 |             |
| **Aucune dépendance** : seulement `stdio.h`, `stdlib.h` et `stdint.h`            | `v1`        |
| **Linux et Windows** : uniquement du C11 standard, testé sur les deux (locale française comprise) | `v2` |
| **Gestion mémoire** complète, testée sous *valgrind* (aucune fuite)              | `v1`        |
| **Messages d'erreur** explicites dans les `JSON_ERROR`                           | `v1`        |

## ⚙️ | Fonctionnement et déploiement
### Requis :
- *Toute version* -  N'importe quel PC sous Linux (*gcc* et *make* conseillés)
- *v2* - Windows aussi, avec un compilateur récent (MinGW-w64 ou Visual Studio 2015 et plus) : voir [`test_windows/`](test_windows/) pour compiler et tester

### Compilation :
```sh
make            # construit la bibliothèque libcson.a
make clean      # supprime tout ce qui a été compilé
```

### Utilisation dans un projet :
Il suffit de copier `cson.c` et `cson.h` dans le projet, ou de lier `libcson.a` :
```sh
gcc mon_programme.c -I chemin/vers/CSON -L chemin/vers/CSON -lcson -o mon_programme
```

## 🧩 | Utilisation
```c
#include <stdio.h>
#include "cson.h"

int main(void){
    JsonValue racine = initCsonFromFile("config.json");
    if(racine.type == JSON_ERROR){
        printf("Erreur : %s\n", racine.value.string);
        return 1;
    }

    if(racine.type == JSON_OBJECT){
        JsonValue* nom = getValueFromObject(racine.value.object, "nom");
        if(nom != NULL && nom->type == JSON_STRING){
            printf("nom = %s\n", nom->value.string);
        }
    }

    printfValue(racine);
    freeValue(racine);
    return 0;
}
```
Le détail de chaque fonction se trouve dans [`cson.h`](cson.h).

### Lire un texte déjà en mémoire :
```c
char* texte = ...;                    // par exemple le coffre déchiffré, dans un bloc alloué
size_t taille = ...;                  // sa taille, à noter AVANT initCson (qui le raccourcit)

JsonValue racine = initCson(texte);   // attention : texte est modifié (espaces retirés)
// ... utilisation de racine ...
freeValue(racine);

stringToZero(texte, taille);          // texte reste à l'appelant : à effacer puis libérer soi-même
free(texte);
```

### Écrire dans un texte en mémoire :
```c
char* texte = NULL;                       // doit valoir NULL avant l'appel
if(sprintfValue(&texte, racine) == 0){    // texte : exactement ce que fprintfValue écrirait dans un fichier
    // ... chiffrer texte, l'envoyer... sans jamais l'écrire en clair sur le disque ...
    stringToZero(texte, strlen(texte));   // à effacer puis libérer soi-même
    free(texte);
}
```

### Les types :
Toute valeur est une `JsonValue` : un **type** et un **contenu**. Il faut toujours vérifier le type avant de lire le contenu.

| **Type**            | **JSON**                     | **Champ à lire**                                    |
|---------------------|------------------------------|-----------------------------------------------------|
| `JSON_NULL`         | `null`                       | aucun                                               |
| `JSON_BOOL`         | `true` / `false`             | `value.integer` (1 ou 0)                            |
| `JSON_NUMBER`       | entier                       | `value.integer` (`long long`)                       |
| `JSON_DECIMAL`      | nombre à virgule             | `value.decimal` (`double`)                          |
| `JSON_EXPONENTIAL`  | nombre gardé en texte        | `value.string`                                      |
| `JSON_STRING`       | `"texte"`                    | `value.string`                                      |
| `JSON_ARRAY`        | `[ ... ]`                    | `value.array` → `nbOfElement`, `listeOfValue[i]`    |
| `JSON_OBJECT`       | `{ ... }`                    | `value.object` → `nbOfElement`, `listeOfPair[i]`    |
| `JSON_ERROR`        | *(erreur)*                   | `value.string` : la cause de l'erreur               |

Un `JsonObject` contient aussi `listeOfSortedKey` : un champ **interne** (les clés rangées par ordre alphabétique, pour les retrouver vite), à ne jamais lire ni modifier soi-même.

Les fonctions qui renvoient un `int` renvoient **0 si tout va bien** et **1 en cas d'erreur**.

## 🧠 | Gestion de la mémoire
- Une valeur renvoyée par `initCsonFromFile()`, `initCson()` ou par un `newJson...()` doit être libérée avec `freeValue()`.
- Le texte donné à `initCson()` est **modifié** : il doit être modifiable (un tableau ou un bloc alloué, pas un texte écrit entre guillemets dans le code), et `initCson()` ne le libère pas.
- Le texte rendu par `sprintfValue()` ou `unescapeString()` est un **nouveau bloc** : `*dest` doit valoir `NULL` avant l'appel, et il faut ensuite l'effacer avec `stringToZero()` puis le libérer avec `free()`.
- `addTo...()` et `modify...()` **copient** la valeur qu'on leur donne : il faut donc libérer la sienne après.
  ```c
  JsonValue v;
  newJsonString(&v, "texte");
  addToObject(obj, "cle", v, obj->nbOfElement);
  freeValue(v);   // l'objet a sa propre copie
  ```
- Un objet doit être créé avec `newJsonObject()` (ou lu avec `initCson...()`), et ses clés ne doivent être changées qu'avec `addToObject()`, `removeToObject()` ou `modifyObject()` : sinon sa liste triée n'est plus à jour.
- Le pointeur renvoyé par `getValueFromObject()` pointe **à l'intérieur** de l'objet : il n'est plus valable après un `addToObject()` ou un `removeToObject()`, et il ne faut jamais le libérer.
- Le message d'un `JSON_ERROR` est un texte constant : il ne faut **jamais** le libérer.

## ⚠️ | À savoir
- **Les strings sont gardées au format JSON**, échappements compris (`"a\nb"` reste `a`, `\`, `n`, `b`), et réécrites à l'identique. `newJsonString()`, `addToObject()` et `modifyObject()` échappent automatiquement le texte donné (`p"ss` est stocké `p\"ss`). Pour récupérer le vrai texte (un mot de passe par exemple), utiliser `unescapeString()`, puis l'effacer avec `stringToZero()` avant de le libérer. Pour écrire un fichier sensible avec `fprintfValue()`, appeler `setvbuf(file, NULL, _IONBF, 0)` juste après le `fopen()` : sinon `stdio` garde une copie du texte dans un tampon que `fclose()` libère sans l'effacer (ou utiliser `sprintfValue()`, qui n'a pas ce problème). `getValueFromObject()` et `getIndexFromObject()` prennent aussi le texte normal de la clé.
- **`\uXXXX`** : `unescapeString()` décode tous les caractères Unicode en UTF-8 (`\u00e9` → `é`, `\u20ac` → `€`, `\ud83d\ude00` → 😀), comme les écrit Python par exemple. Seuls `\u0000` (il couperait la string en C) et une moitié de paire seule (`\ud83d` sans sa suite) sont refusés.
- **Nombres** : 63 caractères au maximum. Un entier trop grand pour un `long long`, ou un nombre de plus de 15 chiffres significatifs, est gardé **en texte** (`JSON_EXPONENTIAL`), sans aucune perte. `-0` est lu comme `0`, et `NaN` / l'infini sont refusés (ils n'existent pas en JSON).
- **Erreurs** : en plus du message dans le `JSON_ERROR`, `initCsonFromFile()` écrit sur `stderr` la ligne et la colonne du problème (`Error : invalid JSON at line 3, column 11`).
- **Imbrication** : 512 niveaux de `{` et `[` au maximum.

## ⏱️ | Performances (v1.1 → v2)
Mesuré sur un Raspberry Pi 4 (Linux, gcc 14 `-O2`) : meilleur temps sur 3 à 5 essais, un seul pour les mesures v1.1 de plus de 5 s.

### Lecture (`initCsonFromFile()`)
| **Fichier**                                          | **v1.1**   | **v2**     | **Gain**          |
|------------------------------------------------------|:----------:|:----------:|:-----------------:|
| Coffre réaliste : 1 000 entrées (110 Ko)             | 0,003 s    | 0,003 s    | identique         |
| Gros fichier : 20 000 entrées imbriquées (5,7 Mo)    | 0,173 s    | 0,181 s    | ≈ identique (+5 %) |
| Un objet de 5 000 clés                               | 0,382 s    | 0,005 s    | × 76              |
| Un objet de 20 000 clés                              | 5,4 s      | 0,038 s    | × 143             |
| Un objet de 40 000 clés                              | 24,6 s     | 0,058 s    | × 424             |
| Un objet de 200 000 clés                             | ≈ 10 min *(estimé)* | 0,24 s | —           |

### Construction d'un objet clé par clé (`addToObject()`)
| **Clés ajoutées**                                    | **v1.1**   | **v2**     | **Gain**          |
|------------------------------------------------------|:----------:|:----------:|:-----------------:|
| 5 000                                                | 0,343 s    | 0,008 s    | × 43              |
| 20 000                                               | 5,3 s      | 0,098 s    | × 54              |
| 40 000                                               | 22,1 s     | 0,242 s    | × 91              |
| 200 000                                              | ≈ 9 min *(estimé)* | 14,4 s | —            |

### Écriture (gros fichier de 5,7 Mo)
| **Fonction**                                         | **v1.1**   | **v2**     |
|------------------------------------------------------|:----------:|:----------:|
| `fprintfValue()` vers un fichier                     | 0,148 s    | 0,149 s    |
| `sprintfValue()` vers un texte en mémoire            | —          | 0,159 s    |

En v1.1, chaque nouvelle clé était comparée à toutes les précédentes : deux fois plus de clés, quatre fois plus de temps. En v2 :
- **à la lecture**, les clés d'un objet sont triées une seule fois (`bt_qsort`), puis seules les voisines sont comparées ;
- **pour `addToObject()`** et les recherches, chaque objet garde ses clés dans une liste triée : une clé est retrouvée par dichotomie (16 comparaisons environ pour 40 000 clés) et insérée directement à sa place.

Le gros fichier est lu un peu plus lentement (+5 %), parce que chacun de ses petits objets construit aussi sa liste triée. Au-delà de quelques dizaines de milliers de clés dans un même objet, le décalage de la liste à chaque insertion redevient coûteux (200 000 ajouts : 14 s).

## 💡 | Remarques
**CSON** est une bibliothèque créée pour apprendre et m'améliorer en C (pointeurs, mémoire, parsing) et dans l'algorithmie en général, dans le cadre de mes études en informatique... Donc c'est pas du sérieux les gars.

## 📜 | Licence

Ce projet est sous licence **MIT** – Vous pouvez faire un peu ce que vous voulez tant que je suis crédité.
