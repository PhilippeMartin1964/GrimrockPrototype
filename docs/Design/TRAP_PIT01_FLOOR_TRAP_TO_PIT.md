# TRAP-PIT01 — Un piège au sol peut ouvrir une fosse vers l'étage inférieur

**Statut : code publié sur master pour validation UE5.5.4 / PIE.**

## Choix d'architecture

Le piège `Trigger` reste un objet de niveau, identifié par son GUID, et
**ne se convertit pas en un nouvel objet `Pit`**. La case comporte un
piège de sol invisible et un `Pit` PIT03 à deux volets, initialement fermé.
Le contrôle d'instance `Ouvrir une fosse au déclenchement` se trouve
uniquement sur les pièges de surface `Floor`, dans `Selected Object`.
Il est sauvegardé sous
`FGridWorldObjectInstanceConfig::DungeonTrap.bOpenPitOnTrigger`
(dans le LevelAsset, jamais le DataAsset commun).

L'auteur doit placer **une seule** fosse `Pit` sur la même cellule et
configurer son option `Open at Start = false`, sa destination
(`Destination Level = None` pour le niveau inférieur automatique,
ou un LevelId explicite) et les coordonnées d'atterrissage.
La fosse doit être dotée des deux volets PIT03.
L'inspecteur indique si une fosse compatible est présente ;
il ne fabrique aucun `.uasset` ni une fosse implicite.

## En jeu

Lorsque le groupe entre dans la case :
1. Un piège armé `OnEnterCell` réalise **une seule** activation et applique
   ses éventuels dégâts individuels selon l'algorithme TRAP01.3.
2. Si l'option est activée et qu'une unique fosse compatible est présente,
   le runtime appelle `SetPitOpen(PitId, true)`.
3. PIT03 ouvre immédiatement la fosse, fait tomber les objets présents
   (PIT02) et démarre la chute du groupe sur la case via PIT01.
4. Le mouvement et l'interaction ordinaires de fin de case sont suspendus
   si la chute a commencé, sans lancer un second transfert.
5. Les GUID de piège et fosse et leurs états sont distincts et persistés
   dans les structures `FGridLevelRuntimeState`. `Save/Continue`
   doit conserver la fosse ouverte et le piège consommé.

Un piège désamorcé ou déjà consommé n'ouvre pas la fosse.
Une fosse absente, ambiguë, déjà ouverte ou sans destination exploitable
n'est pas forcée : le déclenchement du piège et ses dégâts restent valides,
un diagnostic `LogGridDungeonTrapDamage` apparaît.
L'option ne s'applique pas aux pièges mur/plafond.
Les liens logiques existants `Activated -> Open` restent fonctionnels et
ne sont pas remplacés ; si un auteur utilise ces liens, il n'a pas besoin
d'activer la nouvelle option automatique.

**Limite de conception :** un `Pit` statique sans les deux volets PIT03
est physiquement ouvert par contrat PIT01 et ne constitue pas une fosse
fermée qui s'ouvre au déclenchement.

## Tests

- `Grimrock.TRAP_PIT01.UniqueSameCellPairing` : absence, unicité,
  ambiguïté, coordonnées et GUID invalide.
- `Grimrock.TRAP_PIT01.FloorTrapOpensPitAndPersists` :
  ouverture, dégâts puis chute, déclenchement unique et état PIT sauvegardé.
- Régressions `Grimrock.TRAP01`, `Grimrock.TRAP_INSTANCE01`,
  `Grimrock.Pit.PIT01`, `Grimrock.Pit.PIT03` et `Grimrock.Map`.

Validation PIE indispensable : piège non détecté, désamorcé, déclenché,
chute inter-niveaux, sauvegarde/reprise, absence de chute doublée,
visualisation de la trappe et marqueurs carte. Ne pas déclarer terminé avant
la compilation et ces vérifications.

Note : le read-model de la carte garde les symboles Pit et Trap distincts ;
la priorité visuelle de superposition reste à traiter avec le retour PIE.
