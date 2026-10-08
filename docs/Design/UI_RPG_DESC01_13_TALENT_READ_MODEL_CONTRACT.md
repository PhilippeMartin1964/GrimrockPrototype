# UI-RPG-DESC01.13 — Contrat du read-model Talent

Date : **8 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
État : **VALIDÉ PAR L'UTILISATEUR — 8 octobre 2026**  
Dépendances : **UI-RPG-DESC01.5 à .12 validés**  
Périmètre : **contrat uniquement — aucun C++, UMG ou DataAsset modifié**

## 1. Objet

DESC01.13 définit exactement ce que le C++ de présentation devra fournir à WBP_RPGTalentDetail après la normalisation 90/90.

Le but est de supprimer définitivement les ambiguïtés apparues dans DESC01.4 : focus de consultation confondu avec acquisition, TYPE inféré sous forme de texte libre, action présentée comme disponible parce que le Talent est acquis, mécaniques aplaties dans un gros FText, variantes utilisées comme outil d'inspection, humanisation tardive d'identifiants techniques et confusion tour / round.

DESC01.13 ne change aucune règle gameplay.

## 2. Autorités

~~~text
URPGClassAsset::ProgressionChoices
    -> identité / branche / nœud / niveau / coût / prérequis
    -> modifiers / reactions / skill modifiers / party modifiers

URPGClassAsset::CombatActions
UGridItemDefinitionAsset::QuickItemCombatAction
UGridStatusEffectDefinitionAsset
URPGSkillAsset
bestiaire / catégories
    -> mécanique et libellés autoritaires

FRPGClassProgressionService
FRPGClassProgressionTransactionService
    -> disponibilité / acquisition

URPGTalentPresentationAsset
    -> ordre des branches / couleurs / icônes / override conceptuel des nœuds groupés

FGridSkillsPageService
    -> UNIQUE builder du read-model

WBP / Blueprint
    -> rendu uniquement
~~~

Aucun Blueprint ne recalcule un coût, un statut, une portée, un prérequis ou un TYPE.

## 3. Décision structurante — TYPE explicite

Le TYPE normalisé 90/90 n'est pas toujours déductible proprement des primitives gameplay existantes, surtout pour l'Alchimiste. Exemple : Bombe incendiaire = RECETTE + OBJET RAPIDE, alors que Transmutation majeure = RECETTE + ACTIF.

Une heuristique basée sur le nom d'un Id, le préfixe Recipe_ ou la présence d'un modifier serait fragile. DESC01.13 autorise donc **une seule nouvelle métadonnée sémantique authorée** :

~~~text
ERPGTalentPresentationType
    Active
    ActiveSpell
    Passive
    AutomaticReaction
    RecipeQuickItem
    RecipeActive
~~~

Cette valeur devra être portée par chaque FRPGClassProgressionChoiceDefinition.

Règles :

1. elle décrit uniquement la nature UX du Talent ;
2. elle ne modifie aucune mécanique ;
3. toutes les Choice d'un même TalentNodeId doivent avoir le même TYPE ;
4. les six authoring C++ restent l'unique source de cette valeur ;
5. aucun switch(TalentId) n'est autorisé dans le service UI ;
6. un test Automation devra vérifier la matrice TYPE 90/90 de DESC01.12.

C'est la seule duplication sémantique volontaire introduite par le contrat.

## 4. Description = PRINCIPE

Le champ existant FRPGClassProgressionChoiceDefinition::Description devient la source textuelle du PRINCIPE.

Il ne doit plus servir de conteneur de secours pour recopier toutes les valeurs numériques déjà présentes dans les données mécaniques.

Règles de DESC01.14 : conserver le sens validé par les audits 90/90, retirer les identifiants techniques, éviter de répéter les chiffres déjà projetés dans EFFETS / UTILISATION, utiliser l'override conceptuel de URPGTalentPresentationAsset pour le PRINCIPE commun d'un nœud groupé, et conserver la Description de chaque Choice pour le détail propre d'une variante lorsque nécessaire.

Aucune nouvelle table parallèle de 90 descriptions n'est créée.

## 5. Forme générale du read-model

La projection Talent reste dans le FGridSkillsPageView existant.

~~~text
FGridSkillsPageView
└── TalentTree : FGridTalentTreeView
    └── Branches[]
        └── Nodes[]
            ├── identité / tier
            ├── DisplayName
            ├── Type
            ├── State / StatusText
            ├── Principle
            ├── Effects[]
            ├── Usage[]
            ├── Acquisition
            └── Variants[]
~~~

Il n'est pas créé de second service, second arbre Talent ou snapshot durable.

## 6. Ligne de détail générique

EFFETS et UTILISATION sont des tableaux de lignes déjà prêtes à rendre :

~~~text
FGridTalentDetailLineView
    Label : FText
    Value : FText
~~~

Exemples :

~~~text
Dégâts      | 150 % des dégâts de l'arme
Précision   | +2
Durée       | 2 rounds
Coût        | 2 points d'action
Mana        | 6
Cible       | soi-même ou un allié vivant
Portée      | 3 cases
Recharge    | 2 rounds
~~~

