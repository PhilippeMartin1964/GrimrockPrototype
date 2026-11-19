# ARM-CLEAN06.5G.9 — Retrait des déclencheurs de migration terminés

À intégrer après 5G.8 validé. Les déclencheurs exacts des anciens commits ARM-CLEAN03/06, les publications de preuves ponctuelles et la récupération spécifique à 4U sont désormais retirés de l'agent Windows. Le déclencheur ponctuel 4X83 dans ValidateUE est également retiré. Aucun déclencheur ne pouvait normalement se reproduire sur le HEAD courant, mais leur maintien empêchait de retirer proprement les scripts historiques.

## Invariants maintenus

- Build, Automation, contrôle du worktree, Git LFS, statut de réussite/échec, journal et publication bornée des erreurs restent inchangés.
- Les récupérations indépendantes Catalyst/Oil, le traitement de Skill_Traps et la publication EFFECTS-ORG01.1 sont maintenus.
- Les trois auto-tests relatifs aux sauvegardes et au rollback sont conservés. Aucune copie privée, sauvegarde utilisateur, asset, historique Git, ou reçu n'est supprimé.
- Les commandlets et scripts ARM en eux-mêmes seront retirés par dépendance dans un lot suivant, après vérification de leurs appelants.

L'agent installé doit encore publier les deux statuts Windows sur le nouveau HEAD. Ce lot ne constitue pas une recette finale de migration et ne retire aucun package.
