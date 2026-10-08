# UI-RPG-DESC01.13 — Audit statique C++ des 90 Talents

Date : **8 octobre 2026**  
Révision auditée : **d68e09fc38183f5221097fa02b675cf4eed8efda**  
Nature : **audit statique du dépôt — aucun build/test UE relancé dans ce jalon**

## 1. Conclusion

Le code C++ contient bien les **90 identités conceptuelles** attendues :

~~~text
RPGWarriorAuthoring.cpp    15 Talent_*
RPGRogueAuthoring.cpp      15 Talent_*
RPGRangerAuthoring.cpp     15 Talent_* conceptuels
RPGMageAuthoring.cpp       15 Talent_* conceptuels
RPGPriestAuthoring.cpp     15 Talent_*
RPGAlchemistAuthoring.cpp  15 Talent_*
TOTAL                      90
~~~

Les helpers MakeChoice fixent PointCost = 1 et réutilisent le ChoiceId comme TalentNodeId sauf pour les variantes groupées.

Les tests source existants décrivent aussi la structure de production : Guerrier 17 Choice records, Voleur 15, Rôdeur 14 + N catégories de bestiaire, Mage 21, Prêtre 15 et Alchimiste 15. Les tests globaux attendent 15 nœuds conceptuels par classe et 90 au total.

Cela confirme la **présence structurale 90/90**.

## 2. Limite de cette vérification

Cet audit lit les six authoring C++, les tests authoring/support existants, les primitives runtime appelées par les Talents et les six audits DESC01.6-.11 validés.

Il ne remplace pas une exécution locale UE5.5.4. Conformément à la politique du projet, aucun test n'est déclaré passé ici sans sortie ValidateUE.ps1 fournie par l'utilisateur.

## 3. Résultat mécanique consolidé

~~~text
85 / 90 nœuds : mécanique C++ conforme au contrat v0.1 normalisé
 5 / 90 nœuds : divergence ou raccordement runtime incomplet encore identifié
90 / 90 nœuds : identité/structure authorée présente
~~~

Les 7 nœuds ne sont pas absents : ils existent dans l'authoring, mais une partie de leur comportement ne correspond pas encore entièrement au contrat validé.

## 4. Guerrier — 15 conformes

| Talent | Audit C++ |
|---|---|
| Posture défensive | OK |
| Coup de bouclier | OK |
| Interception | OK — règle multi-intercepteurs canonisée |
| Rempart | OK |
| Forteresse | OK |
| Coup puissant | OK |
| Brise-armure | OK |
| Balayage | OK |
| Exécution | OK |
| Ravage | OK |
| Spécialisation martiale | OK — 3 Choice exclusifs |
| Riposte | OK — mains nues autorisées |
| Second souffle | OK — D01 clos : `NoApplicableEffect` à PV maximum via le catalogue générique |
| Maîtrise critique | OK |
| Seigneur de guerre | OK |

## 5. Voleur — 12 conformes / 3 partiels

| Talent | Audit C++ |
|---|---|
| Attaque sournoise | OK |
| Frappe dans le dos | OK |
| Hémorragie | OK |
| Point faible | OK |
| Mise à mort | OK mécanique — DisplayName C++ historique « Finisseur » |
| Esquive | OK |
| Disparition courte | OK |
| Pas de l'ombre | OK selon décision v0.1 : portée +1 |
| Insaisissable | OK |
| Ombre parfaite | OK |
| Désamorçage expert | **D02 — primitive conforme ; consommateur métier différé au vrai runtime Lock/Trap** |
| Piège rapide | OK |
| Bombe fumigène | OK |
| Maître des serrures | **D02 — primitive conforme ; consommateur métier différé au vrai runtime Lock/Trap** |
| Sabotage | **D03 — combat OK ; caller monde différé au flux générique d'aptitudes hors combat** |

Le catalogue Skills de production n'est plus un trou : Crochetage, Pièges / désamorçage et Mécanique existent.

## 6. Rôdeur — 15 conformes

| Talent | Audit C++ |
|---|---|
| Tir précis | OK |
| Tir perforant | OK |
| Tir rapide | OK |
| Volée | OK |
| Œil d'aigle | OK |
| Marque de la proie | OK |
| Ennemi juré | OK gameplay — variantes dynamiques |
| Tir immobilisant | OK |
| Frappe du prédateur | OK |
| Chasseur alpha | OK |
| Vigilance | OK |
| Piège de chasse | OK |
| Repli tactique | OK |
| Maître du terrain | OK |
| Guide du groupe | OK |

D04 reste une dette de présentation : les variantes Ennemi juré sont encore nommées depuis le CategoryId brut dans l'authoring. Ce n'est pas une divergence de mécanique.

## 7. Mage — 15 conformes

