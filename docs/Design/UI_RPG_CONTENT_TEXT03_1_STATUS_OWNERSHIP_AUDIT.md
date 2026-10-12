# UI-RPG-CONTENT-TEXT03.1 — Audit DA_Status text ownership and authoring overwrite paths

Date : **12 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Ticket : **UI-RPG-CONTENT-TEXT03.1**

> **État après TEXT03.3 :** cet audit décrit volontairement l'état initial observé en TEXT03.1.  
> TEXT03.2 a ajouté la préservation du texte sérialisé et TEXT03.3 a supprimé l'autorité prose C++ ainsi que la dépendance textuelle du test Warrior.

## 1. Objet

Caractériser l'autorité texte actuelle des `UGridStatusEffectDefinitionAsset`
sans modifier les Data Assets ni changer la mécanique.

Le ticket mesure les writers C++ qui rematérialisent les statuts de production,
les champs texte qu'ils écrasent et le fallback runtime qui consomme ces
champs.

## 2. Autorité sérialisée candidate

Le Data Asset de statut expose deux champs texte métier :

```text
UGridStatusEffectDefinitionAsset
├─ EffectId
├─ DisplayName
└─ Description
```

`IsValidDefinition()` exige un `DisplayName` non vide. La
`Description` peut être vide ; TEXT03.1 la compte mais n'invente pas une
nouvelle règle éditoriale.

## 3. Writers de production identifiés

Six familles d'authoring reconstruisent des `DA_Status_*` :

| Famille | Statuts |
|---|---:|
| Warrior | 5 |
| Rogue | 9 |
| Ranger | 1 |
| Mage | 4 |
| Priest | 7 |
| Alchemist | 6 |
| **Total** | **32** |

Chemins :

```text
FRPGWarriorAuthoring::ConfigureStatus()
FRPGRogueAuthoring::ConfigureStatus()
FRPGRangerAuthoring::ConfigureMarkedStatus()
FRPGMageAuthoring::ConfigureStatus()
FRPGPriestAuthoring::ConfigureStatus()
FRPGAlchemistAuthoring::ConfigureStatus()
```

Les commandlets associés utilisent des chemins `FindOrCreateStatus ->
Configure*Status -> SaveAuthoredAsset`, sauf Ranger qui possède son helper
spécifique pour `Status_MarkedByRanger`.

## 4. Partage de statuts

Les 32 entrées d'ownership sont uniques.

Des statuts peuvent être consommés par plusieurs systèmes sans appartenir à
plusieurs writers. Exemple : `Status_Burning` est utilisé par l'Alchimiste,
mais appartient au jeu de statuts rematérialisé par le Mage ; l'authoring
Alchimiste ne le liste pas parmi ses six statuts possédés.

## 5. Double autorité actuelle

Les six authorings écrivent aujourd'hui directement :

```text
StatusAsset.DisplayName
StatusAsset.Description
```

avec du texte joueur C++.

TEXT03.1 injecte des sentinelles dans des duplicatas transient de chacun des
32 assets et appelle le writer correspondant. Les modifications sont
journalisées sous :

```text
[UI-RPG-CONTENT-TEXT03.1][TEXT-OVERWRITE]
```

Ces entrées sont informatives et ne doivent pas produire de warning
Automation.

## 6. Runtime de présentation

`FGridStatusEffectPresentationBuilder` lit directement la présentation du
Data Asset :

```text
DisplayName -> DefinitionAsset->DisplayName
Description -> DefinitionAsset->Description
Icon        -> DefinitionAsset->Icon
```

Si `DisplayName` est vide ou si la définition n'est pas résolue, le runtime
utilise uniquement le fallback technique :

```text
FText::FromName(State.EffectId)
```

Il n'existe donc pas de deuxième prose métier de statut dans ce fallback.

Les chaînes génériques de durée, stacks et restrictions du tooltip
(`Durée`, `Permanent`, etc.) relèvent du vocabulaire UI générique, pas de
l'autorité texte d'un `DA_Status_*`.

## 7. Dépendances de tests à nettoyer plus tard

L'audit des suites d'authoring trouve une dépendance directe au prose C++ dans :

```text
RPGRPG0391WarriorAuthoringTests.cpp
```

Le test de `Status_Guarded` exige actuellement que la description contienne
littéralement `Esquive +2`.

Cette dépendance devra être convertie vers une vérification mécanique ou une
lecture du Data Asset sérialisé lorsque TEXT03 retirera l'autorité prose du
C++.

Aucune modification de ce test n'est faite dans TEXT03.1.

## 8. Test de caractérisation

Le filtre introduit :

```text
Grimrock.UI.RPG.CONTENTTEXT03.StatusOwnershipAudit
```

Il vérifie :

- 32 statuts d'ownership uniques ;
- répartition 5 / 9 / 1 / 4 / 7 / 6 ;
- chargement des 32 `DA_Status_*` de production ;
- `EffectId` canonique ;
- validité structurelle ;
- `DisplayName` sérialisé non vide ;
- reconstruction effective de la mécanique sur duplicata transient ;
- comptage des `DisplayName` et `Description` écrasés.

Aucun package n'est sauvegardé.

## 9. Périmètre suivant

TEXT03.1 ne corrige pas encore le comportement.

La suite logique est de préserver le texte sérialisé par `EffectId` lors de
la rematérialisation mécanique, sur le modèle de CONTENT-TEXT01.2 et
CONTENT-TEXT02.2.
