# ARMEMENT01.10.3.3.10 — Dégâts détaillés et éditables dans WBP

Date : 24.10.2026. **Statut : code C++, clés ST_UI et tests publiés ; compilation UE / matérialisation ST_UI / authoring UMG / PIE à valider localement.**

## 1. Motif et autorité

Après succès des quatre armes Guerrier, le testeur relève que la rubrique DÉGÂTS de `WBP_GridCombatTooltip` ne montre que « {Damage} après modificateurs et résistances ». Le résolveur enregistre pourtant déjà chaque étape dans `FGridCombatDamageTrace` ; le journal compact et `FGridCombatEventFeedFormatter::AddDamageDetails` en exploitent une grande partie.

**Décision :** aucun recalcul des dégâts dans le présentateur. Chaque ligne est produite depuis la **trace d'impact immuable** ; aucune modification de `GridCombatResolver`, `DA_WeaponCatalogue`, statistiques Excel, kits, combat ou sauvegardes. Le C++ affecte les textes et les visibilités des TextBlocks ; les marges, polices, bordures, couleurs et positions restent au **Designer UMG**.

Références : `Source/GrimrockPrototype/Public/Runtime/Combat/GridCombatTraceTypes.h` (`FGridCombatDamageTrace`), `GridCombatResolver.cpp` (ordre réel du pipeline).

## 2. Douze nouveaux TextBlocks UMG EXACTS

Ouvrir **le WBP existant** : `/Game/GrimrockPrototype/Blueprints/UI/EventFeed/WBP_GridCombatTooltip` (parent `UGridCombatTooltipWidget`).

Sous la section DÉGÂTS existante (à conserver), insérer un `Vertical Box` nommé `VerticalBox_DamageDetails` dans le Designer, entre DÉFENSE et ABSORPTION, et y placer les **douze TextBlocks ci-dessous**, **Is Variable** activé sur chaque TextBlock. Le conteneur reste librement dimensionnable :

```text
VerticalBox_DamageDetails
  Text_DamageRoll
  Text_DamageSourceBonus
  Text_DamageBase
  Text_DamageRawPercent
  Text_DamageScaledBase
  Text_DamageCriticalPercent
  Text_DamageAfterCritical
  Text_DamageSourceMultiplier
  Text_DamageTargetMultiplier
  Text_DamageAfterMultipliers
  Text_DamageResistance
  Text_DamageTotal
```

| TextBlock | Trace C++ (autorité) | Conditions |
|---|---|---|
| `Text_DamageRoll` | `Roll` / `RawDamage` direct | Jet réel `NdM+K` avec détails ; dégâts directs sans dé fictif ; message « Aucun dégât résolu » si raté |
| `Text_DamageSourceBonus` | `SourceDamageBonus` | Si non nul ; agrégat non attribuable arbitrairement à Force ou arme |
| `Text_DamageBase` | `BaseDamage` | Attaque avec jet résolu ; exclu pour dégâts directs |
| `Text_DamageRawPercent` | `RawDamagePercent` | Si différent de 100 % |
| `Text_DamageScaledBase` | `ScaledBaseDamage` | Avec le coefficient brut, expose son résultat arrondi |
| `Text_DamageCriticalPercent` | `CriticalDamagePercent` | Si différent de 100 % |
| `Text_DamageAfterCritical` | `RawDamage` | Après coefficient critique si applicable |
| `Text_DamageSourceMultiplier` | `SourceDamageMultiplier` | Si différent de ×1 |
| `Text_DamageTargetMultiplier` | `TargetDamageMultiplier` | Si différent de ×1 |
| `Text_DamageAfterMultipliers` | `DamageAfterMultiplier` | Si l'un des deux multiplicateurs a un effet ; valeur finale de la trace |
| `Text_DamageResistance` | `ResistancePercent` | Si différent de 0 % ; négatif = vulnérabilité |
| `Text_DamageTotal` | `DamageAfterResistance` | Toujours pour un dégât résolu, **avant absorption** |

