# ARMEMENT01.11.1 — Préflight des quatre armes de mêlée N1

Date : 24 octobre 2026. **Statut : code de test proposé sur master ; validation UE locale et rapport non encore reçus.**

## Pourquoi ce ticket existe

Le rapport local \`TD04-20261024-225221\` contient \`No automation tests matched 'Grimrock.RPG.ARMEMENT01.11.1'\`. UE avait découvert 6245 tests, mais le filtre n'existait pas encore. Le code de sortie 255 n'indiquait pas un échec de combat ou un plantage d'Unreal : aucun test sous ce filtre n'avait été publié.

Ce ticket ajoute **deux tests Automation Editor** en lecture seule pour la prochaine vague d'armes de mêlée. Il ne crée ni DataAsset, ni mesh, ni icône, ni matériau. Il ne change aucune donnée du catalogue et ne retouche pas l'infobulle validée.

## Autorités et périmètre

- Classement, dés, caractéristiques, main et poids : \`docs/Design/ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_ARMEMENT01_2.xlsx\`, projetés dans \`DA_WeaponCatalogue\`.
- Visualisation : \`DA_WeaponVisualCatalogue\` (mapping existant, icône et WorldMesh effectivement chargeables).
- Kits : \`docs/Design/ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv\`, état \`ApprovedDesign\`.
- Definitions : \`UGridItemDefinitionAsset\` éventuellement présent dans \`/Game/GrimrockPrototype/Core/DataAssets/Weapons\`.

| Archétype | Désignation Excel | Recette approuvée N1 | Slot |
|---|---|---|---|
| ARM002 | Poignard d'assassin | Rogue / Assassin | MainHand |
| ARM003 | Stylet | Rogue / Saboteur | MainHand |
| ARM013 | Main-gauche | Rogue / Shadow | OffHand |
| ARM020 | Masse d'armes | Priest / Protection | MainHand |

Aucune statistique n'est recopiée dans le code. Les quatre armes sont des profils offensifs à une main au niveau 1 ; le test vérifie ce contrat dans le vrai catalogue.

## Tests fournis

- \`Grimrock.RPG.ARMEMENT01.11.1.ApprovedN1Sources\` : lit le manifeste approuvé, contrôle les quatre associations classe/branche/slot au niveau 1, le vrai profil N1, sa validité, l'attaque dérivée du catalogue et l'affinité principale ou secondaire non qualifiée.
- \`Grimrock.RPG.ARMEMENT01.11.1.MeleeAssetReadiness\` : interroge les deux catalogues et l'Asset Registry ; relève la présence du binding visuel, le chargement de l'icône et du WorldMesh, le nombre de définitions, la validité d'un item présent et la disponibilité des attaques par main. Écrit **quatre lignes factuelles** dans \`Saved/ARMEMENT01/ARMEMENT01_11_1_MeleeReadiness.csv\`.

Les absences encore normales sont **informatives** : \`MissingItemAndVisual\`, \`MissingItem\`, \`MissingVisual\`, \`NeedsOffHandActionReview\`, \`NeedsSpecialBehaviorReview\`, \`CandidateForPlaytest\`. Un doublon, une définition déjà présente mais invalide, un profil source invalide ou une impossibilité d'écrire le rapport échoue. \`CandidateForPlaytest\` **ne signifie jamais ProductionReady**.

### Point spécial ARM013 : séparation des deux contrats

Le runtime actuel \`UGridItemDefinitionAsset::CanEquipToSlot(OffHand)\` accepte une arme à une main, mais \`CanProvideAttackFromSlot(OffHand)\` exige en plus que \`CompatibleEquipmentSlots\` contienne explicitement \`OffHand\` et qu'une attaque d'équipement valide existe. Le préflight remonte les deux états sans modifier la règle ni fabriquer une parade.

Le libellé Excel « Parade en main secondaire » n'atteste pas qu'une action de parade soit déjà implémentée. Même avec \`OffHandAttack=true\`, il faudra vérifier **séparément** la parade et son éventuelle activation. Même principe pour « Critique conditionnel depuis l'ombre » (ARM002) et « Perforation ciblée » (ARM003) : aucun effet ni talent N2 inventé.

## Validation locale UE5.5.4 requise

Avec Unreal Editor fermé :

~~~powershell
cd D:\Development\GrimrockPrototype
git branch --show-current
git status --short --untracked-files=all

# Mettre à jour seulement après vérification des changements locaux.
git pull --ff-only origin master

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.ARMEMENT01.11.1"

Get-Content .\Saved\ARMEMENT01\ARMEMENT01_11_1_MeleeReadiness.csv

# Régression après validation du nouveau filtre
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.ARMEMENT01.10"
~~~

**Attendu non attesté** : compilation Development Editor, 2/2 tests, zéro avertissement et zéro échec ; rapport avec une ligne par arme. Le rapport TD04 local utilisateur reste l'unique preuve d'exécution. Ne pas confondre le verdict du test (audit exécuté) avec la disponibilité des armes (statuts CSV).

## Séquence après validation

1. Production graphique **ARM002** : vrai modèle 3D, textures et icône validée à 48×48 ; puis mapping visuel et DataAsset N1 dans des tickets atomiques séparés.
2. Même parcours pour **ARM003**, **ARM013**, puis **ARM020**.
3. Tests d'inventaire/équipement/attaque, avec exigences propres à ARM013 (main secondaire et parade), ARM002 (condition critique) et ARM003 (perforation ciblée).
4. Recette PIE : ramassage, tooltip, MainHand/OffHand, dégât N1, refus hors portée ou PA insuffisants, dépôt, sauvegarde/rechargement. ARM020 doit être essayé avec le bouclier pilote ARM075.

Ne pas activer les 18 kits du wizard pendant cette fabrication. Ne pas toucher au système d'infobulle, aux 12 kits legacy, à Shuriken/ThrowingKnife, aux fichiers UMG ou aux \`.uasset\` utilisateur.
