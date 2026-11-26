# ARMEMENT-INSPECTION01A — Addendum officiel

**Version 1.0 — exigences approuvées le 26.11.2026**

Projet GrimrockPrototype · Unreal Engine 5.5.4

Documents de référence : [DESIGN01](ARMEMENT_DESIGN01_FICHE_MAITRESSE.md) et [INSPECTION01](ARMEMENT_INSPECTION01_SPECIFICATION.md). Cet addendum prévaut en cas de contradiction. Il fixe les exigences, pas leur implémentation. La réalisation fera l'objet d'INSPECTION02.

## 1. Exigences normatives

| ID | Exigence |
|---|---|
| INS-A01 | Différence visuelle perceptible entre chaque niveau consécutif N1–N20. |
| INS-A02 | Progression esthétique exprimant la récompense, cohérente avec l'identité de l'arme. |
| INS-A03 | Fond noir neutre par défaut ; ambiance 2.5D lointaine facultative, sans décor proche. |
| INS-A04 | Examen 3D générique pour toutes les familles d'objets, actuelles et futures. |
| INS-A05 | Support natif des objets sans niveau ; N0 interne jamais affiché au joueur. |
| INS-A06 | Informations et commandes adaptées aux capacités réelles de l'objet. |
| INS-A07 | Cohérence de la projection entre atelier, examen joueur et donjon. |
| INS-A08 | Validation artistique individuelle par le propriétaire. |

## 2. Progression esthétique : vingt niveaux, dix-neuf transitions

L'apparence identique à deux niveaux adjacents est un échec de recette. Les cinq bandes Brut (N1–4), Soigné (N5–8), Gravé (N9–12), Magistral (N13–16) et Mythique (N17–20) restent la structure artistique de référence, sans imposer des effets magiques universels. Elles ne remplacent pas les différences intermédiaires. Une variation chiffrée invisible, une icône, un texte de niveau ou un effet d'interface ne suffisent pas.

