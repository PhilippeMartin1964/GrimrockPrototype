# UI-LOCALIZATION01.3 — UI String Table / CSV authoring contract

> **Statut : CLOSED — contrat de référence.** Commit : `788733b67ee3c48a8ab9d34cb087a55c13f12936`. Le contrat reste l'autorité d'authoring pour `UI.csv -> ST_UI`.


Date : **13 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**  
Ticket : **UI-LOCALIZATION01.3**  
État : **CONTRAT D'AUTHORING — aucune migration de texte joueur dans ce ticket**

## 1. Objet

Définir une autorité commune, versionnable et compatible avec le pipeline de localisation Unreal pour le vocabulaire UI/système identifié par `UI-LOCALIZATION01.2`.

Le contrat doit permettre :

- l'authoring des textes source hors des fichiers C++ ;
- des clés stables ;
- des diffs Git lisibles ;
- un import Editor idempotent vers une `UStringTable` ;
- la traduction via le pipeline de localisation Unreal ;
- la conservation des Data Assets existants comme autorités du contenu RPG authoré.

## 2. Règle de nommage protégée

Le token suivant est **interdit, sans tenir compte de la casse, dans tout nouvel identifiant de localisation** :

~~~text
Grimrock
~~~

Cette interdiction s'applique notamment aux nouveaux :

- IDs de String Table ;
- noms d'assets de String Table ;
- clés CSV ;
- noms de fichiers CSV d'authoring ;
- noms de Localization Targets ;
- namespaces explicitement définis ;
- noms de commandes/commandlets de localisation ;
- filtres de tests Automation de la campagne LOCALIZATION01.

Cette règle ne déclenche **aucun renommage global rétroactif** des modules, classes, chemins Content ou assets historiques du prototype. Un tel renommage serait un chantier indépendant.

En revanche, les nouveaux identifiants créés par LOCALIZATION01 doivent être neutres.

## 3. Décision d'architecture

Le modèle retenu est :

~~~text
Localization/Source/UI.csv
        │
        │ source d'authoring versionnée
        ▼
Editor importer / validator
        │
        ▼
/Game/Localization/StringTables/ST_UI
        │
        │ projection générée
        ▼
UStringTable
        │
        ▼
FText / UMG / C++
        │
        ▼
Unreal Localization Target: Game
        │
        ▼
cultures / .po / .locres
~~~

### 3.1 Autorité

~~~text
UI.csv = autorité d'authoring du vocabulaire UI générique
ST_UI   = projection générée consommée par Unreal
~~~

Le `.uasset` ne devient pas une seconde autorité.

Toute édition manuelle de `ST_UI` est considérée comme temporaire et sera écrasée au prochain import.

### 3.3 Consommation UMG directe — règle UI-TEXT-DIRECT01

Lorsqu'un `TextBlock`, un libellé de bouton ou un tooltip UMG est **strictement
statique** et possède déjà une clé stable dans `ST_UI`, le Designer doit
référencer directement cette entrée de String Table via la propriété `FText`
du widget :

~~~text
UI.csv
    -> ST_UI
    -> WBP Text property (Referenced Text / String Table / Key)
~~~

Le C++ ne doit pas servir d'intermédiaire uniquement pour recopier ce même
texte dans le widget.

La règle de décision devient :

~~~text
texte statique + clé ST_UI stable
    -> référence ST_UI directe dans le WBP

texte dynamique / conditionnel / formaté
    -> C++ / read-model
    -> FUILocalizationText pour le wording nécessaire

contenu métier authoré
    -> Data Asset du domaine
~~~

Un texte local propre à un écran et qui n'a volontairement aucune clé centrale
peut rester authoré dans le WBP. UI-TEXT-DIRECT01 n'impose donc pas « zéro
FText local dans les WBP ».

#### Autorité unique

Un même `TextBlock` ne doit pas cumuler une référence Designer vers `ST_UI`
et un `SetText()` C++ qui lui réinjecte la même valeur. En UE 5.5,
`UTextBlock::SetText()` remplace la valeur de la propriété Text et détruit
notamment son binding. Une surface statique directement liée à `ST_UI` ne
doit donc plus être réécrite au runtime.

Cette règle ne modifie pas l'autorité d'authoring :

~~~text
UI.csv = autorité éditoriale
ST_UI   = projection générée
WBP     = consommateur de la clé
~~~

Le drapeau / menu `Referenced Text` du Designer choisit une clé déjà
matérialisée ; il ne transforme pas `ST_UI.uasset` en fichier à éditer
manuellement.


