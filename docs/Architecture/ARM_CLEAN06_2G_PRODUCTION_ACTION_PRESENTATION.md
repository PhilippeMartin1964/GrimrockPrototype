# ARM-CLEAN06.2G — Actions et présentations réelles des 22 ARM

## Périmètre et justification

L'autorité de production reste `Config/Armement/WeaponProductionManifest.csv`
(22 armes ARM actives, dont trois objets passifs, et 82 progressions dans
le catalogue). Aucun parcours des packages hors manifeste n'est entrepris.

L'ancien `FGridCombatActionDefinition` contient notamment ciblage avancé,
effets, statuts, coûts, exigences, variants de propriétaire, compétences,
mouvements, surfaces et pièges. Le modèle actuel
`FGridCanonicalAction` ne représente PAS encore tous ces paramètres.
**Interdiction de créer maintenant 22 actions prétendument équivalentes
en recopiant seulement ActionId / Targeting / Cooldown.** Ce serait
une conversion avec perte et pourrait modifier le combat.

## Progression effective : présentation migrable sans perte

L'outil Editor-only
`GridArmClean06PresentationProjection::TryProject` prend un
`FGridPlayerAttackPresentationProfile` et un `PresentationId` transmis
explicitement. Il produit une valeur canonique transitoire :

- mouvements, offsets, délais d'impact/récupération et durée feedback ;
- les trois événements audio historiques (attaque, hit, miss), incluant
  toutes les variantes dans le même ordre, volume et limites de pitch ;
- les trois événements Niagara (attaque, hit, miss), systèmes multiples,
  attachement, socket, localisation, rotation, échelle ;
- aucun événement critique, parade, blocage, matières ou son nouveau
  déduit d'une absence de données.

L'outil refuse un profil historique invalide et ne conserve pas de sortie
partiellement remplie. Il est sous `GrimrockPrototypeEditor`, pas dans le
runtime. Aucune présentation n'est sérialisée ni câblée à ce stade.

## Vérifications Automation du vrai contenu

- `Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionActionPresentationParity`
  utilise le lecteur officiel du manifeste, charge uniquement les 22
  `ItemPath` et vérifie les identités, toutes les actions locales, la
  présence de l'attaque principale, l'absence d'attaque sur les passifs,
  et la projection de chaque présentation réellement activée.
- Chaque canal copié est comparé champ par champ, y compris les
  `SoftObjectPath` ordonnés, sans changer le `UPackage::IsDirty`.
- Les chiffres de profils avec/sans présentation, actions locales et
  `PresentationProfileId` absents sont des **diagnostics**. Ils ne
  déclenchent pas de création d'actions supplémentaires.
- `Grimrock.RPG.ARM.CLEAN06_2.Canonical.PresentationProjectionSafety`
  prouve le refus d'un ID nul et d'une plage audio invalide.
- Le test mains précédent utilise maintenant le véritable include
  UE5.5.4 `Serialization/Csv/CsvParser.h`.

## Étape suivante

Relever depuis les **22 actions du manifeste seulement** les valeurs des
champs non représentés, établir les règles de propriété des données puis
étendre `FGridCanonicalAction` par des **payloads typés** avant d'authorer
les quatre catalogues canoniques sous UE Editor. Les noms d'action ne
doivent pas être inventés ; les `PresentationProfileId` absents
ne sont jamais remplacés en silence.

**Condition de clôture :** build UE5.5.4 + tests Automation `Grimrock`
sur le SHA publié ; sans cette preuve, statut `en attente de validation`.
Rien n'est sauvegardé en .uasset/.umap/SaveGame et aucune logique runtime,
arme historique, arme de jet, effet Sword ou RatGiant n'est modifiée.
