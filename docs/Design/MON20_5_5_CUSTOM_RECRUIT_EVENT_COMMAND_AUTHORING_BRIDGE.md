# MON20.5.5 — Custom Recruit Event / Command Authoring Bridge

Statut : **VALIDÉ UE5.5.4 — 22/22 AUTOMATION SUCCESS**  
Date : **24 août 2026**  
Jalon parent : **MON20.5 — Custom Recruit / Wizard Context Reuse**

---

## 1. Objectif

Rendre l'ouverture du wizard `CustomRecruit` pilotable par l'architecture data-driven du niveau :

```text
Event
    -> Command
        -> AGrimrockPartyPawn::ShowCustomRecruitCharacterCreationWidget()
            -> WBP_CharacterCreationWizard existant
```

La tranche ne crée aucun Blueprint Graph, aucun second widget et aucune nouvelle transaction de recrutement.

---

## 2. Nouveau target data-only

`EGridLevelObjectType` reçoit, en fin d'enum :

```cpp
CustomRecruiter
```

Ce type représente un **service de recrutement personnalisable** dans le niveau. Il est volontairement data-only :

- aucun Actor NPC n'est requis ;
- aucune donnée de compagnon scénarisé n'est requise ;
- aucun état de personnage n'est stocké sur l'objet ;
- la création réelle reste effectuée par le wizard et `FRPGCustomRecruitService`.

Le type est ajouté après `StoryCompanion` afin de préserver les valeurs sérialisées existantes.

---

## 3. Nouvelle commande

`EGridObjectCommand` reçoit :

```cpp
OpenCustomRecruit = 24
```

La valeur précédente :

```cpp
OfferRecruitment = 23
```

reste inchangée.

Le contrat est donc :

```text
CustomRecruiter
    -> OpenCustomRecruit
```

Aucune commande générique `Toggle`, `Activate`, etc. n'est utilisée pour lancer le wizard.

---

## 4. Bridge runtime

`UGridActivationComponent::ApplyLinkCommand()` traite désormais le target `CustomRecruiter` avant le chemin générique stateful.

Pour :

```text
Target.Type = CustomRecruiter
Command     = OpenCustomRecruit
```

le runtime :

1. résout le Pawn joueur ;
2. appelle :

```cpp
PartyPawn->ShowCustomRecruitCharacterCreationWidget();
```

3. retourne le succès ou l'échec réel de l'ouverture du modal ;
4. ne marque pas le target comme actif dans `ActiveObjectIds`.

Une autre commande adressée à `CustomRecruiter` est rejetée explicitement.

Le Pawn MON20.5.4 conserve toutes les gardes : héros principal présent, place disponible, absence d'autre modal incompatible, PlayerController et classe de widget valides.

---

## 5. Authoring CONNECTORS

`GridEditorLinkPolicy` expose exactement une commande pour ce target :

```text
OpenCustomRecruit
```

avec :

```text
Runtime Support = Gameplay
Can Receive Commands = true
Can Emit Events      = false
```

Le target est donc volontairement **récepteur uniquement**.

Exemple principal :

```text
Trigger.Activated
    -> CustomRecruiter.OpenCustomRecruit
```

Autres sources possibles :

```text
Button.Activated
    -> CustomRecruiter.OpenCustomRecruit

Logic.Activated
    -> CustomRecruiter.OpenCustomRecruit
```

Les conditions de lien existantes restent disponibles, notamment les variables MON19 :

```text
Condition = LevelVariableBoolEquals
Condition = LevelVariableIntCompare
```

---

## 6. Lua

Le bridge `grid.command()` résout déjà les noms via `EGridObjectCommand`.

La nouvelle valeur devient donc automatiquement utilisable :

```lua
grid.command("RecruiterTarget", "OpenCustomRecruit")
```

à condition que `RecruiterTarget` soit un `LogicId` non ambigu pointant vers un objet `CustomRecruiter`.

Aucune API Lua parallèle n'est ajoutée.

---

## 7. Archetype recommandé

Aucun champ spécialisé de palette n'est nécessaire.

Archetype conseillé :

```text
Archetype Id              = CustomRecruiter_Service
Display Name              = Custom Recruiter
Gameplay Type             = CustomRecruiter
Placement Kind            = Center
Palette Category          = Recruitment
Functional Category       = Decoration
Default Initially Enabled = true
Default Initially Active  = false
Runtime Actor Class       = None
Runtime Interactable      = false
Runtime Readable          = false
Can Share Cell            = true
Can Share Anchor          = true
Blocks Movement           = false
```

Le `PreviewMesh` est optionnel et ne sert qu'à l'authoring visuel.

Une entrée de `GridObjectPaletteAsset` peut simplement référencer cet archetype :

