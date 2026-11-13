# ARM-CLEAN06.2E — Recensement semantique natif des mains et familles

## Situation apres 06.2D

Les 82 archetypes et leurs 20 niveaux sont numeriquement compatibles
avec la cible (06.2C), les variantes audio et Niagara possedent un
modele sans perte (06.2B), et les donnees des six classes et 82 affinites
sont conservees sans reduction (06.2D).

Le catalogue ancien serialize Family et HandUsage comme chaines
informatives provenant du XLSX. Le comportement d'equipement reel
est determine par UGridItemDefinitionAsset::GetEffectiveHandUsage()
et CanEquipToSlot(). Ces autorites sont distinctes.

La baseline contient 24 definitions concretes et 82 archetypes ARM.
Les 24 definitions ne permettent pas a elles seules de determiner
les mains des 82 archetypes. Un profil passif sans degats ne
devient pas automatiquement un bouclier; un focus reste possible.

## Outil Unreal Editor en lecture seule

Le commandlet UGridArmClean06HandSemanticsCommandlet examine :

- les 82 enregistrements sources, leurs valeurs Family et HandUsage
  intactes ainsi que leur activite offensive ;
- les 24 vrais DA_Weapon_*, ItemDefinitionId, ItemType,
  WeaponArchetypeId, bUseWeaponCatalogue, usage de main effectif
  et permissions MainHand / OffHand ;
- la jointure de chaque definition avec son profil ARM N1 ;
- les politiques prouvables TwoHanded, EitherHand, MainHand,
  OffHand, ou la valeur explicite Unresolved ;
- les boucliers passifs et armes offensives demontres par
  ItemType et DamageDice ; les Focus restent indeterminables
  sans information de type certaine ;
- les divergences entre plusieurs definitions du meme archetype
  et les archetypes encore sans definition concrete.

Le texte informatif HandUsage du catalogue ne cree jamais de
restriction d'equipement et aucun raccourci actif n'est invente.

## Execution autonome

Apres build et Automation Grimrock reussis, l'agent execute une seule
fois Scripts/PreflightARM_CLEAN06_2_HandSemantics.ps1 sur le SHA du
commit ayant pour sujet exact :
chore(ARM-CLEAN06.2E): inventory UE weapon hand semantics read-only

Le script exige un worktree detache, propre et synchronise sur
ce SHA, demarre UnrealEditor-Cmd avec -NullRHI et ecrit un rapport
JSON complet UNIQUEMENT dans le dossier prive de validation.
Une issue GitHub publie les compteurs sans noms de fichiers,
champs textuels d'origine ni autres valeurs brutes.

Le statut GitHub dedie est UE5.5.4/ARM-CLEAN06.2E.
Success signifie audit execute en lecture seule, PAS conversion
du contenu ni autorisation d'effacer les anciens champs.

Aucun .uasset, .umap, audio Sword / RatGiant, SaveGame,
lecteur runtime ni presentation WBP n'est modifie.

## Suite

Il reste a trancher les types de Focus, les regles de mains
des archetypes sans definition concrete, les liaisons d'action,
reactions, munitions, visuels et objets non-ARM. Les assets
canoniques seront authorés seulement avec preuve de parite
sur le vrai contenu UE ; la bascule des lecteurs 06.3,
le cutover 06.4 et la suppression 06.5 ne sont pas autorises
sur la base de ce seul audit. Pas de validation PIE necessaire.


## ARM-CLEAN06.2E.1 — Réparation de l'inventaire des identités

Sur le SHA `39e4526d`, le build UE et la suite Automation `Grimrock`
étaient verts, mais le commandlet additionnel s'est arrêté à l'étape
`COMMANDLET`. La cause détaillée n'était pas identifiable dans les
diagnostics publics : elle ne doit donc pas être présentée comme certaine.

La version 06.2E.1 apporte deux protections :

1. Un `ItemDefinitionId` vide ou répété parmi les 24 définitions est
   désormais **une anomalie inventoriée**, pas une raison d'interrompre
   l'examen avant la fin du dossier. Les 24 lignes sont conservées,
   sans renommer, fusionner, corriger ni ignorer aucune variante
   (notamment les ThrowingKnife).
2. Le script Windows distingue par codes **strictement filtrés**
   les causes possibles : argument, catalogue, archétype, nombre
   d'assets, sortie JSON, rapport absent ou erreur native. Seule une
   catégorie prédéfinie peut paraître dans le statut GitHub ; les
   données métiers, chemins, logs bruts et noms d'assets restent privés.

