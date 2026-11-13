# ARM-CLEAN06.2V — Munition réelle et rattachement des arcs ARM

Un unique `DA_Item_Arrows_01` existe déjà sous
`/Game/GrimrockPrototype/Core/DataAssets/Items/`.
Ce jalon convertit **en mémoire seulement** ses champs
nécessaires en `UGridAmmoDefinitionAsset` :

- `ItemDefinitionId` → `ItemId` et
  `AmmoKindId` (réutilisation de **l'identifiant réel**,
  sans invention d'un second type de flèche) ;
- nom, description, poids nominal, tags, icône,
  mesh monde, pile maximale.

Parmi les 22 armes du manifeste, seuls les
`RequiredAmmunitionItemDefinitionId` réellement
renseignés avec `Item_Arrows_01` sont reportés
comme `RequiredAmmoKindId` dans les archétypes
canoniques. Le convertisseur refuse une identité
différente, un item non empilable, une pile
invalide, une référence inconnue ou une
seconde conversion sur le même registre.

Les autres armes ne reçoivent pas de coût
de munitions artificiel. La consommation
d'une flèche à l'acceptation de l'attaque
reste un mécanisme d'exécution à préserver
au cutover 06.3/06.4, pas un doublon de dégâts
dans l'arme ni un lancer physique de l'objet.

Le test
`Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionAmmoIdentityAndBowLinks`
compose le prévol 06.2U, ajoute le vrai item
des flèches, valide tous les liens d'armes
existants, contrôle 23 items dans le registre
candidat, la conservation des champs et
l'interdiction d'une duplication.

**Le registre reste non autorisé pour la
conversion binaire** : les actions sans liaison
canonique, les politiques de lancer et tout
autre écart de parité doivent encore être réglés.
Aucun `.uasset`, niveau, SaveGame, WBP ou
système de combat historique n'est modifié.
