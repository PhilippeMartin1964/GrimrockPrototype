# ARM-CLEAN06 — Point de reprise fiable (15 novembre 2026)

> **Document de continuité, pas une autorisation de conversion.**
> Lors de toute reprise, **relire le HEAD réel de `origin/master` et ses statuts**
> avant d'utiliser ce point daté. Si le dépôt a avancé, ne jamais revenir au
> jalon présenté ici. Une conversation ChatGPT peut s'interrompre sans préavis.

## Dernier jalon fonctionnel confirmé

- Projet : `GrimrockPrototype`, Unreal Engine **5.5.4**.
- Dépôt : `PhilippeMartin1964/GrimrockPrototype`, branche **`master` uniquement**.
- Dernier jalon **validé en UE5.5.4** : **ARM-CLEAN06.4Q**,
  commit `608a83c76205cfc126088fc19f36d5f0e2e173aa`.
- Trois checks sur ce SHA exact :
  `UE5.5.4/SilentTask=success`, `UE5.5.4/LocalAuto=success`,
  `UE5.5.4/ARM-CLEAN06.4Q=success`.
  Issue [150](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/150) :
  6 occurrences empilement, 4 sources stackables, 7 à capacité 1,
  34 définitions transitoires et 44 sondes N0/N1/N20/N21.
- Le propriétaire a confirmé en PIE que le runtime existant (effets,
  torches, Save/Continue) reste inchangé. **Cela ne prouve pas le
  futur lecteur canonique après conversion.**
- **ARM-CLEAN06.4R est en préparation/validation** : quatre actions
  complètes et une présentation non-ARM, depuis les sources privées
  06.4O. Ne le déclarer validé qu'après trois SUCCESS sur son SHA.
  Aucune conversion binaire, SaveGame ou WBP autorisée.

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

## ARM-CLEAN06.3BB — réalisé et validé

- SHA : `69f122fcd895147eef55fe64f93dbefe29147619`
- `UE5.5.4/SilentTask=success`, `UE5.5.4/LocalAuto=success`.
- Disponibilité HUD et combat : munitions canoniques via `RequiredAmmoKindId`
  de l'arme, `AmmoKindId` de `Item_Arrows_01`, débit unitaire,
  vrai projectile récupérable. Pas de conversion binaire.

## ARM-CLEAN06.3BC — continuité des projectiles au Save/Continue

- Voir `ARM_CLEAN06_3BC_CANONICAL_ARROW_PERSISTENCE.md`.
- Capture d'une flèche canonique en vol ou fichée : aucun ancien
  `ProjectileDefinitionAsset` sauvegardé.
- Restauration : flèche N0 enregistrée résolue depuis `DA_ItemSystem` ;
  le monstre vivant conserve la flèche fichée, sinon un pickup réel
  conserve le GUID et la pose. Aucun fallback vers un ancien DA_Item
  pour un ItemId enregistré mais invalide.
- Le résultat du `LocalAuto` doit être contrôlé sur le SHA BC final,
  et le rendu PIE/Chaos reste une validation distincte.

## ARM-CLEAN06.3BD — identité canonique monde/pickup

- Le résolveur de world item est désormais commun au clic,
  ramassage par cellule et capture SaveGame.
- ItemId/ItemLevel connus enregistrés : aucune définition ancienne
  ne peut renommer un objet, ni corriger silencieusement un niveau
  invalide ; les identités inconnues restent compatibles.
- Tests de capture et ramassage effectifs sur le registre réel.
- Lire `docs/Architecture/ARM_CLEAN06_3BD_CANONICAL_WORLD_PICKUP_IDENTITY.md`.
- Vérifier les statuts Windows **sur le SHA BD** avant de déclarer
  la phase validée ; PIE/Chaos non couvert par l'Automation.

## ARM-CLEAN06.3BE — lancers physiques en combat

- Le moteur de combat utilise `FGridCombatPhysicalThrowSourceReadModel` :
  seul le niveau réel du projectile canonique et **le coût unitaire authoré**
  permettent le jet de l'arme équipée ou en raccourci.
- Le vieux `bCombatThrowWeapon` n'est plus l'autorité pour les objets
  enregistrés dans le registre ; il reste transitoire hors registre.
- Un niveau N0/N21 ARM connu ou un coût incompatible refuse le combat
  **avant les PA**. Ni vitesse ni action inventées.
- Consulter `ARM_CLEAN06_3BE_COMBAT_PHYSICAL_THROW.md` et contrôler
  les deux checks Windows du SHA fonctionnel BE avant verdict.

## ARM-CLEAN06.3BF — menu et Pawn cohérents sur les jets physiques

