# UI-RPG-CONTENT-TEXT05.5 — Stable Spell DataAsset identity and safe production materialization authoring

Date : **12 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Ticket : **UI-RPG-CONTENT-TEXT05.5**

> **Correctif de liaison Editor :** les structures publiques `FGridPlayerAttackAudioDefinition`, `FGridPlayerAttackVFXDefinition` et `FGridPlayerAttackPresentationProfile` exportent désormais leurs méthodes out-of-line via `GRIMROCKPROTOTYPE_API`. Ce correctif ne change aucun comportement runtime ; il permet au module Editor de valider les profils Spell sans symbole DLL non résolu.

## 1. Objet

Préparer la matérialisation des quatre sorts MON18.5 dans le type existant UGridSpellDefinitionAsset, sans encore basculer le runtime et sans fabriquer de .uasset hors Unreal Engine.

TEXT05.5 ajoute :

- une identité Primary Asset stable basée sur SpellId ;
- un authoring Editor-only réutilisant la bibliothèque de production actuelle ;
- un commandlet Unreal de matérialisation ;
- un contrat de préservation pour les reruns ;
- des tests transients sans sauvegarde de package.

Le commit TEXT05.5 ne contient aucun .uasset.

## 2. Identité stable

UGridSpellDefinitionAsset redéfinit GetPrimaryAssetId(). Le contrat est :

~~~text
Definition.SpellId != None -> GridSpell:<SpellId>
Definition.SpellId == None -> comportement UPrimaryDataAsset par défaut
~~~

Exemples : GridSpell:Spell_ArcaneBolt, GridSpell:Spell_LesserHeal, GridSpell:Spell_Haste et GridSpell:Spell_CurePoison.

## 3. Dossier de production

L’authoring cible /Game/GrimrockPrototype/Core/DataAssets/Magic/Spells avec DA_Spell_ArcaneBolt, DA_Spell_LesserHeal, DA_Spell_Haste et DA_Spell_CurePoison.

Le nom physique reste lisible, mais l’identité gameplay est SpellId.

## 4. Source de bootstrap

TEXT05.5 ne crée pas une nouvelle autorité métier. FGridSpellCatalogAuthoring::BuildProductionSeeds() dérive ses seeds directement de FGridProductionSpellLibrary::BuildAll() et TryBuildPresentationProfile().

Avant la bascule runtime :

~~~text
FGridProductionSpellLibrary = autorité actuelle
FGridSpellCatalogAuthoring  = outil Editor de matérialisation seulement
~~~

Il n’existe aucune seconde table de coûts, effets, noms ou descriptions.

## 5. Premier passage

Pour un asset neuf, l’authoring copie Definition et Presentation depuis la bibliothèque C++, valide le résultat puis sauvegarde via Unreal. Le premier passage reproduit donc exactement le comportement production actuel.

## 6. Rerun sûr

Pour un asset existant dont Asset.Definition.SpellId == Seed.Definition.SpellId, l’authoring snapshot DisplayName, Description et Presentation, reconstruit la mécanique depuis le seed canonique, restaure exactement ces champs authored puis valide le résultat.

Le rerun ne reprend donc pas silencieusement l’autorité sur la présentation.

## 7. Ce qui reste mécanique

IsCanonicalMechanics() compare uniquement SpellId, School, ManaCost, ActionPointCost, MinRangeCells, MaxRangeCells, TargetingPolicy, bRequiresLineOfSight, CooldownRounds et Effects[].

DisplayName, Description et Presentation sont volontairement exclus.

## 8. Protection entre identités

Si un asset possède un autre SpellId, son texte et sa présentation ne sont pas réutilisés pour le seed demandé. Cela évite de transporter la présentation d’un sort vers un autre.

## 9. Échec plutôt qu’écrasement

Un même SpellId avec DisplayName authored vide est préservé puis rejeté par la validation MissingDisplayName. L’authoring n’écrase pas silencieusement le texte par la valeur C++.

## 10. Commandlet

TEXT05.5 ajoute UGridSpellCatalogAuthoringCommandlet. Invocation cible :

~~~powershell
D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe `
    D:\Development\GrimrockPrototype\GrimrockPrototype.uproject `
    -run=GridSpellCatalogAuthoring `
    -Unattended -NoSplash -NoP4 -NoSound -NullRHI -log
~~~

Le commandlet appelle FGridSpellCatalogAuthoring::AuthorProductionAssets(), qui utilise CreatePackage, NewObject<UGridSpellDefinitionAsset>, FAssetRegistryModule::AssetCreated et UPackage::SavePackage.

Les fichiers binaires ne doivent jamais être fabriqués ou modifiés par un outil externe.

## 11. Atomicité Git

TEXT05.5 versionne uniquement l’infrastructure C++/tests/docs. La création effective des quatre .uasset sera effectuée localement par Unreal dans un ticket de matérialisation distinct avant TEXT05.6.

## 12. Tests

TEXT05.5 ajoute Grimrock.UI.RPG.CONTENTTEXT05.SpellMaterializationAuthoring.

Le test vérifie quatre seeds, l’identité GridSpell:<SpellId>, l’indépendance vis-à-vis du nom UObject, le bootstrap d’un asset neuf, la reconstruction mécanique, la préservation de DisplayName/Description/Presentation, l’absence de contamination entre SpellId et l’échec explicite sur un DisplayName authored invalide.

## 13. Séquence suivante

~~~text
TEXT05.5A
    matérialiser les 4 DA_Spell_* via le commandlet Unreal
    inspecter git status
    valider les assets sous UE
    commit binaire atomique

TEXT05.6
    faire résoudre FGridProductionSpellLibrary depuis DA_Spell_*
    conserver la façade BuildAll/TryBuildPresentationProfile
    retirer ensuite l’autorité contenu C++ devenue inutile
~~~

## 14. Conclusion

TEXT05.5 prépare une migration sûre sans nouvelle architecture. Aucun consommateur runtime n’est encore modifié à ce stade.