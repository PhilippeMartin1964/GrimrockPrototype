# ARM-CLEAN06.3A — Charger le registre natif au démarrage

Cette étape est autorisée **seulement après** publication et validation
des **28 DataAssets réels** dans `CanonicalStage`.

- `Config/DefaultGame.ini` définit une seule référence
  `UGridItemSubsystem.ConfiguredItemSystem` au `DA_ItemSystem` natif,
  et inclut le répertoire dans les packages à cuire.
- Le test Automation `RealNativeRegistryAndActions` charge cette
  **racine réellement sauvegardée** puis enregistre son catalogue
  dans un véritable `UGridItemSubsystem`, sans conversion source
  ni accès aux deux objets hors manifeste.
- Les 22 ARM sont résolues à N1 et N20 (noms/poids/identités,
  19 actions et trois passifs) ; chaque attaque principale
  obtient son `PresentationId` réellement sauvegardé, dont
  exactement 15 références au profil neutre partagé.
- Les flèches proviennent du même registre.

**Portée volontairement limitée** : configuration + test de
lecture du modèle canonique. L'inventaire, le combat et les
acteurs du niveau utilisent encore leurs lecteurs historiques ;
aucune activation prématurée ni fallback ajouté. Le basculement
de ces lecteurs est la suite de 06.3.
