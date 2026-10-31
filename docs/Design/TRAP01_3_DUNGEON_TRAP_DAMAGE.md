# TRAP01.3 — Déclenchement et dégâts individuels des pièges de donjon

Date : 31 octobre 2026. Statut : code et tests publiés, **validation locale UE5.5.4 en attente**.
Prérequis validés : TRAP01.1 (2/2), TRAP01.2 (2/2), ARMEMENT01.16.Trap (3/3).

## Contrat de déclenchement

Le piège reste un Trigger opt-in placé dans l'unique asset du niveau ; il ne
remplace ni les Trigger historiques ni les pièges temporaires de combat.
`AGridLevelRuntimeActor::HandlePartyCellChanged` déclenche uniquement lorsqu'une
translation de case à case a réellement abouti (OldCell != NewCell).
L'initialisation stable de la position ne blesse pas les personnages.
L'action passive de perception (TRAP01.2) arrive après les événements d'entrée.

`TriggerDungeonTrapsOnPartyEnterCell` vérifie la cellule actuelle du groupe,
les GUID des pièges, leur définition, le mode `OnEnterCell`, leur état
persistant et la présence d'au moins un personnage vivant. Un piège détecté
mais non désamorcé demeure dangereux. Les états `Disarmed` et `Disabled`
refusent toujours l'activation ; un piège `Triggered` à usage unique aussi.
`bOneShot=false` autorise un nouveau déclenchement à chaque nouvelle entrée.
Tout rejet préalable ne consomme pas de RNG ni de HP.

Les modes `OnCrossBoundary` et `OnInteract` restent définis dans les données
mais ne déclenchent volontairement aucun dégât avant leur contrat spatial
ultérieur. Sol, mur et plafond sont gérés par le `PlacementSurface` existant :
un déclencheur `OnEnterCell` réagit à l'entrée dans sa cellule.

## Dégâts : une valeur partagée, résistances personnelles

Un seul `RawDamage` est tiré entre `MinimumDamage` et `MaximumDamage`
pour une activation et non par membre du groupe.

Pour chaque personnage vivant et concerné :
1. État de protection : `GetEffectiveCurrentPhysicalArmor`,
   `GetEffectiveCurrentMagicalArmor` (équipement inclus).
2. Résistance adaptée au `DamageType` depuis
   `ComputeCharacterEquipmentResistances` puis
   `FGridCombatResolver::GetResistancePercent`.
3. Modificateurs défensifs existants de classe et statut via
   `FGridCombatModifierResolver`, selon le précédent RPG03.6.
4. Dégâts garantis sans attaque/esquive/critique avec
   `FGridCombatResolver::ResolveDirectDamage`.
5. Consommation réelle des protections par `ApplyPhysicalArmorDamage`
   et `ApplyMagicalArmorDamage`, puis décrément des PV et notification HUD.

`bAffectsEntireParty=true` cible tous les membres vivants.
Sinon : membre sélectionné s'il vit, ou premier membre vivant par index.
Les membres morts ne sont pas touchés de nouveau.
Les dégâts physiques retiennent leur sous-type Slashing/Piercing/Bludgeoning ;
ils partagent toujours une résistance `Physical`, sans famille nouvelle.
Fire, Ice, Lightning, Poison, Holy, Necrotic et Arcane utilisent les
résistances et armures magiques déjà présentes.

Exemple de test pur : 30 feu bruts. Avec 50 % de résistance et
5 armure magique : 10 PV perdus. Sans résistance ni armure : 30 PV perdus.

## États et événements

`TryRecordDungeonTrapTriggered` marque le GUID dans les collections
`TriggeredTriggerTrapIds` et `DetectedTriggerTrapIds` ; cela révèle le
mesh et son overlay éventuel (TRAP01.2), même si la perception a échoué.
Un événement `Dungeon.Trap.Triggered` précise la position relative
du piège, notamment « sous le groupe ».

Le flux publie ensuite un événement `Dungeon.Trap.Damage` par personnage,
avec son nom, le type de dégâts en français, les dégâts reçus en PV et
l'absorption des deux armures. Après application aux victimes, le runtime
émet l'événement logique `EGridObjectEvent::Activated` du piège, permettant
l'exécution des liens de niveau existants. Les vieux triggers conservent
leur ancien traitement et n'activent pas les nouveaux pièges en double.

Les objets candidats sont copiés avant traitement : un lien logique
provoquant un déplacement vers un autre niveau ne peut invalider l'itération.
Le traitement s'arrête si le groupe ou le niveau actif change.

## Sauvegarde et périmètre

Les marqueurs persistants sont enregistrés par niveau et GUID dans
`FGridLevelRuntimeState`. Un piège à usage unique ne se réarme pas après
Save/Continue. Les tests vérifient la restauration d'un snapshot en mémoire,
et non un cycle complet de fichier binaire sur disque.

Non implémentés dans TRAP01.3 : les animations, projectiles, VFX/sons,
statuts de brûlure/gel/poison, esquive spéciale, déclenchement sur
OnCrossBoundary/OnInteract, sélection du meilleur désamorceur et transition
de la compétence Pièges vers la Dextérité (TRAP01.4). Aucun .uasset graphique
n'est créé ou modifié dans ce jalon.

## Validation locale UE 5.5.4

Commande de validation (compilation incluse) :

    cd D:\Development\GrimrockPrototype
    git pull --ff-only origin master
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TRAP01.3"

Deux tests automatiques :
- `Grimrock.TRAP01.3.DamageResistanceAndArmor` : résistance, armures,
  sous-type, plusieurs profils, personnage mort.
- `Grimrock.TRAP01.3.OnEnterGroupDamageAndSave` : trois personnages,
  piège de plafond feu, blessure différente, état déclenché, refus hors
  cellule, anti-double-déclenchement, marqueurs persistés, piège désarmé,
  initialisation stable, entrée physique sur piège perçant.

Puis filtres de régression avec `-SkipBuild` :
`Grimrock.TRAP01.2`, `Grimrock.TRAP01.1`,
`Grimrock.RPG.ARMEMENT01.16.Trap`.

Les tests ci-dessus nécessitent l'exécution locale sous UE 5.5.4 ;
aucune réussite n'est revendiquée avant réception des logs.
