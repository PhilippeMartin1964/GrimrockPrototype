# ARM-CLEAN06.5G.28 — Retrait du pointeur historique de la palette

## Périmètre

- Retirer `FGridObjectPaletteEntry.DefaultItemDefinition` et `MatchesDirectItemSource`.
- Entrée directe : ItemId canonique, niveau physique explicite ; ni définition d'objet monde, ni icône de substitution, ni source de spawn concurrente.
- Le libellé context-free est l'ID stable ; Slate obtient la présentation auprès du registre canonique.
- La condition Details de l'icône utilise la présence de la définition d'objet monde ; la validation structurelle interdit toute configuration directe mixte.
- Adapter les propositions, l'authoring et les audits à l'absence du membre, sans modifier la palette réelle.

## Preuves à valider

Les audits conservent le contrôle des 33 entrées, des niveaux physiques, de la présentation canonique et de l'intégrité des champs lors des propositions transitoires. Les tests historiques de pointeur sont remplacés par la vérification réfléchie de son absence et les refus de configuration encore représentables (ID inconnu, niveau invalide, source mixte). Aucun package, WBP ni SaveGame modifié.

Parent : `b789cc0eb515f01f204061a472904c86907ef68f`. Ne publier qu'après les deux statuts SUCCESS du parent. Compilation et suite Grimrock Windows requises sur le SHA exact de ce lot.
