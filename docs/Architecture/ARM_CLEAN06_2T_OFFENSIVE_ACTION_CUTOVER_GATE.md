# ARM-CLEAN06.2T — Refus explicite des armes offensives incomplètes

Le catalogue numérique transitoire ARM-CLEAN06.2S préserve 82 archétypes et
les 1 640 valeurs N1–N20, mais ne publie pas encore les sources d'attaque et
les liens `ActionId/PresentationId` des 22 armes du manifeste : l'ancienne
présentation n'est pas toujours associée à une clé stable.

**Danger supprimé :** un registre pouvait considérer une fiche ARM offensive
avec `bHandAndKindProven=true` comme valide, même si
`AttackSourceSlots` était vide, ou si les liens présents ne
pointaient que vers une action `Defend`. Ce serait une perte silencieuse de
l'attaque normale au passage au runtime canonique.

Le nouveau contrat fail-closed opère à deux niveaux :

1. `UGridItemSystemAsset::Validate` exige pour tout
   `UGridArmamentDefinitionAsset` concret de type `Weapon` une source
   MainHand/OffHand réellement authorée **et** au moins une action liée
   dont le type canonique est exactement `Attack`.
2. `UGridItemSubsystem::ResolveItem` refuse aussi l'arme
   insuffisamment renseignée même si le registre a changé depuis
   son enregistrement. Les boucliers/focus passifs n'acquièrent
   aucune attaque artificielle.

Les **60 archétypes non concrétisés** continuent d'exister comme profils
numériques sans exiger d'action ni de fiche d'objet factice.

Le test
`Grimrock.RPG.ARM.CLEAN06_2.Canonical.ActiveWeaponAttackReadinessGate`
vérifie successivement un registre valide, la suppression de la source de
main, la suppression du `ItemActionBinding`, une action reclassifiée
`Defend`, puis la restauration et le fonctionnement du bouclier passif.
Chaque état incomplet est rejeté sans fallback ni nouveau comportement de
combat.

Aucune donnée `.uasset`, `.umap`, SaveGame, UI ou gameplay existant
n'est modifiée. Le jalon est limité aux classes canoniques non activées et
à leur sécurité de migration ; la validation Windows sur SHA exact est
obligatoire.
