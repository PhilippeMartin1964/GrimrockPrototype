# UI-LOCALIZATION01.2 — Player-facing C++ text audit

> **Statut : CLOSED — audit historique.** Commit : `6ac148d385381d171b184d4f7b13fc0b58ce413c`. Les P0 identifiés ici ont été fermés par les jalons 01.5 à 01.12. Baseline finale P0 : `6e5cb5f14f532e2685117e4f803d5ae7a13c6e7e`.


Date : **13 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Ticket : **UI-LOCALIZATION01.2**  
Baseline audité : **e8d57f9c99c05bfd85c834ee02bc9003b10c9d41**  
État : **AUDIT UNIQUEMENT — aucune migration de texte, aucun .uasset modifié**

## 1. Objet

Cartographier les textes destinés au joueur qui restent détenus par le C++ après :

- `UI-RPG-TEXT01` ;
- `UI-RPG-CONTENT-TEXT01..05` ;
- `UI-LOCALIZATION01.1`.

L'audit ne cherche pas à supprimer tous les `FText` du C++.

Il distingue quatre responsabilités :

1. **contenu authoré** : noms/descriptions appartenant aux Data Assets ;
2. **vocabulaire système localisable** : `LOCTEXT/NSLOCTEXT`, déjà compatible avec la localisation Unreal ;
3. **wording joueur brut** : `FText::FromString`, `FString` ou `TEXT("...")` converti ensuite en texte affiché ;
4. **notation mécanique / diagnostic technique** : doit rester en C++ tant qu'il n'est pas présenté comme wording joueur.

## 2. Classification utilisée

### P0 — wording joueur brut à migrer

Texte humain directement affiché au joueur mais construit à partir de :

- `FText::FromString(TEXT("..."))` ;
- `FString(TEXT("..."))` ;
- `FString::Printf(TEXT("..."))` puis conversion en `FText` ;
- `FString Error` technique recopié tel quel dans un widget.

Ces textes ne possèdent pas actuellement d'identité de localisation stable.

### P1 — vocabulaire système déjà localisable

Texte porté par `LOCTEXT` ou `NSLOCTEXT`.

Ces textes restent une autorité de wording C++, mais ils sont déjà pris en charge par le pipeline de localisation Unreal.

Ils ne constituent donc pas un blocage pour plusieurs langues.

Une future centralisation dans une String Table / CSV peut être décidée pour l'authoring, mais n'est pas requise pour corriger un défaut de localisation.

### M — notation mécanique

Exemples :

- nombres ;
- `+/-` ;
- pourcentages ;
- `xN` ;
- `current / max` ;
- séparateurs ;
- niveaux en chiffres romains si la décision UX les considère universels.

Ces éléments restent en C++ selon la politique `UI-RPG-TEXT01`.

### T — diagnostic technique

Exemples :

- logs ;
- validations de sauvegarde ;
- erreurs de sérialisation ;
- invariants de runtime ;
- IDs techniques.

Ils ne doivent pas être transformés automatiquement en vocabulaire joueur.

Si un chemin UI commence à les afficher, il doit exposer une raison stable / enum et laisser la couche de présentation résoudre le texte joueur.

## 3. Résultat global

Le contenu RPG authoré est désormais correctement séparé :

~~~text
DA_Class_*                  -> classes / Talents / CombatActions
DA_RPGTalentPresentation    -> branches / nœuds conceptuels
DA_RPGTalentDetailTextSet   -> vocabulaire Talent générique
DA_Item_*                   -> items
DA_Status_*                 -> Status Effects
DA_MONCAT_*                 -> catégories de monstres
DA_Skill_*                  -> Skills
DA_Spell_*                  -> Spells
~~~

Le principal reliquat n'est donc plus le **contenu RPG**, mais le **vocabulaire UI/runtime**.

Les P0 les plus importants se trouvent dans :

~~~text
Character Creation / Custom Recruit
Status Effect presentation
Combat HUD
World interaction prompts
Inventory / save-slot presentation
quelques feedbacks Skills
~~~

