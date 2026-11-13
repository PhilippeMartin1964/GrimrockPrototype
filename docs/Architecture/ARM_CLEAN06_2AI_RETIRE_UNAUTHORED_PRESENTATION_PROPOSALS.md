# ARM-CLEAN06.2AI — Éliminer les propositions de présentations non authorées

**Décision :** les outils exploratoires 06.2Z et 06.2AA ne peuvent pas
devenir une autorité de migration. Ils utilisent des `ActionId` existants
comme *propositions* de `PresentationId`, sans que le contenu original
ait authoré une telle liaison. Leur usage n'a jamais accordé
`ReadyForBinaryCutover`. Ils sont donc retirés de la branche active.

## Retrait

- `GridArmClean06PresentationKeyPreview.{h,cpp}`
- `GridArmClean06ProvisionalPresentationAssembly.{h,cpp}`
- leurs deux fichiers de tests isolés

Les rapports 06.2Z et 06.2AA restent consultables dans l'historique
Git, mais aucune proposition n'est conservée comme source de gameplay.
Le convertisseur réel
`GridArmClean06ProductionActionsCandidate` est inchangé.
Ses exigences de `PresentationProfileId` explicite et de profils
d'actions intégralement représentables restent en vigueur.

## Test conservé dans la chaîne de production

Le test courant `ProductionActionCatalogues22` examine maintenant
les **19 attaques réelles du manifeste** et exige, pour les seules
actions effectivement projetées, qu'un `PresentationProfileId`
absent ne produise **aucun** profil ayant `ActionId` comme ID
substitutif. Le compteur de sources sans identifiant est comparé
à `NoAuthoredPresentationId`. Les tests 06.2Q/U/Y continuent
de refuser une racine incomplète.

Ce changement ne modifie ni la sémantique des armes, ni les assets,
ni le runtime historique, ni les sauvegardes. Il n'inclut pas et
n'inspecte pas les deux `DA_Weapon_*` hors manifeste.

## Bilan contrôlable

- 537 lignes C++ retirées dans six fichiers ;
- 34 lignes ajoutées au test 06.2Q ;
- aucun nouveau résolveur, candidat, convertisseur ou fichier C++ ;
- **réduction C++ nette : 503 lignes**, à vérifier par diff Git ;
- validation UE5.5.4 `SilentTask` et `LocalAuto` obligatoire
  sur le commit exact.

## Blocage désormais explicite

**Il faut authorer les vrais IDs de présentation manquants**
et convertir les actions personnalisées si leurs effets sont
représentables sans perte. Aucune liaison provisoire
ne doit être confondue avec la preuve de parité. Le nouvel
`DA_ItemSystem` réel et la conversion 06.3–06.5 restent bloqués.
