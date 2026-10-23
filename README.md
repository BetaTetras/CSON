# 📦 | CSON
![Badge Langage](https://img.shields.io/badge/Langage%20:-C-blue?style=plastic)
![Badge Licence](https://img.shields.io/badge/Licence%20:-MIT-darkblue?style=plastic)
![Badge Version](https://img.shields.io/badge/Version%20:-v1.1-lime?style=plastic)
![Badge OS](https://img.shields.io/badge/OS%20:-LINUX-blue?style=plastic)
![Badge État](https://img.shields.io/badge/État%20%3A-Stable-green?style=plastic)

**CSON** est une bibliothèque C pour lire, modifier et écrire du JSON

## 📋 | Fonctionnalités de CSON

| **Fonctionnalités**                                                              | **Version** |
|----------------------------------------------------------------------------------|:-----------:|
| [📖] *Lecture*                                                                   |             |
| **Lecture** d'un fichier JSON complet avec `initCson()`                          | `v1`        |
| **Validation stricte** du JSON (RFC 8259) avant la lecture                       | `v1`        |
| **Message d'erreur** avec la ligne et la colonne du problème                     | `v1`        |
| **Vérification de l'UTF-8** dans les strings et les clés                         | `v1`        |
| **Gestion du BOM** UTF-8 et refus des octets NUL                                 | `v1`        |
| **Refus des clés en double**, même dans les objets imbriqués                     | `v1`        |
| [✏️] *Création & modification*                                                   |             |
| **Création** de valeurs avec `newJsonObject()`, `newJsonString()`...             | `v1`        |
| **Ajout / suppression / modification** avec `addTo...()`, `removeTo...()`, `modify...()` | `v1` |
| **Recherche** d'une clé avec `getValueFromObject()` / `getIndexFromObject()`     | `v1`        |
| **Copie complète** d'une valeur avec `cpyValue()`                                | `v1`        |
| **Échappement automatique** des strings et des clés (`"` `\` `\n`...), décodage avec `unescapeString()` | `v1.1` |
| **Effacement** des strings, des clés et du texte du fichier avant libération (`stringToZero()`) | `v1.1` |
| [💾] *Écriture*                                                                  |             |
| **Affichage** indenté avec `printfValue()`, **écriture** avec `fprintfValue()`   | `v1`        |
| **Nombres sans perte** : un décimal écrit puis relu redonne exactement la même valeur | `v1`   |
| **Indépendant de la locale** : toujours un `.` décimal, même en français         | `v1`        |
| **Détection** des erreurs d'écriture (disque plein...)                           | `v1`        |
| [🔩] *Technique*                                                                 |             |
| **Aucune dépendance** : seulement `stdio.h`, `stdlib.h` et `stdint.h`            | `v1`        |
| **Gestion mémoire** complète, testée sous *valgrind* (aucune fuite)              | `v1`        |
| **Messages d'erreur** explicites dans les `JSON_ERROR`                           | `v1`        |

## ⚙️ | Fonctionnement et déploiement
### Requis :
- *Toute version* -  N'importe quel PC sous Linux (*gcc* et *make* conseillés)

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
    JsonValue racine = initCson("config.json");
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

Les fonctions qui renvoient un `int` renvoient **0 si tout va bien** et **1 en cas d'erreur**.

## 🧠 | Gestion de la mémoire
- Une valeur renvoyée par `initCson()` ou par un `newJson...()` doit être libérée avec `freeValue()`.
- `addTo...()` et `modify...()` **copient** la valeur qu'on leur donne : il faut donc libérer la sienne après.
  ```c
  JsonValue v;
  newJsonString(&v, "texte");
  addToObject(obj, "cle", v, obj->nbOfElement);
  freeValue(v);   // l'objet a sa propre copie
  ```
- Le pointeur renvoyé par `getValueFromObject()` pointe **à l'intérieur** de l'objet : il n'est plus valable après un `addToObject()` ou un `removeToObject()`, et il ne faut jamais le libérer.
- Le message d'un `JSON_ERROR` est un texte constant : il ne faut **jamais** le libérer.

## ⚠️ | À savoir
- **Les strings sont gardées au format JSON**, échappements compris (`"a\nb"` reste `a`, `\`, `n`, `b`), et réécrites à l'identique. `newJsonString()`, `addToObject()` et `modifyObject()` échappent automatiquement le texte donné (`p"ss` est stocké `p\"ss`). Pour récupérer le vrai texte (un mot de passe par exemple), utiliser `unescapeString()`, puis l'effacer avec `stringToZero()` avant de le libérer. `fprintfValue()` écrit sans le tampon de `stdio` (aucune copie du texte gardée en mémoire) : l'appeler juste après le `fopen()`, avant toute autre opération sur ce fichier. `getValueFromObject()` et `getIndexFromObject()` prennent aussi le texte normal de la clé.
- **`\uXXXX`** : `unescapeString()` ne décode pour l'instant que les caractères de `\u0001` à `\u007F`. Un `é` écrit `\u00e9` (par Python par exemple) est refusé.
- **Nombres** : 63 caractères au maximum. Un entier trop grand pour un `long long`, ou un nombre de plus de 15 chiffres significatifs, est gardé **en texte** (`JSON_EXPONENTIAL`), sans aucune perte. `-0` est lu comme `0`, et `NaN` / l'infini sont refusés (ils n'existent pas en JSON).
- **Erreurs** : en plus du message dans le `JSON_ERROR`, `initCson()` écrit sur `stderr` la ligne et la colonne du problème (`Error : invalid JSON at line 3, column 11`).
- **Imbrication** : 512 niveaux de `{` et `[` au maximum.
- **Gros objets** *(corrigé en v2)* : un objet de 40 000 clés met environ 20 secondes à être lu (mesuré sur un Raspberry Pi). À éviter avec des fichiers qui ne viennent pas de vous.

## 💡 | Remarques
**CSON** est une bibliothèque créée pour apprendre et m'améliorer en C (pointeurs, mémoire, parsing) et dans l'algorithmie en général, dans le cadre de mes études en informatique... Donc c'est pas du sérieux les gars.

## 📜 | Licence

Ce projet est sous licence **MIT** – Vous pouvez faire un peu ce que vous voulez tant que je suis crédité.
