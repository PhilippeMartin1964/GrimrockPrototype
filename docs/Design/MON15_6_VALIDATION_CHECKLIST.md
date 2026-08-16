# MON15.6 — Validation Checklist

Statut : **IMPLEMENTÉ — EN ATTENTE DE VALIDATION UE5.5.4**  
Date : **16 août 2026**

---

## 1. Compilation

- [ ] `GrimrockPrototype` compile sous UE5.5.4.
- [ ] UHT accepte les nouveaux `USTRUCT` SaveGame.
- [ ] `UGrimrockPartySaveGame::Serialize()` compile et surcharge correctement `UObject::Serialize`.
- [ ] Aucun `.uasset`, `.umap` ou WBP modifié.

---

## 2. Suite MON15.6

Exécuter :

```text
Grimrock.RPG.MON15.6
```

Attendu : 8 tests `Success` :

- [ ] `PersistentChoiceRoundTrip`
- [ ] `LegacyExperienceAheadMigration`
- [ ] `LegacyStoredLevelAheadMigration`
- [ ] `RejectCurrentLevelExperienceMismatch`
- [ ] `RejectInvalidChoiceSnapshot`
- [ ] `PendingLevelUpRoundTrip`
- [ ] `RejectInvalidPendingNotification`
- [ ] `SaveVersionContract`

---

## 3. Régressions MON15

Exécuter :

```text
Grimrock.RPG.MON15.1
Grimrock.RPG.MON15.2
Grimrock.RPG.MON15.3
Grimrock.RPG.MON15.4
Grimrock.RPG.MON15.5
```

- [ ] MON15.1 vert.
- [ ] MON15.2 vert.
- [ ] MON15.3 vert.
- [ ] MON15.4 vert avec compatibilité historique v3.
- [ ] MON15.5 vert avec frontière transient/runtime inchangée avant sérialisation.

---

## 4. Régressions sauvegarde

Exécuter au minimum :

```text
Grimrock.CharacterCreation.CC5
Grimrock.Monsters.MON9
```

- [ ] CC5 SaveGame round-trip vert.
- [ ] MON9 monster/dungeon persistence vert.
- [ ] Les sauvegardes ordinaires sans progression de classe continuent de fonctionner.

---

## 5. Régression combat / requirements

Exécuter :

```text
Grimrock.Monsters.MON12.ActionCatalog
```

- [ ] `Contributions` vert.
- [ ] `GenericAttackLifecycle` vert.
- [ ] Aucun changement de règle MON12.

---

## 6. Migration legacy v1-v3

Vérifications automatisées attendues :

- [ ] `Level=1, XP=6000` migre vers `Level=4, XP=6000`.
- [ ] Le déficit absolu de PV est préservé.
- [ ] Le déficit absolu de mana est préservé.
- [ ] `Level=3, XP=1000` migre vers `Level=3, XP=3000`.
- [ ] Aucun choix de classe n'est inventé pour une sauvegarde legacy.
- [ ] La sauvegarde migrée devient v4.

---

## 7. Validation stricte v4

- [ ] Un couple `Level/Experience` incohérent est refusé sans mutation.
- [ ] Un choix inconnu est refusé.
- [ ] Un choix dupliqué est refusé.
- [ ] Un budget/prérequis invalide est refusé.
- [ ] Une notification Level Up incohérente est refusée.
- [ ] Une sauvegarde v4 valide ne subit aucune migration.

---

## 8. Round-trip des choix

Scénario automatisé :

```text
Level 3
Choice_A + Choice_B confirmés
SaveGameToMemory
ResetRuntimeState
LoadGameFromMemory
```

Attendu :

- [ ] `SaveVersion == 4`.
- [ ] `Choice_A` restauré.
- [ ] `Choice_B` restauré.
- [ ] `Feature_A` restauré dans les requirements.
- [ ] La projection est disponible avant même le rattachement au composant live.

---

## 9. Round-trip notification Level Up

Scénario automatisé :

```text
personnage Level 2
notification persistante 1 -> 2
SaveGameToMemory
clear mirror
LoadGameFromMemory
```

Attendu :

- [ ] une notification est présente dans `PendingLevelUpNotifications`.
- [ ] le `CharacterId` est identique.
- [ ] `PreviousLevel=1`.
- [ ] `NewLevel=2`.
- [ ] le miroir runtime est reconstruit au chargement.

---

## 10. PIE — choix confirmé puis Continue

Préparation recommandée : utiliser le même scénario temporaire de progression que MON15.5.

1. Monter un personnage au niveau 2 ou 3.
2. Confirmer au moins un choix de progression.
3. Sauvegarder.
4. Quitter/revenir au menu ou relancer PIE.
5. Continuer la partie.

Attendu :

- [ ] le niveau et l'XP sont identiques.
- [ ] le choix confirmé est toujours acquis.
- [ ] le choix ne peut pas être acheté une seconde fois.
- [ ] une action exigeant ce `ChoiceId` reste débloquée.
- [ ] log `[GridSaveMigration] ... SourceVersion=4 TargetVersion=4 Migrated=false ... Result=Accepted`.

---

## 11. PIE — sauvegarde avec notification encore disponible

Scénario recommandé :

1. Déclencher un level-up alors qu'un combat est encore actif, afin d'obtenir `Deferred ... Reason=CombatActive`.
2. Sauvegarder avant présentation de la modal si le flux de jeu permet cette sauvegarde.
   - Variante acceptable : sauvegarder alors que la modal Level Up est affichée si le menu de sauvegarde est accessible.
3. Recharger / Continue.

Attendu :

- [ ] `PendingLevelUpNotifications=1` dans le snapshot v4.
- [ ] après chargement : `[GridLevelUpUI] Restored Pending=1`.
- [ ] la notification n'est pas ouverte avant restauration du groupe.
- [ ] si le combat est actif, elle reste différée.
- [ ] au premier point sûr, la modal s'ouvre une seule fois.
- [ ] après confirmation ou annulation, une nouvelle sauvegarde ne contient plus cette notification.

---

## 12. Aucun changement de contenu UE

- [ ] Aucun DataAsset modifié.
- [ ] Aucun Blueprint modifié.
- [ ] Aucun WBP modifié.
- [ ] Aucune map modifiée.

---

## 13. Porte de clôture

MON15.6 pourra être marqué **VALIDÉ ET CLOS** lorsque toutes les cases ci-dessus nécessaires au runtime réel sont validées et que les logs Automation + PIE ont été fournis.

La suite sera :

**MON15.7 — équilibrage et clôture de MON15.**
