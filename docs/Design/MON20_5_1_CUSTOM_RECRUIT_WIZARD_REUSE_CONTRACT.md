# MON20.5.1 — Custom Recruit / Wizard Context Reuse Contract

Statut : **AUDIT TERMINÉ — CONTRAT D’IMPLÉMENTATION DÉFINI**  
Date : **24 août 2026**  
Jalon parent : **MON20.5 — Custom Recruit / Wizard Context Reuse**  
Référence de départ : `df787fdeb947d6dbdf48be6adb30a1a00e97e8a9` (`Close MON20.4 with the correct assets`)

---

## 1. Objectif

Permettre au joueur de créer une **recrue personnalisée en cours de partie** en réutilisant le wizard de création de personnage déjà validé, sans dupliquer :

- les règles de race / classe ;
- l’allocation des caractéristiques ;
- les portraits et icônes ;
- le calcul des statistiques dérivées ;
- le système de groupe ;
- le recrutement MON20.2 ;
- le CharacterPool existant.

Le résultat cible est :

```text
Recruteur / service de recrutement
    -> ouvre le même WBP_CharacterCreationWizard
    -> contexte CustomRecruit
    -> le joueur configure la recrue
    -> validation C++ finale
    -> candidat transitoire dans CharacterPool
    -> FRPGPartyRecruitmentService::TryRecruitFromPool
    -> groupe actif
```

Aucune logique métier parallèle ne doit être ajoutée dans les Graphs Blueprint.

---

## 2. Existant réellement réutilisable

### 2.1 Wizard

`URPGCharacterCreationWizardWidget` existe déjà et dérive de :

```text
URPGCharacterCreationWidget
```

Étapes actuelles :

```text
Race
Class
Attributes
Identity
Summary
```

Le widget gère déjà :

- choix de race ;
- choix de classe ;
- genre / portrait ;
- allocation des caractéristiques ;
- validation du budget de points ;
- validation du nom ;
- résumé final ;
- calcul des attributs finaux ;
- calcul PV / mana / initiative / etc. ;
- aperçu de la capacité de port ;
- classe canonique conservée pour les actions runtime.

Conclusion : **aucun second wizard de recrutement ne doit être créé**.

### 2.2 Requête de création

`FRPGCharacterCreationRequest` transporte déjà :

```text
DisplayName
RaceDefinition
ClassDefinition
CombatActionSourceClassDefinition
PortraitGender
PortraitVariantId
Portrait
ClassIcon
```

Le wizard sait déjà produire les données nécessaires pour construire une recrue personnalisée.

### 2.3 Groupe / réserve-like

L’autorité reste :

```text
UGridPartyInventoryComponent
    -> FGridPartyInventoryState
        -> ActiveCharacters
        -> ActiveEquipment
        -> CharacterPool
```

`CharacterPool` reste le passage obligatoire avant activation afin de réutiliser MON20.2.

### 2.4 Transaction de recrutement

MON20.2 fournit déjà :

```cpp
FRPGPartyRecruitmentService::TryRecruitFromPool(...)
```

Elle gère notamment :

- capacité du groupe ;
- identité ;
- hotbar ;
- ownership ;
- alignement `ActiveCharacters / ActiveEquipment` ;
- rollback atomique.

MON20.5 ne doit pas écrire directement dans `ActiveCharacters`.

---

## 3. Blocages actuels du wizard hors New Game

Le wizard est visuellement et fonctionnellement réutilisable, mais quatre hypothèses empêchent actuellement un usage en cours de partie.

### 3.1 `HasCompletedInitialCharacterCreation()`

`URPGCharacterCreationWidget::RefreshPreview()` et `CanSubmitCharacterCreation()` considèrent actuellement qu’un wizard est invalide dès que la création initiale est terminée.

Or une recrue personnalisée est créée précisément **après** cette étape.

### 3.2 `CreateInitialCharacter()`

`URPGCharacterCreationWizardWidget::SubmitCharacterCreation()` appelle actuellement :

```cpp
InventoryComponent->CreateInitialCharacter(...)
```

Cette API remplace l’état initial du groupe et n’est correcte que pour le héros de New Game.

Elle ne doit jamais être appelée pour une recrue personnalisée.

### 3.3 `HandleInitialCharacterCreated()`

Après création, le wizard appelle :

```cpp
OwningPartyPawn->HandleInitialCharacterCreated();
```

Cette fonction :

- ferme la création initiale ;
- initialise le Spellbook ;
- restaure l’input ;
- sauvegarde ;
- journalise la fin de création New Game.

Une recrue personnalisée nécessite un callback distinct.

### 3.4 `CancelWizard()`

Le bouton Annuler appelle actuellement :

```text
RemoveFromParent
-> UGrimrockGameInstance::RequestReturnToMainMenu
```

C’est correct pendant New Game, mais incorrect pour un recruteur rencontré dans un donjon.

