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

## ARMEMENT-LOT02.13.6A.1 — Suivi du corps animé (régression en cellule adjacente)
Une recette PIE a révélé une flèche parfois suspendue dans le vide pendant
l'animation du monstre, notamment pour une cible à une case, puis libérée lors
du dissolve. La cause plausible est un ancrage trop permissif sur l'os racine
ou une mise à jour de la pose non suivie. La cause précise reste à confirmer
par PIE ; aucun journal d'os fautif n'était disponible.

Renforcement ciblé :
- Sur les rigs à plusieurs os, choisir l'os **non racine** le plus proche
  de la pointe plutôt que permettre l'os racine statique.
- Conserver la transformation monde de la flèche relative à cet os, et la
  réappliquer si nécessaire après la mise à jour de la pose du squelette.
- Établir une dépendance d'ordre de tick avec le SkeletalMesh ; tolérer
  l'absence de squelette dans les tests en s'attachant au composant.
- Lors de la disparition d'un os (réinitialisation du mesh), replier vers
  le composant pour préserver l'objet, puis garder la libération sans
  duplication et la conservation du GUID comme dans 13.6A.
- Test transitoire : une cible à une case dont le composant visuel change
  de position indépendamment de l'acteur doit déplacer la flèche fichée.

Le détail du mouvement et la vérification des montages de mort animés
nécessitent une recette PIE ; le test automation sans SkeletalMesh importé
ne prouve pas encore un suivi exact de chaque os de WereRat ou autre monstre.
