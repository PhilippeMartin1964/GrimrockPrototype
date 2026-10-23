# ARMEMENT01.10.3.3 — Combat Tooltip structuré : source, absorption et conséquences

**Date : 23 octobre 2026**  
**Statut : C++ préparé et publié ; compilation UE5.5.4, tests automatiques et validation UMG/PIE à effectuer.**  
**Cible :** combat dans `L_Dungeon`, écran Flux d'événements (`WBP_GridEventFeed`), événement survolé.

## Décision

Conserver les événements compacts et déplacer dans **une infobulle dédiée** les calculs et l'historique de la protection. Le widget de navigation partagé (`WBP_GridNavigationTooltip`) n'est ni remplacé ni modifié.

L'infobulle comporte trois sections à colonnes alignées :

1. **Attaque** : attaquant, défenseur, jet, défense et dégâts issus de la trace C++.
2. **Absorption** : source | avant | absorbé | après ; autant de lignes que de sources réellement présentes, dans l'ordre de consommation. Si la source passe d'une valeur positive à 0, signaler **Protection épuisée !** sur cet impact uniquement ; les attaques suivantes montreront 0→0 sans répéter une transition inexistante.
3. **Conséquences** : PV avant/après, perte effective et résidu de protection.

Le numéro de niveau, la quantité initiale et les bonus proviennent du catalogue/ItemDefinition existant. **Aucun jet de blocage ni +Défense automatique.**

## Autorité des données et C++

- `FGridArmorAbsorptionSourceTrace` (dans `GridCombatTraceTypes.h`) décrit une source : `ItemDefinitionId`, `DisplayName`, `bEquipment`, `Maximum`, `Before`, `Absorbed`, `After`, `bExhaustedByHit`.
- `ApplyPhysicalArmorDamage` accepte un paramètre de sortie optionnel `OutSources`. La liste est construite **pendant l'unique consommation autoritaire**, y compris pour un bouclier déjà à zéro ; la réserve de base du personnage figure comme source distincte si elle est positive.
- `GridMonsterCombatComponent` transmet cette liste à `FGridAttackResult::Trace.Damage.PhysicalArmorSources`.
- `GridCombatEventFeedFormatter` copie la trace dans l'entrée du flux `FGridHudEventEntry` et ajoute une ligne lisible dans le tooltip texte de repli.
- `UGridCombatTooltipWidget` est un widget **présentation seulement** ; il n'ouvre pas le catalogue et ne relit jamais l'état actuel d'un équipement. Un changement d'arme ou de bouclier après combat ne modifie pas l'événement passé.
- `UGridEventFeedWidget::CombatTooltipWidgetClass` sélectionne le nouveau widget pour les entrées structurées. Sinon, il conserve automatiquement l'ancien tooltip partagé.
- **Couverture actuelle :** snapshots détaillés des attaques de monstres contre un personnage ; les autres types d'événements gardent leurs informations standard et ne fabriquent pas de ventilation d'armure.

## Authoring manuel UMG dans Unreal Editor

Ne pas modifier directement un `.uasset` hors UE. Les étapes suivantes sont nécessaires pour activer la représentation tabulaire ; le code seul conserve le tooltip texte de repli.

1. Ouvrir UE5.5.4 après validation du code. Créer un **Widget Blueprint**, parent **`GridCombatTooltipWidget`**, nommé `WBP_GridCombatTooltip`. Le placer sous le dossier UI du projet, à proximité de `WBP_GridEventFeed`.
2. Construire un `Border` racine inspiré du thème brun/noir/or existant, largeur cible environ 430–520 unités logiques, padding 10–14, sans animation et sans interaction souris propre.
3. Ajouter `VerticalBox` racine, `TextBlock` titre « Détails du combat », puis trois sections séparées « Attaque », « Absorption » et « Conséquences ».
4. **BindWidgetOptional à respecter exactement** : `Text_Attacker`, `Text_Defender`, `Text_Attack`, `Text_Defense`, `Text_Damage`, `VerticalBox_ArmorRows`, `Text_Health`, `Text_Status`. Tous en **Is Variable**, du type indiqué ; les libellés de section et l'entête du tableau sont authored UMG.
5. Pour « Absorption », placer un `HorizontalBox` d'entête avec quatre colonnes « Source », « Avant », « Absorbé », « Après ». Le `VerticalBox_ArmorRows` sous cet entête est alimenté automatiquement depuis les snapshots C++ ; ne pas créer de lignes codées en dur.
6. Dans `WBP_GridEventFeed`, sélectionner le widget racine, puis régler **Class Defaults → Combat Tooltip Widget Class → `WBP_GridCombatTooltip`**. Enregistrer, compiler, contrôler au survol d'une ligne de combat. Pour revenir à l'ancien tooltip, remettre ce champ à None.
7. Tester le positionnement près des bords d'écran, le scroll du journal, le filtre Combat, les polices `Alegreya Sans`, la largeur et les effets d'une autre infobulle affichée simultanément.

La classe UMG porte les couleurs et la police du tableau dans les Class Defaults. Les lignes générées sont purement visuelles et n'altèrent pas les données.

## Recette et résultats attendus

Exemple ARM075 N1, 5 points de protection, pas d'armure de base :

| Coup | Source | Avant | Absorbé | Après | PV |
|---|---|---:|---:|---:|---|
| 4 dégâts | Bouclier rond | 5 | 4 | 1 | inchangés |
| 5 dégâts | Bouclier rond | 1 | 1 | 0 | -4 ; protection épuisée |
| 3 dégâts | Bouclier rond | 0 | 0 | 0 | -3 ; déjà épuisée |

Avec deux points d'armure supplémentaires du personnage, un coup à 6 absorbe 5 sur le bouclier et 1 sur la réserve de personnage. Le tableau expose les **deux sources**, sans addition erronée ni régénération.

## Validation locale exigée

~~~powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.10.3.3"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.10"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.EventHUD01"
~~~

**Attendu (si compilable) :** 3 nouveaux tests ; 17 tests sous `ARMEMENT01.10` si la suite conserve les 14 déjà réussis. Le filtre UI vérifie que l'ancien comportement compact et les tooltips de repli restent intacts. Ne pas annoncer PASS avant logs utilisateur.

**Recette PIE :** bouclier rond équipé, observer les trois attaques ; survoler chacun des trois événements après le troisième coup, confirmer que les valeurs historiques **5→1, 1→0, 0→0** ne changent pas. Vérifier le texte d'épuisement une seule fois sur le deuxième événement, puis les dégâts réels aux PV. Enfin, déplacer/changer le bouclier et vérifier que les anciens tooltips ne changent pas.

Le tableau et la mise en page demandent une **validation visuelle par capture** ; aucun test C++ ne certifie à lui seul le résultat UMG.
