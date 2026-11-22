# ARM-CLEAN06.5 — Retrait des contrôles d'industrialisation branchés sur les anciens catalogues

## Contrat testé sur la donnée installée

`GridEditorARMEMENTINDUS02PipelineTests.cpp`, `GridEditorARMEMENTINDUS05MatrixTests.cpp`
et `GridEditorARMCLEAN062VProductionAmmoTests.cpp` n'exigent plus
`DA_WeaponCatalogue`, `DA_WeaponVisualCatalogue`, les `DA_Weapon_*`
ou `DA_Item_Arrows_01` pour leur validation.

Les noms des huit suites Automation sont conservés. `UGridItemSubsystem`
résout les 22 ARM N1 et N20 contre le manifest historique des identités
(ce CSV n'est plus une autorité de gameplay). La matrice INDUS05 résout
les 22 ARM à 20 niveaux, soit 440 profils canoniques, construit le tooltip
existant avec un pointeur de définition nul, vérifie les catégories passives
et préserve son rapport CSV. Le plan de palette garde ses tests de conflits,
N0 invalide, doublons et idempotence dans des copies transitoires.
Le test des deux arcs vérifie une seule identité de flèches N0 et les liens
stricts `AmmoKindId`.

## Conservation de la preuve binaire

Le test historiquement nommé `LegacyThrowingItemsPreserved` vérifie
Shuriken N0, ThrowingKnife ARM054 N1 et la recette Assassin canoniques.
Il maintient un inventaire AssetRegistry en lecture seule pour l'ancien
ThrowingKnife/redirector et son artwork. Son résultat n'autorise aucune
suppression. Les anciens outils d'authoring et dernières suites de
prévisualisation restent à traiter.

Aucun fichier .uasset ni .sav n'est modifié. La suppression physique des
53 candidats reste interdite jusqu'au contre-audit natif sur SHA final,
preuve 7/7 sauvegardes, PIE, Cook et Shipping.
