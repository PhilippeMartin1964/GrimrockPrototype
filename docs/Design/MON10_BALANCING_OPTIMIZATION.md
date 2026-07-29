# MON10.5 — Équilibrage et optimisation

## 1. Objectif

MON10.5 clôt le socle monstres MON1 à MON10 sans changer ses règles de
gameplay. Le jalon ajoute une graine stable par rencontre, des métriques
runtime optionnelles, un aperçu d’équilibrage orienté données, des catégories
de logs explicites et une politique de fins de ligne.

Aucun asset `Content/` n’est créé ou modifié.

## 2. État final de MON10

Le système couvre désormais le journal de combat, l’Audio, les VFX, les
variations d’Idle et les diagnostics d’équilibrage/performance. Les couches de
présentation restent indépendantes des règles de combat et de leur générateur
aléatoire.

## 3. Graine de base

`UGridTurnManagerComponent::EncounterRandomSeed` est conservé, avec son nom,
son type et sa valeur par défaut `1337`. Il constitue la graine de base et
reste compatible avec les Blueprints existants.

## 4. Construction de la graine par rencontre

`FGridEncounterSeedBuilder::BuildEncounterSeed` mélange un salt MON10.5, la
graine de base, `CurrentDungeonLevelId` et les identités persistantes des
participants. Les GUID invalides sont ignorés, les doublons supprimés et les
GUID valides triés selon `EGuidFormats::Digits`.

L’ordre du tableau d’entrée ne change donc pas le résultat.
`ActiveEncounterRandomSeed` reçoit la valeur calculée avant le premier tirage,
puis initialise `CombatRandomStream`.

## 5. Identité stable des participants

Chaque participant fournit `ResolvePersistenceId()`. Une rencontre refusée ne
change ni `ActiveEncounterRandomSeed`, ni l’état de `CombatRandomStream`, ni le
journal structuré.

## 6. Reproductibilité après rechargement

À graine de base, niveau et ensemble de participants identiques, un combat
reconstruit produit la même graine et la même suite pseudo-aléatoire. Un
`AbortCombat` suivi d’un redémarrage de la même rencontre respecte cette
propriété. `ActiveEncounterRandomSeed` est transitoire et n’est pas sauvegardé.

## 7. Absence d’influence des RNG de présentation

Le builder n’utilise ni l’heure, ni `FMath::Rand`, ni un compteur runtime. Il
ne lit aucun générateur Audio, VFX ou Idle. Réciproquement, ces générateurs de
présentation ne lisent et ne consomment pas `CombatRandomStream`.

## 8. Métriques runtime

`FGridCombatRuntimeMetrics` peut compter :

- les démarrages acceptés et refusés ;
- les manches, tours, actions commencées/terminées et attaques résolues ;
- les candidats de perception examinés ;
- les pics de participants et d’actions planifiées ;
- les frames et secondes de Tick réellement actives ;
- les derniers et maximums de temps de perception et de planification.

Ces valeurs sont diagnostics uniquement et ne sont consultées par aucune
décision gameplay.

## 9. Activation des métriques

Activer `bCollectRuntimeMetrics` sur le TurnManager. Lorsque la propriété vaut
`false`, aucun compteur n’est entretenu et aucun appel à
`FPlatformTime::Seconds` n’est réalisé pour la perception ou la planification.

`ResetRuntimeMetrics` remet le snapshot à zéro. `GetRuntimeMetrics` permet sa
lecture Blueprint.

## 10. Rapport de métriques

`LogRuntimeMetrics` écrit exactement une ligne dans
`LogGridCombatPerformance`, préfixée par `[GridCombatPerformance]`. Elle
regroupe compteurs, pics, activité du Tick et temps en millisecondes.

Les durées sont des mesures d’observation ; aucun seuil arbitraire n’est codé.

## 11. Rapport d’équilibrage

`FGridMonsterBalanceAnalyzer` construit un
`FGridMonsterBalanceSnapshot` depuis un `UGridMonsterDefinitionAsset`.
`BuildBalanceSnapshot` expose cet aperçu à Blueprint et
`LogBalanceSnapshot` écrit une ligne dans `LogGridMonsterBalance`.

Le snapshot reprend identité, danger, vie, armures, initiative, précision,
esquive, PA, perception, attaques, dégâts bruts, coûts et expérience.

## 12. Limites de l’estimation des dégâts moyens

Pour chaque attaque :

```text
minimum brut = MinDamage + DamageBonus
maximum brut = MaxDamage + DamageBonus
```

`AverageBaseDamage` est la moyenne des milieux de ces plages. Il ne s’agit pas
d’un DPS : précision, critique, armure, résistances, vulnérabilités et
multiplicateurs situationnels sont volontairement exclus.

## 13. Réduction des logs