Le rapport natif comporte les compteurs
`unique_nonempty_item_ids`, `missing_item_ids` et
`duplicate_item_ids`. Leur somme doit valoir 24. Un ID absent ou
dupliqué **bloque toujours la migration binaire**, même si l'inventaire
en lecture seule peut être validé. Les joints de catalogue et les
règles de mains encore incertaines restent non résolus.

La reprise automatique est attachée au sujet exact du commit
`fix(ARM-CLEAN06.2E.1): preserve conflicting IDs and diagnose native census`.
L'agent exécute toujours l'audit uniquement **après** réussite de la
compilation et de tous les tests `Grimrock`, dans un worktree détaché
et propre. Une réussite exige aussi la création du JSON natif,
sa validation structurelle, la propreté Git après le commandlet et
la publication d'une issue résumée. Le statut spécifique
`UE5.5.4/ARM-CLEAN06.2E` reste distinct de `UE5.5.4/LocalAuto`.

**Aucune conversion des assets, suppression d'UPROPERTY, bascule
runtime, opération SaveGame ni modification Sword/RatGiant.**
Une recette PIE ne serait pas significative à ce stade.


## ARM-CLEAN06.2E.2 — Inventaire par package, sans perte de classe

### Diagnostic verifie

Le commit 921f5d0d a obtenu Build + Automation Grimrock = success.
Le controle natif 06.2E a echoue avec le code filtre ASSET_COUNT.
Cela etablit uniquement que le nombre d'instances chargees comme
UGridItemDefinitionAsset n'etait pas egal a 24 ; le nombre exact,
la classe effective et la liste des objets non reconnus n'etaient
pas connus. Aucun cas de doublon d'ItemDefinitionId n'est affirme.

La cause structurelle du blocage est la condition de comptage :
l'ancienne boucle ne conservait que les resultats de
Cast<UGridItemDefinitionAsset>(Asset.GetAsset()), avant de
comparer leur nombre a l'inventaire de 24 packages UE.

### Nouvelle couverture native

Le commandlet selectionne les 24 packages dont le nom commence
par DA_Weapon_ dans le repertoire Weapons, independamment
de la classe C++ chargee. Les deux catalogues
DA_WeaponCatalogue et DA_WeaponVisualCatalogue sont exclus par
construction de ce prefixe ; aucune variante ThrowingKnife,
Shuriken, Sword ou autre arme n'est ecartee en raison de son type.

Pour chaque package, le JSON prive conserve :

- PackageName, classe signalee par AssetRegistry, classe de
  l'UObject effectivement charge si disponible ;
- classification expected_class, other_class ou unloadable ;
- doublon de PackageName et, uniquement pour une definition
  UGridItemDefinitionAsset, les champs disponibles de l'ancienne
  fiche, les informations de main et la jointure ARM ;
- les cas inconnus sont explicitement Unresolved, jamais
  convertis en valeur par defaut ou fusionnes.

Les compteurs publics sont weapon_packages (attendu 24),
unique_weapon_packages (24), duplicate_weapon_packages (0),
count_expected_class, count_other_class et count_unloadable.
La somme des trois classes doit valoir 24. La somme des
ItemId uniques, absents et dupliques doit correspondre a
count_expected_class, NON au nombre brut de packages.

Le test Automation
Grimrock.RPG.ARM.CLEAN06_2.Semantics.WeaponPackageRegistry
verifie independamment que les 24 packages et les deux
catalogues sont toujours inventoriables et que les classes
attendues, autres et non chargeables couvrent la totalite
des 24. Il ne presuppose pas 24 objets UGridItemDefinitionAsset.

### Garde-fous, statut et suite

Le script Windows rejette tout rapport incomplet, toute
classification inattendue, le moindre doublon de package,
la perte d'une ligne, et toute modification Git du worktree
detache. Seuls des compteurs filtres sont publies sur GitHub ;
les classes et chemins exacts restent dans le rapport prive.

Le declencheur exact du jalon est :
fix(ARM-CLEAN06.2E.2): inventory package classes without casting loss

La validation requiert UE5.5.4/SilentTask,
UE5.5.4/LocalAuto puis UE5.5.4/ARM-CLEAN06.2E
sur le meme SHA. Le resultat positif signifie uniquement
qu'un inventaire complet a ete execute sans ecriture.
Une ou plusieurs classes differentes ou inchargeables
restent des BLOQUEURS de migration ; le compteur d'assets
n'est jamais force a la valeur attendue.

