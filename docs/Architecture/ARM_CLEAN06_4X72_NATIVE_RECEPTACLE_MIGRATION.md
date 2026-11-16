# ARM-CLEAN06.4X.16.7.2 — Transaction binaire sûre des deux réceptacles

Parent validé : 2dc6348b9ff3fd7df85c932f2bd6b8b3b62d7b16
(mode Audit du migrateur, SilentTask et LocalAuto SUCCESS).

Ce commit ajoute les protections et déclenche une
transaction **uniquement après** un nouveau build
et l'ensemble de la suite Grimrock validés.
La transaction s'exécute dans le worktree dédié
GrimrockPrototype_UEValidation, jamais dans le
checkout de développement de l'utilisateur.

Fichiers autorisés :
- Content/GrimrockPrototype/Core/DataAssets/DA_Receptacle_TorchHolder.uasset
- Content/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Guardian.uasset

Seuls les CanonicalItemId sont ajoutés,
Item_Torch et Gem_Blue. Aucune autre propriété
d'énigme, de torche, de support, de clé ni
de niveau ne doit être volontairement modifiée.

Garde obligatoire :
1. Parent exact, sujet Git exact, worktree propre,
   liste blanche de quatre fichiers code et deux binaires.
2. Hydratation Git LFS limitée à ces deux sources,
   vérification du hash SHA-256 et de la taille
   à partir des pointeurs immuables du commit HEAD,
   plus validation de la signature binaire Unreal.
3. Deux copies privées contrôlées par SHA-256,
   conservées dans la zone Recovery de l'agent.
4. Commandlet -Audit sans modification, puis
   -Apply, vérification stricte des deux seuls
   fichiers modifiés, -Verify dans un autre
   processus UE, et suite complète Grimrock.
5. Contrôle de la référence remote master avant
   d'ajouter exactement deux fichiers, création
   d'un commit binaire enfant par avance rapide.
6. Nouveau cycle UE5.5.4 sur le SHA binaire final,
   puis recette PIE TorchHolder et Guardian.

Un statut dédié UE5.5.4/ARM-CLEAN06.4X72
signale le résultat avec phase contrôlée.
Si une phase échoue avant le commit,
les originaux privés sont restaurés avec
vérification d'empreinte. Après commit mais
avant push, les backups demeurent disponibles ;
aucune restauration risquée n'est tentée.

Les serrures et les SaveGames restent
strictement hors de portée. Ne pas relancer
les migrations de 95 placements physiques.
