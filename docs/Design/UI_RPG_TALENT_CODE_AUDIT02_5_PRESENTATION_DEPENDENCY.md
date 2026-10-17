# UI-RPG-TALENT-CODE-AUDIT02.5 — Talent presentation dependency cleanup

Date : 17 octobre 2026  
Statut : **IMPLÉMENTÉ — EN ATTENTE DE VALIDATION LOCALE**

## Constat

Le catalogue unique `DA_RPGTalentPresentation` était correct comme autorité,
mais son identité runtime était recopiée à plusieurs endroits :

```text
GridSkillsPageService.cpp
    LoadObject<URPGTalentPresentationAsset>("/Game/.../DA_RPGTalentPresentation")

GridSkillsWidget.cpp
    TalentPresentationAssetPath = "/Game/.../DA_RPGTalentPresentation"
    LoadObject<URPGTalentPresentationAsset>(...)
```

Le même object path apparaissait également dans certains tests Editor et dans
l'authoring.

Cela créait une dépendance cachée : déplacer l'asset nécessitait de connaître
tous les consommateurs, et chaque consumer décidait lui-même comment le charger.

## Contrat retenu

L'identité runtime appartient désormais à la classe de l'asset :

```text
URPGTalentPresentationAsset::CanonicalObjectPath()
URPGTalentPresentationAsset::ResolveCanonical()
```

`ResolveCanonical()` :

1. charge le chemin canonique ;
2. vérifie l'existence de l'objet ;
3. vérifie `IsValidDefinition()` ;
4. retourne uniquement un catalogue utilisable.

## Consommateurs

Les deux consommateurs runtime deviennent :

```text
FGridSkillsPageService
    -> URPGTalentPresentationAsset::ResolveCanonical()

UGridSkillsWidget
    -> URPGTalentPresentationAsset::ResolveCanonical()
```

Ils ne possèdent plus de chemin d'asset et n'appellent plus directement
`LoadObject<URPGTalentPresentationAsset>`.

## Pourquoi pas une injection dans toutes les signatures

`FGridSkillsPageService::TryBuildCharacterView()` est utilisé par de nombreux
tests métier et de restauration qui construisent aussi des classes synthétiques.
Le catalogue de présentation n'est requis que pour les overrides conceptuels
des vrais nœuds à variantes de production ; les classes synthétiques disposent
déjà de leur fallback authoré.

Ajouter un paramètre `URPGTalentPresentationAsset*` à toutes les signatures
aurait propagé une dépendance de présentation à des consommateurs qui n'en ont
pas besoin, sans créer de nouvelle autorité. La centralisation du resolver est
donc la frontière la plus petite et la plus explicite.

## Authoring / tests

`FUIRPGTalentPresentationAuthoring::ObjectPath()` délègue au même
`CanonicalObjectPath()`. Son `PackageName()` reste editor-only car il est
nécessaire à la matérialisation de l'asset.

Les tests de production UI-RPG03 et DESC01 utilisent `ResolveCanonical()`
au lieu de recopier l'object path.

## Régressions 02.5

Le filtre `Grimrock.UI.RPG.CODEAUDIT02` ajoute :

```text
TalentPresentationDependency.CanonicalResolver
TalentPresentationDependency.RuntimeSourceBoundary
TalentPresentationDependency.PrivateUiPathUniqueness
```

Le dernier test scanne `Source/GrimrockPrototype/Private/UI/*.cpp` et impose
qu'un seul fichier possède le chemin runtime canonique :
`RPGTalentPresentationAsset.cpp`.

## Périmètre inchangé

Aucun changement de :

- contenu de `DA_RPGTalentPresentation` ;
- `.uasset` ;
- règles Talent ;
- progression / transaction ;
- SaveGame ;
- read-model fonctionnel ;
- ordre des branches ;
- quatre familles à variantes.
