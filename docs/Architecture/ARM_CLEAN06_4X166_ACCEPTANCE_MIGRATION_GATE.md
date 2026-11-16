# ARM-CLEAN06.4X.16.6 — Porte de conversion des règles réelles

## État de preuve

Les commits 50e9f628 (X.16.4, réceptacles) et f12487b6
(X.16.5, serrures) sont validés par SilentTask et LocalAuto
sous UE 5.5.4. Leurs préflights lisent les quatre vrais
niveaux et les vraies définitions mais n'enregistrent
aucun package binaire. Les détails chiffrés des tests
restent uniquement dans le rapport privé de l'agent Windows.

## Extrait de preuve en lecture seule

Pour ne pas transmettre un journal complet contenant
éventuellement des chemins privés, exécuter dans PowerShell
les commandes suivantes et ne communiquer que leurs deux
lignes de synthèse :

    $root = Join-Path $env:LOCALAPPDATA 'GrimrockPrototype\UEValidationAgent\Runs'
    $run = Get-ChildItem -LiteralPath $root -Directory | Where-Object { $_.Name -like 'f12487b61dd0-*' } | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
    if (-not $run) { throw 'Run X.16.5 introuvable; aucune conversion permise.' }
    $log = Get-ChildItem -LiteralPath $run.FullName -File -Recurse | Where-Object { $_.Name -in @('Automation.log','Automation.console.log') } | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
    if (-not $log) { throw 'Journal Automation introuvable; aucune conversion permise.' }
    $lines = Select-String -LiteralPath $log.FullName -Pattern '\[ARM-CLEAN06\.4X\.16\.(4|5)\]' | ForEach-Object { ([regex]::Match($_.Line, '\[ARM-CLEAN06\.4X\.16\.(4|5)\][^\r\n]*')).Value }
    $lines | Where-Object { $_ } | Select-Object -Unique

La commande ne modifie aucun fichier. Si moins de deux
lignes sont trouvées, ne pas inventer les décomptes et
retrouver la preuve complète avant toute conversion.

## Conditions bloquantes

Réceptacles : toutes les listes de définition et
d'instance doivent être sûres ; les compteurs missing,
unknownLegacy, unknownCanonical et contradictory doivent
être nuls. Garder ordre des règles, bAcceptAnyItem,
MaxContainedItems, GUID, liens et priorité des surcharges.

Serrures : safeDefault doit égaler defaultKeyLists,
safeInstance doit égaler instanceKeyLists et les
missingPointers/unknownPointers doivent être nuls.
Ne jamais modifier les AcceptedKeyIds explicites
narratifs même lorsqu'ils sont absents du ItemSystem.
Conserver bConsumeKeyOnUnlock, bStartsUnlocked, textes
et comportement de déverrouillage.

## Transaction binaire future (NON exécutée)

Avant toute écriture des vrais DataAssets :
1. Identifier et limiter exactement les fichiers concernés.
2. Vérifier le worktree master et tous les contenus Git LFS.
3. Sauvegarder les fichiers d'origine avec empreintes SHA-256.
4. Appliquer seulement les champs canoniques permis,
   en gardant les anciens pointeurs pendant le cutover.
5. Fermer et rouvrir les packages dans un second processus UE,
   vérifier identités et ensembles de règles inchangés.
6. Refaire les tests UE, les essais PIE et Save/Continue sur
   copies de sauvegardes, puis valider le SHA final.
7. Publier en commits séparés les réceptacles puis serrures.

Aucune sauvegarde joueur, .uasset, .umap ou Blueprint n'a
été modifiée par cette note ni par X.16.4/X.16.5.