### 3.2 Pourquoi une String Table Asset générée

Le CSV reste hors du contenu runtime :

~~~text
Localization/Source/UI.csv
~~~

Il n'est donc pas nécessaire de le charger directement avec `LOCTABLE_FROMFILE_GAME` ni de le packager comme fichier non-asset.

Le runtime consomme la `UStringTable` générée et cookée comme un asset Unreal normal.

Cela conserve :

- CSV lisible et diffable sous Git ;
- asset Unreal utilisable par UMG et les propriétés `FText` ;
- une seule autorité d'authoring.

## 4. Identifiants retenus

Tous les nouveaux identifiants sont neutres.

~~~text
CSV source repository path
    Localization/Source/UI.csv

Generated String Table asset
    /Game/Localization/StringTables/ST_UI

Asset name
    ST_UI

Logical table role
    UI

Localization Target
    Game

Native culture
    fr
~~~

Le futur importer devra résoudre le vrai `StringTableId` de l'asset via `UStringTable::GetStringTableId()` au lieu de dupliquer à l'aveugle une chaîne supposée.

## 5. Schéma CSV

Le schéma minimal obligatoire est :

~~~csv
Key,SourceString,Comment
~~~

Unreal impose `Key` et `SourceString` pour une String Table CSV. Les colonnes supplémentaires sont importées comme métadonnées.

LOCALIZATION01 fixe une seule métadonnée initiale :

~~~text
Comment
~~~

Aucune colonne `French`, `English`, `German`, `Italian`, etc. n'est autorisée.

Les traductions appartiennent au pipeline de localisation Unreal, pas au CSV d'authoring.

## 6. Exemple conforme

~~~csv
Key,SourceString,Comment
"CharacterCreation.Step.Race","Race","Titre de l'étape de sélection de race"
"CharacterCreation.Step.Class","Classe","Titre de l'étape de sélection de classe"
"CharacterCreation.Attribute.PointsRemaining","Points restants : {Current} / {Total}","Budget de caractéristiques"
"Interaction.Door.Open","Ouvrir la porte","Interaction générique"
"Interaction.Door.Close","Fermer la porte","Interaction générique"
"Combat.Hud.EndTurn","Terminer le tour actif","Tooltip du bouton fin de tour"
"Status.Duration.Permanent","Permanent","Durée d'un effet permanent"
~~~

## 7. Contrat des clés

### 7.1 Syntaxe

Les clés utilisent des segments ASCII séparés par un point :

~~~text
Domain.Area.Concept[.Variant]
~~~

Exemples :

~~~text
CharacterCreation.Step.Race
CharacterCreation.Validation.NameRequired
Progression.Skill.PurchaseFailed
Interaction.Door.Open
Interaction.Receptacle.PlaceItem
Combat.Hud.EndTurn
Combat.Hud.RoundFormat
Status.Duration.Turns
Inventory.Bag.WeightFormat
SaveLoad.Slot.Available
~~~

Pattern recommandé :

~~~text
^[A-Za-z][A-Za-z0-9]*(\.[A-Za-z][A-Za-z0-9]*)+$
~~~

### 7.2 Règles

Une clé :

