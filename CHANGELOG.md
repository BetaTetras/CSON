## 📦 CSON v1.1
Version mineure de corrections : échappement des strings et effacement de la mémoire.

## [🟠] | Changements à connaître (depuis la v1.0)
| **Changement**                                                                  |
|---------------------------------------------------------------------------------|
| `newJsonString()`, `addToObject()` et `modifyObject()` échappent maintenant le texte donné : ne plus écrire `\"` soi-même, sinon il est échappé deux fois |
| `getValueFromObject()` / `getIndexFromObject()` prennent la clé en texte normal (`"k\""` en C pour la clé `k"`) |
| Les strings restent stockées au format JSON (`p"ss` est stocké `p\"ss`) : utiliser `unescapeString()` pour récupérer le vrai texte |
| `fprintfValue()` doit être appelée juste après `fopen()`, avant toute autre opération sur le fichier |
| Le dépôt ne contient plus que la bibliothèque (`cson.c`, `cson.h`, `Makefile`) : l'exemple et les tests ont été retirés |

## [🟢] | Ajouts
| **Fonctionnalité**                                                              |
|---------------------------------------------------------------------------------|
| `unescapeString()` : décode une string stockée vers le texte réel (`\"`, `\\`, `\n`, `\u0041`...) |
| `stringToZero()` : met un texte à zéro avant `free()`, sans que le compilateur puisse supprimer l'effacement |
| Message sur `stderr` quand une fonction publique échoue (`newJson...()`, `addTo...()`, `removeTo...()`, `modify...()`) |

## [🔵] | Améliorations
| **Amélioration**                                                                |
|---------------------------------------------------------------------------------|
| Un `"` ou un `\` dans une string ou une clé ne casse plus le fichier écrit       |
| Caractères de contrôle écrits `\n`, `\t`, `\u0001`... : le fichier écrit est toujours un JSON valide |
| Clés en double détectées même avec des caractères spéciaux (`k"` ajoutée deux fois) |
| Strings, clés et texte du fichier effacés de la mémoire avant d'être libérés    |
| Lecture et écriture du fichier sans le tampon de `stdio` : aucune copie du texte laissée en mémoire |
| Échappement en une seule allocation : plus de copie partielle laissée par un `realloc` |
| Plus aucune clé ni valeur affichée dans les messages d'erreur                    |
| Plus de fuite mémoire si l'échappement échoue en cours de route                  |

## [🔴] | Limitations
| **Limitation**                                                                  |
|---------------------------------------------------------------------------------|
| Gros objets lents : 40 000 clés ≈ 20 s (recherche des doublons, corrigé en v2)  |
| Chaque tableau / objet lu réserve 10 places, même s'il en contient moins (v2)   |
| `unescapeString()` refuse les `\uXXXX` au-delà de `\u007F` (un `é` écrit `\u00e9` par Python par exemple) |
| Lecture et écriture uniquement depuis / vers un fichier (depuis la mémoire en v1.2) |
| Nombres limités à 63 caractères                                                 |
| Testé uniquement sous Linux                                                     |

---

## 📦 CSON v1.0
Première version — bibliothèque C pour lire, modifier et écrire du JSON.

## [🟢] | Ajouts
| **Fonctionnalité**                                                              |
|---------------------------------------------------------------------------------|
| Lecture d'un fichier JSON avec `initCson()`                                     |
| Validation stricte du JSON (RFC 8259) avec la ligne et la colonne de l'erreur   |
| Vérification de l'UTF-8, gestion du BOM et refus des octets NUL                 |
| Refus des clés en double, même dans les objets imbriqués                        |
| Création de valeurs avec `newJsonObject()`, `newJsonString()`...                |
| Ajout, suppression et modification avec `addTo...()`, `removeTo...()`, `modify...()` |
| Recherche d'une clé avec `getValueFromObject()` / `getIndexFromObject()`        |
| Copie complète d'une valeur avec `cpyValue()`                                   |
| Affichage indenté avec `printfValue()`, écriture avec `fprintfValue()`          |
| Messages d'erreur explicites dans les `JSON_ERROR`                              |

## [🔵] | Améliorations
| **Amélioration**                                                                |
|---------------------------------------------------------------------------------|
| Nombres sans perte : entiers 64 bits, décimaux relus à l'identique              |
| Nombres trop précis pour un `double` gardés en texte, sans perte                |
| Indépendant de la locale : toujours un `.` décimal, même en français            |
| Détection des erreurs d'écriture (disque plein...)                              |
| API publique dans `cson.h`, fonctions internes cachées (`static`)               |
| Compilation via `Makefile` (`libcson.a`, exemple et tests)                      |
| 64 tests automatiques (`make test`), aucune fuite sous valgrind (`make valgrind`) |
| Aucune dépendance : seulement `stdio.h`, `stdlib.h` et `stdint.h`               |

## [🔴] | Limitations
| **Limitation**                                                                  |
|---------------------------------------------------------------------------------|
| Gros objets lents : 40 000 clés ≈ 20 s (recherche des doublons, corrigé en v2)  |
| Chaque tableau / objet lu réserve 10 places, même s'il en contient moins (v2)   |
| Strings gardées telles quelles : les échappements (`\n`, `é`) ne sont pas décodés |
| Nombres limités à 63 caractères                                                 |
| Testé uniquement sous Linux                                                     |
