# MON12.2 — Attaques depuis le panneau d’actions

> **Statut historique remplacé.** Les clics directs `MainHand / OffHand`, leur
> adaptateur de widget et leurs tests ont été supprimés après MON12.7. Les
> attaques sont maintenant générées depuis le catalogue d'actions et exécutées
> par `RequestCharacterCombatAction()`. Voir
> `MON12_7_ACTION_ORIENTED_COMBAT_HUD.md`.

## Résultat

MON12.2 rend les deux mains du panneau MON12.1 cliquables :

- un clic sur `MainHand` demande une attaque avec `MainHand` ;
- un clic sur `OffHand` demande une attaque avec `OffHand` ;
- une main vide utilise l’attaque à mains nues existante ;
- un objet équipé non offensif, notamment la torche, reste affiché mais son
  bouton est désactivé ;
- une attaque acceptée dépense 2 PA ; le personnage reste `Active` à 2/4 PA,
  puis devient `Completed` à 0/4 PA ;
- une demande refusée ne consomme ni action ni objet.

Le widget ne sélectionne aucune cible, ne calcule aucun dégât et ne modifie
pas l’inventaire. Il transmet seulement le personnage et la main cliquée au
`UGridTurnManagerComponent`.

## Architecture

### Routage du clic

`UGridCombatActionPanelWidget` lie nativement :

- `Button_MainHand` à `MainHand` ;
- `Button_OffHand` à `OffHand`.

Les deux gestionnaires appellent `RequestAttackFromSlot()`. Cette fonction
transmet l’index résolu du panneau et le slot au TurnManager, puis relit les
sources réelles pour actualiser le rendu.

### Autorité du TurnManager

`RequestCharacterAttackFromSlot()` est la nouvelle entrée autoritaire. Elle
réutilise toute la chaîne MON11 :

1. vérification du combat et de la phase joueur ;
2. vérification du personnage et de son action disponible ;
3. résolution du profil offensif de la seule main demandée ;
4. recherche de la cible dans l’axe du groupe ;
5. validation de la portée et des obstacles ;
6. résolution et diffusion de l’attaque ;
7. présentation existante, y compris le lancer du shuriken.

La méthode historique `RequestCharacterAttack()` reste inchangée pour
`NumPad 7` : elle recherche automatiquement `MainHand`, puis `OffHand`, puis
l’attaque à mains nues. MON12.2 n’introduit donc aucune régression dans la
commande de diagnostic MON11.

### État des boutons

`FGridCombatActionSlotView::bCanAttack` est un état de présentation :

| Contenu du slot | `bCanAttack` | Comportement |
| --- | ---: | --- |
| arme offensive valide | `true` | clic transmis au TurnManager |
| main vide | `true` | attaque à mains nues |
| torche, bouclier ou objet non offensif | `false` | icône visible, bouton désactivé |
| définition absente ou invalide | `false` | bouton désactivé |

Cette propriété ne remplace pas `CanCharacterAct()`. Le bouton n’est actif
que si le slot fournit une action **et** si le TurnManager autorise le
personnage à agir.

## Modification manuelle du Widget Blueprint

Aucun `.uasset` n’est modifié automatiquement. Mettre à jour le
`WBP_GridCombatActionPanel` créé pour MON12.1 après compilation du C++.

### 1. Insérer les deux boutons

1. Ouvrir
   `/Game/GrimrockPrototype/Blueprints/UI/Combat/WBP_GridCombatActionPanel`.
2. Dans **Designer > Hierarchy**, repérer `Overlay_MainHand`.
3. Cliquer avec le bouton droit sur `Overlay_MainHand`.
4. Choisir **Wrap With... > Button**.
5. Renommer le nouveau bouton exactement `Button_MainHand`.
6. Cocher **Is Variable** pour `Button_MainHand`.
7. Vérifier la hiérarchie :

   ```text
   SizeBox_MainHand
   └── Button_MainHand
       └── Overlay_MainHand
           ├── Border_MainHand
           ├── Image_MainHandIcon
           └── Text_MainHandQuantity
   ```

8. Répéter l’opération sur `Overlay_OffHand`.
9. Renommer le bouton `Button_OffHand` et cocher **Is Variable**.
10. Vérifier la hiérarchie :

    ```text
    SizeBox_OffHand
    └── Button_OffHand
        └── Overlay_OffHand
            ├── Border_OffHand
            ├── Image_OffHandIcon
            └── Text_OffHandQuantity
    ```

Si **Wrap With...** n’est pas disponible, couper temporairement l’Overlay,
placer un `Button` comme enfant du `SizeBox`, puis recoller l’Overlay comme
enfant unique du bouton.

### 2. Régler les boutons

1. Sélectionner `Button_MainHand`.
2. Dans son slot, choisir l’alignement horizontal et vertical **Fill**.
3. Dans **Interaction**, laisser **Is Enabled** coché : le C++ modifiera cette
   valeur à l’exécution.
