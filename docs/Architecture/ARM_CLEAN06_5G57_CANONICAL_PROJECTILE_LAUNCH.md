# ARM-CLEAN06.5G.57 — Autorité canonique du lancement

Parent 5ee2bc3c62f3be4d0dd26a0382eb969d14bffab5. Publication après deux statuts SUCCESS.

Le lanceur exige exactement une unité physique, un ItemId et niveau résolus sans réinterprétation et un profil canonique valide. Un ancien asset correspondant à un ID inconnu ne peut plus autoriser un lancement en production. L'initialisation directe de l'acteur applique elle aussi le refus de source non canonique et ne lit plus le pointeur historique lorsque l'identité est connue. Le fallback reste limité aux fixtures Automation actives sans GameInstance. Les règles de vol, balistique, collisions et permissions par famille restent inchangées.

Le test 3AV InvalidAndLegacy vérifie le refus d'un ancien asset correspondant à un ID inconnu et du lancement d'une quantité de deux armes. Les lecteurs de balistique historiques, la capture des lancers non munitions et l'audit de la consommation des projectiles restent distincts. Aucun asset/save modifié ; validation Windows et PIE/Shipping requises.
