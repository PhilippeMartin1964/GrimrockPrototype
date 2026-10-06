# RPG03.9.5C — Prêtre : matérialisation production

Date : **6 octobre 2026**  
Dépendances validées : `RPG03.9.5A` 6/6, `RPG03.9.5B1` 6/6, `RPG03.9.5B2` 7/7  
Statut : **VALIDÉ / MATÉRIALISÉ — DA_Class_Priest et statuts de production présents ; campagne globale 193/193 le 6 octobre 2026**

## Objet

Matérialiser via Unreal Editor, et non par édition binaire externe :

- `DA_Class_Priest` ;
- `DA_Status_Regeneration` ;
- `DA_Status_Blessed` ;
- `DA_Status_HolyProtection` ;
- `DA_Status_Sanctuary` ;
- `DA_Status_DivineBastion` ;
- `DA_Status_TurnedUndead` ;
- `DA_Status_Banished`.

Le commandlet réutilise exactement `FRPGPriestAuthoring::ConfigureClass` et `ConfigureStatus` : il ne possède aucune seconde définition métier.

## Exécution

```powershell
.\Scripts\AuthorRPGPriest.ps1 -EngineRoot D:\UE_5.5
```

Le script exige `master` et un working tree propre, compile l'Editor, lance le commandlet, puis exécute la campagne globale :

```text
Grimrock.RPG.RPG03.9.5
```

Cette campagne doit couvrir le support 5A, Restauration/Protection B1, Exorcisme B2 et les assets de production C.

Les fichiers `.uasset` générés ne sont commités qu'après lecture de la sortie utilisateur et vérification de `git status --short`.
