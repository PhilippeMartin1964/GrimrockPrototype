# Contrat permanent — C++ comportemental / WBP Designer présentation

**Décision corrigée du 24 octobre 2026 — applicable aux futurs WBP et aux refontes.**  
**Erreur explicitement annulée :** « toute mise en forme de texte, création de lignes et visibilité doit être programmée dans l'Event Graph ». Cette lecture était fausse et a entraîné un câblage inutilement long de ARMEMENT01.10.3.3.

## 1. Principe réel

**C++ gère le comportement de l'interface. Le WBP Designer en définit l'aspect. Le Graph Blueprint n'est pas une obligation et doit rester minimal lorsque le C++ réalise déjà la logique.**

| C++ : logique et alimentation | WBP Designer : aspect et structure | Graph : seulement si utile |
|---|---|---|
| Calculs métier, événements, snapshots, état | Hiérarchie des conteneurs et composants existants | Animation ou interaction spécifique qui justifie Blueprint |
| Détermination des valeurs et des lignes à afficher | Widget Blueprint de ligne réutilisable | Pas de copie de boucles déjà codées en C++ |
| Composition des textes dynamiques via ST_UI et FText::Format | TextBlocks éditables, styles, polices, couleurs, graisses | Aucun Format Text répétitif si C++ le fait |
| SetText sur TextBlocks existants exposés par BindWidget | Ancrages, alignement, largeur, hauteur, marges | Pas de remplacement de SetText natif |
| CreateWidget d'une classe WBP authorée ; AddChild à un conteneur authoré | La classe créée possède tous ses TextBlocks et son layout | Pas de création de ligne en graph imposée |
| SetVisibility sur une section complète selon données | Section regroupée dans un conteneur éditable | Aucun binding Tick pour masquer une section |
| Formatage métier exact, cas limites et localisation | Possibilité de modifier la palette UMG sans recompiler | Extensions ciblées, documentées, testables |

### Interdit en C++ lors de l'authoring d'un nouveau WBP

- Construire des TextBlocks individuels ou des cellules, en-têtes, colonnes et grilles de layout avec `WidgetTree->ConstructWidget`, `UGridPanel::AddChildToGrid` ou des appels équivalents.
- Imposer une police, une taille de police, une couleur, un alignement, un padding, une largeur, un style ou une bordure à un widget du Designer à l'exécution.
- Répliquer un arbre visuel qui devrait être défini et modifiable dans le **Designer**.
- Dupliquer dans le Graph l'intégralité de la logique déjà implémentée en C++.

### Autorisé et attendu en C++

- Lire `USTRUCT`, collecter les résultats, choisir les textes localisés `ST_UI`, produire un `FText` via `FText::Format`, gérer les flags et les messages d'état.
- `SetText` pour remplir un `TextBlock` *dynamique* existant, récupéré via `UPROPERTY(meta=(BindWidget))` ou `BindWidgetOptional`.
- `SetVisibility(Visible/Collapsed)` sur un **conteneur complet prédéfini dans le Designer**, sans paramétrage individuel de sa géométrie.
- `CreateWidget<WBPRow>(...)` pour instancier un **WBP de ligne complet déjà mis en page dans le Designer** ; `AddChildToVerticalBox` pour le rattacher à son conteneur WBP. Pas de création de TextBlocks dans ce processus.
- Vérifier les éléments indispensables avant présentation ; en cas d'asset BP incomplet, utiliser le présentateur textuel de repli.

## 2. Patron de référence : Combat Tooltip

1. **WBP_GridCombatTooltip**, parent C++ `UGridCombatTooltipWidget` : le Designer possède la structure des sections et les huit champs texte ; les **quatre colonnes** sont visuelles, pas assemblées dans le C++.
2. **WBP_GridCombatArmorRow**, parent C++ `UGridCombatArmorRowWidget` : le Designer contient `Text_Source`, `Text_Before`, `Text_Absorbed`, `Text_After` dans ses quatre `SizeBox`. Le C++ alimente ces champs via `SetArmorData`.
3. Le C++ principal calcule le contenu (jet, Défense, dégâts, PV et statuts), affecte `SetText` aux TextBlocks du Designer, gère une seule visibilité pour la section ABSORPTION et **instancie le WBP de ligne** pour chaque source réelle qui a absorbé des dégâts.
4. Ni `GridPanel`, ni `TextBlock`, ni font, ni largeur, ni marge ne sont créés ou modifiés par le C++.
5. **Event Graphs non nécessaires** pour ce jalon. Les Graphs historiques d'ARMEMENT01.10.3.3 doivent être supprimés depuis Unreal Editor après recompilation/reparenting.
6. L'ancien `PresentCombatTooltip` est conservé temporairement **seulement pour compatibilité de sérialisation** des WBP existants ; le présentateur natif ne l'appelle plus.

## 3. Localisation : règle complémentaire, non concurrente

Voir [UI_WBP_LOCALIZATION_WORKFLOW.md](UI_WBP_LOCALIZATION_WORKFLOW.md).

- **Texte statique** : dans le Designer, propriété `Text` liée directement à la clé `ST_UI` (pas de SetText C++ redondant).
- **Texte dynamique** (calculs, dégâts, PV, message de statut) : le C++ lit les patterns existants de `ST_UI` avec `FUILocalizationText::Get`, construit le `FText` avec `FText::Format`, puis remplit un TextBlock existant via `SetText`.
- **Noms d'objets** : reprendre leur `FText` authoré dans les données métier, et le nom générique de repli depuis `ST_UI`.
- **Chaîne CSV → ST_UI → WBP → traduction Game (fr/en)** : respecter le workflow complet. Un `UI.csv` committé ne matérialise pas `ST_UI.uasset`.

## 4. Checklist obligatoire

- [ ] Le C++ gère effectivement tous les calculs et textes dynamiques.
- [ ] Les TextBlocks et conteneurs visuels sont déclarés dans le Designer, avec leur police, taille, marge et alignement modifiables.
- [ ] **Une information visuelle que l'on doit styliser/repositionner indépendamment = un TextBlock distinct dans le Designer**. Ne pas concaténer de multiples champs sémantiques dans un seul FText multiligne si l'utilisateur veut modifier séparément ces lignes (exemple ARMEMENT01.10.3.3.8 Attaque/Défense).
- [ ] Les textes dynamiques sont localisés et alimentés par C++ par composant ; ne pas créer d'Event Graph pour recréer cette ventilation.
- [ ] Aucune création de cellule / colonne / police / padding par C++.
- [ ] Pour chaque liste, la classe native instancie **un WBP de ligne authoré** ; elle ne crée pas de TextBlocks individuels.
- [ ] Le C++ peut changer **le contenu** et **la visibilité d'une section** sans imposer sa géométrie.
- [ ] L'Event Graph ne réimplémente pas une logique déjà native ; toute exception est motivée.
- [ ] Les nouvelles clés et tous les textes dynamiques sont localisables via `ST_UI` ; asset matérialisé dans UE si nécessaire.
- [ ] En cas de WBP non configuré, un repli fonctionnel existe ; pas de tooltip vide.
- [ ] Compilation et tests C++ validés par les logs utilisateur ; disposition/typographie validées par capture en PIE.
- [ ] Pas de changement direct des binaires `.uasset` en dehors de l'éditeur UE.
