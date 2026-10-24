# ARMEMENT01.10.3.3 — Combat Tooltip structuré : C++ logique, Designer UMG présentation

**Rectification d'architecture (24 octobre 2026) — ARMEMENT01.10.3.3.4.**  
**Statut :** code C++ préparé pour validation locale ; **WBP à adapter dans UE5.5.4 et rendu PIE non encore validé**. Les tests ARMEMENT01.10.3.3 avaient réussi **3/3 sur la version précédente** ; le correctif présent doit être revalidé.  
**Règle permanente :** [UI_UMG_PRESENTATION_CONTRACT.md](../UI_UMG_PRESENTATION_CONTRACT.md). Localisation : [UI_WBP_LOCALIZATION_WORKFLOW.md](../UI_WBP_LOCALIZATION_WORKFLOW.md).

## 1. Ce que l'utilisateur a demandé (et ce qu'il n'a PAS demandé)

L'ancien code C++ créait un `UGridPanel`, des `TextBlock`, des largeurs, des marges et des polices. Il était impossible de personnaliser ses « cellules » dans le Designer UE. La première tentative de correction a **déplacé toute la programmation de formatage, de boucle et de visibilité dans les Graphs Blueprint**, ce qui était **contraire à la demande**.

**Architecture définitive :**
- **C++ :** combat, snapshots, assemblage des messages localisés, données à afficher, sélection des messages, création des instances de **WBP déjà dessinés**, alimentation des TextBlocks et visibilité des sections.
- **WBP Designer :** widgets, quatre colonnes, espacements, polices, couleurs, alignements, dimensions, bordure ; ces propriétés restent éditables sans recompiler.
- **Graph :** aucune boucle, aucun `Format Text`, aucun événement de présentation nécessaire pour ce jalon. **Supprimer les Graphs de mise en forme créés pendant les étapes précédentes dans l'éditeur UE** ; ne pas refaire ce travail ailleurs.

Le C++ est autorisé à faire `SetText` sur un TextBlock dynamique **existant**, `SetVisibility` sur un conteneur **existant**, et `CreateWidget` d'une classe WBP de ligne **entière**. Il est interdit de construire cellule par cellule ou de régler le style des enfants.

## 2. Assets UE : chemins EXACTS et classes parentes

| Asset UE | Chemin Content Browser | Classe C++ |
|---|---|---|
| `WBP_GridCombatTooltip` (existant) | `/Game/GrimrockPrototype/Blueprints/UI/EventFeed/WBP_GridCombatTooltip` | **GridCombatTooltipWidget** |
| `WBP_GridCombatArmorRow` (existant, à reparent) | `/Game/GrimrockPrototype/Blueprints/UI/EventFeed/WBP_GridCombatArmorRow` | **GridCombatArmorRowWidget** |
| `WBP_GridEventFeed` (existant) | `/Game/GrimrockPrototype/Blueprints/UI/EventFeed/WBP_GridEventFeed` | Classe existante, sans modification |
| `WBP_GridNavigationTooltip` (repli existant) | `/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_GridNavigationTooltip` | Classe existante, sans modification |
| `ST_UI` | `/Game/Localization/StringTables/ST_UI` | Asset de localisation existant |

Chemin physique correspondant aux deux WBP :

```text
D:\Development\GrimrockPrototype\Content\GrimrockPrototype\Blueprints\UI\EventFeed\
  WBP_GridCombatTooltip.uasset
  WBP_GridCombatArmorRow.uasset
```

**Attention :** vous avez **déjà créé** `WBP_GridCombatArmorRow` avec les quatre `SizeBox` et `TextBlock`. **Ne pas le supprimer ni le recréer.** Il faut changer sa **Parent Class** depuis Unreal Editor.

## 3. Petite intervention MANUELLE dans UE après validation C++

### 3.1 WBP_GridCombatArmorRow : reparent sans détruire le Designer

1. Ouvrir `WBP_GridCombatArmorRow` au chemin de la §2.
2. **Class Settings → Parent Class → GridCombatArmorRowWidget** (nouvelle classe C++). Enregistrer le parent.
3. Conserver exactement cette hiérarchie :

   ```text
   WBP_GridCombatArmorRow [Parent = GridCombatArmorRowWidget]
   └─ HorizontalBox_Row
      ├─ SizeBox_Source [Width Override = 204]
      │  └─ Text_Source        [TextBlock / Is Variable]
      ├─ SizeBox_Before [Width Override = 76]
      │  └─ Text_Before        [TextBlock / Is Variable]
      ├─ SizeBox_Absorbed [Width Override = 92]
      │  └─ Text_Absorbed      [TextBlock / Is Variable]
      └─ SizeBox_After [Width Override = 84]
         └─ Text_After         [TextBlock / Is Variable]
   ```

