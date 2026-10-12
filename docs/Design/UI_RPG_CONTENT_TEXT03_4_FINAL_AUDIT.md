# UI-RPG-CONTENT-TEXT03.4 — Final status text ownership audit and residual authority cleanup

Date : **12 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Ticket : **UI-RPG-CONTENT-TEXT03.4**

## 1. Objet

Clore CONTENT-TEXT03 après la séparation entre présentation sérialisée des
statuts, authoring mécanique Editor et projection runtime.

Aucun Data Asset binaire n'est modifié par ce ticket.

## 2. Autorité finale

```text
DA_Status_*
├─ EffectId
├─ DisplayName
├─ Description
└─ Icon
    => autorité de présentation sérialisée

Authoring C++ Warrior/Rogue/Ranger/Mage/Priest/Alchemist
├─ EffectId
├─ tags
├─ disposition / durée / stacking
├─ dégâts / soins périodiques
├─ initiative / contrôles
├─ modifiers / reactions
└─ bootstrap technique seulement
    => autorité mécanique Editor

Runtime
└─ DisplayName sérialisé, sinon EffectId
    => projection / fallback technique, aucune prose métier
```

## 3. Ownership des statuts de production

L'ownership contrôlé par CONTENT-TEXT03 couvre 32 statuts :

| Famille | Nombre |
|---|---:|
| Warrior | 5 |
| Rogue | 9 |
| Ranger | 1 |
| Mage | 4 |
| Priest | 7 |
| Alchemist | 6 |
| **Total** | **32** |

Chaque `EffectId` apparaît une seule fois dans cette carte d'authoring.
Un statut peut être consommé par plusieurs systèmes sans acquérir un deuxième
writer. `Status_Burning`, par exemple, est possédé par l'authoring Mage même
s'il est utilisé par des actions Alchimiste.

## 4. Préservation lors de la rematérialisation

Depuis TEXT03.2, un `DA_Status_*` existant dont l'`EffectId` correspond au
record rematérialisé conserve exactement :

```text
DisplayName
Description
```

Une `Description` vide est également autoritaire.

La mécanique est reconstruite avant restauration du texte puis la définition
est revalidée.

## 5. Bootstrap technique

Depuis TEXT03.3, un record neuf ne reçoit plus de prose joueur C++ :

```text
DisplayName = EffectId
Description = AUTHOR_TEXT_REQUIRED:<EffectId>
```

Les anciennes chaînes métier des six familles ont été supprimées des writers.

Le test historique Warrior qui inspectait littéralement `Esquive +2` dans la
description de `Status_Guarded` vérifie maintenant le contrat mécanique
`EvasionModifier == 2`.

## 6. Résolution runtime

Deux implémentations identiques du fallback de nom existaient dans :

```text
GridStatusEffectPresentation.cpp
GridStatusEffectLifecycleSubsystem.cpp
```

TEXT03.4 les remplace par une seule règle :

```cpp
FGridStatusEffectPresentationBuilder::ResolveDisplayName(State)
```

Ordre de résolution :

```text
DefinitionAsset->DisplayName non vide
        ↓
utilisé tel quel

sinon
        ↓
FText::FromName(State.EffectId)
```

Le fallback est purement technique. Il ne fabrique aucune formulation joueur.

La `Description` runtime reste lue directement depuis le `DA_Status_*`.

## 7. Texte UI générique hors autorité DA_Status

Les chaînes générées par le builder de présentation comme :

```text
Durée
Permanent
Stacks
Initiative
Dégâts périodiques actifs
Les actions de type Sort sont bloquées
```

décrivent la structure générique du tooltip/runtime. Elles ne constituent pas
le nom ou la description d'un statut particulier et restent donc hors de
l'autorité `DA_Status_*`.

## 8. Tests de clôture

Le filtre :

```text
Grimrock.UI.RPG.CONTENTTEXT03
```

contient quatre tests :

```text
StatusOwnershipAudit
Preservation
BootstrapOnly
FinalAudit
```

`FinalAudit` vérifie pour les 32 statuts :

- chargement et validité du Data Asset de production ;
- `EffectId` canonique ;
- absence de bootstrap technique dans la présentation de production ;
- projection exacte du `DisplayName` sérialisé ;
- fallback vers `EffectId` si le nom sérialisé est vide ;
- même fallback si la définition n'est pas résolue.

## 9. Conclusion

Après TEXT03.4, la présentation métier des statuts appartient aux
`DA_Status_*`.

Le C++ d'authoring possède la mécanique et les IDs stables. Le runtime lit les
données sérialisées et ne fournit qu'un fallback technique basé sur
`EffectId`.

CONTENT-TEXT03 peut être clos après validation locale.
