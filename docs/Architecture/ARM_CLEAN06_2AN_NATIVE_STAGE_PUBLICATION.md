# ARM-CLEAN06.2AN — Publication native isolée, contrôlée et unique

**But :** créer les DataAssets canoniques **réels** avec Unreal
Engine 5.5.4, au lieu de prolonger l'architecture transitoire C++.

## Déclenchement strict

Le validateur Windows déjà en place reconnaît **uniquement** le
sujet exact du commit
`feat(ARM-CLEAN06.2AN): enable one-shot guarded canonical asset authoring`,
après une compilation et une suite Automation `Grimrock` réussies
sur le SHA exact. Il lance `Scripts/PublishARMCanonicalStage.ps1`
dans le worktree de validation **détaché**, jamais le dossier
de développement de l'utilisateur.

Le script exige le manifeste exact **22 armes**, dont **19 attaques**
et **3 passifs**. Il refuse toute branche active, modification
initiale du worktree, destination existante, identité inattendue
ou origin Git non approuvé.

## Écriture Unreal et vérification

Le véritable commandlet
`GridArmClean06CanonicalStage -Apply` produit uniquement
**28 nouveaux `.uasset`** dans `CanonicalStage/` :
23 définitions (22 ARM du manifeste + flèches),
4 catalogues et `DA_ItemSystem`.
Il ne lit jamais les deux anciens fichiers hors manifeste.

Les 28 chemins doivent correspondre à une liste blanche construite
à partir des ItemIds du manifeste ; tous les fichiers doivent
être de taille valide et suivis par Git LFS. Un autre processus
UE charge ensuite les packages **depuis le disque** et vérifie
chaque `UPROPERTY` contre les candidats reconstitués.
La suite intégrale `Grimrock` est rejouée sans reconstruire
le C++. L'ancien runtime reste en service.

Après vérification : fetch `origin/master` et refus si le SHA
a changé, staging de **seulement ces 28 fichiers**,
vérification des pointeurs Git LFS et unique commit
`assets(ARM-CLEAN06.2AO)` poussé sans force vers `master`.
Le nouveau SHA doit passer ses validations Windows ordinaires.
Aucune API GitHub ne téléverse des binaires Unreal improvisés.

Si une génération échoue, seuls les fichiers **neufs et explicitement
attendus** du worktree détaché peuvent être copiés dans Recovery
avec SHA256, puis retirés du worktree, pour préserver les runs futurs.
Tout autre changement reste intact et bloque l'opération.

Le commit déclencheur n'est pas un cutover. Le jalon 06.3
doit encore remplacer les lecteurs historiques ; 06.4–06.5
convertir niveaux, palettes et SaveGame puis supprimer le legacy.
PIE visuel reste nécessaire après raccordement du runtime.
