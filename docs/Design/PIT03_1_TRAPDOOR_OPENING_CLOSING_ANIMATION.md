# PIT03.1 — Trapdoor Opening / Closing Animation

> **SUPERSEDED le 02.09.2026 par PIT03.2 — Dual-Leaf Pit Trapdoor.**

Le modèle PIT03.1 utilisait un seul `Moving Mesh` et une seule `Open Relative Rotation`.

Ce modèle a été supprimé à la demande de conception : les fosses contrôlées utilisent désormais exclusivement deux volets indépendants avec deux charnières.

Ne plus configurer pour une Pit :

```text
Moving Mesh
Moving Material
Open Pitch
Open Yaw
Open Roll
Open Relative Rotation
```

Référence actuelle :

`docs/Design/PIT03_2_DUAL_LEAF_TRAPDOOR.md`