- est stable dans le temps ;
- décrit la sémantique, pas la phrase française ;
- ne contient pas de culture (`.fr`, `.en`, etc.) ;
- ne contient pas de numéro de version ;
- ne contient pas de chemin d'asset ;
- ne contient pas le token interdit défini au `2 ;
- n'est pas renommée lorsqu'on corrige ou reformule le français.

Un renommage de clé est une migration de référence, pas une simple correction éditoriale.

## 8. Domaines de clés initiaux

LOCALIZATION01.2 justifie les domaines suivants :

~~~text
CharacterCreation.*
Recruitment.*
Progression.Skill.*
Progression.Attribute.*
Progression.LevelUp.*
Interaction.*
Combat.*
Status.*
Inventory.*
SaveLoad.*
Spellbook.*
Map.*
Startup.*
~~~

Ces domaines ne créent pas de nouvelles couches runtime. Ils organisent seulement les identités de texte.

## 9. Réutilisation ou duplication sémantique

Deux écrans peuvent partager une clé uniquement s'ils expriment **la même notion et doivent rester liés éditorialement**.

Exemple acceptable :

~~~text
Interaction.Item.Take
~~~

si tous les consommateurs doivent toujours afficher exactement la même action.

Exemple à éviter :

~~~text
Common.Invalid
~~~

réutilisé pour plusieurs erreurs sans contexte.

Deux `SourceString` identiques peuvent donc légitimement avoir deux clés différentes lorsque leur sens, leur genre grammatical ou leur contexte de traduction diffère.

## 10. Texte source

La culture source est :

~~~text
fr
~~~

Le CSV contient donc uniquement le français source.

Les textes doivent utiliser les caractères Unicode normaux :

~~~text
é è ê à ç œ ’ « »
~~~

Les approximations ASCII ne sont pas une convention de stockage.

## 11. Formats dynamiques

### 11.1 Placeholders nommés

Les nouveaux formats doivent préférer des placeholders nommés :

~~~text
{Character}
{Level}
{Current}
{Maximum}
{Count}
{Weight}
~~~

Exemple :

~~~csv
"SaveLoad.Slot.Indexed","Sauvegarde {Index}","Nom d'un slot numéroté"
~~~

Le code injecte ensuite les arguments avec `FText::FormatNamed` ou un `FFormatNamedArguments`.

Les nouveaux textes ne doivent pas introduire `%s`, `%d` ou `FString::Printf` pour construire une phrase joueur.

### 11.2 Pluriels

Les quantités grammaticales utilisent les modificateurs de format Unreal lorsque la phrase en a besoin.

Exemple conceptuel :

~~~text
{Count} {Count}|plural(one=point,other=points)
~~~

Le C++ fournit la valeur ; la ressource texte décide de la forme linguistique.

### 11.3 Valeurs mécaniques

Restent en C++ lorsqu'elles ne portent pas de langage humain :

~~~text
+10
-5
25%
x3
12 / 20
~~~

Dès qu'un libellé apparaît :

~~~text
Poids : 12 / 20
Rang 2 / 5
Niveau 4
~~~

le format complet devient un texte localisable.

## 12. Échappement CSV

Le fichier doit être un CSV UTF-8 séparé par des virgules.

Règles Unreal :

- les guillemets contenus dans un champ sont doublés : `""` ;
- les tabulations et nouvelles lignes contenues dans `SourceString` utilisent un échappement de style C ;
- les champs textuels sont systématiquement entourés de guillemets dans notre fichier source.

Exemple :

~~~csv
"Combat.Hotbar.RemoveHelp","Clic droit : retirer le raccourci.\nL’objet reste dans l’inventaire ou équipé.","Aide du raccourci assigné"
~~~

Excel peut être utilisé comme éditeur, mais le fichier commité doit rester un **CSV UTF-8 à séparateur virgule**, pas un `.xlsx` et pas un CSV à séparateur point-virgule dépendant des réglages régionaux.

## 13. Contrat d'import futur

Le prochain importer Editor devra être **idempotent**.

Pour un même `UI.csv` :

~~~text
Import #1 -> ST_UI
Import #2 -> aucun changement sémantique
Import #N -> même résultat
~~~

### 13.1 Validation avant écriture

L'import doit échouer avant toute sauvegarde si :

- le fichier est absent ;
- l'en-tête ne correspond pas au contrat ;
- `Key` est vide ;
- `SourceString` est vide ;
- une clé est dupliquée ;
- une clé ne respecte pas le pattern ;
- un identifiant de localisation contient le token interdit du `2 ;
- un format de texte est manifestement invalide ;
- un placeholder attendu est incohérent avec le contrat du consommateur lorsqu'un contrôle statique est possible.

### 13.2 Projection exacte

Après validation :

~~~text
ST_UI keys == UI.csv keys
ST_UI SourceString == UI.csv SourceString
ST_UI Comment == UI.csv Comment
~~~

Une ligne supprimée du CSV doit donc être supprimée de la projection générée.

Le but est d'empêcher les entrées fantômes et la double autorité.

### 13.3 Sauvegarde

L'importer :

1. modifie/crée uniquement `ST_UI` ;
2. ne touche à aucun widget ;
3. ne migre aucun texte C++ dans le même ticket ;
4. sauvegarde l'asset seulement si la projection diffère réellement.

## 14. Référencement runtime

Pour les références statiques dans UMG/Blueprint, préférer la référence directe à une entrée de String Table via une propriété `FText`.

Pour les résolutions C++ dynamiques, utiliser le mécanisme String Table natif Unreal (`FText::FromStringTable`).

Une façade C++ supplémentaire n'est pas justifiée tant qu'elle ne fournit pas une valeur concrète autre qu'un simple forwarding.

