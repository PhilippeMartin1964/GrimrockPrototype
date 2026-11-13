# ARM-CLEAN06.3D — Actions disponibles : source canonique au combat

La construction des **actions d'armes réellement équipées** dans
`UGridTurnManagerComponent::BuildPlayerCombatActionContributions`
et la contribution des armes à
`GetAvailableCombatActions` lisent maintenant les 22 `ItemId`
présents dans le `DA_ItemSystem` de la GameInstance.

Pour ces identités, plus aucune lecture de
`DA_Weapon_*.CombatActions`, de
`DA_WeaponCatalogue`, ni des tags copiés dans l'ancien asset.
Le seul propriétaire reste le registre des quatre catalogues.
N1–N20, type de dégâts, dés, PA, portée, cible, main autorisée,
coût des lancers, tags et icône proviennent du modèle résolu.

Le moteur MON12 et le HUD réclament encore leur
`FGridCombatActionDefinition` ; le lecteur construit **une vue
transitoire** dans ce format déjà existant, sans ajouter
une nouvelle classe, un adaptateur persistant, une propriété
sur `DA_Weapon_*`, ou un second catalogue.
Cette structure contient des valeurs calculées pour **l'exécution
de l'action**, et n'est pas sérialisée comme une source de données.

Si une définition canonique existe mais ne résout pas son
identité de slot, son niveau ou son attaque, elle est **omise**
sans repli dans l'ancien lecteur. Les armes hors du registre,
consommables, actions de classe et sorts restent sur leurs
lecteurs en attendant leur migration.

La logique particulière DualWield, l'exécuteur, les munitions et
l'effet visuel tenu utilisent encore certains anciens lecteurs :
ils ne sont PAS présentés comme entièrement convertis ici.

Le test de données réelles `RealNativeRegistryAndActions`
couvre l'identité d'équipement et ses 19 attaques au niveau 20,
ainsi que le rejet strict des niveaux 0.
Validations Windows complètes obligatoires. PIE sera demandé
au jalon de vérification du combat raccordé, avant suppression du
legacy et conversion des sauvegardes.

Aucun `.uasset`, `.umap`, SaveGame, ni source hors des 22 ARM du
manifeste n'est modifié.
