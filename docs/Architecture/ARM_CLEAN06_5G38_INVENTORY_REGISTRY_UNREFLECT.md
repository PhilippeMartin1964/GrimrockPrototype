# ARM-CLEAN06.5G.38 — Retrait de la réflexion du registre historique d’inventaire

Précondition : parent 824e6a2b7d277c33f904d5125a4696cea56522fb, SilentTask et LocalAuto SUCCESS. Le scan MIG09 de 5G.37 n’a signalé aucun appel direct dans son périmètre.

- Retirer uniquement BlueprintCallable/UFUNCTION de RegisterItemDefinition et FindItemDefinition. Conserver leurs signatures et implémentations C++ ; ne pas modifier les chemins d’inventaire, de Start/Continue ni les fixtures.
- Conserver les six noms interdits dans le scan MIG09 et ajouter deux assertions de non-réflexion. Vérifier positivement que ApplyItemDefinitionToInstance reste réfléchie.
- Aucun changement de package Unreal, SaveGame, schéma de données ou comportement natif. Le scan ne prouve pas l’absence d’appels dynamiques ou hors périmètre.

Validation requise : UE5.5.4/SilentTask et UE5.5.4/LocalAuto sur le SHA publié ; suite Grimrock incluant MIG09. Ne pas supprimer les méthodes natives avant audit de leurs appelants.

## Correctif après validation Windows

Issue [#205](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/205) : un seul test en échec sur 17a2d9fe562652d8f2d81345f6805c0d8f3b6c9a, TD06.8 Contract. Ce test exigeait encore la réflexion BlueprintCallable des deux fonctions retirées. Le correctif adapte les assertions de non-réflexion et conserve le contrôle positif BlueprintCallable de ApplyItemDefinitionToInstance. Toutes les assertions fonctionnelles natives demeurent inchangées. Le rapport public ne contient pas les assertions brutes ; la cause est identifiée par inspection du code. La réussite du correctif doit être confirmée sur son SHA exact.
