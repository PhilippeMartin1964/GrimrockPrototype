# UI-TEXT-CENTRAL01.2D3 — Legacy metadata cleanup

Date : **16 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**  
État : **VALIDATION LOCALE OK — ST_UI matérialisé à versionner**

## Objectif

Après la purge physique 01.2D2, retirer des fichiers d'authoring actifs les
dernières métadonnées qui présentaient encore l'ancien asset Talent comme
source ou provenance.

Aucun document historique d'audit n'est réécrit : ils restent des traces
chronologiques de migration. Le nettoyage vise uniquement l'état courant.

## UI.csv

162 commentaires de clés Talent utilisaient encore une provenance historique.
Ils sont remplacés sans modifier les clés ni les textes joueur :

~~~text
avant
Vocabulaire UI Talent centralisé depuis [ancienne autorité supprimée]

après
Vocabulaire statique présenté dans l'interface Talent
~~~

Cette modification ne change aucune valeur source, aucun placeholder et aucun
identifiant de String Table.

## Garde anti-régression

Le test :

~~~text
Game.UI.TextCentral.CENTRAL01.TalentLegacyMetadata
~~~

charge `Localization/Source/UI.csv` et refuse les identifiants de l'ancien
pipeline Talent ainsi que l'ancien préfixe de compatibilité Recipe.

## Validation locale

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Game.UI.TextCentral.CENTRAL01.TalentLegacyMetadata"

.\Scripts\Localization\RunLocalizationSmoke.ps1 `
    -EngineRoot D:\UE_5.5
~~~

Le premier smoke après modification des commentaires matérialise `ST_UI` avec
`Changed=true`. C'est attendu : la colonne `Comment` de `UI.csv` est stockée
comme métadonnée `Comment` dans la String Table et fait partie du contrat
d'égalité du materializer.

Après versionnement du `ST_UI.uasset` matérialisé, un nouveau passage direct du
commandlet doit devenir idempotent avec `Changed=false`.

Validation locale observée le 16 octobre 2026 :

~~~text
TalentLegacyMetadata   1/1 — 0 warning — 0 échec
UI-LOCALIZATION01.6        — 0 erreur — 0 warning
Sync_ST_UI                 — Changed=true (attendu pour cette migration de métadonnées)
English smoke              — PASS
~~~

