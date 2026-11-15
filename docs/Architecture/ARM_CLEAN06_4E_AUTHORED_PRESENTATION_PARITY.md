# ARM-CLEAN06.4E — Parité des présentations réellement authorées

**Parent :** 06.4D `046d7850006476925710a06e50033db4da1c0d8e` ;
les trois statuts Windows SUCCESS, issue
https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/137.

06.4D a relevé huit divergences UPROPERTY sur quatre assets
historiques. Les huit noms de propriétés ont été orientés
**à titre indicatif** vers `DA_ActionPresentationCatalogue`.
Il ne suffit pas de constater une projection sans perte en mémoire :
**l'asset canonique persistant doit réellement contenir les
liaisons et valeurs attendues**, faute de quoi la migration
demeure interdite.

## Outils et périmètre

Un commandlet **Editor-only** nouveau,
`UGridArmClean064EPresentationParityCommandlet`, s'exécute
après la compilation UE5.5.4 et l'Automation `Grimrock`.
`Scripts/CheckARM_CLEAN06_4E_PresentationParity.ps1`
retrouve le rapport privé exact de 06.4D, recoupe son SHA,
son schéma, ses compteurs et ses trois listes d'écriture vides,
puis lance `UnrealEditor-Cmd.exe` avec
`-run=GridArmClean064EPresentationParity -NullRHI`.

Le commandlet :
1. exige **quatre sources distinctes**, vérifie que les huit
   UPROPERTY réellement chargées correspondent aux valeurs
   `ExportText` du rapport 06.4D et refuse tout autre type
   de destination candidate ;
2. charge `DA_ItemSystem` et son
   `UGridActionPresentationCatalogueAsset` vraiment sérialisés ;
3. pour chaque `FGridCombatActionDefinition` historique
   d'attaque, cherche la liaison `ActionId → PresentationId`
   dans le véritable archétype canonique (sans inventer d'ID) ;
4. applique le projecteur Editor existant
   `GridArmClean06PresentationProjection::TryProject`.
   Une présentation non activée reprend uniquement la valeur
   **neutre historique** `FGridPlayerAttackPresentationProfile()`,
   conformément à l'ancien runtime ;
5. compare **chaque propriété de**
   `FGridCanonicalActionPresentation` par réflexion Unreal :
   mouvement, délais, offsets, durée de feedback, variantes
   sonores ordonnées (volume/pitch) et Niagara (systèmes,
   socket, attache, transform), mais aussi les canaux canoniques
   supplémentaires (matières, parade, blocage, critique) ;
6. émet la preuve privée pour les égalités et des statuts
   explicites pour une valeur différente, une liaison
   absente, un profil manquant ou une source non résolue ;
7. vérifie que les packages sources et canonique n'ont pas
   changé d'état Dirty puis ne sauvegarde aucun package.

Le commandlet retourne 0 lorsque **l'audit est complet et
cohérent**, même si des liaisons ou valeurs ne sont pas
paritaires : ces écarts sont des **blocages métier** à
examiner, pas des erreurs de fonctionnement du comparateur.

## Rapport et sécurité

Les détails (packages/ActionId/PresentationId/champs réellement
différents) restent exclusivement dans le dossier privé de
l'agent Windows. L'issue `[ARM-CLEAN06.4E]` ne publie que
les nombres d'assets, UPROPERTY, liaisons identiques,
différentes et non prouvées.

Le statut additionnel `UE5.5.4/ARM-CLEAN06.4E` signifie
**rapport généré et validé**, absolument pas conversion autorisée.
Le reçu privé final conserve
`migration_authorized=false` et les **trois** listes
(`allowed_binary_writes`, `allowed_package_deletions`,
`allowed_savegame_writes`) vides.

## Critères et suite

Exiger `UE5.5.4/SilentTask=success`,
`UE5.5.4/LocalAuto=success`,
`UE5.5.4/ARM-CLEAN06.4E=success` **sur le SHA 06.4E exact**,
puis relever les compteurs réels de l'issue.

En **06.4F**, si les profils canoniques sont absents ou
différents, dresser le mapping ponctuel exact en UE Editor
et préparer les valeurs de destination dans des objets
transitoires, avec un mode dry-run testable ; aucun
`SavePackage` avant une preuve de parité complète et une
stratégie de sauvegarde et de rollback.

Aucun WBP ou texte joueur modifié. La chaîne
`UI.csv → ST_UI → WBP → Game/fr/en` est inchangée.
Le PIE/Chaos et le Cook restent distincts de `-NullRHI`.
