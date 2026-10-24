# ARMEMENT01.10.3.3 — Combat Tooltip : modèle C++ minimal et présentation 100 % WBP

**Décision du 24 octobre 2026 — refonte ARMEMENT01.10.3.3.3.**  
**Statut : code de read-model publié pour validation locale ; migration des WBP **à effectuer exclusivement dans Unreal Editor**. Le WBP existant ne sera PAS magiquement reconfiguré par ce commit. Jusqu'à l'authoring du Blueprint, le flux revient à son ancienne infobulle textuelle.**  
**Contrat transversal obligatoire :** [UI_UMG_PRESENTATION_CONTRACT.md](../UI_UMG_PRESENTATION_CONTRACT.md). Localisation : [UI_WBP_LOCALIZATION_WORKFLOW.md](../UI_WBP_LOCALIZATION_WORKFLOW.md).

## 1. Pourquoi ce changement

La création en C++ de `UGridPanel`, `UTextBlock`, colonnes, polices, espacements et alertes par `WidgetTree` avait atteint ~268 lignes et produit des chevauchements. **Toute cette construction est supprimée.**

| Fonction | Autorité unique |
|---|---|
| Dés, coups, Défense, armure, PV, traces historiques | Résolveur/runtime **C++ existant**, inchangé |
| Filtrage des sources : afficher uniquement celles qui ont absorbé plus de 0 | **C++ read-model** |
| Signalement « épuisé maintenant » / « déjà à zéro » | **C++ read-model** (drapeaux et sources) |
| Traduction des libellés, formatage des jets et des nombres | **WBP + ST_UI**, pas C++ |
| Dimensions, en-têtes, disposition, couleurs, polices, alignements | **WBP Designer**, pas C++ |
| Création et ajout des lignes au VerticalBox | **WBP Event Graph**, avec un **WBP de ligne** réutilisable |

La classe C++ `UGridCombatTooltipWidget` ne crée **aucun widget**, ne cherche **aucun TextBlock**, ne formate **aucun FText**, n'importe plus `WidgetTree`, `GridPanel` ou `VerticalBox`. La seule interface avec le Blueprint est un read-model + un appel au présentateur.

## 2. Chemins EXACTS

| Asset | Content Browser UE | Dossier disque du projet |
|---|---|---|
| **Principal existant, à adapter** : `WBP_GridCombatTooltip` | `/Game/GrimrockPrototype/Blueprints/UI/EventFeed/WBP_GridCombatTooltip` | `Content/GrimrockPrototype/Blueprints/UI/EventFeed/WBP_GridCombatTooltip.uasset` |
| **Ligne à créer** : `WBP_GridCombatArmorRow` | `/Game/GrimrockPrototype/Blueprints/UI/EventFeed/WBP_GridCombatArmorRow` | `Content/GrimrockPrototype/Blueprints/UI/EventFeed/WBP_GridCombatArmorRow.uasset` |
| **Flux existant, conserver** : `WBP_GridEventFeed` | `/Game/GrimrockPrototype/Blueprints/UI/EventFeed/WBP_GridEventFeed` | `Content/GrimrockPrototype/Blueprints/UI/EventFeed/WBP_GridEventFeed.uasset` |
| **Repli existant, conserver** : `WBP_GridNavigationTooltip` | `/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_GridNavigationTooltip` | `Content/GrimrockPrototype/Blueprints/UI/Inventory/WBP_GridNavigationTooltip.uasset` |
| String Table | `/Game/Localization/StringTables/ST_UI` | `Content/Localization/StringTables/ST_UI.uasset` |

Le nouveau WBP de ligne est un **Widget Blueprint standard, parent `UserWidget`** (pas de classe C++ supplémentaire). Seul le WBP principal conserve comme parent natif `GridCombatTooltipWidget`.

## 3. Contrat d'échange C++ → WBP

`FGridCombatTooltipViewData` (`GridCombatTooltipWidget.h`), exposé en `BlueprintType`, contient :

