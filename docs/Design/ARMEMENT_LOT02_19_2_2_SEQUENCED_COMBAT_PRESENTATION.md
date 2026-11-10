# ARMEMENT-LOT02.19.2.2 — Séquence audio, impacts et flux d'événements

## Décision de gameplay et périmètre

Retour PIE du 10.11.2026 après ARMEMENT-LOT02.21 : le Voleur/Ombre frappe avec **ARM001 en MainHand puis ARM013 en OffHand**, mais la présentation et les lignes du flux d'événements arrivent simultanément. Une double attaque doit être compréhensible **dans le temps**, sans ajouter des raccourcis pour les attaques simples.

**Contrat implémenté :** le TurnManager conserve une transaction autoritaire **atomique** (3 PA une fois, deux profils/dés, deuxième coup 50 % avant armure, réactions de statut entre les jets, suppression du coup secondaire si la première frappe tue). La **restitution audiovisuelle, le feedback et le flux d'événements** sont ordonnancés indépendamment, sur des frames différentes. Aucun timer/acteur d'impact graphique ne réapplique des dégâts. Il ne s'agit **pas** d'une attente réelle avant mutation des PV ou des statuts : décaler l'autorité gameplay exigerait un chantier spécifique (interactions AI/sauvegarde/réactions). Ce point est explicite pour ne pas présenter un simple délai de HUD comme une exécution des dégâts différée.

## Timeline réutilisable des actions multi-frappes

Le moteur fournit une identité d'action commune `ActionInstanceId`, un identifiant distinct `RequestId` par frappe, `ResolutionIndex` et `ResolutionCount` à chaque `FGridPlayerAttackRequest`. Les frappes du composant `UGridPlayerAttackPresentationComponent` sont mises en file **seulement si `ResolutionCount > 1`**. Les attaques à résolution unique, y compris shuriken et sorts, conservent leur présentation existante.

La timeline est définie par le **profil de présentation de chaque arme** `FGridPlayerAttackPresentationProfile` :
- `ImpactDelaySeconds` : délai début → impact, valeur par défaut **0,16 s** (plage 0,01–2,0 s) ;
- `RecoveryDelaySeconds` : délai impact → prochain départ, valeur par défaut **0,18 s** (0–2,0 s) ;
- `MotionDurationSeconds` : animation d'objet tenu déjà authorée, durée intégrée au minimum de récupération lorsqu'elle est réellement activée.

Avec les valeurs par défaut, et si les deux frappes ont eu lieu :
| Temps approximatif | Action | Audio / événement |
| --- | --- | --- |
| 0,00 s | MainHand, départ du coup | `AttackAudio` ARM001 + VFX/mouvement si authorés |
| 0,16 s | MainHand, impact réel issu du résultat déjà calculé | `ImpactHitAudio` ou `ImpactMissAudio` ARM001 + feedback + **première ligne du flux** |
| 0,34 s | OffHand, départ du deuxième coup | `AttackAudio` ARM013 + VFX/mouvement si authorés |
| 0,50 s | OffHand, deuxième impact distinct | `ImpactHitAudio` ou `ImpactMissAudio` ARM013 + feedback + **deuxième ligne du flux** |
| 0,68 s | fin de récupération | libération du verrou transitoire d'action et du reste du flux (mort/victoire/tour) |

Le temps est un exemple **data-driven**, pas une valeur codée en dur pour la dague. Une frame anormalement longue est bornée à **0,10 s** dans le Tick de la présentation pour ne pas faire retentir deux coups à la même image. Aucun son n'est joué si les tableaux d'assets sont vides.

## Architecture modifiée

- `GridTurnManagerPlayerActions.cpp` : chaque résultat conserve son `RequestId`, mais transporte `ActionInstanceId` commun et les indices de résolution. Les profils d'armes, réactions et dégâts restent inchangés.
- `GridPlayerAttackPresentationComponent.cpp` : ordonnanceur `FScheduledStrike` avec départ, impact et récupération. `AttackAudio` et `ImpactAudio` restent propres au **vrai ItemDefinitionId de chaque coup**. Le VFX et le feedback appartiennent au même événement temporel. Le Tick se désactive après vidage de file.
- `GridTurnManagerComponent.cpp` : `AppendCombatLogEntry` continue d'écrire **immédiatement l'historique autoritaire** et de diffuser `OnCombatLogEntryAdded` ; seule la publication des entrées dans `UGridEventFeedSubsystem` est momentanément différée. `PublishSequencedAttackFeedThroughNextImpact` laisse sortir une seule ligne d'attaque par impact et `FinishSequencedAttackFeed` vide les événements restants dans leur ordre (défaite du monstre, victoire, etc.). Une nouvelle rencontre/Abort/EndPlay écarte les restes périmés.
- `GridTurnManagerPhases.cpp` : pendant la courte timeline multi-coups, `IsPartyAtRest` bloque les nouvelles actions/fin de tour ; lorsque les effets visuels sont terminés, les possibilités d'action redeviennent disponibles. Si le personnage n'a plus de PA, le tour suivant est lancé **après** la séquence, sans animation de monstre intercalée entre les deux impacts.
- Les tests hérités MON11 restent valables pour `ResolutionCount == 1`. Le nouvel ordonnanceur est **réutilisable pour d'autres attaques ayant `ResolutionCount > 1`**, sans switch dédié à `Attack_DualWield`.