- Un seul `FGridPhysicalThrowPreflightReadModel` compose les lectures
  Force/poids et vitesse/arc pour le menu « Lancer » et le Pawn.
- Les balistiques historiques absentes ou non finies n'autorisent
  plus l'action ; les vrais jets N1–N20 restent purement canoniques.
- Aucune vitesse arbitraire pour les jets génériques restant à
  convertir en 06.4. Sans conversion binaire ni texte WBP nouveau.
- Lire `ARM_CLEAN06_3BF_SHARED_UTILITY_THROW_PREFLIGHT.md` et
  vérifier les deux statuts Windows sur le SHA BF publié.

## ARM-CLEAN06.3BG — réceptacles et inventaire de conversion

- Les réceptacles n'utilisent plus `DA_Item_*` pour renommer un
  `ItemId` déjà enregistré dans `DA_ItemSystem`.
- Capture : contrôle de niveau, GUID et accord de l'acteur physique ;
  les anciens contenus initiaux sans `ItemId` doivent être migrés en 06.4.
- Les trois tests `ReceptacleIdentity` couvrent insertion N0,
  niveau ARM N20, capture et conflit actor/ItemId.
- Inventaire de passage vers 06.4 :
  `docs/Architecture/ARM_CLEAN06_3BG_RECEPTACLE_AND_064_READINESS.md`.
- Ne déclarer BG validé qu'après **les deux statuts** du SHA publié.
  La conversion binaire et le PIE restent ouverts.

## ARM-CLEAN06.4A — recapture read-only des vrais contenus UE

- Après UE5.5.4 Build + Automation `Grimrock` réussis, l'agent Windows
  reconnaît **uniquement** le sujet du commit 06.4A et relance le
  commandlet existant `GridArmClean06Baseline`.
- Nouveau contexte `UE5.5.4/ARM-CLEAN06.4A` indépendant de
  `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto`.
  Exiger **les trois SUCCESS** sur le SHA de 06.4A.
- Manifeste UE privé (UPROPERTY et AssetRegistry), SHA-256 des 56
  binaires, preuves des 1640 profils/260 passifs, et SHA-256 du
  vrai `DA_ItemSystem` ; sauvegardes uniquement inventoriées
  par empreinte locale et jamais diffusées.
- Un ticket GitHub `[ARM-CLEAN06.4A]` publie uniquement les nombres
  et l'empreinte du manifeste, sans chemins personnels ni données.
- **Aucun** asset Unreal, SaveGame ou contenu de niveau n'est modifié :
  ce point est un **préflight avant** la migration binaire.
- Lire `docs/Architecture/ARM_CLEAN06_4A_READ_ONLY_ASSET_RECAPTURE.md`.
  Contrôler le ticket et les checks avant de décider 06.4B.

## ARM-CLEAN06.4B — comparaison privée des captures UE

- Le commit 06.4B déclenche, après Build et suite Grimrock, la
  comparaison des deux captures UE réelles (06.0 SHA c66bf68 et
  06.4A SHA 27563be), leurs reçus et leurs SHA-256.
- Les 56 packages, leurs UPROPERTY, les dépendances/référents
  AssetRegistry, les empreintes binaires et les 1640 profils
  sont comparés champ par champ. Les différences restent privées.
- Un rapport public [ARM-CLEAN06.4B] expose uniquement les compteurs.
- Nouveau contexte UE5.5.4/ARM-CLEAN06.4B : SUCCESS signifie
  **comparaison produite**, jamais migration autorisée.
- Aucune écriture binaire. Lire le document
  ARM_CLEAN06_4B_PRIVATE_PARITY_PREFLIGHT.md avant 06.4C.

## ARM-CLEAN06.4C — qualification des divergences UE sans écriture

- 06.4B validé : huit UPROPERTY, quatre graphes de dépendances,
  quatre empreintes binaires sur quatre packages ; zéro profil ARM modifié.
- Le rapport privé 06.4B (`comparison-private.json`) est lu **sur son
  SHA exact**, avec les deux SHA du manifeste vérifiés contre les tickets
  GitHub 84 et 134. Aucun contenu privé n'est déplacé sur GitHub.
- `QualifyARM_CLEAN06_4C_Deltas.ps1` classe les champs en domaines
  **indicatifs**, conserve chaque nom/valeur/edge dans un rapport privé
  et émet un plan `conversion-gate-private.json` qui interdit toute
  écriture, suppression, migration `.sav` ou modification d'asset.
- L'agent Windows publie `UE5.5.4/ARM-CLEAN06.4C` en plus des deux
  checks habituels et une issue `[ARM-CLEAN06.4C]` à compteurs seuls.
