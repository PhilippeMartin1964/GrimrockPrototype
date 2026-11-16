# ARM-CLEAN06.4X.16.8.3 — Préparation du migrateur de serrures (désarmé)

Les jalons X.16.8.1 et X.16.8.2 sont les prérequis :
audit des clés cuivre/fer et compatibilité des audits des réceptacles.
Le manifeste des vrais DataAssets provient de l'issue #176.

Le nouveau script `ApplyARM_CLEAN06_4X83_LockRules.ps1` est
dérivé de la transaction des réceptacles X.16.7.2F, déjà
validée sous UE5.5.4 et en PIE. Il cible **uniquement** :

- `Content/GrimrockPrototype/Core/DataAssets/DA_Lock_CopperWall.uasset`
- `Content/GrimrockPrototype/Core/DataAssets/DA_Lock_IronWall.uasset`

La source de vérité pour l'union des clés autorisées
reste `AGridWallLockActor`, qui combine
`AcceptedKeyItems` et `AcceptedKeyIds`.
Le commandlet `GridArmClean064X81LockRules` vérifie
que chaque ensemble reste respectivement
`{Key_Copper}` et `{Key_Iron}`.

Le script garde les protections de la transaction
réceptacle : worktree de validation propre, parent et
sujet Git exacts, SHA-256 des vrais objets Git LFS,
copies privées vérifiées, audit avant écriture,
Apply limité aux deux packages, Verify dans un autre
processus Unreal, suite Grimrock après écriture, contrôle
du master distant et commit/push enfant contenant
exactement les deux binaires autorisés.

**Ce jalon est désarmé** : le sujet Git attendu
par le script est une sentinelle non publiée,
`NOT_ARMED_X83_MIGRATION_TRIGGER`. Le script n'est
appelé par aucune validation automatique sur ce SHA.
Les fichiers natifs et les sauvegardes restent inchangés.

Un commit ultérieur remplacera la sentinelle par
un couple SHA parent/sujet exact et reliera
le seul déclencheur `ValidateUE.ps1` validé.
Le commit binaire final devra réussir ses deux
contrôles UE5.5.4 et une recette PIE des
serrures avant clôture.
