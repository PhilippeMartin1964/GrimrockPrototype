# ARMEMENT-LOT02.19.2.1 — Icône composite de l'attaque double

## Origine du jalon

Retour PIE après 02.19.2 : l'attaque double fonctionne avec le raccourci 1, mais **l'icône montre uniquement la dague principale**, et induit en erreur. La séquence temporelle de deux frappes, de leurs sons et des événements concerne **ARMEMENT-LOT02.19.2.2** et n'est **pas** modifiée ici.

## Contrat de comportement

- `Attack_DualWield` doit afficher deux **icônes d'armes physiques** distinctes, et pas une illustration générique : la texture MainHand provient de l'attaque simple de la même instance et du même GUID que l'action double ; la texture OffHand provient de l'attaque simple disponible en OffHand.
- Les sources proviennent de `UGridTurnManagerComponent::GetAvailableCombatActions`, déjà autoritaire sur la compatibilité des mains. La vue du raccourci contient `bHasDualAttackIconPair`, `DualMainHandIcon`, `DualOffHandIcon` ; cette projection est recalculée à chaque refresh, **sans enregistrer** de nouvel état ni dupliquer d'arme.
- Toutes les vues résolues correspondant à `Attack_DualWield` peuvent porter les deux icônes, y compris si l'action est assignée à un autre emplacement. Le raccourci 1 est contextuel, et reste toujours `PrimaryAttack` dans le SaveGame.
- S'il manque une texture, une action offensive valide ou un élément Designer, l'ancienne `Image_ActionIcon` demeure utilisable ; un indicateur **« ×2 »** apparaît sur le badge numérique existant `Text_Quantity` afin de distinguer l'action double même dans l'ancien WBP.
- Si le Designer possède les trois éléments, la section composite apparaît et l'ancienne image unique est masquée. Les deux textures sont alimentées par `SetBrushFromSoftTexture`. Le badge « ×2 » reste visible ; il s'agit du **nombre de frappes**, pas d'une quantité d'objets.
- Lorsqu'une main est déséquipée, qu'un bouclier passif prend la place de l'OffHand ou que l'action redevient simple, la section composite disparaît, la texture unique revient et le badge « ×2 » s'efface.
- Ne pas modifier la sélection des actions, les coûts en PA, dégâts, animations, audio, journal, cooldown, inventaire, sauvegardes ou DataAssets.

## Implémentation C++

- `Source/GrimrockPrototype/Public/UI/GridCombatHudWidget.h` : trois propriétés transitoires dans `FGridCombatHudActionView` et trois liaisons facultatives `BindWidgetOptional` dans `UGridCombatHudActionWidget`.
- `Source/GrimrockPrototype/Private/UI/GridCombatHudWidget.cpp` : `ProjectEquippedHandAttacks` prépare les textures de la paire réelle ; `RefreshWidgets` commute entre la présentation simple et double. Aucun widget ni layout graphique de la nouvelle section n'est créé en C++.
- `GridMonsterMON12CombatHudTests.cpp` : `Grimrock.RPG.ARMEMENT.LOT02.19.2.1.ContextualDualAttackIcons` vérifie les vrais assets ARM001/ARM013, la paire d'icônes, le repli texte dans un widget sans couche composite, la couche composite facultative, le retour à l'attaque simple avec ARM075 et l'absence de mutation des raccourcis sauvegardés.

## Étape Designer WBP — obligatoire pour afficher les deux images

> **Statut à la publication C++ :** couche native prête, repli « ×2 » actif ; **binaire WBP non modifié** depuis GitHub. La projection graphique complète exige une sauvegarde du WBP par Unreal Editor, pas un fichier `.uasset` construit artificiellement. Tant que cette étape n'est pas effectuée et vérifiée en PIE, **ne pas clore le jalon graphique**.

- Asset : `/Game/GrimrockPrototype/Blueprints/UI/Combat/WBP_GridCombatHudAction`  
  Disque : `Content/GrimrockPrototype/Blueprints/UI/Combat/WBP_GridCombatHudAction.uasset`  
  Parent C++ : `UGridCombatHudActionWidget`.
- Dans le **Designer**, sous `Overlay_ActionContent`, conserver `Image_ActionIcon`, `Text_ShortcutNumber` et `Text_Quantity`. Ajouter un nouvel **Overlay** `Overlay_DualAttack` comme **frère de** `Image_ActionIcon`, avec deux `Image` enfants :
  - `Image_DualMainHand` : texture de la main principale, idéalement moitié gauche du raccourci.
  - `Image_DualOffHand` : texture de la main secondaire, moitié droite, léger recouvrement central si désiré.
