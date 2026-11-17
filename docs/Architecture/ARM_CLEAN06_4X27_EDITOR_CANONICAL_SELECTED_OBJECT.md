# ARM-CLEAN06.4X.27 — Selected Object : identité canonique et changement explicite d'ItemId

**Date :** 17 novembre 2026. **Parent :** `b34eccf33e56fed6ec8feb8d759eb4399714c048`. **Périmètre : C++ Editor + Slate + tests transitoires + documentation.** Aucun WBP, .uasset, .umap, .ini, SaveGame ni fichier binaire modifié.

## Problème restant après X.25 et X.26

La prévisualisation et la validation acceptent un item portant `CanonicalItemId`, mais le panneau Selected Object affichait `Item Id` seulement depuis `ItemDefinitionAsset` et masquait le réglage `Item Level` si ce pointeur n'existait pas. Le setter `SetSelectedObjectItemDefinitionId` ne savait chercher que l'ancienne palette `DefaultItemDefinition` ; il était impossible de choisir un ID canonique sans vieux DA_Weapon.

## Contrat de la correction

- Le panneau Selected Object affiche d'abord `CanonicalItemId` s'il existe, et seulement sinon le nom de l'ancien `ItemDefinition`. Le champ `Canonical Item Id` est éditable. Il s'applique aux loose items et aux ItemSpawns. Une identité canonique sans pointeur ancien présente une note explicative et, si ARM, un contrôle du niveau **physique N1–N20** ; les non-ARM restent à N0.
- `SetSelectedObjectItemDefinitionId` reconnaît les identités enregistrées dans le registre unique `DA_ItemSystem`. Lorsqu'un auteur choisit volontairement un nouvel ID, le niveau physique existant est conservé s'il est légal pour cette nouvelle famille. Sinon, l'authoring d'une **nouvelle** identité choisit N0 pour non-ARM, N1 pour ARM, jamais ARM N0 enregistré. Ce changement n'est ni une restauration de sauvegarde ni un défaut implicite pendant le jeu.
- Les quantités sont validées contre `MaxStack` de la nouvelle identité. Une pile de cinq flèches ne devient pas cinq épées, et aucun élément n'est supprimé par conversion. En cas de refus, ID, ItemLevel, GUID, quantité et ancienne référence sont inchangés.
- Lors d'un changement d'ID canonique accepté, le vieux pointeur `ItemDefinition` est vidé pour éviter les identités contradictoires. Un simple commit de la **même identité** ne migre jamais les vieux objets réels : leur ancien asset et leur rendu restent intacts jusqu'à parité visuelle native.
- L'ancien sélecteur `ItemDefinitionAsset` ne peut plus assigner une définition contradictoire à un objet dont `CanonicalItemId` est déjà persistant ; un pointeur nul reste permis pour les objets canoniques.
- Les ID totalement inconnus restent accessibles uniquement par l'ancien chemin palette quand il n'y a **aucune** identité canonique fixée. Un ID canoniquement choisi ne peut pas devenir legacy simplement parce que le registre est absent.

## Régression automatisée

`Grimrock.RPG.ARM.CLEAN06_4X27.SelectedObjectCanonicalItemId` construit un véritable acteur d'éditeur dans un `EditorPreview` transitoire. Cas couverts : recommit du même ID legacy sans migration ; changement vers sword N20 ; ancien pointeur contradictoire rejeté ; passage vers flèches N0 ; nouvelle famille bouclier N1 ; rejet d'ID inconnu sans mutation ; conservation des GUID et quantités ; ItemSpawn à cinq flèches refusé pour une arme individuelle, puis conversion validée avec quantité explicitement ramenée à un. Les anciennes régressions X.25/X.26 restent obligatoires.

## Limites

La frappe d'un ID dans le panneau reste textuelle ; aucune liste de choix native n'a été créée. L'authoring de la palette par ID et les références `ItemDefinition` persistantes dans les niveaux ne sont **pas** migrés. Il faut prouver et écrire par Unreal Engine les paramètres des 22 ARM natifs avant tout retrait massif de pointeurs. Un test automatisé valide le setter et la compilation de l'UI, **pas** l'ergonomie/qualité visuelle en PIE.

**06.4 partielle ; 06.5 BLOQUÉE.** Attendre les deux statuts Windows sur le SHA publié.
