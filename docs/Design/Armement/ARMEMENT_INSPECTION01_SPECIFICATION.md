# ARMEMENT-INSPECTION01 — Examen 3D et atelier d'armement

**Version 1.1 — cahier des charges approuvé avec compléments — 26.11.2026**
Projet GrimrockPrototype · Unreal Engine 5.5.4 · Windows / Visual Studio 2026
Prérequis : ARMEMENT-DESIGN01 (fiche maîtresse approuvée par le propriétaire).
Référence technique consultée : `da9f7cbc2fff4bec367a11f68ec7b6c18b866a50` (`master`, deux statuts Windows SUCCESS au moment de la consultation).
Statut : conception approuvée, complétée par [ARMEMENT-INSPECTION01A](ARMEMENT_INSPECTION01A_ADDENDUM.md) qui prévaut en cas de contradiction. Aucun changement C++, WBP, Niagara, DataAsset ou SaveGame autorisé par ce document avant lancement du périmètre d'implémentation.

## 1. Finalité et limites

L'auteur doit pouvoir inspecter chaque objet, en comprendre la configuration réelle, construire et affiner ses ressources artistiques et valider sa présentation avant de les introduire dans le donjon. Support générique des objets dès le socle, priorité artistique aux 22 ARM existants puis aux 82 archétypes lorsqu'ils seront matérialisés ; objets non ARM actuels et équipements futurs inspectables sans nouvelle architecture. L'option Examiner du jeu devient un véritable écran d'examen 3D, en lecture seule. L'atelier d'armement est un outil d'Unreal Editor utilisable hors PIE.

L'atelier n'est ni un nouveau système d'inventaire, ni un éditeur de statistiques parallèle, ni une réimplémentation de Niagara, de Blender ou du Material Editor. Il ne crée pas d'attaque, de rareté, d'enchantement ou de progression automatique. Les niveaux ARM sont fixes pour chaque instance réelle, N1–N20 ; les non ARM restent N0. Une simulation n'altère aucune mécanique, sauvegarde, propriété ou possession d'objet.

## 2. État réel et éléments à réutiliser

| Élément | État vérifié au SHA de référence | Conséquence |
|---|---|---|
| `UGridInventoryWidget::ExecuteResolvedInventoryContextAction` | La branche Examine valide les métadonnées, récupère un texte de tooltip si le GUID du slot correspond, puis appelle `PresentItemExamination(LastContextItem, TooltipText)` | Conserver le point d'entrée et transmettre l'instance ; ne pas analyser le texte du tooltip pour obtenir les stats |
| `PresentItemExamination(const FGridItemInstance&, const FText&)` | BlueprintImplementableEvent déclaré dans `GridInventoryWidget.h` | Préserver le contrat pendant le raccordement ; vérifier les WBP natifs avant tout retrait ou remplacement |
| `GridEditorArmamentPreview::Resolve` | Helper éditeur : vérifie l'identité exacte de la définition enregistrée, utilise un registre transitoire avec la racine, résout N1 et N20 | Étendre/réutiliser ce chemin pour N1–N20, sans seconde autorité ; refuser doublons/non-indexés |
| `AGridItemActor::SetPresentationMesh` et `ApplyWeaponFinish` | Application de finitions par MID, préserve les overrides explicites ; ne change pas les slots du mesh | Factoriser/réutiliser la projection visuelle, pas recopier l'algorithme des teintes |
| Catalogues actuels | Quatre catalogues sous DA_ItemSystem ; audio, VFX, mouvements, projectile et finitions | Conserver les autorités et bindings existants |
| NS de production FX01 | Raccordés techniquement, rendu refusé par le propriétaire | Afficher comme provisoires ; aucune approbation artistique implicite |
| HOTFIX01 | Nettoyage Niagara sécurisé ; succès utilisateur PIE et statuts Windows | Rejouer les tests de destruction, réentrance, fermeture et victoire |

L'état binaire du Blueprint d'inventaire, le routage exact de l'audio de preview dans un monde éditeur et le rendu natif des assets doivent être vérifiés sur le poste UE. Le présent document ne prétend pas les avoir exécutés.

## 3. Architecture cible : une projection commune, deux hôtes

```
DA_ItemSystem -> UGridItemSubsystem -> FGridResolvedItem
                                   -> résolution d'action / présentation canonique
                                             |
                            socle de présentation d'inspection (runtime)
                              /                                  \
                     hôte joueur                           hôte éditeur
                     UMG + rendu 3D                        Slate + viewport natif
```

- Le runtime porte la projection de présentation et les règles de cycle de vie, pas les commandes d'authoring. Le module `GrimrockPrototypeEditor` porte le viewport, les outils, les transactions et les liens vers les éditeurs natifs. Aucune dépendance UnrealEd/Slate d'éditeur dans Shipping ; Slate runtime éventuellement déjà existant n'est pas interdit.
- Construire un objet de présentation **non possédable**, non enregistrable dans le donjon et sans GUID métier réutilisé comme identité active. L'instance réelle est uniquement une source de lecture. Préférer un acteur/composant de preview minimal non interactif réutilisant la résolution et la projection du mesh/finitions, sans `AGridItemActor` jouable enregistré dans le monde. Ne pas copier l'intégralité des acteurs runtime.
- Les actions simulées consomment les profils de présentation, et les sélections de variantes/événements du runtime doivent être réutilisées lorsque disponibles. Les résultats synthétiques ne passent jamais par le TurnManager, ni par les transactions d'inventaire. Si un helper doit être extrait, modification ciblée et tests de parité uniquement.
- Une représentation non ARM possède les champs réellement pertinents ; pas de fausse entrée ARM, pas de niveau 1 artificiel. Les objets sans mesh conservent une icône et les données textuelles. Ne pas réintroduire de fallback historique du registre.
- Pas de cinquième catalogue, pas de DataAsset de configuration par arme en plus de sa définition. Les fiches Markdown servent à la validation humaine et non au runtime. Les préférences locales de caméra/atelier peuvent être conservées en paramètres éditeur hors SaveGame et hors profils d'armes.

