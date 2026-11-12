# ARM-CLEAN03 — Progression N1–N20 compacte et sans perte

**12 novembre 2026 — code de lecture/conversion et tests publiés, migration binaire non exécutée.**
**Autorité éditoriale inchangée :** le classeur ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_ARMEMENT01_2.xlsx.
**Autorité runtime :** DA_WeaponCatalogue. Les représentations anciennes et compactes sont mutuellement exclusives.

## Contrat

FGridWeaponArchetypeProgression contient un unique FGridWeaponLevelProfile pour N1 : identité, nom, famille, type de dégâts, scaling, précision, PA, portée, poids et passivité restent invariants.

Cinq données changent avec le niveau et possèdent chacune une FGridWeaponIntegerGrowthRule : DiceCount, DiceSides, FlatBonus, PhysicalArmor, MagicalArmor.

Formule :

    Valeur(L) = ValeurN1 + floor((L-1)/StepEveryLevels) * StepAmount
    Si une exception existe au niveau L, utiliser sa valeur absolue.

Le générateur teste les intervalles de 1 à 20 et toutes les pentes déductibles de valeurs existantes ; il conserve la progression nécessitant le moins d'exceptions (départage déterministe). Tous les 1 640 profils doivent être reconstruits avec parité exacte. Les 260 entrées passives restent sans attaque et les dés offensifs restent NdM+K.

L'asset compact ne sérialise plus les 1 640 profils ; un cache transitoire les reconstitue au runtime, sans modifier l'API FindProfile. Les modifications de propriétés dans l'éditeur invalident ce cache.

## Audit du classeur (sans ecriture d'asset)

    cd D:\Development\GrimrockPrototype
    .\Scripts\AnalyzeARMWeaponProgression.ps1 -ExportFirst

Rapport CSV : Saved/ARMEMENT01/CLEAN03_GrowthRules.csv (410 règles avec exceptions).
Ce script doit refuser toute clé ou niveau incohérent et toute perte de parité.

## Conversion contrôlée (ne pas lancer avant les tests)

1. Compiler le module Unreal Editor et exécuter les suites de tests ci-dessous.
2. Fermer Unreal Editor ; vérifier git status --short et la branche master.
3. Dry-run sans sauvegarde :

    .\Scripts\CompactARMWeaponCatalogue.ps1 -EngineRoot D:\UE_5.5

Exiger dans les logs PARITY_OK Profiles=1640 Archetypes=82 Rules=410. Aucun asset n'est modifié.
4. Après validation seulement, appliquer :

    .\Scripts\CompactARMWeaponCatalogue.ps1 -EngineRoot D:\UE_5.5 -Apply

Le commandlet compare chaque propriété des 1 640 profils avant SavePackage. ClassAffinities et ClassPreferences sont conservés. Ne committer le fichier .uasset que lorsque la validation globale UE et le PIE ont réussi.

## Suites automatisées

    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT.CLEAN03"
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT.CLEAN02"
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT.Industrial"
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock"

Le test LosslessCompactCatalogue compare les 1 640 profils et vérifie le refus d'exceptions de niveau dupliquées.

## Migration automatique isolée — ARM-CLEAN03.3

Une migration **unique** peut désormais être autorisée en publiant sur `master` le
commit portant exactement le titre
`feat(ARM-CLEAN03.3): enable one-shot guarded catalogue migration`.
Ce commit regroupe le helper `Scripts/PublishARMCompactCatalogue.ps1` et son
raccordement à `Scripts/UEValidationAgent.ps1`.

Le dispositif VALIDATION-AUTO01 doit d'abord obtenir `UE5.5.4/LocalAuto=success`
sur ce SHA. L'agent exécute alors le commandlet natif
`GridWeaponCatalogueCompact -Apply` **dans son worktree détaché**, sans modifier
le checkout de l'utilisateur ni sa configuration globale d'UnrealBuildTool.

Garde-fous supplémentaires : origine GitHub vérifiée, SHA exact, déclencheur
uniquement pour ce commit, worktree détaché initialement propre, cible unique
`DA_WeaponCatalogue.uasset` sous Git LFS, fichier réel (pas pointeur LFS),
taille strictement réduite, aucune autre modification, et **suite complète
`Grimrock` sur le `.uasset` sauvegardé AVANT tout commit ou push**.
Un `git fetch` final refuse toute divergence de `origin/master` et aucune
publication forcée n'est autorisée.

Après succès, l'agent publie exactement un commit d'asset
`assets(ARM-CLEAN03): migrate weapon catalogue to compact lossless rules`,
puis le cycle Windows suivant compile et valide ce SHA à son tour.
Le statut dédié `UE5.5.4/WeaponCompact` porte l'état de cette opération sur
le commit déclencheur. Le code `UE5.5.4/LocalAuto` représente séparément la
compilation et la suite automatisée initiale.

En cas d'échec, aucun asset n'est poussé ; une modification unique du catalogue
dans le worktree isolé est restaurée à partir du SHA vérifié. Une divergence
ou des changements inattendus sont conservés pour diagnostic au lieu d'être effacés.

L'approbation PIE porte sur la finition visuelle des profils N1/N20, les sons
et les interactions ; la migration des données numériques n'est jamais utilisée
pour inventer ou reparamétrer les armes.

## Retour arrière

Un réimport du XLSX par GridWeaponCatalogueImport recrée le tableau complet Profiles, efface ArchetypeProgressions et invalide le cache. L'identité et les niveaux des instances demeurent inchangés. Ne supprimer aucune propriété sérialisée des items lors de cette migration.

## Limites

- Le gain réel de taille disque doit être mesuré après conversion ; 82 bases et 410 règles ne sont pas nécessairement plus petits une fois compressés par Unreal.
- Les DataAssets d'item conservent temporairement leur CombatAction N1 comme template de métadonnées, non comme autorité des dégâts.
- La spécification audio/Niagara ARMEMENT-FX01 est indépendante : cette migration ne crée aucun média.
- La publication du C++ ne vaut pas validation d'une compilation ni d'un test PIE local.
