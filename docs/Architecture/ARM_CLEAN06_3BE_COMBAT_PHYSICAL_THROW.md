# ARM-CLEAN06.3BE — Lancers de combat canoniques sans ancien DA_Weapon

**Parent vérifié :** ARM-CLEAN06.3BD (`331666a`) ; Windows `SilentTask` et `LocalAuto` SUCCESS.

## Diagnostic

Le chemin `RequestCharacterAttackInternal` pouvait créer un vrai projectile canonique
en exploration, mais la décision de **lancer physiquement en combat** dépendait
encore de `SourceItemDefinition->IsCombatThrowable()`. Le résolveur de
l'attaque était pourtant déjà canonique : `DA_ItemSystem` fournissait le
profil de projectile de l'arme, le coût d'objet dans `DA_ActionCatalogue`,
le niveau N1–N20 et l'identité exacte en main ou dans le sac.

Sans ancien `DA_Weapon_*`, l'exécuteur risquait donc de consommer l'objet
équipé sans créer le vrai projectile ni la récupération correspondante.

## Contrat implémenté

- `FGridCombatPhysicalThrowSourceReadModel` projette la décision éphémère
  depuis la source physique, le registre et le coût **déjà authoré**.
  Un ARM enregistré avec `Weapon.Projectile.bEnabled` et
  `SourceItemQuantityCost=1` produit exactement un projectile,
  même sans `UGridItemDefinitionAsset`.
- Le même booléen commande le débit (une seule unité) et le branchement
  vers `TryLaunchEquippedItemForAttack` ou
  `TryLaunchInventoryItemForAttack`; ces méthodes réemploient les
  politiques canoniques de balistique, Force, masse, mesh et GUID.
- Une identité enregistrée invalide N0/N21, un profil visuel incomplet
  ou une attaque de jet à distance avec coût manquant / différent de 1
  est refusée **avant réservation des PA**.
- Une définition ancienne contradictoire ne peut ni rendre un projectile
  canonique gratuit, ni interdire un projectile déjà authoré, ni faire
  lancer physiquement une épée canonique ordinaire.
- Hors registre, l'ancien `bCombatThrowWeapon` conserve sa sémantique
  tant que la conversion des contenus n'est pas faite.
- Aucun `ActionId`, `SourceItemQuantityCost`, `ProjectilePolicy`,
  `SaveGame` ou asset binaire nouveau n'est créé.

## Tests

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalCombatPhysicalThrow` :
`AuthoredActions` (trois ARM jetables N1/N20, action réelle et coût 1),
`InvalidKnown` (N0/N21 et épée non jetable), `LegacyCompatibility`
(l'ancienne règle uniquement pour les ItemId hors registre).
La suite `Grimrock` Windows doit être vérifiée sur le SHA fonctionnel final.

**À confirmer en PIE :** coût PA, projectile dans le combat actif,
impact/miss, retour au sol, inventaire et Save/Continue.
L'Automation `-NullRHI` et les tests de décision ne prouvent pas la
trajectoire GPU, les animations ni la physique Chaos.

**Localisation :** `UI.csv → ST_UI.uasset → WBP → Game/fr/en`
strictement inchangée ; aucune chaîne visible, aucun WBP ajouté.
**Restent ouverts :** jets génériques hors `ProjectilePolicy`, lecteur
de présentation de flèches N0 en transition, conversion Editor 06.4,
nettoyage 06.5 et recette 06.6.