Le dépôt contient également une quantité importante de P1 déjà correctement localisables avec `LOCTEXT/NSLOCTEXT`.

## 4. Talents — état satisfaisant

### 4.1 Autorités

Le pipeline Talent reste conforme :

~~~text
DA_Class_*                  -> noms/descriptions des Talents
DA_RPGTalentPresentation    -> noms/descriptions de présentation
DA_RPGTalentDetailTextSet   -> vocabulaire Talent générique
C++                         -> IDs / valeurs / mécanique / read-model
~~~

### 4.2 Occurrences C++ conservables

Les occurrences restantes dans les widgets Talent sont principalement mécaniques :

~~~text
I / II / III / IV / V
?
×N
current / max
joins / séparateurs
~~~

`GridSkillsPageService.cpp` reconstruit aussi certains `FText` depuis du texte déjà authoré dans les Data Assets, par exemple lors de la résolution de références de statuts.

Ce n'est pas une nouvelle autorité de wording.

### Décision

**Aucune nouvelle migration Talent n'est demandée par LOCALIZATION01.2.**

`DA_RPGTalentDetailTextSet` reste protégé par :

~~~text
Grimrock.UI.Localization.LOCALIZATION01.TalentTextSetIntegrity
~~~

## 5. Character Creation / Custom Recruit — P0 prioritaire

C'est la plus grosse zone de wording joueur non localisable.

### 5.1 `RPGCharacterCreationWidget.cpp`

Exemples directs :

~~~text
Féminin
Masculin
Race non configurée
Classe non configurée
Aucun portrait disponible pour cette race et ce sexe.
Aucune description disponible pour ce portrait.
Identité du personnage
Nom du personnage
Choix actuels
Race : {0}
Classe : {0}
Sexe : {0}
Variante de portrait
Description du portrait
Portrait final
Création du personnage impossible.
~~~

Le widget possède aussi plusieurs messages de validation construits avec `FText::FromString`.

### 5.2 `RPGCharacterCreationWizardWidget.cpp`

Le wizard contient du wording brut à toutes les étapes :

~~~text
Race
Classe
Caractéristiques
Identité
Résumé
~~~

ainsi que les textes de guidage / validation :

~~~text
Choisissez la race du personnage, puis passez à la classe.
Choisissez la classe du personnage, puis passez aux caractéristiques.
Caractéristiques validées. Vous pouvez passer à l'identité.
Identité validée. Vous pouvez consulter le résumé.
Il reste %d point(s) de caractéristiques à répartir.
Saisissez un nom pour pouvoir créer le personnage.
Tous les choix sont valides. Vous pouvez créer le personnage.
~~~

La page Résumé construit également en `FString` :

~~~text
Nom : ...
Race : ...
Classe : ...
Genre : ...
Portrait : ...
Caractéristiques finales
Statistiques dérivées
PV
Mana
Charge max.
Force
Dextérité
Constitution
Intelligence
Sagesse
Charisme
~~~

Ces chaînes finissent dans `SetWizardOptionalText()`, qui les convertit en `FText::FromString`.

Elles sont donc P0 même si l'appel `FText::FromString` n'apparaît pas sur la ligne où le français est écrit.

### 5.3 `RPGCharacterCreationAttributesStepWidget.cpp`

P0 :

~~~text
Points restants : {0} / {1}
Les valeurs de classe peuvent être ajustées. Les bonus de race sont appliqués ensuite.
Charge ... · CàC ...
Précision ... · Esquive ...
Santé ... · Résistance ...
Savoirs ... · Alchimie ...
Perception ... · Volonté ...
Influence ... · Recrutement ...
~~~

Les valeurs numériques et signes restent mécaniques ; seuls les libellés/formats linguistiques doivent sortir.

### 5.4 Erreurs de services remontées au widget

`GridPartyInventoryComponent.cpp` retourne des `FText` français qui peuvent être affichés directement par le wizard :

