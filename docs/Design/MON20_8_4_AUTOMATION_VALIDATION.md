# MON20.8.4 — Automation / PIE Validation

Date : **24 août 2026**  
Statut : **VALIDÉ UE5.5.4 — CLOS — 8/8 ciblés + PIE**

Validation Automation fournie depuis Unreal Engine 5.5.4 :

```text
Grimrock.MON20.8.SkillsPage
8 / 8 Success
0 Fail
0 Error
```

Tests validés :

```text
DeterministicSkillOrder
DuplicateDefinitionAtomic
MissingRankDefinitionAtomic
SelectedCharacterAuthority
SelectedCharacterIdentity
SkillRanks
TalentPointBalance
TalentProjection
```

Les 16 tests MON20.8.2 / MON20.8.3 avaient déjà été validés lors des passes précédentes. À ce stade, les 24 tests MON20.8 ont donc chacun été validés au moins une fois. La campagne cumulative `Grimrock.MON20.8 = 24/24` reste volontairement réservée à MON20.8.5 avant clôture du jalon MON20.8.

## Validation asset / PIE

`WBP_GridSkills` a été reparenté vers `UGridSkillsWidget` puis validé en PIE.

Le premier rendu après reparent confirmait uniquement la présence du parent natif, sans changement visuel, car le widget C++ calculait le read model mais ne construisait pas encore de présentation UMG. Le correctif runtime a ensuite été introduit par :

```text
7123dc53645e52db34fee448448ceb561fe32239
Render MON20.8.4 skills page natively
```

Le `Border` existant de `WBP_GridSkills` reste le conteneur Blueprint. Au runtime, `UGridSkillsWidget` y construit une vue native scrollable présentant le personnage sélectionné, le budget de talents, les compétences et les talents acquis, sans Graph Blueprint supplémentaire.

Validation PIE fournie le **24 août 2026** :

```text
[OK] Onglet Compétences ouvert depuis le menu principal
[OK] Placeholder Blueprint remplacé au runtime
[OK] Titre "Compétences & talents" visible
[OK] Personnage sélectionné Elias correctement projeté
[OK] Changement de personnage -> Elarion correctement projeté
[OK] Rafraîchissement de la page après changement de personnage
[OK] Points de talent visibles
[OK] Section Compétences visible
[OK] Section Talents acquis visible
```

Les captures montrent actuellement, pour Elias comme pour Elarion :

```text
Points de talent : 0 disponibles — 0 dépensés — 0 accordés
Aucune compétence définie.
Aucun talent acquis.
```

Cet état n'est pas une erreur de présentation : il reflète simplement l'absence actuelle de définitions de compétences de production / rangs attribués et de talents acquis pour ces personnages. Le read model et la présentation se comportent correctement avec un état vide.

Aucune conclusion séparée de compilation autonome n'est enregistrée ici : la validation fournie est Automation + exécution PIE.

## Conclusion

MON20.8.4 est clos. La suite est :

```text
MON20.8.5 — Automation / PIE Regression & Closure
    -> relancer Grimrock.MON20.8
    -> résultat attendu : 24/24 Success
    -> vérifier absence de régression du menu Compétences en PIE
    -> clôturer MON20.8
```
