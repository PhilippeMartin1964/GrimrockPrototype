# MON15.7 — Validation Checklist

Statut : **IMPLEMENTÉ — EN ATTENTE DE VALIDATION UE5.5.4**  
Date : **16 août 2026**

---

## 1. Préparation DataAsset

Dans Unreal Editor :

```text
/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant
```

régler :

```text
Monster | Rewards | Experience Reward = 500
```

- [ ] la valeur temporaire `1000` a été retirée ;
- [ ] la valeur historique `10` n'est plus la valeur de production ;
- [ ] l'asset est sauvegardé.

---

## 2. Compilation

- [ ] `Development Editor x64` compile sous UE5.5.4 ;
- [ ] aucun changement de SaveVersion ;
- [ ] aucun nouveau warning C++ MON15.7.

---

## 3. Tests dédiés

Exécuter :

```text
Grimrock.RPG.MON15.7
```

Attendu : **4/4 Success**.

- [ ] `FrozenCurve`
- [ ] `SoloRatPacing`
- [ ] `PartyRatPacing`
- [ ] `ProductionRatAsset`

`ProductionRatAsset` doit charger le vrai DataAsset et confirmer `ExperienceReward=500`.

---

## 4. Courbe finale

- [ ] L1 = 0 XP ;
- [ ] L2 = 1000 XP ;
- [ ] L3 = 3000 XP ;
- [ ] L4 = 6000 XP ;
- [ ] L5 = 10000 XP ;
- [ ] L20 = 190000 XP ;
- [ ] niveau maximum = 20.

---

## 5. Pacing solo de référence

Avec un seul personnage actif :

- [ ] 1 Rat Géant = 500 XP, toujours niveau 1 ;
- [ ] 2 Rats Géants cumulés = 1000 XP, niveau 2 ;
- [ ] 6 Rats Géants cumulés = 3000 XP, niveau 3 ;
- [ ] 12 Rats Géants cumulés = 6000 XP, niveau 4 ;
- [ ] 20 Rats Géants cumulés = 10000 XP, niveau 5.

---

## 6. Pacing groupe de référence

Avec quatre personnages actifs et éligibles :

- [ ] 500 XP se divisent exactement en 125 XP chacun ;
- [ ] 8 rat-equivalents cumulés = niveau 2 ;
- [ ] 24 rat-equivalents cumulés = niveau 3 ;
- [ ] 48 rat-equivalents cumulés = niveau 4.

Ces nombres sont des budgets de balance, pas une obligation de construire le donjon uniquement avec des rats.

---

## 7. Régression MON15 complète

Exécuter :

```text
Grimrock.RPG.MON15
```

- [ ] MON15.1 vert ;
- [ ] MON15.2 vert ;
- [ ] MON15.3 vert ;
- [ ] MON15.4 vert ;
- [ ] MON15.5 vert ;
- [ ] MON15.6 vert ;
- [ ] MON15.7 vert.

---

## 8. Régressions externes

Exécuter :

```text
Grimrock.CharacterCreation.CC5
Grimrock.Monsters.MON9
Grimrock.Monsters.MON12.ActionCatalog
```

- [ ] CC5 vert ;
- [ ] MON9 vert ;
- [ ] MON12 ActionCatalog vert.

Si possible avant clôture majeure :

```text
Grimrock.Monsters
```

- [ ] campagne monstres globale verte, ou écarts analysés et documentés.

---

## 9. PIE final — 1 rat puis 2 rats

Depuis une nouvelle partie solo avec 0 XP :

### Après le premier Rat Géant

Attendu :

```text
Reward=500
Previous=0
New=500
LevelStored=1
```

- [ ] aucune modal Level Up ;
- [ ] le combat se termine normalement.

### Après le deuxième Rat Géant

Attendu :

```text
Reward=500
Previous=500
New=1000
PreviousLevel=1
NewLevel=2
```

- [ ] une seule modal Level Up ;
- [ ] choix/annulation fonctionnels ;
- [ ] `ModalGuard Restored` ;
- [ ] déplacement/interaction/combat reprennent après fermeture.

---

## 10. Save / Continue

Après le niveau 2 :

- [ ] sauvegarder ;
- [ ] arrêter PIE ;
- [ ] relancer et `Continue` ;
- [ ] XP = 1000 ;
- [ ] niveau = 2 ;
- [ ] choix confirmé toujours acquis s'il y en a un ;
- [ ] aucun overlay de chargement bloqué ;
- [ ] gameplay utilisable.

---

## 11. Documentation de clôture MON15

À la clôture :

- [ ] créer `MON15_CLOSURE.md` ;
- [ ] mettre à jour `00_PROJECT_OVERVIEW.md` ;
- [ ] mettre à jour `PROJECT_COMPLETION_ROADMAP.md` ;
- [ ] mettre à jour `99_DECISIONS_LOG.md` ;
- [ ] mettre à jour `MON1_RAT_GIANT_DATA_ASSET_SETUP.md` pour signaler que la valeur historique `10` est supersédée par MON15.7 `500` ;
- [ ] vérifier la cartographie du projet si nécessaire.

---

## 12. Porte de clôture

MON15.7 ne sera déclaré **VALIDÉ ET CLOS** qu'après validation de l'asset réel et du scénario PIE 1 rat / 2 rats.

La clôture de MON15 entraîne ensuite le passage à :

**MON16 — Status Effects**.