Aucun .uasset, .umap, SaveGame ni effet sonore Sword/RatGiant
n'est modifie par cette etape. Aucun lecteur runtime,
WBP ou equipement n'est change. Pas de PIE requis ;
les conversions 06.3-06.5 restent interdites.


## ARM-CLEAN06.2E.3 — Rapprochement du manifeste et des classes natives

### Preuves d'entree

Le SHA a08b4590 (06.2E.2) est valide sous UE 5.5.4 : SilentTask,
LocalAuto et le commandlet read-only sont tous success. Son rapport
public recense 24 packages DA_Weapon_*, 23 instances de la classe
UGridItemDefinitionAsset, une instance chargeable d'une autre classe
et aucun echec de chargement. Il confirme 22 jointures ARM, 19
regles de mains demontrables et 21 types offensif/bouclier demontrables,
sans ItemId duplique ni manquant parmi les 23 instances attendues.

La source de production Config/Armement/WeaponProductionManifest.csv
contient exactement **22 lignes** distinctes (ItemPath, ItemDefinitionId,
ArchetypeId et BuildKind). Les **deux autres packages** sont des
elements historiques/specialises possibles : ils ne doivent pas etre
supposes inutiles ni fusionnes.

### Rapprochement autonome

Le nouveau module PowerShell read-only
Scripts/ARM_CLEAN06_2E3_Reconcile.ps1 est execute apres le
commandlet natif et l'examen de son rapport prive.

Il confronte les 22 chemins du manifeste aux 24 packages natifs,
en conservant l'identite de chaque fichier :

- manifeste vs hors manifeste ; classe attendue vs autre classe
  ou fichier non chargeable ;
- correspondance ItemDefinitionId / ArchetypeId sur les vraies
  instances UGridItemDefinitionAsset du manifeste ;
- causes de jointure absente : opt-out historique, archetype
  vide, archetype inconnu ou profil source introuvable ;
- profils joints mais sans regle de mains/type d'armement prouve ;
- regroupement de la classe inattendue par **module** seulement
  (Engine, module natif du jeu, Blueprint, editeur, autre).

Les invariants exigent 22 packages trouves dans le manifeste,
exactement deux autres, 82 archetypes sources et coherence de
tous les compteurs avec les statistiques natives. Toute anomalie
de structure rejette l'audit.

Les noms des packages, ItemId, ArchetypeId, chemins de classes,
valeurs BuildKind, analyses de jointure et correspondances exactes
sont conserves uniquement dans un second JSON **prive**
reconciliation-private.json. L'issue GitHub ne comporte
que des comptes derives et des classes de modules predeterminees.
Il est interdit d'inferer un type gameplay ou une regle de main
depuis le seul nom du package.

### Execution et conditions de sortie

Le sujet de commit autorise une seule execution dans le worktree
Windows detache, apres compilation et Automation Grimrock :

chore(ARM-CLEAN06.2E.3): reconcile source-class and ARM coverage

Le helper est soumis au parseur PowerShell avant l'execution.
L'agent valide le rapport natif, le manifeste, le second rapport
et l'absence de modification du worktree. La validation du SHA exact
requiert UE5.5.4/SilentTask, UE5.5.4/LocalAuto et
UE5.5.4/ARM-CLEAN06.2E.3 tous a success.

Le rapprochement ne convertit aucun .uasset/.umap,
ne touche ni Sword/RatGiant ni SaveGame et ne modifie pas
les regles runtime. Un resultat vert **identifie la nature du
blocage**, mais n'autorise pas la migration binaire, le
nettoyage de legacy ni un changement automatique de type.
Aucune PIE n'est necessaire pour cet audit purement structurel.


## ARM-CLEAN06.2E.4 — Qualification des types et regles de mains non resolus

### Reference de depart (preuve native E.3)

Le commit 5dd1db70 a passe UE5.5.4/SilentTask,
UE5.5.4/LocalAuto et UE5.5.4/ARM-CLEAN06.2E.3.
L'inventaire UE a montre :

- 22/22 objets declares au manifeste de production ont la
  classe UGridItemDefinitionAsset et les IDs attendus ;
- deux packages existent hors manifeste, dont un
  UGridItemDefinitionAsset et un autre objet UE chargeable ;
- un seul des 23 UGridItemDefinitionAsset est en opt-out
  (bUseWeaponCatalogue=false) ; aucun ARM vide, inconnu ou
  dont le profil N1 manque parmi les 22 objets relies ;
