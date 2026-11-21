# ARM-CLEAN06.5H.23 — Inventaire strict des lecteurs textuels des 48 anciennes définitions

## Pourquoi cette étape est nécessaire

Le groupe **post-retrait de la palette** comprend 53 packages : 48 anciennes définitions, 2 catalogues sources utilisés en authoring Editor, 2 Readables effectivement référencés par SaveGame v27 et 1 redirecteur ThrowingKnife. Les 48 identités sont couvertes par la racine canonique, et sept sauvegardes ont déjà passé la validation en mémoire. **Ce ne sont pas 53 fichiers à effacer aveuglément.**

Les lots H.21 et H.22 ont supprimé des chargements directs de vieux `DA_Item_*` dans plusieurs tests actifs (RatLoot, Alchimiste, Gobelin). D'autres outils et tests d'authoring utilisent encore les mêmes packages historiques.

Le rapport `Scripts/PlanARM_CLEAN06_5H12_Retirement.ps1` vérifie le périmètre et quelques motifs génériques. Le nouveau `Scripts/InspectARM_CLEAN06_5H23_HistoricalSources.ps1` établit une **correspondance nominative**, pour chaque ancien package, entre son `DA_Item_*/DA_Weapon_*` exact et les mentions dans `Source/`, `Scripts/` et `Config/`, avec chemin relatif, ligne et rôle (test Editor, test du jeu, outil Editor, source de production, script, configuration).

## Contrat de preuve

- Lecture seule de l'évidence native privée `ARM-CLEAN06.5A/retirement-evidence/v1`. Exige 53 candidats, dont 48 définitions, 48 identités canoniques, palette absente, zéro référent externe déclaré et **7/7** sauvegardes décodées / roundtrips corrects.
- Refuse un checkout sale, une branche différente de master, un `ExpectedHead` différent du HEAD local ou distant, un emplacement de sortie dans le dépôt, une sortie préexistante ou une identité non canonisée.
- Exécute une recherche statique bornée aux chemins de packages historiques exacts (expression de délimitation, pas de simple sous-chaîne) dans les fichiers C++/inl, PS, ini, cs. Identifie les tests dépendants sans donner un faux feu vert lorsqu'aucune mention n'est trouvée.
- Écrit exclusivement un JSON **privé, hors dépôt**, avec `deletion_authorized=false` et `evidence_not_bound_to_this_head=true`. Aucun asset, fichier de sauvegarde ou catalogue n'est modifié.
- Fixture `-SelfTest` négative sur le découpage exact des identifiants et distinction des catégories de sources, lancée par `ValidateUE.ps1` dans l'agent Windows. Les données personnelles ne sont pas présentes dans le worktree de validation.

## Exécution native de reprise (uniquement après validation Windows H.23)

Depuis un checkout local propre et synchronisé sur le SHA H.23 :

```powershell
cd D:\Development\GrimrockPrototype
.\Scripts\AuditARM_CLEAN06_5_Retirement.ps1 -ExpectedHead "<SHA-H23>"
```

L'audit produit une copie privée, un reçu des 7 SaveGames et `retirement-evidence.json` sous `D:\GrimrockNativeSnapshots\ARM_CLEAN06_5A_...`. Ensuite :

```powershell
.\Scripts\InspectARM_CLEAN06_5H23_HistoricalSources.ps1 `
  -ExpectedHead "<SHA-H23>" `
  -EvidencePath "D:\GrimrockNativeSnapshots\ARM_CLEAN06_5A_...\retirement-evidence.json" `
  -OutputPath "D:\GrimrockNativeSnapshots\h23-static-source-inventory.json"
```

**Le rapport est un inventaire, jamais une autorisation de suppression.** Une suppression nécessite encore un lot natif Editor distinct, des copies privées de tous les vrais fichiers concernés, les contrôles d'AssetRegistry / chemins sérialisés, la conversion de tous les consommateurs et un contre-audit des sept sauvegardes sur le SHA concerné. Le ciblage jaune/vert Shipping reste une tâche de présentation différée (#221), et la flaque d'huile est confirmée fonctionnelle dans le bon exécutable.
