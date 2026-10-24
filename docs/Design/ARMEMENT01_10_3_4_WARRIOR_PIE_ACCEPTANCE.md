# ARMEMENT01.10.3.4 — Recette PIE des quatre armes Guerrier N1

**Établi le 24.10.2026 — phase de validation manuelle (pas de nouvelle mécanique C++).**

## 1. Preuves déjà reçues et limites

Le développeur a fourni les trois rapports locaux suivants :

| Filtre TD04.2 | Rapport | Succès | Warnings | Échecs | Non exécutés |
|---|---|---:|---:|---:|---:|
| `Grimrock.RPG.ARMEMENT01.10.3.3` | `TD04-20261024-204340` | 8 | 0 | 0 | 0 |
| `Grimrock.RPG.ARMEMENT01.10` | `TD04-20261024-204357` | 22 | 0 | 0 | 0 |
| `Grimrock.UI.EventHUD01` | `TD04-20261024-204413` | 55 | 0 | 0 | 0 |

Le filtre ARMEMENT01.10 couvre également les cinq tests `Grimrock.RPG.ARMEMENT01.10.3.*` du lot Guerrier (les noms sont déclarés dans `GridEditorARMEMENT01103WarriorFunctionalTests.cpp`). **Il ne prouve pas les interactions d'un joueur réel dans PIE.** Les trois filtres ne représentent pas 85 tests disjoints.

Le nettoyage `ARMEMENT01.10.3.3.9` est considéré **terminé côté C++/régressions** sur la base des logs et de la confirmation utilisateur. Le présent jalon ne rouvre pas l'infobulle.

## 2. Périmètre à accepter

| ID | Arme | Définition réelle | Usage important |
|---|---|---|---|
| ARM014 | Épée droite | `Weapon_StraightSword_01` | main principale, scénario avec bouclier ARM075 |
| ARM015 | Épée longue | `Weapon_LongSword_01` | main principale, attaque/équipement |
| ARM025 | Pic de guerre | `Weapon_WarPick_01` | main principale, attaque/équipement |
| ARM031 | Grande hache | `Weapon_GreatAxe_01` | **deux mains** : OffHand virtuelle réservée, une seule instance physique |

Autres dépendances : `Weapon_RoundShield_01` (ARM075), `DA_WeaponCatalogue`, `DA_WeaponVisualCatalogue`, `DA_ObjectPalette_Default`, `DA_GridLevel_00`, `L_Dungeon`.

**Aucune valeur chiffrée de dégâts, portée, coût PA ou poids n'est inventée dans cette recette** : elles proviennent des DataAssets et doivent être observées dans le jeu.

## 3. Préparation reproductible

- Dans Unreal Editor 5.5.4, ouvrir `L_Dungeon` ; pour le parcours normal, partir du menu de création de personnage (ne pas contourner le startup flow).
- Utiliser **une partie/sauvegarde de test séparée**. Ne jamais modifier la sauvegarde de jeu courante pour une recette.
- Les quatre armes sont présentes dans le DataAsset de niveau d'après le test `ProductionBindings` ; relever leur position dans l'éditeur si elles ne sont pas immédiatement visibles en PIE, sans les déplacer dans la carte de production.
- Personnage de test : disposer de places libres dans le sac ; noter le nom du personnage, ses PA et ses slots MainHand/OffHand. Le sac plein fait l'objet d'un essai **séparé**.
- Raccourci inventaire : `I` ; les interactions souris, menus et raccourcis de combat restent ceux de l'interface existante. Ne pas modifier les WBP pour réaliser le test.
- Pour chaque capture ou anomalie : préciser arme, scénario, étapes de reproduction, PA et état des mains, et conserver les logs de session pertinents.

## 4. Première passe courte — identifier un blocage réel