- sur ces 22, trois HandPolicy et un Kind restent indetermines.

Ces constats etablissent des comptes, pas la signification
ni l'utilite de l'objet non conforme a la classe supposee.

### Observations de 06.2E.4 (en lecture seule)

Le commandlet UE ajoute sur **chaque** ligne de package :

- is_data_asset, is_primary_data_asset, class_compiled_from_blueprint
  derives de l'heritage reel de l'UObject charge (sans supposition
  fondee sur le nom de fichier) ;
- sur les instances UGridItemDefinitionAsset uniquement :
  HandUsage saisi, usage de main effectif, declaration explicite
  de MainHand/OffHand, autorisations CanEquipToSlot, nombre
  d'actions locales, opt-in de lancer, et activation des des N1.

Un helper PowerShell distingue dans qualification-private.json :

- classe atypique : DataAsset ou non-DataAsset,
  PrimaryDataAsset ou non, classe compilee par Blueprint ou non ;
- opt-out historique : dans/hors manifeste, actions locales
  presentes et capacite de lancer active ;
- main indeterminee : NotHandHeld effectif, Auto non resolu,
  incoherence de permissions OneHanded/TwoHanded, autre ;
- kind indetermine : Weapon sans degats (candidat de Focus
  **sans attribution automatique**), Shield avec degats
  ou autre ecart ItemType/DamageDice ;
- difference entre la liste de slots explicitement auteurisee
  dans le DataAsset et les slots autorises par CanEquipToSlot.

**Attention capitale :** dans le code legacy actuel,
CanEquipToSlot(MainHand/OffHand) autorise les DEUX mains
pour une definition OneHanded ou TwoHanded, independamment
de CompatibleEquipmentSlots. Il ne faut donc pas importer
CompatibleEquipmentSlots comme restriction active sans valider
le comportement historique exact. Une politique EitherHand
observee dans le runtime n'est pas forcement l'intention du
createur de l'asset.

La valeur FocusPassive dans le manifeste de production reste
une **intention source** distincte d'une preuve runtime.
Aucun changement de gameplay, categorie de classe, de
HandPolicy ou de type passif n'est opere.

### Execution, controles et limites

Apres le build et tous les tests Grimrock, l'agent sur worktree
detache invoque le sujet exact :

chore(ARM-CLEAN06.2E.4): qualify native weapon semantics without asset writes

Le prevol valide successivement le recensement UE v1 (complements
de colonnes ajoutees), le rapprochement E.3 et le helper E.4.
Il refuse toute somme de compteurs incoherente ou ligne perdue,
puis confirme la proprete Git. Le rapport individuel et les
classes exactes demeurent dans le RunDir prive ; l'issue GitHub
ne contient que des categories fixees d'avance.
Le statut de validation est UE5.5.4/ARM-CLEAN06.2E.4.

Un resultat success atteste seulement d'un **diagnostic
structurel qualifie**. La preuve d'equivalence de gameplay
n'est pas encore faite : ni les 60 archetypes sans definition
reliee, ni les politiques de mains, Focus ou les refs
d'actions et projectiles ne sont convertis. 06.3–06.5
restent bloques jusqu'aux preuves de parite pertinentes.
Aucun .uasset, .umap, SaveGame ni effet audio Sword/RatGiant
n'est ecrit. Pas de controle PIE pour cet audit sans
changement fonctionnel.


## ARM-CLEAN06.2E.4.1 — Réparation contrôlée de la qualification

Le SHA d352a889 a passé UE5.5.4/LocalAuto (compilation et suite
Grimrock), mais le sous-contrôle E4 s'est arrêté à la phase
QUALIFY. Cette phase regroupait auparavant le parsing, l'accès aux
colonnes natives, le calcul des métriques et la sortie JSON :
**la cause exacte de ce premier échec n'a pas été démontrée.**

Le helper E4 utilise désormais des affectations PowerShell
explicites plutôt que l'expression conditionnelle inline
pour la qualification de classe et les usages de mains.
Il vérifie chaque invariant indépendamment et lève un code
préfixé E4_ (nombre de packages, classe, opt-out, mains,
kind, Focus) en cas de défaut.