Les `LogTemp` du domaine Combat/Monsters ont été remplacés sans supprimer
l’information. `bLogPhaseChanges` vaut désormais `false` par défaut ; son
activation conserve le diagnostic de phase.

## 14. Catégories et verbosités

- `LogGridTurnManager` : orchestration et erreurs de séquencement ;
- `LogGridMonsterAI` : perception, agression et choix tactiques détaillés ;
- `LogGridMonsterMovement` : initialisation et mouvements ;
- `LogGridMonsterOccupancy` : état du registre ;
- `LogGridCombatPerformance` : rapport demandé de métriques ;
- `LogGridMonsterBalance` : rapport demandé d’équilibrage.

Les catégories historiques State, Death, Loot, Audio, VFX et IdleVariation
sont conservées. Les transitions et décisions ordinaires sont `Verbose`; les
rapports demandés sont `Log`; les configurations récupérables sont `Warning`;
les états impossibles ou fallbacks de timeout sont `Error`.

## 15. Politique de Tick

`AGridMonsterActor`, Behavior, Audio, VFX et IdleVariation restent sans Tick.
Le TurnManager active son Tick uniquement pendant le délai de début ou une
action active. Les métriques ne participent pas à `RefreshTickEnabled` et ne
peuvent donc pas maintenir le Tick actif.

## 16. Chargement différé

MON10.5 n’ajoute aucun chargement global. Les variantes Audio, VFX et Idle ne
sont toujours pas toutes chargées au `BeginPlay`. Aucun cache global,
multithreading, pooling ou système asynchrone supplémentaire n’est introduit.

## 17. Procédure de profilage PIE

Répéter le protocole avec 1, 4, 8, puis 16 Rats si la scène le permet :

1. activer `bCollectRuntimeMetrics` ;
2. appeler `ResetRuntimeMetrics` ;
3. déclencher une rencontre ;
4. jouer au moins trois manches ;
5. appeler `LogRuntimeMetrics` ;
6. relever candidats examinés, pic de participants et actions ;
7. relever frames actives et durée cumulée du Tick ;
8. relever les temps de perception et de planification ;
9. vérifier qu’un monstre inactif ne raisonne pas en continu ;
10. utiliser Unreal Insights comme validation complémentaire.

Aucun chiffre n’est consigné ici : aucun profilage PIE réel n’a été exécuté
pendant l’implémentation C++.

## 18. Procédure d’équilibrage du Rat géant

1. ouvrir `DA_MON_RatGiant` ;
2. appeler `LogBalanceSnapshot` ;
3. relever les valeurs actuelles ;
4. jouer au moins dix combats courts ;
5. relever les manches avant victoire ;
6. relever les dégâts moyens reçus par le groupe ;
7. relever la fréquence de réussite du Rat et de son repli ;
8. relever la durée moyenne de rencontre ;
9. modifier une seule famille de valeurs à la fois ;
10. répéter les tests et comparer.

Base historique non obligatoire du guide : `MaxHealth=8`, armures `0/0`,
`Initiative=12`, `Accuracy=+2`, `Evasion=+1`, `ActionPointsPerTurn=2`,
perception `5/3`, `ExperienceReward=10` et `Attack_Bite=1d4+1`.
`RetreatChance` doit être validé en PIE. Ces nombres ne sont imposés par
aucune règle C++.

## 19. Politique de fins de ligne

`.gitattributes` impose LF aux sources, configurations et Markdown, et CRLF
aux scripts Windows et solutions. Toutes les règles Git LFS existantes sont
conservées. MON10.5 ne lance aucune commande de renormalisation globale :
seuls les fichiers texte réellement modifiés par le jalon sont concernés.

## 20. Tests

Sept scénarios sous `Grimrock.Monsters.MON10.Optimization` couvrent :

- déterminisme et indépendance de l’ordre de la graine ;
- distinction et cycle de vie des rencontres ;
- snapshot de balance ;
- cycle de vie et neutralité gameplay des métriques ;
- invariants Tick, persistance et compatibilité.

Ils n’utilisent ni rendu, ni GPU, ni asset `Content/`, ni son réel et
n’imposent aucun seuil temporel.

## 21. Limites du jalon

MON10.5 n’ajoute ni nouvelle IA, ni difficulté dynamique, ni sauvegarde d’un
combat actif ou des métriques, ni pooling, ni multithreading, ni Mass Entity.
Il ne modifie pas les statistiques de `DA_MON_RatGiant` et ne crée aucun asset.

## 22. Critères de clôture MON10

MON10 est clos lorsque la compilation UE 5.5.4 réussit, que les 72 tests MON
et les 2 tests CC5 passent, qu’aucun `LogTemp` ciblé ne subsiste, que le commit
ne contient aucun asset et que le profilage/équilibrage manuel restant est
effectué séparément dans l’éditeur.