1. **ARM014 :** localiser au sol, ramasser par l'interface prévue, vérifier qu'il disparaît du monde et entre **une seule fois** dans le sac. Examiner icône, nom et poids. Équiper en MainHand, lancer une attaque en combat réel puis déséquiper ; vérifier la restitution au sac. Déposer/récupérer.
2. **ARM015 / ARM025 :** reproduire ramassage → inventaire → équipement → attaque → déséquipement → dépôt/récupération, chacun isolément. Enregistrer les éventuelles différences de profil provenant réellement des DataAssets.
3. **ARM031 :** équiper d'abord ARM014 MainHand + ARM075 OffHand, puis la grande hache. Vérifier **épée et bouclier dans le sac**, hache uniquement comme objet physique en MainHand, OffHand réservée, action de hache unique. Retirer la hache (y compris par le slot virtuel si exposé dans le widget) et vérifier la restitution sans duplication.
4. **Arme/PA/portée :** pour chaque arme, confronter au moins une attaque valide au journal et aux PA avant/après. Tester un refus à PA insuffisants et un refus hors portée ; ne pas déduire que ces deux refus sont couverts par les tests de profils en mémoire.
5. **Persistance :** sauvegarder avec ARM031 équipée, quitter/recharger. Vérifier niveau de l'instance = 1, main principale et main secondaire virtuelle réservée, GUID/quantités et aucune réapparition de la même arme au sol ou dans le sac. Si la sauvegarde n'est pas accessible dans ce contexte, enregistrer **BLOQUÉ**, pas PASS.
6. **Sac plein (test isolé) :** avant l'équipement ARM031, occuper les emplacements de manière à ne pas pouvoir rendre l'épée et le bouclier au sac. Le refus doit être atomique : ARM014 et ARM075 restent équipés, ARM031 reste où elle était, aucun élément ne disparaît.

**En cas de premier échec bloquant, arrêter la chaîne et corriger uniquement ce problème dans un ticket distinct avant de poursuivre la recette.** Une simulation en mémoire ne remplace pas l'essai souris en PIE.

## 5. Matrice de résultats à renseigner après la campagne

Codes : `OK` vérifié réellement en PIE ; `KO` avec preuve ; `BLOQUÉ` impossibilité d'exécuter ; `NT` non testé. **Ne jamais reporter un OK sur la base d'un test unitaire.**

| Scénario PIE | ARM014 | ARM015 | ARM025 | ARM031 |
|---|---|---|---|---|
| P1 Mesh/matériau/placement corrects | NT | NT | NT | NT |
| P2 Ramassage unique / retrait du monde | NT | NT | NT | NT |
| P3 Inventaire, icône, poids, tooltip | NT | NT | NT | NT |
| P4 Équipement/déséquipement et mains | NT | NT | NT | NT |
| P5 Dépôt et récupération | NT | NT | NT | NT |
| P6 Combat réel, PA, portée, dégâts | NT | NT | NT | NT |
| P7 Sauvegarde/rechargement | NT | NT | NT | NT |

**ARM031 spécifique :** T1 remplacement épée+bouclier = NT ; T2 main secondaire virtuelle = NT ; T3 refus lorsque sac plein = NT.

### Preuve à associer à chaque KO

- Arme et `ItemDefinitionId` (sans supposer que le nom visible suffit)
- Étapes exactes (clics, ordre de placement, état initial)
- MainHand / OffHand / slots de sac avant-après, PA si combat
- Capture d'écran + entrée du journal d'événements, log si disponible
- Sauvegarde de test si le problème survient au rechargement

**Retour utilisateur du 24.10.2026 :** ARM014/ARM015/ARM025/ARM031 testées en jeu avec succès, objets récupérés dans l'inventaire, dégâts conformes au catalogue Excel, et sauvegarde / rechargement sans perte. Les scénarios particuliers non décrits expressément (notamment refus sac plein, PA insuffisants et hors portée) ne sont pas automatiquement déclarés PASS. La rubrique DÉGÂTS de l'infobulle a révélé un manque de détail ; correctif ciblé lancé sous [ARMEMENT01.10.3.3.10](ARMEMENT01_10_3_3_10_DAMAGE_TOOLTIP_DETAILS.md), sans remettre en cause les dégâts des armes.

## 6. Critères de clôture et jalon suivant

- **Technique :** suite globale ARMEMENT01.10 et EventHUD01 validées (acquis le 24.10.2026).
- **PIE :** P1–P7 **OK pour les quatre armes**, T1–T3 ARM031 OK ; si un scénario est impossible pour une fonctionnalité non implémentée, ouvrir un ticket spécifique et laisser le statut BLOQUÉ.
- **Git :** ne publier que les corrections nécessaires, une par commit atomique, sans réécrire le catalogue Excel, les statistiques ou les kits ; ne pas committer de copies de niveaux/SaveGame de test.
- **Ensuite :** reprendre la production N1 des **autres armes de mêlée** ARM002, ARM003, ARM013, ARM020 (ordre issu d'ARMEMENT01.10.1), puis portée/jet et magie/passifs. Les 18 recettes ne sont pas encore activées dans le wizard.

Sources internes : `ARMEMENT01_10_3_WARRIOR_FUNCTIONAL_VALIDATION.md` et `ARMEMENT01_10_1_N1_PRODUCTION_READINESS.md`.