- `success` = qualification produite, **PAS** divergence résolue et
  **PAS** conversion autorisée. La classification détaillée et le
  mapping des valeurs doivent être prouvés avant le convertisseur.
- Voir `docs/Architecture/ARM_CLEAN06_4C_PRIVATE_DELTA_QUALIFICATION.md`.

## ARM-CLEAN06.4D — dry-run réel Unreal Editor : destinations candidates

- Charge la qualification privée 06.4C (`5da0a70`) et le manifeste
  UE 06.4A (`27563be`) avec leurs preuves et son verrou d'écriture.
- Après Build et suite `Grimrock` réussis, le commit au sujet exact
  `chore(ARM-CLEAN06.4D): verify live UE mapping candidates without binary writes` déclenche
  `PlanARM_CLEAN06_4D_EditorDryRun.ps1`.
- Lance le commandlet **réel** `GridArmClean06Baseline` en lecture seule ;
  compare 56 packages (UPROPERTY, liens, classes), 56 SHA-256 binaires,
  les 1640 profils, puis les 8 propriétés et les 4 liens divergents.
- Calcule des **destinations candidates**, sans inventer les valeurs
  ni attribuer automatiquement un ancien champ aux catalogues canoniques.
  Sortie privée et issue publique à compteurs seuls.
- `UE5.5.4/ARM-CLEAN06.4D=success` certifie la **parité de la
  recapture live** avec 06.4A, pas la migration ni son mapping final.
  Liste d'écriture des `.uasset` et sauvegardes **toujours vide**.
- Voir `ARM_CLEAN06_4D_LIVE_EDITOR_DRYRUN.md` ; vérifier les
  trois statuts Windows et l'issue avant 06.4E.

## ARM-CLEAN06.4E — contrôle du catalogue de présentations réel

- **Préflight** : rapport privé `ARM-CLEAN06.4D/dry-run-private.json`
  du SHA `046d785`, huit propriétés sur quatre packages, conversion interdite.
- Commandlet **Editor-only**
  `GridArmClean064EPresentationParity` : contrôle la valeur actuelle de
  chaque champ UE divergent, charge le vrai `DA_ItemSystem` et son
  `DA_ActionPresentationCatalogue`, suit les `ActionId` et liaisons
  `PresentationId` réellement authorés sur les archétypes.
- Le projecteur read-only `GridArmClean06PresentationProjection`
  existant fournit la valeur attendue sans inventer d'action/son ;
  comparaison stricte de **tous** les champs USTRUCT, y compris
  sons ordonnés, timings, Niagara et canaux supplémentaires.
- Rapports détaillés et états `EQUAL_ALL_FIELDS`,
  `DIFFERENT_FIELDS`, `UNBOUND` et manquants : **privés**.
  Issue `[ARM-CLEAN06.4E]` : compteurs agrégés uniquement.
- `UE5.5.4/ARM-CLEAN06.4E=success` prouve uniquement que
  l'audit a produit son rapport, pas une égalité présumée.
  **Écritures autorisées = 0** ; voir
  `docs/Architecture/ARM_CLEAN06_4E_AUTHORED_PRESENTATION_PARITY.md`.

## ARM-CLEAN06.4F — provenance des changements audio historiques

- 06.4E prouve la stricte égalité des **4 profils d'attaque actuels**
  avec les **4 présentations réellement authorées** dans le catalogue
  canonique ; issue 138, trois statuts Windows SUCCESS.
- 06.4F recoupe les **trois captures privées Unreal**
  06.0 (avant ajout des sons Sword), 06.2A (audit des ajouts utilisateur)
  et 06.4A (avant conversion), avec leurs SHA-256 et
  l'attestation de parité 06.4E.
- Compare les UPROPERTY des 56 assets et les 1640 profils ;
  dénombre les changements apparus au moment de 06.2A et
  toute dérive supplémentaire jusqu'à 06.4A.
- Rapport détaillé privé, issue `[ARM-CLEAN06.4F]` à
  **compteurs seuls**, contexte `UE5.5.4/ARM-CLEAN06.4F`.
  SUCCESS signifie « comparaison produite », pas « tout égal ».
- Un écart tardif reste bloquant. Aucune écriture, aucune suppression,
  aucun SaveGame converti. Lire
  `ARM_CLEAN06_4F_THREE_WAY_USER_AUDIO_PROVENANCE.md`.

## ARM-CLEAN06.4G — photographie et restauration binaire en bac à sable

- Source 06.4F validée : quatre définitions Sword présentent les huit
  champs audio modifiés dès 06.2A, **sans dérive** ultérieure ;
  06.4E certifie leur parité canonique strictement actuelle.