## 4. Mode joueur — Examen 3D

### 4.1 Parcours

Clic droit sur un objet d'inventaire ou équipé -> menu contextuel -> Examiner -> fermeture du menu -> résolution canonique de l'instance sélectionnée -> fenêtre d'examen. La provenance de l'instance (personnage/slot/GUID) est vérifiée à l'ouverture. Le niveau affiché est le niveau physique réel. Le panneau ne déplace, n'équipe, ne consomme et ne modifie rien. Au retour, restaurer le focus et l'état initial de l'inventaire, y compris le personnage et les panneaux ouverts. Si l'objet devient indisponible, fermer ou afficher un état explicite, jamais une autre instance du même ItemId.

L'ouverture via d'autres surfaces (objet au sol, réceptacle) est souhaitable à terme uniquement si leur chemin existant fournit une instance autorisée et stable ; ne pas ajouter un nouveau droit d'interaction au monde pour cette première version. L'action Lire reste distincte, les Readables et leurs panneaux ne sont pas touchés.

### 4.2 Présentation proposée

```
┌─────────────────────────────────────────────────────────────────────┐
│ EXAMINER      [Nom de l'objet]  [Niveau si ARM]                  [X] │
├───────────────────────────────────────┬─────────────────────────────┤
│                                       │ Identité / description      │
│           VUE 3D DE L'OBJET           │ Poids / quantité            │
│                                       │ Caractéristiques           │
│        Rotation • zoom • pan          │ Dégâts / protection         │
│                                       │ Portée / PA / mains         │
│                                       │ Actions et conditions       │
│ [Face] [Profil] [Dos] [Recentrer]      │ Informations pertinentes   │
├───────────────────────────────────────┴─────────────────────────────┤
│ [Retour]                 Glisser : tourner • molette : zoom          │
└─────────────────────────────────────────────────────────────────────┘
```

Exigence : fond noir neutre par défaut, ambiance 2.5D lointaine facultative et non intrusive, aucun décor proche, cadre RPG sobre, éclairage studio doux de l'objet, dimensions adaptatives. Vue 3D prioritaire (~60 % de largeur à 16:9), panneau de détails à droite défilable. Éprouver 1280×720, 1920×1080, 2560×1440, 3840×2160 et un écran ultralarge, DPI et marges safe zone. Pas d'animations de combat en boucle en mode joueur ; seuls les effets persistants légitimes de l'objet examiné sont visibles. Pas de slider de niveaux, simulateur, noms internes, paramètres debug, boutons audio/VFX ni commandes d'écriture.

### 4.3 Données visibles

- Nom et description métier localisables, icône éventuelle, niveau ARM, poids (unitaire et total si utile), quantité, famille et main(s) autorisées.
- Arme offensive : dés et bonus intrinsèques au niveau résolu, type/sous-type de dégâts, portée en cases, coût PA, attribut de scaling, précision, munitions et actions effectivement disponibles ; distinguer valeurs intrinsèques de celles influencées par le personnage si ces dernières sont ajoutées. Ne pas déduire la formule de dégâts d'un texte affiché.
- Passif : protection nominale, réserve restante si disponible pour cette instance, conditions et réactions réelles. Un ARM070 à zéro n'est pas présenté comme absorbant des dégâts. Une arme passive n'a pas de fausse action.
- Exigences de compétence authentiques et recommandations de classe clairement distinguées. Aucun affichage d'une compétence spéciale non implémentée.
- Descriptions longues/lore si déjà authorés et autorisés ; le lore historique des feuilles de conception n'est pas un champ runtime établi. Décision ultérieure si besoin, sans inventer de champ ni dupliquer les six propriétés de la définition.
- Non ARM : niveau absent à l'écran, description et propriétés adaptées à la catégorie ; si aucune donnée 3D, conserver une présentation textuelle utilisable. Les sections non applicables sont masquées, pas remplies de zéros artificiels.

### 4.4 Souris, clavier et focus

| Entrée | Comportement proposé |
|---|---|
| Glisser gauche dans le viewport | Rotation orbitale yaw/pitch, sans changer le transform de l'asset |
| Molette dans le viewport | Zoom borné selon les bounds de l'objet, sans clipping |
| Glisser milieu | Pan caméra borné |
| Face / Profil / Dos / Dessus / Recentrer | Vues reproductibles ; cadrage selon l'échelle réelle |
| Échap, X, Retour | Fermeture ; restauration de l'input et du focus antérieurs |
| Molette hors viewport | Scroll du panneau de détails, jamais zoom |

Le clic droit n'ouvre pas de menu contextuel dans la vue 3D. La capture souris n'est active que dans le viewport et relâchée à la perte de focus. L'écran ne doit pas propager les actions souris/clavier au donjon ni à la hotbar (notamment mouvement, attaque, Use, drag/drop). Proposition : ouverture autorisée selon la disponibilité actuelle d'Examiner, y compris pendant la phase joueur si le menu le permet ; aucune consommation de PA et aucun changement d'initiative ou de politique globale de pause. Aucun gel de combat par simple ouverture ; si le contexte ne permet pas une consultation sûre pendant une transition de tour, fermer/refuser proprement et tester. Le comportement exact de pause du monde doit être décidé et testé avant raccordement ; ne pas modifier la règle de combat en passant.

### 4.5 Rendu et sécurité runtime

