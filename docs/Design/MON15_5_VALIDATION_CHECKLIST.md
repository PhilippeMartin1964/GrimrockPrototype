# MON15.5 — Validation Checklist

Statut : **à valider sous Unreal Engine 5.5.4**.

Ne pas commencer MON15.6 avant validation de cette checklist.

---

## 1. Compilation

- [ ] Compiler `GrimrockPrototype` sous UE5.5.4 / Visual Studio.
- [ ] Aucun warning/error UHT dans les nouveaux `USTRUCT` / `UCLASS`.
- [ ] Vérifier les includes Slate/UMG du widget natif.
- [ ] Aucun `.uasset`, `.umap` ou WBP requis.

---

## 2. Tests dédiés MON15.5

Exécuter :

```text
Grimrock.RPG.MON15.5
```

Attendus :

- [ ] `AtomicBatchCommit` — Success
- [ ] `AtomicFailure` — Success
- [ ] `WidgetCancelIsNonMutating` — Success
- [ ] `WidgetConfirmTransaction` — Success
- [ ] `CombatCatalogUnlock` — Success
- [ ] `LevelUpNotificationSource` — Success
- [ ] `CharacterIsolation` — Success
- [ ] `TransientPersistenceBoundary` — Success

Aucun assert, ensure, breakpoint de sécurité ou exception ne doit être forcé
avec `Continuer`.

---

## 3. Transaction par lot

Avec la fixture niveau 3 :

```text
Granted = 2
Choice_A coût 1
Choice_B coût 1, prerequisite Choice_A
```

- [ ] `{Choice_A, Choice_B}` réussit en une transaction.
- [ ] Les deux choix sont présents après commit.
- [ ] `Spent=2`, `Remaining=0`.
- [ ] Le delegate de commit n'est émis qu'après validation complète.

---

## 4. Échec atomique

Tester un lot dépassant le budget :

```text
{Choice_A, Choice_Expensive}
```

- [ ] Transaction refusée.
- [ ] Raison `InsufficientChoicePoints`.
- [ ] `Choice_A` n'est pas partiellement confirmé.
- [ ] Aucun requirement de choix refusé n'est projeté.
- [ ] Les grants automatiques valides restent disponibles.

---

## 5. Modal staged

- [ ] Un clic ajoute seulement un choix pending.
- [ ] Aucun choix runtime n'est acquis avant `Confirmer`.
- [ ] Un prérequis pending peut rendre un choix dépendant pending sélectionnable.
- [ ] Un prérequis pending ne peut pas être retiré si un autre pending en dépend.
- [ ] `Annuler` vide le staging sans mutation autoritaire.
- [ ] `Confirmer` applique tout le staging d'un coup.
- [ ] Une confirmation refusée laisse la modal ouverte.

---

## 6. Comparaison avant / après

- [ ] `PreviousLevel` et `NewLevel` sont corrects.
- [ ] PV max avant/après proviennent de `CalculateDerivedStats`.
- [ ] Mana max avant/après proviennent de `CalculateDerivedStats`.
- [ ] Armure physique/magique avant/après sont cohérentes.
- [ ] Aucun bonus d'équipement n'est bake dans les stats de base.
- [ ] Les points affichés correspondent au niveau final.

---

## 7. Notification level-up

- [ ] Le delegate MON15.3 historique continue de fonctionner.
- [ ] Le delegate MON15.5 source-aware est émis une fois par transaction level-up.
- [ ] Il transporte le bon `UGridPartyInventoryComponent`.
- [ ] Il transporte les bons niveaux précédent/nouveau.
- [ ] La projection de requirements est prête avant présentation de la modal.

---

## 8. Catalogue combat

- [ ] Avant confirmation, une action `Requirements=[Choice_A]` est `MissingRequirement`.
- [ ] Après confirmation, la même action devient disponible si le reste du contexte l'autorise.
- [ ] Les grants automatiques de niveau sont eux aussi ajoutés au contexte.
- [ ] `FGridCombatActionDefinition::Requirements` reste l'unique contrat.
- [ ] Aucun second catalogue/circuit d'exécution n'est créé.

---

## 9. Isolation personnages

