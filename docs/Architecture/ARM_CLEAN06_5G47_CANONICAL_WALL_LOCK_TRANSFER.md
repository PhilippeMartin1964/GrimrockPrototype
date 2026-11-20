# ARM-CLEAN06.5G.47 — Serrure murale : transfert physique canonique

Parent 6f70b0999121c74185a0080be03d2a7a03072cce. Publication conditionnée aux deux statuts SUCCESS.

La règle d’acceptation ID-only reste distincte de la validité physique : une serrure peut déclarer un ID sans que celui-ci soit présent dans ItemSystem. CanAcceptKeyInstance impose la résolution de l’instance réelle, le niveau exact, la capacité de pile et la disponibilité de la serrure avant toute extraction depuis le sac ou insertion depuis le curseur. Le contexte transfère exactement une unité ; le visuel ne consulte plus FindItemDefinition et n’utilise que la projection canonique. Aucun changement de la liste d’IDs authorée, du comportement des liens, de la consommation historique de la clé ou des textes joueur.

X30 conserve les assertions d’identité, mesh, lumière, Niagara et collision ; ajoute les refus d’une ancienne définition explicitement acceptée, d’une pile hors capacité, la conservation du sac et le transfert positif. Les possibilités de mutations réentrantes pendant l’émission des liens restent à auditer : ce lot ne crée pas un nouveau protocole transactionnel de notifications. Les réceptacles génériques, notamment leurs scripts et classification EGridItemType, restent hors périmètre. Aucun asset, sauvegarde ou schéma modifié ; validation Windows et recette PIE/Shipping à confirmer.
