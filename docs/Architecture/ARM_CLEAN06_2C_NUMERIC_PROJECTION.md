# ARM-CLEAN06.2C — Projection numérique canonique transitoire

**Nature :** test de conversion UE5.5.4 strictement en mémoire, après
ARM-CLEAN06.2A (inventaire/delta) et 06.2B (modèle audio sans perte).

Le commandlet GridArmClean06CatalogueProjection charge exclusivement le
DA_WeaponCatalogue existant, déjà compressé en 82 lignes de progression.
Il construit un UGridCanonicalWeaponCatalogueAsset **transient** en copiant
la baseline N1 et les cinq règles de croissance, sans enregistrer de package.

Pour chacun des 82 archétypes et chaque niveau N1–N20, il compare
directement à l'ancienne source UE les dés NdM+K, leur activation,
le type/sous-type des dégâts, le scaling, la précision, les points d'action,
la portée et les armures physique/magique. Il valide les 1 640 profils,
y compris 260 passifs, à travers le validateur du catalogue canonique.

## Ce qui reste volontairement indécidé

Les valeurs suivantes sont **explicitement marquées à convertir** et ne
doivent pas être déduites de simples valeurs numériques :

- Le type réel d'un objet passif (bouclier ou focaliseur) et les règles
  exactes de mains / dual-wield, auparavant portées par Family et HandUsage.
  Une valeur de catégorie provisoire dans la projection transitoire est
  une astuce de calcul, **pas une décision de gameplay**.
- L'association entre chaque archétype et les 24 définitions d'armes
  concrètes, sources uniques finales de DisplayName et WeightKg.
- Les affinités de classes, y compris Primary/Secondary et les qualifications
  textuelles, qui ne doivent pas être ramenées à une simple liste de noms.
- Les vrais ActionId/PresentationId, les défenses et réactions passives,
  les paramètres de lancer, les munitions et les pistes Sword.
- Les objets non-ARM, objets placés dans les niveaux/palettes et SaveGames.

Le JSON privé contient notamment Family, HandUsage, nom et poids de source
pour **chaque archétype** ; le rapport public n'expose que les cardinalités.
L'essai refusera tout écart numérique, nombre d'archétypes inattendu,
ou modification Git dans le worktree détaché.

## Déclenchement et publication

Après compilation et Automation Grimrock réussies, l'agent
VALIDATION-AUTO01 exécute une seule fois
Scripts/PreflightARM_CLEAN06_2_Catalogue.ps1 sur le commit dont le sujet est
« chore(ARM-CLEAN06.2C): prove transient compact catalogue numeric parity ».

Un statut UE5.5.4/ARM-CLEAN06.2C distinct et une issue GitHub résument
le résultat. Une réussite prouve **uniquement** la projection numérique
transitoire sans perte. Elle n'autorise ni publication d'assets binaires
canoniques, ni bascule des lecteurs, ni destruction d'UPROPERTY historiques.

Les 56 assets sources et les modifications utilisateur Sword/RatGiant
demeurent intacts. La suite 06.2D devra fermer les écarts sémantiques
avant d'authorer les vrais catalogues.
