# ARMEMENT01.6.2 — Résolveur visuel de l'armement

**Date :** 20 octobre 2026.  
**Statut :** code préparé, validation UE5.5.4 locale à fournir.  
**Référence validée :** TD04.2, 20 octobre 2026 à 19 h 34 : **1 476 tests**, zéro avertissement, zéro échec.  
**Contrat parent :** docs/Design/ARMEMENT01_6_CONTRAT_PRESENTATION_VISUELLE.md.

## Périmètre

Ce ticket ajoute uniquement le service C++ FGridWeaponVisualResolver, ses diagnostics et six tests automatisés. Aucun asset binaire, icône, mesh, WBP, acteur, réglage de combat ou système de sauvegarde n'est modifié.

Fichiers :
- Source/GrimrockPrototype/Public/Runtime/Combat/GridWeaponVisualResolver.h
- Source/GrimrockPrototype/Private/Runtime/Combat/GridWeaponVisualResolver.cpp
- Source/GrimrockPrototype/Private/Tests/GridARMEMENT0162WeaponVisualResolverTests.cpp

## Contrat de l'API

La méthode statique TryResolve(Definition, OptionalInstance, OutVisual, OutError) reçoit la UGridItemDefinitionAsset canonique et une FGridItemInstance facultative. Elle retourne des **références soft non chargées**.

- Retour true / OutVisual.bResolved=true : au moins une référence à une icône, WorldMesh ou EquippedMesh est disponible. Cela ne garantit pas encore l'existence de son fichier graphique sur disque.
- Retour false : aucune ressource graphique. MissingVisualResource est signalé si aucune autre erreur n'existe.
- bLegacyFallback=true : au moins une source visuelle provient des champs Icon, WorldMesh ou EquippedMesh de la définition historique, y compris le fallback total.
- OutError et OutVisual.Error : cause explicite conservée même lorsque le repli Legacy permet de retourner true.
- bHasItemLevel : seulement pour un ItemLevel 1..20 d'une instance réelle valide. Sans instance (preview) ou au niveau Legacy 0, aucun niveau n'est inventé.

## Ordre de résolution

1. Réinitialiser les sorties. Si une instance est fournie, vérifier son IsValid() et la concordance de ItemDefinitionId avec la définition ; sinon erreur InstanceContextUnavailable et fallback Legacy, sans faux badge de niveau.
2. Si l'opt-in bUseWeaponVisualCatalogue est inactif, recopier exactement Icon, WorldMesh et EquippedMesh Legacy ; sans recréer de mesh équipé et sans décoration inventée.
3. Si l'opt-in est actif, utiliser WeaponVisualCatalogue.Get() (**jamais LoadSynchronous**). Un catalogue absent/non chargé donne CatalogueUnavailable et repli Legacy.
4. Résoudre ArchetypeId → VisualId → FGridWeaponVisualProfile. Binding manquant = MissingBinding ; VisualId sans profil = UnknownVisualId ; structure invalide = MissingVisualResource.
5. Copier les références soft et transforms du profil. Si certains canaux sont absents, réutiliser les références Legacy correspondantes. EquippedMesh utilise d'abord le WorldMesh résolu lorsqu'aucune ressource équipée spécifique n'est authorée ; à défaut, utiliser le mesh équipé Legacy.
6. Si aucune ressource n'est disponible, signaler MissingVisualResource. Jamais d'archétype voisin, de texture devinée ou de calcul RPG dans ce service.

**Invariants :** aucune modification d'ItemLevel, RuntimeObjectId, quantité, poids, dommages, WeaponArchetypeId, rareté ou forge. Aucun Tick, FRand, actor spawn ou chargement synchrone.

**Limite volontaire :** une SoftObjectPath non vide et non chargée est seulement une référence structurellement présente ; sa disponibilité réelle sera testée avec les assets créés en 6.3. L'intégrateur devra précharger/cacher les graphismes à des transitions adaptées, jamais par frame d'UMG.

## Compatibilité

- Les propriétés ItemDefinitionAsset.Icon, WorldMesh et EquippedMesh restent en place. Les anciens call sites UMG et AGridItemActor ne sont **pas encore raccordés** au résolveur.
- Le monde et la preview sans instance de niveau connu n'affichent aucun niveau fictif.
- Aucun changement de Material Slots, collision, physique, projectile, hotbar ou Attack_Unarmed.
- DA_WeaponVisualCatalogue.uasset ne sera créé qu'en ARMEMENT01.6.3.

## Tests (6)

Filtre : **Grimrock.RPG.ARMEMENT01.6.Resolver**

1. SharedSilhouette : deux archétypes, une silhouette, vrai niveau d'instance, preview neutre.
2. LegacyExactFallback : icône et deux meshes historiques strictement conservés ; aucune ressource = échec explicite.
3. PartialResourceFallback : profils icon-only/mesh-only, sources Legacy par canal.
4. MissingCatalogueAndBindings : catalogue manquant/non chargé, binding et VisualId absents, sans chargement.
5. InvalidInstanceAndProfile : mauvaise identité, niveau hors bornes, profil incomplet.
6. ReadOnlyGameplay : double résolution déterministe ; identité, niveau, quantité, poids et définition inchangés.

## Validation UE5 locale

    cd D:\Development\GrimrockPrototype
    git pull --ff-only origin master
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.6.Resolver"
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock"

**Attendu après réussite :** 6/6 nouveaux tests et 1 482 tests globaux, zéro avertissement d'automation, zéro échec, exit 0. Ces chiffres ne valent pas preuve de validation sans les logs de l'utilisateur.

## Ticket suivant

ARMEMENT01.6.3 : trois silhouettes pilotes, icônes RGBA, StaticMeshes, DA_WeaponVisualCatalogue, contrôle graphique dans UE5. Puis ARMEMENT01.6.4 et 6.5 : raccordement aux affichages existants.