Ces lignes sont des valeurs de présentation dérivées, pas une copie gameplay. Le Widget n'interprète jamais leur contenu.

## 7. Nœud conceptuel

Contrat cible :

~~~text
FGridTalentNodeView
    TalentNodeId
    TalentBranchId
    Tier
    DisplayName
    Type : ERPGTalentPresentationType
    State : EGridTalentNodeState
    StatusText
    Principle
    Effects[]
    Usage[]
    Acquisition : FGridTalentAcquisitionView
    SimpleChoiceId
    bCanAcquireSimple
    Variants[]
~~~

Pour un nœud simple, DisplayName vient de ProgressionChoice.DisplayName. Pour un nœud groupé, il vient de l'override conceptuel de URPGTalentPresentationAsset. ChoiceId et TalentNodeId ne sont jamais affichés directement.

State conserve l'enum existant : Acquired, Available, LockedLevel, LockedPrerequisite, LockedPoints, LockedExclusive. StatusText est dérivé en C++ par le service : ACQUIS, DISPONIBLE, VERROUILLÉ — niveau X requis, VERROUILLÉ — nécessite « Talent X », VERROUILLÉ — nécessite N point(s) de Talent, INDISPONIBLE — autre variante déjà choisie.

Le Widget ne formate pas lui-même ces raisons.

## 8. Acquisition structurée

~~~text
FGridTalentAcquisitionView
    MinimumLevel
    PointCost
    PrerequisiteTalentNames[]
    ExclusivityText
    GrantedRecipeNames[]
~~~

Le read-model conserve aussi les identités internes nécessaires aux commandes d'acquisition, mais elles ne sont jamais rendues comme texte.

Les prérequis viennent des vraies autorités : PrerequisiteChoiceIds, PrerequisiteRequirementIds et alias conceptuels accordés par les variantes. PreviousNodeDisplayName peut rester temporairement pour compatibilité, mais ne doit plus être l'autorité de la section ACQUISITION.

Les Recipe_* peuvent rester comme identités internes. Il est interdit de fabriquer un nom joueur par HumanizeId(Recipe_*). Tant qu'un catalogue Crafting autoritaire n'existe pas, une recette sans autorité de DisplayName n'est pas inventée par l'UI.

## 9. Variantes

Pour les quatre nœuds à variantes seulement :

~~~text
FGridTalentVariantView
    ChoiceId
    DisplayName
    State
    StatusText
    bAcquired
    bCanChoose
    Principle
    Effects[]
    Usage[]
~~~

Le tableau Variants est vide pour les 86 nœuds simples.

Règles : toutes les variantes sont projetées simultanément ; consultation et acquisition restent séparées ; bCanChoose vient du service de progression ; CONFIRMER envoie uniquement le ChoiceId déjà projeté ; aucune ComboBox n'appartient au contrat final ; après acquisition, les autres variantes restent visibles avec INDISPONIBLE — autre variante déjà choisie.

## 10. EFFETS — projection structurée

EFFETS est construit depuis les vraies primitives, jamais depuis une analyse de texte de Description.

Sources possibles : CombatModifiers, CombatReactions, SkillModifiers, PartyModifiers, FirstRoundInitiativeModifier, CombatAction.OffensiveProfile, DirectDamageScaling, StatusApplications, StatusRemovals, ArmorEffects, SurfaceEffects, SurfaceConversions, MovementEffects, TargetFilter, OwnerVariants, QuickItemScaling et les définitions de Status Effects.

Le service résout les DisplayName des Skills, Status, Actions, Items et catégories lorsque leur autorité existe.

ArmorGate doit produire une formulation du type « Si l'armure physique est épuisée après les dégâts : ... ». Une RÉACTION AUTOMATIQUE produit explicitement Déclencheur / Fréquence / Effet / Exclusions. Les surfaces produisent Surface / Durée / Dégâts périodiques / Coût de traversée / Conversion.

## 11. UTILISATION — projection structurée

UTILISATION n'existe que lorsqu'il y a activation volontaire. Ordre canonique :

~~~text
Coût
Mana
PAM
Objet consommé
Cible
Portée
Zone
Ligne de vue
Résolutions
Recharge
Condition
~~~

Les lignes absentes sont omises.

Le read-model Talent ne contient pas bActionAvailable, ActionStatusText, « Action disponible » ou « Action accordée après acquisition ». La disponibilité instantanée reste dans le catalogue combat et ses EGridCombatActionAvailabilityReason.

## 12. Tour / round

Le service lit l'unité autoritaire EGridStatusEffectDurationUnit::Turns, EGridStatusEffectDurationUnit::Rounds, CooldownRounds et les durées de surface.

Il formate 1 tour / 2 tours et 1 round / 2 rounds sans remplacement lexical global. Le terme technique « tick » n'est pas généré par le read-model.

## 13. Ciblage

