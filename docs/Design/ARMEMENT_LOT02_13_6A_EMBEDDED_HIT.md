# ARMEMENT-LOT02.13.6A — Flèche fichée sur réussite

## Contrat implémenté
Une flèche d'arc physique et récupérable ne devient plus immédiatement un pickup
au terme d'un trajet vers le monstre **si** le résultat autoritaire est un Hit
et le monstre possède un SkeletalMesh visible. La pointe de la flèche est
positionnée sur la face du volume de collision tournée vers le tireur,
avec pénétration visuelle de 8 cm. Un décalage déterministe issu du GUID
répartit les impacts, sans aléatoire de combat ni nouveau dégât.
La flèche s'attache au SkeletalMesh, sur l'os le plus proche s'il existe ;
elle suit donc les déplacements et animations de sa cible. Le mouvement
projectile, la gravité, la collision et le timer d'expiration s'arrêtent.
L'ItemInstance unitaire et son GUID restent uniques et inchangés.

## Limite volontaire A vs B/C
- Miss : comportement précédent inchangé. Déviation selon règle du T : 13.6B.
- Hit contre mur avant la cible : pickup précédent inchangé.
- Monstre et sorts visuels, shurikens : aucun changement de politique.
- À la disparition du mesh du monstre, l'acteur fiché se convertit pour le
  moment en pickup ordinaire (sécurité contre la perte). **La chute naturelle,
  son orientation physique et la persistance des flèches encore fichées ne
  sont pas livrées ici** ; elles nécessitent 13.6C.
- Si un monstre ne possède pas de mesh visible au moment de l'arrivée,
  la récupération classique est conservée.

## Vérification
Nouveau test : `Grimrock.RPG.ARMEMENT.LOT02.EmbeddedRangedArrowHit`,
incluant attachement, suivi mouvement, absence de duplication et libération
lors du masquage de la cible. Puis exécuter tous les tests LOT02 et MON11.
Recette PIE : tir réussi dans les quatre orientations, observer la pointe
fichée et le suivi du monstre animé ; tuer le monstre et vérifier qu'une
seule flèche apparaît au sol à sa disparition. Save/Load d'une flèche
fichée non couvert par ce jalon.

Aucune modification `.uasset` ni des dommages ou PA.