~~~text
La création initiale du personnage est déjà terminée.
Le nom du personnage doit contenir entre 1 et 24 caractères.
Une définition de race valide est requise.
Une définition de classe valide est requise.
Toutes les caractéristiques initiales doivent être comprises entre 6 et 20.
~~~

`RPGCustomRecruitService.cpp` retourne actuellement des `FString Error` en anglais :

~~~text
Party inventory component is missing.
Initial party character creation is not completed.
Active party is full.
Custom recruit display name must contain between 1 and 24 characters.
A valid race definition is required.
A valid class definition is required.
...
~~~

Le wizard utilise :

~~~text
Result.Error -> FText::FromString(ErrorMessage)
~~~

Ces erreurs techniques peuvent donc devenir du texte joueur anglais dans une UI française.

### Décision

**P0 le plus prioritaire du prochain chantier.**

La cible ne doit pas être de déplacer les règles dans un Data Asset.

La règle recommandée est :

~~~text
service C++
    -> enum / reason stable + valeurs dynamiques

UI
    -> résolution d'une identité de texte localisable
~~~

## 6. Recruitment compagnon — mélange P1 / P0

`RPGStoryCompanionRecruitmentWidget.cpp` utilise correctement `LOCTEXT` pour son vocabulaire principal :

~~~text
Recruter
Refuser
Voir la fiche
Ce compagnon souhaite rejoindre le groupe.
Ce compagnon fait déjà partie du groupe.
...
~~~

=> **P1, déjà localisable.**

En revanche :

~~~text
RPGStoryCompanionService.Error
RPGPartyRecruitmentService.Error
    -> BuildRegistrationFailureText()
    -> BuildRecruitmentFailureText()
    -> FText::FromString(Error)
~~~

Les services produisent des diagnostics anglais tels que :

~~~text
Party inventory component is missing.
Story companion definition is invalid.
Active party is full.
Candidate CharacterId is invalid.
Candidate RPG progression state is invalid.
...
~~~

Ils peuvent être affichés tels quels.

=> **P0**, même défaut de frontière que Custom Recruit.

## 7. Skills / Attributes / Level Up

### 7.1 P0 restant

`GridSkillsWidget.cpp` possède encore au moins :

~~~text
Attribution de compétence impossible.
Annulation de compétence impossible.
~~~

sous forme de `FText::FromString`.

=> P0.

### 7.2 P1 déjà localisable

`RPGProgressionFeedbackService.cpp` utilise `LOCTEXT` pour :

- raisons de refus Skill ;
- achat / annulation de rang ;
- progression d'attributs ;
- montée de niveau.

Exemples :

~~~text
Il ne reste aucun point de compétence.
Compétence améliorée
Caractéristique améliorée
Niveau {0} atteint
+{0} point(s) de compétence.
Rang maximal des compétences : {0}.
~~~

`GridSkillEntryWidget.cpp` utilise également `NSLOCTEXT` pour :

~~~text
Compétence inconnue
Rang {0} / {1}
Entraînée
Test possible sans entraînement
Entraînement requis
~~~

`GridCharacterSheetWidget.cpp` utilise `LOCTEXT` pour les points de caractéristiques et les six noms d'attributs.

=> **P1 : déjà localisable, pas un bug de localisation.**

### Décision

Le chantier Skills/Attributes/LevelUp doit d'abord supprimer les deux P0.

La migration éventuelle des P1 vers une String Table relève d'une décision d'authoring centralisé, pas d'une urgence runtime.

## 8. Status Effects — P0 prioritaire

Les noms et descriptions des statuts viennent correctement des `DA_Status_*`.

En revanche, `GridStatusEffectPresentation.cpp` reconstruit encore tout le vocabulaire de présentation en C++ brut :

~~~text
Durée : ...
Stacks : ...
Initiative : ...
Dégâts périodiques actifs.
La prochaine activation correspondante est perdue.
Les actions de type Sort sont bloquées.
Les déplacements de case sont bloqués.

