# Checklist de régression du donjon de test

## Périmètre

- Carte : `/Game/GrimrockPrototype/Maps/L_GrimrockRuntime`
- Niveau : `/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00`
- Acteur runtime : `/Game/GrimrockPrototype/Blueprints/Runtime/BP_GridLevelRuntimeActor`
- Pawn : `/Game/GrimrockPrototype/Blueprints/Runtime/BP_GrimrockPartyPawn`
- Palette de validation : `/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default`
- Mode de jeu global : `/Game/GrimrockPrototype/Blueprints/Runtime/BP_GrimrockGameMode`

Statuts :

- **PASS** : vérifié pendant cette passe.
- **BLOQUÉ** : le test ne peut pas être conclu à cause d'une dépendance identifiée.
- **NON COUVERT** : absent du niveau ou nécessitant une session PIE interactive.

## Chargement et validation

- [x] **PASS** - La carte, le niveau, la palette, l'acteur runtime et le pawn se chargent.
- [x] **PASS** - Le niveau mesure `32 x 32`, avec des cellules de `200 uu`.
- [x] **PASS** - Le départ est configuré en `(28, 23)`, orienté au nord.
- [x] **PASS** - Le niveau contient `21` objets et `6` liens depuis le commit `bb9941e`.
- [x] **PASS** - La validation complète s'exécute sans erreur : `0` erreur, `10` avertissements, `1` information.
- [x] **PASS** - Le démarrage runtime automatisé atteint le monde de jeu sans plantage.
- [ ] **NON COUVERT** - Vérifier visuellement la géométrie, les matériaux, les plafonds et les collisions en PIE.

## Déplacement et interaction souris

- [ ] **NON COUVERT** - Déplacement avant, arrière et latéral.
- [ ] **NON COUVERT** - Rotation du groupe et respect des murs.
- [ ] **NON COUVERT** - Blocage par une porte fermée et passage après ouverture.
- [ ] **NON COUVERT** - Curseur, survol, portée et priorité du premier hit bloquant.
- [ ] **NON COUVERT** - Clic sur les objets de bord depuis les cellules adjacentes.

## Mécanismes présents

- [ ] **NON COUVERT** - Le bouton secret en `(27, 22)` ouvre la porte secrète en `(28, 22)`.
- [ ] **NON COUVERT** - Le levier en `(20, 28)` bascule la porte grillagée de la même cellule.
- [ ] **NON COUVERT** - Le retrait d'un item du porte-torche en `(28, 27)` ouvre la porte liée.
- [ ] **NON COUVERT** - La réinsertion dans ce porte-torche produit le comportement attendu.
- [ ] **NON COUVERT** - Les deux autres réceptacles acceptent, conservent et rendent leurs items conformément à leur configuration.
- [ ] **NON COUVERT** - L'inscription en `(28, 27)` affiche puis ferme correctement son texte.
- [ ] **NON COUVERT** - Les retours courts n'écrasent pas durablement un message lisible.
- [ ] **NON COUVERT** - L'escalier en `(26, 25)` déclenche la transition attendue.

## Items

- [ ] **BLOQUÉ** - Le Blueprint utilise `/Game/GrimrockPrototype/Art/FX/NS_Flame_8_Torch` et ne référence plus l'ancien système absent, mais ce nouvel asset dépend encore de `/Game/F_FreeFlameFx_Pack/Materials/MI_Distortion_1` et `/Game/F_FreeFlameFx_Pack/Materials/MI_Flame_8`, également absents. La validation visuelle PIE reste à effectuer après correction.
- [ ] **NON COUVERT** - Ramassage d'un item au centre d'une cellule.
- [ ] **NON COUVERT** - Ramassage d'un item placé sur un bord.
- [ ] **NON COUVERT** - Dépôt et retrait avec inventaire plein.
- [ ] **NON COUVERT** - Refus d'un item incompatible et retour utilisateur associé.

## Scénarios absents du niveau

- [ ] **NON COUVERT** - Plaque de pression.
- [ ] **NON COUVERT** - Trigger de cellule.
- [ ] **NON COUVERT** - Liens conditionnels par définition, tag, type, quantité, poids ou inversion.
- [ ] **NON COUVERT** - Source ou cible de lien désactivée.
- [ ] **NON COUVERT** - Porte verrouillée et mécanisme dont la rétraction est désactivée.

## Contrôle avant intégration

- [x] **PASS** - Aucun `.uasset` n'a été modifié.
- [x] **PASS** - La correction C++ reste limitée à la catégorie des messages de validation.
- [x] **PASS** - Compilation UBT `GrimrockPrototypeEditor Win64 Development`.
