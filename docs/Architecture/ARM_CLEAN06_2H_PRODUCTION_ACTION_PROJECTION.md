# ARM-CLEAN06.2H — Projection des attaques ARM standard sans perte

## Périmètre

Seuls les 22 `ItemPath` du manifeste de production sont examinés.
Les 82 progressions restent la source numérique ARM N1–N20 ; les trois
passifs Shield/Focus ne reçoivent jamais d'attaque fictive.

## Modèle de mécanique élargi, pas de copies N1

L'action canonique expose désormais `bRangedAttack`,
`bRequiresLineOfSight` et `SourceItemQuantityCost`.
Elle conserve déjà le ciblage, le coût spécifique en mana, le cooldown,
le `ActionId` et les effets typés. Le coût PA normal, la portée normale
et les dégâts sont toujours détenus par le `WeaponCatalogue`.
Une consommation par lancer est distincte d'une munition
(`AmmoKindId`).

## Preuve de projection

Le convertisseur Editor-only
`GridArmClean06ActionProjection::TryProjectGeneratedAttack`
reconstitue exactement l'attaque du générateur
`GridWeaponPipelineCommandlet` (N1 + `BuildKind` + `AttackId` +
icône de l'item). Le `UScriptStruct` natif compare **toutes**
les propriétés de `FGridCombatActionDefinition`, pas une liste
de champs supposés suffisante. La copie canonique est produite
**uniquement** si l'attaque entière correspond.

Si une modification locale touche les besoins, les variantes
de propriétaire, la présentation, une réaction, la quantité,
les effets, des cibles ou un quelconque champ non représenté,
la projection est rejetée comme `NonStandardAction` :
l'arme historique est préservée, pas de fallback ni d'écrasement.
C'est le pré-requis sûr pour convertir les personnalisations
ultérieurement en payloads typés, sans inventer d'effets.

Le test `ProductionGeneratedActionProjection` charge uniquement
les 22 vrais DA_Weapon de production, refuse les identités ou
actions manquantes, compte les attaques standard complètement
prouvées, les actions personnalisées laissées intactes, les
passifs, attaques distantes et coûts de consommation ; aucune
donnée confidentielle de package n'est publiée. Le test
`RejectModifiedActionProjection` vérifie qu'un cooldown ou une
exigence supplémentaire force le rejet, sans laisser de résultat
partiel. Aucun asset n'est écrit.

Une correspondance complète permettra l'authoring de ces
actions de base ; chaque `NonStandardAction` devra obtenir une
projection spécialisée avant toute migration. Le succès des
tests ne vaut pas permission de bascule 06.3–06.5.
Validation requise sur le SHA exact : `SilentTask` et `LocalAuto`
compilation + `Grimrock`.
