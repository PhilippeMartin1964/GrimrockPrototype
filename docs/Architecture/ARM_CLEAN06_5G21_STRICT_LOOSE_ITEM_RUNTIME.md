# ARM-CLEAN06.5G.21 — Matérialisation runtime des objets au sol

Précondition : b6562b17a3dd618f158202c874913cb012534f34 validé. Les 44 placements natifs sont déjà pointerless (X.46/X.48). Le runtime AddPlacedItemActor s'aligne sur l'aperçu strict de 5G.20.

- Refuser ancien pointeur même concordant, GUID invalide, ID vide/inconnu, quantité non positive, registre absent et niveau physique invalide. Aucune création partielle ni GUID de remplacement, aucun clamp de quantité ou héritage de N0 ARM. Le rendu, mesh, transform, physique, Niagara et lumière de départ viennent exclusivement de la projection canonique.
- Garder coordonnées, offsets, readable content, GUID et quantité, entrée runtime et capture existants. Aucun changement de schéma SaveGame ni des chemins de chute, drop, transfert ou restauration.
- X.8 maintient les vérifications ARM/nonARM réelles, N1 et N20, mesh/transform/lumière/Niagara et ajoute le refus même pour une ancienne source concordante, une quantité nulle, un ID inconnu et un GUID invalide.
- LIGHT02 conserve le test de torche allumée avec le vrai Item_Torch ; MON11.4.2 utilise le Shuriken N0 réel avec le registre configuré pour vérifier l'unicité des reconstructions. ARM0181 maintient les deux chemins de pickup et le roundtrip monde/sauvegarde avec N1 explicite, N9 et rejet ARM N0. Les assertions dédiées aux anciens transferts synthétiques hors registre ne sont pas retirées.
- Le scanner historique de ResolveRuntimeItemDefinition reste provisoirement en place pour les fixtures TD021/MON et autres chemins de transfert non encore retirés. Sa suppression, l'authoring strict et le retrait du champ FGridLooseItemInstance.ItemDefinition feront l'objet du lot suivant, distinct de la matérialisation.
- Aucun package UE, SaveGame, WBP ou texte joueur modifié. Le contrôle visuel PIE et Chaos reste obligatoire en 06.6.
