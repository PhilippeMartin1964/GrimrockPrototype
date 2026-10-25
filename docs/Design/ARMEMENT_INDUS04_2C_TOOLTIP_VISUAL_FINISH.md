# ARMEMENT-INDUS04.2C — Finition visuelle du tooltip (UMG Designer)

**État : contrat Designer publié, micro-correction native de l'encombrement des icônes. Vérification locale UE5.5.4 et captures PIE requises.**

## Périmètre

L'affichage de la dague et des comparaisons fonctionne déjà en PIE (preuve : capture utilisateur du 25 octobre 2026). Il reste trois défauts exclusivement de présentation : libellés/valeurs collés, colonnes de comparaison illisibles, et forte hauteur des lignes sans icône. **Ne pas recréer de branche, de boucle ou de `SetText` dans les Graphs**. Ne pas éditer directement les binaires WBP depuis GitHub.

`UGridItemTooltipWeaponLineWidget` présente toujours sept TextBlocks dynamiques : `Text_Label`, `Text_Value`, `Text_Equipped`, `Text_Slot`, `Text_DeltaPositive`, `Text_DeltaNegative`, `Text_DeltaNeutral`, plus `Image_StatIcon`. Les propriétés de couleur, police, brosse et alignement appartiennent exclusivement aux widgets Designer. Les trois deltas sont mutuellement exclusifs par le C++.

### Micro-correction C++ de compacité

Le widget natif lie désormais **facultativement** `SizeBox_StatIcon` (nom exact présent dans `WBP_ItemTooltipNativeLine`). Lorsque `StatIconTextures[StatId]` est absent ou invalide, le C++ met **le conteneur 48x48 lui-même** en `Collapsed`, et pas seulement `Image_StatIcon` en `Hidden`. La ligne sans icône retrouve une hauteur de texte compacte ; les lignes possédant une icône conservent une vraie icône 48x48. Si le SizeBox n'existe pas dans un autre WBP, le binding reste optionnel. Aucun calcul métier, formatage ou style n'a été modifié.

## 1. Dans `WBP_ItemTooltipNativeLine`

Conserver le parent `GridItemTooltipWeaponLineWidget` et les noms existants. Dans le Designer, réorganiser les widgets en colonnes alignées sur un `HorizontalBox_Line` qui remplit la largeur de son parent.

| Conteneur (nouveau si utile) | Enfant existant | Largeur suggérée | Placement / comportement |
|---|---|---:|---|
| `SizeBox_StatIcon` | `Image_StatIcon` | **48 px**, hauteur 48 px | icône 48x48 sans déformation ; conteneur `Collapsed` en C++ si absente |
| `SizeBox_Label` | `Text_Label` | **165 px** | aligné à gauche, Auto Wrap si nécessaire |
| `SizeBox_Value` | `Text_Value` | **130 px** | aligné à droite, Auto Wrap pour les valeurs longues |
| `SizeBox_Equipped` | `Text_Equipped` | **130 px** | aligné à droite, Auto Wrap |
| `SizeBox_Delta` | `Overlay_Delta` | **64 px** | aligne à droite les trois `Text_Delta*` superposés |

Le `Text_Slot` facultatif reste dans le WBP avec son nom exact, même s'il est généralement vide quand une comparaison a déjà un titre de slot au-dessus.

La largeur théorique sans espacements est 537 px ; ajouter ~4–6 px de padding entre les colonnes, puis vérifier le rendu dans la section de comparaison, qui est la plus étroite. Les valeurs recommandées sont **des valeurs initiales ajustables**, pas des dimensions codées en dur. Les wrappers nouvellement créés ne sont pas des membres `BindWidget` (excepté le `SizeBox_StatIcon` déjà nommé) : pas de Graph à créer.

