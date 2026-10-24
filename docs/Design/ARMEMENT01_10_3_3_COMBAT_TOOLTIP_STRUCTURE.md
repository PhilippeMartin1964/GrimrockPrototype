# ARMEMENT01.10.3.3 — Infobulle de combat structurée : implantation UE5.5.4

**Mise à jour : 24 octobre 2026 — documentation précise, localisation, nettoyage C++.**  
**État :** compilation UE5.5.4 confirmée par l'utilisateur après `d67a56de` ; tests automatisés, WBP et vérification PIE du nettoyage **non encore validés**. Aucun `.uasset` n'a été créé ni modifié par ce jalon de code.

## 1. Contrat : une seule infobulle, pas deux

Survol d'une ligne de **Combat** dans le Flux d'événements : afficher un tableau de détail **Attaque / Absorption / Conséquences**, en conservant l'entrée du flux compacte.

- **Nouveau chemin :** `UGridCombatTooltipWidget`, pour les événements munis d'une trace structurée et lorsque `CombatTooltipWidgetClass` est configuré.
- **Repli ancien :** `UGridNavigationTooltipWidget`, déjà utilisé pour les tooltips du HUD persistant et pour les événements anciens ; le conserver. Il ne doit **jamais** s'afficher simultanément au nouveau.
- **Dernier repli :** texte UMG natif si le widget de repli n'est pas disponible.
- **Ne rien raccorder dans l'Event Graph** : `UGridEventFeedWidget` crée et attache déjà l'unique infobulle via `Row->SetToolTip(...)`. Ne pas ajouter de `OnMouseEnter`, `CreateWidget` ou autre `SetToolTip` dans le Blueprint.

## 2. Localiser précisément les assets : chemins vérifiés dans le dépôt

| Objet | Dossier Content Browser (`/Game`) | État |
|---|---|---|
| **Nouveau WBP** `WBP_GridCombatTooltip` | `/Game/GrimrockPrototype/Blueprints/UI/EventFeed/` | **À créer** |
| Flux `WBP_GridEventFeed` | `/Game/GrimrockPrototype/Blueprints/UI/EventFeed/` | Existant, à configurer |
| Ancienne infobulle `WBP_GridNavigationTooltip` | `/Game/GrimrockPrototype/Blueprints/UI/Inventory/` | Existante, à garder |
| String Table `ST_UI` | `/Game/Localization/StringTables/` | Existante, à régénérer depuis CSV |

Le chemin physique du nouveau Blueprint, **à enregistrer depuis UE**, est exactement :

```text
D:\Development\GrimrockPrototype\Content\GrimrockPrototype\Blueprints\UI\EventFeed\WBP_GridCombatTooltip.uasset
```