Approche proposée : `SceneCaptureComponent2D` + RenderTarget transitoire dans une scène de présentation contrôlée, affiché par un matériau UI dans le WBP. Étudier en premier une scène de capture isolée du gameplay (hôte non enregistré, composants show-only, éclairages isolés, pas de collision/interaction, pas de lumière/son parasite dans le donjon). Valider un prototype natif UE5.5.4 avant de figer le mode d'isolation ; si l'isolation dans le monde du jeu ne peut être garantie, employer un monde de présentation dédié avec cycle de vie maîtrisé et vérifier capture/rendering. Ne pas supposer que `FAdvancedPreviewScene` d'éditeur est disponible en Shipping.

Caméra cadrée selon bounds : focale/FOV stable, distance minimale/maximale dérivées, near clip sans traversée ; pivot de caméra indépendant du pivot asset. Fond noir neutre, exposition fixe, profil d'éclairage studio indépendant du fond ; ambiance 2.5D facultative, aucun décor proche en mode Examen. RenderTarget initial 1024×1024, résolution adaptative proposée 512–2048 à mesurer, aspect ratio respecté, pas de distorsion ni alpha défectueux. Capture à l'ouverture et aux changements, mise à jour pendant rotation/zoom/VFX actif uniquement, aucune capture quand masqué. Profilage réel GPU obligatoire avant validation. Libérer ressources, callbacks et capture à fermeture, changement de map, EndPlay et destruction du widget. Si pas de mesh ou capture indisponible, utiliser icône + texte, log explicite ; aucun crash ni duplication.

## 5. Mode auteur — Atelier intégré à Unreal Editor

### 5.1 Accès et disposition

Proposition : menu Outils → Grimrock → Atelier d'armement ; onglet dockable enregistré dans le module éditeur ; bouton d'ouverture depuis une fiche ARM ou un objet sélectionné dans l'éditeur de donjon à ajouter après vérification du point d'intégration natif. Sans PIE pour les fonctions d'authoring principales. Le viewport utilise une scène de preview native éditeur (candidat `FAdvancedPreviewScene` / `SEditorViewport`, à vérifier dans la version installée) et un client de viewport dédié. Le moteur de présentation commun reçoit un hôte de scène adapté ; pas de dépendance editor dans le jeu.

```
┌────────────────────────────────────────────────────────────────────────────────────┐
│ ATELIER D'ARMEMENT   ARM014 — Épée droite    N1 [──────●────] N20  [N17] [↶] [↷]  │
│ [Studio | Torche | Donjon]   [Examen | Sol | Décor | Main | Vol]  [Comparer]       │
├──────────────────────┬───────────────────────────────────────┬─────────────────────┤
│ Catalogue            │                                       │ Inspecteur          │
│ Filtre / recherche   │          VIEWPORT 3D                 │ Identité            │
│ ARM001 Dague         │                                       │ Mesh / transform    │
│ ARM002 Dague assassin│          Objet sélectionné            │ Matériaux           │
│ ...                  │                                       │ Finition N17        │
│ ARM014 Épée droite   │          Grille / échelle            │ SFX / VFX           │
│ ...                  │                                       │ Timing / stats      │
│ ARM081 Égide sacrée  │ [Face] [Profil] [Dos] [Recentrer]     │ [Ouvrir asset]      │
├──────────────────────┴───────────────────────────────────────┴─────────────────────┤
│ SIMULATEUR : Action • résultat • surface • variante • vitesse/seed                │
│ [▶ Jouer] [■ Stop] [Rejouer]       Départ ── Impact ── Récupération              │
│ État : SFX / Niagara / source / position / orientation / durée / nettoyage         │
└────────────────────────────────────────────────────────────────────────────────────┘
```

Les noms de widgets/commandes et la disposition sont proposés ; la palette et les proportions exactes seront décidées sur le premier prototype. L'atelier ne nécessite pas de WBP pour son chrome éditeur : utiliser Slate et les Property Editors natifs, cohérents avec UE. Le mode joueur reste UMG.

### 5.2 Sélection et niveaux

- Énumérer toutes les définitions indexées dans DA_ItemSystem ; filtrer par type, ARM, famille, nom, identifiant. 22 ARM réels au départ, plus les objets non ARM matérialisés. Les 60 archétypes numériques seuls sont explicitement absents du catalogue d'objets inspectables (option future « non matérialisé », sans mesh fictif). Recherche par ARMXXX, ItemId, nom.
- Sélecteur entier N1–N20 : slider, saisie numérique, précédent/suivant, raccourcis de bandes N1/5/9/13/17/20. Valeur N1 par défaut pour ARM ; non ARM résolu à N0, tous contrôles de niveau masqués. Résoudre chaque niveau via `ResolveItem`, pas d'interpolation des stats par UI.
- Afficher le profil mécanique résolu, le FinishId et toutes ses valeurs réelles, les références des assets et leur provenance. Les valeurs actuelles des 5 bandes (Brut/Soigné/Gravé/Magistral/Mythique) sont des données, pas des obligations de direction artistique.
- Afficher une table N1–N20 issue du résolveur ; marquer erreurs de résolution, champs manquants ou paramètres matériau absents. Une différence visuelle perceptible à chacun des 19 passages adjacents est obligatoire ; les paramètres et leur support réel seront décidés sur le pilote. Un changement de niveau stoppe la simulation précédente, remplace mesh/MID/NS puis recadre uniquement si nécessaire. Pas de rechargement coûteux à chaque tick ; gérer drag du slider avec mise à jour coalescée.
- Comparaison A/B : même objet N1/N20, chacun des 19 passages adjacents dont les frontières N4/N5, N8/N9, N12/N13, N16/N17 ; option arme différente après validation. Deux instances de présentation indépendantes, caméra/éclairage synchronisables, vues A/B côte à côte ; aucun Material Parameter Collection global pour les différencier.

