# ARM-CLEAN06 — Point de reprise fiable (15 novembre 2026)

> **Document de continuité, pas une autorisation de conversion.**
> Lors de toute reprise, **relire le HEAD réel de `origin/master` et ses statuts**
> avant d'utiliser ce point daté. Si le dépôt a avancé, ne jamais revenir au
> jalon présenté ici. Une conversation ChatGPT peut s'interrompre sans préavis.

## Dernier jalon fonctionnel confirmé

- Projet : `GrimrockPrototype`, Unreal Engine **5.5.4**.
- Dépôt : `PhilippeMartin1964/GrimrockPrototype`, branche **`master` uniquement**.
- Dernier jalon de code examiné : **ARM-CLEAN06.3BA**,
  commit `3c537bc7fd43815bf0d090545f739ae904b6f9f7`.
- Statuts publiés et vérifiés sur **ce SHA** :
  `UE5.5.4/SilentTask=success` et `UE5.5.4/LocalAuto=success`.
  Ces statuts attestent l'automatisation Windows, la compilation et la suite
  `Grimrock`, **pas** le comportement GPU/PIE des WBP.
- Aucun `.uasset`, `.umap`, SaveGame ni table de localisation n'a été
  converti par BA. Les étapes **06.4–06.6 restent ouvertes**.

### Jalons récents déjà réalisés — ne pas les refaire

| Jalon | Portée | Commit |
|---|---|---|
| 06.3AV | Vols ARM spécialisés canoniques | `bf444f0` |
| 06.3AW | Interactions, lecture, raccourcis | `96e02fa` |
| 06.3AX | `Use / Self / Heal` | `4f78087` |
| 06.3AY | `Use / Self / RemoveStatus` | `b12ad68` |
| 06.3AZ | `Use / Self / ApplyStatus` | `d486a2e` |
| 06.3BA | `Use / Ally`, clic portrait, transaction ciblée | `3c537bc` |

Documents de détail :
- `docs/Architecture/ARM_CLEAN06_3_REMAINING_LEGACY_READERS_AUDIT.md`
- `docs/Architecture/ARM_CLEAN06_3BA_CANONICAL_ALLY_USE_TARGETING.md`
- `docs/Architecture/ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md`
- `docs/Architecture/ARM_CLEAN06_MIGRATION_PLAN.md`

## Avancement 06.3BB — munitions canoniques

Le jalon 06.3BB ajoute la résolution unique des exigences de munitions,
les préflights partagés HUD/exécution et la porte de spawn canonique
des flèches N0. Lire `ARM_CLEAN06_3BB_CANONICAL_RANGED_AMMUNITION.md`
et vérifier les deux statuts de validation **sur le SHA fonctionnel BB**
avant tout verdict. Les paramètres visuels des flèches conservés en
transition et la recette PIE restent hors du cutover 06.4.

## Validation fonctionnelle encore nécessaire pour BA

L'Automation vérifie les projections d'actions `Heal`,
`RemoveStatus`, `ApplyStatus`, leur sélection `Ally` et leurs
préflights. **PIE du WBP réel non vérifié.** Recette visuelle ciblée :

1. Dans un combat, lanceur à PV maximum, allié blessé ; sélectionner
   le raccourci `Use / Ally / Heal` sans consommer l'objet.
2. Cliquer sur le **portrait de l'allié**. Vérifier priorité du clic UMG
   sur le clic monde, cible correcte, soin et **une seule** unité consommée.
3. Répéter pour `RemoveStatus` et `ApplyStatus`, contrôler
   résultat, refus d'effet inutile, cooldown et son.
4. Vérifier Échap, fin de tour, cible morte et inventaire ouvert :
   pas d'exécution parasite, pas de dépense lors d'un refus.

Ne pas présenter une simulation `-NullRHI` comme une preuve du
hit testing du `UGridCombatActionPanelWidget` dans son WBP binaire.

## Prochain périmètre de travail proposé — **06.3BB, non commencé**

**Migration canonique des exigences de munitions pour les attaques à
distance**, avec tests de non-régression avant toute modification binaire.

Motif établi par lecture du HEAD `3c537bc` :
- `GridTurnManagerPlayerActionCatalog.cpp` appelle encore
  `FGridRangedAmmunitionPolicy::HasRequiredAmmunitionForAttack`
  sur `UGridItemDefinitionAsset`, même pour des attaques d'armes
  déjà enregistrées dans `DA_ItemSystem` si l'ancien asset existe.
- `GridTurnManagerPlayerActions.cpp` utilise encore
  `RequiredAmmunitionItemDefinitionId` de cet ancien asset pour le
  préflight, la préparation visuelle d'une flèche et sa consommation.
- Le modèle normatif prévoit **`RequiredAmmoKindId` côté arme et
  `AmmoKindId` côté munition**, la jointure et l'identité réelle de
  `Item_Arrows_01`, sans copie de la liaison dans `DA_Weapon_*`.
- La disparition d'une ancienne définition ne doit ni autoriser
  gratuitement un arc canonique ni empêcher un tir pourtant authoré.
  Les effets secondaires visuels du projectile doivent rester intacts.

**Contrat de sortie BB à définir et tester :** source canonique
prioritaire sur toute métadonnée legacy ; identité de munition
déterministe, un seul débit après attaque acceptée ; zéro débit sur
attaque refusée ; comportement des armes inconnues inchangé ;
intégrité des projectiles au monde, de l'inventaire et du SaveGame.
Ne pas inventer de munition, de projectile ou de politique de tir.
Ne pas supprimer l'ancienne définition utilisée pour fabriquer le
projectile visuel avant une solution UE de conversion (06.4).

Ce périmètre est **une proposition issue de l'audit**, non une
implémentation achevée, ni un jalon validé.

## Étapes ultérieures inchangées

1. Terminer l'audit des autres lecteurs 06.3 (jets génériques,
   monde/réceptacles, outils d'authoring, catégories restantes).
2. **06.4 :** conversion contrôlée des contenus binaires via UE Editor,
   y compris les cas SaveGame ; pas d'écriture artificielle des `.uasset`.
3. **06.5 :** retrait des anciens champs, chemins et outils après
   preuve de migration et contrôle des références AssetRegistry.
4. **06.6 :** régression complète UE, PIE transversal, Save/Continue,
   puis vérification Cook/Shipping.

## Règles permanentes de reprise

- Lire `AGENTS.md` avant toute modification.
- Une étape fonctionnelle = **un seul commit final maximum**, sur
  `master`, push automatique après vérification, jamais un commit
  par correction ou fichier.
- Après push C++ : consulter `UE5.5.4/SilentTask` **et**
  `UE5.5.4/LocalAuto` du **nouveau** SHA exact ; suivre les échecs
  Windows automatiquement, sans imposer `ValidateUE.ps1` à l'utilisateur.
- C++ = comportement ; WBP Designer = présentation. Si le jalon
  touche au texte visible, appliquer
  `docs/UI_WBP_LOCALIZATION_WORKFLOW.md` et contrôler
  `UI.csv → ST_UI.uasset → WBP → Game/fr/en`.
- Ne pas effacer de modifications distantes imprévues. Ne pas
  conclure `zéro legacy` avant 06.5 et la recette 06.6.
- Lors d'une reprise avec deux conversations de titre similaire,
  identifier **la plus récente** puis recouper impérativement le
  dernier compte rendu avec `master` et ce document.