- Police : style Alegreya Sans du projet, corps lisible ~15–17, chiffres alignés à droite.
- Couleur : `Text_DeltaPositive` favorable (vert), `Text_DeltaNegative` défavorable (rouge), `Text_DeltaNeutral` neutre (or pâle/gris). **La polarité ne se déduit pas du signe**, PA -1 = favorable.
- Laisser `Text` des champs dynamiques **vide**, aucun `TextBlockDefaultValue`.
- Map `StatIconTextures` dans les Class Defaults : conserver/copier les **18 `StatId` existants** de l'ancien `WBP_ItemTooltipStatLine` (sans supprimer l'asset). IDs exacts issus du C++ : `Strength`, `Dexterity`, `Constitution`, `Intelligence`, `Wisdom`, `Charisma`, `MaxHealth`, `MaxMana`, `CarryWeight`, `Armor`, `PhysicalResistance`, `FireResistance`, `IceResistance`, `LightningResistance`, `PoisonResistance`, `HolyResistance`, `NecroticResistance`, `ArcaneResistance`.
- Les nouvelles clés `WeaponDice`, `WeaponAccuracy`, `WeaponAP`, etc. peuvent rester sans texture tant qu'une iconographie dédiée n'a pas été adoptée ; leurs lignes restent **compactes**.

## 2. Dans `WBP_ItemTooltipNativeComparison`

Conserver les frères `VerticalBox_ComparisonStats` et `VerticalBox_ComparisonWeaponSection` directement sous `VerticalBox_ComparisonRoot` ; le C++ appelle `ClearChildren()` sur les conteneurs de lignes. Ne jamais placer la section weapon **dans** `VerticalBox_ComparisonStats`.

- `Text_EquipmentSlot` : petit en-tête de groupe, typographie forte.
- `Text_EquippedItem` : valeur de référence immédiatement dessous, légèrement atténuée.
- `Text_WeaponComparisonTitle` : texte statique **lié à `ST_UI`** : `Inventory.Tooltip.Weapon.ComparisonTitle`.
- Deux conteneurs remplissent horizontalement le groupe : `VerticalBox_ComparisonStats` et `VerticalBox_ComparisonWeaponFacts`. Ils hébergent tous deux le même `WBP_ItemTooltipNativeLine` avec **exactement les mêmes largeurs** de colonnes.
- Enlever les paddings/redondances qui créent une indentation supplémentaire sur les seules lignes d'armes.

## 3. Dans `WBP_ItemTooltip` racine

La fonction `InitFromInventorySlot` continue de déléguer à `PresentFromInventorySlot`. Ne changer ni cette liaison confirmée en PIE ni les propriétés `WeaponLineWidgetClass` / `ComparisonRowWidgetClass` déjà configurées.

- Garder le cadre médiéval, l'icône d'objet, le poids et les séparateurs.
- Ajouter 8–12 px de marge verticale avant `VerticalBox_WeaponSection` et 8–12 px avant `VerticalBox_ComparisonSection`, sans modifier la dimension/épaisseur des bordures.
- Aligner `VerticalBox_WeaponFacts` sur la même largeur exploitable que les faits du groupe de comparaison. Vérifier la largeur sur le WBP et en PIE plutôt que compenser avec des espaces ajoutés dans les chaînes.
- Éviter les titres statiques codés dans C++ : garder `Text_WeaponTitle` et `Text_WeaponComparisonTitle` alimentés par `ST_UI`. Aucune nouvelle clé `UI.csv` nécessaire dans cette passe.

## 4. Recette et publication

1. Sauvegarder les WBP avant tout ajustement, sans effacer l'ancien Graph déconnecté.
2. Compiler le C++ avec `Scripts/ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.WeaponTooltip04"` (attendu 8/8) puis régression `Grimrock.UI.Item01` (attendu 5/5).
3. En PIE : dague N1 et N9 ; poing/mains vides ; comparaison dague ↔ poignard d'assassin, test PA et dés textuels ; bouclier rond et armure sans duplication ; objet legacy avec résistance ; ouverture/fermeture répétée sans accumulation.
4. Capture d'écran du tooltip complet **sur un écran 1920x1080**. Vérifier absence de dépassement vertical et d'écrasement des textes longs (FR/EN). Si la taille est encore trop grande, documenter la hauteur observée plutôt que masquer arbitrairement des statistiques.
5. Compiler/enregistrer uniquement les WBP modifiés volontairement et créer leur **commit binaire distinct** une fois la recette visuelle approuvée.

**Ne pas annoncer 04.2C terminé après le seul commit source : l'esthétique reste un travail Designer / PIE.**