### 5.3 Contextes de scène et éclairage

| Contexte | Ce qu'il faut observer | Règle |
|---|---|---|
| Examen | Mesh complet, détail PBR, ornements, effets persistants | Fond noir, cadrage studio indépendant du monde ; ambiance 2.5D lointaine optionnelle |
| Sol | WorldMesh, WorldTransform, pivot, contact avec dalle, échelle | Repère cellule 200 cm ; afficher pose réelle, pas normalisation du mesh |
| Décor | Table, étagère ou réceptacle de référence, clipping, orientation | Assets de test isolés ; pas de placement dans DA_GridLevel réel |
| Main | EquippedMesh, transform, vue subjective et mouvement | Distinguer pose illustrative et représentation réellement utilisée ; arme non lumineuse pas nécessairement visible en exploration |
| Vol | ProjectileMesh, orientation, Stable/Tumble/Spin, trajectoire et FX | Utiliser les paramètres physiques pour une simulation fidèle lorsque faisable, sans lancer réel ; signaler les approximations |

Trois presets d'éclairage : studio neutre (exposition fixe), torche chaude (référence d'éclairage de production), donjon sombre (matériaux et exposition représentatifs). Fond noir par défaut dans le contexte Examen, ambiance 2.5D facultative ; décor proche réservé aux contextes techniques auteur Sol/Décor/Main/Vol ; lumière personnalisable en session. Comparer couleurs dans les mêmes conditions. Le mode studio ne valide pas le rendu Lumen/occlusion du donjon ; prévoir PIE puis Shipping pour preuves réelles.

Outils viewport : orbite, zoom, pan, recadrage, vues orthographiques/perspective si approprié, grille/axes, bounds, sockets, collisions, wireframe, visualisation des canaux PBR via modes UE disponibles. Aucun nouvel éditeur de mesh. Dimensions cm, nombre de triangles/LOD et slots consultables quand accessibles ; accès au Static Mesh Editor et au dossier source documenté, pas de fabrication Blender automatique.

### 5.4 Inspecteur et maîtrise des paramètres

Chaque onglet expose `Actuel / Essai / Autorité / État` et renvoie au champ ou asset réel. Lecture seule pour les valeurs résolues ou calculées ; champ réellement éditable distingué. Le Property Editor natif est préféré à un formulaire de duplication de tout le catalogue.

| Onglet | Champs et commandes prioritaires | Autorité |
|---|---|---|
| Identité | ItemId, ArchetypeId, DisplayName, Description, WeightKg, Tags ; navigation | DA_Weapon_* |
| Mesh | World/Equipped/ProjectileMesh, OrnamentMesh, transforms, axes, sockets, bounds | DA_WeaponVisualCatalogue + SM |
| Matériaux | Slots, parent, MI, textures, paramètres exposés, diagnostics | Material Slots du SM, M/MI/T |
| Finitions | LevelFinishIds, FinishId, RoughnessScale, NormalScale, Tint ; paramètre réel et valeur résolue | DA_WeaponVisualCatalogue |
| Persistant | VisualEffects, seuil, LightEmitter, sparkle, transforms | DA_WeaponVisualCatalogue ; distinguer lumière mécanique et cosmétique |
| Projectile | Mode, axe, rotation, scale, tumble/spin, FlightVFX ; lecture de politique mécanique | Catalogue visuel / catalogue mécanique séparés |
| Audio | Sons, variantes, volume/pitch min/max, écoute isolée, source et droits ; lien asset audio | DA_ActionPresentationCatalogue / SoundWave/Cue/MetaSound |
| Niagara | Systems, attach, socket, LocationOffset, RotationOffset, Scale, aperçu ; lien NS/emitters | DA_ActionPresentationCatalogue / NS ; persistant dans catalogue visuel |
| Mouvement | MotionStyle, bAnimateHeldItem, MotionDurationSeconds, ImpactDelaySeconds, RecoveryDelaySeconds, PeakLocationOffset/RotationOffset, FeedbackDurationSeconds | DA_ActionPresentationCatalogue |
| Gameplay | dés, protections, coût, portée, réactions, prérequis, actions, affinités ; lecture et lien vers source | DA_WeaponCatalogue et DA_ActionCatalogue |
| Validation | Résumé des références, état provisoire/approuvé, captures et rapport | Fiche ARM Markdown (pas de données runtime) |

Matière : le code actuel de finition applique notamment `RoughnessMultiplier`, `RoughnessOverride`, `NormalStrength`, `BaseColorTint` aux matériaux les exposant et ne remplace pas les overrides explicites. Afficher les valeurs parentes et effectives ; signaler un slot non compatible. L'auteur ajuste un MID de preview sans toucher à l'original. Les masques de gravures, ornements variant par niveau, emissive ou autres paramètres non raccordés sont des propositions, pas des contrôles réputés opérationnels. Le Material Editor reste l'autorité pour graphes et textures.

Niagara : lien « Ouvrir dans Niagara Editor », afficher le système/variant sélectionné, état et transforms ; éditer les emitters, modules, courbes et renderers dans l'éditeur natif. Seuls les `User.*` réellement exposés et compatibles peuvent être manipulés dans l'atelier à terme ; ne pas inventer des contrôles dont le NS ne tient pas compte. Prévisualiser CPU/GPU en vrai viewport ; pas d'export d'un faux rendu statique. Audio : sélectionner la variante exacte ou rejouer une sélection déterministe ; écouter source, puis traitement appliqué (volume/pitch/spatialisation réellement disponibles). Accès SoundWave/Cue/MetaSound et SoundClass ; pas de mixage fictif. La qualité du son est jugée à l'oreille dans l'atelier ET dans le donjon.

### 5.5 Édition temporaire, persistance et contrôle utilisateur

Trois opérations distinctes :