Le preflight Windows vérifie les colonnes requises du rapport
natif (y compris les nouvelles propriétés UDataAsset,
HandUsage et slots), puis publie une catégorie prédéfinie :
QUALIFY_PARSE, QUALIFY_SCHEMA, QUALIFY_EXEC, QUALIFY_REPORT,
QUALIFY_CLASS_HIERARCHY, QUALIFY_EXPECTED_CLASS ou E4_... .
Aucun message d'exception, chemin, ItemId, ArchetypeId, classe
exacte ou donnée du package n'est publié.

Le déclencheur exact du prochain audit est :
fix(ARM-CLEAN06.2E.4.1): normalize semantic qualification and classify failures

Statut exact : UE5.5.4/ARM-CLEAN06.2E.4.1, distinct de
UE5.5.4/LocalAuto. Un vert atteste seulement du diagnostic
en lecture seule et de la propreté Git du worktree isolé.

L'architecture et les règles actuelles de combat ne sont pas
modifiées. Pas de conversion automatique, pas de suppression
d'asset ni de .uasset/.umap/SaveGame touché ; aucune
validation PIE n'est significative pour ce sous-jalon.


## ARM-CLEAN06.2E.4.2 — Localisation de l'erreur d'exécution (read-only)

La validation du SHA e973875c (E.4.1) a confirmé
SilentTask=success et LocalAuto=success, mais
ARM-CLEAN06.2E.4.1=failure, categorie QUALIFY_EXEC.
Le parsing et la presence des champs natifs ont donc ete
verifies. Il reste une exception **non categorisee** a
l'interieur du helper PowerShell ; aucun defaut d'asset
ne peut en etre deduit.

Cette etape ne change aucun critere de parite ni les
regles du jeu. Elle place des jalons constants dans
le helper : INIT, INIT_FACTS, ROW_START, ROW_CLASS,
ROW_OPTOUT, ROW_SLOTS, ROW_SEMANTICS, ROW_FACT,
ASSERT, OUTPUT_MODEL et OUTPUT_JSON. Le preflight
publie uniquement un code predefini E4_<phase>
lorsque l'exception est inconnue.

Un nouveau rapport qualification-error-private.json
enregistre, uniquement dans le RunDir du worktree
de validation, le SHA, l'etape, le type d'exception,
l'ID PowerShell, la pile et le message exact. Aucune
de ces donnees n'est envoyee aux statuts GitHub.
Les codes d'invariants connus E4_* restent actifs.

Sujet du commit unique de reprise :
fix(ARM-CLEAN06.2E.4.2): locate qualification runtime failure safely

Un succes du controle
UE5.5.4/ARM-CLEAN06.2E.4.2 atteste la qualification
native sans sauvegarde binaire. Un echec E4_<phase>
identifie exactement la portion de script a corriger
dans un jalon additif suivant. Aucun .uasset,
.umap, SaveGame ni asset audio n'est ecrit ou supprime.
La conversion ARM-CLEAN06.3-06.5 demeure interdite.


## ARM-CLEAN06.2E.4.3 — Proteger les colonnes propres a UGridItemDefinitionAsset

La sortie native E.4 contient 24 lignes : 23 objets de la classe
UGridItemDefinitionAsset et un objet chargeable d'une autre classe.
Ce dernier est volontairement represente sans les champs de main
et de combat propres au type ItemDefinitionAsset.

Or le helper E4 faisait, sans garde de classe, des lectures de
declared_slot_main / declared_slot_off, can_equip_main / can_equip_off,
y compris sur la ligne d'autre classe. Sous PowerShell
Set-StrictMode -Version Latest, une propriete absente leve une
exception, alors que cet objet n'est pas une definition d'item.
Ceci explique le risque d'echec avant meme toute analyse semantique.

La version E4.3 initialise pour ces champs une valeur **null**,
explicite et distincte de false, puis ne lit les colonnes de
main et d'usage que dans le cas
load_classification == expected_class. Aucune main fictive,
aucun enum invente, aucun fallback legacy. Les 24
packages demeurent dans le rapport prive.

Le declencheur est :
fix(ARM-CLEAN06.2E.4.3): restrict item-only reads to verified UE class

Ce sous-jalon exige SilentTask=success, LocalAuto=success,
et UE5.5.4/ARM-CLEAN06.2E.4.3=success sur le meme SHA.
Les rapports precis restent prives et les issues ne diffusent
que des comptes agreges. Un succes reste un diagnostic sans
materialisation des nouveaux DataAssets ; les conversions
06.3-06.5 demeurent conditionnees aux preuves de parite
completes. Aucun uasset, umap, audio ou SaveGame n'est ecrit.
