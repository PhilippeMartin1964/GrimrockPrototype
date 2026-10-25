# ARMEMENT01.11.2.1 — ARM002 Poignard d'assassin : contrôle et référencement des visuels

Statut : code proposé sur master ; **aucun asset binaire créé ou modifié par ce commit, tests UE locaux non encore exécutés**. ARMEMENT01.11.1 est validé localement (TD04-20261024-230123, 2/2 tests, 0 warnings/failed). Son rapport CSV avait ARM002=MissingItemAndVisual **avant** la nouvelle production graphique.

## Ressources utilisateur à inspecter dans Unreal Editor

Six assets sont annoncés comme créés. Le commandlet exige les emplacements canoniques suivants (la présence locale et le nom ne suffisent pas à garantir qu'ils sont aux bons chemins) :

| Rôle | UE Asset | Content/GrimrockPrototype/... |
|---|---|---|
| Mesh | SM_Weapon_AssassinDagger_01 | Meshes/Weapons |
| Icône RGBA | T_Weapon_AssassinDagger_01 | UI/Icons/Weapons |
| Matériau | MI_AssassinDagger | Art/Materials/Instances/Weapons |
| BaseColor | T_AssassinDagger_01_BC | Art/Textures/Final/Weapons/AssassinDagger |
| Normal | T_AssassinDagger_01_N | Art/Textures/Final/Weapons/AssassinDagger |
| ORM | T_AssassinDagger_01_ORM | Art/Textures/Final/Weapons/AssassinDagger |

Contrôles manuels :
- Réexaminer la silhouette ARM002 (pas un clone d'ARM001), les UV, normales, axe et pivot, et une longueur cible d'environ 38 cm (proposition artistique, pas règle du catalogue).
- Vérifier que **MI_AssassinDagger est assignée dans un Material Slot du StaticMesh**, pas uniquement dans un acteur.
- Le matériau parent doit être valide et ses paramètres relier réellement les cartes PBR ; le test d'existence ne démontre pas le branchement.
- BC sRGB ON ; N et ORM sRGB OFF ; Normal en compression Normalmap ; ORM suivant la convention de canaux du parent.
- **T_Weapon_AssassinDagger_01 désigne l'icône**, pas la BaseColor. Elle doit être carrée, au moins 256×256, et lisible en 48×48.
- Ne pas supposer que ces .uasset locaux sont publiés sur GitHub ; préserver le travail de l'utilisateur.

## Code / contrats

Un commandlet `GridAssassinDaggerVisualRegistration` reprend le code déjà utilisé pour les Guerriers, **mais la table autorise seulement ARM002**.

Sans `-Apply` : diagnostics READY/BLOCKED/REGISTERED, aucun fichier écrit. Avec `-Apply -Archetype=ARM002`, uniquement après READY : ajoute **un** binding `ARM002 -> VIS_AssassinDagger_01` et un profil visuel de famille `Light`, `BaseIcon` et `WorldMesh` dans le **DA_WeaponVisualCatalogue existant**. Refus des conflits de références, pas d'écrasement des profils/transformations existants. `EquippedMesh` reste vide et utilise le fallback WorldMesh. Le catalogue RPG, les définitions d'items, les DataAssets Guerrier, l'infobulle et les 12 kits legacy ne sont pas modifiés.

Le test `Grimrock.RPG.ARMEMENT01.11.2.Visual.AssassinDaggerAssetContracts` vérifie les catalogues et les profils s'ils ont été enregistrés ; l'absence d'un binding est seulement **informative** jusqu'à la phase Apply. Le statut READY en préflight n'est pas une validation de gameplay.

## Procédure exacte sur Windows, Unreal Editor fermé

~~~powershell
cd D:\Development\GrimrockPrototype
git branch --show-current
git status --short --untracked-files=all

# Si aucun travail local n'est écrasé :
git pull --ff-only origin master

# Étape 1 (lecture seule) :
.\Scripts\RegisterARMEMENT011121AssassinDaggerVisual.ps1 -EngineRoot D:\UE_5.5 -Archetype ARM002

# Étape 2 après READY et inspection visuelle UE :
.\Scripts\RegisterARMEMENT011121AssassinDaggerVisual.ps1 -EngineRoot D:\UE_5.5 -Archetype ARM002 -Apply

# Étape 3 : contrôle du catalogue, tests et fichiers locaux
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.11.1"
Get-Content .\Saved\ARMEMENT01\ARMEMENT01_11_1_MeleeReadiness.csv
git status --short --untracked-files=all
~~~

**Ne pas lancer -Apply si le préflight est BLOCKED.** Le script n'effectue ni git add/commit/push ni opération destructive. Relire l'état du dépôt et Git LFS avant de versionner les six .uasset + le catalogue visuel, et ne sélectionner que ces fichiers. On ne modifie aucun asset non lié à ARM002.

Ensuite, ticket distinct ARMEMENT01.11.2.2 : authorer `DA_Weapon_AssassinDagger_01` depuis les données N1 de `DA_WeaponCatalogue`, puis tester inventaire, MainHand, attaque, sauvegarde et PIE. L'effet « critique conditionnel depuis l'ombre » n'est pas implicitement implémenté par un binding visuel.