- L'agent Windows construit une **liste fermée** :
  les 56 fichiers des reçus de 06.4A + les 28 fichiers réellement
  versionnés sous `CanonicalStage` (registre, quatre catalogues,
  23 définitions). Toute apparition, disparition, LFS pointer,
  altération de hash ou fichier hors préfixe refuse le lot.
- Relance `GridArmClean06Baseline` dans UE5.5.4 : vérifie les
  UPROPERTY, graphes AssetRegistry et 1640 profils de 06.4A.
- Sauvegarde les 84 vrais binaires dans un dossier **privé** de l'agent,
  vérifie SHA-256 avant et après chaque copie, puis simule
  la restauration **uniquement dans un autre dossier privé**.
- `UE5.5.4/ARM-CLEAN06.4G` publie le statut du SHA exact ;
  ticket GitHub `[ARM-CLEAN06.4G]` à compteurs seuls.
- La restauration du vrai projet n'est **pas** exécutée, et aucune
  écriture .uasset/.umap/.sav n'est autorisée. Les SaveGames
  personnels ne sont ni recopiés ni ouverts.
- Livrable `docs/Architecture/ARM_CLEAN06_4G_BINARY_TRANSACTION_GUARD.md`.
  Suite : audit exhaustif d'identités, niveaux, jets et références
  puis transaction Editor-only explicite avec sauvegardes prouvées.

## ARM-CLEAN06.4H — inventaire des identités monde et schéma SaveGame

- Source 06.4G validée : sauvegarde privée et restauration simulée
  des 84 binaires, SHA-256 de chaque fichier, sans aucune écriture
  projet ni SaveGame (issue 140).
- Commandlet **Editor-only** `GridArmClean064HReferenceAudit` :
  charge les quatre `UGridLevelAsset`, la vraie palette,
  les définitions de monde `UGridWorldObjectDefinitionAsset`
  répertoriées et les 23 définitions du vrai `DA_ItemSystem`.
- Inventorie les objets physiques `LooseItemInstances`,
  générateurs `ItemSpawns`, `ReceptacleInitialContent`,
  les `DefaultItemDefinition` de palette et les contenus
  initiaux par défaut des définitions d'objets. Enregistre
  les `ItemId`, niveaux, quantités, GUID et pointeurs exacts
  **dans le rapport privé uniquement**.
- Contrôle par domaine les candidats canoniques, identifiants
  sans correspondance, IDs vides, niveaux ou quantités invalides
  et archétypes incompatibles ; un candidat d'identité n'est
  PAS une preuve de parité comportementale ou de conversion.
- Examine **uniquement le schéma C++** SaveGame : les deux
  pointeurs legacy `ProjectileDefinitionAsset` et
  `ItemDefinitionAsset` subsistent ; **aucun .sav ouvert**.
- Le script rehash les 84 binaires locaux et leurs copies privées
  06.4G avant l'audit ; contrôle la propreté Git après.
  Statut `UE5.5.4/ARM-CLEAN06.4H` sur le SHA et ticket
  `[ARM-CLEAN06.4H]` à compteurs agrégés uniquement.
- Migration, suppression et mise à jour de sauvegardes **interdites**.
  Lire `ARM_CLEAN06_4H_WORLD_REFERENCES_READ_ONLY.md`.

## ARM-CLEAN06.4I — distinguer 40 occurrences des ItemId manquants

- Source 06.4H validée : 95 références physiques réelles (55 IDs
  candidats et **40 occurrences non mappées**), zéro GUID dupliqué,
  quatre niveaux, 78 entrées de palette, 44 définitions monde ;
  issue GitHub 141.
- Commandlet Editor-only `GridArmClean064IIdentityGap` : relit les
  **95 lignes privées exactes**, charge les `UGridItemDefinitionAsset`
  correspondants, recoupe l'identité/l'état N0–N20 et les
  23 définitions sérialisées dans `DA_ItemSystem`.
- Regroupe les **40 références** par **ItemId distinct**, inventorie
  les définitions historiques associées et compare **toutes leurs
  UPROPERTY** pour détecter les duplications contradictoires.
  Les valeurs complètes (SFX/VFX, meshes, slots, jets, poids, tags...)
  restent dans les rapports privés.
- Sur les 55 candidats canoniques, compare uniquement quelques
  métadonnées *sélectionnées* (DisplayName, Description, Weight,
  Tags, classe/archétype, Icon et WorldMesh non-ARM).
  Ceci **n'est pas une preuve d'équivalence générale**.
- Le script revalide le verrou 06.4H, le reçu 06.4G et
  les SHA-256 des **84 sources plus sauvegardes privées**.
  Issue publique `[ARM-CLEAN06.4I]` à chiffres agrégés.