En contexte CustomRecruit, Annuler doit uniquement :

```text
fermer le modal
-> ne modifier aucune donnée de groupe
-> restaurer l’input de jeu
```

---

## 4. Décisions architecturales MON20.5

### D1 — Un seul wizard

Le projet conserve :

```text
URPGCharacterCreationWizardWidget
WBP_CharacterCreationWizard
```

pour :

```text
NewGameMainHero
CustomRecruit
```

Aucun `WBP_CustomRecruitWizard` parallèle.

### D2 — Contexte transitoire, non SaveGame

MON20.5 introduira un contexte minimal :

```cpp
ERPGCharacterCreationContext
{
    NewGameMainHero,
    CustomRecruit
}
```

Le contexte appartient à l’instance du wizard. Il n’est pas sérialisé dans le personnage et ne nécessite aucune migration SaveGame.

Les anciennes variantes conceptuelles :

```text
TavernCustomRecruit
MarketCustomRecruit
GuildCustomRecruit
```

ne sont pas introduites comme valeurs d’enum pour l’instant. Le lieu qui ouvre le wizard est une donnée d’auteur ; il ne change pas la transaction de création.

### D3 — Pas de `PartyMemberKind` dans MON20.5

La décision MON20.2 reste valide : ne pas modifier le contrat SaveGame tant que la réserve / persistance MON20.9 n’exige pas explicitement ce champ.

MON20.5 n’ajoute donc pas encore :

```text
MainHero / CustomRecruit / StoryCompanion / TemporaryGuest
```

comme propriété persistante de `FGridCharacterInventoryState`.

### D4 — Le wizard reste le draft

Le wizard manipule déjà des données transitoires jusqu’au clic final :

- race ;
- classe ;
- attributs alloués ;
- nom ;
- genre ;
- portrait.

Aucune mutation de `PartyInventoryState` n’a lieu pendant les étapes.

Il n’est donc pas nécessaire d’ajouter immédiatement une seconde structure `FRPGCharacterCreationDraft` uniquement pour recopier cet état.

Si un futur besoin impose sauvegarde de brouillon, équipement de départ configurable ou reprise du wizard, un draft explicite pourra être ajouté alors.

### D5 — Transaction CustomRecruit via CharacterPool

La création personnalisée doit suivre :

```text
Wizard validé
    -> construction d’un FGridCharacterInventoryState complet
    -> CharacterId = nouveau GUID
    -> ajout temporaire à CharacterPool
    -> TryRecruitFromPool(CharacterId)
    -> succès : candidat devient actif
    -> échec : rollback intégral du candidat temporaire
```

Le service CustomRecruit sera responsable du rollback extérieur autour de MON20.2 afin qu’un échec ne laisse pas une recrue invisible/orpheline dans `CharacterPool`.

### D6 — Groupe plein : refus avant commit

Dans MON20.5, la réserve n’est pas encore une fonctionnalité joueur complète.

Le wizard doit donc refuser la validation si :

```text
ActiveCharacters.Num >= MaxActiveCharacters
```

Aucune recrue personnalisée n’est créée dans le pool lorsque le groupe est plein.

MON20.9 pourra changer cette règle en « créer en réserve ».

### D7 — Niveau initial

Première version MON20.5 :

```text
CustomRecruit StartingLevel = 1
```

Le scaling sur niveau moyen du groupe, progression de campagne ou type de recruteur est reporté au balancing / offre de recrutement ultérieure.

### D8 — Coût

Aucun système monétaire autoritaire n’est actuellement requis par le contrat MON20.5.

Le coût en or est donc hors scope de la première implémentation. Il ne doit pas être simulé par un champ UI sans transaction économique réelle.

### D9 — Équipement de départ

MON20.5 n’ajoute pas encore de système de pack d’équipement de départ.

La recrue personnalisée est créée avec :

```text
inventaire vide
équipement vide
hotbar vide
```

Les actions de classe restent disponibles via la `ClassDefinition` canonique, comme pour le héros initial.

Les packs de départ pourront être traités séparément lorsqu’un contrat d’équipement de recrue sera défini.

### D10 — Spellbook

Après recrutement réussi, le Pawn devra appeler le système Spellbook existant pour le nouveau `CharacterId`, de la même manière que la création initiale garantit l’existence des spellbooks actifs.

Aucun second registre de sorts ne sera créé.

### D11 — Modal / input

Le Pawn réutilisera :

```text
bCharacterCreationModalActive
CharacterCreationWidgetInstance
ApplyCharacterCreationInputMode(...)
```

Le flag devient conceptuellement « un wizard de création est ouvert », pas « New Game seulement ».

Le recrutement scénarisé MON20.4 et le wizard CustomRecruit restent mutuellement exclusifs.

---

## 5. Transaction CustomRecruit cible

API cible minimale :