1 tour
%d tours
1 manche
%d manches
Permanent
~~~

Les formes compactes :

~~~text
T%d
R%d
PERM
~~~

sont également des abréviations de langage/UX, pas des valeurs numériques pures.

=> **P0.**

Les noms/descriptions du statut doivent rester dans `DA_Status_*`; seuls les formats et libellés génériques doivent rejoindre une autorité UI localisable.

## 9. Interactions monde — P0 et mélange de langues

Plusieurs `GetInteractionText_Implementation()` contiennent du texte anglais brut :

### `GridDoorActor.cpp`

~~~text
Open door
Close door
~~~

### `GridLeverActor.cpp`

~~~text
Push lever
Pull lever
~~~

### `GridButtonActor.cpp`

~~~text
Push button
~~~

### `GridItemActor.cpp`

~~~text
Take
~~~

### `GridReceptacleActor.cpp`

~~~text
Take
Place item
~~~

### `GridGenericObjectActor.cpp`

~~~text
Read
~~~

`GridWallLockActor.cpp` contient à l'inverse des fallbacks français :

~~~text
La serrure est verrouillée.
La serrure s'ouvre avec un déclic métallique.
La serrure est déjà déverrouillée.
Il vous manque la clé adéquate.
~~~

=> **P0.**

Ces textes sont du vocabulaire d'interaction générique et ne doivent pas être recopiés dans chaque World Object Data Asset.

La cible naturelle est une autorité UI/système commune.

## 10. Combat — P0 + P1

### 10.1 `GridCombatHudWidget.cpp` — P0

Le HUD contient encore un volume significatif de wording brut :

~~~text
Personnage indisponible.
Définition d'objet indisponible.
Lancer {0}
Aucun exemplaire dans l'inventaire.
Déposez ici un objet de l'inventaire, un sort ou une capacité.
ACTIF
Combat inactif
Tour ennemi
Déplacement en cours
Résolution en cours
Terminer le tour actif
ROUND %d
~~~

et le tooltip de raccourci construit en `FString` :

~~~text
Coût : ...
quantité : ...
consomme ...
Indisponible : ...
Clic droit : retirer le raccourci.
L’objet reste dans l’inventaire ou équipé.
~~~

=> **P0.**

### 10.2 `GridCombatActionPanelWidget.cpp` — P0

~~~text
ACTIF
INCAPACITÉ
VAINCU
PA ...
~~~

Les nombres restent mécaniques ; les libellés sont localisables.

### 10.3 `GridTurnManagerPlayerActionCatalog.cpp` — P0

~~~text
Cible invalide.
Un effet de statut empêche l'utilisation des sorts.
~~~

### 10.4 Combat déjà localisable — P1

Les fichiers suivants utilisent déjà `LOCTEXT/NSLOCTEXT` :

- `GridCombatLog.cpp` ;
- `GridCombatActionCatalog.cpp` ;
- `GridPlayerAttackPresentationTypes.cpp` ;
- `GridPlayerAttackPresentationComponent.cpp` ;
- une partie de `GridTurnManagerComponent.cpp`.

Ils couvrent notamment :

~~~text
Le combat commence.
Manche {0}.
Victoire.
Défaite.
Coup critique...
Aucun combat n’est actif.
Ce personnage n’a pas assez de mana.
La cible est hors de portée.
...
~~~

=> **P1, déjà localisable.**

### 10.5 Attack_Unarmed

La décision `UI-RPG-CONTENT-TEXT05.7` reste inchangée :

~~~text
Attack_Unarmed
    -> primitive runtime universelle
    -> DisplayName / Description en LOCTEXT
~~~

LOCALIZATION01.2 ne rouvre pas cette décision.

## 11. Inventaire / Items

### 11.1 P0

`GridInventoryWidget.cpp` contient notamment :

~~~text
Poids : %.1f / %.1f
~~~

via `FText::FromString`.

