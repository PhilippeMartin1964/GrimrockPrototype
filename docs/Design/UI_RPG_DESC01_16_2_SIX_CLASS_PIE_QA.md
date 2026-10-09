# UI-RPG-DESC01.16.2 — Six-Class PIE QA

Date : **9 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
État : **À VALIDER EN PIE**  
Précondition : **UI-RPG-DESC01.16.1 validé — 6 classes / 90 Talents / 86 simples / 4 nœuds à variantes**

## 1. Objet

DESC01.16.2 valide la fiche Talent finale dans le vrai rendu UMG/PIE.

DESC01.16.1 couvre déjà exhaustivement le read-model de production. DESC01.16.2 ne répète donc pas les 90 Talents un par un : il valide les comportements que l'automatisation ne remplace pas correctement :

- hiérarchie visuelle réelle ;
- masquage des sections facultatives ;
- scroll global ;
- lisibilité des variantes simultanées ;
- boutons d'acquisition ;
- transition CHOISIR / CHOIX EN COURS / CONFIRMER / ANNULER ;
- reprojection après acquisition ;
- absence de tout vestige de l'ancien ComboBox.

Aucune modification gameplay n'est attendue pendant ce jalon.

## 2. Invariants visuels

Pour chaque Talent consulté, vérifier l'ordre :

NOM → TYPE → STATUT → PRINCIPE → EFFETS → UTILISATION si nécessaire → VARIANTES si nécessaire → ACQUISITION.

Critères globaux :

- un seul scroll vertical pour toute la fiche ;
- aucun ComboBox ;
- aucun ancien bloc de variante ;
- aucune section vide visible ;
- aucun identifiant joueur du type Status_, Skill_, Action_, Choice_, Talent_, Recipe_ ;
- pas de statut ACTION DISPONIBLE, ACTION DÉBLOQUÉE ou équivalent ;
- TYPE et STATUT restent deux informations distinctes ;
- ACQUISITION reste la dernière section ;
- consulter un Talent ne l'acquiert jamais.

## 3. Passe A — six classes, lecture seule

### Guerrier

**Posture défensive**
- TYPE : ACTIF ;
- STATUT séparé ;
- EFFETS lisibles, notamment la projection de Garde ;
- UTILISATION présente ;
- aucune VARIANTES.

**Interception**
- TYPE : RÉACTION AUTOMATIQUE ;
- UTILISATION absente ;
- PRINCIPE et EFFETS décrivent une réaction automatique.

**Spécialisation martiale**
- TYPE : PASSIF ;
- VARIANTES visible ;
- Tranchant / Perforant / Contondant visibles simultanément ;
- aucun ComboBox ;
- aucune sous-zone scrollable.

### Voleur

**Désamorçage expert**
- texte joueur Pièges / désamorçage ;
- aucune occurrence de Skill_Traps ;
- aucune VARIANTES.

**Maître des serrures**
- texte joueur Crochetage ;
- aucune occurrence de Skill_Lockpicking ;
- aucun terme technique RequirementGrants ou Rank.

**Bombe fumigène**
- UTILISATION présente ;
- TYPE et STATUT non fusionnés.

### Rôdeur

**Ennemi juré**
- TYPE : PASSIF ;
- variantes Gobelins / Vermine visibles simultanément ;
- aucun identifiant de catégorie technique ;
- CHOISIR sur chaque variante acquérable.

**Chasseur alpha**
- TYPE : RÉACTION AUTOMATIQUE ;
- UTILISATION absente.

**Tir précis**
- TYPE : ACTIF ;
- UTILISATION présente.

### Mage

**Affinité élémentaire**
- TYPE : PASSIF ;
- Feu / Glace / Air / Terre visibles simultanément ;
- pas de ComboBox.

**Chaîne élémentaire**
- TYPE : SORT ACTIF ;
- texte joueur rang d'Arcane ;
- aucune occurrence de Skill_Arcana ;
- UTILISATION présente.

**Imprégnation**
- TYPE : SORT ACTIF ;
- Feu / Glace / Air / Terre visibles simultanément ;
- blocs de variantes lisibles ;
- UTILISATION présente si projetée pour l'action volontaire.

### Prêtre

**Soin de groupe**
- TYPE : SORT ACTIF ;
- texte joueur rang de Médecine ;
- aucune occurrence de Skill_Medicine ;
- UTILISATION présente.

**Soin renforcé**
- TYPE : PASSIF ;
- UTILISATION absente ;
- aucune VARIANTES.

### Alchimiste

**Bombe incendiaire**
- TYPE : RECETTE + OBJET RAPIDE ;
- recette lisible dans ACQUISITION ;
- UTILISATION présente ;
- aucune VARIANTES.

**Réaction en chaîne**
- TYPE : RÉACTION AUTOMATIQUE ;
- UTILISATION absente.

**Transmutation majeure**
- TYPE : RECETTE + ACTIF ;
- recettes et acquisition compréhensibles ;
- UTILISATION présente ;
- aucune VARIANTES.

## 4. Passe B — acquisition des quatre familles à variantes

Utiliser le même dispositif PIE/debug déjà employé pour DESC01.15.4 afin que le Talent soit réellement DISPONIBLE et qu'au moins un point de Talent puisse être dépensé.

Pour éviter qu'un achat pollue le cas suivant, repartir d'un état frais entre familles.

Flux obligatoire pour chaque famille :

1. Ouvrir le Talent.
2. Vérifier toutes les variantes visibles.
3. Cliquer CHOISIR.
4. Vérifier CHOIX EN COURS, autres variantes toujours visibles, CONFIRMER et ANNULER visibles.
5. Cliquer ANNULER.
6. Vérifier le retour à CHOISIR.
7. Refaire CHOISIR.
8. Cliquer CONFIRMER.
9. Vérifier immédiatement :
   - Talent conceptuel : STATUT ACQUIS ;
   - variante choisie : ACQUISE ;
   - variantes sœurs : INDISPONIBLE ;
   - aucun second achat possible ;
   - point de Talent réellement dépensé.

Familles :
- Guerrier — Spécialisation martiale : Tranchant / Perforant / Contondant.
- Rôdeur — Ennemi juré : Gobelins / Vermine.
- Mage — Affinité élémentaire : Feu / Glace / Air / Terre.
- Mage — Imprégnation : Feu / Glace / Air / Terre.

## 5. Passe C — stabilité de navigation

Après acquisition :

1. changer de personnage ;
2. revenir au personnage ;
3. cliquer sur un autre Talent ;
4. revenir au Talent à variantes acquis ;
5. fermer la page Compétences ;
6. la rouvrir.

Attendu :
- la fiche correspond toujours au Talent consulté ;
- aucune variante ne repasse artificiellement en CHOIX EN COURS ;
- le STATUT acquis reste cohérent ;
- la variante acquise reste ACQUISE ;
- les variantes sœurs restent INDISPONIBLES ;
- aucun vieux widget n'apparaît au refresh.

## 6. Critères de clôture

DESC01.16.2 est validé si :
- les six classes passent la passe A ;
- les quatre familles passent le flux complet de la passe B ;
- la passe C est stable ;
- aucune anomalie visuelle ou textuelle structurelle n'est observée ;
- aucun ComboBox ou bloc legacy ne réapparaît.

Une anomalie purement esthétique est reportée à UI-RPG-VISUAL01 et ne rouvre pas DESC01.15/16.

Après validation utilisateur :
- DESC01.16 est clos ;
- suite : UI-RPG-DESC01.17 — Documentation & Closure.