| Talent | Audit C++ |
|---|---|
| Affinité élémentaire | OK — 4 variantes |
| Surcharge élémentaire | OK |
| Explosion contrôlée | OK |
| Chaîne élémentaire | OK |
| Cataclysme | OK selon friendly fire canonisé |
| Bouclier arcanique | OK |
| Dissipation | OK |
| Manipulation runique | OK |
| Téléportation courte | OK |
| Maîtrise de l'Arcane | OK |
| Imprégnation | OK — 4 variantes indépendantes des variantes Évocateur |
| Conversion élémentaire | OK |
| Conduction | OK |
| Surface persistante | OK |
| Architecte du terrain | OK selon sémantique Terre canonisée |

Les Skills Arcane et Runes existent en production.

## 8. Prêtre — 15 conformes

| Talent | Audit C++ |
|---|---|
| Soin renforcé | OK |
| Régénération | OK |
| Soin de groupe | OK |
| Purification | OK |
| Miracle | OK |
| Bénédiction | OK |
| Égide | OK |
| Protection sacrée | OK |
| Sanctuaire | OK |
| Bastion divin | OK |
| Lumière sacrée | OK |
| **Repousser les morts-vivants** | **OK — D05 clos : 4 dégâts sacrés fixes, aucun scaling Sagesse/Skill** |
| Dissipation sacrée | OK |
| Châtiment | OK |
| Exorcisme majeur | OK — destruction future d'invocations faibles volontairement absente |

## 9. Alchimiste — 13 conformes / 2 partiels

| Talent | Audit C++ |
|---|---|
| Bombe incendiaire | OK |
| Bombe toxique | OK |
| Charge précise | OK |
| **Réaction en chaîne** | **D06 SOURCE CORRIGÉE — SurfaceEffects Feu/Glace raccordés au pipeline SurfaceReaction ; validation locale requise** |
| Maître grenadier | OK |
| Potion renforcée | OK |
| Antidote | OK |
| Élixir défensif | OK — 4 recettes, pas des variantes |
| Diffusion | OK selon sémantique 50 % canonisée |
| Panacée | OK |
| Huile glissante | OK |
| Flasque acide | OK |
| Nuage corrosif | OK |
| Catalyseur | OK |
| **Transmutation majeure** | **PARTIEL D07/D08 — durée d'une surface existante et bonus +50 % de réaction non conformes/complets** |

## 10. Dépendance Crafting D09

Neuf nœuds Alchimiste accordent un ou plusieurs droits Recipe_*.

Le C++ authoré possède les GrantedRequirementIds Recipe_*, les ItemDefinitions QuickItem des huit consommables simples, leurs actions/effets et le helper de contribution de Transmutation majeure.

Mais il n'existe pas encore de moteur complet recette + ingrédients + fabrication + consommation des ingrédients. D09 est donc une dépendance système orthogonale. Elle n'est pas comptée parmi les 7 divergences Talent ci-dessus, sauf Transmutation majeure qui possède en plus D07/D08.

## 11. Écarts de présentation C++ actuels

Le gameplay 83/90 n'implique pas que le read-model actuel respecte DESC01.12. Le code UI actuel contient encore :

~~~text
EffectCategory : FText inféré
TYPE = CHOIX DE VARIANTE
ACTION DISPONIBLE
ACTION ACCORDÉE APRÈS ACQUISITION
MakePlayerReadableText()
HumanizeId()
CooldownRounds affiché comme tour(s)
ComboBox de variante pendant l'acquisition
~~~

Ces éléments sont des dettes du read-model/UI, pas des erreurs dans les 83 Talents gameplay conformes.

## 12. Corrections gameplay à ne pas mélanger à DESC01.14

DESC01.14 doit implémenter le read-model, pas réparer silencieusement le gameplay.

~~~text
D01  Second souffle — CLOS (garde-fou générique + test spécifique)
D02  échecs sûrs Pièges / Crochetage — dépendance du futur runtime Lock/Trap
D03  Sabotage monde — dépendance du futur flux générique d'aptitudes hors combat
D05  Repousser les morts-vivants — CLOS (4 dégâts fixes, 21/21 RPG03.9.5)
D06  Réaction en chaîne — source corrigée ; validation RPG03.9.6A/B1 requise
D07  Transmutation majeure — durée 4 rounds
D08  Transmutation majeure — décision/raccord +50 % réaction
~~~

D04 est présentation bestiaire. D09 est le futur Crafting.

## 13. Niveau de confiance

~~~text
Identité 90/90                   ÉLEVÉ
Branches / tiers / coût          ÉLEVÉ
Variantes structurelles          ÉLEVÉ
Actions / modifiers / reactions  ÉLEVÉ
Conformité 85/90                 ÉLEVÉ sur audit statique + D01/D05 validés
Exécution UE au commit courant   NON REVALIDÉE dans ce jalon
PIE                              NON REVALIDÉ
~~~

La prochaine validation exécutable interviendra après une modification C++, pas pour ce jalon documentaire.