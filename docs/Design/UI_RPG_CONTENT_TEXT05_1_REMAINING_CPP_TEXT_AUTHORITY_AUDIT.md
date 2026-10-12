# UI-RPG-CONTENT-TEXT05.1 — Audit remaining RPG player-facing C++ text authorities

Date : **12 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Ticket : **UI-RPG-CONTENT-TEXT05.1**

> **État après TEXT05.5 :** la dette Skills est fermée et l’infrastructure de matérialisation Spell est prête. UGridSpellDefinitionAsset est identifié par GridSpell:<SpellId> et FGridSpellCatalogAuthoring peut créer/réparer les quatre assets via Unreal en préservant leur texte et leur présentation authored. Les binaires DA_Spell_* restent à matérialiser localement dans un ticket atomique séparé avant la bascule runtime.

## 1. Objet

Après la fermeture de CONTENT-TEXT01 à CONTENT-TEXT04, auditer transversalement
le dépôt pour identifier les **dernières autorités de contenu joueur encore
portées par le C++**.

Ce ticket est un audit uniquement :

- aucun texte n'est migré ;
- aucun `.uasset` ou `.umap` n'est modifié ;
- aucun commandlet de sauvegarde n'est exécuté ;
- les textes UI génériques, messages d'erreur et fallbacks techniques sont
  distingués des vraies données de contenu.

## 2. Domaines déjà fermés

Les audits précédents ont retiré la prose métier C++ de :

```text
CONTENT-TEXT01
    DA_Class_* / Choices / CombatActions / Talent presentation

CONTENT-TEXT02
    DA_Item_* / QuickItems

CONTENT-TEXT03
    DA_Status_*

CONTENT-TEXT04
    DA_MONCAT_*
```

Ces domaines ne constituent donc plus la cible de TEXT05.1.

## 3. Résultat global

Trois autorités de contenu joueur subsistent réellement :

| Priorité | Domaine | Autorité C++ restante | Autorité sérialisée disponible |
|---|---|---|---|
| P1 | Skills | 25 `DisplayName` | Oui — 25 `DA_Skill_*` |
| P2 | Spells | 4 noms + 4 descriptions | Type DataAsset existant, aucun asset de production |
| P3 | Action universelle | nom + description de `Attack_Unarmed` | Non dédiée actuellement |

Le reste des chaînes trouvées appartient principalement au vocabulaire UI
générique, aux messages de refus/erreur, aux fixtures de tests ou aux fallbacks
techniques.

---

## 4. P1 — Skills : double autorité confirmée

### 4.1 Assets de production

Le dépôt contient exactement **25** assets :

```text
/Game/GrimrockPrototype/Core/DataAssets/RPG/Skills/
    DA_Skill_*
```

`URPGSkillAsset` possède notamment :

```text
SkillId
DisplayName
Description
GoverningAttribute
MaxRank
bAllowUntrainedChecks
RequirementGrants
```

### 4.2 Autorité C++ restante

`FRPGSkillCatalogAuthoring::GetCanonicalDefinitions()` contient encore les
25 libellés joueur, par exemple :

```text
Skill_HeavyWeapons  -> Armes lourdes
Skill_LightWeapons  -> Armes légères
Skill_RangedWeapons -> Armes à distance
...
Skill_Deception     -> Tromperie
```

Puis :

```cpp
SkillAsset.DisplayName = Definition.DisplayName;
```

Un rerun de l'authoring peut donc reprendre l'autorité sur le
`DisplayName` sérialisé.

TEXT05.1 le caractérise en dupliquant chaque `DA_Skill_*`, injectant un
`DisplayName` sentinelle puis appelant `ConfigureSkill()`.

Le compteur :

```text
[SKILL-TEXT-OVERWRITE]
```

mesure les reprises d'autorité sans sauvegarder l'asset.

### 4.3 Description

Le cas est déjà meilleur pour `Description`.

`ConfigureSkill()` ne la modifie pas et les tests historiques garantissent
déjà sa conservation.

La dette Skills porte donc principalement sur :

