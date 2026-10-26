# UI-TOOLTIP-CLEAN01 — Audit des anciens widgets et protection rechargeable

**Date : 26 octobre 2026. État : code + tests publiés sur master, validation locale et vérification Reference Viewer attendues.**

## Contrat fonctionnel — Bouclier rond ARM075

L'armure physique du bouclier est **une réserve, pas une durabilité**. Le profil de niveau N1 offre un **maximum nominal permanent de 5 points**. L'instance conserve séparément les points dépensés dans `AbsorbedEquipmentPhysicalArmor`. Après 3 points absorbés, elle affiche 2/5 ; une victoire normale ou un repos explicite hors combat restaure 5/5 pour un personnage vivant. Pas de recharge par Tick, tour, chargement SaveGame, victoire diagnostic, défaite ou abandon. Au niveau N12, le maximum vient du vrai profil N12 et non d'un « 5 » codé en dur.

UI-TOOLTIP-CLEAN01 rend cette différence explicite dans `FGridWeaponTooltipProjection::Build` :

| Info-bulle Bouclier rond N1 | Pendant le combat | Après recharge |
| --- | --- | --- |
| `Armure physique` (maximum du profil) | 5 | 5 |
| `Armure restante` (réserve de l'instance) | **2/5** | **5/5** |
| Simulation d'équipement d'une Grande Hache à deux mains, delta d'armure effective | **−2** | **−5** |

Les deux lignes du bouclier **ne s'additionnent pas** : elles représentent le maximum et le restant **de la même réserve**. La simulation globale est déjà fondée sur l'armure effective restante ; aucun calcul de bonus de bouclier doublé n'est autorisé. La ligne supplémentaire `WeaponPhysicalArmorRemaining` est uniquement visuelle, sans delta et uniquement sur le profil catalogue **passif à protection physique positive**. Son libellé réutilise la clé de localisation existante `Inventory.Tooltip.Global.Stat.CurrentPhysicalArmor` (« Armure restante »), et son icône utilise le fallback `Armor` : **aucune édition ST_UI ni UMG nécessaire**.

Un bouclier équipé peut donc perdre ses 5 points pendant une rencontre sans perdre son potentiel N1 pour les combats ultérieurs. Il n'est ni détruit ni définitivement dégradé.

## Audit des widgets historiques

L'ancien audit `UI-ASSET-CLEAN01` trouvait **un referencer entrant pour `WBP_ItemTooltipComparisonRow` : `WBP_ItemTooltip`**. Le rapport `UI-WBP-AUDIT02` du 19 octobre trouvait 45 widgets référencés sur 46 ; seul `WBP_ItemReadPanel` était candidat orphelin. Ces constats ne justifient **aucune suppression aveugle** de widget tooltip.

Nouveau test read-only `Grimrock.Editor.UITooltipClean01.ReferenceAudit` :
- `WBP_ItemTooltip` (racine actuelle) : **conserver** ;
- `Inventory/WBP_ItemTooltipNativeLine` (lignes statistiques et icônes) : **conserver**, modifié localement ;
- `Inventory/WBP_ItemTooltipNativeComparison` (groupes de comparaison) : **conserver**, modifié localement ;
- `WBP_ItemTooltipComparisonRow` : ancien widget, référencements à contrôler ;
- `WBP_ItemTooltipStatLine` : ancien widget, référencements à contrôler.

L'audit interroge les dépendances entrantes et sortantes de l'AssetRegistry, sans écrire aucun package. Rapport :

`Saved/Audits/UI-TOOLTIP-CLEAN01/UI-TOOLTIP-CLEAN01_Referencers.csv`

Classification : `KEEP_ACTIVE`, `KEEP_REFERENCED` ou `ZERO_REFERENCER_REVIEW`. Cette dernière n'autorise **pas** une suppression à elle seule.

### Recette manuelle UE — impérative avant suppression

1. Dans `WBP_ItemTooltip`, vérifier en **Class Defaults** que `WeaponLineWidgetClass` vaut `WBP_ItemTooltipNativeLine` et `ComparisonRowWidgetClass` vaut `WBP_ItemTooltipNativeComparison` ; conserver le parent C++ `UGridItemTooltipWidget`.
2. Dans `WBP_ItemTooltipNativeComparison`, vérifier le remplissage des conteneurs par les lignes natives.
3. Dans Content Browser, ouvrir **Reference Viewer** pour `WBP_ItemTooltipComparisonRow` et `WBP_ItemTooltipStatLine` : inspecter les dépendances hard/soft, les Class Defaults, les éventuels chargements dynamiques et la compatibilité avec les 18 icônes migrées.
4. Ne pas supprimer ces widgets dans ce jalon. Si certains ne servent réellement plus après rebranchement et smoke PIE, les supprimer dans un **ticket ultérieur séparé**, depuis Unreal Editor avec **Fix Up Redirectors**, pas via suppression de .uasset sur Git.
5. En PIE, confirmer 5/5 → 2/5 → 5/5 sur le Bouclier rond, les comparaisons Dague/Grande Hache, les icônes 48×48, les descriptions narratives, les deltas et l'absence de chevauchement.

La propriété obsolescente `VerticalBox_WeaponComparisons` est encore utilisée pour masquer/vider un conteneur de compatibilité ; ne pas retirer une propriété UMG sérialisée de manière aveugle.

## Validation UE5.5.4

Fermer Unreal Editor, puis :

~~~powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.TooltipClean01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.Editor.UITooltipClean01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.UI.WeaponTooltip04"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.UI.EquipmentTooltip043B"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.SHIELD02.2"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.SHIELD02.3"
git status --short
~~~

Nouveaux tests : **2 tests runtime de protection nominale/restante, victoire et comparaison de la Grande Hache ; 1 test éditeur de référencements.** Aucun test ne sera déclaré réussi avant les logs UE5.5.4 du poste Windows.

## Working tree

Le commit `8a7f1a547ffe389e3d015b01971fa303ca67e5d4` a publié le XLSX et les huit DataAssets narratifs. Ce jalon ne modifie aucun binaire. Les quatre fichiers locaux précédemment identifiés restent à préserver :
- `T_Icon_PA.uasset` ;
- `WBP_ItemTooltipNativeComparison.uasset` ;
- `WBP_ItemTooltipNativeLine.uasset` ;
- `ST_UI.uasset`.

Aucune suppression binaire, migration automatique de Blueprint, `git add -A` ou modification du mécanisme de combat. La suppression physique des anciens WBP sera décidée **après** les nouveaux rapports et contrôles manuels.
