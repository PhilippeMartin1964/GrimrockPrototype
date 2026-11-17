# ARM-CLEAN06.4X.52 — Tests natifs compatibles avant/après les quatre règles partagées

**Parent :** `9ab587af5d7c80aac6041eeb9a72ed70309b3f75`, X.51 validé par les deux statuts Windows.
**Nature :** mise à jour ciblée des tests et vérificateurs de schéma
avant conversion binaire, sans changement de runtime, d'editor gameplay,
de `.uasset` ni de SaveGame.

## Correction proactive des assertions obsolètes

Les quatre véritables définitions déjà dotées d'IDs canoniques
(`Item_Torch`, `Gem_Blue`, `Key_Copper`, `Key_Iron`)
conservent toujours un ancien `UGridItemDefinitionAsset*`.
La transaction native future doit retirer ces quatre champs
sans changer la logique d'offrande/serrure. Plusieurs anciens
tests exigeaient systématiquement des pointeurs présents et
auraient échoué sur le **bon** binaire converti, comme l'avait
fait X.8 avant la correction X.47.

X.52 sécurise les deux états autorisés dans les vrais packages :

- état A : les deux anciennes sources de réceptacle et les deux
  anciennes sources de serrure sont toutes présentes, cohérentes
  avec leurs IDs canoniques déjà persistés
- état B : les quatre pointeurs sont retirés, les mêmes quatre IDs
  étant conservés et vérifiés
- **tout état partiel (un, deux, trois pointeurs) ou contradictoire**
  est rejeté par les audits croisés, notamment X.48 et X.51

## Fichiers et invariants

- `GridEditorARMCLEAN064X48PostNativeReferenceAuditTests.cpp` :
  le total de références historiques ne peut être que **63 avant**
  ou **59 après**, avec les autres 59 positions toujours identiques :
  18 contenus initiaux, 3 overrides inactifs, 33 palette, 5 monstres.
  Les deux règles d'offrandes restent précisément Torch/Gem,
  les deux serrures restent Copper/Iron. Pas de nouvelle clé.
- `GridEditorARMCLEAN064X1661RealAcceptanceManifestTests.cpp` :
  conserve quatre lignes de manifeste, deux règles d'offrande
  et deux listes de clés, avec IDs exacts, source optionnelle
  cohérente et no-op natif. Les quatre lignes sont indépendantes
  de la présence des pointeurs.
- `GridEditorTD073ApprovedCanonicalLockAcceptance.h` :
  autorise exactement deux schémas pour les vraies serrures
  approuvées : un `AcceptedKeyItems[0]` cohérent + un
  `AcceptedKeyIds[0]`, ou cet ID seul ; toujours un seul ID,
  aucun doublon ni valeur inconnue.
- `GridEditorTD0737CurrentAssetNormalizationTests.cpp`
  et test synthétique `GridEditorARMCLEAN064X841...` :
  contrôle des vrais assets avant/après, refus d'une entrée
  `AcceptedKeyItems[0]` nulle, ID erroné, doublon ou chemin
  de serrure incorrect ; les tests de loot restent inchangés.
- Les anciens tests `X.71.RealReceptacleMigrationAuditOnly`
  et `X.81.RealLockKeyMigrationAuditOnly` n'appellent
  **plus les commandlets temporaires X.71/X.81**. Ils
  inspectent désormais directement les véritables définitions
  en lecture seule, vérifient ID/ancien pointeur optionnel
  cohérent, état complet et absence de modification. Les
  anciens commandlets `-Apply` sont **interdits** sur cette
  étape ; les conserver temporairement ne les autorise pas.

L'audit de contenu initial des 18 réceptacles et le moteur
de sauvegarde v27 ne sont pas modifiés. Le préflight X.51
duplique de véritables assets sans les sauvegarder : X.52
ne remplace pas une preuve de réouverture post-conversion.

## Suite

Après `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto`
**success** sur le SHA X.52, préparer un outil unique
de migration UE Editor dans une transaction isolée :
backup privé SHA-256 des quatre packages, vérification
identité et état propre, annulation des quatre pointeurs
exactement, `UPackage::SavePackage` limité aux quatre
fichiers autorisés, Verify distinct, tests `Grimrock`
complets sur les fichiers modifiés, puis commit/push
natif atomique de quatre binaires seulement.
Aucune écriture native n'est déclenchée par X.52.

**06.4 PARTIELLE ; 06.5 BLOQUÉE.**