```text
DisplayName
```

et non sur toute la structure `URPGSkillAsset`.

### 4.4 Priorité

C'est le prochain chantier le plus direct parce que :

- les 25 Data Assets existent déjà ;
- le runtime les consomme déjà ;
- le problème est exactement une double autorité de présentation ;
- la correction peut suivre le modèle TEXT02/TEXT03/TEXT04 sans nouvelle
  architecture.

---

### 4.5 État après TEXT05.3

La dette P1 est désormais fermée :

```text
FRPGCanonicalSkillDefinition
    = SkillId + mécanique uniquement

DA_Skill_*.DisplayName
    = autorité joueur

record neuf
    = DisplayName = SkillId (bootstrap technique)
```

Les 25 libellés français ont été retirés de l'authoring C++.

---

## 5. P2 — Spells : contenu de production encore intégralement C++

### 5.1 Bibliothèque actuelle

`FGridProductionSpellLibrary` construit quatre définitions :

```text
Spell_ArcaneBolt
Spell_LesserHeal
Spell_Haste
Spell_CurePoison
```

Chaque définition reçoit directement en C++ :

```text
DisplayName
Description
School
coûts
portées
targeting
effects
```

Les quatre noms et les quatre descriptions sont donc des autorités joueur C++.

Exemples actuels :

```text
Arcane Bolt
Lesser Heal
Haste
Cure Poison
```

avec leurs descriptions anglaises directement dans
`GridProductionSpellLibrary.cpp`.

### 5.2 Contradiction architecturale utile

Le projet possède déjà :

```cpp
UGridSpellDefinitionAsset : UPrimaryDataAsset
```

qui expose :

```text
FGridSpellDefinition Definition
FGridSpellPresentationProfile Presentation
```

La documentation MON18.1 le décrit explicitement comme conteneur data-driven.

Cependant, l'audit du tree Git ne trouve **aucun DataAsset de sort de
production**. Les seuls assets contenant `Spell` sont actuellement les
widgets Spellbook.

La production continue donc à passer par la bibliothèque C++.

### 5.3 Nature de la dette

Contrairement aux Skills, il ne s'agit pas encore d'une double autorité :

```text
Skills = DataAsset + C++        -> double autorité

Spells = C++ uniquement         -> contenu non matérialisé
```

Le futur chantier Spell devra donc d'abord décider comment matérialiser les
quatre définitions dans les `UGridSpellDefinitionAsset` déjà prévus, sans
inventer une nouvelle couche.

---

## 6. P3 — Action universelle Attack_Unarmed

`FGridCombatActionCatalog::MakeUnarmedAttackDefinition()` possède encore :

```cpp
ActionId    = Attack_Unarmed
DisplayName = À mains nues
Description = Attaque physique effectuée sans arme.
```

La mécanique universelle de l'attaque à mains nues a une justification forte
pour rester une primitive C++.

En revanche, son **libellé joueur** et sa **description joueur** constituent
encore une petite autorité de contenu C++.

Cette dette est isolée : un seul record, sans catalogue sérialisé dédié.

Elle est classée P3 afin de ne pas créer prématurément un nouveau DataAsset
uniquement pour deux chaînes.

Une solution future peut réutiliser une autorité de vocabulaire/presentation
existante si cela reste simple ; TEXT05.1 ne choisit pas encore cette solution.

---

## 7. Recipes / Crafting : pas de nouvelle autorité prose C++ détectée

Les `Recipe_*` restent des identités techniques.

Le read-model construit des clés stables :

```text
Talent.Recipe.Recipe_*
```

résolues par le `DA_RPGTalentDetailTextSet`.

Aucun :

```text
HumanizeId(RecipeId)
```

n'est présent dans le chemin courant.

Les recettes ne nécessitent donc pas un nouveau chantier d'autorité texte à
ce stade.

---

## 8. Races, Classes, Companions et monstres individuels

L'audit des writers de production ne trouve pas de nouvelle table C++ de
`DisplayName` comparable à celle des Skills.

