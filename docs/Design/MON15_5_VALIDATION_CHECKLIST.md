# MON15.5 — Validation Checklist

Statut : **VALIDÉ ET CLOS — UE5.5.4 — 16 août 2026**.

MON15.6 est désormais débloqué.

---

## 1. Compilation

- [x] `GrimrockPrototype` compile sous UE5.5.4 après correction des bindings Slate UObject.
- [x] Aucun problème UHT dans les nouveaux `USTRUCT` / `UCLASS`.
- [x] Includes Slate/UMG validés.
- [x] Aucun `.uasset`, `.umap` ou WBP requis.

Correctif compilation : les boutons Slate `Confirmer` / `Annuler` utilisent `FOnClicked::CreateUObject` et non une surcharge `CreateSP` exigeant `AsShared()`.

---

## 2. Tests dédiés MON15.5

Les huit tests uniques sont `Success` :

- [x] `AtomicBatchCommit`
- [x] `AtomicFailure`
- [x] `WidgetCancelIsNonMutating`
- [x] `WidgetConfirmTransaction`
- [x] `CombatCatalogUnlock`
- [x] `LevelUpNotificationSource`
- [x] `CharacterIsolation`
- [x] `TransientPersistenceBoundary`

Campagne finale : la suite MON15.5 a été exécutée 3 fois, soit **24/24 Success**.

- [x] Aucun assert.
- [x] Aucun ensure.
- [x] Aucun `CheckAddress`.
- [x] Aucune exception ni `Fatal error`.

---

## 3. Transaction par lot

- [x] `{Choice_A, Choice_B}` réussit en une transaction.
- [x] Les deux choix sont présents après commit.
- [x] `Spent=2`, `Remaining=0` dans la fixture niveau 3.
- [x] Le commit n'est émis qu'après validation complète.

---

## 4. Échec atomique

- [x] Un lot dépassant le budget est refusé.
- [x] Raison `InsufficientChoicePoints`.
- [x] Aucun choix partiel n'est confirmé.
- [x] Aucun requirement refusé n'est projeté.
- [x] Les grants automatiques valides restent disponibles.

---

## 5. Modal staged

- [x] Un clic ajoute seulement un choix pending.
- [x] Aucun choix runtime n'est acquis avant `Confirmer`.
- [x] Un prérequis pending peut rendre un choix dépendant sélectionnable.
- [x] Un prérequis pending requis par un autre pending ne peut pas être retiré.
- [x] `Annuler` ne mute pas l'état autoritaire.
- [x] `Confirmer` applique le lot en une transaction.
- [x] Une confirmation invalide laisse la modal ouverte.

---

## 6. Comparaison avant / après

- [x] `PreviousLevel` et `NewLevel` sont cohérents.
- [x] PV max via `CalculateDerivedStats`.
- [x] Mana max via `CalculateDerivedStats`.
- [x] Armures cohérentes.
- [x] Aucun bonus d'équipement n'est bake dans les stats de base.
- [x] Les points affichés correspondent au niveau final.

---

## 7. Notification level-up

- [x] Delegate MON15.3 historique conservé.
- [x] Delegate MON15.5 source-aware valide.
- [x] Bon `UGridPartyInventoryComponent` transmis.
- [x] Bons niveaux précédent/nouveau transmis.
- [x] Projection requirements prête avant consommation par le catalogue.

---

## 8. Sécurité combat

Scénario PIE validé :

```text
Queued 1->2
Deferred ... Reason=CombatActive
...
Coalesced 1->3 Gained=2 Pending=1
...
CombatSafePoint Result=Victory Pending=1
ModalGuard Applied ... PausedByModal=true
Opened Previous=1 New=3
```

- [x] Aucun écran Level Up n'est ouvert pendant un combat actif.
- [x] Le TurnManager est résolu depuis le `AGridLevelRuntimeActor`.
- [x] Le combat continue normalement pendant l'attente.
- [x] La modal s'ouvre seulement au point sûr `OnCombatEnded`.
- [x] La modal pause effectivement le jeu.
- [x] L'ancien warning de focus `InputMode:UIOnly` a disparu.

---

## 9. Coalescence des level-ups

- [x] Deux montées successives du même personnage avant présentation sont fusionnées.
- [x] `1->2` puis `2->3` devient une seule notification `1->3`.
- [x] `LevelsGained=2` après fusion.
- [x] Une seule modal est ouverte.
- [x] Les choix sont évalués avec le vrai niveau final.

---

## 10. Catalogue combat

- [x] Avant confirmation : action exigeant `Choice_A` => `MissingRequirement`.
- [x] Après confirmation : action disponible si le reste du contexte le permet.
- [x] Grants automatiques ajoutés au contexte.
- [x] `FGridCombatActionDefinition::Requirements` reste l'unique contrat.
- [x] Aucun second catalogue/circuit d'exécution.

Régression finale :

```text
Grimrock.Monsters.MON12.ActionCatalog.Contributions              Success
Grimrock.Monsters.MON12.ActionCatalog.GenericAttackLifecycle     Success
```

Les deux tests apparaissent deux fois dans le log final : **4/4 Success**.

---

## 11. Hotbar / modalité

Suite `Grimrock.Monsters.MON12.8.*` : **26 tests uniques / 26 Success**.

Elle couvre notamment :

- [x] hotbar vide par défaut ;
- [x] migration legacy hotbar ;
- [x] bindings par personnage ;
- [x] sauvegarde mémoire ;
- [x] drag/drop et swaps ;
- [x] clic et clavier ;
- [x] garde modal ;
- [x] mains nues ;
- [x] quick items ;
- [x] parchemins ;
- [x] sorts / mana ;
- [x] ciblage cellule et zone ;
- [x] armes de jet ;
- [x] nettoyage des bindings après consommation.

---

## 12. Isolation personnages

- [x] Choix indexés par `CharacterId` stable.
- [x] Le choix d'un personnage ne modifie pas les autres.
- [x] Une projection obsolète d'un autre composant n'est pas réutilisée.

---

## 13. SaveGame — frontière volontaire

- [x] `UGrimrockPartySaveGame::CurrentSaveVersion == 3`.
- [x] Aucun nouveau champ SaveGame en MON15.5.
- [x] Choix confirmés runtime uniquement.
- [x] Reset du registry => perte volontaire des choix.
- [x] Cette limite est réservée à MON15.6.

MON15.5 ne doit pas être présenté comme une persistance finale des choix.

---

## 14. Régressions globales déjà validées

Les campagnes antérieures de MON15.5 ont également confirmé :

- [x] MON15.1–15.4 verts ;
- [x] CharacterCreation pertinente verte ;
- [x] `ActionCatalog` vert ;
- [x] MON12.8 / hotbar vert.

---

## 15. Résultat final

```text
Compilation UE5.5.4                         OK
8 tests Grimrock.RPG.MON15.5.*              Success
Campagne finale MON15.5                      24/24 Success
ActionCatalog final                           4/4 Success
MON12.8 final                                26/26 Success
Total log final                              54/54 Success
PIE modal automatique après combat            OK
Déféré pendant combat                         OK
Coalescence 1->2 + 2->3 => 1->3              OK
Annulation sans mutation                      OK
Confirmation atomique                         OK
Projection action après choix                 OK
File de notifications                         OK
SaveVersion                                  3
Aucun asset/WBP modifié                      OK
```

**MON15.5 — VALIDÉ ET CLOS.**