- `UE5.5.4/ARM-CLEAN06.4I=success` = inventaire exact produit,
  pas migration autorisée. **Zéro écriture d'asset ou SaveGame**.
  Voir `ARM_CLEAN06_4I_DISTINCT_IDENTITY_GAPS.md`.

## ARM-CLEAN06.4J — projections non-ARM transitoires en mémoire

- Parent 06.4I validé sur `d3fc4e1`, issue 142 : **11 ItemId
  distincts** manquants dans `DA_ItemSystem`, **40 occurrences**
  physiques, **zéro** conflit de définition historique, **zéro**
  arme manquante ; 95 placements en N0.
- Le commandlet UE Editor-only
  `GridArmClean064JNonArmProjection` relit les **11 exports complets
  d'UPROPERTY** privés et les confronte aux véritables `DA_Item_*`.
- Pour chacun, déduit **une famille candidate** depuis `ItemType`
  et les flags de gameplay, construit un `UObject` **transitoire**
  de la classe canonique adéquate et vérifie nom, ID, description,
  poids, tags, icône et mesh. Le cas munitions **n'est jamais
  déduit automatiquement** d'un nom/tag.
- Comptabilise tous les anciens champs **non par défaut et sans
  propriétaire canonique démontré**, plus les verrous spécifiques
  (consommables/actions, équipement, lumière, interaction,
  balistique/Chaos, réceptacles et SaveGame).
- `Scripts/AuditARM_CLEAN06_4J_NonArmTransient.ps1` exige les preuves
  privées de 06.4I, le verrou d'écriture fermé et les 84 binaires
  d'origine + copies 06.4G vérifiées SHA-256.
- Rapport privé `nonarm-projection-private.json`, ticket
  `[ARM-CLEAN06.4J]` à compteurs agrégés et statut Windows
  `UE5.5.4/ARM-CLEAN06.4J` sur le SHA exact.
  **SUCCESS = simulation terminée, pas authoring autorisé.**
- Aucun asset, niveau, palette ni SaveGame sauvegardé ;
  `migration_authorized=false` et listes d'écriture vides.
  Voir `ARM_CLEAN06_4J_NONARM_TRANSIENT_PROJECTION.md`.

## ARM-CLEAN06.4K — propriétaire des 50 propriétés legacy non-armes

- Source 06.4J validée : **11** identités absentes, **40** occurrences,
  **11/11** projections temporaires exactes pour les champs communs,
  **50** propriétés historiques actives sans destination prouvée
  (1 consommable candidat, 9 génériques, 1 non classifiée), issue 143.
- Le commandlet Editor-only `GridArmClean064KNonArmFieldOwnership`
  recharge les sources réelles, compare **toutes les UPROPERTY**
  avec les preuves privées 06.4I, puis vérifie les 50 champs
  non par défaut reportés par 06.4J.
- Chaque champ entre dans **un domaine explicite de revue** :
  lecture, lumière, FX persistant, physique Chaos, lancer générique
  (non-ARM), actions, équipement, empilement, legacy armement,
  ou non classifié. Aucune catégorie ne constitue une autorisation
  de conversion ni une preuve de parité.
- Les règles de balistique **non-armement** ne doivent pas être
  transférées vers le `WeaponCatalogue.ProjectilePolicies`, qui
  concerne les armes. Les textes lisibles et Niagara persistant
  exigent également une autorité spécialisée.
- Le PowerShell `AuditARM_CLEAN06_4K_NonArmFieldOwners.ps1`
  contrôle les preuves privées 06.4I/4J, rehash les 84 binaires et
  leurs sauvegardes privées 06.4G, puis lance le commandlet UE.
- Publication du statut `UE5.5.4/ARM-CLEAN06.4K` et du rapport
  GitHub `[ARM-CLEAN06.4K]` **agrégé uniquement**.
  Exactes valeurs/champs/identités conservés sur Windows.
- `migration_authorized=false`, **0 écriture de .uasset/.umap/.sav**,
  aucune modification du jeu. Voir
  `ARM_CLEAN06_4K_NONARM_FIELD_OWNERSHIP.md`.

## Reprise recommandée — ARM-CLEAN06.4L (après 06.4K validé)

- Preuve publique 06.4K : issue
  https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/144 ;
  11 ItemId non-armement manquants, 40 occurrences monde,
  **50 occurrences de propriétés actives**, **26 noms distincts**,
  aucune autorité totalement approuvée, zéro écriture autorisée.
