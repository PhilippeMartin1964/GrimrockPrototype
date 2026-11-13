# ARM-CLEAN06.2D — Parité sémantique des classes, sans écriture binaire

Ce sous-jalon complète les contrôles 06.2A (delta Sword/RatGiant),
06.2B (variantes audio/Niagara) et 06.2C (82 archétypes,
1 640 profils, 260 passifs, zéro écart numérique).

La projection précédente RecommendedClassIds perdait le rang Primary /
Secondary et les qualificatifs du classeur. La nouvelle structure
canonique conserve par ARM une PrimaryClassId, la liste ordonnée
SecondaryClassIds et SecondarySourceLabel; les six préférences de classe
sont conservées globalement, avec leurs sept champs éditoriaux.

Le test Automation
Grimrock.RPG.ARM.CLEAN06_2.Semantics.ClassAffinityParity
charge le véritable DA_WeaponCatalogue dans Unreal, construit une
projection transitoire des 82 archétypes et 82 affinités, copie
les six préférences et compare chaque champ. Il refuse les
clés de classes invalides, les doublons et les pertes de primauté.

Les recommandations de classe sont informatives et n'empêchent
jamais d'équiper une arme.

Aucun package, .uasset, niveau, palette ou SaveGame n'est sauvegardé.
Les trois HandUsage et les familles doivent encore être mappés
explicitement; aucune décision de gameplay ne leur est attribuée
par ce test. Le travail restant sur les munitions, actions,
réactions, objets concrets et conversion de binaires demeure
obligatoire avant les étapes 06.3–06.6.

Le résultat n'est reconnu comme validé que si UE5.5.4/LocalAuto
est success sur le SHA exact. Aucune recette PIE n'est requise
pour cette vérification de structure.
