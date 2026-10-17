# UI-SKILLS-CODE-AUDIT02.7 — Skill Error Boundary & Localization Polish

Date : **17 octobre 2026**  
État : **VALIDÉ / CLOS**

## 1. Défaut de présentation

Les transactions Skill utilisaient encore :

~~~text
FText::FromName(SkillId)
~~~

comme fallback de nom pour les notifications d'achat/remboursement.

Une transaction pouvait rester valide via la définition canonique alors que la
ligne correspondante n'était momentanément plus présente dans le read-model UI.
Une notification de succès pouvait alors exposer un identifiant technique tel
que `Skill_Lockpicking`.

Ce comportement contredisait le contrat déjà appliqué aux lignes Skills :
`Skill.Fallback.Unknown` est le fallback joueur.

## 2. Boundary final

Le nouveau resolver de feedback applique strictement :

~~~text
1. FGridSkillEntryView.DisplayName
2. URPGSkillAsset::DisplayName du même SkillId
3. Skill.Fallback.Unknown
~~~

Le `SkillId` n'est jamais converti en texte joueur. Il reste disponible pour
l'identité, le logging et les diagnostics.

Une définition canonique dont le `SkillId` ne correspond pas à la requête est
également ignorée.

## 3. Validation d'entrée centralisée

`UGridSkillEntryWidget` possédait encore son propre `IsValidEntry()`, plus
ancien et plus permissif que `FGridSkillEntryView::IsValid()`.

02.7 supprime ce duplicata. L'initialisation d'une ligne utilise désormais
directement :

~~~text
InEntry.IsValid()
~~~

Le read-model est donc l'unique autorité structurelle de la ligne, y compris
pour l'invariant `Rank <= CurrentRankCap` introduit en 02.2.

## 4. Test

Nouveau test :

~~~text
Grimrock.UI.Skills02.ErrorBoundary.PlayerFacingSkillName
~~~

Il vérifie :

~~~text
nom projeté                   -> priorité 1
nom du Data Asset canonique   -> fallback si ligne absente/vide
nom canonique d'un autre ID   -> refusé
présentation absente          -> Skill.Fallback.Unknown
notification de succès        -> aucun SkillId technique
ligne Rank > CurrentRankCap   -> refusée par read-model et widget
~~~

La suite `Grimrock.UI.Skills02` passe de 28 à **29 tests** si aucun autre test
n'est ajouté parallèlement.

## 5. Non-objectifs

02.7 ne modifie pas :

~~~text
FRPGSkillPointService
règles de transaction
textes LOCTEXT du service de feedback
UI.csv / ST_UI
affinités
tri
undo
cache canonique
UMG / .uasset
~~~

## 6. Validation locale

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Skills02"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.RPG05.Feedback"
~~~

Validation locale reçue :

~~~text
Grimrock.UI.Skills02       29/29 PASS
Grimrock.UI.RPG05.Feedback  4/4 PASS
warnings                    0
failures                    0
~~~

Rapports :

~~~text
Skills02       TD04-20261017-155827
RPG05.Feedback TD04-20261017-155845
~~~

État final : **UI-SKILLS-CODE-AUDIT02.7 VALIDÉ / CLOS**.