## 15. Contenu qui ne va PAS dans UI.csv

Le CSV UI ne remplace pas les autorités de contenu existantes.

Restent dans leurs Data Assets :

~~~text
DA_Class_*                  noms/descriptions classes et Talents
DA_Race_*                   noms/descriptions races
DA_Skill_*                  noms/descriptions Skills
DA_Item_*                   noms/descriptions items
DA_Status_*                 noms/descriptions Status Effects
DA_Spell_*                  noms/descriptions Spells
DA_MONCAT_*                 noms/descriptions catégories
DA_RPGTalentPresentation    présentation Talent authorée
~~~

`DA_RPGTalentDetailTextSet` reste temporairement son autorité existante.

Sa convergence éventuelle vers la String Table commune devra être un ticket séparé avec migration de références et tests de non-régression.

## 16. Diagnostics techniques

Les erreurs internes de validation, sérialisation ou gameplay ne sont pas automatiquement injectées dans `UI.csv`.

Exemple :

~~~text
Candidate CharacterId is invalid.
SaveVersion mismatch.
Inventory ownership invalid.
~~~

Ces diagnostics restent techniques.

Si une UI doit exprimer l'échec correspondant :

~~~text
service
    -> enum / stable reason + data
presentation
    -> UI key
    -> localized FText
~~~

Le service ne doit pas fournir une phrase anglaise ou française comme API de présentation.

## 17. Pipeline de traduction

Le CSV ne contient que le texte natif français.

Le pipeline visé est :

~~~text
UI.csv (fr source)
    -> ST_UI
    -> Gather Text
    -> Localization Target "Game"
    -> export .po
    -> traduction
    -> import .po
    -> compile .locres
~~~

Les premières cultures additionnelles pourront être décidées ultérieurement.

Aucune duplication `ST_UI_FR`, `ST_UI_EN`, etc. n'est autorisée.

## 18. Git et édition

### Autorité textuelle

~~~text
Localization/Source/UI.csv
~~~

est le fichier lisible et mergeable à revoir dans les diffs.

### Projection binaire

~~~text
/Game/Localization/StringTables/ST_UI
~~~

est généré.

Le `.uasset` peut être commité pour le runtime/cook, mais il n'est jamais l'endroit où l'on arbitre un conflit éditorial.

## 19. Tests attendus lors de l'implémentation

Le futur jalon d'implémentation devra au minimum vérifier :

1. parsing du CSV ;
2. unicité des clés ;
3. clés non vides ;
4. `SourceString` non vide ;
5. syntaxe des clés ;
6. interdiction case-insensitive du token du `2 dans les identifiants de localisation ;
7. import idempotent ;
8. exactitude CSV -> ST_UI ;
9. disparition d'une entrée supprimée de la source ;
10. résolution d'une entrée via `FText` ;
11. absence de modification de l'asset au second import identique.

Les tests de cette campagne utilisent désormais un préfixe neutre :

~~~text
Game.UI.Localization
~~~

## 20. Correction de convention LOCALIZATION01.1

Le filtre introduit par LOCALIZATION01.1 est renommé de :

~~~text
Grimrock.UI.Localization.LOCALIZATION01.TalentTextSetIntegrity
~~~

vers :

~~~text
Game.UI.Localization.LOCALIZATION01.TalentTextSetIntegrity
~~~

Le comportement du test ne change pas.

Cette correction aligne immédiatement la campagne LOCALIZATION01 sur la règle du `2.

## 21. Prochain jalon recommandé

Après validation de ce contrat :

~~~text
UI-LOCALIZATION01.4 — UI String Table CSV importer + integrity tests
~~~

Ce ticket devra :

- créer `Localization/Source/UI.csv` ;
- créer/matérialiser `ST_UI` ;
- implémenter l'import Editor-only ;
- implémenter les validations du `19 ;
- ne migrer encore aucun gros domaine UI.

La première migration fonctionnelle pourra ensuite cibler Character Creation.

## 22. Conclusion

Le contrat final est :

~~~text
contenu RPG authoré
    -> Data Assets existants

vocabulaire UI générique
    -> Localization/Source/UI.csv
    -> ST_UI générée

traductions
    -> Unreal Localization Target "Game"

mécanique / valeurs
    -> C++

diagnostics techniques
    -> C++ / logs / stable reasons
~~~

Aucun nouvel identifiant de localisation défini par LOCALIZATION01 ne doit contenir le token interdit du `2.