- Répartition vérifiée : lecture **1**, lumière **1**, présentation
  persistante **11**, physique/Chaos **2**, lancer générique **3**,
  actions d'objet **11**, équipement hors ARM **15**, empilement
  hors consommable **6**, ancien catalogue ARM **0**, non classé **0**.
- **Prochain travail 06.4L :** lire les rapports privés existants dans
  l'agent Windows avec preuve SHA ; établir les destinations
  sémantiques *champ par champ*, en priorité pour les domaines dont
  l'autorité canonique n'existe pas encore, puis réaliser des
  comparaisons sur objets Unreal transitoires et tests comportementaux.
  Un domaine de classement ne suffit **jamais** à valider une conversion.
- Conserver distinctes l'autorité des lancers **non-armes** et celle
  des projectiles des armes : ne pas réutiliser abusivement
  `DA_WeaponCatalogue.ProjectilePolicies`. Pas d'inférence depuis le
  nom d'un objet, une icône ou un tag ; ne pas perdre le N0.
- 06.4L reste **bloqué pour les écritures binaires** tant que
  lecture, lumière, FX persistants, physique, interactions, équipements,
  empilement, références monde et schéma SaveGame n'ont pas leurs
  preuves. Garder le snapshot 06.4G (84 sources + 84 copies privées)
  vérifiable ; aucun `SavePackage`, aucun `.sav` modifié.
- Les validations automatiques `-NullRHI` ne prouvent ni les clics UMG,
  ni Chaos/PIE, ni un Cook. Garder ces recettes pour la phase 06.6.
  Documentation détaillée :
  `ARM_CLEAN06_4K_NONARM_FIELD_OWNERSHIP.md`.

## ARM-CLEAN06.4L — contrat présenté au contrôle UE (statuts à vérifier)

- Sujet exact attendu : `feat(ARM-CLEAN06.4L): preserve nonarm light and persistent visual data in transient canonical projection`.
- 06.4K et ses trois statuts SUCCESS sont le parent de preuve ; il a
  distingué **50** occurrences actives : lumière **1**, présentation
  persistante **11**, les autres domaines **38**.
- La classe canonique `UGridNonArmamentDefinitionAsset` reçoit les
  huit champs lumière/VFX/scintillement présents sur les sources legacy.
  Aucune nouvelle autorité n'entre dans les armes ou leur catalogue.
- `GridArmClean064LNonArmPresentationParity` doit copier **88** valeurs
  de propriétés dans **11** objets transitoires UE et constater
  l'identité exacte, y compris les défauts, ainsi que les prédicats purs.
- L'agent Windows doit publier pour ce commit :
  `UE5.5.4/SilentTask`, `UE5.5.4/LocalAuto`,
  `UE5.5.4/ARM-CLEAN06.4L`.
  **Ne déclarer 06.4L validé qu'après trois SUCCESS sur son SHA exact**.
- Des tests PIE/Chaos/SaveGame restent nécessaires : le commandlet n'a
  produit qu'une parité de *données* ; aucune conversion `.uasset`,
  `.umap` ni `.sav`. Les 38 autres propriétés sont encore bloquantes.
- Document détaillé :
  `docs/Architecture/ARM_CLEAN06_4L_NONARM_PRESENTATION_DATA_PARITY.md`.
- Suite envisagée : 06.4M, traitement des politiques non-ARM encore
  sans propriétaire et contrôles comportementaux, sans migration hâtive.

## ARM-CLEAN06.4M — données de lecture et de physique non-ARM

- 06.4L contrôlé : `a279761d52fba8d8a5280104e009bd336cb6c2a1`,
  trois SUCCESS, issue 145 ; aucun binaire migré.
- Scope 06.4M : `ReadText` (**1** champ actif) et opt-in
  masse/tilt Chaos (**2** champs actifs), en conservant le noyau
  canonique à cinq champs et les politiques non-ARM spécialisées.
- Une capacité explicite `bReadable` conserve livres/parchemins
  même sans texte ; les overrides de texte restent des états d'instance.
- La projection UE temporaire prévoit **33 copies réfléchies** des
  trois propriétés sur 11 sources et **77 contrôles purs**
  (44 lectures et 33 règles de masse/tilt), sans `SavePackage`.
- Le vrai runtime continue d'utiliser le legacy tant que les
  11 `ItemId` et les 40 placements n'ont pas été convertis en UE.
- L'agent doit publier `UE5.5.4/SilentTask`,
  `UE5.5.4/LocalAuto` et `UE5.5.4/ARM-CLEAN06.4M` sur le
  **SHA du commit 06.4M**. Aucune validation anticipée.
- Les autres **35** occurrences actives restent à auditer :
  lancer non-ARM 3, actions 11, équipement non-ARM 15, empilement 6.
