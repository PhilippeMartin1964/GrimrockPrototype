# UI-RPG-VISUAL01.4 — Final visual/code/data audit and cleanup

Date : **12 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Ticket : **UI-RPG-VISUAL01.4**

## 1. Objet

Clore le chantier VISUAL01 sur une base propre avant de déplacer le contenu
français des `RPG*Authoring.cpp` vers les Data Assets authorés.

L'audit couvre :

- cohérence du pipeline Talent C++ ;
- restes de migration / code mort ;
- autorité unique du vocabulaire Talent ;
- fallbacks d'identifiants techniques ;
- couverture sérialisée des assets visuels ;
- préparation des `DA_Class_*` au futur chantier de texte authoré ;
- compatibilité des WBP après suppression de propriétés réfléchies devenues
  inutiles.

## 2. Bilan VISUAL01.1 → VISUAL01.4

### VISUAL01.1 — identité visuelle Talent

Réalisé :

- résolution des icônes par `TalentNodeId` canonique ;
- icône de détail ;
- classe / bannière / drapeau / accent via `URPGClassVisualAsset` ;
- chrome UMG finalisé côté Designer.

### VISUAL01.2 — lignes sémantiques et glyphes

Réalisé :

- `FGridTalentDetailLineView` ;
- `UGridTalentDetailLineWidget` ;
- `URPGTalentDetailGlyphSet` ;
- lignes structurées label / valeur / glyphe ;
- couverture des coûts, cibles, portée, zone, LOS, cooldown, prérequis,
  exclusivité et recettes.

### VISUAL01.3 — fiche structurée et variantes

Réalisé :

- EFFETS / UTILISATION / ACQUISITION structurés ;
- variantes visibles simultanément ;
- suppression des anciens TextBlocks/fallbacks de détail ;
- carte variante compacte ;
- formatage final laissé à l'UMG ;
- aucune ComboBox de variante.

### VISUAL01.4 — audit final

Ce ticket :

- supprime les miroirs numériques de migration
  `ResolvedNumericValue`, `ResolvedValueStyle`,
  `bShowPositiveSign`, `bShowPercentUnit` ;
- conserve `FGridTalentDetailLineView` comme unique donnée numérique de ligne ;
- supprime le `DetailTextSet` configurable des widgets Talent ;
- utilise une seule autorité :
  `URPGTalentDetailTextSet::ResolveProductionText()` ;
- retire les fallbacks joueur vers `ChoiceId`, `TalentBranchId` et
  `ClassId` dans la présentation Talent ;
- rend obligatoire l'override conceptuel de
  `DA_RPGTalentPresentation` pour les quatre familles à variantes ;
- retire le fallback `ConceptualNodeDisplayName()`, qui reconstruisait un nom
  conceptuel en découpant un libellé ;
- corrige le chemin obsolète de `WBP_GridSkills` dans le test de compatibilité
  Blueprint ;
- recompile désormais les WBP Talent principaux dans
  `Grimrock.UI.RPG.CODEAUDIT01.BlueprintCompatibility`.

## 3. Audit des Data Assets sérialisés

Le nouveau filtre :

~~~text
Grimrock.UI.RPG.VISUAL01.FinalAudit
~~~

charge réellement les `.uasset` via Unreal.

Il vérifie :

~~~text
6 DA_Class_*
6 DA_ClassVisual_*
DA_RPGTalentDetailGlyphSet

6 classes
18 branches
90 TalentNodeId
18 backgrounds de branche
90 icônes de Talent
6 ClassIcon
6 Banner
6 Flag
14 glyphes de détail
~~~

Pour chaque classe visuelle :

- aucun ID vide ;
- aucun ID de branche ou de Talent dupliqué ;
- correspondance exacte avec les IDs du `DA_Class_*` ;
- aucune référence texture attendue nulle ;
- chaque texture référencée doit pouvoir être chargée.

Le même audit vérifie que les `DisplayName` / `Description` des
`ProgressionChoices` et `CombatActions` sont bien présents dans les
`DA_Class_*`. Cela prépare le futur transfert d'autorité du texte authoré sans
encore modifier ces assets.

## 4. Data Assets : sérialisation et validation

Les Data Assets Unreal sont des objets sérialisés dans des `.uasset`. Le dépôt
les stocke sous Git LFS, ce qui signifie que GitHub montre principalement un
pointeur binaire et non une représentation textuelle exploitable.

La validation correcte consiste donc à :

~~~text
UnrealEditor-Cmd
    -> LoadObject<URPGClassAsset>()
    -> lire les UPROPERTY sérialisées
    -> Automation assertions
~~~

Le script :

~~~powershell
.\Scripts\ValidateUIRPGVisual.ps1 -EngineRoot D:\UE_5.5
~~~

automatise cette validation sans réenregistrer les assets.

## 5. Autorités après cleanup

~~~text
DA_Class_*
    -> identité / mécanique Talent et Action
    -> FText authorés actuellement matérialisés

DA_RPGTalentPresentation
    -> ordre des branches
    -> noms/principes conceptuels des 4 familles à variantes

DA_ClassVisual_*
    -> ClassIcon / Banner / Flag
    -> 3 backgrounds
    -> 15 Talent icons

DA_RPGTalentDetailGlyphSet
    -> glyphes sémantiques communs

DA_RPGTalentDetailTextSet
    -> vocabulaire UI générique Talent

C++
    -> read-model / mécanique / nombres / notation universelle
    -> mapping état sémantique -> style configuré, sans RGB d'état codé

Config/DefaultGame.ini
    -> couleurs/tints des états de nœud Talent

UMG
    -> composition / taille / couleur / typographie / spacing
    -> fallback Designer si un style configuré est indisponible
~~~

## 6. Point restant volontairement hors VISUAL01

`UGridTalentDetailWidget::MakeVariantDisplayLabel()` retire encore le préfixe
conceptuel d'un **DisplayName joueur déjà authoré** pour obtenir le titre compact
de la variante.

Exemple :

~~~text
"Spécialisation martiale — Tranchant"
    -> "Tranchant"
~~~

Il ne parse plus aucun `ChoiceId` et n'est pas un fallback gameplay. Néanmoins,
le contrat idéal reste de ne pas analyser un libellé.

Sa suppression appartient au prochain chantier de contenu
`UI-RPG-CONTENT-TEXT01`, lorsque les `DA_Class_*` deviendront l'autorité
directe du texte authoré et pourront stocker une présentation de variante qui
n'exige plus cette normalisation.

## 7. Validation attendue

~~~powershell
git pull --ff-only

.\Scripts\ValidateUIRPGVisual.ps1 -EngineRoot D:\UE_5.5
~~~

La clôture n'est acquise qu'après retour des sorties locales.

Aucun `.uasset` n'est modifié par ce ticket.
