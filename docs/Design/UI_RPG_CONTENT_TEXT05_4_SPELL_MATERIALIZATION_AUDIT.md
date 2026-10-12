# UI-RPG-CONTENT-TEXT05.4 — Audit production Spell asset materialization and runtime ownership paths

Date : **12 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Ticket : **UI-RPG-CONTENT-TEXT05.4**

## 1. Objet

Auditer le bloc P2 identifié par TEXT05.1 avant toute création de DataAsset ou
modification du runtime.

Le ticket répond à quatre questions :

1. quels sorts de production existent réellement ;
2. où se trouve aujourd'hui leur autorité ;
3. si le modèle `UGridSpellDefinitionAsset` existant est suffisant ;
4. quel est le chemin minimal pour migrer sans créer une seconde architecture.

Aucun `.uasset`, `.umap` ou package n'est modifié.

## 2. État de production actuel

`FGridProductionSpellLibrary` construit exactement quatre sorts :

```text
Spell_ArcaneBolt
Spell_LesserHeal
Spell_Haste
Spell_CurePoison
```

Chaque définition contient directement en C++ :

```text
SpellId
DisplayName
Description
School
ManaCost
ActionPointCost
MinRangeCells
MaxRangeCells
TargetingPolicy
bRequiresLineOfSight
CooldownRounds
Effects[]
```

La présentation est elle aussi construite en C++ via :

```text
MakeArcaneBoltPresentation()
MakeLesserHealPresentation()
MakeHastePresentation()
MakeCurePoisonPresentation()
TryBuildPresentationProfile()
```

Il ne s'agit donc pas uniquement d'une dette texte : la bibliothèque C++ est
actuellement l'autorité complète des quatre définitions de production.

## 3. Texte joueur encore en C++

Les quatre `DisplayName` et les quatre `Description` sont toujours écrits
directement dans `GridProductionSpellLibrary.cpp`.

Le scan source caractérise actuellement :

```text
CxxDisplayNames  = 4
CxxDescriptions = 4
```

Ce sont les huit chaînes joueur déjà identifiées par TEXT05.1.

## 4. DataAsset cible déjà existant

Le projet possède déjà :

```cpp
UGridSpellDefinitionAsset : UPrimaryDataAsset
{
    FGridSpellDefinition Definition;
    FGridSpellPresentationProfile Presentation;
}
```

TEXT05.4 instancie quatre assets transients, copie pour chacun la définition et
le profil fournis par la bibliothèque actuelle puis vérifie :

```text
ValidateDefinition() == None
HasValidPresentation() == true
```

Résultat attendu : les quatre sorts tiennent dans ce conteneur **sans changer
le modèle de données**.

Il n'est donc pas justifié d'introduire un second type d'asset, un nouveau
catalogue de données ou une nouvelle hiérarchie de sorts.

## 5. Absence actuelle de matérialisation

Le dépôt ne contient actuellement aucun :

```text
DA_Spell_*.uasset
```

Le test compte ces fichiers de manière informative afin de rester compatible
avec leur future création.

L'audit Git confirme également qu'aucun chemin de production
`DataAssets/Magic/Spells` ou équivalent n'a encore été défini.

## 6. Identité Primary Asset incomplète

`UGridSpellDefinitionAsset` dérive de `UPrimaryDataAsset`, mais contrairement
à `URPGSkillAsset`, il ne redéfinit pas encore explicitement :

```cpp
GetPrimaryAssetId() const
```

Le pattern Skills déjà validé est :

```text
RPGSkill:<SkillId>
```

Pour les sorts, la migration devra définir une identité stable équivalente,
basée sur `SpellId`, afin de ne pas dépendre du nom physique du fichier.

TEXT05.4 ne choisit pas encore le nom final du `FPrimaryAssetType`, mais
verrouille ce manque avant matérialisation.

## 7. Consommateurs runtime actuels

La façade `FGridProductionSpellLibrary` est déjà utilisée partout comme
catalogue canonique.

Les appels `BuildAll()` de production se trouvent dans :

```text
GridSpellbookUI.cpp
    - validation d'un sort connu
    - construction des lignes Spellbook

GridSpellbookPersistence.cpp
    - validation des SpellId persistés

GridPartySpellbookComponent.cpp
    - validation LearnSpell

GridTurnManagerPlayerActionCatalog.cpp
    - résolution d'un SpellId au moment d'exécuter/caster
    - reconstruction des actions Spell connues pour le catalogue de combat

GridSpellbookWidget.cpp
    - commande de développement SeedProduction
```

Le code courant représente **sept appels runtime `BuildAll()`** répartis dans
ces cinq fichiers.