1. **Essayer** : copie transitoire du profil de présentation, MID local ou override de session explicitement étiqueté. Aucune transaction sur le catalogue, aucun package dirty causé par cet essai, aucune mutation du monde. Bouton Réinitialiser. Ne pas écrire de copie persistante du profil dans un nouveau DataAsset.
2. **Ouvrir la source** : navigation et édition via les éditeurs natifs (catalogue, SM, MI, NS, audio). Les changements natifs sont des modifications réelles d'assets, même si non enregistrés ; ils ne doivent pas être confondus avec les essais transitoires. Indicateur asset modifié, rafraîchissement du résolveur à la notification de changement, respect de l'Undo/Redo. Au début, cette voie est le mécanisme d'écriture principal.
3. **Appliquer les essais** (lot ultérieur) : liste exacte des différences et des armes impactées, validation explicite du propriétaire, contrôles de types/valeurs/identités, transaction Unreal `FScopedTransaction`, `Modify`, notifications de propriété, validation des catalogues, Undo/Redo ; ne pas auto-save, auto-commit ou modifier les assets liés sans accord. Prévoir annulation si conflit, asset read-only ou source modifiée depuis l'essai. Écriture limitée aux champs supportés, jamais de modification de données mécaniques depuis le simulateur. Confirmer l'enregistrement natif après revue. Aucune écriture directe binaire hors Unreal.

Un profil, un NS, un matériau ou un SoundCue partagé peut affecter plusieurs armes. Afficher l'étendue connue des références depuis les catalogues et le Reference Viewer/Asset Registry ; signaler les limites des références dynamiques. Ne pas écraser un profil partagé sans décision. Créer une variante spécialisée seulement si la conception le justifie. À la fermeture : choix conserver la session ouverte, abandonner les essais ou ouvrir le flux d'application ; respecter les dialogues natifs pour packages dirty. Aucun abandon silencieux de changements utilisateur.

## 6. Simulateur audiovisuel — comportement et réglages

Le simulateur dispose de son propre état (Idle, Preparing, Playing, Stopping), d'une horloge de présentation, d'un seed de variation optionnel et d'un résultat de scénario explicitement synthétique. Il n'accède pas au TurnManager, n'inflige aucun dégât et ne consomme ni PA, mana, munitions, charges de bouclier, ni objets. La sélection des événements disponibles doit dériver des actions/réactions réelles et de la présence des canaux médias ; pas de bouton d'attaque pour un bouclier passif. Les événements projetés à titre expérimental (ex. absorption ARM070 avec réserve positive) sont identifiés comme tels et ne sont pas présentés comme utilisables en production.

### 6.1 Événements et surfaces

| Événement à simuler | Audio | VFX | Particularités |
|---|---|---|---|
| Départ d'attaque | AttackAudio | AttackVFX | Mouvement réel lorsque disponible |
| Impact réussi générique | ImpactHitAudio | ImpactHitVFX | Position de contact sur cible neutre |
| Échec | ImpactMissAudio | ImpactMissVFX | Aucun effet de blessure/dégâts fictif |
| Chair / métal / bois | ImpactFleshAudio / MetalAudio / WoodAudio | ImpactHitVFX actuellement commun | Les VFX spécifiques de surface demandent extension approuvée si nécessaire |
| Critique | CriticalAudio avec CriticalAudioSurface | ImpactHitVFX | Accent réel sans double lecture ; VFX critique distinct non garanti |
| Parade | ParryAudio | ParryVFX | ARM013 OffHand seulement selon contrat actuel |
| Absorption physique | BlockAudio | BlockVFX | ARM075/081, débit effectif en jeu ; simulation d'un résultat positif |
| Absorption magique | MagicalBlockAudio | MagicalBlockVFX | ARM070 : réserve production nulle ; scénario expérimental distinct |
| Projectile départ | Projectile.ReleaseAudio | Projectile.ReleaseVFX | Objet lancé et source distincts selon cas |
| Projectile vol | Pas de canal audio dédié garanti | ProjectileAppearance.FlightVFX | Traînée attachée, axe et rotation conformes |
| Projectile collision | Projectile.CollisionAudio | Projectile.CollisionVFX | Contact simulé avec normale et surface identifiables |
| Projectile arrêt | Aucun canal dédié actuellement | Projectile.StopVFX | Stop traînée, fin propre |
| Persistant | Selon assets effectivement disponibles | VisualEffects / seuil / sparkle | Distinct du VFX d'action ; lumière réelle non altérée |
| Lourd / spécial / équiper / dépose / pierre | Champ HeavyAudio existant, autres à vérifier | Extensions à approuver | Afficher non raccordé plutôt que prétendre une mécanique disponible |

Surfaces de test : cible neutre à point d'impact visible, chair représentée par support de test, bois, métal, pierre comme décor ; seuls les canaux effectivement supportés par le runtime sont proposés comme raccordés (pas de canal pierre spécialisé garanti). Distance en cm et cases, normale de contact et orientation; ne pas dépendre d'un ennemi réel. Une cible d'atelier n'a aucun inventaire, IA ou PV de production. La cible peut être masquée pour observer seulement la trajectoire ou les particules. Les démonstrations audio et Niagara séparées sont disponibles même en l'absence d'une séquence complète si le média existe, clairement marquées « écoute/aperçu isolé ».

### 6.2 Contrôles