Le parent natif est `UGridCombatTooltipWidget` (nom affiché dans l'éditeur : `GridCombatTooltipWidget`). **Ne pas choisir** `UserWidget` standard ou `GridNavigationTooltipWidget` comme parent.

## 3. Localiser aussi les textes : autorité ST_UI

Les **14 clés statiques** ci-dessous sont déclarées dans `Localization/Source/UI.csv` et doivent être matérialisées dans `/Game/Localization/StringTables/ST_UI`. Il ne faut pas saisir les mots français directement dans les TextBlock du Designer.

**Correctif des glyphes du 24.10.2026 :** le projet utilise aussi **19 clés de textes dynamiques et de résumé compact** sous `Combat.Tooltip.*` et `Combat.Feed.Compact.*`. Elles sont définies dans le même `UI.csv` (33 clés nouvelles au total depuis le début du jalon) ; elles **ne sont pas entrées dans les TextBlock** : elles sont lues au runtime par `FUILocalizationText` puis mises en forme avec `FText::Format`. Il faut donc **relancer `UIStringTableAuthoring` après le nouveau git pull**, même si vous l'avez déjà fait pour les 14 premières clés.

| Texte affiché | Clé `ST_UI` |
|---|---|
| DÉTAILS DU COMBAT | `Combat.Tooltip.Title` |
| ATTAQUE | `Combat.Tooltip.Section.Attack` |
| ABSORPTION | `Combat.Tooltip.Section.Absorption` |
| CONSÉQUENCES | `Combat.Tooltip.Section.Consequences` |
| Attaquant | `Combat.Tooltip.Label.Attacker` |
| Défenseur | `Combat.Tooltip.Label.Defender` |
| Jet d'attaque | `Combat.Tooltip.Label.Attack` |
| Défense | `Combat.Tooltip.Label.Defense` |
| Dégâts | `Combat.Tooltip.Label.Damage` |
| Points de vie | `Combat.Tooltip.Label.Health` |
| Protection | `Combat.Tooltip.Column.Source` |
| Avant | `Combat.Tooltip.Column.Before` |
| Absorbé | `Combat.Tooltip.Column.Absorbed` |
| Après | `Combat.Tooltip.Column.After` |

### 3.1 Matérialiser la String Table avant de créer le WBP

Fermer Unreal Editor et exécuter le commandlet existant du projet :

```powershell
cd D:\Development\GrimrockPrototype
& "D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\Development\GrimrockPrototype\GrimrockPrototype.uproject" -run=UIStringTableAuthoring -unattended -nop4 -nosplash -nullrhi
```

Ce commandlet fait évoluer **localement** le vrai asset UE `Content/Localization/StringTables/ST_UI.uasset`. Ne pas créer/modifier ce fichier binaire manuellement : l'inspecter dans l'éditeur avant tout commit de matérialisation ultérieur. Les traductions anglaises doivent encore être contrôlées dans le pipeline `Game` : l'ajout de clés françaises à `UI.csv` ne prouve **pas** la traduction automatique.

Pour chaque libellé **statique** du WBP : dans **Details → Content → Text**, choisir **String Table**, sélectionner `ST_UI` et sa clé exacte. Ne pas créer de Text Binding Blueprint.

Les champs **dynamiques** sont remplis par C++ avec des `FText` ; les nouveaux textes propres à cette infobulle utilisent désormais des **clés `ST_UI`** et non des littéraux français dans le C++. La traduction en anglais dépend toujours du pipeline de traduction `Game`, pas de la seule matérialisation. Ces champs ne doivent avoir **aucun binding Designer / Graph**.


### 3.2 Workflow COMPLET et obligatoire pour ce WBP

**Procédure transversale permanente :** [`docs/UI_WBP_LOCALIZATION_WORKFLOW.md`](../UI_WBP_LOCALIZATION_WORKFLOW.md). La procédure ne s'arrête pas à l'ajout de la clé dans le CSV : elle couvre sa matérialisation, la référence Designer, les traductions Game, les tests et les assets à versionner.

**L'incident constaté sur `Combat.Tooltip.Title` est expliqué :** les 14 clés existent dans `Localization/Source/UI.csv` depuis le commit `47f4d077` ; il est **normal** qu'elles restent introuvables dans l'ancienne version de `ST_UI.uasset` tant que le commandlet n'a pas été exécuté localement. Aucun texte n'a à être recréé manuellement dans le WBP. Ce guide et sa modification Git **ne régénèrent pas le binaire ST_UI**.

Effectuer ces étapes **dans l'ordre, sans passer à la suivante si une preuve manque** :

1. **Vérifier la source CSV après `git pull`.** Sous PowerShell :
   
   ~~~powershell
   Import-Csv .\Localization\Source\UI.csv | Where-Object Key -like 'Combat.Tooltip.*' | Select-Object Key,SourceString
   ~~~
   
   Attendu : **14** clés `Combat.Tooltip.*`, notamment `Combat.Tooltip.Title` = « DÉTAILS DU COMBAT ».
2. **Fermer Unreal Editor.** Exécuter la commande `-run=UIStringTableAuthoring` de la §3.1. Attendre le retour du processus et son code de sortie **0** ; chercher `[UI-LOCALIZATION01.4A] ST_UI materialization succeeded. Changed=true` dans le journal. `Changed=false` n'est acceptable que si les clés sont déjà présentes. Si le commandlet échoue, **STOP**.
3. **Vérifier le disque ET l'asset Unreal.** Exécuter :
   
   ~~~powershell
   Test-Path .\Content\Localization\StringTables\ST_UI.uasset
   git status --short -- .\Content\Localization\StringTables\ST_UI.uasset
   ~~~
   
   Rouvrir Unreal et inspecter `/Game/Localization/StringTables/ST_UI` ; rechercher manuellement `Combat.Tooltip.Title` et `Combat.Tooltip.Column.Absorbed`. **STOP si l'une manque**, même si le CSV et Git sont corrects.
4. **Seule maintenant, créer le WBP** dans `/Game/GrimrockPrototype/Blueprints/UI/EventFeed/WBP_GridCombatTooltip`, parent `GridCombatTooltipWidget`, arborescence §4.1. Lier **directement** chaque TextBlock **statique** à `ST_UI + clé` selon le tableau §3. Les champs **dynamiques C++** ne doivent pas être liés une seconde fois à la String Table ou à un Text Binding Blueprint.
5. **Compiler / sauvegarder les WBP dans UE**, puis tester PIE. Vérifier la fiche sur une attaque du Rat géant, le tableau de protection et l'absence de double tooltip. Ne pas committer de `.uasset` WBP avant inspection et capture.
6. **Traiter la traduction du jeu séparément :** le target `Game` (fr/en) requiert collecte des nouvelles entrées, export des PO, traduction anglaise réelle, importation, compilation et test anglais. La commande `UIStringTableAuthoring` **ne traduit pas**. Le script `RunLocalizationSmoke.ps1` valide la chaîne technique avec un **échantillon** anglais, pas les 14 libellés traduits. Les phrases C++ `LOCTEXT` doivent également être couvertes par le gather approprié. Voir le workflow transversal §3.D.
7. **Tests et Git :** exécuter les filtres de la §7, inspecter `git status --short` et distinguer clairement `UI.csv` (source), `ST_UI.uasset` (projection), `WBP_GridCombatTooltip.uasset` (présentation), et PO/locres `Game` (traductions). Versionner les vrais assets produits **par Unreal** dans l'étape de matérialisation validée, en respectant un seul commit atomique par étape. Un commit Git ne contenant que les nouvelles clés CSV n'est pas une validation finale de l'UMG.

**CHECKLIST DE SORTIE OBLIGATOIRE :** CSV à jour ✅, commandlet UE code 0, clés visibles dans ST_UI, WBP lié, PIE lisible, statut traduction fr/en explicite, assets binaires inspectés/versionnés, tests réussis. **Tant que ces vérifications ne sont pas attestées, le WBP et la localisation restent EN COURS.**

## 4. Création du WBP dans Unreal Editor, pas à pas

1. Après compilation puis matérialisation `ST_UI`, ouvrir UE5.5.4.
2. Content Drawer → `Content/GrimrockPrototype/Blueprints/UI/EventFeed`.
3. Clic droit → **User Interface → Widget Blueprint → All Classes** ; rechercher le parent **`GridCombatTooltipWidget`**.
4. Nommer l'asset **`WBP_GridCombatTooltip`** (dans **EventFeed**, pas dans Inventory/Combat ni à la racine Content).
5. Ouvrir le WBP → **Class Settings** → vérifier **Parent Class = GridCombatTooltipWidget**.
6. Dans **Designer**, construire la hiérarchie exacte ci-après. Laisser **Graph vide**.

### 4.1 Hiérarchie UMG complète (type et noms exacts)

```text
WBP_GridCombatTooltip                  (Parent = GridCombatTooltipWidget)
└─ SizeBox_Root                       [SizeBox ; Width Override 480]
   └─ Border_CombatTooltip            [Border ; Padding 12 ; fond brun/noir]
      └─ VerticalBox_Content          [VerticalBox ; hauteur automatique]
         ├─ Text_Title                        [TextBlock : ST_UI Combat.Tooltip.Title]
         ├─ Spacer_Title                      [Spacer ; 8 px]
         │
         ├─ Text_SectionAttack                [TextBlock : ST_UI Combat.Tooltip.Section.Attack]
         ├─ HorizontalBox_Actors              [HorizontalBox]
         │  ├─ VerticalBox_Attacker           [VerticalBox ; Fill 1.0]
         │  │  ├─ Text_LabelAttacker          [TextBlock : ST_UI Combat.Tooltip.Label.Attacker]
         │  │  └─ Text_Attacker               [TextBlock : C++ DYNAMIQUE]
         │  └─ VerticalBox_Defender           [VerticalBox ; Fill 1.0]
         │     ├─ Text_LabelDefender          [TextBlock : ST_UI Combat.Tooltip.Label.Defender]
         │     └─ Text_Defender               [TextBlock : C++ DYNAMIQUE]
         ├─ HorizontalBox_Attack              [HorizontalBox]
         │  ├─ Text_LabelAttack               [TextBlock : ST_UI Combat.Tooltip.Label.Attack]
         │  └─ Text_Attack                    [TextBlock : C++ DYNAMIQUE]
         ├─ HorizontalBox_Defense             [HorizontalBox]
         │  ├─ Text_LabelDefense              [TextBlock : ST_UI Combat.Tooltip.Label.Defense]
         │  └─ Text_Defense                   [TextBlock : C++ DYNAMIQUE]
         ├─ HorizontalBox_Damage              [HorizontalBox]
         │  ├─ Text_LabelDamage               [TextBlock : ST_UI Combat.Tooltip.Label.Damage]
         │  └─ Text_Damage                    [TextBlock : C++ DYNAMIQUE]
         │
         ├─ Spacer_AttackToArmor              [Spacer ; 10 px]
         ├─ Text_SectionAbsorption            [TextBlock : ST_UI Combat.Tooltip.Section.Absorption]
         ├─ HorizontalBox_ArmorHeader         [HorizontalBox ; colonnes]
         │  ├─ Text_HeaderSource              [TextBlock : ST_UI Combat.Tooltip.Column.Source]
         │  ├─ Text_HeaderBefore              [TextBlock : ST_UI Combat.Tooltip.Column.Before]
         │  ├─ Text_HeaderAbsorbed            [TextBlock : ST_UI Combat.Tooltip.Column.Absorbed]
         │  └─ Text_HeaderAfter               [TextBlock : ST_UI Combat.Tooltip.Column.After]
         ├─ VerticalBox_ArmorRows             [VerticalBox : C++ DYNAMIQUE — VIDE au Designer]
         │
         ├─ Spacer_ArmorToConsequences        [Spacer ; 10 px]
         ├─ Text_SectionConsequences          [TextBlock : ST_UI Combat.Tooltip.Section.Consequences]
         ├─ HorizontalBox_Health              [HorizontalBox]
         │  ├─ Text_LabelHealth               [TextBlock : ST_UI Combat.Tooltip.Label.Health]
         │  └─ Text_Health                    [TextBlock : C++ DYNAMIQUE]
         └─ Text_Status                       [TextBlock : C++ DYNAMIQUE ; alerte ambre]
```

### 4.2 Les 8 bindings C++ impératifs

Ces champs sont déclarés dans `GridCombatTooltipWidget.h` en `BindWidgetOptional` : **une erreur de nom peut produire un champ vide sans erreur de compilation** ! Ils doivent porter exactement le nom ci-dessous, être du bon type, et avoir **Is Variable = coché** dans le Designer.

| Nom | Type | Exemple de valeur injectée |
|---|---|---|
| `Text_Attacker` | TextBlock | Rat géant |
| `Text_Defender` | TextBlock | Elias |
| `Text_Attack` | TextBlock | 1d20 (18) ; Total : 20 |
| `Text_Defense` | TextBlock | 10 + 1 = 11 |
| `Text_Damage` | TextBlock | 4 après modificateurs et résistances |
| `VerticalBox_ArmorRows` | VerticalBox | Lignes source/avant/absorbé/après produites par le C++ |
| `Text_Health` | TextBlock | 11 → 11 (−0 PV) |
| `Text_Status` | TextBlock | Bouclier rond : protection épuisée !, uniquement si applicable |

Les TextBlock statiques (`Text_Title`, `Text_Section*`, `Text_Header*`, `Text_Label*`) **n'ont pas besoin de Is Variable** : leur texte provient directement de `ST_UI`.

### 4.3 Réglages indispensables du tableau

Dans le `HorizontalBox_ArmorHeader`, sélectionner successivement ses quatre TextBlock enfants → **Details → Slot (Horizontal Box Slot) → Size → Fill**. Attribuer les poids suivants :

| Colonne | Nom enfant | Fill Weight | Alignement |
|---|---|---:|---|
| Source | `Text_HeaderSource` | **2.5** | Left |
| Avant | `Text_HeaderBefore` | **1.0** | Right |
| Absorbé | `Text_HeaderAbsorbed` | **1.0** | Right |
| Après | `Text_HeaderAfter` | **1.0** | Right |

Le C++ attribue **ces mêmes proportions 2.5 : 1 : 1 : 1** aux lignes qu'il insère dans `VerticalBox_ArmorRows`, afin d'aligner en-tête et données. **Ne pas créer de ligne d'armure à la main** ni un nouveau `WBP_ArmorRow` : le C++ gère le nombre de sources.

**Correction de l'affichage des caractères « A » encadrés :** l'ancienne propriété C++ `TableFont` a été supprimée. Les `UTextBlock` de chaque ligne dynamique prennent désormais automatiquement la **police de `Text_HeaderSource`**, qui doit conserver une police UMG valide (par exemple Alegreya Sans). Aucun réglage `TableFont` dans Class Defaults n'est nécessaire. Dans le Designer, vérifier que `Text_HeaderSource` existe sous **ce nom exact**, affiche correctement « Protection » et utilise la police souhaitée ; s'il manque, la ligne utilise la police par défaut du widget. **Ne pas régler une police FontInfo vide dans le C++ ni dessiner des lignes de texte à la main.**

**Espacement des lignes Attaque/Défense/Dégâts/PV :** dans chacun des `HorizontalBox_Attack`, `HorizontalBox_Defense`, `HorizontalBox_Damage` et `HorizontalBox_Health`, séparer le libellé et la valeur par un **Padding Right de 8 à 12** sur le slot du libellé, ou régler les slots `Text_Label*` en `Fill 1.0` et les valeurs en `Auto`. La capture du 24.10 montrait notamment `Jet d'attaque1d20` et `Défense10+1` sans espace, qui relèvent du **layout UMG**, pas de la localisation.

**Autres réglages** : `SizeBox_Root` 480 unités de largeur (ajustable jusqu'à 520), hauteur non forcée ; `Border_CombatTooltip` padding 12 ; `VerticalBox_Content` taille au contenu. Titres ~16 pt, valeurs ~14–16 pt Alegreya Sans ; couleur or désaturée pour les sections, vert doux pour l'absorption, ambre pour l'épuisement, sans surcharge. Le tooltip doit être **Hit Test Invisible** ; aucun CanvasPanel qui occupe l'écran entier, aucun suivi souris via Tick.

### 4.4 Activation dans le flux existant

1. Ouvrir `/Game/GrimrockPrototype/Blueprints/UI/EventFeed/WBP_GridEventFeed`.
2. Cliquer **Class Defaults** de ce **WBP_GridEventFeed**.
3. Rechercher **Combat Tooltip Widget Class** dans la catégorie `HUD | Event Feed | Combat Tooltip`.
4. Choisir **WBP_GridCombatTooltip** ; compiler et enregistrer les **deux** WBP.
5. **Ne pas remplacer** la classe `SharedTooltipWidgetClass` injectée par le HUD persistant ; elle sert au repli et aux autres boutons.
6. Lancer `L_Dungeon` en PIE puis survoler une ligne d'attaque.

Si le champ `Combat Tooltip Widget Class` est sur **None**, l'ancien tooltip textuel s'affiche **à la place du nouveau**, jamais en même temps. Ne rien ajouter dans Event Graph.

## 5. Données historiques et comportement attendu

Les instantanés sont capturés lors de la **vraie consommation d'armure** par `ApplyPhysicalArmorDamage` et transportés jusqu'au Flux. L'infobulle **ne relit pas l'équipement actuel**.

Bouclier rond ARM075 N1 : 5 points de protection au départ, aucune autre réserve :

| Attaque physique | Source | Avant | Absorbé | Après | PV si 20 au départ | Alerte |
|---|---|---:|---:|---:|---|---|
| 4 dégâts | Bouclier rond | 5 | 4 | 1 | 20 → 20 | Aucune |
| 5 dégâts | Bouclier rond | 1 | 1 | 0 | 20 → 16 | Bouclier rond : protection épuisée ! |
| 3 dégâts | Bouclier rond | 0 | 0 | 0 | 16 → 13 | Bouclier rond : aucune protection restante |

Si deux points d'armure de base restent au personnage, une morsure de 6 absorbe d'abord 5 au bouclier, puis 1 aux ressources du personnage : **deux lignes distinctes**. Aucun bonus automatique de Défense, aucune réparation implicite, aucun nouvel effet de combat.

**Portée actuelle :** la liste détaillée des équipements est renseignée sur les **attaques de monstres vers les personnages**. Les autres événements conservent leur trace habituelle : ne pas inventer des sources de bouclier quand la ventilation n'existe pas.

## 6. Nettoyage de code effectué

- Suppression de `FormatSourceRow`, un utilitaire exposé seulement pour un test et inutilisé en présentation réelle ; adaptation du test.
- Conservation **raisonnée** de `UGridNavigationTooltipWidget` : il n'est plus un concurrent mais un **fallback**. Le supprimer casserait d'autres tooltips HUD.
- Meilleure sélection du widget structuré même si `Details` texte est vide.
- Largeur dynamique des colonnes alignée avec l'en-tête.
- Message d'épuisement **avec nom d'équipement** et ligne d'alerte masquée quand sans objet.
- **Aucun changement de RPG**, de dégâts, de catalogue, d'armure ou de fichier `.uasset` dans le correctif de glyphes.
- **Audit des textes en C++ (24.10.2026) :** les nouveaux libellés `GridCombatTooltipWidget.cpp` (résultat des jets, absorption, PV, messages d'épuisement) passent par `FUILocalizationText::Get(`ST_UI`)`. Le nom de l'armure de base est résolu au niveau présentation depuis `bEquipment=false`, et non plus inscrit en français dans le composant d'inventaire. Les nouvelles chaînes compactes non localisables dans `GridCombatEventFeedFormatter.cpp` sont aussi remplacées par des patterns de `ST_UI`. **Les `LOCTEXT` plus anciens du formateur, des autres systèmes et leurs règles de traduction appartiennent à un audit transversal distinct** ; ils sont localisables par Unreal en principe, mais leur collecte effective doit être vérifiée. Il ne serait pas exact d'affirmer qu'il n'existe plus aucun mot français dans tout le C++ du projet.
- **Barrière anti-régression :** le test ciblé vérifie la présence des 33 clés dans le **vrai `ST_UI`** ; lancer le commandlet de matérialisation avant les tests après cette mise à jour.

## 7. Validation exigée (non présumée réussie)

**Compilateur UE5.5.4 :** réussite du correctif précédent déclarée par l'utilisateur ; le nouveau nettoyage doit être recompilé et testé.

```powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master
# IMPORTANT : régénérer ST_UI (19 nouvelles clés C++ depuis la capture)
& "D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\Development\GrimrockPrototype\GrimrockPrototype.uproject" -run=UIStringTableAuthoring -unattended -nop4 -nosplash -nullrhi
if ($LASTEXITCODE -ne 0) { throw "Echec UIStringTableAuthoring: $LASTEXITCODE" }
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.10.3.3"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.10"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.EventHUD01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Game.UI.Localization.LOCALIZATION01"
```

Attendus : **3 tests ciblés** et **17 tests cumulés** ARMEMENT01.10 si les 14 antérieurs subsistent. Les filtres UI et localisation restent à valider avec les logs réels.

**Après matérialisation ST_UI et construction UMG**, vérifier en PIE : exactement un tooltip par ligne, les 8 champs renseignés, les colonnes alignées, la ventilation de plusieurs sources, les morsures historiques `5→1 / 1→0 / 0→0`, le message d'épuisement au bon moment, l'ancienne infobulle en mode repli et les autres boutons HUD. Envoyer une **capture de l'infobulle visible** : les tests commandlet ne prouvent pas son rendu.

**Ne pas modifier les fichiers `.uasset` directement.** La création du WBP et la matérialisation ST_UI sont deux étapes UE à valider visuellement.