Les autres helpers numériques `current/max`, bonus signés, etc. sont principalement **M**.

### 11.2 P1

`GridInventorySlotWidget.cpp` utilise déjà `NSLOCTEXT` pour :

- slots d'équipement ;
- types d'items ;
- attributs ;
- résistances ;
- statistiques.

`GridItemContextActionLibrary.cpp` utilise `NSLOCTEXT` pour :

- Examiner ;
- Équiper / Enlever ;
- Consommer ;
- Lire ;
- Lancer ;
- Déposer ;
- actions sur serrures / alcôves / supports ;
- raisons d'indisponibilité.

=> **P1.**

Remarque de contenu : certaines chaînes actuelles y sont orthographiées sans accents (`tete`, `epaules`, `S'equiper`). C'est un problème de wording, pas une faille d'architecture.

## 12. Spellbook

`GridSpellbookEntryWidget.cpp` utilise `NSLOCTEXT` pour le vocabulaire générique :

~~~text
Sort inconnu
Définition introuvable
Mana {0} | PA {1}
Portée {0}
Non assigné
Raccourci {0}
~~~

=> **P1.**

`GridSpellbookUI.cpp` possède encore le fallback brut :

~~~text
Définition de sort indisponible.
~~~

=> **P0**, mais uniquement fallback de présentation.

Les erreurs internes de `GridSpellEffectResolver` / `GridSpellHotbarExecution` sont actuellement consommées par le pipeline comme diagnostics/logs et ne sont pas projetées directement dans la présentation C++ observée.

=> **T** tant que cette frontière reste vraie.

## 13. Sauvegardes / slots

### 13.1 P0 visible

`GrimrockGameInstance.cpp` :

~~~text
Sauvegarde principale
Sauvegarde %d
~~~

`GrimrockLoadGameSlotWidget.cpp` :

~~~text
Disponible
Indisponible
~~~

=> **P0.**

### 13.2 Erreurs techniques — T actuellement

Les fichiers suivants contiennent beaucoup de `FText::FromString` français :

- `GrimrockPartyPawnSave.cpp` ;
- `GrimrockPartySaveGame.cpp` ;
- `GrimrockSaveSlotLibrary.cpp` ;
- `GridCombatSavePolicy.cpp` ;
- parties de `GridPartyInventoryComponent.cpp`.

Ils décrivent notamment :

- version de sauvegarde incompatible ;
- état runtime invalide ;
- ownership invalide ;
- asset introuvable ;
- checkpoint invalide ;
- sérialisation impossible.

Dans les consommateurs C++ actuels audités, une grande partie de ces erreurs est journalisée avec `UE_LOG` plutôt qu'affichée au joueur.

=> **T pour l'instant.**

Règle future : si une UI Save/Load affiche ces erreurs, elle ne doit pas exposer directement la prose technique. Elle doit traduire une raison stable en message joueur localisable.

## 14. Startup / chargement du donjon — P1

`GridDungeonBuildProgressWidget.cpp` et `GrimrockStartupModeComponent.cpp` utilisent `LOCTEXT` :

~~~text
Construction du donjon
Préparation...
Chargement de la partie
Construction de la géométrie...
Placement du groupe...
Donjon prêt.
Partie chargée.
~~~

=> **P1, déjà localisable.**

## 15. Carte — P1

`GridMapWidget.cpp` utilise :

~~~text
Niveau {0}
~~~

via `NSLOCTEXT`.

=> **P1.**

## 16. Ce qui ne doit pas être externalisé par réflexe

Les éléments suivants ne doivent pas être déplacés simplement parce qu'ils passent par `FText` ou `FString` :

~~~text
+10
-25
10%
+10%
x3
12 / 20
IDs techniques
noms de slots internes
noms d'assets
messages UE_LOG
invariants de validation
~~~

Le test de LOCALIZATION01 doit continuer à protéger la frontière définie par `UI-RPG-TEXT01` :

