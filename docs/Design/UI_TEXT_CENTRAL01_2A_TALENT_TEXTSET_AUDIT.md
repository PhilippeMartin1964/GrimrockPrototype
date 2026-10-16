# UI-TEXT-CENTRAL01.2A — Talent TextSet read-only audit

Date : **16 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**  
État : **SOURCE IMPLÉMENTÉE — audit local requis**

## 1. Objet

Préparer la convergence de :

~~~text
DA_RPGTalentDetailTextSet
    -> Localization/Source/UI.csv
    -> ST_UI
~~~

sans modifier encore le runtime Talent ni le Data Asset de production.

Le TextSet Talent est actuellement une autorité binaire. Avant de migrer ses
textes vers le CSV central, il faut inventorier les TextIds et surtout leur
texte français exact sans reconstruire ou deviner les phrases depuis leur nom.

## 2. Commandlet ajouté

~~~text
UITalentTextSetAudit
~~~

Il charge uniquement :

~~~text
/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_RPGTalentDetailTextSet
~~~

et produit :

~~~text
Saved/Localization/UI-TEXT-CENTRAL01.2A/TalentTextSetAudit.csv
Saved/Localization/UI-TEXT-CENTRAL01.2A/TalentTextSetAudit_summary.txt
~~~

Le commandlet est strictement **read-only** :

- aucun SetEditorProperty ;
- aucune sauvegarde de package ;
- aucun changement de ST_UI ;
- aucun changement de DA_RPGTalentDetailTextSet.

## 3. Classification conservatrice

Chaque entrée reçoit une disposition initiale :

~~~text
MIGRATE_ST_UI
    vocabulaire UI Talent : Section, Action, Type, Status, Node, Screen,
    Feedback, Detail, Effect, Context, Skill, Party, Reaction, Acquisition

DEFER_CONTENT_AUTHORITY
    Talent.Recipe.*
    -> noms de recettes : contenu métier à rattacher ultérieurement
       à l'autorité Crafting/Recipe appropriée

REVIEW
    tout TextId qui ne rentre pas dans ces familles
~~~

Cette classification ne modifie rien. Elle sert uniquement à préparer
UI-TEXT-CENTRAL01.2B.

## 4. Exécution

Après build Editor :

~~~powershell
& "D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "D:\Development\GrimrockPrototype\GrimrockPrototype.uproject" `
  -run=UITalentTextSetAudit `
  -unattended -nop4 -nosplash -nullrhi
~~~

Le log doit se terminer par :

~~~text
[UI-TEXT-CENTRAL01.2A] Read-only audit completed.
~~~

Le résumé affiche :

~~~text
Entries
MIGRATE_ST_UI
DEFER_CONTENT_AUTHORITY
REVIEW
Already present in UI.csv
Invalid entries
Duplicate TextIds
~~~

## 5. Validation attendue

Avant toute migration :

~~~text
Invalid entries   = 0
Duplicate TextIds = 0
~~~

Le fichier CSV doit ensuite être relu pour confirmer que :

- les textes français exportés correspondent au Data Asset actuel ;
- aucune entrée de contenu métier n'est aspirée par erreur dans ST_UI ;
- les clés réutilisables entre écrans peuvent éventuellement être renommées
  sémantiquement avant migration, plutôt que copiées mécaniquement.

## 6. Suite

~~~text
UI-TEXT-CENTRAL01.2B — Talent UI vocabulary migration
~~~

01.2B utilisera le CSV d'audit comme vérité de migration, ajoutera uniquement
le vocabulaire UI validé dans Localization/Source/UI.csv, puis migrera les
consommateurs vers FUILocalizationText::Get().

DA_RPGTalentDetailTextSet ne sera retiré qu'après régressions Talent vertes.
