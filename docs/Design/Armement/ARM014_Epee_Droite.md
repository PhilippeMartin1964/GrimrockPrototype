# ARM014 — Épée droite

## INSPECTION02.3B — Premier choix artistique (27.11.2026)

Base technique et recette utilisateur : `34e4afdf25b6178bbc43a90606502456092f3212`. Atelier et comparateur acceptés. Cette fiche est ouverte conformément à [DESIGN01](ARMEMENT_DESIGN01_FICHE_MAITRESSE.md) ; aucun changement d'asset ni de mécanique.

### Identité et ressources vérifiées dans le dépôt

- ItemId `Weapon_StraightSword_01`, ArchetypeId ARM014, fiche `DA_Weapon_StraightSword_01`. MainHand, attaque MeleeOneHand, selon la matrice FX01.
- Ressources présentes : `/Game/GrimrockPrototype/Meshes/Weapons/SM_Weapon_StraightSword_01`, `/Game/GrimrockPrototype/Art/Materials/Instances/Weapons/MI_StraightSword`, textures `Art/Textures/Final/Weapons/StraightSword/T_StraightSword_01_{BC,N,ORM}` et icône `UI/Icons/Weapons/T_Weapon_StraightSword_01`. La présence des fichiers ne prouve pas leur liaison effective, ni les valeurs natives des paramètres.
- Les tests de production `Grimrock.RPG.ARMEMENT.FX01.LevelFinishes` et `Grimrock.Items.Inspection.PresentationParity` vérifient deux épées droites de niveaux 5 et 17 sur le même mesh, des MID indépendants au slot 0, un tint distinct et le retour au matériau de base N1. Le code commun exige RoughnessMultiplier, RoughnessOverride, NormalStrength et BaseColorTint avant application d'une finition. Les noms et nombre de slots, leurs valeurs natives, le parent de MI, les références partagées et la couverture de chaque texture sont encore à confirmer dans l'éditeur natif.

### Premier essai proposé : rugosité uniquement

Conserver N1 tel quel. Isoler un seul canal : amélioration de l'état de surface par réduction progressive de la rugosité, sans modifier couleur, relief, metallic, mesh, gameplay ou effets. Premier essai N2 : multiplicateur de finition 0,85 par rapport à N1 (proposition, pas une valeur approuvée ni une roughness absolue). Observer N1/N2 en A/B synchronisé sous Studio puis Torche et Donjon. Si l'écart est insuffisant, essayer 0,70 comme borne de discussion, sans écrire dans le catalogue partagé. Contrôler si le paramètre affecte également cuir/bois : si le rendu exige une progression de la lame seule, vérifier les slots ou un masque existant avant d'étendre le matériau. Ne pas appliquer globalement un polissage inadapté à la poignée.

La démonstration doit être temporaire et isolée ; ne modifier ni les cinq finitions communes ni les 21 autres armes. L'authoring définitif de la progression fine et des paliers N4/5, N8/9, N12/13, N16/17 dépend du choix artistique et de cette preuve visuelle. Pas de 20 meshes, ni de nouveau catalogue, ni de gravures ou Niagara inventés à ce stade.

**Décision du propriétaire (27.11.2026) :** canal rugosité approuvé pour le premier essai ; valeurs 0,85 et 0,70 non approuvées définitivement. Les valeurs natives précises et le résultat GPU ne sont pas accessibles à distance par GitHub. Les relever à travers l'atelier ou les outils natifs existants avant modification définitive.

Sources : `docs/Design/ARMEMENT_FX01_ACTIVE22_CUE_MATRIX.csv`, `docs/Design/ARMEMENT_FX01_AUDIO_VFX_PROGRESSION_N1_N20.md`, `Source/GrimrockPrototype/Private/Runtime/Inspection/GridItemMeshPresentation.cpp`, tests cités ci-dessus. La fiche reste ouverte ; toutes les autres rubriques DESIGN01 seront renseignées au fur et à mesure des décisions et recettes, pas au moyen de valeurs inventées.


### Lot technique 02.3B — essai isolé dans l'atelier

L'atelier propose uniquement pour ARM014 deux essais temporaires (× 0,85 et × 0,70), avec A=N1 canonique et B=N2 résolu, puis copie de la finition N1 et multiplication de sa rugosité dans la projection B. L'essai réutilise le présentateur commun et ses MID indépendants, sans écriture d'asset, sans modifier les règles de résolution ou le catalogue. Les autres propriétés visuelles du premier niveau restent la référence ; les données mécaniques N2 ne sont pas modifiées. Les deux paramètres de rugosité existants sont multipliés suivant le contrat commun, pas remplacés par une roughness absolue. Le retour au rendu canonique, changement de niveau ou objet, actualisation des assets et fermeture de B annulent l'essai. Contextes Examen, Sol et Décor ; les trois éclairages restent disponibles. Les autres contextes annulent l'essai.

Le panneau affiche les valeurs natives/effectives des slots, leur source et parent, et le nombre de slots dynamiques en essai. Le pilote ne garantit aucune séparation lame/poignée : inspecter le matériau natif, les textures et les références partagées avant toute écriture définitive. Une variation sur la poignée ou une différence imperceptible bloque l'authoring permanent. Aucun masquage ni retouche de texture n'est ajouté arbitrairement.

Test `Grimrock.Editor.Items.Inspection.ARM014RoughnessTrial` : vrai mesh ARM014, comparaison des valeurs MID, indépendance, données mécaniques ciblées, retour au matériau canonique et garde sur l'identité/niveau. Validation Windows du commit requis ; aucune preuve de rendu GPU à ce stade.

**Recette artistique suivante :** sélectionner ARM014, cliquer Essai × 0,85 ; observer lame et poignée en A/B synchronisé, Studio/Torche/Donjon puis Sol/Décor. Essayer × 0,70 seulement si nécessaire, relever le slot et ses paramètres (et la présence d'un masque éventuel via le matériau natif). Cliquer Rendu canonique et vérifier la restauration. Ne pas authorer N3–20 avant cette recette.
