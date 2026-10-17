# UI-SKILLS-CODE-AUDIT02.3 — Session Undo Reconciliation

Date : **17 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**  
État : **IMPLÉMENTÉ — validation locale requise**

## 1. Défaut

L'undo Skill est volontairement limité à la session UI courante :

~~~text
SessionSkillRankFloors
SessionPurchasedSkillRanks
~~~

Après un achat effectué dans l'écran Skills, l'historique représente :

~~~text
ExpectedRank = SessionFloor + SessionPurchaseCount
~~~

Avant 02.3, si le rang autoritaire changeait depuis un autre chemin pendant que
l'écran restait ouvert, cet historique n'était pas réconcilié. Un bouton de
diminution pouvait donc rester disponible à partir d'une hypothèse devenue
obsolète.

## 2. Règle de réconciliation

Lors de `RefreshSkills()`, chaque Skill disposant d'un historique de session
est comparé au rang autoritaire projeté.

L'historique reste valide uniquement si :

~~~text
PurchaseCount > 0
SessionFloor >= 0
AuthoritativeRank == SessionFloor + PurchaseCount
~~~

Sinon, seule la paire `(CharacterId, SkillId)` concernée est invalidée :

~~~text
SessionPurchasedSkillRanks.Remove(Key)
SessionSkillRankFloors.Remove(Key)
~~~

Les autres Skills et les autres personnages ne sont pas affectés.

L'état gameplay n'est jamais modifié par cette réconciliation.

## 3. Broadcast synchrone des transactions internes

`FRPGSkillPointService::TryPurchaseNextRank()` et
`TryRefundPurchasedRank()` notifient l'inventaire de façon synchrone.

Pendant un achat normal :

~~~text
rang autoritaire change
    -> OnPartyInventoryChanged
    -> RefreshSkills
    -> retour transaction
    -> RecordSessionSkillPurchase
~~~

et pendant un remboursement :

~~~text
rang autoritaire change
    -> OnPartyInventoryChanged
    -> RefreshSkills
    -> retour transaction
    -> ConsumeSessionSkillPurchase
~~~

Le refresh intermédiaire ne doit donc pas interpréter le décalage temporaire
comme une mutation externe.

`UGridSkillsWidget` possède désormais un garde transitoire :

~~~text
bSkillAllocationTransactionInProgress
~~~

Les deux appels transactionnels sont entourés par `TGuardValue<bool>`.
La réconciliation est suspendue uniquement pendant leur broadcast synchrone.
Le `RefreshSkills()` final s'exécute ensuite sans suppression et vérifie le
nouvel état complet.

## 4. Propriété de l'undo inchangée

02.3 ne déplace aucune autorité :

~~~text
SkillRanks durable                -> FGridCharacterInventoryState
transactions                      -> FRPGSkillPointService
historique d'undo de la fenêtre   -> UGridSkillsWidget uniquement
~~~

L'historique n'est ni sérialisé, ni déplacé dans SaveGame, ni transformé en
monnaie persistante.

`BeginSkillAllocationSession()` continue de réinitialiser les deux maps à
chaque ouverture de la fenêtre Skills.

## 5. Test de non-régression

Nouveau test :

~~~text
Grimrock.UI.Skills02.SessionUndo.Reconciliation
~~~

Il vérifie :

~~~text
historique cohérent                   -> conservé
broadcast transaction interne         -> réconciliation suspendue temporairement
mutation autoritaire externe +rang    -> historique de la Skill invalidé
mutation autoritaire externe -rang    -> historique de la Skill invalidé
autre Skill                           -> historique préservé
historique partiel/corrompu           -> invalidé défensivement
~~~

## 6. Non-objectifs

02.3 ne modifie pas :

~~~text
FRPGSkillPointService règles métier
coût ou cap des Skills
read-model invariants 02.2
affinités
tri
localisation
UMG
.uasset
SaveGame
~~~

## 7. Validation locale

~~~powershell
cd D:\Development\GrimrockPrototype

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Skills02"
~~~

Puis l'autorité transactionnelle existante :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.SKILL01"
~~~

Ne pas déclarer `UI-SKILLS-CODE-AUDIT02.3` clos avant les résultats locaux.
