# ARM-CLEAN06.3W.1 — Classification des flèches invalides avant fallback

**Incident observé :** issue GitHub [#115](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/115) sur le commit `4b30e47`, `UE5.5.4/LocalAuto = failure`, **un** test échoué : `Grimrock.RPG.ARM.CLEAN06_3.UnlevelledAmmoFlightMesh`.

## Diagnostic et correction

`FGridCanonicalProjectileVisualReadModel::Resolve` rejetait `Instance.IsValid() == false` **avant** le test d'appartenance à `DA_ItemSystem`. Un ItemId de flèche connu avec un niveau N-1 devenait alors `bCanonicalIdentity == false`. Le test 06.3W attend explicitement l'invariant correct : une identité canonique connue reste canonique, même quand son état d'instance est invalide, et **ne peut pas emprunter un ancien mesh**.

Le résolveur reconnaît désormais l'ItemId dans `UGridItemSubsystem` **avant** de contrôler le GUID, la quantité ou le niveau de l'instance. Les instances invalides sont classées `bCanonicalIdentity = true` mais restent `bResolved = false`, avec mesh vide ; les identités non enregistrées restent hors registre. Le vrai `DA_ItemSystem`, le gameplay, les niveaux N0/N1, les flèches récupérables et les sauvegardes restent inchangés.

Le test défaillant est **conservé** et étendu au cas de GUID absent. Aucun contournement, test désactivé ou assertion assouplie. La validation complète doit redevenir verte sur le nouveau SHA, après publication via VALIDATION-AUTO01.

Aucun WBP, traduction, chaîne joueur ni asset binaire modifié. **UI.csv → ST_UI → WBP → Game/fr/en : inchangé.**
