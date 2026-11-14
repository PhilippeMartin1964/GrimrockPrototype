# ARM-CLEAN06.3U — Force/poids : un seul contrat entre menu, contrôleur et lancer

**Prérequis :** ARM-CLEAN06.3T validé sur son SHA exact.

La règle de lancer physique et le facteur de vitesse lié à la Force sont résolus dans `FGridPhysicalThrowStrengthReadModel`, à partir du véritable `ItemId`/`ItemLevel` de chaque instance. Les ARM enregistrées utilisent seulement `DA_ItemSystem.WeightKg` pour le seuil de Force et le ratio de ralentissement (courbe historique strictement conservée : 0,25 kg/Force, `sqrt(Force/10)`, pénalité jusqu'à 35 %). L'admissibilité main unique vient de `HandPolicy` canonique. Une identité enregistrée invalide (N0) ne tombe pas sur une ancienne masse ou sur une copie `DA_Weapon_*`.

Les trois familles de lecteurs utilisent la même méthode :
- menu `UGridItemContextActionLibrary::BuildItemContextActions` : autorisation, raison d'indisponibilité et valeur dynamique `Interaction.Throw.TooHeavy` ;
- contrôleur `AGrimrockPlayerController` : préflight de la MainHand et du premier slot correspondant dans le sac ;
- Pawn `AGrimrockPartyPawn` : cinq chemins de lancer, contrôle avant extraction, coefficient `SpeedScale` canonique.

Les objets hors registre et les munitions sans niveau fixe restent régis par la compatibilité historique. Aucun changement d'AP, portée, cible, destruction, sauvegarde ni contenu binaire UE.

## Workflow de textes UI obligatoire

`docs/UI_WBP_LOCALIZATION_WORKFLOW.md` relu en amont et en sortie : **UI.csv inchangé → ST_UI inchangée → WBP inchangé → Game/fr/en inchangé**. Les chaînes `Interaction.Throw.TooHeavy`, `ThrowMissingDefinition`, `ThrowNeedsOneHand` et les autres motifs existaient déjà ; aucun texte statique ou clé n'est ajouté. Seule la valeur numérique dynamique insérée dans le motif localisable change de source. Aucun nouveau `SetText`, `TextBlock`, Graph UMG, WBP ou `.uasset` à matérialiser.

## Tests

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalPhysicalThrowStrengthGate` charge le vrai `DA_ItemSystem`, ARM054, ARM057, ARM058 N1/N20, masse historique contradictoire, permission/threshold, coefficient de vitesse, N0 invalide et item hors registre. `Grimrock` complet et compilation UE5.5.4 doivent réussir via `UE5.5.4/LocalAuto` sur le SHA publié.

Le rendu PIE des lancers, la vitesse physique et les messages UI demandent ultérieurement un contrôle GPU distinct.
