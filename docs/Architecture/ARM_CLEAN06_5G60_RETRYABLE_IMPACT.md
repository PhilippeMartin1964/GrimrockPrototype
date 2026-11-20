# ARM-CLEAN06.5G.60 — Conversion à l'impact sans perte

Parent 614e000b080cadebdcaa659c2ef7dc595a5fa754. Publication après deux statuts SUCCESS.

Une conversion à l'impact était marquée tentative avant le dépôt, puis détruisait son acteur même si tous les replis échouaient. Le projectile reste maintenant arrêté, propriétaire de son GUID et de son niveau, avec tentative suivante après 0,5 s si le runtime existe. Le garde de conversion bloque les notifications réentrantes pendant le transfert et est relâché en cas d'échec. Les passages de fosse et relâchements de flèches posent aussi ce garde avant notification. Le dépôt physique hors fosse matérialise et applique sa pose avant la notification des plaques, sans déclarer un transfert échoué parce qu'un lien a ensuite déplacé le pickup. La route des fosses conserve son comportement.

Le test bloque toutes les cellules de réception du couteau N20, vérifie l'échec non destructif, rétablit les cellules puis appelle le vrai callback de fin de vol et vérifie un seul pickup. Le test n'est pas une preuve de bon rendu du timer ou de tous les scénarios Lua réentrants ; ces vérifications restent requises. Aucun asset/save modifié ; PIE/Shipping nécessaires.

## Correction de validation

Le premier SHA `2e380f11ce4cbd4dc3b68d14f067dac735197163` a compilé mais l'unique test 5G60 a échoué (issue #206, diagnostics publics sans assertion détaillée). L'hypothèse testable est l'invocation indirecte Tick/ProcessEvent dans le monde transitoire avant BeginPlay. Le test appelle directement les vrais gestionnaires natifs via un friend limité à Automation, vérifie que le timer existe après refus et que la cellule réouverte est praticable avant le callback de reprise. Les assertions d'identité, de non-destruction et de restitution unique sont conservées. Aucun changement de comportement de production dans cette correction préalable à la nouvelle validation.
