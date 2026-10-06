# RPG03 — Audit final des quatre fils de travail

Date : **6 octobre 2026**  
Périmètre : arbre de compétences / RPG03 / six classes / 90 talents  
État de référence avant audit : `master` validé `Grimrock.RPG.RPG03` **193/193**, 0 warning, 0 échec, exit 0.

## 1. Fils revus

### « Arbre de compétence RPG »

Ce fil a fixé puis développé le cœur RPG03 : profils génériques C1→C8, attaques de talents basées sur l'arme, puis authoring classe par classe. Il a confirmé la règle structurante : **C++ = primitives génériques/runtime, DataAssets = authoring, aucun switch sur TalentId**.

Éléments marquants :

- RPG03.1 introduit `FGridCombatModifierProfile` ;
- RPG03.9.1 Guerrier : 15 talents conceptuels, 17 Choice records, 10 actions, 5 statuts ;
- RPG03.9.2 Voleur : 15 talents et authoring de ses statuts ;
- RPG03.9.3 Rôdeur : correction générique de portée d'arme dynamique ;
- Mage, Prêtre et Alchimiste ont ensuite été découpés en socles génériques puis authoring/matérialisation.

### « Validation locale RPG038 »

RPG03.8 y a été validé **8/8**, fermant C1→C8 avant l'authoring des classes. Le fil a ensuite grossi jusqu'à provoquer des expirations de réponse ; le processus a été corrigé en découpant chaque classe en opérations bornées : audit, primitive générique si nécessaire, authoring, matérialisation, validation.

Cet incident était un problème de taille/orchestration du travail, pas une preuve d'échec du code. Le découpage borné a évité de rejouer des opérations Git déjà effectuées.

### « Audit Git RPG03.2A »

Ce fil a surtout servi de garde-fou Git et d'état réel du dépôt : vérification de `master`, de `origin/master`, du working tree et de la présence réelle des assets avant de poursuivre. Il a notamment évité de rematérialiser des classes déjà présentes et a mis en évidence les cas où le code d'authoring était commité avant les `.uasset`.

### « Audit du dépôt RPG03 »

Le fil final a terminé l'Alchimiste, matérialisé les six classes, puis créé RPG03.10. La première campagne globale a volontairement révélé plusieurs dettes historiques qui n'apparaissaient pas dans les validations par sous-jalon.

## 2. Échecs/reprises significatifs et résolution

| Incident | Cause | Résolution |
|---|---|---|
| RPG03.9.6A ne compile pas | forward declaration manquante pour `FGridResolvedCombatModifiers` | correctif ciblé `15453a14` |
| RPG03.9.6B1 ne compile pas | tentative d'assigner un `UObject` : `Item = UGridItemDefinitionAsset()` | reset explicite des champs, `271f20fc` |
| campagne RPG03.10 quitte avant rapport | fixture RPG03.1 sélectionnait un talent sans budget puis indexait `Profiles[0]` vide | fixture corrigée et accès sécurisé, `b94f5a12` |
| six classes avec 0 Talent Point grants en production | les tests de branches injectaient localement des grants et masquaient l'absence dans les `DA_Class_*` | autorité commune `FRPGClassProgressionAuthoring`, matérialisation des six classes, `19e09072` + `557b75b6` |
| RPG03.3 et RPG03.4 échouent dans la campagne globale | deux fixtures historiques devenues incompatibles avec les contrats C3/MON15 actuels | correctif tests-only `36433596` |
| threads longs / timeouts | opérations trop larges et trop nombreuses dans une réponse | découpage classe/ticket/commit borné |

Résultat final avant cet audit : **193/193**.

## 3. Cohérence du code constatée

La recherche statique finale ne trouve aucun identifiant `Talent_*`, `Action_Warrior_*`, `Action_Rogue_*`, `Action_Ranger_*`, `Action_Mage_*`, `Action_Priest_*`, `Action_Alchemist_*` ni `Status_*` spécifique dans les répertoires Runtime/RPG de production. Les identités de classe restent dans les DataAssets/authoring/tests.

Les six authorings appellent désormais `FRPGClassProgressionAuthoring::ConfigureCanonicalTalentGrants()`, ce qui supprime la double autorité qui avait permis aux assets de production de rester sans grants.

Les commandlets et scripts `AuthorRPG*.ps1` sont **conservés volontairement** : ils rendent les assets reproductibles depuis la source d'authoring et ne sont donc pas du code mort.

`BuildMajorTransmutationRecipeAction()` n'est pas du runtime mort : c'est une frontière Editor explicitement testée pour le futur Crafting. La supprimer obligerait à réinventer le contrat de Transmutation majeure plus tard.

## 4. Code mort supprimé par l'audit

RPG03.11A supprime cinq blocs de fixture devenus sans effet :

- Mage authoring ;
- Prêtre authoring ;
- Prêtre Exorcisme ;
- Alchimiste B1 ;
- Alchimiste B2.

Ces fixtures ajoutaient manuellement des grants `2/6/10/14/18`, puis appelaient `ConfigureClass()`, lequel les effaçait immédiatement pour installer les grants canoniques `2,4,6,...,20`. Les blocs étaient donc réellement morts et pouvaient induire en erreur.

Deux commentaires d'API Prêtre/Alchimiste encore rédigés comme si B2 était futur ont également été corrigés.

Aucun code runtime de production n'a été supprimé par cet audit.

## 5. Dette documentaire corrigée

Avant l'audit :

- C8 était encore marqué « validation requise » ;
- plusieurs jalons Mage/Prêtre/Alchimiste étaient encore « à matérialiser/valider » ;
- RPG03.10 était encore « validation requise » ;
- `RPG_Talents_Mechanics_v0_1.md` disait « implémentation à venir » et parlait du futur contrat Surface ;
- le document de progression mélangeait le volet Talents terminé avec le volet Skills/Level Up encore ouvert ;
- le Guerrier RPG03.9.1 n'avait aucune fiche `docs/Design/RPG03_9_1_*`.

RPG03.11B corrige ces états et ajoute une synthèse transversale.

## 6. Limites qui restent ouvertes

Le code/data des Talents est cohérent et la campagne globale est verte, mais deux sujets restent volontairement hors clôture technique RPG03 :

- **Skills / Level Up** : aucun `URPGSkillAsset` de production n'est présent dans `Content/` ; achat transactionnel et écran complet restent à faire ;
- **profondeur de validation gameplay** : le PIE actuel est un smoke test réel de progression/projection des six classes, pas l'exécution exhaustive des 90 talents via UI/hotbar/targeting.

Ces limites doivent être traitées dans de futurs jalons sans rouvrir un second système de Talents.