- Voir `docs/Architecture/ARM_CLEAN06_4M_NONARM_READING_WORLD_PHYSICS_PARITY.md`.
  Prochaine phase recommandée : 06.4N, lancer physique non-armement.

## ARM-CLEAN06.4N — lancer générique non-ARM, projection en mémoire

- Parent 06.4M validé sur `8d925f186e02f419c3b5087f7df8f8f848548552`,
  trois contrôles SUCCESS, issue 146.
- 06.4N projette les **3 propriétés actives** du domaine
  `GENERIC_THROW` sans inférer de nouvelle action.
- `FGridNonArmamentThrowPolicy`, propriété de
  `UGridNonArmamentDefinitionAsset`, conserve 12 paramètres
  (balistique, collision/retombée, apparence du mesh, rotation).
  Aucune modification de `DA_WeaponCatalogue.ProjectilePolicies`.
- Commandlet `GridArmClean064NNonArmGenericThrow` : 11 objets
  transitoires sur les sources réelles, **132 propriétés** correspondantes,
  **121 copies exactes** et **11 modes enum** mappés explicitement,
  **77 sondes pures** (33 balistique, 44 visuel).
- Les identités et valeurs privées ne sont jamais publiées. 84 sources
  et leurs copies immuables vérifiées SHA ; sauvegardes non lues.
- **À vérifier sur SHA 06.4N :** `UE5.5.4/SilentTask`,
  `UE5.5.4/LocalAuto`, `UE5.5.4/ARM-CLEAN06.4N` tous SUCCESS ;
  ce jalon ne vaut qu'une parité de données, pas un PIE/Chaos/cutover.
- Autres **32** occurrences actives restant à projeter : actions 11,
  équipement hors ARM 15, empilement 6 ; ne pas convertir les assets
  avant les contrôles d'interaction, SaveGame et runtime.
- Document détaillé :
  `docs/Architecture/ARM_CLEAN06_4N_NONARM_GENERIC_THROW_PARITY.md`.
  Suite prévue : 06.4O, actions non-ARM, puis équipements et empilement.

## ARM-CLEAN06.4O — inventaire des actions non-ARM (lecture seule)

- Parent 06.4N validé sur `99bd3afc3207830c76542078eaba417dcbbc9b63`,
  issue 147 et les trois statuts SUCCESS.
- Objet du jalon : **11** occurrences d'UPROPERTY `ITEM_ACTION`
  actives sur 11 ItemId/40 placements. Conserver leurs détails réels
  avant de proposer des liens au `DA_ActionCatalogue`.
- Commandlet Editor-only `GridArmClean064ONonArmActionSemantics` :
  inspecte les vrais `CombatActions`, QuickItems,
  présentations, validités, champs non par défaut et collisions d'IDs ;
  produit une preuve Windows privée, pas de migration.
- Contrôles : provenance 06.4K et 06.4N exactes, 84 binaires et
  84 copies vérifiés, **zéro mutation** d'assets/SaveGames.
- Requiert `UE5.5.4/SilentTask`, `UE5.5.4/LocalAuto` et
  `UE5.5.4/ARM-CLEAN06.4O` SUCCESS sur son SHA.
- Même avec SUCCESS, ne déclarer **aucune** parité sémantique
  des actions. Les **32** champs actifs restants comprennent
  actions 11, équipement hors ARM 15, empilement 6.
- Voir `docs/Architecture/ARM_CLEAN06_4O_NONARM_ACTION_SEMANTIC_INVENTORY.md`.

## ARM-CLEAN06.4P — équipements non-ARM, projection avec invariants de main

- Source validée : 06.4O `20f641621cff0f2db0c12a94f9ff38176f0ca666`,
  trois SUCCESS et issue 148. Les 11 propriétés d'actions restent en revue.
- 06.4P traite les **15 occurrences actives** du domaine
  `NONARM_EQUIPMENT` parmi les 11 ItemId/40 placements réels.
- `UGridOtherEquipmentDefinitionAsset` reçoit
  `HandPolicy` explicite, `Resistances` et `EquippedMesh`,
  sans dupliquer `EquipmentSlots` ou `Bonuses` existants.
- Le commandlet `GridArmClean064PNonArmEquipmentParity`
  vise **44 copies strictes**, **11 politiques de main résolues**,
  **209 vérifications des emplacements**, **11 listes UI**, **33
  sondes de bonus N0/N1/N20** et **11 replis de mesh**.
- 84 sources et 84 copies privées rehashées, 50 valeurs actives
  recontrôlées. Aucune écriture binaire et aucun SaveGame ouvert.