Les modèles suivants portent déjà leur présentation dans leurs données :

```text
URPGRaceAsset
URPGClassAsset
URPGStoryCompanionAsset
UGridMonsterDefinitionAsset
```

Les fallbacks trouvés dans les consommateurs sont de nature technique, par
exemple :

```text
RaceId
ClassId
MonsterId
ItemDefinitionId
SkillId
EffectId
```

Ils ne constituent pas une deuxième prose métier authored en C++.

---

## 9. Textes C++ explicitement hors migration de contenu

Le scan trouve encore de nombreux textes joueur dans le C++, mais ils relèvent
du **vocabulaire d'interface ou des messages système**, pas de la définition
d'un contenu RPG particulier.

Exemples :

```text
Aucun combat n'est actif
Ce personnage n'a pas assez de mana
Compétence inconnue
Entraînement requis
Sauvegarde principale
Race non configurée
Construction du donjon
Take / Read
Lancer {ItemDisplayName}
```

Il serait possible de centraliser/localiser certains de ces textes dans un
chantier UX/i18n séparé, mais les déplacer dans les Data Assets métier serait
une mauvaise séparation d'autorité.

TEXT05.1 ne les considère donc pas comme dette `CONTENT-TEXT`.

---

## 10. Fallbacks techniques

Plusieurs consommateurs utilisent encore un ID stable comme fallback :

```text
FText::FromName(SkillId)
FText::FromName(ItemDefinitionId)
FText::FromName(EffectId)
FText::FromName(MonsterId)
```

Ils servent à garder un état diagnostic exploitable lorsqu'une présentation
n'est pas disponible.

Ils ne réintroduisent pas de prose métier et restent hors migration, sauf si un
écran utilisateur final exige explicitement de ne jamais exposer un identifiant
technique.

---

## 11. Test de caractérisation

TEXT05.1 introduit :

```text
Grimrock.UI.RPG.CONTENTTEXT05.RemainingAuthorityAudit
```

Il vérifie/mesure :

### Skills

- 25 définitions canoniques ;
- chargement des 25 `DA_Skill_*` ;
- `SkillId` canonique ;
- `DisplayName` sérialisé non vide ;
- reprise actuelle du `DisplayName` sur duplicata transient ;
- conservation de `Description`.

### Spells

- quatre sorts de production ;
- validation structurelle ;
- quatre `DisplayName` ;
- quatre `Description`.

### Attack_Unarmed

- `ActionId = Attack_Unarmed` ;
- `DisplayName` C++ non vide ;
- `Description` C++ non vide.

### Source audit

Le test journalise également les writers C++ détectés et vérifie le chemin
Recipe basé sur des clés stables.

Les compteurs d'overwrite sont informatifs : les futures corrections doivent
pouvoir faire diminuer ces compteurs sans rendre l'audit rouge.

---

## 12. Ordre de travail recommandé après validation

### Prochain domaine : Skills

Le prochain chantier devrait cibler uniquement les 25 `DA_Skill_*` :

```text
préserver DisplayName sérialisé
-> supprimer les 25 libellés C++ joueur
-> bootstrap technique pour un Skill neuf
-> audit final
```

Aucune nouvelle architecture n'est nécessaire.

### Ensuite : Spells

Après les Skills, auditer/matérialiser les quatre
`UGridSpellDefinitionAsset` de production et faire consommer cette autorité
par le Spellbook/runtime.

### Enfin : Attack_Unarmed

Traiter séparément ce record universel, avec la solution la plus légère
possible.

---

## 13. Conclusion

Après CONTENT-TEXT01..04, les grandes autorités sérialisées RPG sont propres.

Les dettes restantes ne sont plus diffuses :

```text
P1  Skills          -> 25 DisplayName C++ en double des DA_Skill_*
P2  Spells          -> 4 noms + 4 descriptions encore uniquement C++
P3  Attack_Unarmed  -> 1 nom + 1 description C++ universels
```

TEXT05.1 ne modifie aucune de ces autorités ; il fournit la carte de travail
pour les prochains tickets.
