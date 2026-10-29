# ARMEMENT-LOT02.15 — Activation technique et état des classes

**29 octobre 2026 — publication GitHub, validation locale UE/PIE restant à obtenir.**

## Activation des armes

Les onze archétypes ARM038, ARM048, ARM054, ARM057, ARM058, ARM061, ARM062, ARM067, ARM069, ARM070 et ARM081 passent de `Planned` à `Active` dans l'unique `Config/Armement/WeaponProductionManifest.csv`. Le manifeste compte donc **22 armes Active**. Les tests génériques LOT02 de production et de cycle monde/sac/monde vérifient maintenant cette valeur.

L'utilisateur a déjà observé que les onze nouvelles armes sont **visibles en éditeur et en PIE, ramassables et équipables** : ces points ne sont pas remis en attente. **Le combat et Save/Continue sur disque ne sont pas déclarés validés** par cette observation ; il faudra un test par famille.

Cette activation est **déclarative**. Aucun .uasset, palette, niveau, escalier ou classe n'est modifié par le commit. En particulier, `ProductionState=Active` **n'active aucun kit de personnage dans le wizard**.

## Classes : existant vs conception

- Six classes de production : **Warrior, Rogue, Ranger, Mage, Priest, Alchemist** (six `DA_Class_*`).
- Trois branches de talent réelles par classe, soit **18 orientations** ; premiers choix de talents au niveau 2, aucun talent implicite au niveau 1.
- Le système courant `FRPGStartingEquipmentPresetCatalog` injecte **12 kits historiques**, deux par classe, dans une copie transitoire. Les identités SaveGame existantes sont conservées.
- **18 recettes de kits N1 approuvées** dans `docs/Design/ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv` : trois par classe, **non encore distribuées** dans le wizard.
- `Grimrock.RPG.ARMEMENT.LOT02.NewGameFiveCandidateKits` vérifie cinq recettes sur classe transitoire et un snapshot SaveGame en mémoire ; cela ne vaut pas activation des 18 ni test de sérialisation après redémarrage.
- Cinq compléments non-armement restent à définir : outil de désamorçage, catalyseur de surfaces, soin, potion/composants de l'Apothicaire, composants de transmutation. Les kits Rôdeur/Tireur et Éclaireur doivent recevoir une dotation en vraies flèches avant une activation fiable ; torche et bombe prévues par les recettes doivent être distribuées elles aussi.
- Étape ultérieure : **bascule groupée des 18 kits dans l'assistant de création** via le service transactionnel existant, les traductions UI et une politique explicite de compatibilité SaveGame. Aucun second système de classes.

## Prévalidation locale — pipeline unique

L'assertion `Industrial05.ActiveN1ToN20TooltipAndNarrative` exige que chaque objet Active possède **une Description non vide**. Les onze précédents `Planned` peuvent ne pas encore disposer du texte narratif approuvé du XLSX. Vérifier le rapport narratif et, uniquement si l'import est prêt et sans conflit, utiliser le pipeline existant AVANT de lancer la validation Industrial : ne pas supprimer le garde-fou narratif.

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
.\Scripts\WeaponPipeline.ps1 -EngineRoot D:\UE_5.5 -Action Narrative -All
# Vérifier Saved/ARMEMENT-INDUS/INDUS05_Narrative_All.csv avant écriture.
.\Scripts\WeaponPipeline.ps1 -EngineRoot D:\UE_5.5 -Action Narrative -All -Apply
.\Scripts\WeaponPipeline.ps1 -EngineRoot D:\UE_5.5 -Action Validate -All
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.ARMEMENT.LOT02"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.ARMEMENT.Industrial"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock"
git status --short
```

Si `Narrative -All` rapporte des conflits ou une source non approuvée, **ne pas lancer `-Apply`** : relever les résultats et corriger la cause, sans écraser les DataAssets modifiés. Tout asset narratif modifié localement doit être validé dans UE, puis commité séparément en excluant `DA_Stairs_Up` et `DA_Stairs_Down`.

## Recette PIE fonctionnelle restante, par famille

- ARM038 : main secondaire réservée, portée, PA et attaque réelle.
- ARM048 : 2 flèches → 1 → 0, refus sans munition et hors portée sans perte de PA.
- ARM054/057/058 : jets depuis main/sac, collision, récupération unique.
- ARM061/062/067/069 : action magique N1, portée, coût en PA ; aucun effet spécial non implémenté.
- ARM070 : bonus de protection magique et consommation sur dommage, sans attaque.
- ARM081 : protection physique, absorption et recharge sans double bonus.
- **Save → Quitter → Continue** dans une sauvegarde de test : équipement, quantité, niveau N1 et GUID préservés ; au moins une flèche au sol et une flèche fichée, sans duplication.

Les anciens rapports des jalons LOT02.13 et LOT02.14 sont historiques ; leur mention de `Planned` ou du couteau legacy ne réouvre pas les tâches clôturées.