`GridTurnManagerPlayerActionCatalog.cpp` appelle aussi :

```cpp
FGridProductionSpellLibrary::TryBuildPresentationProfile()
```

pour la présentation MON18.6.

Aucun de ces consommateurs ne charge actuellement
`UGridSpellDefinitionAsset`.

## 8. Conséquence architecturale

Le chemin minimal n'est pas :

```text
ajouter un nouveau SpellCatalog
+ garder FGridProductionSpellLibrary
+ faire migrer chaque consommateur
```

Cela créerait une nouvelle couche et deux autorités.

Le chemin minimal recommandé est :

```text
FGridProductionSpellLibrary
    aujourd'hui : construit le contenu C++
    demain      : résout les UGridSpellDefinitionAsset de production
```

Ainsi, les consommateurs existants peuvent continuer à utiliser :

```text
BuildAll()
TryBuildPresentationProfile()
SpellId helpers
```

pendant la migration.

La façade change de source de vérité, pas le reste du runtime.

## 9. Séquence de migration recommandée

### TEXT05.5 — identité + authoring de matérialisation

Ajouter le minimum nécessaire pour matérialiser proprement les quatre assets :

```text
UGridSpellDefinitionAsset
    -> PrimaryAssetId stable basé sur SpellId

Editor-only Spell authoring
    -> crée/met à jour DA_Spell_<suffixe>
    -> copie mécanique + présentation existantes
    -> préserve ensuite texte/presentation authored lors des reruns
```

Ce ticket ne doit pas encore retirer la bibliothèque C++ de secours tant que
les quatre assets n'existent pas réellement.

### Étape locale UE obligatoire

Les `.uasset` doivent être créés par Unreal Engine, via un authoring
Editor/commandlet contrôlé. Ils ne doivent pas être fabriqués ou modifiés
directement hors UE.

### TEXT05.6 — bascule runtime

Une fois les quatre assets matérialisés et validés :

```text
BuildAll()
    -> UGridSpellDefinitionAsset

TryBuildPresentationProfile()
    -> UGridSpellDefinitionAsset.Presentation
```

Les quatre noms/descriptions C++ peuvent alors disparaître avec les builders
de contenu devenus inutiles, ou être réduits à un bootstrap technique
Editor-only si un besoin réel subsiste.

### TEXT05.7 — audit final Spells

Vérifier :

- quatre assets sérialisés ;
- aucune prose Spell joueur dans la bibliothèque runtime ;
- Spellbook, persistence, LearnSpell, TurnManager et présentation utilisent la
  même autorité ;
- aucun fallback ne recrée silencieusement les anciens noms/descriptions.

## 10. Fallback Spellbook existant

`GridSpellbookUI.cpp` contient encore un fallback utilisateur lorsque la
définition est introuvable :

```text
DisplayName = SpellId
Description = "Définition de sort indisponible."
```

Il s'agit d'un fallback d'erreur UI, pas de l'autorité d'un sort particulier.

Il pourra rester tant qu'il est explicitement traité comme état dégradé et non
comme source de présentation normale.

## 11. Tests

TEXT05.4 ajoute :

```text
Grimrock.UI.RPG.CONTENTTEXT05.SpellMaterializationAudit
```

Il vérifie :

- quatre SpellId de production uniques ;
- quatre définitions structurellement valides ;
- quatre noms/descriptions non vides ;
- compatibilité des quatre définitions avec `UGridSpellDefinitionAsset` ;
- compatibilité des quatre profils MON18.6 avec ce même asset ;
- présence des quatre SpellId attendus.

Il journalise en outre :

```text
[UI-RPG-CONTENT-TEXT05.4][MATERIALIZATION]
[UI-RPG-CONTENT-TEXT05.4][OWNERSHIP]
[UI-RPG-CONTENT-TEXT05.4][SUMMARY]
```

avec :

- nombre de `DA_Spell_*.uasset` présents ;
- présence/absence d'un override `PrimaryAssetId` ;
- nombre de writers C++ de nom/description ;
- nombre de consommateurs runtime de `BuildAll()` ;
- présence d'un consommateur direct de DataAsset.

## 12. Conclusion

Le problème P2 est maintenant précisément borné :

```text
4 sorts
8 chaînes joueur C++
1 bibliothèque de production
1 type DataAsset déjà suffisant
7 appels runtime BuildAll()
1 appel runtime de profil de présentation
0 DA_Spell_* matérialisé actuellement
```

La migration peut rester simple :

```text
matérialiser l'asset existant
-> faire de FGridProductionSpellLibrary une façade de résolution
-> supprimer l'autorité contenu C++
```

Aucune nouvelle architecture n'est nécessaire.
