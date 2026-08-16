# MON15.7 — Validation Checklist

Statut : **VALIDÉ ET CLOS sous Unreal Engine 5.5.4**  
Date : **16 août 2026**

---

## 1. Préparation DataAsset

`/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant`

- [x] valeur temporaire `1000` retirée ;
- [x] valeur historique `10` n'est plus la valeur de production ;
- [x] `Experience Reward = 500` ;
- [x] asset sauvegardé ;
- [x] asset poussé sur `origin/master` via Git LFS (`9b1fc8c...`).

---

## 2. Compilation / chargement UE

- [x] les nouveaux tests MON15.7 sont chargés sous UE5.5.4 ;
- [x] aucun changement de SaveVersion ;
- [x] aucun nouveau warning C++ MON15.7 bloquant.

---

## 3. Tests dédiés

`Grimrock.RPG.MON15.7` : **4/4 Success**.

- [x] `FrozenCurve`
- [x] `SoloRatPacing`
- [x] `PartyRatPacing`
- [x] `ProductionRatAsset`

`ProductionRatAsset` charge le vrai DataAsset et confirme `ExperienceReward=500`.

---

## 4. Courbe finale

- [x] L1 = 0 XP ;
- [x] L2 = 1000 XP ;
- [x] L3 = 3000 XP ;
- [x] L4 = 6000 XP ;
- [x] L5 = 10000 XP ;
- [x] L20 = 190000 XP ;
- [x] niveau maximum = 20.

---

## 5. Pacing de référence

Solo :

- [x] 1 Rat Géant = 500 XP, toujours niveau 1 ;
- [x] 2 Rats Géants cumulés = 1000 XP, niveau 2 ;
- [x] 6 Rat-equivalents cumulés = 3000 XP, niveau 3 ;
- [x] 12 Rat-equivalents cumulés = 6000 XP, niveau 4 ;
- [x] 20 Rat-equivalents cumulés = 10000 XP, niveau 5.

Groupe de quatre :

- [x] 500 XP se divisent exactement en 125 XP chacun ;
- [x] 8 Rat-equivalents = niveau 2 ;
- [x] 24 Rat-equivalents = niveau 3 ;
- [x] 48 Rat-equivalents = niveau 4.

---

## 6. PIE final

Scénario observé :

```text
Rat 1 : Previous=0    New=500  LevelStored=1
Rat 2 : Previous=500  New=1000 LevelStored=2
Rat 3 : Previous=1000 New=1500 LevelStored=2
```

- [x] aucun Level Up après le premier rat ;
- [x] Level Up 1 -> 2 exactement après le deuxième rat ;
- [x] notification différée pendant combat ;
- [x] présentation au safe point de victoire ;
- [x] modal cliquable ;
- [x] choix confirmable ;
- [x] `ModalGuard Restored` ;
- [x] gameplay utilisable après fermeture.

---

## 7. Régression MON15 complète

Campagne finale : **95/95 Success**.

MON15 : **42/42**.

- [x] MON15.1 — 4/4 ;
- [x] MON15.2 — 5/5 ;
- [x] MON15.3 — 6/6 ;
- [x] MON15.4 — 7/7 ;
- [x] MON15.5 — 8/8 ;
- [x] MON15.6 — 8/8 ;
- [x] MON15.7 — 4/4.

---

## 8. Régressions externes

Dans la campagne finale fournie :

- [x] MON9 — 13/13 ;
- [x] MON13.5 RealPIEIntegration — Success ;
- [x] MON14.1–MON14.4 — Success ;
- [x] aucune erreur Automation dans les 95 tests.

Couverture antérieure conservée :

- [x] CC5 — validé pendant MON15.6 ;
- [x] MON12.ActionCatalog — validé pendant MON15.6.

Ces deux groupes ne figurent pas dans le dernier log de 95 tests. MON15.7 n'a modifié aucun de leurs chemins runtime.

---

## 9. Save / Continue

Le contrat Save/Continue a été revalidé en profondeur pendant MON15.6, notamment :

- [x] SaveVersion 4 ;
- [x] Level / Experience cohérents ;
- [x] choix confirmé restauré ;
- [x] `PendingLevelUps=1` restauré ;
- [x] modal restaurée exactement une fois ;
- [x] overlay de chargement retiré avant modal ;
- [x] gameplay rendu au joueur après fermeture.

MON15.7 ne modifie aucune logique SaveGame.

---

## 10. Documentation

- [x] `MON15_7_BALANCING.md` finalisé ;
- [x] `MON15_7_VALIDATION_CHECKLIST.md` finalisé ;
- [x] `MON15_CLOSURE.md` créé ;
- [x] `00_PROJECT_OVERVIEW.md` mis à jour ;
- [x] `PROJECT_COMPLETION_ROADMAP.md` mis à jour ;
- [x] `MON1_RAT_GIANT_DATA_ASSET_SETUP.md` annoté pour signaler que `10 XP` est historique et supersédé par `500 XP` ;
- [ ] `99_DECISIONS_LOG.md` non réécrit dans ce commit : `MON15_CLOSURE.md` constitue le record autoritaire de décision MON15 afin d'éviter une réécriture massive du journal historique.

---

## 11. Porte de clôture

Toutes les portes fonctionnelles de MON15.7 sont franchies.

**MON15.7 — VALIDÉ ET CLOS.**  
**MON15 — XP & Level Progression — VALIDÉ ET CLOS.**

Prochain travail autoritaire :

```text
MON16.1 — Status Effect Definition & Runtime State
```