| Propriété | Type | Usage dans le WBP |
|---|---|---|
| `bValid` | bool | Refuser une trace invalide |
| `Attacker`, `Defender` | `FText` existants | Noms à afficher tels quels, sans nouvelle clé |
| `Trace` | `FGridCombatResolutionTrace` | Nombres d'attaque, Défense, dégâts et PV issus du combat |
| `AttackDice` | `FGridCombatTooltipDiceNumbers` | Nombres déjà calculés : `bHasDice`, `Count`, `Sides`, `Rolled`, `Offset`, `Total` |
| `bShowAbsorption` | bool | Afficher/masquer **la section complète** |
| `VisibleArmorRows` | `TArray<FGridArmorAbsorptionSourceTrace>` | Lignes dont `Absorbed > 0` (ordre réel) |
| `NewlyExhaustedSources` | tableau de sources | Sources passées à zéro sur cette attaque |
| `AlreadyDepletedSources` | tableau de sources | Équipements à zéro avant l'attaque |

Chaque ligne de `VisibleArmorRows` contient `DisplayName`, `bEquipment`, `Before`, `Absorbed`, `After`, `bExhaustedByHit`. **Aucune chaîne préformatée.**

Entrée de présentation **WBP** :

```cpp
// Dans UGridCombatTooltipWidget :
bool InitializeCombatTooltip(const FGridHudEventEntry& Entry);

// Surcharger dans WBP_GridCombatTooltip (Overrides) :
bool PresentCombatTooltip(const FGridCombatTooltipViewData& Data);
```

Le WBP doit renseigner son interface puis **retourner True**. L'implémentation native retourne **False** par défaut ; `GridEventFeedWidget` affiche alors l'ancien tooltip au lieu d'un cadre vide. Pas de double infobulle.

## 4. Dans Unreal Editor : conserver et simplifier le WBP principal

1. Ouvrir UE5.5.4 → Content Browser → `Content/GrimrockPrototype/Blueprints/UI/EventFeed/` → `WBP_GridCombatTooltip`.
2. **Class Settings → Parent Class = GridCombatTooltipWidget**.
3. Dans **Designer**, conserver `SizeBox_Root` (480 unités), `Border_CombatTooltip` (padding 12), `VerticalBox_Content` et les éléments `Text_Title`, `Text_Attacker`, `Text_Defender`, `Text_Attack`, `Text_Defense`, `Text_Damage`, `Text_Health` et `Text_Status`.
4. Au lieu de laisser trois éléments d'absorption indépendants, **ajouter un `VerticalBox` nommé `VerticalBox_AbsorptionSection`**. Y déplacer les widgets UMG **existants**, dans cet ordre :

   ```text
   WBP_GridCombatTooltip  (parent GridCombatTooltipWidget)
   └── SizeBox_Root [SizeBox : WidthOverride = 480]
       └── Border_CombatTooltip [Border : Padding = 12]
           └── VerticalBox_Content [VerticalBox]
               ├── Text_Title [ST_UI: Combat.Tooltip.Title]
               ├── Text_SectionAttack [ST_UI: Combat.Tooltip.Section.Attack]
               ├── HorizontalBox_Actors [Text_Attacker, Text_Defender et leurs labels]
               ├── HorizontalBox_Attack [Text_LabelAttack, Text_Attack]
               ├── HorizontalBox_Defense [Text_LabelDefense, Text_Defense]
               ├── HorizontalBox_Damage [Text_LabelDamage, Text_Damage]
               │
               ├── VerticalBox_AbsorptionSection [VerticalBox ; visibilité Blueprint]
               │   ├── Spacer_AttackToArmor [Spacer]
               │   ├── Text_SectionAbsorption [ST_UI: Combat.Tooltip.Section.Absorption]
               │   ├── HorizontalBox_ArmorHeader [HorizontalBox, 4 colonnes FIXES]
               │   │   ├── SizeBox_HeaderSource [Text_HeaderSource, largeur 204]
               │   │   ├── SizeBox_HeaderBefore [Text_HeaderBefore, largeur 76]
               │   │   ├── SizeBox_HeaderAbsorbed [Text_HeaderAbsorbed, largeur 92]
               │   │   └── SizeBox_HeaderAfter [Text_HeaderAfter, largeur 84]
               │   ├── VerticalBox_ArmorRows [VerticalBox ; enfant ajouté par BP]
               │   └── Spacer_ArmorToConsequences [Spacer]
               │
               ├── Text_SectionConsequences [ST_UI: Combat.Tooltip.Section.Consequences]
               ├── HorizontalBox_Health [Text_LabelHealth, Text_Health]
               └── Text_Status [TextBlock ; visibilité BP]
   ```