Deux couches complémentaires sont retenues : évolution fine à chaque niveau (poli, rugosité, patine, teinte, gravure, détail de fabrication, selon l'arme) et accent plus marqué aux frontières de bande. Les éléments constitutifs de l'identité restent reconnaissables. Le N1 reste un objet crédible ; le N20 n'est pas nécessairement lumineux. Les variations devront être lisibles sous éclairage de référence et vérifiées dans le donjon, sans exiger une distinction visible sous tous les angles et dans l'obscurité totale.

Le mécanisme technique reste à décider : paramètres interpolés, courbes, masques par partie, ornements ou seuils authorés selon les besoins d'ARM014. Le code actuel de FGridWeaponFinish ne résout que FinishId, RoughnessScale, NormalScale et Tint par bande. Il ne garantit pas vingt rendus distincts. La progression fine n'est donc pas déclarée implémentée. Pas de vingt meshes obligatoires, pas de duplication des fiches, pas de cinquième catalogue, pas de matériau global modifiant simultanément les instances. Le modèle devra être déterministe à ItemId/niveau identiques, indépendant de la caméra, du hasard et de la session.

L'évolution est cosmétique. Elle n'ajoute ni XP d'arme, ni mécanisme d'amélioration, ni rareté, ni enchantement, ni bonus ou lumière de gameplay. Le niveau demeure fixe pour l'instance réelle ; sa simulation ne modifie jamais cet objet. Aucune modification de progression numérique ni SaveGame v27.

## 3. Examen : l'objet seul au premier plan

Le fond par défaut est noir neutre (référence proposée : #000000). Le mesh est réellement éclairé par une scène de preview indépendante : l'objet sombre doit rester lisible, le métal doit conserver ses reflets PBR. Exposition stable, éclairage doux, cadrage automatique selon bounds, rotation et zoom sans clipping. Aucun socle décoratif, meuble, pierre, torche, architecture proche ni VFX d'ambiance envahissant. L'objet peut remplir la vue en zoom rapproché ; ne pas écraser sa silhouette ni masquer ses détails.

Une ambiance 2.5D facultative peut être ajoutée derrière l'objet : image ou plans lointains très atténués, sans concurrence visuelle, sans interaction et sans influence sur le matériau de l'objet. Aucun asset d'ambiance n'est requis pour la V1 ; sa création sera décidée par le propriétaire.

Le contexte Examen de l'atelier reprend ce fond et les conditions de l'écran joueur. Les scènes Sol, Décor, Main, Vol et les éclairages Torche/Donjon restent des outils techniques auteur séparés. Le joueur n'accède pas au laboratoire ou à ses contrôles de simulation. L'éclairage, le cadrage, la taille écran et le fond restent identiques dans une comparaison A/B ; pas de changement d'exposition pour fabriquer une différence de niveau.

## 4. Inspection générique

Les capacités d'inspection sont dérivées de la définition résolue et de la catégorie métier existante ; aucune seconde taxonomie persistante ni liste d'objets parallèle. Le socle énumère les définitions concrètes enregistrées dans DA_ItemSystem. Les archétypes numériques sans définition ne deviennent pas des objets fictifs.

| Famille | Niveau actuel | Présentation et données |
|---|---|---|
| ARM offensif | N1–N20 | Mesh, identité, niveau réel, caractéristiques, actions et conditions applicables. Simulation 1–20 dans l'atelier seulement. |
| ARM passif (bouclier, orbe, etc.) | N1–N20 | Protection et réactions réelles, sans attaque factice ; mêmes capacités de progression visuelle. |
| Équipements futurs (armures, vêtements, bijoux, accessoires) | N0 selon le contrat actuel | Mesh, slots, bonus, résistances et propriétés effectivement définis. Aucune obligation de niveaux ajoutée implicitement. |
| Clés, pierres, objets utilitaires, composants, objets de quête | N0 | Mesh, description, poids, quantité et usage pertinent ; aucun contrôle de niveau. |
| Livres et parchemins | N0 | Mesh, description, informations métier ; action Lire existante préservée, sans duplication du panneau de lecture. |
| Consommables et munitions non ARM | N0 | Mesh, quantité et effets authentiques lorsque disponibles. |

Dans l'atelier, le catalogue et le viewport sont génériques ; le filtre ARM sert à prioriser les armes, pas à exclure les autres objets. Un objet non ARM est résolu à N0 conformément au contrat canonique. Le sélecteur, les bandes, le comparateur de niveaux et les sections d'attaque non pertinentes sont masqués. Les contrôles généraux (orbite, zoom, recadrage, inspecteur) restent accessibles. L'écran joueur utilise le niveau physique réel si applicable ; aucune simulation auteur ni écriture.

Les objets dont le mesh existe doivent être rendus en 3D. Si un objet existant n'a pas de mesh, l'icône et les informations constituent un repli explicite et temporaire ; il reste à fabriquer ou raccorder le mesh avant validation artistique 3D. Pas de faux mesh présenté comme réel. Les deux Readables protégés restent inchangés. Une évolution future autorisant des niveaux pour d'autres catégories exigera une décision normative distincte.

## 5. Contrat de réalisation

- Réutiliser UGridItemSubsystem::ResolveItem et les quatre catalogues ; une projection de présentation commune à l'atelier, à l'examen joueur et au runtime. C++ porte le comportement ; WBP Designer la présentation en jeu ; module éditeur séparé de Shipping.
- Construire un preview isolé, non possédable, non enregistrable, sans effet sur inventaire, équipement, PA, munitions, combats, niveaux, carte ou sauvegarde. Préserver les identités et le schéma v27.
- Essais locaux sur instances transitoires ; signaler tout profil, matériau ou média partagé. Pas d'application implicite, autosave ou modification native hors Unreal.
- L'outil doit permettre de tester un niveau et de comparer ses deux voisins sans changer l'éclairage ou le cadrage. Les champs et ressources réellement résolus sont inspectables ; les effets non raccordés sont signalés.
- Les armes ne sont pas approuvées en masse. ARM014 reste le pilote ; les 21 autres sont travaillées une à une. La production artistique et le choix des valeurs sont soumis à l'approbation du propriétaire.

## 6. Recette complémentaire obligatoire

| ID | Contrôle | Preuve |
|---|---|---|
| INSP-A-T01 | Résolution des 20 niveaux exacts par ARM ; 440 combinaisons pour les 22 ARM actuels | Automation, identité et paramètres effectifs. |
| INSP-A-T02 | 19 transitions visibles par arme (418 pour les 22) ; N1/N20 et frontières de bande | Comparaisons A/B sous mêmes conditions, captures et appréciation du propriétaire. |
| INSP-A-T03 | Rendu stable après changement de niveau, réouverture et rechargement ; deux instances indépendantes | Tests et captures runtime/atelier, aucun changement de données mécaniques. |
| INSP-A-T04 | Fond noir, absence de décor proche, détail lisible jusqu'au zoom maximal ; ambiance éventuelle non intrusive | Recette visuelle atelier, PIE et Shipping. |
| INSP-A-T05 | ARM offensif, passif et objets non ARM : au moins clé, pierre, livre, parchemin ; équipement non ARM disponible et futurs types lorsque matérialisés | Résolution, vraie vue 3D si mesh présent, absence de niveau et sections adaptées ; signaler les meshes manquants. |
| INSP-A-T06 | Lecture intacte, fermeture et restauration du focus ; aucune mutation ni accès auteur en Shipping | Tests et recette sur copies isolées, contrôles fr/en. |
| INSP-A-T07 | Cohérence atelier/examen/donjon ; pas de crash, fuite ou effet parasite | Tests de cycle de vie et captures GPU, écoute et recette native. |

Les tests automatisés prouvent les données et la stabilité, jamais la perceptibilité artistique à eux seuls. Pour ARM014, documenter N1 puis chaque N2–N20 avec son delta visible, les paramètres utilisés, les références et l'approbation. Reproduire ce protocole pour chaque arme. Une recette refusée reste ouverte même lorsque les tests passent.

## 7. Incidence sur INSPECTION02

Le socle et l'atelier V1 doivent être génériques et prévoir dès leur conception le niveau exact, le mode N0 sans contrôles de niveau et le fond noir. La progression fine est un besoin fonctionnel du pilote, et non un nouveau chantier de migration. L'implémentation choisira une solution proportionnée, démontrée sur ARM014, avant de l'étendre. L'atelier doit pouvoir fonctionner avant la finition artistique des 22 armes ; leurs 418 transitions constituent une porte de clôture artistique progressive, pas un prérequis pour ouvrir le premier viewport.

Le découpage technique détaillé, la solution native de rendu et les paramètres artistiques de chaque arme seront établis dans INSPECTION02 et les fiches individuelles. La validation du document n'autorise pas à reconstruire automatiquement les Niagara Systems rejetés ni à modifier les mécaniques. Aucun code, WBP, média ou DataAsset n'est produit dans INSPECTION01A.