| Contrôle | Exigence |
|---|---|
| Action / main / scénario | Sélection issue des données réelles, main physique source explicitée |
| Variante SFX | Choix de chaque fichier et mode déterministe/variation réelle |
| Variante Niagara | Choix du système et de ses paramètres exposés compatibles |
| Jouer / Stop / Rejouer | Relance propre ; aucune superposition résiduelle ; Stop sûr durant toute phase |
| Audio seul / VFX seul / mouvement seul / combiné | Bascule locale sans modifier le DataAsset |
| Vitesse | 0,25× / 0,5× / 1× / 2× proposés pour le mouvement et la timeline ; audio normal par défaut, préciser le comportement Niagara selon support de simulation |
| Pause / pas temporel / scrub | Extension après prototype ; ne pas promettre seek fiable des GPU emitters ou du son, indiquer les limitations par canal |
| Seed / répétition | Fixer seed pour comparaison, puis écouter les variations ; boucle uniquement sur commande auteur avec intervalle configurable et nettoyage entre lectures |
| Cible / distance / transform | Déplacer le point de contact de test, afficher axes, socket, offset, orientation et scale ; ne jamais éditer le mesh via déplacement de caméra |

Timeline : départ, temps d'impact, récupération, éventuelle seconde main, libération/vol/contact/arrêt ; repères en secondes, curseur courant, événement effectif, média sélectionné, diagnostics. Respecter le calendrier de présentation du runtime, qui n'est pas une règle de coût PA ou initiative. Prévoir la séquence de double frappe ARM001 + ARM013 avec deux sources, deux variantes et chronologie réelle, uniquement lorsque la configuration est autorisée. Ne pas fabriquer de deuxième attaque si une première frappe fatale la rend inapplicable.

### 6.3 Cycle de vie et prévention de crash

- Chaque session possède ses propres références de rendu, MID, audio et Niagara ; aucune référence aux acteurs de combat réels. Détruire proprement à Stop, changement d'arme/niveau/contexte, rechargement asset, fermeture, hot reload pertinent, changement de map et EndPlay. Invalider les callbacks asynchrones d'une génération de preview obsolète.
- Respecter auto-destruction Niagara : détacher le tableau de suivi avant callbacks ; résoudre les références faibles de nouveau après `DeactivateImmediate` ; aucune double destruction ni déréférencement d'un composant expiré. Nettoyage idempotent et sûr en cas de réentrance. Ne pas stopper les Niagara ou sons du jeu en nettoyant le viewport.
- Aucun effet résiduel après retour à Idle ; distinguer effets ponctuels et persistants, ces derniers ne sont recréés que pour la session suivante. Limiter concurrence audio, ne pas accumuler de composants. Après 50 alternances de niveaux, 50 lectures/arrêts, 20 ouvertures/fermetures et plusieurs changements de map : compteurs/mesures doivent revenir à leur état de référence sans croissance inexpliquée.
- Les tests NullRHI couvrent logique et cycle de vie, pas la qualité de rendu ni l'audibilité. Exercices D3D12 et écoute humaine indispensables. Les actions Preview ne déclenchent pas de victoire de combat ; ajouter néanmoins un test PIE réel de dernier coup fatal au titre de non-régression HOTFIX01.

## 7. Spécification UI joueur — contrat WBP / C++

Noms proposés (à confirmer contre les assets existants avant création) : dossier `/Game/GrimrockPrototype/Blueprints/UI/Inspection/` (`Content/GrimrockPrototype/Blueprints/UI/Inspection/`) ; `WBP_ItemExamination`, parent C++ candidat `UGridItemExaminationWidget` ; `WBP_ItemExaminationStatRow`, parent `UGridItemExaminationStatRowWidget`. Réutiliser un WBP de ligne existant si son contrat sémantique et visuel conviennent. Le parent C++ et les variables `BindWidget` doivent être compilés avant construction native des WBP. C++ logique et alimentation, Designer UMG style et structure ; pas de construction native de TextBlocks, de padding/police/couleur ni de layout. Graph minimal uniquement si justifié.

Hiérarchie proposée :

```
WBP_ItemExamination
  Overlay_Root
    Border_ModalBackdrop
    Border_ExaminationFrame
      VerticalBox_Content
        HorizontalBox_Header
          Text_Title                 (statique)
          Text_ItemName              (dynamique, nom métier)
          Border_LevelBadge
            Text_ItemLevel           (dynamique)
          Button_Close
        HorizontalBox_Main
          VerticalBox_Preview
            Overlay_Viewport
              Image_RenderTarget
              Image_FallbackIcon
              Text_PreviewUnavailable (statique, état géré en C++)
            HorizontalBox_ViewControls
              Button_Front
                Text_Front
              Button_Profile
                Text_Profile
              Button_Back
                Text_Back
              Button_Reset
                Text_Reset
          ScrollBox_Details
            VerticalBox_Details
              Text_Description       (dynamique, métier)
              VerticalBox_Identity   (lignes WBP)
              VerticalBox_Offense    (lignes WBP)
              VerticalBox_Defense    (lignes WBP)
              VerticalBox_Actions    (lignes WBP)
              VerticalBox_Requirements (lignes WBP)
        HorizontalBox_Footer
          Text_ControlsHint
          Button_BackToInventory
            Text_BackToInventory
WBP_ItemExaminationStatRow
  HorizontalBox_Row
    Image_StatIcon (facultative)
    Text_Label     (dynamique, libellé localisé)
    Text_Value     (dynamique, valeur métier formatée)
```

Un même TextBlock ne concatène pas plusieurs valeurs à styliser indépendamment. Les états visibles reposent sur des sections préconstruites. Le panneau n'est pas une copie du tooltip. C++ instancie uniquement des WBP de lignes authorés. Les commandes de rotation et le viewport doivent disposer d'une surface de hit test et d'un focus contrôlables sans capturer les clics destinés aux autres panneaux.

### 7.1 Inventaire de textes (clés candidates, non créées)