5. Les largeurs 204 + 76 + 92 + 84 = **456**, soit les 480 unités du `SizeBox_Root` moins deux paddings de 12. Les TextBlocks numériques sont alignés à **droite** ; Source à gauche. L'UMG contrôle toutes les polices (Alegreya Sans), couleurs et marges.
6. **Supprimer de la logique Blueprint existante** tout éventuel doublon de `SetToolTip`/`OnMouseEnter` dans ce WBP. Le C++ du flux choisit un seul tooltip.

## 5. Nouveau WBP de ligne réutilisable (aucune classe C++ supplémentaire)

1. Dans **le même dossier `EventFeed`**, clic droit → **User Interface → Widget Blueprint → UserWidget** → nom `WBP_GridCombatArmorRow`.
2. Dans **Graph**, ajouter une variable **`ArmorRow`** de type `FGridArmorAbsorptionSourceTrace` ; activer **Instance Editable** et **Expose on Spawn**.
3. Designer :

   ```text
   WBP_GridCombatArmorRow [UserWidget]
   └── HorizontalBox_Row [HorizontalBox ; taille au contenu]
       ├── SizeBox_Source [Width Override 204]
       │   └── Text_Source [TextBlock ; Align Left ; AutoWrap]
       ├── SizeBox_Before [Width Override 76]
       │   └── Text_Before [TextBlock ; Align Right]
       ├── SizeBox_Absorbed [Width Override 92]
       │   └── Text_Absorbed [TextBlock ; Align Right]
       └── SizeBox_After [Width Override 84]
           └── Text_After [TextBlock ; Align Right]
   ```

4. Dans **Event Construct** de ce WBP de ligne :
   - `Get ArmorRow → Break FGridArmorAbsorptionSourceTrace`.
   - `Text_Source.SetText(DisplayName)` si `DisplayName` non vide ; sinon afficher `ST_UI Combat.Tooltip.Fallback.Armor` si `bEquipment` est vrai, ou `Combat.Tooltip.Armor.Character` sinon.
   - `Text_Before.SetText(ToText(Int Before))` ; `Text_Absorbed.SetText(ToText(Int Absorbed))` ; `Text_After.SetText(ToText(Int After))`.
   - Couleurs et mise en valeur du dernier point consommé via `bExhaustedByHit` **dans CE WBP seulement**.
5. Compilier/enregistrer. Pour modifier la présentation d'une ligne à l'avenir : **ouvrir ce seul WBP**, pas le C++.

## 6. Event Graph du WBP principal : une fonction, pas de Tick

