# ARM-CLEAN06.5H.15 — Cohérence du snapshot natif et du canari historique

**Parent :** `0e7369ac266fd24d65e7af093e4a05c1955b629d`, validé `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` SUCCESS.

## Rapports fournis, examinés le 21 novembre 2026

Le nouvel audit `retirement-evidence(2).json` retrouve l'état pré-retrait : **54 packages**, dont 48 définitions historiques et une palette d'icônes `DA_EquippedWeaponActionIcons`. Pour cette palette : `referencer_query=true`, aucun référent AssetRegistry, y compris hard, soft ou extérieur. Les 7 sauvegardes v27 sont décodées sans erreur, `canonical_world_valid`, `memory_roundtrip_equal` et `cache_normalized_roundtrip_equal` sont tous à `true`. Il reste **deux références persistantes aux assets Readables** dans `GrimrockParty_2.sav`, qui sont exclus de toute suppression.

Le reçu `palette-copy-receipt.json` porte le schéma `ARM-CLEAN06.5H.14/native-palette-copy/v1`, le HEAD `0e7369ac266fd24d65e7af093e4a05c1955b629d`, le chemin `Content/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons.uasset`, la taille **4 340 octets**, et le SHA256 `3ac913df8d755a38db741c7c40aa1707990a5f0869554f6982521f0d6ccb2749`. Il déclare `source_unchanged=true` et `backup_byte_identical=true` ; le script H.15 les revérifiera **sur les vrais fichiers Windows**, sans prendre la seule déclaration du reçu pour preuve actuelle. Le binaire privé ne se trouve pas dans les pièces jointes de ce chat.

## Préflight H.15, strictement en lecture seule

Le script `Scripts/PreflightARM_CLEAN06_5H15_PaletteContinuity.ps1` relie les preuves produites précédemment aux octets natifs réellement disponibles sur Windows :

- refuse `master` sale, SHA local ou distant différent de `-ExpectedHead`, Editor ou jeu ouvert, et tous les chemins de données ou de sortie dans le dépôt ;
- exige un reçu H.14 complet, le vrai binaire `.uasset` non-LFS et sa copie dans le même dossier privé que le reçu ; compare taille et SHA256 des deux fichiers au reçu, puis revérifie que la source reste inchangée ;
- exige que le HEAD de la copie soit un **ancêtre Git** de la transaction actuelle (la copie peut avoir été effectuée sous H.14) ;
- vérifie les quatre modes de préflight, les 7 sauvegardes, la parité canonique, les 48 identités, la classification de 54 packages et **zéro référent AssetRegistry à la palette**, ainsi que deux Readables distincts dans les références persistées ;
- écrit exclusivement un **nouveau** reçu JSON hors dépôt avec empreintes du reçu H.14, du rapport d'audit et du vrai binaire ; `deletion_authorized=false`, `requires_fresh_transaction_sha_audit=true`. Aucune création de redirecteur ni de suppression de package.

Le self-test H.15 (intégré à `Scripts/ValidateUE.ps1`) simule une copie exacte, une copie altérée, un audit acceptable et un référent injecté sur le canari. Les octets synthétiques de ce test ne sont pas des vrais assets Unreal.

### Opération Windows après validation du SHA H.15

Avec Unreal Editor et le jeu fermés, à partir de `D:\Development\GrimrockPrototype` propre au dernier `master` :

```powershell
.\Scripts\PreflightARM_CLEAN06_5H15_PaletteContinuity.ps1 `
    -ExpectedHead "<SHA-H15-validé>" `
    -EvidencePath "D:\GrimrockNativeSnapshots\<dossier audit H14>\retirement-evidence.json" `
    -PaletteReceiptPath "D:\GrimrockNativeSnapshots\<dossier copie H14>\palette-copy-receipt.json" `
    -OutputPath "D:\GrimrockNativeSnapshots\palette-preflight-h15.json"
```

Les chemins `<dossier...>` ne sont **pas inventés** par le script : utiliser les vrais dossiers créés localement par H.14. Le chemin `-OutputPath` doit être nouveau, hors dépôt, sans préexister.

## Limites et décision

Le rapport `retirement-evidence/v1` ne contient pas le SHA du commit qui l'a généré. La preuve H.15 démontre la continuité de **la palette** via ses octets et son ancêtre Git ; elle ne peut pas garantir qu'un audit JSON non signé correspond exactement au HEAD courant. **Avant toute suppression**, relancer l'audit natif sur le SHA de la transaction et coupler son `copy-receipt.json` au rapport, puis effectuer la suppression seulement dans Unreal Editor avec un rollback natif testé, avant les contrôles PIE + Save/Continue + Cook/Shipping. Aucune suppression des 48 définitions historiques, des deux catalogues, des deux Readables ou du redirecteur du couteau n'entre dans ce lot.

**Correction préflight Windows du lot H.15 :** la première publication `14448aedeae8ee76f78bb9c9b994c6601588644b` a échoué à l'étape `PREFLIGHT` (issue GitHub #220) : `New-Item -LiteralPath` n'est pas un paramètre de `New-Item` sous Windows PowerShell. Le self-test crée désormais son répertoire temporaire avec `New-Item -Path` (nom aléatoire contrôlé, sans caractères génériques). Les gardes et la logique de conservation sont identiques. La révision remplace exclusivement le HEAD H.15 défaillant, sans réécrire les travaux utilisateur. Vérifier le double statut Windows du nouveau SHA.

**Statut : aucun retrait et aucune autorisation implicite.**
