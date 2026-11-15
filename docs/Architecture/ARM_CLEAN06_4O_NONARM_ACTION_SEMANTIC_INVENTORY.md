# ARM-CLEAN06.4O — Inventaire sémantique des actions d'objets non-ARM

## État de départ confirmé

- **06.4N** : `99bd3afc3207830c76542078eaba417dcbbc9b63`, trois checks Windows SUCCESS, rapport [147](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/147).
- **06.4K** : les vraies sources UE exposent **11 propriétés actives `ITEM_ACTION`**, sur 11 identités d'objets non-armement, 40 références de niveau/palette/réceptacle.
- **06.4G** : les 84 fichiers binaires et les 84 copies de sauvegarde privées sont immuables et revérifiés SHA-256.

Il serait dangereux de convertir l'ancien `FGridCombatActionDefinition` en un seul `ActionId` ou un simple `FGridCanonicalActionEffect` : la source comporte des propriétés de ciblage, coûts PA et ressources, résolution, ciblage multiple, effets/états, interactions, surfaces, pièges, modificateurs et présentations qui peuvent porter des règles de gameplay réellement authorées.

## Objet précis du jalon

`ARM-CLEAN06.4O` n'authorise **aucune nouvelle action** et ne modifie **aucune définition canonique**. C'est une étape d'inventaire exact, pour décider des destinations d'autorité sans perdre un seul effet. Elle conserve seulement en privé les détails :

- Les six anciens champs du domaine `ITEM_ACTION` : `CombatActions`, `bProvidesQuickItemCombatAction`, `QuickItemCombatAction`, `QuickItemActionIdOverride`, `bProvidesAttackPresentation`, `PlayerAttackPresentationProfile`.
- Pour chaque entrée `CombatActions` : son `ActionId`, sa validité `IsValid()`, le **`ExportText` de toutes ses `FProperty`**, et les noms des champs qui diffèrent des valeurs par défaut de `FGridCombatActionDefinition`.
- Pour chaque QuickItem activée : l'action effectivement normalisée par `BuildQuickItemCombatActionDefinition`, avec ses coûts d'objet, tags et action ID ; si la normalisation échoue, conserver séparément le payload exact sans l'inventer.
- Pour chaque présentation : le payload de `PlayerAttackPresentationProfile` et la validité de la source réelle.
- Les collisions des actions réellement authorées avec les identifiants déjà présents dans le vrai `DA_ActionCatalogue`.

L'existence d'un champ de même nom dans les deux schémas **n'est jamais** considérée comme une preuve d'équivalence. La liste complète des valeurs en Windows doit servir au futur mapping de l'ActionCatalogue, du `ItemActionBinding`, du catalogue de présentation et du véritable exécuteur. Les champs QuickItem ne peuvent pas être réduits automatiquement à Damage/Heal/ApplyStatus/RemoveStatus.

## Commandlet et contraintes de validation

Le commit exact `chore(ARM-CLEAN06.4O): inventory full legacy nonarm action semantics without writes` active `Scripts/AuditARM_CLEAN06_4O_NonArmActionSemantics.ps1` après compilation UE et Automation `Grimrock`.

Le script :

1. sélectionne une seule preuve privée réelle de 06.4K, ou plusieurs captures **de même contenu sémantique strict** et munies de reçus SHA-256 indépendants ; refuse tout conflit ;
2. contrôle les portes 06.4K et 06.4N **fermées**, leurs SHA exacts et leurs compteurs ;
3. rehash les 84 binaires UE d'origine et leurs 84 copies du snapshot 06.4G ;
4. ouvre les 11 assets et le `DA_ItemSystem` réel, y compris le vrai `DA_ActionCatalogue`, et vérifie le contenu des **50** anciennes propriétés actives ;
5. enregistre en privé tous les détails de 11 propriétés `ITEM_ACTION` actives, les actions équipées, QuickItems normalisées ou invalides, les présentations et les éventuels conflits d'IDs ;
6. garantit l'absence de `Dirty` modifié, de `SavePackage`, de lecture/écriture des SaveGames et de modification du worktree ;
7. produit `action-semantics-private.json`, `action-audit-gate-private.json` et un ticket GitHub à **compteurs agrégés exclusivement**.

Les trois statuts nécessaires sont : `UE5.5.4/SilentTask`, `UE5.5.4/LocalAuto`, `UE5.5.4/ARM-CLEAN06.4O`. Vérifier les trois sur le **SHA du commit 06.4O** avant toute déclaration de succès.

## Ce qu'on pourra décider ensuite

Selon les **valeurs réellement observées** et sans supposer que le schéma cible suffit : construire la correspondance exacte de chaque champ non défaut vers `DA_ActionCatalogue`, `DA_ActionPresentationCatalogue`, les liaisons `ItemActionBinding`, les spécialisations d'objet ou les politiques non-ARM. Pour chaque action :

- préserver le ciblage, les coûts, la portée, les tags, les effets secondaires, les états et les règles de surfaces/puzzles ;
- prouver l'exécution et les refus avant consommation/PA ;
- vérifier `Self`/`Ally`, combats, exploration, cooldown, hotbar et la persistance des objets ;
- ne jamais dupliquer des profils de dégâts N1 d'armes ;
- ne pas utiliser le fallback legacy comme solution finale.

Le domaine des **11 champs actions** demeure *non migré* après 06.4O. Restent également 15 champs équipement non-ARM et 6 champs d'empilement. La phase binaire 06.4 et la suppression 06.5 restent strictement bloquées.

**Aucun changement de gameplay, WBP, textes `ST_UI`, `.uasset`, `.umap` ou `.sav`.** La validation `-NullRHI` ne remplace ni PIE ni tests Chaos/Save-Continue.
