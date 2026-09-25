# MON14.3.2 — Superseded par MON14.3.3

Le bootstrap ajouté pendant le diagnostic de la régression de patrouille a été supprimé par `MON14.3.3 — Monster Movement Authority Cleanup`.

La cause racine était un état d'awareness incohérent (`Alert/Pursuing` sans perception ni `LastKnownPartyCell`), pas l'absence d'un second mécanisme d'amorçage.

Voir `MON14_3_RUNTIME_PATROL_INVESTIGATION.md` et le journal de décisions.