| Élément | Texte FR / EN indicatif | Autorité et clé candidate |
|---|---|---|
| Text_Title | Examiner / Examine | ST_UI, `Item.Examination.Title` |
| Text_ItemName | nom réel / nom traduit | FText de la définition |
| Text_ItemLevel | N{Level} / Lv. {Level} | pattern ST_UI `Item.Examination.Level` |
| Text_Description | description réelle | FText de la définition |
| Text_PreviewUnavailable | Aperçu 3D indisponible / 3D preview unavailable | `Item.Examination.PreviewUnavailable` |
| Text_Front / Profile / Back / Reset | Face / Front ; Profil / Side ; Dos / Back ; Recentrer / Reset view | `Item.Examination.View.Front`, `.Side`, `.Back`, `.Reset` |
| Text_ControlsHint | Glisser : tourner · Molette : zoom / Drag: rotate · Scroll: zoom | `Item.Examination.ControlsHint` |
| Text_BackToInventory / bouton X | Retour / Back ; Fermer / Close | réutiliser clé commune si présente ; sinon clé dédiée |
| Text_Label des lignes | Poids, quantité, dégâts, PA, portée, protection nominale/restante, main, précision, type de dégâts, munitions, actions, prérequis | rechercher clés existantes dans UI.csv, compléter uniquement les absentes |
| Text_Value des lignes | données résolues et unités/notation | données métier, ST_UI patterns, FText::Format |
| État source indisponible | Objet indisponible / Item unavailable | `Item.Examination.ItemUnavailable` si nécessaire |

Avant chaque création WBP : relever chaque texte exact, contrôler `Localization/Source/UI.csv`, réutiliser les clés ; ajouter les absentes, exécuter `UIStringTableAuthoring` sous UE natif, vérifier code 0 et clés dans `ST_UI.uasset`, seulement ensuite authorer le Designer. Vérifier `Game/fr` et `Game/en`, locres/cook. Les clés ci-dessus sont des candidats, pas des clés déclarées existantes. Le mode éditeur Slate emploie des textes éditeur appropriés, non injectés dans le jeu ni dans SaveGame. Respecter `docs/UI_UMG_PRESENTATION_CONTRACT.md` et `docs/UI_WBP_LOCALIZATION_WORKFLOW.md`.

## 8. Plan d'implémentation — lots atomiques et portes de validation

Aucun lot de réalisation n'est lancé par l'approbation d'un document sans accord de lancement ; les noms ci-dessous sont proposés. Chaque lot technique : code/tests/docs cohérents, commit/push master, vérifier les deux statuts Windows sur le SHA exact avant le suivant. Pas de création globale des 22 VFX ; la première arme est ARM014, les autres servent de cas de vérification de l'outil.

| Lot | Livrable | Critère de passage |
|---|---|---|
| INSPECTION02.1 | Socle de projection commun et sélection canonique, preview non possédable, cycle de vie | Tous les ARM résolus sur 20 niveaux, non ARM N0, aucune mutation/duplication, tests de cleanup |
| INSPECTION02.2 | Atelier éditeur V1 : onglet, liste, viewport, orbit/zoom, niveau, stats, slots/matériaux et liens natifs | ARM014 N1–N20 observables hors PIE ; sélection des 22 ; fermeture sans fuite |
| INSPECTION02.3 | Scènes studio/torche/donjon, sol/décor/main/vol, comparaison A/B | Comparaison indépendante, preuve de scale/pose et seuils N4/5,8/9,12/13,16/17 |
| INSPECTION02.4 | Simulateur SFX, Niagara, mouvement, événements et essais locaux | Contrôles effectifs, Stop/Replay sûrs, preuves ARM014 et cas défensif/projectile/magique/dual |
| INSPECTION02.5 | Mode joueur : spike capture, route Examine, WBP Designer et localisation ; lecture seule | Réel viewport PIE et Shipping, focus/restauration, aucune mutation et repli sans mesh |
| INSPECTION02.6 (option à autoriser) | Application explicite des essais vers les champs natifs, audit d'impact, Undo/Redo ; export rapport de recette | Écriture transactionnelle ciblée, absence d'autosave/commit, contrôle des profils partagés |
| ARM014-DESIGN | Fiche ARM014 renseignée et production artistique sous contrôle du propriétaire | Chaque rubrique de DESIGN01 approuvée, recette réelle ; l'atelier n'est pas synonyme d'arme terminée |

Le choix de commencer par l'atelier éditeur est intentionnel : il donne à l'auteur la main avant de refaire les assets. Le prototype capture runtime peut être anticipé dans le socle pour lever le risque technique, sans refaire tout le WBP prématurément. Les lots sont ajustables sans mélange de responsabilité, selon preuves natives ; création de nouveaux médias seulement sur décision artistique.

## 9. Recette d'acceptation