```cpp
FRPGCustomRecruitService::TryCreateAndRecruit(
    UGridPartyInventoryComponent* PartyInventoryComponent,
    const FRPGCharacterCreationRequest& Request,
    FRPGCustomRecruitResult& OutResult);
```

Le service doit :

1. vérifier le composant ;
2. vérifier que le héros initial existe ;
3. vérifier la cohérence du groupe actif ;
4. vérifier qu’une place active existe ;
5. valider nom / race / classe / source d’actions ;
6. calculer les attributs finaux ;
7. valider les bornes d’attributs ;
8. générer un `CharacterId` unique ;
9. construire le candidat complet ;
10. initialiser inventaire et hotbar ;
11. ajouter temporairement le candidat au `CharacterPool` ;
12. appeler `FRPGPartyRecruitmentService::TryRecruitFromPool` ;
13. supprimer / rollback le candidat temporaire si la transaction échoue ;
14. retourner l’index actif et le `CharacterId` en cas de succès.

Le service ne possède aucun état durable.

---

## 6. Comportement du wizard par contexte

| Comportement | NewGameMainHero | CustomRecruit |
|---|---|---|
| création initiale déjà terminée | bloque | requise |
| destination | nouveau groupe initial | nouveau membre actif |
| API finale | `CreateInitialCharacter` | `FRPGCustomRecruitService` |
| groupe plein | sans objet | bloque |
| Annuler | retour menu principal | ferme le modal |
| callback Pawn | `HandleInitialCharacterCreated` | `HandleCustomRecruitCreated` |
| sauvegarde après succès | oui | oui |
| WBP | même wizard | même wizard |

---

## 7. Découpage MON20.5

### MON20.5.1 — Audit & Contract

Présent document.

Critère de sortie : architecture et règles de non-régression figées avant modification du wizard.

### MON20.5.2 — Context + Custom Recruit Transaction

Ajouter :

- `ERPGCharacterCreationContext` ;
- service `FRPGCustomRecruitService` ;
- résultat / raisons de rejet ;
- tests atomiques de création + recrutement.

Aucune modification UMG dans cette tranche.

### MON20.5.3 — Wizard Context Reuse

Adapter le même widget pour :

```text
NewGameMainHero
CustomRecruit
```

avec :

- validation contextuelle ;
- submit contextuel ;
- cancel contextuel ;
- aucune régression New Game.

### MON20.5.4 — Pawn Runtime Modal Integration

Ajouter l’entrée runtime :

```text
ShowCustomRecruitCharacterCreationWidget
HandleCustomRecruitCreated
CloseCustomRecruitCharacterCreationWidget
```

Réutiliser le guard d’input existant et assurer l’exclusion mutuelle avec MON20.4.

### MON20.5.5 — Recruiter Entry Point / Dungeon Authoring

Définir le point d’entrée utilisable depuis un niveau : objet / commande / callback existant, sans logique Blueprint métier parallèle.

La forme exacte sera choisie après validation de MON20.5.2 à MON20.5.4 afin de ne pas figer prématurément un nouveau type de GridObject.

### MON20.5.6 — PIE / Closure

Valider au minimum :

```text
New Game wizard non régressé
Custom Recruit ouvre le même wizard
navigation ne modifie pas le groupe
Annuler conserve le groupe
Validation ajoute exactement une recrue
CharacterId unique
classe / race / attributs / visuels corrects
spellbook créé
sauvegarde / recharge conserve la recrue
party full bloque sans mutation
Story Companion modal et Custom Recruit modal exclusifs
```

---

## 8. Tests cibles MON20.5.2

Filtre proposé :

```text
Grimrock.MON20.5.CustomRecruit
```

Première suite :

```text
ValidCreateAndRecruit
PartyFullAtomicReject
InvalidRequestAtomicReject
UniqueCharacterIdentity
AllocatedAttributesPreserved
VisualSelectionPreserved
RecruitmentRollbackLeavesNoPoolCandidate
InitialHeroStatePreserved
```

Les tests doivent vérifier les tableaux complets avant/après lorsqu’un scénario est rejeté.

---

## 9. Hors scope de MON20.5.1 / 20.5.2

Ne pas ajouter maintenant :

- `PartyMemberKind` persistant ;
- migration SaveGame ;
- économie / prix de recrutement ;
- packs d’équipement ;
- niveau dynamique des mercenaires ;
- réserve visible / gestion active-réserve ;
- dialogue généraliste ;
- second wizard ;
- Graph Blueprint de règles ;
- nouveau type de GridObject avant validation du runtime.

---

## 10. Conclusion

MON20.5 ne nécessite pas de reconstruire la création de personnage.

Le bon axe est :

```text
même wizard
+ contexte transitoire
+ nouvelle destination transactionnelle
+ CharacterPool / MON20.2
```

La prochaine implémentation autoritaire est :

```text
MON20.5.2 — Context + Custom Recruit Transaction
```
