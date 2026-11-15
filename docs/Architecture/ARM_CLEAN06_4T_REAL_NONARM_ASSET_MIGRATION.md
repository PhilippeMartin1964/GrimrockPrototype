# ARM-CLEAN06.4T — Première migration réelle des DataAssets non-ARM

## Décision : agir sur les vrais fichiers maintenant

Le propriétaire a explicitement demandé la **migration réelle** après la validation de 06.4S, commit `07c757485a929707bcaed9832ed5933ab409950d` (trois statuts Windows SUCCESS et issue [#152](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/152)). Git et Git LFS protègent les fichiers suivis. Nous n’ajoutons **aucune branche de fallback legacy au runtime**.

Ce jalon **utilise directement `SavePackage` d'Unreal Editor** pour écrire des nouveaux `DA_Canonical_*.uasset` dans `CanonicalStage/Definitions` et mettre à jour le **vrai** `DA_ItemSystem.uasset`. Le C++ nouveau est un outil Editor transitoire uniquement ; il lit les anciens `DA_Item_*` mais n'introduit aucune lecture supplémentaire au runtime. Les anciennes références monde et SaveGames ne sont **pas encore** commutées : ne pas les confondre avec les définitions enregistrées.

## Premier lot : source réelle, aucune perte volontaire

Entrées : les **11 identités non-ARM** du rapport de propriétés signé 06.4K, vérifiées sur les vrais assets UE. La sélection est déterministe et ne s'appuie pas sur un nom d'icône, un tag arbitraire ou une catégorie inventée.

**On convertit uniquement les définitions dont les actions sont réellement absentes** (`CombatActions` vide, pas de QuickItem ni de présentation d'attaque), et dont la famille métier est démontrable. Celles possédant une route d'action encore non approuvée ou des règles d'équipement/empilement incompatibles sont laissées intactes et comptées comme `deferred_with_unproven_semantics`.

Chaque objet converti reçoit une **classe C++ canonique concrète** : `UGridNonArmamentDefinitionAsset`, `UGridGenericStackableDefinitionAsset`, `UGridConsumableDefinitionAsset` ou `UGridOtherEquipmentDefinitionAsset`. Aucune création d'arme ni de `AmmoKindId` fictif. Les copies préservent exactement les valeurs authorées utiles : `ItemId`, `DisplayName`, `Description`, `WeightKg`, tags, icône, mesh monde, lecture, émetteur de lumière, Niagara persistant, sparkle, physique Chaos et lancer non-ARM. Les données d'équipement, de résistances et le `HandPolicy` explicite sont copiés sur les classes appropriées. Le booléen d'empilement historique est remplacé par **une seule capacité `MaxStack`** dans la bonne spécialisation.

L'outil `GridArmClean064TNonArmMaterialize` contrôle chaque source contre les exports privés 06.4K. Avant toute écriture, il construit et valide les nouvelles définitions en mémoire. Puis il sauvegarde chaque vraie définition et le registre. Dans un **nouveau processus Editor**, `-Verify` recharge les fichiers depuis le disque, reconstruit les projections sources et compare toutes leurs `FProperty` ; cette vérification ne modifie aucun package.

**Ce lot n'est pas une conversion silencieuse des 40 références de niveau.** Les références continuent de fonctionner selon le code actuel, en attendant un cutover unique et ses tests de non-régression. Les cas différés sont des données explicitement non migrées, pas des fallbacks de compatibilité ajoutés.

## Un seul commit Git final : code et binaires

Sujet déclenchant le travail via `UEValidationAgent.ps1` :

`feat(ARM-CLEAN06.4T): materialize real canonical nonarm items in UE Editor`

1. Publier le commit de code temporaire sur `master`, parent **06.4S**.
2. L'agent Windows valide le C++ et l'Automation `Grimrock`, puis charge les preuves 06.4K/06.4S et compare les 84 anciennes sources avec leur sauvegarde figée 06.4G.
3. Il exécute Unreal Editor `-run=GridArmClean064TNonArmMaterialize -Apply`. Aucun shell ou Git n'écrit artificiellement un `.uasset`.
4. Il refuse toute modification hors de la liste exacte produite par UE : **nouvelles définitions canoniques et `DA_ItemSystem.uasset` uniquement**. Il exige Git LFS sur chaque fichier.
5. Il relance un second Editor `-Verify` et la suite `Grimrock` complète sur les **binaires réellement resauvegardés**.
6. Après contrôle de `origin/master` et du parent exact 06.4S, l'agent **amende uniquement le commit de code qu'il vient de recevoir** avec les binaires et pousse par `--force-with-lease` contre le SHA qu'il vient de vérifier. C'est un seul commit final atteignable depuis `master`, pas un commit par asset.
7. Le nouveau SHA repasse automatiquement Build, Automation et `UE5.5.4/ARM-CLEAN06.4T` ; l'outil n'autorise **plus** `-Apply` : il n'exécute que `-Verify` sur les `.uasset` persistés. Les SHA et identités détaillées restent privés ; seuls les nombres figurent dans le rapport public.

La permission de remplacer le SHA intermédiaire est **strictement limitée au commit ChatGPT 06.4T**, avec parent fixé à 06.4S, sans réécriture de commit utilisateur.

## Périmètre exclu et legacy

- **Pas de nouveau legacy runtime.** Les outils Editor qui lisent les anciennes définitions existent temporairement pour convertir les données ; ils seront retirés en 06.5, pas conservés comme chemin d'exécution.
- **Ancien runtime encore présent** : le fait de matérialiser un DataAsset ne change pas encore les pointeurs hérités des niveaux, palettes, spawns et SaveGames. Leur conversion sera la suite du travail, et doit préserver GUID, états mutables, N0 et quantités.
- Les quatre actions non-ARM de 06.4R ne sont pas perdues ou inventées ; leurs propriétaires sont **différés** tant qu'il manque une route `ActionId/PresentationId` réellement exécutable.
- Les 84 backups précédents sont une référence d'origine et **n'autorisent pas** la réécriture aveugle de données utilisateur contemporaines. Les SaveGames personnels non versionnés n'ont pas été modifiés.
- Aucune modification WBP, aucun nouveau texte joueur. Localisation : `UI.csv → ST_UI → WBP → Game/fr/en` inchangé.

## Verdict et reprise

**Valider seulement après** `SilentTask`, `LocalAuto` et `ARM-CLEAN06.4T` SUCCESS **sur le SHA final contenant les fichiers binaires** et publication de l'issue agrégée. Si le commit ne contient pas de nouveau `.uasset` et `DA_ItemSystem.uasset`, le jalon est **NON ACHEVÉ**, même si son C++ compile.

Après cela : terminer les objets différés/actions, convertir les **40 références réelles des niveaux/palettes/réceptacles** via Editor, puis basculer l'exécution sans branche legacy et migrer SaveGame sur copie avec vérification ; 06.5 supprime l'ancien modèle, 06.6 teste PIE/Cook.
