# ARM-CLEAN06.2F — Mains et sources d'attaque des ARM de production

## Périmètre strict

Le manifeste `Config/Armement/WeaponProductionManifest.csv` est l'autorité de
sélection : **22 définitions actives**, reliées à leurs profils ARM parmi les
82 archétypes du catalogue. Les 60 autres ARM sont toujours des **archétypes
numériques valides** (N1–N20) ; **une définition concrète par ARM n'est pas
nécessaire** tant qu'aucun item de production ne l'utilise.

Les anciens objets hors manifeste sont hors périmètre de cette étape.
Aucun inventaire de packages, redirector, autre classe, sauvegarde, niveau
ou asset audio ne constitue désormais une condition de progression ARM.
La conservation de leur comportement par le code historique est inchangée.

## Comportement canonique modélisé

Le modèle `FGridCanonicalWeaponArchetype` détient maintenant, à côté de
`HandPolicy`, une liste **indépendante** `AttackSourceSlots`, sur le
catalogue d'armes uniquement (aucune nouvelle propriété dans
`UGridArmamentDefinitionAsset`).

- `HandPolicy` définit la permission d'équipement (MainHand, OffHand,
  EitherHand, TwoHanded).
- `AttackSourceSlots` définit exclusivement les mains depuis lesquelles
  une **attaque réellement authorée** peut être fournie. Elle ne
  crée pas d'action.
- Le résultat transitoire `FGridCanonicalResolvedWeapon` expose
  `CanEquipInHand()` et `HasAttackSourceInHand()`. Ces deux fonctions
  ne sont **pas** raccordées aux lecteurs historiques dans ce jalon.
- Un profil passif `Shield` ou `Focus` refuse tout AttackSourceSlot.
  Une source d'attaque non permise par HandPolicy, répétée ou sans
  ItemActionBinding est refusée par la validation du catalogue.
- `TwoHanded` signifie que le transfert inventaire devra occuper
  physiquement les deux mains, avec un seul objet stocké en MainHand.
  Ce contrat existe déjà dans le legacy ; cette étape ne change
  pas le transfert ni les sauvegardes.

Le mapping test utilise **les booléens effectifs des vraies définitions UE** :
`CanEquipToSlot` pour l'équipement,
`CanProvideAttackFromSlot` pour les sources d'attaque, et la classe
`FocusPassive/ShieldPassive` explicitement présente dans le manifeste
pour les passifs. Il n'utilise ni les descriptions XLSX comme règles, ni
l'absence de dégâts comme preuve suffisante d'un Focus.

## Test production et suites

`Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionHandActionParity` :

- lit uniquement les 22 chemins `ItemPath` du manifeste, exige leurs
  `ItemDefinitionId` et `ArchetypeId` correspondants ;
- construit un `FGridCanonicalWeaponArchetype` transitoire par item
  (aucun .uasset sauvé), en recopiant la progression ARM compressée ;
- compare les deux permissions d'équipement et les deux sources d'attaque
  avec les vraies fonctions runtime, de N1 à N20 (22 × 20 = **440** niveaux) ;
- vérifie la parité des dés et protections pour ces 440 profils ;
- caractérise les 17 permissions élargies **sans attaque supplémentaire**,
  les trois passifs NotHandHeld équipables, les deux boucliers, le
  FocusPassive manifesté et les 19/2 sources d'attaque Main/Off.

**Il ne matérialise pas encore les ItemActionBinding et PresentationId** :
les valeurs viennent des vrais assets et demandent une conversion de
présentation sans perte. Aucun identifiant de présentation n'est inventé.

Une fois le SHA validé par `UE5.5.4/LocalAuto`, la suite utile est
la migration des actions / présentations des **22 ARM du manifeste** dans
les quatre catalogues canoniques, d'abord par projection vérifiable,
puis par authoring UE contrôlé. Aucun nettoyage des objets historiques
hors manifeste n'est planifié.

**Pas de modification binaire ni de changement de gameplay dans 06.2F.**
Les anciennes APIs restent actives jusqu'au cutover cohérent ;
`ARM-CLEAN06.3–06.5` ne sont pas lancés dans ce commit.
