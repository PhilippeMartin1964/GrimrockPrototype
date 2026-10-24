# ARMEMENT01.10.3.3 — Combat Tooltip structuré : C++ logique, Designer UMG présentation

**Rectification d'architecture (24 octobre 2026) — ARMEMENT01.10.3.3.4.**  
**Statut au 24.10.2026 :** **compilation et tests C++ de ARMEMENT01.10.3.3.4 VALIDÉS localement : 3/3, 0 erreur, 0 avertissement**, rapport `Saved/Automation/TD04/TD04-20261024-152922`. L'utilisateur indique avoir affecté `Armor Row Widget Class = WBP_GridCombatArmorRow` et que l'interface semble fonctionner en PIE. **Clôture graphique non encore démontrée par une capture complète ; nettoyages UMG et versionnement des deux WBP encore à confirmer.**  
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

**Attention :** vous avez **déjà créé** `WBP_GridCombatArmorRow` avec les quatre `SizeBox` et `TextBlock`. **Ne pas le supprimer ni le recréer.** Son parent doit être `GridCombatArmorRowWidget` : vérifier ce point dans UE avant nettoyage. **Audit GitHub du 24.10.2026 : ces deux WBP ne sont pas encore suivis sous ces chemins dans `master`**, bien que `WBP_GridEventFeed.uasset` s'y trouve. La configuration locale ne sera pas reproductible tant qu'ils ne seront pas versionnés.

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

## 4.1 — Correctif de lisibilité du jet d'attaque (ARMEMENT01.10.3.3.6)

**Incident du 24.10.2026, capture du combat contre le Rat géant :** l'infobulle affichait « Jet d'attaque : 1d20 (3) ; Total : 5 ». Le dé **n'a pas donné 5** : il a donné **3**. La trace du résolveur contient aussi le bonus `Attack.SourceAccuracy` (ici **+2**) et `Attack.AttackAccuracyBonus` (bonus/malus de l'action, ici **0**). La première version de `BuildDisplayTexts` n'affichait pas ces termes et sautait donc directement au total.

**Correction limitée à la présentation :**

- `FGridCombatEventFeedFormatter::FormatAttackTerms(Attack)` fournit la **notation exacte des termes**, sans total : dé naturel, bonus fixe éventuel de `NdM+K`, puis `SourceAccuracy` et `AttackAccuracyBonus` **séparément avec leurs signes**. Les termes à zéro n'ajoutent pas de « +0 ».
- `UGridCombatTooltipWidget::BuildDisplayTexts` utilise cette fonction pour l'argument `Roll` du pattern ST_UI **existant** `Combat.Tooltip.Value.AttackRoll` ; l'argument `Total` reste `Attack.FinalValue` issu du résolveur.
- Le journal compact et l'infobulle **partagent maintenant le même formateur de termes**, évitant deux logiques divergentes.
- Exemple reproduit : `Jet d'attaque : 1d20 (3) + 2 ; Total : 5`. Si deux modificateurs sont présents : `1d20 (3) + 4 - 2 ; Total : 5`.
- Si le dé porte un bonus intrinsèque (`NdM+K`), le formateur l'ajoute **une seule fois** avant les bonus de précision.
- **Aucune nouvelle clé ST_UI, aucun nouveau TextBlock, aucune modification du Designer ou des règles de combat.** Les textes dynamiques restent produits par C++ et insérés dans `Text_Attack`.
- Nouveau test `Grimrock.RPG.ARMEMENT01.10.3.3.TransparentAttackTotal` : dé 3 / précision +2 / total 5, plusieurs bonus/malus, offset et absence de +0.

**Validation attendue :** filtre `Grimrock.RPG.ARMEMENT01.10.3.3` passe désormais de **3 à 4 tests**. Après validation locale, refaire un combat PIE et vérifier que `Text_Attack` affiche explicitement les termes expliquant le total, aussi bien en cas d'échec qu'en cas de touche. La **Défense** et le tableau ABSORPTION ne changent pas.

## 5. Nettoyage final sûr — **sans casser le WBP qui fonctionne**

**Ne pas entreprendre un nouveau refactor C++ pour « nettoyer » :** les structures historiques `FGridCombatTooltipDiceNumbers` et `FGridCombatTooltipViewData` et la fonction native `PresentCombatTooltip` subsistent temporairement pour la compatibilité avec l'asset Blueprint existant. `InitializeCombatTooltip` **n'appelle plus** `PresentCombatTooltip`. Supprimer aujourd'hui la déclaration C++ de cette fonction, sans vérifier d'abord sa suppression dans le WBP et la sauvegarde de l'asset, peut casser l'override sérialisé et le chargement du Blueprint.

