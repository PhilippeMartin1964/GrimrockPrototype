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

## ARMEMENT-LOT02.13.6A.2 — Audit critique et correction du vrai contact (29.10.2026)

La suite UE5 locale 14/14 et MON11 8/8 ne détectait pas un bug PIE :
sur une cellule adjacente, une flèche pouvait rester suspendue alors que
le monstre effectuait une animation de mort. Le test ancien instancie un
`AGridMonsterActor` **sans mesh/squelette réel ni animation**, et ne
vérifie que l'attachement et la translation du composant. Il n'est pas
probant sur la pose du monstre en jeu.

**Défaut trouvé :** `TryEmbedInMonster` plaçait la pointe sur la face
avant du `CollisionComponent` logique (boîte 45 cm), alors que la
géométrie réelle est un `SkeletalMeshComponent` pouvant avoir scale,
offset, rotation, pose `Hurt/Death` très différents. Une flèche attachée
à un os correct pouvait être placée *hors du corps* dès son arrivée.
Le correctif précédent (13.6A.1) avait empilé un recalcul de transform
après un `AttachToComponent` et ajouté ~75 lignes non justifiées. **Ce
suivi manuel a été retiré** : Unreal assure le suivi via les sockets/os.

La position est dorénavant calculée depuis la pose visuelle actuelle :
1. point d'approche depuis les bounds du Skeletal Mesh vers le tireur ;
2. `K2_GetClosestPointOnPhysicsAsset` d'UE5.5 : surface animée et os
   associé, **sans activer la collision du mesh** ;
3. pénétration visuelle de 5 cm dans le sens du tir, puis
   `AttachToComponent(... KeepWorldTransform, Bone)` ;
4. repli explicite si pas de Physics Asset : os non racine le plus proche
   dans la pose courante (ou origine visuelle si pas de skeleton), avec
   message `[GridProjectile] EmbeddedNoPhysicsSurface`.

Une nouvelle assertion dans le test 13.6A force un décalage de 130 cm
entre squelette et boîte logique : cela échouait avec le code précédent.
Le test ne simule toujours pas le cas exact d'un montage de mort; la
validation PIE réelle est indispensable.

**Surveiller :** `[GridProjectile] Embedded ... Contact=PhysicsAsset`
est le chemin nominal. Si `Contact=Fallback`, inspecter le Physics Asset
du monstre (collision shapes animées). Ne pas modifier le mesh de flèche.
Ce jalon ne change pas les tirs manqués, dégâts, dégâts mortels,
shurikens ou conversion en pickup après disparition. Gravité et
persistance restent 13.6C.

### Ajustements de revue statique 13.6A.2.1
- Suppression du membre `EmbeddedBone` : il était écrit, jamais lu.
- Les logs `LogicalCenter` contiennent désormais les coordonnées réelles
  de la boîte logique, pas le nom de son composant.
- Dispersion déterministe légère (±6 cm en latéral et vertical) du point
  d'approche pour conserver les flèches multiples visibles ; la surface
  physique/osseuse reste l'autorité pour le point final.

## ARMEMENT-LOT02.13.6A.3 — Correction de l'assertion de récupération (29.10.2026)
Le test `EmbeddedRangedArrowHit` a échoué en local (13/14) sur
`Recovered in target cell`, tandis que MON11 est resté à 8/8.
Il ne s'agit pas d'un échec de compilation : la version précédente
avait rendu le test contradictoire avec le comportement attendu.

La régression décale volontairement le mesh visuel de +130 cm par rapport
au centre logique du monstre, puis déplace l'acteur de +32 cm sur X.
Sur une grille de 200 cm, la pointe passe ainsi de la cellule de combat
(1,2) à la cellule (2,2). `ReleaseEmbeddedArrowToPickup` utilise
le véritable point de libération pour déterminer la cellule enregistrée.

L'ancien test supposait à tort que l'objet devait rester dans (1,2).
Il vérifie maintenant :
- la cellule réellement résolue depuis `GetActorLocation()` juste avant
  disparition du mesh, avec assertion explicite (2,2) ;
- la présence du pickup enregistré dans cette cellule, pas dans (1,2) ;
- l'absence de pickup fantôme dans l'ancienne cellule ;
- une et une seule instance portant le même GUID et ItemDefinitionId,
  dont `RuntimeCellX/Y` correspondent à la cellule de libération.

Aucun changement de physique/gameplay n'est nécessaire pour ce défaut.
La chute Chaos, la transform exacte au sol et la persistance restent à
livrer dans LOT02.13.6C. Les tests automation avec monstre sans véritable
Skeletal Mesh animé **ne suffisent pas à valider** la disparition du
défaut visuel de flèche suspendue : contrôle PIE toujours indispensable.
