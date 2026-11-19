# ARM-CLEAN06.5G.13 — Serrures : AcceptedKeyIds comme autorité unique

- Les deux serrures natives cuivre/fer ont déjà été converties et X.48 contrôle zéro référence historique dans les données actives.
- Le lecteur runtime n'utilise plus AcceptedKeyItems, même si AcceptedKeyIds est vide. Les IDs de script restent valides, les doublons et NAME_None ignorés comme avant.
- La classification de type serrure ne dépend plus de l'ancien tableau. La copie d'un override refuse atomiquement toute source historique : aucun nouveau droit d'ouverture ne peut être inventé par projection d'un ancien pointeur.
- X.29 teste le refus en présence de pointeurs contradictoires, sans IDs et avec IDs invalides ; X.50 teste le refus atomique de sources historiques, le maintien des IDs explicites et des règles de consommation, et la copie des deux assets réels. Les anciennes propriétés réfléchies restent temporairement disponibles pour les preuves d'audit et le prochain lot de retrait du schéma.
- Aucun asset, SaveGame, Blueprint ou WBP modifié. Deux statuts Windows exact-SHA requis avant publication du lot suivant.
