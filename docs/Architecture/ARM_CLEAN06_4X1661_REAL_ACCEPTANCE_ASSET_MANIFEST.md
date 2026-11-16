# ARM-CLEAN06.4X.16.6.1 — Manifeste nominatif des quatre sources réelles

## Baseline vérifiée depuis le rapport UE Windows de X.16.5

Source : lignes de synthèse fournies par l'utilisateur, issues du
run f12487b61dd0 sous UE 5.5.4 :

| Mesure | Réceptacles | Serrures |
| --- | ---: | ---: |
| Définitions natives inspectées | 44 | 44 |
| Listes de règles de définition | 6 | 2 |
| Listes sûres | 6 | 2 |
| Pointeurs legacy convertibles | 2 | 2 |
| Source absente/inconnue/contradictoire | 0 | 0 |
| Surcharges de niveau | 0 | 0 |
| IDs déjà persistés | 0 | 0 |

Quatre niveaux sont inspectés. Seules deux règles de
réceptacles et deux références de clés sont candidates.
Le fait qu'il existe six listes de réceptacles ne signifie
pas six règles à modifier ; les quatre autres listes sont vides.

## Nouveau contrôle en lecture seule

Test :
Grimrock.RPG.ARM.CLEAN06_4X1661.RealAcceptanceDefinitionManifest

Le test recharge les 44 vrais assets de définition, les
quatre niveaux et DA_ItemSystem, refuse toute dérive
des comptes ci-dessus, exécute les deux préflights
atomiques validés (X.16.4 et X.16.5) et imprime **une
ligne par référence** : surface, package UE, DefinitionId,
index, ancien ItemId et ItemId canonique proposé.

Il n'écrit ni asset, ni map, ni sauvegarde, ni script
de migration. Les pointeurs historiques restent intacts.
Les lignes ainsi produites doivent servir au manifeste
de fichiers, au calcul des empreintes SHA-256 et aux
sauvegardes privées avant conversion.

### Extraction PowerShell read-only

Rechercher le run de validation du commit X.16.6.1
dans les dossiers privés de l'agent Windows, puis
extraire exclusivement les enregistrements
[ARM-CLEAN06.4X.16.6.1] RECEPTACLE, LOCK et SUMMARY.
Ne transmettre ni chemins locaux absolus ni logs complets.

La migration ultérieure sera séparée :
1. réceptacles : écrire seulement les deux CanonicalItemId ;
2. serrures : ajouter deux AcceptedKeyIds en préservant
   leur union et leurs pointeurs durant la transition.

Sans le manifeste nominatif réel, ne jamais émettre une
commande -Apply, ni supposer quels fichiers binaires
modifier. Ne pas rejouer les migrations 06.4W / X.5.
