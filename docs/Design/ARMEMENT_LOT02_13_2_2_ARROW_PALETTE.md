# ARMEMENT-LOT02.13.2.2 — Afficher les flèches dans l'éditeur de donjon

## Défaut observé (PIE utilisateur, 28 octobre 2026)

- Étapes PIE 1–4 confirmées par le développeur.
- ARM048 : absence de flèches dans la palette du donjon.
- `[GridActionCatalog] Accepted=false ... InsufficientSourceItems` : comportement attendu avec stock nul, **pas un bug de combat**.
- `DA_Item_Arrows_01` (identité `Item_Arrows_01`) est déjà créé. ARM045 et ARM048 ont un lien vers cet item. Il ne faut pas dupliquer les munitions.

## Correction prévue, sans remplacer le gameplay

Ajout **d'une seule entrée** dans `DA_ObjectPalette_Default` :
- `EntryId = Item_Arrows_01`
- `PaletteCategory = Items`
- `DefaultItemDefinition = /Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Arrows_01`
- Pas de `DefaultWorldObjectDefinition` compagnon ni de `Icon` recopiée dans l'entrée.
- Contrôle des doublons/alias et validation complète de la palette avant sauvegarde.
- Conservation de toutes les entrées de palette et de tous les objets existants.

Le commandlet `GridArrowPaletteAuthoring` propose le changement en lecture seule ; `-Apply` n'enregistre que la palette, puis le test `Grimrock.RPG.ARMEMENT.LOT02.ArrowPaletteDirectCollectible` contrôle le placement dans un niveau **temporaire en mémoire**.

## Application locale

Fermer Unreal Editor :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
.\Scripts\PublishArrowPalette.ps1 -EngineRoot D:\UE_5.5
.\Scripts\PublishArrowPalette.ps1 -EngineRoot D:\UE_5.5 -Apply
```

La seconde commande reconstruit l'Editor, applique au plus une modification de `.uasset`, vérifie les hashes des deux arcs et des flèches, puis exécute la suite LOT02. Elle ne publie aucun changement Git.

Après tests, ouvrir `L_Dungeon` ou la copie de test, outil `Paint Object`, catégorie `Items`, recherche `Flèches` ou `Item_Arrows_01`. L'entrée existe alors dans l'éditeur. Placer un item de test dans le `UGridLevelAsset` isolé, régler sa quantité à 2 et lancer PlayTest.

**Limite critique :** la définition des flèches de LOT02.5 a été créée **sans icône et sans mesh dédiés**. La palette fournit un libellé de secours, mais un objet sans mesh risque de ne pas être visible ni cliquable au sol. La réussite de l'entrée de palette n'est **pas** une validation de ramassage souris ou de tir PIE. Fournir ensuite un modèle et une icône de flèches propres et les affecter à `DA_Item_Arrows_01`, avec tests des collisions, de l'icône 48×48 et de la pile, avant de clore le point 5.

Une fois les vrais visuels testés et les rapports locaux verts, versionner **uniquement** `DA_ObjectPalette_Default.uasset` dans un commit séparé. Ne jamais inclure les modifications de XLSX, le document Meshy non suivi, les copies locales de niveau, ni les sauvegardes de test. Les onze armes restent `Planned`.
