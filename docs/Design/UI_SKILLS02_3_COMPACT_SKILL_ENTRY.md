# UI-SKILLS02.3 — Compact Skill Entry

Date : **15 octobre 2026**  
Parent : **UI-SKILLS02 — Skill Browser & Detail Panel**  
Prérequis : **UI-SKILLS02.2 validé localement : Skills02 7/7 ; RPG.SKILL01 8/8 ; MON20.8 SkillsPage 8/8 ; RPG06 Skills 7/7**  
État : **SOURCE IMPLÉMENTÉE — validation locale requise**

## 1. Objectif

Préparer la ligne de compétence compacte du futur navigateur Skills sans
modifier à l'aveugle WBP_RPGSkillEntry.

UI-SKILLS02.3 ajoute au widget C++ existant :

- sélection par clic sur la ligne ;
- états sémantiques Normal / Hovered / Selected ;
- icône de présentation optionnelle ;
- deux overlays Designer optionnels pour hover et sélection ;
- statut d'entraînement plus court et plus lisible ;
- suppression visuelle de la description longue dans la ligne ;
- synchronisation de la sélection avec UGridSkillsWidget ;
- conservation des transactions - / + existantes.

Aucune règle gameplay n'est déplacée dans UMG.

## 2. Interaction de ligne

UGridSkillEntryWidget diffuse désormais :

~~~text
OnSelectSkillRequested(SkillId)
~~~

Le clic gauche sur le corps de la ligne appelle RequestSkillSelection().

Les boutons - et + restent des enfants interactifs autonomes et conservent leurs
delegates existants. La sélection n'est pas une transaction : elle ne modifie
ni SkillRanks, ni points, ni SaveGame.

UGridSkillsWidget reçoit la demande via :

~~~text
HandleSkillSelectionRequested()
    -> SelectSkill()
~~~

## 3. États visuels

Nouvelle sémantique :

~~~text
EGridSkillEntryVisualState
    Normal
    Hovered
    Selected
~~~

Priorité :

~~~text
Selected > Hovered > Normal
~~~

C++ ne définit aucune couleur ni brush. Il ne fait qu'exposer l'état et piloter
la visibilité de deux zones optionnelles :

~~~text
Border_SkillSelection
Border_SkillHover
~~~

Leur apparence restera entièrement Designer/UMG lors de UI-SKILLS02.5.

## 4. Icône

UGridSkillEntryWidget expose :

~~~text
SkillIcon
SetSkillIcon()
Image_SkillIcon   [BindWidgetOptional]
~~~

Pendant RebuildSkillEntryWidgets(), UGridSkillsWidget recherche la métadonnée
correspondante dans SkillPresentationAsset et transmet l'icône à la ligne.

Tant que DA_RPGSkillPresentation n'est pas authoré en UI-SKILLS02.6, l'icône est
simplement absente et Image_SkillIcon reste masquée.

Aucune seconde table d'icônes n'est introduite.

## 5. Statut d'entraînement compact

La ligne utilise maintenant la même sémantique que le detail read model :

~~~text
Rank > 0
    -> Entraînée

Rank == 0 && bAllowUntrainedChecks
    -> Utilisable sans entraînement

Rank == 0 && !bAllowUntrainedChecks
    -> Entraînement requis
~~~

Le détail explicatif complet reste destiné au panneau latéral. La ligne ne
présente qu'un statut court.

## 6. Description longue

Text_SkillDescription reste temporairement un BindWidget obligatoire afin de
ne pas casser WBP_RPGSkillEntry avant la matérialisation Designer.

Cependant, UI-SKILLS02.3 le vide et le Collapse systématiquement.

La description canonique reste présente dans FGridSkillEntryView et
FGridSkillDetailView ; elle n'est donc ni perdue ni dupliquée.

La suppression physique du TextBlock du WBP sera effectuée en UI-SKILLS02.5,
après mise à jour du contrat C++ correspondant.

## 7. Synchronisation de sélection

Lors de RebuildSkillEntryWidgets() :

~~~text
Entry.SkillId == SelectedSkillId
    -> SetSelected(true)
~~~

SelectSkill() et ClearSkillSelection() appellent ensuite :

~~~text
RefreshSkillEntrySelection()
~~~

La sélection peut donc évoluer sans reconstruire toute la liste.

Les boutons - / + continuent également à sélectionner leur Skill avant
transaction, conformément à UI-SKILLS02.2.

## 8. Bindings optionnels préparés

UI-SKILLS02.3 n'exige encore aucune modification binaire.

Les bindings suivants sont ajoutés comme BindWidgetOptional :

~~~text
Image_SkillIcon
Border_SkillSelection
Border_SkillHover
~~~

Ils deviendront visibles uniquement après leur création manuelle dans le
Designer.

Les bindings historiques obligatoires restent inchangés pour éviter toute
régression de WBP_RPGSkillEntry.

## 9. Tests

Trois nouveaux tests sont ajoutés :

~~~text
Grimrock.UI.Skills02.CompactEntry.Semantics
Grimrock.UI.Skills02.CompactEntry.StateAndIcon
Grimrock.UI.Skills02.CompactEntry.WidgetContract
~~~

Ils vérifient notamment :

- les trois états d'entraînement ;
- le nouveau libellé Utilisable sans entraînement ;
- la priorité Selected > Hovered ;
- SetSelected() ;
- SetSkillIcon() ;
- RequestSkillSelection() ;
- le reset de ClearSkillEntry() ;
- l'existence du delegate de sélection ;
- les trois BindWidgetOptional préparés ;
- l'existence du handler parent.

## 10. Hors scope

UI-SKILLS02.3 ne :

- modifie aucun .uasset ;
- ne crée aucune icône de production ;
- ne crée pas WBP_RPGSkillDetail ;
- ne change pas la formule des tests ;
- ne change pas les points ou plafonds ;
- ne définit aucune couleur ou texture de sélection en C++ ;
- ne supprime pas encore physiquement Text_SkillDescription ;
- ne modifie pas le panneau de détail.

## 11. Validation locale

~~~powershell
cd D:\Development\GrimrockPrototype

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.Skills02"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.RPG06.Skills"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.SKILL01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.MON20.8.SkillsPage"
~~~

Le premier filtre doit passer de 7 à **10 tests**.

Ne pas considérer UI-SKILLS02.3 validé avant réception des sorties locales.

## 12. Suite

UI-SKILLS02.4 — Skill Detail Widget introduira le widget C++ de détail et son
contrat BindWidget, toujours sans modifier à l'aveugle les assets binaires.

UI-SKILLS02.5 matérialisera ensuite manuellement dans Unreal :

- la ligne 48x48 avec icône ;
- les overlays hover / selected ;
- le layout compact ;
- le panneau latéral complet ;
- la suppression du Text_SkillDescription historique.
