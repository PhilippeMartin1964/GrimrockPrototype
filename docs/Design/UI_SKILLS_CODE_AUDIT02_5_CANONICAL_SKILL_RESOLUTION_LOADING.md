# UI-SKILLS-CODE-AUDIT02.5 — Canonical Skill Resolution & Loading

Date : **17 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**  
État : **IMPLÉMENTÉ — validation locale requise**

## 1. Problème

Le service canonique est volontairement stateless :

~~~text
ResolveCanonicalSkillDefinitions()
    -> scan RPGSkill dans /Game
    -> récupération des PrimaryAssetId
    -> résolution / TryLoad
    -> validation
~~~

Ce comportement reste approprié pour les commandlets et audits ponctuels, mais
le widget l'appelait à répétition pendant une même session UI :

~~~text
RefreshSkills
SelectSkill
CommitSkillRankIncrease
CommitSkillRankDecrease
~~~

Les rebuilds de lignes faisaient également un LoadSynchronous de l'icône pour
chaque Skill à chaque reconstruction.

## 2. Cache par instance, non global

UGridSkillsWidget possède désormais un snapshot transitoire :

~~~text
CachedCanonicalSkillDefinitions
CachedCanonicalSkillDefinitionsById
~~~

Le premier besoin appelle le resolver existant une fois. Les URPGSkillAsset
résolus sont ensuite conservés par des références UObject fortes et transitoires
pendant la durée de vie du widget.

L'index SkillId permet aux transactions de résoudre directement la définition
sans rescanner AssetManager.

Ce cache est explicitement vidé lors de InitializeSkillsWidget et
NativeDestruct. Il n'est ni statique, ni global, ni sérialisé.

## 3. Catalogue de présentation

Dans RefreshSkills, la validité de SkillPresentationAsset est maintenant
calculée une fois avant la boucle des Skills, au lieu d'être recalculée pour
chaque entrée.

La règle ResolveClassAffinity ne change pas.

## 4. Cache des icônes Skill

Le widget conserve :

~~~text
CachedSkillIconTextures
CachedSkillIconPresentationAsset
~~~

Chaque icône est résolue comme suit :

~~~text
cache SkillId
    -> hit : réutiliser UTexture2D
    -> miss :
         Presentation.Icon.Get()
         puis LoadSynchronous() uniquement si nécessaire
         conserver le résultat
~~~

Si SkillPresentationAsset change, le cache d'icônes est vidé avant toute
nouvelle résolution.

GridSkillDetailWidget tente également Icon.Get() avant le fallback
LoadSynchronous. Dans le flux normal, la liste de Skills a déjà chargé et
conserve les textures, donc le détail réutilise l'objet chargé.

## 5. Autorité inchangée

FGridSkillsPageService::ResolveCanonicalSkillDefinitions reste inchangé et
public. Les commandlets, audits de production et autres consommateurs peuvent
toujours demander une résolution fraîche.

Le cache du widget n'est qu'une optimisation de présentation. Il ne devient
jamais l'autorité des Skills.

## 6. Tests

Nouveau contrat :

~~~text
Grimrock.UI.Skills02.Architecture.CanonicalSkillCacheContract
~~~

Il vérifie :

~~~text
caches UObject -> Transient
GridSkillsWidget.cpp -> un seul point de scan canonique
GridSkillsWidgetSkills.cpp -> aucun rescan
row rebuild -> cache d'icônes
LoadSynchronous row icon -> seulement dans le miss du cache
detail icon -> Get() avant fallback synchrone
~~~

La suite Grimrock.UI.Skills02 passe de 27 à **28 tests** si aucune autre suite
n'est ajoutée parallèlement.

## 7. Non-objectifs

02.5 ne modifie pas :

~~~text
découverte canonique des PrimaryDataAsset RPGSkill
contenu des 25 Skills
SkillPresentationAsset
affinités
tri
économie / allocation / undo
UMG
.uasset
SaveGame
chargement des icônes Talent
~~~

Aucun cache global AssetManager n'est introduit.

## 8. Validation locale

~~~powershell
cd D:\Development\GrimrockPrototype

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Skills02"
~~~

Le résultat attendu, si aucun autre test n'a été ajouté en parallèle, est :

~~~text
Succeeded : 28
Warnings  : 0
Failed    : 0
~~~

Ne pas déclarer UI-SKILLS-CODE-AUDIT02.5 clos avant le résultat local.
