# ARM-CLEAN06.4X.62 — preuve native et projection d'alchimie

Parent binaire : `4cdde5e4dbe670de0bb95efa30f36167c5a56c62` (X.61B, SilentTask + LocalAuto SUCCESS, 1997 tests réussis dont 31 avec avertissements ; recette éditeur/PIE 8/8 validée). Les 107 références du périmètre X.48 sont supprimées, registre de 38 définitions.

## Périmètre réel

Dix identités hors registre : Item_Antidote, Item_Bomb_Fire, Item_Bomb_Toxic, Item_DefensiveElixir_Fire, Item_DefensiveElixir_Ice, Item_DefensiveElixir_Lightning, Item_DefensiveElixir_Poison, Item_Flask_Acid, Item_Flask_CorrosiveCloud, Item_Panacea. Item_Flask_Oil (déjà canonique) est le onzième témoin, jamais un nouveau doublon à créer.

L'absence de référent *package* dans l'audit X.19 ne signifie pas absence d'usage. Le C++ emploie notamment
- `RPGStartingEquipmentPresetCatalog.cpp` : le kit approuvé Grenadier contient DA_Item_Bomb_Fire, et le kit Transmuter contient DA_Item_Flask_Oil ;
- `RPGAlchemistAuthoring.cpp` : les onze QuickItems, les Recipe_* et les statuts ; AuthorProductionAssets appelle FindOrCreateItem/ConfigureItem sur les onze, y compris Oil déjà canonique ;
- `GridWeaponFireBombPalettePlan.h` et `RPGStartingSurfaceCatalystAuthoring.cpp` : chemins source de FireBomb, à traiter dans le lot de retrait des outils anciens ;
- `RPGRPG0310GlobalBalanceTests.cpp` et tests RPG03.9.6 : économie/action/statuts, à conserver en migrant leurs preuves de gameplay vers le registre natif.

Les quatre élixirs partagent dans l'authoring C++ `Action_Alchemist_DefensiveElixir` mais appliquent quatre statuts distincts. Les vraies valeurs sérialisées doivent être lues avant de statuer sur les collisions. `UGridActionCatalogueAsset::Validate` refuse deux ActionId égaux, même sur les détails non-ARM. Aucune insertion des quatre sous un même identifiant ni réutilisation arbitraire du premier effet. Les bindings SaveGame v27 ont notamment ActionId et SourceDefinitionId : une future conversion des identifiants d'action doit pouvoir distinguer le véritable objet source, préserver les états/cooldowns et ne pas casser un raccourci. Aucune renumérotation dans ce lot.

## Lot X.62 (lecture seule)

`GridEditorAlchemyMigrationAudit.h` charge uniquement les onze chemins exacts via FRPGAlchemistAuthoring. Exige root réel valide de 38 et absence de ces dix ItemId ; Oil seul déjà enregistré. Lit les propriétés natives, reconstruit la véritable action effective par BuildQuickItemCombatActionDefinition (tags, coût d'unité, texte et icône fallback historiques inclus). Compare toutes les propriétés réfléchies de cette action, identifie collisions inter-items et catalogue, et relève les différences avec l'action canonique existante d'Oil.

Chaque candidat non-ARM est projeté **isolément** dans une copie transitoire du root et des deux catalogues d'action/présentation : FText originaux, poids, pile, tags, mesh, icon, ReadText, config lumière/VFX/sparkle, physique et GenericThrow par copies exactes du précédent lot. Aucun changement de l'ActionId ou de la structure d'action effective. Un profil sans présentation utilise seulement le lien de test au profil no-FX existant ; toute présentation détaillée active est copiée dans le catalogue transitoire ou une collision est explicitement signalée. Valide le registre UE, résout l'identité N0, son action détaillée et les métadonnées comparées. Un candidat non prouvé reçoit une raison, sans suppression d'information. Une parité individuelle **n'est pas** une preuve de coexistence des onze, d'intégration en combat, ni de capacité de sauvegarde. La présentation, les catégories UI et l'exécution (ciblage Ally/Hostile/Area, surfaces, coût PA, effets, tags, économie, statut/cooldown, UI/hotbar) restent des gates de migration finale. Le mapping Potion de présentation et les anciens liens de kit/authoring doivent être examinés avant Apply.

`GridArmClean064X62AlchemyAudit` : entrée strictement `-ReadOnly` avec fichier JSON absolu neuf hors projet ; refuse Apply/Publish. Registry.SearchAllAssets et dépendances/référents package, action effective et source réfléchies, références explicites de statuts et présence/validité des vrais assets, statut des projections, groupes d'identité. Les tags de retrait de statut demeurent dans le manifeste complet ; une absence de référent package ne couvre pas les lecteurs C++/SaveGame. Aucun SavePackage, aucune écriture native, aucune lecture/écriture de SaveGame joueur. Les catalogues et sources restent propres.

Test `Grimrock.RPG.ARM.CLEAN06_4X62.NativeAlchemyEvidenceReadOnly` : couverture exacte 10+1, actions répétables, collision exhaustive, mutation de PA détectée, rejet catalogue de deux détails au même ID, absence de dirty packages et de modification du root. Un rapport complet peut signaler des obstacles (test de collecte réussi ≠ migration réussie).

## Preuve opérateur à recevoir

Après les deux statuts Windows du SHA code, sur le checkout master propre à jour, fermer UE, exécuter le commandlet en lecture seule :

~~~powershell
cd D:\Development\GrimrockPrototype
$Dir = 'D:\GrimrockNativeSnapshots\ARM_CLEAN06_4X62'
New-Item -ItemType Directory -Path $Dir -Force | Out-Null
$Report = Join-Path $Dir ('AlchemyEvidence-'+(Get-Date -Format 'yyyyMMdd-HHmmssfff')+'.json')
& 'D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' "$PWD\GrimrockPrototype.uproject" -run=GridArmClean064X62AlchemyAudit -ReadOnly "-Output=$Report" -Unattended -NoP4 -NoSplash -NoSound -NullRHI
if ($LASTEXITCODE -ne 0) { throw "Audit failed: $LASTEXITCODE" }
Write-Host $Report
git status --short
~~~

Le JSON exporté est le livrable attendu, pas un nouveau commit binaire. Pas de nouvelle recette PIE pour cette lecture seule. Aucun Apply autorisé ici. Compléter ensuite le plan des dix définitions et actions uniques sur la preuve réelle ; prévoir le traitement explicite des anciens bindings, les outils et les tests natifs, et seulement ensuite une transaction protégée. Aucun nouveau texte joueur, UI.csv/ST_UI/WBP/Game-fr/en non modifiés. **X.62 n'enregistre pas encore les dix objets : 38 définitions, 107/107 références du périmètre X.48. ARM-CLEAN06.4 partielle ; 06.5/06.6 non clôturées.**
