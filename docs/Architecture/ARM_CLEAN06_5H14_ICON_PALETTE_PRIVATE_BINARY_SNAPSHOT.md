# ARM-CLEAN06.5H.14 — Snapshot privé de la palette native avant transaction

**Parent :** `ec0344291de1960eac90f5c4970b1ed91a1c9332` (UE5.5.4/SilentTask et UE5.5.4/LocalAuto SUCCESS).

Le premier candidat potentiel à un retrait isolé est `/Game/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons`. Le rapport 5A/v1 transmis le 21.11 confirme actuellement zéro référent AssetRegistry au package ; H.2 a neutralisé son chargement direct par les tests 06.2 ; H.13 a rendu les gardes natifs compatibles avec son absence **et sa présence**. **L'asset n'est pas encore supprimé.**

## Pourquoi une copie spécifique ?

Le fichier GitHub `Content/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons.uasset` est représenté par un **pointeur Git LFS de 129 octets**. Copier ce pointeur serait une fausse sauvegarde : seule l'installation locale contenant le véritable binaire Unreal peut fournir un snapshot fiable.

`Scripts/PrepareARM_CLEAN06_5H14_IconPaletteSnapshot.ps1` :
- refuse tout SHA différent du `master` local et distant, un dépôt sale, une palette non suivie par Git, un processus UE/jeu en cours, un état privé intérieur au dépôt et un chemin symlink/reparse ;
- exige un vrai fichier `.uasset` de plus de 200 octets, et rejette explicitement l'en-tête d'un pointeur Git LFS ;
- produit **une copie privée neuve**, exclusivement hors dépôt, avec contrôles SHA256 et taille avant/après, et vérifie l'original inchangé ;
- écrit un reçu JSON privé qui comporte **`deletion_authorized=false`**. La seule opération sur l'asset source est sa lecture.

L'autotest `-SelfTest` travaille dans un dossier temporaire sur des octets synthétiques et prouve la détection d'une copie altérée et le refus d'un pointeur LFS. Ce test est intégré au préflight `ValidateUE.ps1` de l'agent Windows. Il ne lit aucun asset ni sauvegarde réel de l'utilisateur.

## Frontière de l'autonomie distante

La copie native n'est **pas** réalisée par la CI ou cette modification Git : elle exige le checkout local, les vrais octets LFS et la fermeture du jeu. La commande **après validation H.14** est :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
.\Scripts\PrepareARM_CLEAN06_5H14_IconPaletteSnapshot.ps1 -ExpectedHead "<SHA-H14-validé>"
```

Elle n'efface rien. Pour un retrait ultérieur, exiger en plus un audit natif renouvelé au SHA de la transaction, vérification des sources/tests, suppression via Unreal Editor, nouvel audit natif (53 candidats stricts), nouvelle validation Windows, PIE, Save/Continue et Cook/Shipping. Ne pas supprimer les 48 définitions anciennes en même temps.

**Statut :** préparation seulement. Aucun fichier `.uasset`, `.umap`, `.sav` ni binaire LFS n'a été modifié.