```text
Entry Id          = CustomRecruiter_Service
Display Name      = Custom Recruiter
Category Override = Recruitment
Default Archetype = CustomRecruiter_Service
```

Aucune `DefaultStoryCompanionDefinition` n'est requise, puisque le joueur définit lui-même le personnage dans le wizard.

---

## 8. Différence avec StoryCompanion

Les deux modèles restent volontairement distincts :

```text
StoryCompanion
    -> possède URPGStoryCompanionAsset
    -> OfferRecruitment
    -> identité prédéfinie par l'auteur

CustomRecruiter
    -> aucune définition de personnage
    -> OpenCustomRecruit
    -> identité créée par le joueur
```

Les deux convergent ensuite vers l'autorité de groupe existante.

---

## 9. Répétition de l'offre

Contrairement au Story Companion, `CustomRecruiter` n'a pas de notion `AlreadyActive` ni de refus mémorisé.

Le service est **réutilisable** tant qu'une place reste disponible :

```text
ouvrir
-> Annuler
-> revenir plus tard
-> ouvrir à nouveau
```

ou :

```text
ouvrir
-> engager une recrue
-> encore une place disponible
-> ouvrir à nouveau pour une autre recrue
```

Lorsque le groupe est plein, MON20.5.4 rejette l'ouverture sans mutation.

---

## 10. Automation Tests

Le filtre reste :

```text
Grimrock.MON20.5.CustomRecruit
```

MON20.5.5 ajoute quatre tests :

```text
CustomRecruiterArchetypeContract
EditorLinkPolicy
EventCommandContract
EventCommandMissingPlayerPawn
```

### EventCommandContract

Vérifie :

- `OfferRecruitment` conserve la valeur 23 ;
- `OpenCustomRecruit` utilise la valeur 24 ;
- la résolution par nom d'enum fonctionne pour Lua ;
- `CustomRecruiter` est exposé par l'enum des types.

### CustomRecruiterArchetypeContract

Vérifie qu'un archetype `CustomRecruiter` data-only peut être valide sans `RuntimeActorClass`.

### EventCommandMissingPlayerPawn

Vérifie qu'un vrai lien :

```text
Trigger.Activated
    -> CustomRecruiter.OpenCustomRecruit
```

est rejeté proprement lorsqu'aucun Pawn joueur n'existe, sans transformer le target en état actif.

### EditorLinkPolicy

Vérifie que :

- `OpenCustomRecruit` est l'unique commande exposée ;
- son support runtime est `Gameplay` ;
- `Toggle` est refusé ;
- le target reçoit des commandes mais n'émet aucun événement.

Les 18 tests MON20.5.2 à MON20.5.4 restent présents.

### Validation UE5.5.4 — 24 août 2026

Automation Controller exécuté après le correctif unity-build du module `GrimrockPrototypeEditor` :

```text
Grimrock.MON20.5.CustomRecruit
22 / 22 Success
0 Fail
0 Error
```

Les quatre tests MON20.5.5 sont `Success`, ainsi que les dix-huit tests précédents. Le warning du test négatif `EventCommandMissingPlayerPawn` est attendu : le lien échoue proprement avec `Reason=missing player party pawn`, puis le test termine en `Success`.

MON20.5.5 est donc **validé en automation sous UE5.5.4**. La validation manuelle PIE est portée par MON20.5.6.

---

## 11. Validation manuelle après automation

Après `22 / 22 Success`, l'authoring de production peut être effectué dans le Grid Editor :

1. créer `DA_Archetype_CustomRecruiter_Service` ;
2. ajouter l'entrée correspondante dans le `GridObjectPaletteAsset` ;
3. placer le target `CustomRecruiter` ;
4. sélectionner le Trigger source ;
5. ouvrir `CONNECTORS` ;
6. cliquer `+` ;
7. configurer :

```text
Source Object = Trigger
Event         = Activated
Target Object = CustomRecruiter placé
Command       = OpenCustomRecruit
Condition     = None
```

8. `Create` ;
9. lancer PIE.

PIE attendu :

```text
Trigger
    -> OpenCustomRecruit
    -> même WBP_CharacterCreationWizard
```

Puis :

```text
Annuler
    -> fermeture
    -> retour au jeu
    -> groupe inchangé
```

et :

```text
Engager
    -> nouvelle recrue active
    -> fermeture
    -> retour au jeu
```

---

## 12. Hors scope

MON20.5.5 n'ajoute pas :

- coût en or ;
- dialogue d'auberge/guilde ;
- NPC 3D recruteur ;
- niveau dynamique de recrue ;
- équipement de départ ;
- réserve lorsque le groupe est plein ;
- nouveau WBP ;
- nouvelle logique Blueprint.

Ces extensions restent indépendantes du bridge Event -> Command.
