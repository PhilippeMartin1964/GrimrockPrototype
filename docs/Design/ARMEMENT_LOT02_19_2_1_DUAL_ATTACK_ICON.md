# ARMEMENT-LOT02.19.2.1 — Simplification de l'icône d'attaque double

> **CONTRAT DE BARRE ACTUEL (ARMEMENT-LOT02.21) :** les modes d'armement disposent désormais d'une **unique attaque normale contextuelle** sur le raccourci 1, sans suggestion automatique des attaques simples en emplacements 2+. Cette règle ne modifie pas la sélection d'une seule icône `Image_ActionIcon` exposée par le présent jalon. Voir [contrat actuel](ARMEMENT_LOT02_21_SINGLE_EQUIPPED_ATTACK_HOTBAR.md).

> **NOTE DE CONSOLIDATION (10.11.2026) :** ce texte est la recette historique du remplacement du badge « ×2 » par une icône unique. Le contrat actuel, élargi à toutes les familles ARM001–ARM082 et **prioritaire**, figure dans [ARMEMENT-LOT02.20](ARMEMENT_LOT02_20_EQUIPPED_ACTION_ICON_FOUNDATION.md). La propriété DualAttackIcon indiquée ci-dessous est désormais **un fallback transitoire** : la nouvelle propriété EquippedWeaponActionIconPalette du même WBP a priorité dès qu'une icône de famille est renseignée. La hiérarchie Image_ActionIcon reste inchangée.

## Décision corrigée après capture du WBP utilisateur (09.11.2026)

Le système de superposition proposé initialement (deux `UImage`, un `Overlay_DualAttack`, un badge « ×2 ») est **abandonné** : il compliquait inutilement un raccourci de combat. Une action combinée doit se reconnaître à **une icône d'action dédiée**, comme les actions génériques des RPG de type Divinity: Original Sin 2. Ce visuel **ne dépend pas de l'identité des deux armes** : il est remplacé seulement si l'action disponible change.

Le changement reste strictement visuel. L'attaque double conserve **3 PA, deux jets distincts, MainHand 100 % et OffHand 50 %**. Le séquencement des sons/impacts/journal reste explicitement reporté à 02.19.2.2.

## Structure réelle du Designer — à conserver à l'identique

Asset : `/Game/GrimrockPrototype/Blueprints/UI/Combat/WBP_GridCombatHudAction`  
Fichier : `Content/GrimrockPrototype/Blueprints/UI/Combat/WBP_GridCombatHudAction.uasset`  
Parent C++ : `UGridCombatHudActionWidget`.

D'après la capture utilisateur, la hiérarchie pertinente est :

~~~text
WBP_GridCombatHudAction
└─ SizeBox_ActionRoot
   └─ Button_Action
      └─ Overlay_Action
         ├─ SizeBox_ActionIcon
         │  └─ Image_ActionIcon        <- SEULE image d'action
         └─ Text_ShortcutNumber         <- numéro 1 à 0, déjà existant
~~~

La capture ne prouve pas l'absence d'autres widgets hors du cadrage ; cette arborescence est **la partie constatée**. Conserver toutes les autres sections existantes et **toutes les tailles, marges, alignements et styles** réglés par l'utilisateur. Aucun changement d'ancres, de dimensions ni d'Event Graph.

**Ne rien ajouter à cette hiérarchie.** Ne pas créer `Overlay_ActionContent`, `Overlay_DualAttack`, `Image_DualMainHand`, `Image_DualOffHand` ou une étiquette « ×2 ». Ces noms appartiennent à l'ancienne proposition et ne sont plus utilisés par le C++.

## Configuration minimale : une propriété

Après compilation du C++ dans UE5.5.4 :

1. Ouvrir `WBP_GridCombatHudAction`.
2. Cliquer **Class Defaults**. Dans **Combat → HUD → Icons**, trouver `Dual Attack Icon` (propriété native `DualAttackIcon`).
3. Affecter une seule texture `Texture2D` propre à l'action double, représentant par exemple **deux dagues croisées**, et non la dague ARM001 seule. L'icône devrait être carrée, avec fond transparent, et conserver les styles de `Image_ActionIcon` définis dans le Designer.
4. Compiler et sauvegarder ce **même WBP**, puis vérifier la modification de son `.uasset` avec Git.