4. Vérifier les **noms et les types exacts** des quatre TextBlocks. Les propriétés `UPROPERTY(meta=(BindWidgetOptional))` du C++ les récupèrent par nom ; `Is Variable` doit être actif.
5. **Supprimer Event Construct et les quatre SetText du Graph de la ligne** : `UGridCombatArmorRowWidget::SetArmorData` renseigne les textes après l'ajout au VerticalBox. Ne pas conserver la vieille variable BP `ArmorRow` si elle n'est plus utilisée ; la supprimer aussi, ainsi que les anciens nœuds `Break Grid Armor Absorption Source Trace`. Ne pas supprimer les TextBlocks.
6. Dans **Designer**, conserver **tous les réglages graphiques** (Alegreya Sans, couleurs, dimensions, alignements). Le C++ **ne modifie aucun de ces paramètres**. Compiler et enregistrer.

### 3.2 WBP_GridCombatTooltip : garder le layout et retirer les Graphs

1. Ouvrir `WBP_GridCombatTooltip` dans le même dossier. Parent **GridCombatTooltipWidget** inchangé.
2. Conserver `SizeBox_Root`, `Border_CombatTooltip`, `VerticalBox_Content`, la section ATTAQUE, la section CONSÉQUENCES, ainsi que le conteneur **`VerticalBox_AbsorptionSection`**.
3. La section d'absorption est une simple hiérarchie UMG authorée :

   ```text
   WBP_GridCombatTooltip [Parent = GridCombatTooltipWidget]
   └─ SizeBox_Root [WidthOverride = 480]
      └─ Border_CombatTooltip [Padding = 12]
         └─ VerticalBox_Content
            ├─ Text_Title           [ST_UI: Combat.Tooltip.Title]
            ├─ Text_SectionAttack   [ST_UI: Combat.Tooltip.Section.Attack]
            ├─ HorizontalBox_Actors [Text_Attacker, Text_Defender + labels]
            ├─ HorizontalBox_Attack [Text_Attack + label]
            ├─ HorizontalBox_Defense[Text_Defense + label]
            ├─ HorizontalBox_Damage [Text_Damage + label]
            │
            ├─ VerticalBox_AbsorptionSection [Is Variable]
            │  ├─ Spacer_AttackToArmor
            │  ├─ Text_SectionAbsorption [ST_UI: Combat.Tooltip.Section.Absorption]
            │  ├─ HorizontalBox_ArmorHeader  [quatre colonnes UMG]
            │  │  ├─ SizeBox_HeaderSource    [204] → Text_HeaderSource
            │  │  ├─ SizeBox_HeaderBefore    [76]  → Text_HeaderBefore
            │  │  ├─ SizeBox_HeaderAbsorbed  [92]  → Text_HeaderAbsorbed
            │  │  └─ SizeBox_HeaderAfter     [84]  → Text_HeaderAfter
            │  ├─ VerticalBox_ArmorRows      [Is Variable, VIDE dans Designer]
            │  └─ Spacer_ArmorToConsequences
            │
            ├─ Text_SectionConsequences  [ST_UI: Combat.Tooltip.Section.Consequences]
            ├─ HorizontalBox_Health      [Text_Health + label]
            └─ Text_Status               [TextBlock / Is Variable]
   ```

4. Vérifier **les neuf variables UMG attendues** : `Text_Attacker`, `Text_Defender`, `Text_Attack`, `Text_Defense`, `Text_Damage`, `Text_Health`, `Text_Status`, `VerticalBox_AbsorptionSection` et `VerticalBox_ArmorRows`. Les noms doivent correspondre **à l'identique** aux membres C++.
5. **Supprimer l'override `PresentCombatTooltip` construit dans le Graph** (et les variables `Fmt_Defense`, `Fmt_Attack`, `Fmt_Damage`, `Fmt_NoDamage`, `Fmt_Health`, `Fmt_NoHealthChange`, `Fmt_StatusDepleted`, `Fmt_StatusAlreadyDepleted` si plus utilisées). Dans **My Blueprint → Functions**, sur l'override, utiliser *Remove Override* / *Delete* selon le menu de la version UE ; après retrait vérifier que le widget compile. **Ne pas simplement débrancher le Return Node** : il ne doit plus y avoir de duplication de logique Blueprint.
6. Dans **Class Defaults → Combat | Tooltip → Armor Row Widget Class**, sélectionner **`WBP_GridCombatArmorRow`** (après son reparenting). Ce champ est indispensable quand une source absorbe des dégâts ; sinon le code utilise l'ancienne infobulle.
7. Conserver dans `WBP_GridEventFeed` le champ **`Combat Tooltip Widget Class = WBP_GridCombatTooltip`**. Les `SharedTooltipWidgetClass` / autres infobulles ne sont pas modifiés.
8. Compiler les deux WBP et enregistrer, puis lancer un combat PIE.

