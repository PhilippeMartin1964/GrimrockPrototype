# ARM-CLEAN06.4X.16.8.1 — Migrateur de serrures, AUDIT uniquement

Précondition vérifiée : 377d36ee4784641a456e5bbddbc1fcaff358810e est validé
par SilentTask et LocalAuto. Les deux réceptacles ont passé la recette PIE utilisateur.

Identités exactes confirmées par l'issue #176 :
- DA_Lock_CopperWall : DefinitionId Lock_CopperWallLock, AcceptedKeyItems[0]=Key_Copper
- DA_Lock_IronWall : DefinitionId Lock_IronWallLock, AcceptedKeyItems[0]=Key_Iron

Le commandlet GridArmClean064X81LockRules comporte Audit/Apply/Verify,
mutuellement exclusifs. **La suite automatisée de ce commit n'utilise
que Audit**. Il vérifie 44 définitions, 4 niveaux, les 2 réceptacles
déjà canoniques, 2 listes de serrures, 2 pointeurs et 0 override.
Une liste complète de clés est calculée avant toute écriture,
via GridEditorLockKeyMigrationPreview ; elle doit rester strictement
identique après ajout du Canonical ItemId à AcceptedKeyIds.

Après migration (jalon ultérieur) l'audit acceptera exactement 2
AcceptedKeyIds, un par serrure ; le mode Apply refusera ce cas et le
mode Verify l'exigera. Les pointeurs historiques sont conservés ;
bStartsUnlocked, bConsumeKeyOnUnlock, les messages et les liens d'énigme
ne sont pas modifiés.

Le mode Apply n'est PAS appelé à cette étape. L'écriture des deux .uasset
exigera une transaction indépendante avec manifestes Git LFS, copies
SHA-256, deuxième processus Unreal, tests et validation du SHA binaire
final. Aucun fichier binaire/SaveGame/WBP/map n'est modifié ici.
