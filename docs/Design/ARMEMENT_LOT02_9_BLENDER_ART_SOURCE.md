# ARMEMENT-LOT02.9 — Sources artistiques des onze armes

Ce jalon ajoute un générateur Blender pour les 11 modèles N1 restants, sans réécriture des DataAssets existants.

Chaque source contient une géométrie originale distincte (lance, arc, couteau, javelot, dard, deux baguettes, bâton, sceptre, orbe, égide).
Le générateur crée un .blend éditable, un FBX, une icône RGBA 512x512, et les cartes BaseColor/Normal/ORM 256x256.
Une palette PBR procédurale distingue bois, cuir, fer, acier, bronze, or, cristal et gemmes.
Les normales sont neutres: le détail géométrique existe dans les meshes, sans prétendre avoir fait un baking high-poly.

## Production locale groupée

Fermer Unreal Editor si nécessaire puis synchroniser master. Blender 4.2 ou supérieur est requis.

    cd D:\Development\GrimrockPrototype
    git pull --ff-only origin master
    .\Scripts\GenerateStarterWeaponArt.ps1

Si Blender n'est pas dans PATH, fournir -BlenderExe 'C:\...\blender.exe'.

Sortie locale: Saved/ARMEMENT-LOT02/LOT02_9_ArtSource/ avec le manifeste LOT02_9_ArtProduction.csv.
Pour auditer un lot généré précédemment sans mutation: -InspectOnly.

## Limites et validation artistique

Ces fichiers sont des sources Blender procédurales de première passe, non des .uasset déjà importés ni des assets finalisés à la main.
Vérifier individuellement la silhouette, la lisibilité en 48x48, les UV, la collision, l'échelle et l'orientation.
Importer sous UE les vrais FBX/PNG, connecter le matériau aux slots du mesh et enregistrer les chemins réels dans le manifeste en mode Planned.
Le pipeline existant pourra ensuite produire Visual / Item / Palette; les tests PIE devront être effectués avant de passer en Active.

Ce jalon ne modifie aucun kit, aucune sauvegarde, aucun niveau, aucun fichier L_MainMenu.umap.
Il ne réalise aucun commit automatique de binaire. Les onze armes demeurent UNDECLARED tant que le contenu n'est pas approuvé et importé.
