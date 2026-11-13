# ARM-CLEAN06.2AB — Résolution d’attaque par vraie main autorisée

Le futur `UGridItemSubsystem` possède déjà
`ResolveBoundAction(ItemId, ItemLevel, ActionId)` pour
résoudre des actions génériques, des consommables et leur
présentation. Cette méthode **ne reçoit pas de main** et
ne doit pas servir seule à exécuter une attaque d'équipement.

## Nouveau point d'entrée canonique

`ResolveBoundAttackForHand(ItemId, ItemLevel, ActionId,
SourceHand, OutAttack)` ajoute une protection métier
stricte sans changer le runtime historique :

1. `ItemId` et `ItemLevel` sont résolus par le registre
   unique ; aucune entrée hors catalogue n'est consultée.
2. La définition doit être une arme ARM concrète, dont
   `FGridCanonicalResolvedWeapon::HasAttackSourceInHand`
   accorde **explicitement** la main demandée.
3. La liaison `ActionId/PresentationId` est résolue
   par `ResolveBoundAction`, sans autre source ni fallback.
4. L'action liée doit être de type `Attack` ; un
   `Defend`, `Use` ou `Ability` n'est pas une
   attaque de base même si son ID existe.

Les erreurs `AttackSourceNotAuthorized` et
`ActionIsNotAttack` sont distinctes des erreurs
d'item, de liaison et de présentation existantes.
Un refus remet la sortie à zéro afin que le HUD
n'utilise jamais une action précédemment réussie.

## Séparation équipement / combat

**Pouvoir équiper une arme dans les deux mains
ne donne pas automatiquement une attaque OffHand.**
Les 19 sources MainHand et deux OffHand attestées
dans les 22 ARM resteront les seules autorisations.
Une arme à deux mains peut être occupante dans
les deux slots sans que cela crée une seconde attaque.

Les boucliers et focaliseurs passifs n'obtiennent
jamais d'attaque artificielle. Un consommable peut
conserver ses usages génériques via
`ResolveBoundAction`, mais ne peut pas emprunter
un slot de frappe d'arme.

## Preuve

`Grimrock.RPG.ARM.CLEAN06_2.Canonical.AuthorizedAttackSourceHandResolver`
teste le registre absent, une épée N20, OffHand
interdit, politique d'équipement élargie sans
attaque supplémentaire, bouclier, potion,
ActionId inconnu, niveau invalide, une action
`Defend` liée et une présentation absente.

Les changements concernent **seulement le nouveau
sous-système canonique non activé** et ses tests.
Aucun lecteur combat historique, acteur UE,
WBP, asset `.uasset`, niveau ni SaveGame
n'est modifié. Ce jalon ne constitue pas
une activation de 06.3 ni une validation PIE.
