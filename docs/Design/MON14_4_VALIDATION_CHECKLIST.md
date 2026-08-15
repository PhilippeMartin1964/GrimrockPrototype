# MON14.4 — Validation Checklist

## 1. Synchronisation

```powershell
git pull
```

Vérifier que `master` contient le commit MON14.4 annoncé.

## 2. Compilation

Compiler l'éditeur UE 5.5.4 :

```powershell
D:\UE_5.5\Engine\Build\BatchFiles\Build.bat GrimrockPrototypeEditor Win64 Development "D:\Development\GrimrockPrototype\GrimrockPrototype.uproject" -WaitMutex -NoUBA -NoUBALocal -Log="D:\Development\GrimrockPrototype\Saved\Logs\UBT-MON144.log"
```

Attendu : aucune erreur C++/UHT.

## 3. Tests ciblés MON14.4

Dans Session Frontend / Automation :

```text
Grimrock.Monsters.MON14.4
```

Attendu :

```text
HearingAlarmPropagation  Success
AlarmFiltering           Success
SharingDisabled          Success
```

## 4. Régressions MON14

Exécuter :

```text
Grimrock.Monsters.MON14.1
Grimrock.Monsters.MON14.2
Grimrock.Monsters.MON14.3
```

Puis, si ces suites sont vertes :

```text
Grimrock.Monsters.MON
```

Aucun test précédemment validé ne doit régresser.

## 5. Validation manuelle dans L_GrimrockEditor

### Scénario A — Garde qui entend et alerte un allié dormant

Configurer deux Rat Géant avec :

```text
MonsterId identique
EncounterGroupId = Guard_Test_01
Source.bSharesAggroWithGroup = true
Source.AggroPropagationRange >= distance entre les deux monstres
```

Le premier doit pouvoir entendre le groupe sans le voir. Le second doit être
`Dormant` et ne pas entendre le groupe directement.

Attendu :

1. le premier passe en `Alert / Investigating` ;
2. le second quitte `Dormant` ;
3. le second commence à se diriger vers la dernière cellule connue ;
4. aucun combat ne démarre tant qu'aucun monstre n'a de vision réelle.

Log utile :

```text
[MON14.4] ExplorationAlert ... Alerted=1 ...
```

### Scénario B — Même groupe mais hors portée

Déplacer le second garde au-delà de `AggroPropagationRange`.

Attendu : il reste dormant/inactif lorsque le premier entend le groupe.

### Scénario C — Groupe différent

Conserver la distance valide mais donner au second :

```text
EncounterGroupId = Guard_Test_02
```

Attendu : aucune alerte reçue.

### Scénario D — Partage désactivé

Sur la source :

```text
bSharesAggroWithGroup = false
```

Attendu : le garde source peut enquêter lui-même, mais aucun allié n'est réveillé
par propagation.

### Scénario E — Passage ultérieur au combat

Après l'alarme auditive, laisser un des monstres alertés obtenir une ligne de vue
réelle sur le groupe.

Attendu :

1. MON14.1 démarre un seul combat automatique ;
2. MON7 ajoute les alliés éligibles selon le même groupe/range ;
3. aucune double entrée dans l'initiative ;
4. toutes les locomotions d'exploration sont suspendues atomiquement par MON14.3.

## 6. Anti-reset

Pendant qu'un allié se dirige vers une cellule connue, provoquer plusieurs
rafraîchissements de perception sans déplacer le groupe.

Attendu : l'allié ne revient pas sans cesse à sa cellule de départ et son
mouvement d'investigation ne redémarre pas à chaque notification identique.

Déplacer ensuite le groupe pendant que la source conserve une perception valide.

Attendu : une nouvelle cellule connue peut rediriger l'allié.

## 7. Validation finale

Le jalon MON14.4 est validé lorsque :

- compilation UE réussie ;
- 3/3 tests MON14.4 réussis ;
- suites MON14.1–14.3 sans régression ;
- alarme auditive observée en jeu ;
- cible hors portée/groupe différent non alertée ;
- `bSharesAggroWithGroup=false` respecté ;
- aucune alarme auditive ne démarre directement le combat ;
- une vision ultérieure démarre bien le combat automatique.
