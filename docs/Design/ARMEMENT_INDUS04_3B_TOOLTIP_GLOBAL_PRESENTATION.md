# ARMEMENT-INDUS04.3B — Présentateur global avant/après (C++ + UI.csv)

**État : C++ et UI.csv proposés pour validation locale.** Dépendance : ARMEMENT-INDUS04.3A validé 4/4 le 25 octobre 2026 ; WeaponTooltip04 8/8 ; Item01 5/5.

## Contrat

Le moteur \`PreviewEquipFromInventorySlot\` (04.3A) reste **l'autorité**.
\`FGridEquipmentTooltipGlobalProjection::BuildScenario\` transforme chaque simulation en \`FGridItemTooltipEquipmentComparison\` réutilisable par les deux WBP enfants natifs déjà créés. **Aucun nouveau WBP, aucun Graph, aucun Text Block ajouté**.

- Source **InventorySlot** réellement contenue dans le sac : le modèle du
  tooltip énumère les placements acceptés par \`CanEquipToSlot()\`, dans
  l'ordre MainHand, OffHand, puis autres emplacements. Une arme à deux mains
  ne génère **qu'un scénario** intitulé « Deux mains — conséquences
  globales ». Une arme à une main peut en générer deux, selon les règles
  effectives (même si le manifeste ne déclare que MainHand).
- Chaque scénario explique les **objets retirés** (noms, sans duplication
  de la main virtuelle d'une arme à deux mains) et l'**inventaire
  insuffisant**, s'il y a lieu. Un preview impossible peut rester affiché
  en lecture seule : il ne lance jamais d'équipement.
- Dans la comparaison, les colonnes sont **Après / Avant / Delta**,
  et non « objet candidat / équipement local seulement ». Les écarts
  numériques non nuls des attributs effectifs, PV/Mana max, capacité
  de charge, résistances et **armure restante effective** sont affichés,
  en réutilisant les couleurs sémantiques de \`Text_DeltaPositive\`,
  \`Text_DeltaNegative\`, \`Text_DeltaNeutral\`.
  Pour éviter le double comptage, \`ArmorBonus\` nominal n'est pas
  répété à côté de \`CurrentPhysicalArmor\`.
- Les profils offensifs \`WeaponFacts\` N1–N20 continuent à comparer
  la candidate avec l'arme occupant réellement la main ciblée, **si
  les deux profils sont comparables**. Il n'y a jamais de comparaison
  de dés fictive entre la Grande Hache et le bouclier : la disparition
  du bouclier est représentée par son impact d'armure et de résistances.
- Les vues d'objets équipés, de curseur, et les slots de test synthétiques
  non adossés à une vraie case du sac conservent la projection historique.
  Les futures opérations directes depuis le curseur nécessiteront une
  API de prévisualisation spécifique à leur règle de capacité.
- Aucun équipement réel, sauvegarde, inventaire, DataAsset ou règle
  RPG n'est modifié.

## Localisation

Toutes les nouvelles chaînes de la projection utilisent
\`FUILocalizationText::Get\` pour \`Inventory.Tooltip.Global.*\`.
**\`Localization/Source/UI.csv\` reste la seule source d'autorité.**
Le DataAsset \`ST_UI\` est généré sur le poste local par le pipeline de
localisation du projet et doit être sauvegardé par UE, pas modifié
directement dans GitHub. Ne pas écrire de \`LOCTEXT\` de repli pour
masquer une clé manquante.

Attention : cette passe introduit de nouvelles clés dans \`UI.csv\`.
**Avant les tests C++ de projection et toute recette PIE**, exécuter le
smoke de localisation (qui synchronise ST_UI), avec UE fermé :

    cd D:\Development\GrimrockPrototype
    git pull --ff-only origin master
    .\Scripts\Localization\RunLocalizationSmoke.ps1 -EngineRoot D:\UE_5.5

La synchronisation doit réussir, sans placeholder UMG. Les changements
locaux de \`ST_UI.uasset\` et éventuellement des archives sont attendus
et devront être examinés puis committés séparément après vérification.

## Validation C++ et PIE

Après smoke :

    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.UI.EquipmentTooltip043B"
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.UI.EquipmentPreview043A"
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.UI.WeaponTooltip04"
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.UI.Item01"

Attendu **3/3 + 4/4 + 8/8 + 5/5 = 20/20** et zéro avertissement,
avant validation visuelle en PIE.

Tests 04.3B :
- \`GreatAxeShieldAndBeforeAfter\` : deux objets réellement retirés ;
  Force +3 nette, armure -5, pas de mutation ;
- \`OneHandOffHandScenario\` : le pic à une main propose MainHand et
  OffHand même si la définition ne liste que MainHand, et le scénario
  OffHand montre -5 armure lorsque le bouclier est remplacé ;
- \`FullBagWarning\` : le tooltip montre une simulation bloquée avec
  avertissement localisé, sans effectuer l'équipement.

La présentation réelle doit être vérifiée **dans les WBP existants**
après ces tests. Le style, les colonnes, polices, 48x48 et bordures
personnalisées en 04.2C sont conservés. Pour les icônes manquantes,
le jalon 04.3C continuera d'utiliser la map \`StatIconTextures\`
de \`WBP_ItemTooltipNativeLine\`, pas un nouveau Graph.

## Risques assumés

Une même arme à une main peut provoquer **deux scénarios complets** ;
sur un petit écran le tooltip peut devenir long. Ne pas supprimer
les impacts : traiter l'ergonomie (navigation/section repliable)
dans une passe Designer ultérieure si nécessaire. Un utilisateur peut
voir les deltas offensifs sur le scénario MainHand et les pertes
d'armure sur OffHand, **jamais la somme illégitime des deux scénarios**.

ARMEMENT-INDUS04.3B n'est pas déclaré validé par le seul commit.
