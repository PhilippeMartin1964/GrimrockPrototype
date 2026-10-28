# ARMEMENT-LOT02.2 — Validation transactionnelle des cinq kits candidats

**Statut : code publié sur `master`, validation locale UE5.5.4 et tests de non-régression encore à confirmer.**

## Objectif

Vérifier les cinq kits `CANDIDATE_FOR_PIE` déclarés par le rapport
`Saved/ARMEMENT-LOT02/LOT02_ApprovedKitsReadiness.csv` :

- `Kit_Warrior_Guardian` — `ARM014` MainHand + `ARM075` OffHand ;
- `Kit_Warrior_Breaker` — `ARM031` MainHand (deux mains) ;
- `Kit_Warrior_WeaponMaster` — `ARM015` MainHand + `ARM025` Bag ;
- `Kit_Rogue_Shadow` — `ARM001` MainHand + `ARM013` OffHand ;
- `Kit_Priest_Protection` — `ARM020` MainHand + `ARM075` OffHand.

Les compositions proviennent exclusivement du CSV de conception approuvé.
Les définitions d'armes proviennent du manifeste industriel et des DataAssets
réellement importés. Aucun nombre de dégâts ou bonus n'est redéfini.

## Test automatisé

Filtre : `Grimrock.RPG.ARMEMENT.LOT02.NewGameFiveCandidateKits`.

Le test lit chaque recette approuvée, duplique la classe de production dans
un objet transitoire, y ajoute seulement le kit testé, puis invoque le
véritable `UGridPartyInventoryComponent::CreateInitialCharacter`.
Il contrôle la sélection, les niveaux N1, le placement MainHand/OffHand/Bag,
la non-duplication, l'identité des définitions enregistrées et les invariants
d'ownership. Il crée une copie en mémoire de `UGrimrockPartySaveGame`, puis
restaure cet état dans un nouveau composant pour comparer IDs et GUID.

**Limites strictes :** ce contrôle de snapshot en mémoire n'est **pas** un
test de sérialisation binaire sur disque, de rechargement après redémarrage,
de combat ou d'interaction souris en PIE.

## Isolation et sécurité

- Ne modifie aucun `DA_Class_*`, aucune palette, aucun WBP, ni le
  `FRPGStartingEquipmentPresetCatalog` historique.
- Les **12 kits legacy** restent seuls affichés dans l'assistant de création.
- Les 18 nouvelles recettes restent `ApprovedDesign`, non activées.
- Aucun talent de niveau 2 n'est accordé et aucune sauvegarde réelle n'est écrite.
- La modification locale indépendante de `L_MainMenu.umap` est exclue.

## Validation locale et publication

Fermer l'éditeur, puis :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT.LOT02"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.ARMEMENT01.9"
```

Attendu : deux tests LOT02 (readiness + création), aucune erreur ;
la suite `ARMEMENT01.9` doit préserver ses non-régressions historiques.
La publication du code de test ne remplace pas sa validation UE locale : si une régression apparaît, corriger dans un nouveau commit atomique sur `master`.

## Ensuite

Compléter les dépendances des 13 kits bloqués, puis développer et tester
la migration des 18 recettes approuvées vers le wizard **en une bascule
cohérente**, tout en conservant la lecture des `StartingEquipmentKitId`
historiques dans les sauvegardes existantes.