| ID | Critère | Preuve attendue |
|---|---|---|
| INSP-01 | Liste réelle des 22 ARM ; N1–N20 résolus exactement pour chacun ; 440 combinaisons | Automation du registre et lecture atelier |
| INSP-02 | Rejet ARM N0/N21, non ARM autre que N0, ID absent/doublon | Tests d'échec et messages explicites |
| INSP-03 | Finition, meshes, transforms, icônes et FX conformes à la projection runtime | Comparaison des données et captures D3D12 ; tests MID par instance |
| INSP-04 | N1/N20 et les 19 transitions adjacentes ; A/B indépendants, aucune pollution matériau | Captures et valeurs effectives, modifications locales contrôlées |
| INSP-05 | Orbite, zoom, pan, cadrage, grille, changements de presets | Recette utilisateur atelier hors PIE |
| INSP-06 | Examen joueur d'un ARM, d'un passif et d'un objet non ARM sans niveau ; repli sans mesh | PIE + Shipping, cultures fr/en |
| INSP-07 | Sélection source correcte, aucune modification de GUID, quantité, niveau, propriétaire, équipement, charges ou sauvegarde | Comparaison avant/après sur copies isolées, vérification du focus et restauration UI |
| INSP-08 | Aucune action donjon/hotbar transmise par le viewport ; fermeture Échap, clic, perte de focus ; transition de tour/map | Tests manuels PIE, logs et tests de routage |
| INSP-09 | SFX attaque/hit/miss, variantes, volume/pitch, sélection correcte d'impact et critique | Test de sélection, mixeur et écoute réelle |
| INSP-10 | Niagara attaque/hit, défense, persistant, projection et nettoyage | Tests de composants et captures GPU ; 50 alternances, 50 lectures/arrêts, 20 ouvertures/fermetures |
| INSP-11 | Double frappe ARM001/013, boucliers ARM075/081, orb ARM070, arc ARM048, couteau ARM054, feu ARM062 ; scénarios selon applicabilité | Tests ciblés ; aucun faux gameplay ou faux canal ; ARM070 zéro réserve explicitement indiqué |
| INSP-12 | Essai n'altère aucun package ; Reset ; édition native actualisée ; Apply/Undo/Redo si lot optionnel autorisé | Git/package dirty état avant/après, tests de transaction/impact |
| INSP-13 | Aucun code éditeur dans Shipping ; cook des références médias ; rendu et performance | Development Editor, Shipping, IoStore/cook, GPU D3D12, profiling capture |
| INSP-14 | Absence du crash HOTFIX01 ; arrêt/fermeture pendant VFX, victoire dans donjon | Test de non-régression natif et recette PIE/Shipping réelle |
| INSP-15 | Fiche ARM014 complète, captures, référence des sons et droits, approbation personnelle | Fiche Markdown validée et SHA ; pas d'approbation automatique par résultat de tests |

Seuils de performance à mesurer avant fixation : capture inactive lorsque panneau fermé, aucune croissance durable des composants/MID/RenderTarget, coût GPU et temps de frame comparés avant/après ouverture sur le PC de référence, aucun effet de scène ou son parasite dans le monde. Préserver sept sauvegardes v27 et les deux Readables ; empreintes avant/après des originaux en recette sur copies. Tests Grimrock complets via scripts existants, pas de nouveau harnais générique. Ne pas confondre compilation, validation logique, rendu GPU et approbation artistique. Le propriétaire signe chaque arme après revue ; un système Niagara provisoire ne devient pas définitif parce qu'il est raccordé.

## 10. Décisions approuvées et points d'implémentation ouverts

- Deux interfaces : atelier natif éditeur hors PIE, écran joueur en lecture seule ; un seul socle de projection.
- Priorité atelier et ARM014, écran joueur raccordé après validation du socle ; examen générique des non ARM dès le mode joueur, fonctionnalités spécialisées progressivement.
- Cinq bandes existantes comme base de prévisualisation ; différence perceptible à chaque niveau obligatoire ; aucune nouvelle mécanique de niveau. Le pilote détermine les paramètres artistiques nécessaires.
- Essais transitoires et ouverture des éditeurs natifs d'abord ; application directe dans le catalogue uniquement comme lot optionnel approuvé ; aucun auto-save/commit.
- Examen sur fond noir, ambiance 2.5D facultative ; éclairages studio/torche/donjon et contextes techniques sol/décor/main/vol réservés à l'atelier, A/B et simulateur. Aucune simulation auteur accessible au joueur.
- Politique de pause de l'examen à confirmer : ne pas changer la mécanique de temps/tours actuelle ; valider le focus et la disponibilité de l'action en combat.
- Approbation graphique, sons, Niagara, niveaux et placement par le propriétaire, un armement à la fois. Pas de reconstruction des cinq NS refusés avant spécification de l'effet attendu.

## 11. Références et état de publication

- `ARMEMENT_DESIGN01_FICHE_MAITRESSE.md` (modèle approuvé le 26.11.2026) et [ARMEMENT-INSPECTION01A](ARMEMENT_INSPECTION01A_ADDENDUM.md).
- `docs/Architecture/ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md`, `docs/Design/ARM_CLEAN06_ITEM_AUTHORING_CONTRACT.md`, `docs/Architecture/DataModel.md`.
- `docs/Design/ARMEMENT_FX01_AUDIO_VFX_PROGRESSION_N1_N20.md`, `docs/Design/ITEM_CONTEXT_ACTION_SYSTEM.md`, `docs/Design/INVENTORY_INTERACTION_ROUTING.md`, `docs/Design/UI_ITEM01_ITEM_TOOLTIP_COMPARISON.md`.
- `docs/Architecture/MATERIAL_OWNERSHIP.md`, `docs/Architecture/ITEM_VFX01_DATA_DRIVEN_ITEM_NIAGARA.md`.
- `docs/UI_UMG_PRESENTATION_CONTRACT.md`, `docs/UI_WBP_LOCALIZATION_WORKFLOW.md`.
- `Source/GrimrockPrototype/Private/UI/GridInventoryWidget.cpp`, `Source/GrimrockPrototype/Public/UI/GridInventoryWidget.h`, `Source/GrimrockPrototypeEditor/Private/Details/GridEditorArmamentPreview.h`, `Source/GrimrockPrototype/Private/Runtime/GridItemActor.cpp`, `Source/GrimrockPrototype/Public/Runtime/Items/GridItemCatalogues.h`.

Destination documentaire : `docs/Design/Armement/ARMEMENT_INSPECTION01_SPECIFICATION.md`, à versionner avec la fiche maîtresse et l'addendum approuvés. Aucun asset natif ni code produit par ce lot documentaire. Les choix de classe, WBP, hôte de scène et chemins nouveaux sont des propositions, à confirmer sur l'UE installé avant réalisation. La validation technique existante du HOTFIX01 ne constitue pas une validation de cet outil qui n'existe pas encore.