Si aucune texture dédiée n'existe encore, il faut **la créer/importer via Unreal Editor** ou sélectionner un visuel existant adapté. Le dépôt vérifié ne comporte actuellement pas de texture clairement nommée « DualAttack ». La propriété n'est pas automatiquement renseignée et **le C++ conserve temporairement l'icône principale si elle reste vide**. Ne pas déclarer le résultat graphique terminé avant affectation et vérification de la texture.

## Code C++ révisé

- `UGridCombatHudActionWidget::DualAttackIcon` : unique `TSoftObjectPtr<UTexture2D>` configurable dans les Class Defaults du WBP.
- `ResolveDisplayedIcon(View, DualAttackIcon)` : si le raccourci a une action `Attack_DualWield` valide **et** si l'icône dédiée est renseignée, retourne cette seule texture ; sinon retourne `Action.Definition.Icon` d'origine.
- `RefreshWidgets` alimente exclusivement **`Image_ActionIcon`**, sans widget supplémentaire ni composition d'images.
- `Text_Quantity` et son ancien fallback runtime restent exclusivement réservés aux **quantités de QuickItems** ; **« ×2 » est supprimé**.
- `ProjectEquippedHandAttacks` ne contient plus la projection de deux textures individuelles. Il conserve le fonctionnement validé de la barre d'action : 1 = combinaison, puis les attaques simples dans les premiers emplacements libres sans écraser les raccourcis du joueur.
- Aucun changement de source d'attaque, sauvegarde, coût, cible, dégâts, bruitage ou animation.

Tests : `Grimrock.RPG.ARMEMENT.LOT02.19.2.1.ContextualDualAttackIcons` refait pour vérifier l'attaque double ARM001/ARM013, le choix d'une **seule** icône dédiée, le fallback non configuré, l'absence de « ×2 », la conservation du widget `Image_ActionIcon` et le retour à l'icône normale lorsque la main secondaire porte ARM075.

## Texte et localisation : contrôle en ouverture et en clôture

Documents d'autorité : `AGENTS.md`, `docs/UI_UMG_PRESENTATION_CONTRACT.md`, `docs/UI_WBP_LOCALIZATION_WORKFLOW.md`.

| Surface | Source | Modification |
| --- | --- | --- |
| `Text_ShortcutNumber` | Notation mécanique existante | Aucune |
| `Text_Quantity` | Quantité mécanique d'objet (QuickItem) | Retrait de « ×2 » uniquement |
| Tooltip « Attaque double » | `NSLOCTEXT` du catalogue, existant depuis 02.19.2 | Aucune chaîne nouvelle |
| `Image_ActionIcon` | `Texture2D` d'action (WBP) ou DataAsset pour actions simples | Une propriété d'icône dédiée, sans texte |

**État vérifiable :** `UI.csv` inchangé → `ST_UI.uasset` inchangé (aucune nouvelle clé) → WBP binaire **inchangé côté GitHub** tant que l'éditeur local ne l'a pas sauvegardé → `Game/fr/en` inchangé par ce ticket. Aucune matérialisation ST_UI requise **pour ce jalon**, ni nouveau texte joueur. Les chaînes NSLOCTEXT déjà ajoutées au jalon 02.19.2 restent à auditer dans le gather global fr/en.

## Recette PIE

Dans `L_Dungeon` avec un Voleur-Ombre :

- En `MainHand + OffHand` offensives : raccourci 1 affiche **une seule icône dédiée** (et non la dague ARM001), sans « ×2 ». Raccourcis simples de mains disponibles à côté.
- Sans texture dédiée assignée : **repli provisoire à l'icône de la dague**, aucune donnée perdue, mais correction **visuelle non validée**.
- Déséquiper l'OffHand ou équiper un bouclier : retour immédiat à la vraie icône de la main principale, sans changement de géométrie du raccourci.
- Contrôler avec des actions non doubles (sorts, consommables, frappe simple) qu'elles conservent leur icône, quantité d'objet, raccourci, tooltip et interaction.
- Build et Automation `Grimrock` sur le SHA exact via VALIDATION-AUTO01 ; la mise en page du WBP et la texture choisie exigent une validation PIE distincte.

## Suite

`ARMEMENT-LOT02.19.2.2` : vraie séquence temporelle réutilisable pour les coups, sons, animations, impacts et événements. **Ne pas introduire de retards audio artificiels dans le jalon de l'icône.**