- Attendre `UE5.5.4/SilentTask`, `UE5.5.4/LocalAuto` et
  `UE5.5.4/ARM-CLEAN06.4P` tous SUCCESS **sur le SHA de 06.4P**.
  Un succès signifie parité en mémoire, pas test PIE ni cutover.
- **17 champs** resteraient ouverts après validation des données :
  actions 11, empilement 6. Suite : 06.4Q pour empilement, puis
  mapping complet d'actions et régression Save/Continue.
- Documentation :
  `docs/Architecture/ARM_CLEAN06_4P_NONARM_EQUIPMENT_PARITY.md`.

## ARM-CLEAN06.4Q — pile générique et véritable registre UE

- Parent 06.4P validé : `29955df612e1478b94f30a706bac009df3c8d531`,
  3 statuts Windows SUCCESS, issue 149.
- Propriété active du domaine `NONARM_STACK` : **6 occurrences**
  de `bStackable` et/ou `MaxStackSize`. Ne pas confondre avec
  six ItemId ; sources : 11 ItemId non-ARM et 40 placements.
- `UGridGenericStackableDefinitionAsset.MaxStack` est une autorité
  distincte pour les piles *génériques* ; le `MaxStack` existant des
  munitions et consommables reste leur seule autorité. Le booléen
  de pile legacy se résout en capacité, pas en second champ.
- Commandlet `GridArmClean064QNonArmStackParity` : vrais assets UE,
  22 valeurs source, 6 actives, **34 définitions transitoires**,
  `UGridItemSubsystem` réellement enregistré, **44 sondes
  N0/N1/N20/N21** et **33 tests de quantités**.
- **Ne valider 06.4Q qu'avec :** `UE5.5.4/SilentTask`,
  `UE5.5.4/LocalAuto` et `UE5.5.4/ARM-CLEAN06.4Q`
  SUCCESS sur son SHA exact. Un succès n'approuve **aucune**
  conversion de niveau, SaveGame ou choix de famille réelle.
- Suite **resserrée** : 06.4R pour décider tous les mappings
  sémantiques des 4 actions et de la présentation observées en
  06.4O, puis migration Editor contrôlée, vérification et cutover.
  **Éviter de prolonger les micro-audits** pour des champs déjà
  couverts par preuve transitoire. La migration ne commence qu'avec
  le schéma SaveGame, les références monde et le rollback validés.
- Détails : `docs/Architecture/ARM_CLEAN06_4Q_NONARM_STACK_REGISTRY_PARITY.md`.

## ARM-CLEAN06.4R — action complexe non-ARM sans perte de champ

- Parent : 06.4Q validé sur `608a83c76205cfc126088fc19f36d5f0e2e173aa`,
  issue 150 et trois statuts SUCCESS.
- Source : 06.4O a inventorié **11 occurrences ITEM_ACTION**,
  **1** action équipement, **3** QuickItems normalisées,
  **1** présentation et **54** champs imbriqués non défaut.
- Nouveau pont **dans les catalogues uniques existants** :
  `DA_ActionCatalogue.DetailedActions` contient le vrai
  `FGridCombatActionDefinition` intégral, et
  `DA_ActionPresentationCatalogue.DetailedPresentations` la
  vraie `FGridPlayerAttackPresentationProfile`. Les IDs
  sont disjoints de leurs entrées compactes. AUCUN cinquième catalogue.
- Des `ActionBindings` optionnels sont ajoutés à la définition
  non-ARM, mais ne sont **pas activés** par R. Les anciennes
  `UseActions` des consommables ne sont pas dupliquées.
- Le commandlet `GridArmClean064RNonArmFullActionBridge`
  projette les 11 vrais ItemId dans un root de 34 entrées,
  vérifie les 50 champs actifs et chaque FProperty des
  **4 actions** contre la capture 06.4O ; conserve la présentation.
- Interdits : aucun `SavePackage`, aucune bascule runtime,
  aucun nouveau contenu joueur. Les quatre liens détaillés
  restent **non approuvés** tant que leurs routes n'ont
  pas été testées sur l'exécuteur et l'UMG.
- Vérifier sur SHA R : `UE5.5.4/SilentTask`,
  `UE5.5.4/LocalAuto`, `UE5.5.4/ARM-CLEAN06.4R`
  tous SUCCESS. Ne jamais inférer la parité PIE des comptes.
- **Suite resserrée : outil de migration Editor transactionnel,
  activation des bindings réels, tests Save/Continue, puis 06.5/06.6.**
  Ne pas rouvrir un audit de chacune des familles déjà couvertes.
- Document :
  `docs/Architecture/ARM_CLEAN06_4R_NONARM_ACTION_LOSSLESS_BRIDGE.md`.

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