- [ ] Les choix sont indexés par `CharacterId` stable.
- [ ] Le choix de Character 0 ne change pas Character 1.
- [ ] Une projection obsolète appartenant à un autre composant n'est pas réutilisée.

---

## 10. Input / modalité PIE

Pendant la modal :

- [ ] déplacement impossible ;
- [ ] rotation impossible ;
- [ ] touche 0–9 hotbar sans effet gameplay ;
- [ ] clic monde sans interaction ;
- [ ] souris utilisable dans la modal ;
- [ ] ciblage combat annulé/masqué correctement.

Après fermeture :

- [ ] déplacement/rotation restaurés ;
- [ ] hotbar restaurée ;
- [ ] interaction monde restaurée ;
- [ ] état précédent de l'inventaire cohérent.

---

## 11. File de notifications PIE

Avec deux personnages franchissant un seuil dans la même distribution XP :

- [ ] une seule modal à la fois ;
- [ ] la seconde attend la fermeture de la première ;
- [ ] les index/personnages affichés ne sont pas mélangés.

Si cette situation est difficile à produire manuellement, la validation du
mécanisme de queue peut être complétée par logs diagnostics, mais aucune double
modal ne doit être observée.

---

## 12. SaveGame — frontière volontaire

- [ ] `UGrimrockPartySaveGame::CurrentSaveVersion == 3`.
- [ ] Aucun nouveau champ SaveGame en MON15.5.
- [ ] Les choix confirmés sont runtime seulement dans ce sous-jalon.
- [ ] Un reset du registry runtime perd volontairement ces choix.
- [ ] Cette perte est documentée et sera corrigée par MON15.6.

Ne pas présenter MON15.5 comme une persistance finale des choix.

---

## 13. Régressions

Exécuter au minimum :

```text
Grimrock.RPG.MON15.1
Grimrock.RPG.MON15.2
Grimrock.RPG.MON15.3
Grimrock.RPG.MON15.4
Grimrock.CharacterCreation.CC2
Grimrock.CharacterCreation.CC5
Grimrock.CharacterCreation.CC6
Grimrock.Monsters.MON12.ActionCatalog
Grimrock.Monsters.MON12.8
```

- [ ] MON15.1–15.4 verts.
- [ ] CharacterCreation pertinente verte.
- [ ] ActionCatalog vert.
- [ ] MON12.8/hotbar vert.

---

## 14. PIE minimal conseillé

Pour éviter de modifier durablement les assets, utiliser temporairement une
classe de test ou une classe existante puis **ne pas sauvegarder/pousser** les
changements `.uasset` :

```text
Level 2 grant : +1 ChoicePoint
Choice_A : MinimumLevel=2, PointCost=1
Action de classe : Requirements=[Choice_A]
```

Mettre temporairement un monstre assez généreux en XP pour franchir le seuil
rapidement.

Scénario :

1. personnage niveau 1 proche/avant 1000 XP ;
2. tuer le monstre ;
3. vérifier `[GridLevelUp]` puis `[GridLevelUpUI] Queued/Opened` ;
4. modal : contrôler niveau et stats avant/après ;
5. sélectionner `Choice_A`, puis **Annuler** ;
6. confirmer qu'aucun `[GridClassProgression]` n'a été émis ;
7. reproduire le level-up sur une session de test et choisir `Choice_A` ;
8. **Confirmer** ;
9. vérifier `[GridClassProgression] ... Committed=1` ;
10. en combat, vérifier que l'action requérant `Choice_A` n'est plus `MissingRequirement`.

Remettre les valeurs de test avant toute validation d'asset.

---

## 15. Critère de clôture

```text
Compilation UE5.5.4                         OK
8 tests Grimrock.RPG.MON15.5.*              Success
Régressions MON15.1–15.4                    Success
Régressions CharacterCreation               Success
Régressions ActionCatalog / MON12.8          Success
PIE modal automatique                        OK
Annulation sans mutation                     OK
Confirmation atomique                        OK
Projection action après choix                OK
File de notifications                        OK
SaveVersion                                  3
Aucun asset/WBP modifié                      OK
```

Lorsque ces éléments sont confirmés, MON15.5 peut être marqué **VALIDÉ ET CLOS**.