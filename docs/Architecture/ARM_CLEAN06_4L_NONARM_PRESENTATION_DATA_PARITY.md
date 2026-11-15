# ARM-CLEAN06.4L — Parité de données pour lumière et présentation persistante non-ARM

**Préconditions :** 06.4K validé sur `4d5e7eb8b1406d29eeb9b0805f82507aa8a1bfc7`, les trois statuts `SilentTask`, `LocalAuto` et `ARM-CLEAN06.4K` SUCCESS ; issue [144](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/144). Baseline protégée : 06.4G `e0f877d17f39d913c585b55ba18110745c5385b6`.

## Problème réellement vérifié dans 06.4K

Les 11 `ItemId` non-armes absents du registre correspondent à 40 occurrences dans les vrais niveaux, réceptacles et palettes. Sur 50 occurrences de propriétés historiques actives, deux familles concernent la **présentation non-armement** :

- `LIGHT` : **1** propriété active `LightEmitter`.
- `PERSISTENT_VISUAL` : **11** propriétés actives couvrant `VisualEffects` et `WorldSparkle*`.

L'ancienne classe `UGridItemDefinitionAsset` détient déjà les valeurs `FGridLightEmitterConfig`, `FGridItemVisualEffectsConfig` et les paramètres de scintillement. La classe canonique `UGridNonArmamentDefinitionAsset` ne possédait auparavant que `Icon` et `WorldMesh`. Il fallait une destination **non-ARM explicite**, au lieu de détourner `DA_WeaponVisualCatalogue` ou `DA_ActionPresentationCatalogue`, réservés aux armes et à leurs événements d'attaque.

## Modèle ajouté (sans conversion binaire)

`UGridNonArmamentDefinitionAsset` conserve `Icon` et `WorldMesh` et possède désormais les huit propriétés de présentation ci-dessous, dans la définition spécialisée **elle-même** :

| Champ source conservé à l'identique | Propriétaire cible unique |
|---|---|
| `LightEmitter` | `UGridNonArmamentDefinitionAsset.LightEmitter` |
| `VisualEffects` | `UGridNonArmamentDefinitionAsset.VisualEffects` |
| `bEnableWorldSparkle` | `UGridNonArmamentDefinitionAsset.bEnableWorldSparkle` |
| `WorldSparkleMaterial` | `UGridNonArmamentDefinitionAsset.WorldSparkleMaterial` |
| `WorldSparkleColor` | `UGridNonArmamentDefinitionAsset.WorldSparkleColor` |
| `WorldSparkleIntensity` | `UGridNonArmamentDefinitionAsset.WorldSparkleIntensity` |
| `WorldSparkleSpeed` | `UGridNonArmamentDefinitionAsset.WorldSparkleSpeed` |
| `WorldSparkleVariation` | `UGridNonArmamentDefinitionAsset.WorldSparkleVariation` |

Le noyau commun demeure **cinq champs** ; aucune propriété n'est ajoutée à la définition d'arme `UGridArmamentDefinitionAsset`. Les types `FGridLightEmitterConfig` et `FGridItemVisualEffectsConfig` sont réutilisés sans recréer d'autorité concurrente. L'ancienne source reste temporairement en place **uniquement le temps de la migration**, comme prévu par 06.4. Ni le runtime ni l'état initial des torches ne lisent encore ces nouvelles propriétés canoniques.

## Preuve ciblée dans le véritable UE Editor 5.5.4

Le commandlet Editor-only `GridArmClean064LNonArmPresentationParity` est déclenché uniquement par le sujet de commit :

`feat(ARM-CLEAN06.4L): preserve nonarm light and persistent visual data in transient canonical projection`

Après la compilation et l'Automation `Grimrock`, `Scripts/AuditARM_CLEAN06_4L_NonArmPresentationParity.ps1` :

1. vérifie les reçus privés de 06.4K sur leur SHA d'origine, les 11 identités, 40 références, 50 champs et 26 noms distincts ;
2. contrôle la répartition exacte des domaines et le verrou de conversion fermé ;
3. recontrôle les SHA-256 des **84 binaires Unreal originaux ET des 84 copies privées immuables** de 06.4G, sans lire les sauvegardes utilisateur ;
4. charge les vraies définitions historiques et le `DA_ItemSystem` (23 définitions) en lecture seule ;
5. crée **11 objets `UGridNonArmamentDefinitionAsset` sous `GetTransientPackage()`**, avec les identités réelles ; vérifie les 50 valeurs source actives à partir du rapport 06.4K ;
6. copie les **huit propriétés de présentation pour chacun des 11 objets**, y compris leurs valeurs C++ par défaut : **88 copies** réfléchies strictes, avec contrôle du type exact, `FProperty::Identical` et `ExportText` ;
7. compare aussi les prédicats purs `HasEmitter`, `RequiresRuntimeTick`, `IsValid` de la lumière et `IsValid` des effets persistants ;
8. contrôle l'absence de `Dirty` nouveau sur les packages sources et de toute écriture dans le worktree ;
9. conserve les identités et chemins de source uniquement dans `presentation-parity-private.json` sous `StateRoot/Runs`, avec reçu SHA-256 privé `presentation-gate-private.json` ; publie sur GitHub uniquement les nombres agrégés.

Le contexte supplémentaire `UE5.5.4/ARM-CLEAN06.4L` n'est marqué SUCCESS **que si la projection réelle UE et le reçu privé réussissent**. Il doit être vérifié, avec `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto`, sur le **SHA exact** du nouveau commit avant de déclarer 06.4L réussi.

## Portée exacte — ce que SUCCESS ne prouve PAS

- SUCCESS est une **parité de données en mémoire et de prédicats purs**, pas la preuve que `UGridLightEmitterComponent`, Niagara ou les matériaux fonctionnent à l'identique en PIE.
- Les **38 autres occurrences** actives restent à résoudre : lecture (1), physique/Chaos (2), lancer générique (3), action (11), équipement non-ARM (15), empilement (6).
- Les champs par défaut sont copiés et testés pour ne pas effacer les valeurs nécessaires au démarrage, mais il faut encore prouver la conservation de l'état mutable de lumière et des effets au cours de `Save/Continue`.
- Aucun `SavePackage`, pas de migration `.uasset` / `.umap` / `.sav`, pas de suppression des champs legacy et pas de changement de WBP ou des textes joueur.
- La porte `migration_authorized=false` reste fermée ; toutes les listes d'écriture sont vides. Le traitement des armes et de leurs catalogues de combat reste inchangé.

## Prochaine étape : 06.4M et suivantes

Traiter les autres champs selon **leur autorité métier effective**, en premier lieu les lectures/actions et les contrats physiques non-ARM, sur les vrais états UE. Prouver les chemins runtime : lumière/torche et scintillement, objet sur le sol, lancer, lecture et interaction, équipement, quantité, réceptacles, état mutable et Save/Continue. Une fois les données complètes et les recettes PIE/Chaos démontrées, préparer un cutover binaire Editor-only à liste fermée (backup, dry-run, rollback, sauvegardes compatibles), puis 06.5 et 06.6. Ne jamais inférer un `AmmoKindId` depuis le nom ou transférer le lancer non-ARM vers `WeaponCatalogue.ProjectilePolicies`.
