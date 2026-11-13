# ARM-CLEAN06.2AH — Retrait des audits exploratoires remplacés

## Décision

La refonte doit **réduire** le code livré ; les outils d'audit
intermédiaires ne deviennent pas une deuxième chaîne de migration.

Les commandlets 06.2C (projection numérique seule) et 06.2E
(inventaire sémantique de l'ancien dossier Weapons) ont achevé
leur rôle historique. Ils sont retirés de la **branche courante**
avec leurs deux wrappers PowerShell et les helpers 06.2E3/2E4.
Les deux branches de déclenchement ponctuel correspondantes sont
supprimées de `UEValidationAgent.ps1`.

La validation structurelle actuelle n'est **pas supprimée** :
- `GridArmClean06ProductionNumericsCandidate::Build` vérifie
  les 82 archétypes, 1 640 profils et 260 profils passifs ;
- `GridArmClean06FullProductionCandidate::Build` démontre
  les règles de mains et les types des 22 définitions actives ;
- `GridArmClean06CompleteProductionPreview::Build` confronte
  les armes du manifeste, la munition source, les visuels et
  les politiques de projectile ;
- tous les tests Automation des étapes 06.2S/06.2U/06.2Y et
  les nouveaux résolveurs runtime sont maintenus.

Le retrait ne touche pas aux assets source, DataAssets binaires,
BluePrints, gameplay, SaveGame ou au manifeste des 22 ARM.
Les rapports historiques restent archivés dans les docs et
les commits Git ; leurs commandlets ne sont plus présents
dans le code courant.

### Bilan exact (ancien → nouveau)

- 581 lignes physiques C++ retirées
  dans 4 fichiers ;
- 921 lignes de scripts d'audit retirées
  dans 4 fichiers ;
- 117 lignes retirées de l'agent Windows,
  uniquement dans les déclenchements ponctuels 06.2C et 06.2E ;
- aucun nouveau résolveur, aucun nouveau test doublon ;
- documentation présente : ce document uniquement.

Les validations courantes `UE5.5.4/SilentTask` et
`UE5.5.4/LocalAuto` doivent réussir sur le SHA exact.
La suppression est **une réduction de la dette d'outillage**
et non une activation du nouveau système d'armes. Les
bloqueurs restants sont toujours les vrais liens d'actions /
présentations, puis l'authoring des assets et le cutover 06.3–06.6.
