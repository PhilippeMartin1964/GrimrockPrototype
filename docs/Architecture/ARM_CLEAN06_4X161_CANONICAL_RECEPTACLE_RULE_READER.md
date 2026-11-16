# ARM-CLEAN06.4X.16.1 — Règles d'acceptation canoniques : schéma et lecture sûre

Parent : ARM-CLEAN06.4X.15 (`8b9d14d8e6499e3f7c3a778d4497b1d07b32e4c1`)
et documentation (`0c8da4bc36ea96b90b146d10e51a6a75cb5d5262`),
tous deux validés sur `master` avec `UE5.5.4/SilentTask` et
`UE5.5.4/LocalAuto` SUCCESS. Le retour PIE utilisateur a
également validé les réceptacles et les énigmes existantes.

## Autorité d'identité et compatibilité

`FGridReceptacleAcceptedItemConfig` gagne un champ persistable
`CanonicalItemId`, mais **aucun vrai asset ne le reçoit par cette
étape**. Les définitions et overrides natifs gardent donc leur
ancien comportement jusqu'à une migration transactionnelle dédiée.

Le lecteur `FGridReceptacleAcceptedCanonicalReadModel` s'applique
après résolution effective Definition/Instance :
- règle avec `CanonicalItemId` : membership exact du seul
  `DA_ItemSystem` ; le pointeur historique, s'il existe,
  doit porter le même ItemId. Aucune retombée historique
  si le registre manque, si l'ID est inconnu ou contradictoire ;
- règle sans `CanonicalItemId` : parcours historique inchangé
  pour les anciennes sources et fixtures non converties ;
- `bAcceptAnyItem`, capacité, insertion autorisée, priorité
  des surcharges, réactions logiques et GUID ne changent pas ;
- une règle d'acceptation compare l'**ItemId** et ne crée ni ne
  convertit un niveau physique N0/N1.

Le runtime consomme ce lecteur pour les règles de réceptacles,
sans modifier les clés acceptées des serrures. Celles-ci ont
déjà un contrat distinct `AcceptedKeyItems/AcceptedKeyIds`
et nécessitent leur propre préflight de migration. L'inspecteur
permet encore la sélection historique, en attendant un
write-through canonique avant la conversion des vrais assets.

## Validation

Test `Grimrock.RPG.ARM.CLEAN06_4X161.CanonicalReceptacleRuleReader` :
ID-only valide, pointeur concordant/contradictoire, identité
inconnue, registre absent, règle legacy, acteur runtime avec
acceptation/refus, accept-any explicite et remplacement complet
de la règle de définition par sa surcharge d'instance.

Ne pas supprimer les pointeurs historiques ni relancer
06.4W / 06.4X.5. Zéro écriture `.uasset`, `.umap`,
`.sav`, Blueprint ou WBP. La recette PIE des anciens
réceptacles reste acquise mais **ne prouve pas une nouvelle
règle canonique réelle** tant que les DataAssets ne sont
pas convertis et joués.

**Suite :** préparation X.16.2 de l'authoring canonique
du champ `CanonicalItemId` dans l'inspecteur, puis préflight
et migration des règles réelles uniquement avec comptage
et validation transactionnelle sur copies. Séries distinctes
pour les serrures et SaveGames.
