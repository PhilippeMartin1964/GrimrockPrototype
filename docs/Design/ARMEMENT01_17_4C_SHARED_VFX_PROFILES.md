# ARMEMENT01.17.4C.1 — Profil VFX réutilisable et protection de FireFX

## Choix d'architecture

Un objet ou un effet partage une définition de présentation. Le niveau stocke
les placements et, si nécessaire, une **référence** à une variante de profil,
mais plus l'intégralité des réglages de flammes pour chaque donjon.

- `UGridSurfaceVfxProfileAsset` est un DataAsset qui contient les réglages
  Niagara, flammes, fumée, éclairage et onde du catalyseur.
- `AGridLevelRuntimeActor.DefaultSurfaceVfxProfile` sélectionne le profil
  commun aux niveaux.
- `UGridLevelAsset.SurfaceVfxProfile` sert seulement à une **exception**
  par niveau (ex. flammes bleues dans un temple).
- L'ancien `UGridLevelAsset.SurfaceVfxSettings` est conservé sous la catégorie
  *Legacy*, ainsi que les anciens champs du runtime : pas de suppression de
  données ni de modification d'assets existants.

Ordre de résolution : choix explicite du niveau → anciens réglages du niveau
déjà activés (compatibilité 17.4B) → profil partagé du runtime → anciens
réglages du runtime. Les paramètres VFX ne changent jamais les dégâts ou les
durées des surfaces RPG03.6.

## FireFX local, pas GitHub

Le dossier acheté est installé **sur la machine de l'utilisateur**
dans `Content/FireFX/`. La connexion GitHub ne permet pas de lire les
uassets du disque local ni leurs véritables noms de systèmes Niagara.
Ne jamais inventer un nom de système ni prétendre avoir validé FireFX sous
UE 5.5.4. Le profil fonctionne avec n'importe quel Niagara compatible.

Le fichier .gitignore protège maintenant explicitement :

- `/Content/FireFX/`
- `/Content/GrimrockPrototype/LocalVFX/`

Créer un `UGridSurfaceVfxProfileAsset` dans le dossier LocalVFX depuis
l'éditeur Unreal, puis attribuer le véritable Ground Fire du dossier FireFX
et renseigner les User Parameters vérifiés sur place. Le profil local
et le contenu commercial restent hors GitHub.

**Important** : gitignore ne protège pas un fichier *déjà suivi*. Vérifier
`git ls-files Content/FireFX` et `git ls-files Content/GrimrockPrototype/LocalVFX`
avant le moindre push. Un niveau ou BP déjà versionné peut aussi
sérialiser une référence à ces assets locaux : ne pas pousser
les références locales dans les assets partagés/canonicalisés.

## Validation

- Test `Grimrock.RPG.ARMEMENT01.17.4C.SharedVfxProfile` :
  profil commun, profil d'exception, ancien format et retour au fallback.
- Suite Unreal Development Editor UE 5.5.4, avec `UE5.5.4/LocalAuto`.
- PIE/RHI et inspection du pack local indispensables pour valider
  réellement les flammes et les User Parameters.

## Suite — objets placés

L'inspecteur du Dungeon Editor possède déjà des *instance overrides*
pour les mécanismes. L'extension suivante ajoutera un override Niagara
fin aux objets génériques placés, sans copier tous les paramètres du
profil ni modifier les définitions de tous les objets.

## ARMEMENT01.17.4C.2 — Exception Niagara sur l'objet placé

Un objet générique placé dans le Dungeon Editor hérite de son
UGridWorldObjectDefinitionAsset (StaticPart.ActiveNiagaraSystem et
ActiveNiagaraLocalTransform). Dans l'inspecteur **Visual Effects — Placed
Object**, les exceptions sont **facultatives**, selon les cases cochées :

- **Override active Niagara system** : choisir un effet Niagara de l'instance,
  ou laisser vide pour désactiver l'effet uniquement sur cet exemplaire.
- **Override Niagara scale multiplier** : multiplier l'échelle de la
  définition (0.01 à 5), sans muter l'asset partagé.
- **Override local offset** : ajouter X, Y, Z en centimètres dans le repère de
  l'objet, sans altérer la définition ou les autres exemplaires.

Les valeurs sont sérialisées sur FGridWorldObjectInstanceConfig.NiagaraOverrides,
puis transférées dans FGridRuntimeWorldObjectData. Une résolution pure dans
GridWorldObjectInstanceVisual applique uniquement les champs cochés.
Le runtime AGridGenericObjectActor utilise le résultat sans ajouter de
référence obligatoire au fournisseur FireFX. Les règles de jeu et les états
actif/inactif existants sont conservés.

**Portée initiale** : objets du type RuntimeActorClass dérivé de
AGridGenericObjectActor. Les mécanismes spécialisés et les objets ramassables
FGridLooseItemInstance utilisent d'autres chaînes de présentation ; ne pas
leur attribuer d'override tant que leurs chemins runtime n'ont pas été
explicitement adaptés et testés.

Une référence vers Content/FireFX sur un exemplaire du niveau ou vers
LocalVFX dans un asset suivi par Git reste une **référence locale**. Si
l'asset du niveau est suivi par Git, contrôler les dépendances avant push ;
un simple .gitignore sur le dossier FireFX ne supprime pas les références
sérialisées dans le .uasset du niveau.

Test ajouté : Grimrock.RPG.ARMEMENT01.17.4C.PlacedObjectNiagaraOverrides.
