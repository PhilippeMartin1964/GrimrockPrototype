# MON21.6.4 — Secret Discovery

Date : **28 septembre 2026**  
Statut : **IMPLÉMENTÉ — VALIDATION LOCALE UTILISATEUR REQUISE**

## 1. Objectif

MON21.6.4 mémorise explicitement qu’une porte secrète a été découverte, indépendamment de son état ouvert/fermé.

Avant découverte, la future projection Map doit continuer à la traiter comme un mur normal. Après découverte, son `ObjectId` reste connu pendant toute la session, même si la porte se referme.

## 2. Autorité

`FGridMapExplorationState` reçoit `DiscoveredSecretObjectIds : TSet<FGuid>`.

L’identité est l’`InstanceId/ObjectId` stable du `FGridWorldObjectInstance`. Aucun identifiant cartographique parallèle n’est créé.

API : `IsSecretDiscovered`, `TryMarkSecretDiscovered`, `GetDiscoveredSecretCount`. `Reset()` efface cellules explorées et secrets connus.

## 3. Déclencheur de découverte

Une porte secrète fermée ou simplement en cours d’ouverture n’est pas encore considérée découverte.

Le secret est enregistré lorsque :

- une `AGridSecretDoorActor` atteint effectivement l’état **fully open** et émet `OnDoorAnimationFinished` ;
- une porte secrète est initialement ouverte lors de son enregistrement runtime ;
- un état runtime applique explicitement une porte secrète ouverte et non bloquante.

Après fermeture, l’`ObjectId` reste dans l’ensemble des secrets découverts.

## 4. Validation de l’identité

`AGridLevelRuntimeActor::TryDiscoverMapSecretDoor()` refuse :

- GUID invalide ;
- objet absent du `LevelAsset` courant ;
- objet qui n’est pas une porte ;
- porte qui n’est pas reconnue par `UGridDoorSystemComponent::IsSecretDoorOnEdge()`.

Le DoorSystem déclenche la découverte ; `FGridMapExplorationState` reste l’unique autorité de connaissance.

## 5. Frontière SaveGame

`DiscoveredSecretObjectIds` n’a volontairement pas le flag `SaveGame`. Le schéma reste **v22**.

MON21.6.5 rendra simultanément persistants les cellules explorées et les secrets découverts, avec incrément exact-match du SaveGame.

## 6. Validation MON21.6.3 actée

Validation utilisateur du 28 septembre 2026 :

```text
Filter                  : Grimrock.Map.MON21_6_3
Succeeded               : 4
Succeeded with warnings : 0
Failed                  : 0
Not run                 : 0
Process exit code       : 0
Report                  : TD04-20260928-083313
```

MON21.6.3 est **VALIDÉ**.

## 7. Automation MON21.6.4

Filtre : `Grimrock.Map.MON21_6_4`

```text
SecretDiscovery.StateIdentityAndReset
SecretDiscovery.FullOpenThenCloseRemainsKnown
SecretDiscovery.InitiallyOpenIsKnown
SecretDiscovery.NormalDoorAndSaveBoundary
```

Aucun résultat MON21.6.4 n’est déclaré avant retour du harness UE5.5.4 local.

## 8. Hors périmètre

- rendu mur/secret dans le WBP : MON21.6.6/6.8 ;
- persistance disque : MON21.6.5 ;
- projection multi-dalles : MON21.6.7.

Prochaine tranche après validation : **MON21.6.5 — Exploration Persistence / Save Schema**.