À effectuer **dans Unreal Editor, sans créer de nouvelle classe ni modifier le Designer** :

1. **WBP_GridCombatArmorRow** : vérifier `Class Settings → Parent Class = GridCombatArmorRowWidget`. Garder les quatre SizeBox/TextBlock et tous leurs styles. Dans le **Graph**, supprimer les anciens `Event Construct`, `SetText`, `Break Grid Armor Absorption Source Trace` ; supprimer la variable Blueprint `ArmorRow` si plus utilisée. Compiler, enregistrer.
2. **WBP_GridCombatTooltip** : garder le layout UMG ; retirer dans **My Blueprint → Functions → PresentCombatTooltip** l'**override** de formatage de l'ancienne approche (menu `Remove Override` / supprimer l'override suivant UE). Supprimer les variables `Fmt_*` qui ne sont plus référencées. Ne pas supprimer `Text_Status` ni `VerticalBox_ArmorRows` ! Compiler, enregistrer.
3. **Class Defaults** du WBP principal : vérifier `Combat | Tooltip → Armor Row Widget Class = WBP_GridCombatArmorRow`. Le développeur a indiqué l'avoir déjà fait ; pas besoin de recommencer si la valeur est toujours bonne.
4. Refaire un **PIE** (voir §6 ci-dessous) après la suppression des Graphs, afin de prouver que toute la logique passe exclusivement par C++.
5. **Quitter Unreal Editor** une fois les deux WBP enregistrés. Contrôler les deux assets locaux (PowerShell) :

   ~~~powershell
   cd D:\Development\GrimrockPrototype
   $dir = 'Content\GrimrockPrototype\Blueprints\UI\EventFeed'
   Test-Path "$dir\WBP_GridCombatTooltip.uasset"
   Test-Path "$dir\WBP_GridCombatArmorRow.uasset"
   git status --short -- "$dir\WBP_GridCombatTooltip.uasset" "$dir\WBP_GridCombatArmorRow.uasset"
   ~~~

   Les deux `Test-Path` doivent retourner `True`. Ils sont suivis par **Git LFS** selon `.gitattributes` ; il faudra inclure leur contenu dans le dépôt, pas seulement les laisser dans `Content/` en local.
6. **Après revue du statut Git**, et uniquement pour les deux WBP ci-dessus, versionner les assets Unreal sauvegardés dans un **seul commit atomique dédié** (ne pas inclure tous les changements locaux accidentellement) :

   ~~~powershell
   git add -- "Content/GrimrockPrototype/Blueprints/UI/EventFeed/WBP_GridCombatTooltip.uasset" "Content/GrimrockPrototype/Blueprints/UI/EventFeed/WBP_GridCombatArmorRow.uasset"
   git diff --cached --name-only
   git commit -m "ARMEMENT01.10.3.3.5: save authored combat tooltip WBP assets"
   git push origin master
   ~~~

   **Ne pas lancer le commit avant** le rendu PIE concluant, la compilation/sauvegarde des deux WBP et la revue des modifications. En cas de changements locaux inattendus ou de push non fast-forward, ne pas forcer la publication.

7. **Retrait définitif du vieux code C++** `PresentCombatTooltip` et, éventuellement, de la projection de dés `AttackDice` : **jalon de nettoyage distinct**, uniquement **après** vérification que les deux assets WBP propres, sans override ni ancienne variable, sont réellement versionnés. Ne pas supprimer un symbole qui peut encore être référencé par un asset non committé.

## 6. Recette / validation après publication

**Tests natifs : validés 3/3 le 24.10.2026** (rapport ci-dessus). Le filtre ciblé ne prouve pas le fonctionnement visuel des WBP ; les régressions et la validation PIE restent nécessaires pour la clôture intégrale.

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

**État au 24.10.2026 :** C++ validé 3/3 ; réglage `Armor Row Widget Class` déclaré effectué et interface jugée fonctionnelle par l'utilisateur. Restent **l'inspection/nettoyage des Graphs**, **les régressions ARMEMENT01.10 et EventHUD01**, **la preuve de rendu PIE**, **le commit Git LFS des deux WBP**, puis **le retrait ultérieur du symbole C++ obsolète après publication des assets nettoyés**. Le jalon n'est pas encore totalement clos.
