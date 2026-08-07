# MON12.8.7 — Barre persistante et affectations d'inventaire

## Résultat

La barre personnelle de dix raccourcis reste visible pendant l'exploration.
Hors combat, elle affiche le personnage actuellement sélectionné dans
l'inventaire. Pendant le tour d'un membre du groupe, elle suit le personnage
actif du `TurnManager` ; pendant un tour ennemi, elle reste visible mais les
actions sont indisponibles.

Les éléments exclusivement liés à la rencontre restent masqués hors combat :

- initiative ;
- panneaux de statut des combattants ;
- PAM ;
- bouton et motif de fin de tour.

Les raccourcis ne deviennent pas exécutables en exploration. Le catalogue les
résout pour permettre leur affichage et leur configuration, puis les marque
indisponibles avec la raison `CombatInactive`.

Lorsque le menu d'inventaire est ouvert, le HUD passe temporairement du niveau
`CombatActionPanelZOrder` au niveau `CombatHotbarConfigurationZOrder`. La barre
et la palette restent ainsi des cibles de dépôt au-dessus du menu, tandis que
le reste du HUD de combat est masqué. Les clics et touches d'exécution sont
bloqués pendant cette configuration ; le glisser-déposer et le clic droit de
suppression restent disponibles.

## Combat à mains nues

`Attack_Unarmed` est désormais une action universelle manuelle permanente. Elle
reste présente dans la palette même si le personnage tient une arme. Le choix
automatique d'une attaque conserve sa priorité équipement avant mains nues,
mais le joueur peut toujours glisser explicitement l'icône « À mains nues »
vers l'un des dix slots.

Le `WBP_GridCombatHud` versionné ne contient pas encore
`Panel_ActionPalette`. Le HUD crée donc automatiquement un `WrapBox` de secours
au-dessus de `Panel_Actions` lorsque ce widget est absent. L'ajout manuel du
panneau dans le Designer reste possible pour maîtriser précisément sa mise en
page, mais il n'est plus nécessaire au fonctionnement.

## Shuriken depuis l'inventaire

Une arme marquée `bThrowable`, possédant un profil offensif valide et présente
dans l'inventaire peut être glissée directement vers la barre. Elle est
normalisée comme action `QuickItem` :

```text
ActionId           Use_<ItemDefinitionId>
SourcePolicy       QuickItem
SourceDefinitionId <ItemDefinitionId>
QuantityCost       au minimum 1
```

L'affectation ne déplace et ne consomme jamais l'objet. L'exécution utilise le
profil offensif de la définition, consomme exactement une unité seulement
après acceptation et conserve le binding à quantité zéro. Une nouvelle unité
de la même définition réactive automatiquement le raccourci.

Pour une présentation `Throw`, le composant de présentation crée un projectile
récupérable à partir de l'objet d'inventaire. Cette création est visuelle et ne
mute pas l'inventaire ; la consommation reste autoritaire dans le
`TurnManager`.

## Tests automatisés

Filtre :

```text
Grimrock.Monsters.MON12.8.7
```

Les tests vérifient :

1. visibilité des dix slots hors combat ;
2. sélection de la barre personnelle depuis l'inventaire ;
3. masquage des commandes exclusivement liées au combat ;
4. blocage des clics et touches d'exécution pendant l'ouverture de l'inventaire ;
5. création automatique de la palette absente du WBP ;
6. affectation manuelle de `Attack_Unarmed` hors combat ;
7. dépôt d'un shuriken non équipé depuis l'inventaire ;
8. résolution et exécution de son profil offensif ;
9. consommation d'une seule unité après acceptation ;
10. création d'un projectile récupérable sans consommation par la présentation.
