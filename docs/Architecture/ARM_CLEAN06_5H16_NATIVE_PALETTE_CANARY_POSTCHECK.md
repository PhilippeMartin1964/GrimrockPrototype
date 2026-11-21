# ARM-CLEAN06.5H.16 — Retrait natif isolé de la palette et contre-audit

**Parent :** `fff35f0d2c72f5c79f8fa3326951ac0f3d83f231` — contrôles Windows `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` SUCCESS.

## Preuve d'entrée

Le reçu opérateur `palette-preflight-H15-20261121-134832.json` annonce :
- `head=fff35f0d2c72f5c79f8fa3326951ac0f3d83f231` ; backup créé sous `0e7369ac266fd24d65e7af093e4a05c1955b629d` ;
- véritable `DA_EquippedWeaponActionIcons.uasset` : 4 340 octets, SHA256 `3ac913df8d755a38db741c7c40aa1707990a5f0869554f6982521f0d6ccb2749` ;
- `snapshot_still_byte_identical=true`, `asset_registry_preflight_zero_referrers=true`, `seven_saves_roundtrip_valid=true` ;
- **`deletion_authorized=false`** et **`requires_fresh_transaction_sha_audit=true`**.

Un rapport de preuve est un préflight et **ne signifie pas que la palette a été supprimée**.

## Procédure contrôlée en trois étapes

### 1. Audit frais AVANT la suppression

Sur le SHA **H.16 validé** et le checkout développeur `master` propre, Unreal Editor et jeu fermés :

```powershell
.\Scripts\AuditARM_CLEAN06_5_Retirement.ps1 -ExpectedHead "<SHA-H16>"
```

Ce script historique crée, dans `D:\GrimrockNativeSnapshots\ARM_CLEAN06_5A_<sha12>_<identifiant>\`, un `retirement-evidence.json` frais avec **54 candidats**, un `copy-receipt.json` dont le champ `head` lie les sept sauvegardes copiées au SHA H.16, et leur dossier `AuditCopies`. Conserver ce dossier intact : il est la base privée immuable des deux audits.

### 2. Action manuelle UNIQUE via Unreal Editor

Dans le Content Browser, ouvrir `/Game/GrimrockPrototype/UI/Icons/Combat`. Sélectionner **exclusivement** `DA_EquippedWeaponActionIcons` puis choisir **Delete**. Si Unreal détecte un référent inattendu, exige `Force Delete`, demande de modifier/sauvegarder d'autres assets ou affiche une anomalie, **annuler** et préserver la palette. Ne pas employer Explorer, `git rm`, suppression de dossier, `Fix Up Redirectors` global ou scripts de nettoyage. Fermer Unreal Editor avant le contre-audit ; ne PAS committer ni pousser.

### 3. Contre-audit post-retrait AVANT commit

```powershell
.\Scripts\VerifyARM_CLEAN06_5H16_Canary.ps1 `
    -ExpectedHead "<SHA-H16>" `
    -FreshEvidencePath "<DOSSIER_AUDIT_H16>\retirement-evidence.json" `
    -CopyReceiptPath "<DOSSIER_AUDIT_H16>\copy-receipt.json" `
    -PaletteReceiptPath "<DOSSIER_SNAPSHOT_H14>\palette-copy-receipt.json" `
    -OutputPath "D:\GrimrockNativeSnapshots\h16-canary-postcheck.json"
```

Nouveau script `VerifyARM_CLEAN06_5H16_Canary.ps1` :
- refuse si HEAD local/distant divergent, si UE/jeu ouvert ou si le Git diff n'est **pas exactement** la seule suppression **non indexée** du fichier binaire de palette, sans autre modification ni fichier non suivi ;
- vérifie que les rapports `retirement-evidence.json` et `copy-receipt.json` sont issus **du même** audit frais sous le SHA H.16 ; valide les rôles des 54 candidats et toutes les preuves canoniques ;
- vérifie le vrai fichier de snapshot H.14 privé de **4 340 octets** et son empreinte SHA256, ainsi que les sept sauvegardes live et AuditCopies par rapport au reçu de copies fraîches ;
- exécute `UnrealEditor-Cmd.exe -run=GridArmClean065RetirementAudit -ReadOnly` sur ces copies : exige **53** packages seulement, rôle `historical_action_icon_palette=0`, 48 anciens DataAssets, 2 catalogues historiques, 2 Readables et 1 redirecteur ; contrôle 7/7 parités ;
- vérifie de nouveau le Git diff et l'inventaire des sauvegardes et écrit deux fichiers privés JSON hors dépôt, les preuves brutes du contre-audit et le reçu `post-retirement-read-only/v1` portant `deletion_authorized=false`, `commit_and_push_not_performed=true`.

Le script n'efface rien, ne restaure rien et n'autorise pas automatiquement le commit ; en cas d'échec, conserver la copie privée et restaurer exclusivement dans des conditions sûres (Editor fermé, branche et source vérifiés) plutôt que masquer l'échec.

## Validation du lot

Le self-test `-SelfTest` fonctionne sur un schéma synthétique : état attendu **54**, état autorisé **53**, refus d'un référent externe et refus d'un roundtrip SaveGame incorrect. Il s'exécute dans le préflight Windows `ValidateUE.ps1`, sans toucher aux assets et aux vraies sauvegardes.

**Intervention native encore requise** : créer l'audit H.16 frais, supprimer la palette dans Unreal Editor puis fournir le JSON postcheck. Un commit de suppression doit être **une transaction séparée** et ne pourra être publié qu'après contre-audit réussi, contrôle du fichier Git LFS et vérification PIE, Save/Continue et Cook/Shipping selon la politique du projet. Aucun autre asset n'est concerné.
