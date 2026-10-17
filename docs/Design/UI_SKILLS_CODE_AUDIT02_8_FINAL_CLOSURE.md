# UI-SKILLS-CODE-AUDIT02.8 — Regression Coverage & Final Closure

Date : **17 octobre 2026**  
Projet : **GrimrockPrototype / Unreal Engine 5.5.4**  
État : **IMPLÉMENTÉ — documentation uniquement ; validation fonctionnelle déjà reçue**

## 1. Objet

Ce jalon ferme l'audit de qualité du sous-système Skills après les corrections
02.1 à 02.7.

Aucun nouveau comportement runtime n'est introduit par 02.8. Le travail consiste
à vérifier que chaque défaut P1/P2 identifié dispose désormais d'une correction
et d'une preuve de non-régression, puis à aligner la documentation sur l'état
réel du dépôt.

## 2. Matrice de clôture

| Jalon | Défaut traité | Garde de non-régression | Validation reçue |
| --- | --- | --- | --- |
| 02.1 | `DiscouragedClasses` perdu avant le détail UMG | Pipeline DetailProjection + DetailWidget Formatting/RejectAndClear/WidgetContract + LOCALIZATION02 | Skills02 25/25 ; LOCALIZATION02 1/1 |
| 02.2 | état `Rank > CurrentRankCap` accepté par la lecture | SKILL01 DerivedBalance + Skills02 DetailReadModel/Pipeline + MON20.8 | Skills02 25/25 ; SKILL01 8/8 ; MON20.8 8/8 |
| 02.3 | undo de session obsolète après mutation externe | `Skills02.SessionUndo.Reconciliation` | Skills02 26/26 ; SKILL01 8/8 |
| 02.4 | tri chaîne non localisé + dépendance ordinal enum | tests Name/Attribute/Affinity/Rank | Skills02 27/27 |
| 02.5 | rescans catalogue + rechargements icônes répétés | `Architecture.CanonicalSkillCacheContract` | Skills02 28/28 |
| 02.6 | bridge `Text_SkillPoints` et clés legacy encore actifs | `Summary.WidgetContract` + DESC01 + TextCentral + LOCALIZATION02 | Skills02 28/28 ; DESC01 24/24 ; TalentStringTableRuntime 1/1 ; LOCALIZATION02 1/1 |
| 02.7 | `SkillId` technique pouvant fuiter dans le feedback + validateur de ligne dupliqué | `ErrorBoundary.PlayerFacingSkillName` + RPG05.Feedback | Skills02 29/29 ; RPG05.Feedback 4/4 |

## 3. Couverture finale

Après 02.7, la suite principale est :

~~~text
Grimrock.UI.Skills02
    29 tests
    29 PASS
    0 warning
    0 failure
~~~

Dernière validation reçue :

~~~text
TD04-20261017-155827
~~~

Les autorités adjacentes ont également été validées pendant la campagne :

~~~text
Grimrock.RPG.SKILL01
    8/8 PASS

Grimrock.MON20.8.SkillsPage
    8/8 PASS

Grimrock.UI.RPG05.Feedback
    4/4 PASS

Grimrock.UI.RPG.DESC01
    24/24 PASS

Game.UI.TextCentral.CENTRAL01.TalentStringTableRuntime
    1/1 PASS

Game.UI.Localization.LOCALIZATION02.FullGameGatherContract
    1/1 PASS
~~~

Le dernier résultat LOCALIZATION02 est :

~~~text
TD04-20261017-155220
~~~

## 4. Architecture finale

Autorités :

~~~text
FGridCharacterInventoryState::SkillRanks
    -> état durable

FRPGSkillPointService
    -> économie / cap / achat / remboursement

FGridSkillsPageService
    -> construction du read-model

FGridSkillEntryView / FGridSkillDetailView / FGridSkillsPageView
    -> invariants structurels de présentation

UGridSkillsWidget
    -> orchestration UI
    -> cache transitoire par instance
    -> historique undo de session uniquement

URPGSkillPresentationAsset
    -> affinités / icônes / métadonnées de présentation

ST_UI / WBP
    -> vocabulaire statique et présentation Designer
~~~

Aucune monnaie Skill parallèle, aucun cache global de catalogue et aucun undo
persisté n'ont été introduits.

## 5. Dettes P3 restantes

Les points suivants restent volontairement hors correction car ils ne
constituent pas des défauts fonctionnels dans le catalogue actuel de 25 Skills :

~~~text
Session undo key
    -> FString composite CharacterId|SkillId
    -> pourrait devenir une structure typée si le système grossit

Detail / synergy lookup
    -> recherches linéaires sur un petit catalogue
    -> micro-optimisation non justifiée actuellement

Presentation structural validation
    -> certains IDs de classe inconnus peuvent rester structurellement valides
    -> les audits/commandlets de production contrôlent le catalogue réel

Progression feedback LOCTEXT
    -> reste une autorité C++ légitime pour des messages système
    -> pas une duplication de contenu Skill authoré
~~~

Ces éléments sont classés **P3 / différés**, sans blocage pour la suite du
projet.

## 6. Correction documentaire 02.1

Le document 02.1 conservait encore une commande historique incorrecte :

~~~text
Game.UI.Localization.LOCALIZATION01.LocalizationTargetContract
~~~

L'assertion modifiée pour `Skill.Affinity.DiscouragedClasses` appartient
réellement à :

~~~text
Game.UI.Localization.LOCALIZATION02.FullGameGatherContract
~~~

02.8 corrige cette trace. Le contrat complet a depuis été validé 1/1.

## 7. Conclusion

État de l'audit :

~~~text
P1 identifiés     -> 0 ouvert
P2 identifiés     -> 0 ouvert
P3 bloquants      -> 0
Skills02          -> 29/29 PASS
~~~

`UI-SKILLS-CODE-AUDIT02` peut être déclaré **VALIDÉ / CLOS** sur la base des
validations locales reçues pour chaque modification fonctionnelle.

02.8 ne changeant que la documentation, aucune relance Automation n'est requise
pour ce commit de clôture.