Les champs sans effet sont remplis par `FText::GetEmpty` puis définis en `Collapsed` ; pas de lignes vides. Les valeurs de stages restent exactement celles **enregistrées** par le résolveur (arrondis non refaits dans l'UI). Les multiplicateurs sont affichés avec jusqu'à 6 décimales, seule leur représentation est formatée.

**L'ancien `Text_Damage` est supprimé du contrat C++ (pas de fallback multiligne graphique).** Le conserver dans le Designer n'affichera plus rien dynamiquement et pourrait produire un doublon statique ; le supprimer **dans UE** une fois les 12 nouveaux TextBlocks ajoutés. Garder le titre de section existant « Dégâts » si voulu. **Conserver** les neuf champs ATTAQUE/DÉFENSE, `Text_Health`, `Text_Status`, les personnages et le tableau ABSORPTION.

## 3. Séquence locale sûre

**Avec Unreal Editor fermé**, sauvegarder l'asset utilisateur :

```powershell
cd D:\Development\GrimrockPrototype
$backup = "Saved\Backups\ARMEMENT01103310_$(Get-Date -Format 'yyyyMMdd_HHmmss')"
New-Item -ItemType Directory -Force -Path $backup | Out-Null
Copy-Item "Content\GrimrockPrototype\Blueprints\UI\EventFeed\WBP_GridCombatTooltip.uasset" $backup
Copy-Item "Content\Localization\StringTables\ST_UI.uasset" $backup
git status --short
```

Le code est publié sur `master`, avec **14 clés** supplémentaires dans `Localization/Source/UI.csv`. Si `UI.csv` est modifié localement, **ne pas écraser les modifications** : les comparer et sauvegarder avant de faire le pull. Sinon :

```powershell
git pull --ff-only origin master

& "D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
    "D:\Development\GrimrockPrototype\GrimrockPrototype.uproject" `
    -run=UIStringTableAuthoring -unattended -nop4 -nosplash -nullrhi
if ($LASTEXITCODE -ne 0) { throw "Échec de la matérialisation ST_UI : $LASTEXITCODE" }

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.10.3.3"
```

**Attendu 11/11 tests** : 8 tests validés du jalon .9 + 3 nouveaux tests de dégâts : jets `2d6+K`, bonus, coefficient, multiplicateurs, résistance/arrondis/armure, critique, dégâts directs et attaque manquée.

Le test des clés ST_UI et les nouveaux tests ne réussiront **pas** si `ST_UI.uasset` n'a pas été régénéré après le `git pull`.

**Ensuite**, ouvrir Unreal Editor, dans `WBP_GridCombatTooltip` :

1. Ajouter `VerticalBox_DamageDetails` et les **12 TextBlocks EXACTS** sous DÉGÂTS. Activer **Is Variable**.
2. Régler séparément les propriétés de chaque TextBlock dans **Details** (ex. Alegreya Sans, taille, couleur, Padding, SizeToContent/Auto selon parent). Aucun TextBlock enfant ne doit être créé par C++ ou Graph.
3. Supprimer **l'ancien** `Text_Damage` du Designer ; conserver les en-têtes statiques.
4. Ne toucher ni aux neuf widgets ATTAQUE/DÉFENSE, ni à la partie ABSORPTION, ni au WBP de ligne d'armure. Aucune logique de formatage Blueprint à rajouter.
5. **Compile → Save**. Puis PIE.

Le C++ vérifie la présence de tous les champs avant d'afficher la tooltip structurée : tant que le Designer est incomplet, le journal d'événements utilise son **texte générique de secours**, pas un ancien layout UMG.

## 4. Recette PIE

- ARM014 / ARM015 / ARM025 / ARM031 : au moins un coup réussi ; noter les dés réellement obtenus, l'éventuel K et le bonus source ; le total avant absorption doit correspondre aux dégâts pré-armure du combat.
- Si `2d6+K` : afficher les valeurs individuelles des dés, ainsi que le K **une seule fois**.
- Attaque manquée : aucune ligne de dégâts résolus fictive, seul « Aucun dégât résolu ».
- Critique (si reproductible) : coefficient critique et sous-total visibles, absent en coup normal.
- Bonus/coefficients/résistances : zéro/100 %/×1 masqués ; valeurs non neutres visibles. Une résistance négative est une vulnérabilité.
- Dégâts directs : ne pas afficher de lancer de dés fictif.
- Garder séparés `Text_DamageTotal` (dégâts **avant absorption**), tableau ABSORPTION, `Text_Health` (PV réellement perdus).
- Vérifier qu'aucune ancienne ligne `Text_Damage` ne reste et que chaque police, couleur et marge se règle bien dans UMG.
- Campagnes de régression après authoring : `Grimrock.RPG.ARMEMENT01.10` et `Grimrock.UI.EventHUD01` ; les nombres augmenteront en conséquence.

Après validation, versionner **uniquement** `WBP_GridCombatTooltip.uasset` et `ST_UI.uasset` modifiés par UE en commit **séparé et ciblé** ; pas de `git add .`, pas de `git restore` des autres assets.

## 5. Critère de clôture

- C++ compilé localement ; **11/11 tests** attendus et régressions sans échec.
- 14 nouvelles clés ST_UI réellement matérialisées.
- WBP designer avec 12 champs indépendants ; l'ancien `Text_Damage` supprimé, sans Graph logique parasite.
- Exemples PIE cohérents avec les données Excel, l'ordre des coefficients et l'absorption.
- Binaires UE sauvegardés/versionnés sélectivement.
