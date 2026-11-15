# ARM-CLEAN06.3AW — Capacités contextuelles, lecture et raccourcis canoniques

**Parent :** ARM-CLEAN06.3AV, SHA `bf444f0`.

## Problème réel

La migration du lancer physique 06.3AV autorisait la création, le vol
et la récupération d'une arme de jet enregistrée, sans `DA_Weapon_*`.
Cependant le menu contextuel, `SetCharacterCombatHotbarBindingFromItem`
et `BuildPhysicalThrowInventoryAction` exigeaient encore l'ancienne
définition pour afficher, créer et exécuter le raccourci
`ThrowItem_<ItemId>`.

Les champs historiques `ItemType`, `ReadText`,
`bProvidesQuickItemCombatAction` et `CombatActions` pouvaient aussi
faire apparaître de fausses actions `Consommer`, `Lire` et
`Ajouter à la barre` sur un `ItemId` déjà enregistré, même si
`DA_ItemSystem` ne prévoyait pas cette capacité.

## Réalisation

`FGridCanonicalItemInteractionReadModel` sépare les deux familles :

- **ItemId canonique** : résolution de l'instance à son niveau réel.
  Une arme à `Weapon.Projectile.bEnabled` et à politique de main
  compatible gagne le raccourci physique déjà pris en charge par le
  système ; aucune définition ni action historique n'est récupérée.
  Les munitions N0 ne deviennent pas artificiellement lançables à
  la main. Un ARM enregistré N0 ou N21 échoue sans repli.
- **Lecture** : pour les IDs canoniques, seuls `ReadTextOverride`
  et `ReadableContentAsset.BodyText` de l'**instance effective**
  autorisent `Lire`. Le panneau affiche le nom du registre lorsque
  aucun titre explicite n'est fourni. Ni le `ReadText` ni le type
  `Book/Scroll` d'un ancien `DA_Weapon_*` ne font autorité.
- **Consommation** : les IDs canoniques ne sont jamais confiés au
  vieux `BuildQuickItemCombatActionDefinition` ni à l'exécuteur
  `TryConsumeSelfRestorativeQuickItem`. Les `UseActions` du nouveau
  `UGridConsumableDefinitionAsset` demandent une migration
  spécifique de l'exécuteur avant activation ; **ne pas inventer
  de consommation**. L'inventaire et le catalogue de combat
  vérifient ce garde-fou même si le menu était déjà ouvert.
- **Raccourci** : le menu, la création du binding et son affichage
  physique dans le HUD utilisent la même identité canonique et
  la politique de Force. Le HUD lit le nom, la description et
  l'icône de `DA_ItemSystem` sans appliquer l'ancien
  `DA_Weapon_*.Icon`, même si celui-ci existe encore.
  L'état et le compteur d'objets viennent du sac actuel.
- **ItemId hors registre** : conserve les exécuteurs et les champs
  historiques d'usage, de lecture, de nourriture, de livre et de
  raccourci, en attendant la migration de sa famille.

Un raccourci physique n'est pas un raccourci `Use_*` :
`FGridCombatHotbarBinding::MakePhysicalThrowItemActionId` et
`IsPhysicalThrowItemBinding` restent les identités officielles.
Le système de lancement de 06.3AV demeure seul responsable de la
mutation de l'inventaire.

## Test de régression

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalItemInteractionCapabilities`
utilise le vrai `DA_ItemSystem` (GameInstance/World transitoires) et
contrôle : N1/N20, ancienne définition volontairement
contradictoire, menu contextuel avec et sans cet asset, instance
portant un véritable texte, N0 munitions, ARM N0/N21 invalides,
livre non enregistré, binding réel du sac vers le raccourci physique,
détection de doublon et refus d'une ancienne consommation
sur l'arme canonique sans retirer l'objet.

L'intégralité de `Grimrock` est ensuite exécutée par
`UE5.5.4/LocalAuto` sur le SHA publié. Ce test automatisé
ne prétend pas remplacer une recette visuelle UMG/PIE, ni
valider un asset `UGridConsumableDefinitionAsset` non encore
raccordé à un exécuteur canonique.

## Limites et phases suivantes

- **06.3AX :** déterminer l'exécution métier de `UseActions`
  canoniques, puis la lier au catalogue de combat et à
  l'exploration sans double autorité.
- **06.4 :** convertir les vrais `.uasset` et SaveGame par
  commandlet Unreal avant toute suppression historique.
- **06.5–06.6 :** retrait contrôlé du legacy, recette PIE,
  contrôle des traductions et des effets visuels.

## Localisation — vérification entrée et sortie

`Localization/Source/UI.csv → ST_UI.uasset → WBP → Game/fr/en` :
**inchangé sur les cinq étapes**. Toutes les chaînes visibles
(`Consommer`, `Lire`, `Ajouter à la barre d'action`, raisons
d'indisponibilité et format `Combat.Action.ThrowName`) existaient
déjà. Aucun nouveau TextBlock, WBP, `LOCTEXT`, `UI.csv`,
`ST_UI.uasset`, PO ou `Game.locres` n'est créé dans ce jalon.
Aucune nouvelle matérialisation de String Table n'est requise.
