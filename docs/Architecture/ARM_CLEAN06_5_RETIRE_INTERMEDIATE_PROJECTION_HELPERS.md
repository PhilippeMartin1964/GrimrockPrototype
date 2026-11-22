# ARM-CLEAN06.5 — Retrait des anciens helpers de projection en production

## Contenu

Ce lot retire sept couples .cpp/.h dont l'unique rôle était la conversion
intermédiaire des définitions et catalogues historiques (noms, visuels,
icônes d'action, attaques, présentations, affinités et copie d'instance).

Huit fichiers tests 062AD/AE/G/H/J/K/L/M conservent les noms des
suites Automation existantes et lisent désormais directement le vrai
DA_ItemSystem via UGridItemSubsystem.

- AD: 44 instances ARM N1/N20 sérialisées puis désérialisées avec
  UGameplayStatics::SaveGameToMemory/LoadGameFromMemory en RAM ;
  vérifie GUID, niveau, dette d'armure, propriétaire, lecture et transform.
- AE: 44 réserves N1/N20, y compris boucliers et dette d'armure ;
  aucune régénération de protection lors d'une résolution canonique.
- G et H: 19 actions ARM N1 avec identités, sources de main et
  présentations ; refus des actions non liées et niveaux invalides.
- J et K: 22 bindings visuels, ActionIcons du catalogue unique ;
  pas de nouvelle source icône historique.
- L et M: 22 ItemIds physiques stables, classes et 82 archétypes
  canoniques, 6 préférences de classes ; aucun fallback DA_Weapon.

Les helpers de politique/visuel de projectiles sont conservés
temporairement car GridEditorArmHandThrowPlan.h les référence encore.

## Sécurité

Aucun asset .uasset, sauvegarde .sav ou WBP n'est modifié par ce lot.
Cette preuve en mémoire ne remplace pas le contre-audit des sept vraies
sauvegardes v27 de l'utilisateur. Le retrait physique des anciens
assets reste bloqué (deletion_authorized=false), de même que tout
effacement des deux Readables historiques présents dans ces saves.
