# RPG-DEV01 — Simulateur de progression PIE

Date : **8 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
État : **SOURCE IMPLÉMENTÉE — validation locale requise**

## 1. Objectif

Permettre de tester en PIE la progression d'un personnage jusqu'au niveau 20
sans dépendre des combats, des monstres ou des quêtes.

RPG-DEV01 ne crée aucune seconde progression.

Le flux reste :

```text
commande PIE
    -> XP cumulative exacte du niveau cible
    -> FRPGLevelUpService
    -> Level
    -> DerivedStats / Resources
    -> FRPGClassProgressionTransactionService::RefreshCharacterProjection()
    -> notification Level Up
    -> UI Compétences / Talents
```

Le champ `Level` n'est jamais modifié directement par la commande.

## 2. Commande

Disponible uniquement hors Shipping et uniquement dans un monde PIE :

```text
Grimrock.RPG.SetSelectedLevel <niveau>
```

Exemples utiles pour l'arbre de talents :

```text
Grimrock.RPG.SetSelectedLevel 2
Grimrock.RPG.SetSelectedLevel 6
Grimrock.RPG.SetSelectedLevel 10
Grimrock.RPG.SetSelectedLevel 14
Grimrock.RPG.SetSelectedLevel 18
Grimrock.RPG.SetSelectedLevel 20
```

La commande cible toujours le personnage actuellement sélectionné par
`UGridPartyInventoryComponent::SelectedCharacterIndex`.

## 3. Seuils XP utilisés

Les règles existantes sont conservées :

```text
Niveau  1 :      0 XP
Niveau  2 :  1 000 XP
Niveau  6 : 15 000 XP
Niveau 10 : 45 000 XP
Niveau 14 : 91 000 XP
Niveau 18 :153 000 XP
Niveau 20 :190 000 XP
```

La formule canonique reste portée par
`URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel()`.

## 4. Procédure recommandée en PIE

1. lancer le PIE ;
2. sélectionner le personnage à tester ;
3. exécuter `Grimrock.RPG.SetSelectedLevel 2` ;
4. ouvrir `K` et tester l'arbre ;
5. répéter directement avec 6, 10, 14, 18 puis 20.

Depuis RPG-LEVELUX01, il n'existe plus de popup Level Up à fermer :
`LastAcknowledgedLevel` est synchronisé automatiquement avec `Level` et les
montées successives sont autorisées immédiatement.

## 5. Sécurité des sauvegardes

Une simulation réussie désarme la sauvegarde sur le pawn PIE :

```text
bAutoSaveOnInventoryClose = false
PartySaveSlotName         = vide
```

Conséquences pour la session PIE simulée :

- fermer l'inventaire ne déclenche pas l'autosave du slot réel ;
- `EndPlay` ne sauvegarde pas le personnage simulé dans le slot réel ;
- les checkpoints qui dépendent du slot actif ne doivent pas remplacer la
  sauvegarde réelle.

Ces changements appartiennent uniquement au pawn du monde PIE.

Si la tentative de simulation échoue, le nom du slot et la politique
d'autosave précédents sont restaurés immédiatement.

## 6. Refus intentionnels

RPG-DEV01 refuse :

- une utilisation hors PIE ;
- un niveau hors 1..20 ;
- un niveau inférieur ou égal au niveau courant ;
- un état `Level / Experience` incohérent ;
- toute progression rejetée par `FRPGLevelUpService`.

Il n'existe volontairement aucune commande de démotion.

Pour recommencer au niveau 1, relancer le PIE depuis un état propre.

## 7. Ce que le simulateur permet de valider

À chaque palier, vérifier notamment :

- niveau et XP ;
- HP / Mana recalculés ;
- Talent Points disponibles ;
- états Disponible / Niveau / Prérequis / Points / Exclusif ;
- acquisitions simples ;
- acquisitions à variantes ;
- refresh de `WBP_GridSkills` ;
- notification de montée de niveau ;
- comportement au niveau 20.

RPG-DEV01 n'invente aucune économie : les Skill Points et achats de rang sont
désormais fournis séparément par RPG-SKILL01.

## 8. Automation

Filtre :

```text
Grimrock.RPG.DEV01
```

Tests attendus :

```text
ProgressionSimulator.Level20
ProgressionSimulator.RejectInvalidTarget
ProgressionSimulator.SuccessiveNonModalJumps
ProgressionSimulator.RollbackOnLevelUpFailure
ConsoleCommand.Registered
```

Validation locale :

```powershell
cd D:\Development\GrimrockPrototype

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.DEV01"
```

RPG-DEV01 n'est clos qu'après fourniture du log local et un essai PIE réel
jusqu'à au moins un palier de Talent.
