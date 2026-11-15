# ARM-CLEAN06.4S — Transaction Editor fermée, manifeste et retour arrière privé

## Point de départ validé

- **06.4R** : SHA `37cf07dc21359a9762c6dd244eebe74bba02bc41`, trois statuts Windows **SUCCESS**, rapport [#151](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/151).
- **06.4G** : SHA `e0f877d17f39d913c585b55ba18110745c5385b6`, **84 sources binaires + 84 copies privées immuables** et exercice de restauration privée.
- **06.4K / 06.4O / 06.4Q** : 11 `ItemId` non-ARM distincts absents des 23 définitions existantes, 40 références monde/palette/réceptacle, 4 actions complètes dont 3 QuickItem, 1 présentation, données de pile préservées en mémoire.
- **Contrats obligatoires :** `ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md`, `ARM_CLEAN06_MIGRATION_PLAN.md` et `AGENTS.md`.

## Objet précis

**06.4S n'est pas un convertisseur et n'effectue aucune écriture Unreal.** Il assemble et exerce une **transaction fermée avec manifeste de 84 fichiers exactement**, à l'intérieur du dossier privé du validateur Windows. Le rapport formel distingue trois opérations qui ne doivent jamais être confondues :

| Opération | 06.4S | Après autorisation spécifique |
|---|---|---|
| Vérifier le SHA de chaque vrai binaire et de son backup 06.4G | Oui, 84 paires | À refaire immédiatement avant écriture |
| Copier 84 sources dans un bac à sable privé | Oui | Préparation de migration dans un workspace isolé |
| Détecter une mutation et restaurer depuis la copie immuable | Oui, uniquement sur **une copie** dans le bac à sable | Un rollback de production requiert un outil, un manifeste et des assertions distincts |
| `SavePackage` des vrais `.uasset` / `.umap` | **Non** | Seulement une fois conversion, parité et allowlist approuvées |
| Lire ou convertir les `.sav` utilisateurs | **Non** | Protection/compatibilité SaveGame dédiées, sans altérer l'original |
| Modifier ou resauvegarder les WBP, presets et palettes périphériques | **Non** | Nouvel inventaire et backup de toute ressource touchée |

Le script `Scripts/AuditARM_CLEAN06_4S_TransactionRehearsal.ps1` est une commande **sans paramètre `-Apply` et sans code de mutation du projet**. Le déclencheur exact de `UEValidationAgent.ps1` est :

`chore(ARM-CLEAN06.4S): rehearse sealed UE asset transaction without writes`

## Conditions préalables et modèle de transaction

1. Validation depuis le worktree Windows dédié **détaché**, `HEAD` égal au SHA du commit S et `git status` totalement propre.
2. Relecture des reçus 06.4K et **06.4R** : SHA signés, 11 `ItemId` distincts, 40 références, 4 actions complètes, 1 présentation, 176 exports de propriétés comparés et 54 champs non défaut ; **0 route runtime approuvée** et toutes les portes précédentes fermées. Les éventuelles captures R répétées sont acceptées uniquement si les reçus du SHA final sont valides et si les rapports approuvés sont identiques bit à bit.
3. Rehash de l'ensemble **84 sources binaires et leurs 84 sauvegardes privées**, depuis le manifeste figé 06.4G : 56 `legacy` et 28 `canonical`. Chaque entrée possède un chemin relatif, un SHA-256, une taille et une catégorie. Aucun chemin absolu fourni par une source non fiable, aucune traversée `..`, aucun lien symbolique.
4. Manifestation privée `transaction-plan-private.json` : 84 lignes, statut **`REVIEW_ONLY`**, cible non renseignée, `write_authorized=false`, liste blanche de sortie **vide**. Aucun chemin de sortie réelle ne peut être inventé à partir d'un nom, tag ou icône.
5. Copie des 84 fichiers **dans `<StateRoot>/Runs/<run>/ARM-CLEAN06.4S/TransactionSandbox` seulement**. Comparaison des tailles et hash de la source du projet, de la copie 06.4G et de la copie temporaire.
6. Corruption contrôlée d'**un octet ajouté à une seule copie de test privée** : le détecteur doit constater l'écart. Restauration depuis le backup 06.4G, puis contrôle SHA-256 de **84/84** copies, sources et backups.
7. Suppression du seul dossier `TransactionSandbox` créé par cette exécution, **jamais du snapshot 06.4G, des vrais assets ou des SaveGames**. Vérification finale du worktree propre.
8. Écriture du reçu privé `transaction-rehearsal-gate-private.json` avec les SHA des preuves sources et du plan, la restauration attestée, ainsi que `migration_authorized=false` et trois listes d'écritures vides.

Les chemins complets de packages, noms d'objets, valeurs UPROPERTY, états ou GUID restent exclusivement dans le stockage privé du PC Windows. Le ticket GitHub n'expose que des comptes.

## Verrous qui doivent encore être levés

Ces cinq conditions sont **volontairement bloquantes**, même lorsque la répétition du retour arrière fonctionne :

1. **Choix définitif des familles par `ItemId` :** les 11 nouveaux `ItemId` et les 40 références ne doivent pas être reconstruits à partir d'une simple capacité de pile ; les bonus, mains, luminosité, physique, lecture et actions réelles doivent conserver le bon propriétaire.
2. **Action/routage réellement exécutable :** quatre profils intégraux existent seulement **en mémoire**. Il reste à associer `ActionId` et `PresentationId` aux vrais DataAssets et à éprouver les refus avant PA/consommation, HUD/hotbar, ciblage et cooldowns.
3. **Sauvegardes réelles :** le schéma comporte encore deux champs pointeurs d'assets historiques (`FGridRuntimeItemState`, `FGridPendingWorldItemState`). Aucune sauvegarde personnelle n'a été ouverte ni sauvegardée par les étapes 06.4G–S. Prévoir une sauvegarde immuable et une lecture/écriture de test sur **copies**, en conservant `RuntimeObjectId`, `ItemLevel` (N0 compris), `Quantity`, états mutables et données d'énigmes.
4. **WBP/presets non couverts :** les 84 binaires ne sont pas la totalité des assets pouvant référencer des objets. Les dépendances directes/indirectes et les ressources hors liste doivent être classées et sauvegardées **avant** autorisation de leur écriture.
5. **Recette post-conversion :** comparer AssetRegistry, niveaux, spawn/réceptacles et placements, tester PIE/Chaos et Cook/Shipping ; `-NullRHI` ne suffit pas.

## Conditions de succès et suite concrète

Exiger sur **le SHA final S** :
`UE5.5.4/SilentTask=SUCCESS`,
`UE5.5.4/LocalAuto=SUCCESS` et
`UE5.5.4/ARM-CLEAN06.4S=SUCCESS`.

**Succès S = sécurité du manifeste/retour arrière privé, pas migration effective.**

**06.4T** doit produire le véritable outil Editor-only, avec sélection explicite des familles et des sorties, mutations de données *transitoires* vérifiées, mode `-DryRun` non destructif et transaction binaire explicitement opt-in **uniquement lorsque** les cinq conditions ci-dessus ont été satisfaites. Si une dépendance est encore non couverte, garder la porte fermée et annoncer l'obstacle réel plutôt que lancer une conversion partielle dangereuse.

Aucun changement UI : `UI.csv → ST_UI → WBP → Game/fr/en` inchangé.
