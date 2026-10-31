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

## TRAP-PIT01.1 — Camouflage en PIE et sélection des objets superposés

Le TRAP-PIT01 initial était difficile à éditer et rendait visible une fosse
qui devrait être secrète. **Correctif :** lorsqu'un piège au sol placé porte
`Ouvrir une fosse au déclenchement`, une fosse PIT03 contrôlée (deux volets)
associée sur sa case est **visuellement dissimulée tant qu'elle est fermée**.

### Configuration dans le Grid Editor

1. Posez un piège de type **sol** et une seule fosse PIT03 à deux volets
   **sur la même case**. Ne remplacez pas la case par une `CellType::Pit`
   statique.
2. Dans **Overview Map → Selected Cell → Objects on Cell**, les deux objets
   disposent chacun d'un bouton **Select**. Le clic dans le viewport
   privilégie maintenant le piège de sol lorsqu'il est superposé à une fosse.
3. Sélectionnez le piège, puis activez **Ouvrir une fosse au déclenchement**.
   Utilisez le nouveau bouton **Configurer la fosse associée** pour accéder
   aux réglages de la fosse. Un bouton inverse permet de revenir au piège.
4. Sur la fosse : **Open at Start = false**, **Use Same Cell Coordinates =
   true** si chute verticale, et **Relocation → Destination Level** vers
   le niveau inférieur (ou `None` pour l'étage inférieur automatique).
5. Enregistrez le LevelAsset. En PIE, une case encore armée conserve son
   **sol normal** ; le cadre et les deux volets PIT03 restent cachés.
   Après déclenchement, le sol se retire, la fosse apparaît puis le groupe
   chute par PIT01. Une fosse normale non associée n'est pas modifiée.

Sur la carte explorée, la fosse secrète n'apparaît pas tant qu'elle est
fermée, y compris après découverte d'un piège détecté. Après ouverture,
le marqueur de fosse remplace celui du piège au lieu de se superposer.

L'architecture conserve deux placements typés (Trigger + Pit) dans la même
case, mais leur navigation est explicite et l'apparence de la fosse est
gérée comme un élément du piège. Les réglages restent **propres à chaque
placement**, sans modification du DataAsset partagé.

### Tests de régression

`Grimrock.TRAP_PIT01` : secret de la fosse, suppression du sol à l'ouverture,
déclenchement et restauration des états ; `Grimrock.Map.TRAP_MAP01` :
non-divulgation, découverte et priorité Pit ; `Grimrock.Pit.PIT03` :
aucune altération d'une fosse ordinaire.

**Compilation UE5.5.4 et recette PIE obligatoires avant validation.**

## TRAP-PIT01.2 — Aucun dégât direct sur une instance de piège

L'option **Type de dégâts → Aucun** est disponible uniquement dans
l'inspecteur des pièges placés, stockée dans `FGridDungeonTrapInstanceOverrides::bNoDamage`
du `LevelAsset`. Elle ne modifie ni les DataAssets partagés, ni l'énumération
globale `EGridDamageType` des combats. Type, sous-type et plage de dégâts
restent mémorisés et réapparaissent en rechoisissant un type de dégâts.

Le piège sans dégâts continue de se déclencher, ouvrir sa fosse PIT03,
activer les liens logiques et conserver son état persistant. Il ne lance
pas de jet de dégâts, n'altère pas PV ou armure et ne publie pas
`Dungeon.Trap.Damage`. Les conséquences de la chute PIT01 sont distinctes.

À valider dans UE5.5.4 : `Grimrock.TRAP_PIT01`,
`Grimrock.TRAP_INSTANCE01`, `Grimrock.TRAP01`, puis PIE avec les choix
Aucun / Physiques / Feu et plusieurs pièges partageant le même DataAsset.