4. Laisser **Click Method** à `Down And Up`.
5. Régler les styles `Normal`, `Hovered` et `Pressed` selon le cadre du HUD ;
   au minimum, rendre `Hovered` légèrement plus clair et `Pressed` plus sombre.
6. Appliquer les mêmes réglages à `Button_OffHand`.
7. Ne créer aucun événement `OnClicked` dans l’Event Graph.
8. Ne créer aucun binding Blueprint.
9. Compiler et sauvegarder le Widget Blueprint.

Les noms `Button_MainHand` et `Button_OffHand` sont obligatoires. Les clics
sont liés nativement par la classe C++.

## Tests automatisés

Filtre :

```text
Grimrock.Monsters.MON12
```

MON12.2 ajoute :

| Test | Vérifications |
| --- | --- |
| `CombatActionPanel.SlotAttackRouting` | les deux armes sont exposées, le clic `OffHand` conserve le slot, l’objet et l’attaque de la main secondaire, puis dépense 2 PA |
| `CombatActionPanel.SlotAttackRejection` | la torche est non offensive, le refus ne consomme pas l’action, puis une main vide déclenche l’attaque à mains nues |

Les tests MON12.1 `LiveData` et `TurnAuthority` restent également exécutés.

## Validation PIE détaillée

### Préparation

1. Compiler le projet en **Development Editor / Win64**.
2. Mettre à jour le Widget Blueprint avec les deux boutons ci-dessus.
3. Ouvrir le niveau de validation MON11/MON12.1.
4. Équiper au moins trois shurikens offensifs en `MainHand`.
5. Équiper la torche en `OffHand`.
6. Placer le Rat géant dans l’axe du groupe et lancer PIE.
7. Déclencher le combat et attendre `PlayerPhase`.

### Attaque MainHand

1. Vérifier `Active`, `PA 4 / 4` et l’icône du shuriken.
2. Survoler `Button_MainHand` et vérifier son état `Hovered`.
3. Cliquer une seule fois sur `MainHand`.
4. Vérifier que le Rat reçoit exactement une attaque.
5. Vérifier que le shuriken est visible en vol, tombe au sol et reste
   récupérable.
6. Vérifier que la quantité équipée diminue exactement d’une unité.
7. Vérifier `PA 2 / 4` et le maintien de l'état `Active`.
8. Cliquer de nouveau : une seconde attaque est acceptée, puis le panneau
   passe à `Completed` avec `PA 0 / 4`.
9. Un troisième clic ne doit produire aucun projectile, dégât ou décrément.

### Objet OffHand non offensif

1. Attendre la manche suivante afin de retrouver `Active` et `PA 4 / 4`.
2. Vérifier que la torche reste visible et lumineuse.
3. Vérifier que `Button_OffHand` est désactivé.
4. Cliquer sur la zone OffHand : aucune attaque ne doit partir.
5. Vérifier que le personnage reste `Active` à `PA 4 / 4`.
6. Vérifier que `MainHand` demeure cliquable.

### Arme OffHand

1. Hors PIE, remplacer temporairement la torche par une arme dont la
   définition autorise `OffHand` et fournit un profil offensif valide.
2. Relancer PIE et revenir à `PlayerPhase`.
3. Cliquer `OffHand`.
4. Vérifier dans le log `[GridPlayerAttack] Accepted=true` que `Slot=OffHand`
   et que `Item` correspond à cette arme.
5. Vérifier que 2 PA sont consommés.

### Main vide

1. Hors PIE, vider temporairement `OffHand`.
2. Relancer PIE et revenir à `PlayerPhase`.
3. Cliquer la zone vide `OffHand`.
4. Vérifier dans le log :
   - `Attack=Attack_Unarmed` ;
   - `Item=None` ;
   - `Slot=None`.
5. Vérifier que l’attaque est résolue une seule fois et dépense 2 PA.

### Non-régressions

1. Vérifier que `NumPad 7` déclenche toujours l’attaque automatique MON11.
2. Vérifier que la torche équipée reste lumineuse.
3. Vérifier qu’aucune arme non lumineuse ne reste devant la caméra.
4. Vérifier le cycle complet du shuriken : équipement, vol, sol,
   récupération.
5. Vérifier qu’il ne revient pas comme un boomerang.
6. Vérifier que les collisions ne produisent aucun dégât supplémentaire.
7. Vérifier qu’un refus ne consomme ni action ni objet.

## Hors périmètre historique

- tours individuels : MON12.4 ; les PA des personnages sont implémentés en
  MON12.3 ;
- initiative globale : désormais MON12.4 ;
- déplacement payant du groupe : désormais MON12.5 ;
- catalogue d'actions : désormais MON12.6 ;
- quatre panneaux simultanés : désormais MON12.7 ;
- sélection d’une cible dans le HUD ;
- panneau et lancement des sorts : désormais MON12.8 ;
- capacités, défense, attente et objets consommables ;
- calcul de dégâts ou mutation d’inventaire dans le widget ;
- modification automatique d’un asset Unreal binaire.
