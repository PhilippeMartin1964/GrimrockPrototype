# UI-RPG06.3A — Finalisation du contrat COMPÉTENCES

Date : **7 octobre 2026**  
Parent : **UI-RPG06 — Unification Compétences + Talents UX**  
État : **SOURCE IMPLÉMENTÉE — validation locale requise**

## Objectif

Clore le contrat technique de la page COMPÉTENCES après la matérialisation
réussie de `WBP_RPGSkillEntry` et de la liste scrollable de
`WBP_GridSkills`.

Cette tranche ne crée aucune nouvelle règle gameplay.

## Ordre joueur

L'ancien ordre était déterministe mais technique :

```text
SkillId
```

La page trie désormais sur :

```text
DisplayName (case-insensitive)
puis SkillId comme tie-break
```

Le catalogue reste inchangé. Aucun champ `DisplayOrder` supplémentaire n'est
ajouté et aucune seconde autorité de catalogue n'est créée.

## Contrat UMG désormais obligatoire

Après validation PIE de UI-RPG06.2C, les bindings temporaires optionnels
deviennent obligatoires.

`WBP_GridSkills` :

```text
Panel_SkillEntries
Text_EmptySkills
```

`WBP_RPGSkillEntry` :

```text
Text_SkillName
Text_SkillAttribute
Text_SkillRank
Text_SkillTrainingPolicy
Text_SkillDescription
```

Une suppression ou un renommage doit désormais faire échouer la compilation du
Widget Blueprint au lieu de masquer silencieusement le défaut.

## Compatibilité Talent plate

L'audit source montre que `FGridTalentEntryView`,
`FGridSkillsPageView::Talents`, `GetTalentEntryCount()` et
`GetTalentEntry()` ne sont plus consommés par la présentation C++ actuelle.

Ils restent toutefois Blueprint-readable et peuvent être référencés dans des
`.uasset`. Ils sont donc volontairement conservés dans UI-RPG06.3A.

La réduction de cette dette exige un audit explicite des références Blueprint
binaires ; elle ne doit pas être réalisée à l'aveugle.

## Validation locale

Après pull :

```powershell
cd D:\Development\GrimrockPrototype

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.RPG06.Skills"
```

Puis régression :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.MON20.8.SkillsPage"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.RPG04"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.RPG05"
```

PIE final :

```text
K ouvre Skills
COMPÉTENCES affiche les 25 entrées par ordre de libellé
scroll vertical jusqu'à la dernière entrée
changement de personnage rafraîchit rangs/statuts
TALENTS commute toujours correctement
acquisition simple et variantes toujours fonctionnelles
toast progression toujours fonctionnel
```

## Suite

UI-RPG06.3B est uniquement une tranche de régression/clôture documentaire si
les validations ci-dessus sont vertes.
