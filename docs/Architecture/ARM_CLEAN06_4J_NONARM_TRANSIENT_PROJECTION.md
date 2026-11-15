# ARM-CLEAN06.4J — Simulation des 11 objets non-armes sans écriture

**Référence vérifiée :** ARM-CLEAN06.4I, SHA
`d3fc4e10de5b82e5669c4dd03c309c30abeab9c6`,
statuts `SilentTask`, `LocalAuto` et `ARM-CLEAN06.4I` tous
SUCCESS, rapport https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/142.

## Base factuelle

L'inventaire Unreal actuel a relevé 95 références physiques :
55 occurrences représentent les 23 `ItemId` déjà présents dans le
registre canonique, tandis que 40 occurrences concernent **11
`ItemId` non-armement distincts**. Aucun conflit de propriétés
entre vieux assets de même identité ; aucun `ItemLevel` authoré
entre N1 et N20 (tous les placements examinés restent en N0).

**Ce que l'on ne sait pas encore :** l'authoring canonique complet
des onze objets, leurs interactions de puzzle, règles de torche,
lancers, usages et états SaveGame. Le présent jalon ne remplace
pas ces preuves par une affectation d'après le nom de l'objet.

## 06.4J : simulation par objets UE transitoires

Le script `Scripts/AuditARM_CLEAN06_4J_NonArmTransient.ps1`
retrouve le rapport **privé exact** de 06.4I, en vérifie le
SHA-256 par le reçu `identity-gate-private.json`, l'unicité
des 11 identités et l'intégralité des 40 occurrences.

Le script relit et hache également les **84 sources binaires**
ainsi que les **84 sauvegardes privées** de 06.4G.
Aucune commande de restauration dans le projet ni de
migration des SaveGames n'est disponible.

Le commandlet `UGridArmClean064JNonArmProjectionCommandlet`
s'exécute dans le véritable UE5.5.4 après Build et
Automation `Grimrock` réussis. Pour les 11 définitions
réelles, il :
1. recharge le `UGridItemDefinitionAsset` historique exact,
   confronte **toutes** ses `FProperty` `ExportText` à
   l'état privé immuable de 06.4I ; toute dérive arrête l'audit ;
2. vérifie que l'`ItemId` demeure absent du vrai
   `DA_ItemSystem` (23 définitions canoniques uniques) ;
3. classe les objets en famille **candidate** à partir de
   l'`EGridItemType` et des autorisations d'action explicites :
   consommable, équipement hors ARM, non-armement générique
   à revoir, ou famille non classable ;
4. n'infère **jamais** `UGridAmmoDefinitionAsset` ou
   `AmmoKindId` d'un nom, d'une icône ou d'un tag ;
5. crée **en mémoire seulement** une instance `RF_Transient`
   de la classe candidate, recopie puis compare
   `ItemId`, `DisplayName`, `Description`,
   `WeightKg`, `Tags`, `Icon` et `WorldMesh`.
   Pour les consommables, prévisualise l'empilement ;
   pour les équipements, les slots et bonus propres ;
6. classe les propriétés anciennes **non par défaut**
   qui n'ont pas encore de destination canonique prouvée,
   sans les supprimer ni leur inventer un propriétaire ;
7. conserve sur Windows le profil source, la projection
   transitoire complète, les propriétés encore sans mapping
   et la liste de contrôles par identité.

La validation des champs communs **n'est pas une parité
complète**. Même si les onze projections réussissent,
chaque identité garde des verrous séparés pour la physique
du lancer, Chaos, usages réels, réception dans le monde,
conservation du N0, contenus des réceptacles et schéma
SaveGame. Le cas torche/lumière, lecture et actions rapides
exige une correspondance sémantique explicite, même si la
propriété historique vaut son défaut C++.

## Sorties et contrat de confidentialité

- Rapport privé `nonarm-projection-private.json` :
  les 11 `ItemId` et chemins exacts, projections par champ,
  familles candidates et points d'authoring encore bloquants.
- Reçu privé `nonarm-transaction-gate-private.json` :
  SHA source, SHA du rapport, compteurs contrôlés et
  `migration_authorized=false`.
- Issue GitHub `[ARM-CLEAN06.4J]` : **seulement des nombres
  agrégés**, aucune identité, nom de DataAsset, valeur
  `FText`, propriété métier ou chemin Windows.
- Contexte de validation sur le SHA 06.4J exact :
  `UE5.5.4/ARM-CLEAN06.4J`.

**Pas de `SavePackage`** ; liste d'écriture des `.uasset`,
`.umap` et `.sav` toujours vide. SUCCESS indique
seulement que le diagnostic a été calculé sur les vrais
assets Unreal, pas la permission de générer les
11 définitions physiques.

## Ce qui suivra

ARM-CLEAN06.4K devra définir une correspondance **explicite**
pour chaque propriété non recouverte par les classes de
destination (torche, lecture, actions, effets, règles de
pression, physiques et lancer), puis comparer réellement
le comportement des objets. Il faudra inventorier
les autres contenus binaires qui seraient modifiés par
le cutover et préparer le plan de compatibilité/réécriture
des SaveGames avec backup, rollback et preuves PIE/Chaos
avant un commit contenant des `.uasset`.

Les WBP et textes joueur ne sont pas concernés ;
`UI.csv → ST_UI → WBP → Game/fr/en` est inchangé.