1. Ouvrir `WBP_GridCombatTooltip → Overrides → PresentCombatTooltip`. Cette fonction est fournie par le parent C++. Entrée : `Data (FGridCombatTooltipViewData)` ; sortie : bool `Return Value`.
2. `Break FGridCombatTooltipViewData`. Si `bValid` est faux, **Return False**.
3. `SetText(Text_Attacker, Data.Attacker)` ; idem `Text_Defender`.
4. `Break Data.Trace.Attack`, `Data.Trace.Defense`, `Data.Trace.Damage` et `Data.AttackDice` pour alimenter les TextBlock :
   - `Text_Attack` : formatter dans Blueprint la notation **mécanique** `{Count}d{Sides} ({Rolled})` (et le décalage `Offset` s'il existe), puis le pattern localisé **ST_UI `Combat.Tooltip.Value.AttackRoll`** avec `Roll` et `Total = Trace.Attack.FinalValue`. Si `bHasDice=false`, afficher le nombre brut. Ne pas reconstituer les règles de combat : les six nombres proviennent déjà de C++.
   - `Text_Defense` : pattern **ST_UI `Combat.Tooltip.Value.DefenseRoll`** avec `Base, Evasion, Total` pris directement de `Trace.Defense`.
   - `Text_Damage` : si `Damage.bResolved`, pattern `Combat.Tooltip.Value.DamageResolved` avec `DamageAfterResistance` ; sinon `Combat.Tooltip.Value.NoDamage`.
   - `Text_Health` : pattern `Combat.Tooltip.Value.HealthAfterAttack` avec `HealthBefore / HealthAfter / HealthDamage` ; sinon `Combat.Tooltip.Value.NoHealthChange`.
5. **UN SEUL nœud `Set Visibility`** sur `VerticalBox_AbsorptionSection` : `Visible` si `bShowAbsorption=true`, sinon `Collapsed`. Le titre, l'en-tête, les lignes et les espacements disparaissent ensemble.
6. `VerticalBox_ArmorRows → Clear Children`, puis `For Each Loop(Data.VisibleArmorRows)` :
   - `Create Widget` : classe `WBP_GridCombatArmorRow`, renseigner l'entrée `ArmorRow = Array Element` (variable Expose on Spawn).
   - `VerticalBox_ArmorRows → Add Child` : résultat de `Create Widget`.
   - Cette boucle appartient **uniquement au Blueprint**. Aucune création de Widget ou colonne côté C++.
7. Pour `Text_Status` : en présence de sources dans `NewlyExhaustedSources`, formater le message avec `Combat.Tooltip.Status.DepletedNamed` et le nom. Sinon, si `AlreadyDepletedSources` n'est pas vide, utiliser `Combat.Tooltip.Status.AlreadyDepletedNamed`. Masquer `Text_Status` quand il n'y a aucun message. Les sources sont fournies dans un tableau, de sorte qu'une présentation multi-messages peut être authorée plus tard sans changement du C++.
8. **À la fin du graphe, Return True.** Cette valeur certifie au C++ que le WBP a pris en charge les données. **Tant que le graphe n'existe pas, la classe C++ retourne False** et le flux continue de montrer l'ancienne infobulle.

**Note sur Format Text et ST_UI :** créer dans le WBP des variables de type `Text` pour les **patterns déjà matérialisés** dans `ST_UI` (\`Combat.Tooltip.Value.*\`, \`Combat.Tooltip.Status.*\`). Brancher ces variables sur le pin `Format` du nœud Blueprint `Format Text` et y fournir les arguments. Pas de clés nouvelles, pas de texte français littéral.

## 7. Contrats inchangés, recette et validation

**Règle :** si `VisibleArmorRows` est vide, **cacher ABSORPTION entièrement** (même quand un objet reste équipé à 0). Quand il y a plusieurs sources qui absorbent, une ligne par source. Un coup qui épuise un bouclier affiche la dernière absorption + le message. Un coup suivant sans protection n'affiche plus la table.

| Historique ARM075 N1 | VisibleArmorRows | bShowAbsorption | Conséquences |
|---|---:|---|---|
| 5 → 1 après 4 absorbés | 1 | True | PV inchangés |
| 1 → 0 après 1 absorbé | 1 | True | Bouclier épuisé ; PV diminuent du surplus |
| 0 → 0 après 0 absorbé | 0 | False | Bouclier déjà épuisé ; dégâts PV |
| Bouclier 5 + armure de classe 2, coup de 6 | 2 | True | Protection totale 6, PV inchangés |

**Le contenu reste historique, sans relire le bouclier équipé.** La Défense n'est pas modifiée.

### Exécuter après `git pull --ff-only origin master`, UE fermé

```powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.10.3.3"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.10"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.EventHUD01"
```

**Aucune clé ST_UI supplémentaire** dans ce jalon : conserver les **33 clés existantes**, déjà dans `UI.csv`, et vérifier qu'elles ont été matérialisées dans `ST_UI.uasset`. Ne jamais fabriquer ce binaire hors Unreal. Valider ensuite les deux WBP en PIE : infobulle unique, textes localisés, alignement des 4 colonnes, absence de section vide, régression des tooltips de navigation. Leurs `.uasset` seront enregistrés par Unreal puis inspectés/versionnés.

**Statut « VALIDÉ » seulement après logs C++ et capture d'un WBP effectivement configuré.** Le repli textuel est attendu pendant la transition.