Le service possède un mapper générique de EGridCombatTargetingPolicy vers une formulation joueur et tient compte des filtres supplémentaires de l'action : TargetFilter, catégories, statuts requis, armures et vitaux.

Exemple : TargetingPolicy = Ally devient « soi-même ou un allié vivant » lorsque rien n'exclut le lanceur.

Le Widget n'effectue aucun mapping d'enum.

## 14. Friendly fire

bAffectsAlliesInArea doit produire une ligne EFFETS explicite lorsque vrai et tenir compte des modificateurs personnels/alliés déjà authorés. La fiche ne transforme jamais Area en « tous les ennemis » si le groupe peut être touché.

Cas normalisés : Cataclysme, Bombe incendiaire et Bombe toxique.

## 15. Résolution des noms

Le service ne doit plus utiliser HumanizeId() ou MakePlayerReadableText() comme autorité de production.

~~~text
SkillId  -> URPGSkillAsset::DisplayName
EffectId -> UGridStatusEffectDefinitionAsset::DisplayName
Category -> autorité de présentation du bestiaire
ActionId -> FGridCombatActionDefinition::DisplayName
ItemId   -> UGridItemDefinitionAsset::DisplayName
~~~

Un fallback technique peut exister uniquement pour diagnostic non-production.

## 16. Relation avec URPGTalentPresentationAsset

Le catalogue de présentation reste strictement visuel / conceptuel : ordre des branches, nom et description courte de branche, couleurs, emblèmes, icônes et nom/description conceptuels des quatre nœuds groupés.

Il ne reçoit pas les dégâts, coûts, portées, cooldowns, statuts, prérequis gameplay, états d'acquisition ou TYPE 90/90.

## 17. Compatibilité et nettoyage

Le contrat autorise une migration, mais pas deux autorités finales. Les éléments actuels suivants sont à remplacer : FGridTalentUnlockedActionView centré sur « unlocked action », FGridTalentVariantView::EffectCategory sous forme de FText inféré, FGridTalentVariantView::MechanicsSummary aplati, ainsi que BuildMainDetailText(), BuildActionSummary() et MakePlayerReadableText() dans UGridTalentDetailWidget.

Le Widget final reçoit des sections déjà construites et se contente de les rendre.

La projection plate FGridTalentEntryView reste hors de ce ticket : sa suppression nécessite toujours l'audit de références Blueprint binaires prévu par l'architecture.

## 18. Flux final

~~~text
ProgressionChoices / Actions / Items / Status / Skills
        |
        v
FGridSkillsPageService
        |
        +--> TYPE authoré
        +--> STATUT
        +--> PRINCIPE
        +--> EFFETS[]
        +--> UTILISATION[]
        +--> ACQUISITION
        +--> VARIANTES[]
        |
        v
FGridTalentTreeView
        |
        v
UGridTalentDetailWidget
        |
        v
WBP_RPGTalentDetail
~~~

## 19. Tests exigés pour DESC01.14

Le futur jalon C++ devra au minimum vérifier : TYPE 90/90, 15 nœuds conceptuels par classe, 3 branches × 5 tiers, les 4 seuls nœuds à variantes, Variants vide pour un nœud simple, toutes les variantes visibles, STATUT simple et par variante, niveau/coût/prérequis, absence de texte « ACTION DISPONIBLE », PASSIF sans UTILISATION, RÉACTION avec déclencheur/fréquence, SORT ACTIF avec PA/mana/cible/portée/recharge, QuickItem avec objet consommé, ArmorGate, Status DisplayName + unité de durée, Skill DisplayName, friendly fire, recette multiple non traitée comme variante, absence d'identifiants techniques dans le texte joueur, absence de switch(ClassId) et switch(TalentId) pour construire les fiches.

Le filtre proposé reste Grimrock.UI.RPG.DESC01.

## 20. Frontière avec DESC01.15

DESC01.14 implémente uniquement le C++ du read-model.

DESC01.15 pourra ensuite reconstruire WBP_RPGTalentDetail sur ce contrat : sections fixes, ScrollBox global, variantes visibles, boutons CHOISIR par variante, CONFIRMER / ANNULER, aucun ComboBox final.

La charte graphique reste sous l'autorité du thread parallèle UI-RPG-VISUAL01.

## 21. Critères de validation DESC01.13

Le contrat a été validé par l'utilisateur le **8 octobre 2026**. Ont été approuvés :

1. FGridSkillsPageService comme unique builder ;
2. l'ajout unique de ERPGTalentPresentationType dans les Choice authorées ;
3. Description comme source de PRINCIPE ;
4. EFFETS / UTILISATION sous forme de lignes structurées ;
5. STATUT déjà résolu en C++ ;
6. acquisition et variantes sans ComboBox ;
7. aucun état instantané d'action dans la fiche Talent ;
8. résolution des DisplayName depuis leurs vraies autorités ;
9. suppression du formatage sémantique tardif dans UGridTalentDetailWidget ;
10. les tests 90/90 exigés pour DESC01.14.

Après validation : **UI-RPG-DESC01.14 — projection C++ du read-model Talent**.