## Situations sécurisées

1. **Première frappe létale :** le moteur n'émet pas de deuxième requête ; le scheduler n'invente pas de coup, et conserve le son, le résultat et le feedback du premier avant de vider le flux de mort/victoire.
2. **Une des armes remplacée pendant la résolution :** le moteur revalide physiquement les deux GUID et ne crée aucune seconde frappe fictive.
3. **Audio manquant :** pas de faux bruitage ni de piste générique détournée ; les sorties média restent vides tout en conservant les événements temporels, VFX et le journal.
4. **Quitter/recharger/abandonner le combat :** files transitoires annulées ; elles ne sont pas sérialisées dans le SaveGame.
5. **Nouvelle action pendant la séquence :** refus `PartyBusy` tant que la présentation précédente n'est pas terminée ; pas de double paiement de PA ni de séquence chevauchante.
6. **Autres modes de combat :** attaques simples, arcs, magie, consommables inchangés ; aucune action supplémentaire créée dans le HUD.

## Vérifications audio nécessaires dans Unreal Editor

**Le dépôt GitHub confirme des dossiers audio de décor/interaction mais ne permet pas d'attester, sans inspection des vrais DataAssets, que ARM001 et ARM013 ont des sons de frappe/impact.** Le code ne fabrique pas d'assets sonores.

Sur `DA_Weapon_Dagger_01` (ARM001) et `DA_Weapon_MainGauche_01` (ARM013), vérifier :
1. `bProvidesAttackPresentation = true`.
2. `PlayerAttackPresentationProfile.AttackAudio.Sounds` : bruit de mouvement de lame spécifique à cette arme (whoosh).
3. `ImpactHitAudio.Sounds` : vrai bruitage de contact ; `ImpactMissAudio.Sounds` : mouvement manqué (si disponible).
4. `MotionStyle`, `ImpactDelaySeconds`, `RecoveryDelaySeconds` et éventuellement `MotionDurationSeconds` : adapter en PIE, sans exagérer les délais.
5. Cocher les modes `bAudioEnabled` et `bNativeAudioPlaybackEnabled` du composant de présentation. Les deux armes doivent être audibles **à des moments distincts**.
6. Éviter les sons `ImpactHit` sur un miss, éviter un son de coup sur une deuxième frappe absente, et ne pas assigner d'échantillon audio d'un autre événement sans pertinence.

**Validation artistique bloquée** tant que les vrais sons ne sont pas confirmés dans UE et écoutés en PIE ; `UE5.5.4/LocalAuto=success` ne prouve ni la présence des sons ni leur synchronisation perçue.

## Tests

- `Grimrock.RPG.ARMEMENT.LOT02.19.2.2.SequencedStrikePresentation` : avant impacts, MainHand uniquement ; ensuite une frappe et son feedback ; puis OffHand ; puis deuxième impact ; nettoyage fin de séquence et victoire après première frappe létale, pas de seconde fantôme.
- `Grimrock.Monsters.MON11.Presentation.ProfileValidation` : bornes des timings.
- `Grimrock.RPG.ARMEMENT.LOT02.19.2.N1DualWieldTwoProfilesOneCost` : 3 PA et deux calculs distincts inchangés.
- Régression complète `Grimrock` par VALIDATION-AUTO01, statuts `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` sur le SHA final.
- Contrôle PIE obligatoire : vérifier **flux des deux lignes à deux moments distincts**, mouvements/sons correspondants et absence de chevauchement, monstre tué sur le premier coup, refus d'une deuxième attaque pendant la récupération, puis retour à la normale.

## UI et localisation

Contrats `AGENTS.md`, `docs/UI_UMG_PRESENTATION_CONTRACT.md`, `docs/UI_WBP_LOCALIZATION_WORKFLOW.md` relus. Aucun ajout de champ visible, widget ni texte. Les textes de combat existants restent dans `FGridCombatLogFormatter` / `FGridPlayerAttackFeedbackFormatter` et leurs `FText` localisés ; seule la **date de publication** change. `UI.csv` **inchangé** → `ST_UI.uasset` **inchangé** (aucune nouvelle clé) → WBP **inchangés** → `Game/fr/en` **aucune nouvelle traduction nécessaire pour ce jalon**. Les traductions antérieures non attestées restent un audit séparé.

## Non-objectifs

Ne change pas la palette `DA_EquippedWeaponActionIcons`, l'unique raccourci de frappe sur `1`, l'armement ARMXXX ni l'arbre de compétences. Les changements au moteur authoritative différant **réellement les dégâts** jusqu'aux impacts nécessiteraient une refonte transactions/réactions/save/load distincte, non introduite subrepticement par une présentation.