### 3.3 Les propriétés que vous modifierez librement en Designer

| Propriété | Lieu unique de réglage |
|---|---|
| Police, taille, graisse, couleur des quatre chiffres / nom de la source | **Text_Source / Text_Before / Text_Absorbed / Text_After** dans `WBP_GridCombatArmorRow` |
| Largeurs et alignements de chaque colonne | `SizeBox_*` dans WBP de ligne + `SizeBox_Header*` dans WBP principal |
| Bordures, fond, espace entre sections, taille de l'infobulle | `WBP_GridCombatTooltip`, Designer |
| Couleur et présence du message « protection épuisée » | Style de `Text_Status` dans Designer ; C++ choisit le contenu et sa visibilité |
| Titre, en-têtes, labels statiques | `ST_UI` référencé dans le Designer |
| Format et paramètres des textes dynamiques | **C++** via patterns localisés ST_UI ; aucun Graph Blueprint |

**Ne pas mettre de texte français littéral dans C++** : pour les valeurs dynamiques le code emploie `FUILocalizationText::Get` + `FText::Format`. Aucune clé supplémentaire à créer par rapport aux **33 clés déjà présentes** dans `UI.csv` ; s'assurer qu'elles sont matérialisées dans `ST_UI.uasset` (workflow transversal).

## 4. Que fait le C++ exactement ?

- `UGridCombatTooltipWidget::InitializeCombatTooltip(Entry)` prépare la trace historique, appelle `BuildDisplayTexts` et renseigne les sept TextBlocks dynamiques existants.
- `FGridCombatEventFeedFormatter::FormatRollAsDice` gère aussi le signe et les cas de plage/offset ; **aucune formule de dés à écrire dans Blueprint**.
- `VerticalBox_AbsorptionSection->SetVisibility` masque l'ensemble lorsque aucune source physique n'a absorbé de dégâts ; zéro ligne signifie **zéro tableau et zéro en-tête visible**.
- Pour chaque source avec `Absorbed > 0`, le C++ instancie **une classe WBP de ligne entière**, la met dans `VerticalBox_ArmorRows`, puis appelle `UGridCombatArmorRowWidget::SetArmorData(Source)` qui renseigne les quatre TextBlocks par `SetText`.
- Si plusieurs sources s'épuisent au même impact, les noms sont regroupés dans un message unique (au lieu du vieux `Get [0]` Blueprint).
- Si aucune source de protection n'a absorbé et qu'un bouclier était déjà à zéro, le message « aucune protection restante » peut rester sous CONSÉQUENCES, **sans tableau inutile**.
- Le C++ ne détermine **aucune police**, aucune marge et aucune position de cellule. Les TextBlocks sont créés **seulement par le Designer UMG**.
- Si le WBP est incomplet (TextBlocks manquants ou classe de ligne non renseignée), `InitializeCombatTooltip` retourne `false`, et `GridEventFeedWidget` conserve **une seule infobulle de repli** ; il n'affiche pas de cadre vide.
- Pour faciliter le nettoyage des Blueprints déjà sauvegardés, l'ancienne fonction `PresentCombatTooltip` est **temporairement conservée dans la classe C++ mais n'est plus jamais appelée**. C'est un contrat de compatibilité transitoire, pas une logique secondaire.

## 5. Recette / validation après publication

**Ne pas déclarer le jalon validé sur la simple présence d'un commit GitHub.**

```powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.10.3.3"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.10"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.EventHUD01"
```

**La première commande teste aussi la présence des 33 clés dans le vrai `ST_UI`**. Si elles manquent : exécuter avec UE fermé `-run=UIStringTableAuthoring` (voir [workflow](../UI_WBP_LOCALIZATION_WORKFLOW.md)) ; les 33 clés sont dans `UI.csv`, aucune nouvelle entrée n'a été ajoutée par cette rectification.

Vérification PIE obligatoire :
- une seule infobulle ; défense et attaque correctes avec les dés non standard et leurs offsets ;
- nom et chiffres de chaque source dans **les TextBlocks UMG** au style voulu ;
- colonnes correctement alignées via les `SizeBox` (204/76/92/84 pour une largeur intérieure de 456) ;
- bouclier 5→1 après 4 points absorbés, 1→0 après 1 absorbé, puis ABSORPTION masquée à 0→0 ;
- statut d'épuisement, statut « déjà vide » puis aucun statut, et sources multiples ;
- cas d'attaque manquée, repli texte si les WBP sont incomplets et tooltips des boutons HUD inchangés ;
- capture PIE et vérification des modifications `.uasset` créées **dans Unreal Editor**, puis versionnement atomique séparé de la matérialisation.

**État final :** code C++ à compiler ; Blueprints à reparent/nettoyer/configurer dans UE ; tests et capture PIE à obtenir. Le travail est incomplet tant que ces preuves manquent.
