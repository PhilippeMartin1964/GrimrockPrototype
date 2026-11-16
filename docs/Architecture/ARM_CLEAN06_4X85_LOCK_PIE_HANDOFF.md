# ARM-CLEAN06.4X.16.8.5 — Recette PIE des serrures canoniques

## Migration enregistrée

Le commit binaire `da9c6c046f1fdc0322274484c995555a8b90a5ca`
modifie exactement :
- `Content/GrimrockPrototype/Core/DataAssets/DA_Lock_CopperWall.uasset`
  : `DefaultBehavior.Lock.AcceptedKeyIds = [Key_Copper]` ;
- `Content/GrimrockPrototype/Core/DataAssets/DA_Lock_IronWall.uasset`
  : `DefaultBehavior.Lock.AcceptedKeyIds = [Key_Iron]`.

Les anciens `AcceptedKeyItems` ne sont pas supprimés.
L'ensemble des clés acceptées avant/après est le même :
`Key_Copper` pour cuivre, `Key_Iron` pour fer.
`bConsumeKeyOnUnlock`, `bStartsUnlocked`, messages,
GUID, liens d'énigmes, niveaux et sauvegardes
personnelles restent hors des écritures autorisées.

Le commit de transaction parent `871c97d5046720c9ac3d664ba73b2e92a17429e2`
a passé SilentTask, LocalAuto et ARM-CLEAN06.4X83 SUCCESS.
La validation indépendante du SHA binaire final doit
également être contrôlée avant la recette PIE.

## Recette UE5.5.4 (sans modifier les vrais niveaux)

Fermer l'Editor, mettre à jour le checkout master propre
avec `git pull --ff-only origin master`, puis
`git lfs pull origin master`. Recompiler C++ et lancer PIE.

1. Serrure cuivre : utiliser `Key_Copper` depuis le
   curseur/l'action contextuelle. Elle doit s'ouvrir
   comme avant ; `Key_Iron` ou un objet quelconque
   ne doivent pas fonctionner.
2. Serrure fer : utiliser `Key_Iron`. Elle doit
   s'ouvrir ; `Key_Copper` doit être refusée.
3. Vérifier messages verrouillé/déverrouillé,
   effets visuels, liens vers les portes et
   absence de déverrouillage automatique avec
   une clé rangée dans le sac.
4. Contrôler la consommation : elle doit respecter
   exactement la valeur déjà définie dans
   `bConsumeKeyOnUnlock` ; ne pas changer le flag.
5. Vérifier que support de torche, Gardien,
   inventaire et ramassage restent fonctionnels.
6. Si Save/Continue est testé, utiliser une copie
   de sauvegarde uniquement.

Ne pas enregistrer de nouveaux changements dans
les véritables niveaux ni effacer d'anciens SaveGames.

## Suite après recette

Les quatre références historiques identifiées
dans l'issue GitHub #176 sont alors persistées
sous forme canonique. La suppression des anciens
pointeurs ne doit pas être lancée avant un audit
complet de leurs derniers lecteurs et une
validation des SaveGames sur copies privées.