~~~text
langage humain -> ressource localisable
notation mécanique -> C++
contenu authoré -> Data Asset
diagnostic technique -> C++ / log
layout -> UMG
~~~

## 17. Autorité cible recommandée

L'audit ne recommande **pas** de créer un nouveau Data Asset spécialisé pour chaque groupe de messages.

La cible proposée pour la suite est :

### 17.1 Contenu RPG

Reste dans les Data Assets existants :

~~~text
DA_Class_*
DA_Skill_*
DA_Race_*
DA_Item_*
DA_Status_*
DA_Spell_*
DA_MONCAT_*
DA_RPGTalentPresentation
~~~

### 17.2 Vocabulaire UI / système

Une autorité centralisée à définir au prochain jalon :

~~~text
String Table Unreal
    <- source CSV UTF-8 versionnée
~~~

Domaines de clés possibles :

~~~text
CharacterCreation.*
Progression.Skill.*
Progression.Attribute.*
Progression.LevelUp.*
Interaction.*
Combat.*
Status.*
Inventory.*
SaveLoad.*
Recruitment.*
Spellbook.*
Map.*
Startup.*
~~~

Le CSV serait une source d'authoring/versioning ; les cultures traduites doivent utiliser le pipeline de localisation Unreal plutôt qu'un Data Asset par langue.

### 17.3 Services gameplay

Les services ne devraient pas produire de phrase destinée au joueur.

Cible :

~~~text
enum / stable reason
+ données dynamiques
        ↓
présentation
        ↓
texte localisable
~~~

Cela concerne en priorité :

- Character Creation ;
- Custom Recruit ;
- Story Companion registration ;
- Party Recruitment.

## 18. Priorités issues de l'audit

### P0-A — Character Creation / Recruitment

Plus gros bloc de texte brut et présence d'erreurs anglaises directement projetables dans l'UI.

### P0-B — Status Effect presentation

Vocabulaire entièrement construit en français brut malgré des DA_Status correctement authorés.

### P0-C — World interaction prompts

Mélange anglais/français visible immédiatement en jeu.

### P0-D — Combat HUD

Beaucoup de wording brut, formats et instructions joueur.

### P0-E — Save-slot / Inventory / petits fallbacks

Surface plus petite, correction simple après mise en place de l'autorité commune.

### P1 — LOCTEXT/NSLOCTEXT existants

Déjà localisables. Leur centralisation peut être faite progressivement sans urgence fonctionnelle.

## 19. Stratégie de tests

LOCALIZATION01.2 n'ajoute volontairement **aucun test qui fige le nombre actuel de littéraux C++**.

Un tel test caractériserait la dette comme un contrat et devrait être modifié à chaque migration.

La stratégie recommandée est :

1. définir l'autorité de vocabulaire UI commune ;
2. migrer un domaine ;
3. ajouter ensuite un garde-fou source pour ce domaine ;
4. généraliser le contrôle d'intégrité des catalogues de texte lorsque plusieurs assets présentent le même contrat que `DA_RPGTalentDetailTextSet`.

Le garde-fou de LOCALIZATION01.1 reste valide et doit être généralisé seulement lorsqu'une seconde structure réellement équivalente existe.

## 20. Conclusion

L'audit confirme trois faits :

1. **les Talents et le contenu RPG authored ne sont plus le problème principal** ;
2. **Character Creation, Status Effects, interactions monde et Combat HUD contiennent encore du wording joueur non localisable** ;
3. **beaucoup d'autres chaînes C++ sont déjà en LOCTEXT/NSLOCTEXT et sont donc localisables sans migration urgente**.

La prochaine décision architecturale doit porter sur la source commune du vocabulaire UI :

~~~text
String Table Unreal
+ CSV UTF-8 versionné
+ clés stables
+ pipeline de localisation Unreal
~~~

avant de lancer les migrations massives.

UI-LOCALIZATION01.2 est un audit uniquement : aucun runtime, Data Asset, Blueprint ou comportement gameplay n'est modifié.
