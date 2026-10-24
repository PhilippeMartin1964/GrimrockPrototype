# ARMEMENT01.10.3.3 — Combat Tooltip structuré : C++ logique, Designer UMG présentation

**RECTIFICATIF DE CLÔTURE ARMEMENT01.10.3.3.9 (24.10.2026) :** le nettoyage natif supprime définitivement `Text_Attack`, `Text_Defense`, `PresentCombatTooltip`, la projection `FGridCombatTooltipDiceNumbers` et le fallback multiligne associé. **Les sections historiques 3 et 4.3 conservent l'ordre des anciennes migrations ; pour toute intervention actuelle, appliquer UNIQUEMENT la procédure §4.4 ci-dessous.** Le journal d'événements dispose toujours d'un fallback textuel indépendant, qui n'est pas un ancien WBP à conserver. **8 tests attendus** après compilation du correctif ; aucune modification de `UI.csv` ou d'asset binaire dans ce commit C++.

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

**Attention :** vous avez **déjà créé** `WBP_GridCombatArmorRow` avec les quatre `SizeBox` et `TextBlock`. **Ne pas le supprimer ni le recréer.** Son parent doit être `GridCombatArmorRowWidget` : vérifier ce point dans UE avant nettoyage. **Mise à jour :** le commit utilisateur [`f858351b`](https://github.com/PhilippeMartin1964/GrimrockPrototype/commit/f858351b9a7a5dbb6f2a1bceed1d4fc41aaea0b4) (« ARMEMENT : hand implementation ») a depuis ajouté les deux WBP dans `master`, ainsi que `ST_UI.uasset` et les changements de localisation. Il faut toujours les modifier **uniquement dans Unreal Editor** ; ne pas écraser leur version.

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

## 4.2 — ARMEMENT01.10.3.3.7 : infobulle explicative et décision de toucher

**Incident confirmé du 24 octobre 2026 :** le journal indiquait `1d20 (5) + 1 + 1 = 7` ; la tooltip répétait les mêmes nombres sans identifier les deux +1. Cela contredisait sa fonction explicative.

### Provenance exacte des valeurs du résolveur

| Trace C++ | Signification autoritaire | Texte affiché |
|---|---|---|
| `Attack.Roll` | Jet réellement consommé (y compris NdM+K) | Jet de dé |
| `Attack.SourceAccuracy` | `FGridAttackSourceStats::Accuracy` : précision globale déjà agrégée de l'attaquant | Précision de l'attaquant |
| `Attack.AttackAccuracyBonus` | `FGridAttackDefinition::AccuracyBonus` : modificateur de l'attaque exécutée | Modificateur de l'attaque |
| `Attack.FinalValue` | Total calculé par le résolveur | Total de l'attaque |
| `Defense.BaseValue` | Base du seuil de défense | Défense de base |
| `Defense.EvasionModifier` | `FGridAttackTargetStats::Evasion` | Esquive du défenseur |
| `Defense.FinalValue` | Seuil calculé | Défense totale |
| `bNaturalAutoMiss`, `bNaturalAutoHit` et `bAlwaysHits` | Exceptions de toucher enregistrées | Explication de la réussite / de l'échec |
| `CriticalMinimumRoll` et tirage naturel | Seuil autoritaire de coup critique | Coup critique, si réellement atteint |

**Limite honnête :** `SourceAccuracy` est une valeur globale agrégée, et la trace ne dit PAS quels sous-bonus viennent d'une arme, compétence ou caractéristique. Il serait incorrect d'attribuer arbitrairement le +1 à la dague. L'éventuelle ventilation par équipement nécessitera une évolution distincte du résolveur et des traces.

### Affichage attendu, d'après la capture fournie

    ATTAQUE
    Attaquant : Elias                  Défenseur : Rat géant

    Jet de dé : 1d20 (5)
    Précision de l'attaquant : +1
    Modificateur de l'attaque : +1
    Total de l'attaque : 7

    Défense de base : 10
    Esquive du défenseur : +1
    Défense totale : 11
    Résultat : ÉCHEC (7 < 11)

    Dégâts : Aucun dégât résolu

    CONSÉQUENCES
    Points de vie : PV inchangés

**Pas de nouvelle interface à créer.** Le C++ prépare les `FText` descriptifs et les affecte aux TextBlocks **existants** `Text_Attack` et `Text_Defense` ; la police, les espacements et le layout restent contrôlés par le Designer du `WBP_GridCombatTooltip`. L'Event Feed compact reste inchangé.

### Autres situations couvertes

- Omission des bonus à zéro, mais affichage des bonus négatifs avec leur signe.
- Gestion de `NdM+K` sans double comptage.
- Comparaison `Attack.FinalValue >= Defense.FinalValue`, l'égalité étant une touche.
- `1 naturel` : échec automatique, même si le score dépasse la défense.
- `20 naturel` : touche automatique, même si le score est inférieur à la défense.
- `bAlwaysHits` : toucher garanti prioritaire sur le 1 naturel.
- Coup critique expliqué lorsque le jet naturel atteint le seuil réel.
- Dégâts directs sans jet : aucun faux dé ni comparaison fictive.

### Localisation et tests obligatoires

- **18 nouvelles clés** dans `Localization/Source/UI.csv`, sous `Combat.Tooltip.Attack.*`, `Combat.Tooltip.Defense.*` et `Combat.Tooltip.Outcome.*`. L'automate contrôle désormais **51 clés** (33 anciennes + 18 nouvelles).
- **2 nouveaux tests** fondés sur le résolveur réel : `Grimrock.RPG.ARMEMENT01.10.3.3.7.LabeledAccuracyAndMissExplanation` et `Grimrock.RPG.ARMEMENT01.10.3.3.7.AutomaticHitMissCriticalAndDirectDamage`.
- Test antérieur `TransparentAttackTotal` mis à jour pour vérifier que la tooltip ne reprend pas la formule compacte.
- **6 tests attendus** sur le filtre `Grimrock.RPG.ARMEMENT01.10.3.3`, non validés tant que les logs locaux ne sont pas disponibles.
- **Un CSV Git ne crée PAS la String Table Unreal** ; matérialiser `ST_UI.uasset` avec l'outil Unreal avant la validation. Ne pas fabriquer de binaire Unreal hors UE.

Commandes PowerShell (UE fermé, à partir de `D:\Development\GrimrockPrototype`) :

1. `git status --short` puis `git pull --ff-only origin master`.
2. `& "D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\Development\GrimrockPrototype\GrimrockPrototype.uproject" -run=UIStringTableAuthoring -unattended -nop4 -nosplash -nullrhi`. Vérifier le code de sortie 0.
3. `.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.10.3.3"`.
4. Ouvrir `/Game/Localization/StringTables/ST_UI` et vérifier les 18 nouvelles clés ; contrôler le rendu en PIE et préserver le binaire généré via Git LFS.
5. Faire les régressions `Grimrock.RPG.ARMEMENT01.10`, `Grimrock.UI.EventHUD01` et le smoke localisation. Les traductions anglaises Game sont à compléter indépendamment du CSV français.

## 4.3 — ARMEMENT01.10.3.3.8 : édition INDIVIDUELLE des lignes dans UMG

**Décision corrigée du 24.10.2026 :** la version .7 produisait 2 blocs multilignes `Text_Attack` et `Text_Defense`. Les lignes étaient justes mais leur couleur, police, largeur, marges et disposition n'étaient PAS réglables individuellement. Cela ne répondait pas au contrat Designer. La version **.8** livre des données indépendantes en C++ et demande **neuf TextBlocks existants dans le Designer**. Il n'y a **aucun Graph à refaire**, ni « Create Widget » des cellules en C++.

### 1. État et sauvegarde préalable

Les deux assets graphiques sont maintenant **versionnés sur `master`** depuis le commit utilisateur `f858351b` et peuvent être récupérés par Git LFS. Ils contiennent déjà son travail graphique : **ne pas les remplacer ni les recréer**. Avant d'éditer, fermer UE puis effectuer une copie de sécurité des deux `.uasset` dans `Saved/Backups/ARMEMENT0110338/`. Les enregistrer ensuite normalement depuis Unreal Editor.

Le présentateur natif reste **compatible avec les WBP existants** : si les neuf nouveaux champs ne sont pas encore tous présents, il affiche les anciens `Text_Attack` / `Text_Defense` complets, et masque les éventuels champs .8 déjà ajoutés. Quand les **neuf champs** sont prêts, le C++ remplit séparément chaque champ .8 et masque les deux anciens blocs. On peut donc progresser dans le Designer **sans casser le jeu**.

**Attention :** tout changement de disposition, de police, de couleur ou de taille est manuel dans UE (les binaires .uasset ne se modifient pas par le connecteur GitHub) ; ce commit n'installe que la couche C++ compatible, les tests et la documentation.

### 2. Chemin WBP principal à ouvrir

Content Browser :
`/Game/GrimrockPrototype/Blueprints/UI/EventFeed/WBP_GridCombatTooltip`

Fichier local :
`D:\Development\GrimrockPrototype\Content\GrimrockPrototype\Blueprints\UI\EventFeed\WBP_GridCombatTooltip.uasset`

**Parent C++ inchangé :** `GridCombatTooltipWidget`.

### 3. Hiérarchie UMG demandée dans ce WBP

Ne modifier **aucun** widget de la section ABSORPTION ni du WBP `WBP_GridCombatArmorRow`.

```text
WBP_GridCombatTooltip  [Parent: GridCombatTooltipWidget]
└─ SizeBox_Root
   └─ Border_CombatTooltip
      └─ VerticalBox_Content
         ├─ ... titre + section ATTAQUE + HorizontalBox_Actors (inchangés)
         │
         ├─ HorizontalBox_Attack               [EXISTANT, conservé]
         │  ├─ Text_LabelAttack                [titre existant, nom exact à vérifier]
         │  └─ Text_Attack                     [EXISTANT, fallback C++]
         │
         ├─ VerticalBox_AttackDetails          [NOUVEAU conteneur UMG]
         │  ├─ Text_AttackDice                 [NOUVEAU / TextBlock / Is Variable]
         │  ├─ Text_AttackSourceAccuracy       [NOUVEAU / TextBlock / Is Variable]
         │  ├─ Text_AttackActionAccuracy       [NOUVEAU / TextBlock / Is Variable]
         │  └─ Text_AttackTotal                [NOUVEAU / TextBlock / Is Variable]
         │
         ├─ HorizontalBox_Defense              [EXISTANT, conservé]
         │  ├─ Text_LabelDefense               [titre existant, nom exact à vérifier]
         │  └─ Text_Defense                    [EXISTANT, fallback C++]
         │
         ├─ VerticalBox_DefenseDetails         [NOUVEAU conteneur UMG]
         │  ├─ Text_DefenseBase                [NOUVEAU / TextBlock / Is Variable]
         │  ├─ Text_DefenseEvasion             [NOUVEAU / TextBlock / Is Variable]
         │  ├─ Text_DefenseTotal               [NOUVEAU / TextBlock / Is Variable]
         │  ├─ Text_AttackOutcome              [NOUVEAU / TextBlock / Is Variable]
         │  └─ Text_AttackCritical             [NOUVEAU / TextBlock / Is Variable]
         │
         ├─ HorizontalBox_Damage               [EXISTANT, inchangé]
         ├─ VerticalBox_AbsorptionSection      [EXISTANT, inchangé]
         └─ ... section CONSÉQUENCES, PV et statut (inchangés)
```

**Note sur Text_LabelAttack/Text_LabelDefense :** noms indicatifs des titres **déjà présents** : leur nom réel dans votre UMG peut différer. Les seuls noms devant impérativement être identiques aux déclarations `BindWidgetOptional` C++ sont les **neuf TextBlocks nouveaux** listés exactement ci-dessus, ainsi que les anciens `Text_Attack` et `Text_Defense` conservés.

### 4. Procédure dans le Designer UE5.5.4

1. Ouvrir `WBP_GridCombatTooltip` → onglet **Designer**, afficher **Hierarchy**.
2. Cliquer sur `VerticalBox_Content`. Insérer un **Vertical Box** entre `HorizontalBox_Attack` et `HorizontalBox_Defense` et le nommer exactement `VerticalBox_AttackDetails`. Aucun `Is Variable` nécessaire pour ce conteneur.
3. Dans ce Vertical Box, déposer quatre **Text** (TextBlocks), leur donner **exactement** les noms `Text_AttackDice`, `Text_AttackSourceAccuracy`, `Text_AttackActionAccuracy`, `Text_AttackTotal` et activer **Is Variable** pour chacun.
4. Insérer un deuxième **Vertical Box** entre `HorizontalBox_Defense` et `HorizontalBox_Damage`, nommé `VerticalBox_DefenseDetails`.
5. Y placer cinq TextBlocks : `Text_DefenseBase`, `Text_DefenseEvasion`, `Text_DefenseTotal`, `Text_AttackOutcome`, `Text_AttackCritical`, tous **Is Variable**.
6. Sélectionner chaque nouveau TextBlock et choisir librement dans **Details** sa police (ex. Alegreya Sans), taille, couleur, ombre, wrapping, justification, min desired width, etc. Régler ses **Slot/Vertical Box Slot → Padding, Horizontal Alignment, Vertical Alignment** dans le Designer. Aucun `SetFont/SetPadding` n'existe dans cette logique C++.
7. Conserver `Text_Attack`, `Text_Defense` et leurs lignes/titres existants **pour le fallback** ; quand les neuf nouveaux widgets sont détectés, le C++ masque uniquement ces deux blocs anciens mais les titres statiques peuvent servir de **rubriques graphiques** « Jet d'attaque » et « Défense ». Renommer, repositionner ou restyler les titres selon vos préférences ; ne pas les dupliquer.
8. **Aucun Event Graph, Format Text, binding ou SetText Blueprint n'est nécessaire.** Les nouvelles valeurs affichées dans le Designer avant PIE ne sont que des placeholders ; elles seront remplacées par les `FText` localisés du C++.
9. **Compile → Save**, puis tester PIE. Si la disposition actuelle semble conservée sans les neuf champs, c'est le **fallback**, pas un échec. Si les neuf champs existent, les nouvelles lignes se stylisent de manière indépendante.

### 5. Comportement C++ et particularités

- `UGridCombatTooltipWidget::BuildDetailTexts` retourne **neuf textes indépendants**, sans positions ni styles. `BuildDisplayTexts` reconstruit les deux lignes multilignes de fallback **à partir des mêmes données**, évitant deux logiques concurrentes.
- Le C++ remplit `Text_AttackDice` (ou « pas de jet »), `Text_AttackSourceAccuracy`, `Text_AttackActionAccuracy`, `Text_AttackTotal`, `Text_DefenseBase`, `Text_DefenseEvasion`, `Text_DefenseTotal`, `Text_AttackOutcome`, `Text_AttackCritical`.
- Un bonus `0` ou une ligne « coup critique » non applicable fournit un FText vide ; le C++ la met en `Collapsed`, donc **pas de ligne inutile**.
- Une attaque sans jet masque les valeurs qui n'existent pas et affiche seulement les indications de non-jet.
- Migration **atomique** : tant que les neuf widgets de détails ne sont pas présents, les anciens blocs restent visibles et les nouveaux sont masqués. Pas de rendu double ni de texte partiel.
- Les libellés et leurs variables de substitution sont **localisés en ST_UI** ; l'utilisateur gère `UI.csv` séparément. **AUCUNE clé ajoutée dans ce commit**.
- Le C++ ne crée pas de TextBlocks, ne modifie aucune police et ne calcule aucune position. Toutes les valeurs graphiques des neuf lignes sont dans le Designer. Les noms des clés/localisation peuvent être modifiés via le CSV ; les propriétés visuelles via UMG.

### 6. Test après intégration

```powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.10.3.3"
```

**Attendu : 7/7 tests** (six tests précédents déjà validés plus `Grimrock.RPG.ARMEMENT01.10.3.3.8.IndependentDesignerTexts`). Vérifier ensuite le WBP sous UE et une capture PIE. Pas de validation des neuf bindings UMG par le seul filtre de tests C++ ; ce sont des assets binaires **locaux**.

**À ne pas faire :** supprimer les deux vieux TextBlocks, restaurer les .uasset depuis Git, appliquer le stash `UI.csv` sans comparer, ni modifier manuellement le C++ pour un changement de police ou de colonnes. Le CSV et le contenu local sont sous la responsabilité du développeur, qui récupère ses changements indépendamment.

## 4.4 — ARMEMENT01.10.3.3.9 : nettoyage définitif sans fallback UI obsolète

**Prérequis confirmé par l'utilisateur :** les neuf nouvelles lignes de la tooltip fonctionnent en PIE ; test `Grimrock.RPG.ARMEMENT01.10.3.3` **7/7 réussis** dans `Saved/Automation/TD04/TD04-20261024-200621`, zéro avertissement/échec. Les deux WBP et `ST_UI.uasset` sont versionnés dans le commit utilisateur `92c1d234 ARMEMENT tooltip closure`. Ce nettoyage ne change ni les règles RPG, ni les statistiques ni les textes du CSV.

### Règle de suppression

| Élément | Action |
|---|---|
| `WBP_GridCombatTooltip → Text_Attack` | **SUPPRIMER** du Designer |
| `WBP_GridCombatTooltip → Text_Defense` | **SUPPRIMER** du Designer |
| `PresentCombatTooltip` (ancien override/Graph) | **SUPPRIMER** dans My Blueprint / Overrides |
| Variables BP `Fmt_*` inutilisées | **SUPPRIMER** des variables Blueprint |
| Anciens `Event Construct` et `SetText` dans `WBP_GridCombatArmorRow` | **SUPPRIMER** si présents |
| `Text_AttackDice`, `Text_AttackSourceAccuracy`, `Text_AttackActionAccuracy`, `Text_AttackTotal`, `Text_DefenseBase`, `Text_DefenseEvasion`, `Text_DefenseTotal`, `Text_AttackOutcome`, `Text_AttackCritical` | **CONSERVER impérativement** avec `Is Variable` |
| `Text_Damage`, `Text_Health`, `Text_Status`, personnages et sections ABSORPTION | **CONSERVER impérativement** |
| Titres statiques « Jet d'attaque », « Défense » dans le Designer | **CONSERVER ou réorganiser librement** ; supprimer uniquement les anciens contenus dynamiques redondants |
| `WBP_GridEventFeed → Combat Tooltip Widget Class` | **CONSERVER** la référence au WBP principal |
| Les deux WBP et `ST_UI.uasset` | **CONSERVER** les fichiers binaires et leurs styles, ne pas les recréer |

Le C++ est désormais l'autorité **exclusive** des neuf TextBlocks. Les anciens noms C++ et la fonction native obsolète sont supprimés ; il n'y a plus de branche « neuf champs → sinon deux anciens blocs ». L'ancienne projection virtuelle de dés `AttackDice` n'était utilisée que par le vieux Graph : elle est supprimée et les tests vérifient directement le format final du résolveur.

**Important :** le repli **générique de l'Event Feed**, déjà indépendant de cette infobulle, est conservé en cas de classe Blueprint absente ou mal configurée : il évite un tooltip vide. Ce mécanisme n'est PAS le fallback `Text_Attack/Text_Defense` supprimé.

### Ordre d'opérations — NE PAS inverser

**A. Sauvegarde des fichiers binaires avec UE fermé**, avant de toucher au Designer :

~~~powershell
cd D:\Development\GrimrockPrototype
$source = "Content\GrimrockPrototype\Blueprints\UI\EventFeed"
$backup = "Saved\Backups\ARMEMENT0110339_$(Get-Date -Format 'yyyyMMdd_HHmmss')"
New-Item -ItemType Directory -Force -Path $backup | Out-Null
Copy-Item "$source\WBP_GridCombatTooltip.uasset" $backup
Copy-Item "$source\WBP_GridCombatArmorRow.uasset" $backup
git status --short
~~~

**B. AVANT de récupérer la version C++ nettoyée**, ouvrir le projet dans **Unreal Editor avec le C++ ancien** : l'ancien `PresentCombatTooltip` existe encore et peut être retiré proprement de l'asset Blueprint. Modifier seulement :

- `/Game/GrimrockPrototype/Blueprints/UI/EventFeed/WBP_GridCombatTooltip` : supprimer `Text_Attack` et `Text_Defense` du Designer, sans supprimer les titres statiques si on souhaite les conserver. Si les conteneurs `HorizontalBox_Attack` / `HorizontalBox_Defense` deviennent vides, les réagencer dans le Designer.
- Supprimer l'override `PresentCombatTooltip` de **My Blueprint → Functions** (Remove Override / suppression selon version), ainsi que les variables `Fmt_*` inutilisées.
- `WBP_GridCombatArmorRow` : retirer uniquement les restes du Graph/variable `ArmorRow` s'ils existent.
- Vérifier chaque `TextBlock` requis (les neuf nouveaux + `Text_Damage`, `Text_Health`, `Text_Status`), leurs noms exacts et `Is Variable`.
- **Compile → Save** les deux WBP. Fermer ensuite Unreal Editor. Ne pas tester le PIE entre B et C : le C++ ancien demande encore ses vieux TextBlocks à l'exécution.

**C. Mise à jour C++ sur master, les assets locaux restent protégés :**

~~~powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.10.3.3"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.10"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.EventHUD01"
~~~

Attendu pour le filtre ciblé : **8/8 tests**, dont le nouveau `Grimrock.RPG.ARMEMENT01.10.3.3.9.NoLegacyBindings` qui vérifie l'absence de membres natifs obsolètes et la présence des 14 propriétés UMG nécessaires. Les deux autres filtres sont des **régressions** et leur nombre exact de tests dépend de la suite actuelle.

**D. Ouvrir UE, compiler les WBP, tester en PIE** : résultats séparés du dé, précision et défense, coup normal, coup manqué, bouclier 5→1 puis 1→0 et 0→0 ; section d'armure invisible à zéro ; aucun ancien texte double ni Graph résiduel. Captures à conserver.

**E. Versionnement sélectif des binaires nettoyés**, après sauvegarde sous UE et validation PIE :

~~~powershell
git status --short
git add -- "Content/GrimrockPrototype/Blueprints/UI/EventFeed/WBP_GridCombatTooltip.uasset" "Content/GrimrockPrototype/Blueprints/UI/EventFeed/WBP_GridCombatArmorRow.uasset"
git diff --cached --name-only
git commit -m "ARMEMENT01.10.3.3.9: remove obsolete combat tooltip WBP graph and fields"
git push origin master
~~~

Ne pas lancer `git add .` ni `git restore` sur les assets d'autres fonctionnalités. Les binaires restent gérés par **Git LFS**. Si un WBP ne s'est pas modifié, ne pas l'ajouter artificiellement ; vérifier ce qui a réellement été sauvegardé.

### Critères de clôture

- C++ compile, test ciblé **8/8**, régressions ARMEMENT01.10 et EventHUD01 réussies.
- Aucun `Text_Attack`, `Text_Defense`, `Fmt_*` ou override `PresentCombatTooltip` résiduel dans le WBP.
- Les neuf champs du Designer restent indépendamment stylables, sans police/marge forcée depuis C++.
- PIE : données exactes, statuts de bouclier, sections masquées sans bruit.
- Les deux WBP nettoyés sont sauvegardés dans UE et versionnés sélectivement. **Ne pas déclarer le nettoyage intégral achevé avant ces preuves.**

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