- Pour ces trois nouveaux widgets, cocher **Is Variable** pour autoriser le `BindWidgetOptional`. Les dimensions, le centrage, l'opacité et le recouvrement doivent rester **éditables dans le Designer**. Préférer deux surfaces carrées de taille similaire, qui ne déforment pas les textures. L'Overlay parent doit initialement être **Collapsed** ; le C++ l'active uniquement si les deux armes sont valides.
- `Image_ActionIcon` reste comme repli et sera masquée automatiquement pendant l'action double si l'Overlay complet est présent. Conserver la visibilité de `Text_Quantity` et son cadre pour « ×2 ». Laisser les éléments interactifs du raccourci inchangés (pas de widget bloquant la souris).
- **Aucun Event Graph requis.** Il ne faut pas ajouter de logique de comparaison d'armes, de cast vers l'inventaire ou de binding Tick au Blueprint. C++ contrôle les textures et la visibilité.
- Compiler et enregistrer le WBP **dans UE5.5.4**, puis vérifier l'asset modifié avec Git et l'intégrer au jalon lors de sa matérialisation. Ne pas prétendre que les trois widgets existent avant sauvegarde réelle.

Schéma logique du seul emplacement d'action (mise en page finale libre dans le Designer) :

~~~text
WBP_GridCombatHudAction / Button_Action
└─ Overlay_ActionContent
   ├─ Image_ActionIcon           [attaque simple / repli]
   ├─ Overlay_DualAttack         [Collapsed par défaut]
   │  ├─ Image_DualMainHand      [arme physique MainHand]
   │  └─ Image_DualOffHand       [arme physique OffHand]
   ├─ Text_Quantity              [x12 pour consommable OU ×2 pour double]
   └─ Text_ShortcutNumber        [1]
~~~

## Localisation — checklist du ticket

Références appliquées : `AGENTS.md`, `docs/UI_UMG_PRESENTATION_CONTRACT.md` et `docs/UI_WBP_LOCALIZATION_WORKFLOW.md`.

| Élément visible | Autorité du texte | État |
| --- | --- | --- |
| « ×2 » | Notation **mécanique** indépendante de la langue | Aucun nouvel identifiant ST_UI |
| « 1 » | Notation numérique de raccourci existante | Inchangée |
| « Attaque double », coût/description | `FGridCombatActionDefinition` déjà créée au jalon 02.19.2 ; `NSLOCTEXT` et clés existantes `Combat.Hotbar.Tooltip.*` | Aucun nouveau texte dans ce ticket |
| Noms/images ARM001 et ARM013 | `UGridItemDefinitionAsset` + icônes `FGridCombatActionDefinition` | Réutilisés dynamiquement |

**État de la chaîne :** `Localization/Source/UI.csv` **inchangé** → `ST_UI.uasset` **inchangé** (aucune clé nouvelle à matérialiser) → `WBP_GridCombatHudAction.uasset` **en attente d'authoring Designer UE** → `Game/fr/en` **aucune nouvelle chaîne à traduire** dans ce ticket ; le contrôle linguistique global de 02.19.2 reste distinct. Les champs visuels libres ne doivent pas contenir de texte français dupliqué.

## Recette de validation PIE

1. Voleur / Ombre neuf : MainHand ARM001, OffHand ARM013. La barre doit montrer une **composition distincte dague + Main-gauche** dans la case 1, avec « ×2 », et les deux attaques simples dans les premiers raccourcis libres.
2. Dans le WBP antérieur à l'authoring, **repli attendu** : icône de dague seule **avec** badge « ×2 » ; ne pas déclarer la composition validée sur cette base.
3. Déplacer/remplacer une arme : les images suivent les textures des armes actuelles, pas un identifiant « Voleur/Ombre » codé en dur.
4. Déséquiper OffHand, équiper ARM075 ou équiper une arme à deux mains : l'icône simple revient et le badge « ×2 » disparaît.
5. Vérifier que les chiffres 0–9 et les 10 raccourcis personnalisables, le clic, le survol, l'opacité indisponible, le drag/drop et les tooltips n'ont pas régressé.
6. Exécuter les tests `Grimrock.RPG.ARMEMENT.LOT02.19.2.1`, toute la suite `Grimrock` par l'agent Windows, puis vérifier visuellement le rendu et le cadrage **PIE**. `LocalAuto=success` ne démontre pas l'authoring du WBP ni la qualité de la composition.

## Suivant

`ARMEMENT-LOT02.19.2.2` : séquencement temporel des deux frappes, animation, sons, impact et publication des événements, indépendamment du rendu de l'icône.
