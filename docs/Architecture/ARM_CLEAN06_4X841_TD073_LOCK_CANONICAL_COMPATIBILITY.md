# ARM-CLEAN06.4X.16.8.4.1 — Ajuster les audits TD073 avant l'écriture des clés

La transaction x16.8.4 a réussi Audit/Apply/Verify mais la suite
Grimrock après écriture a échoué (POST_AUTOMATION_FAILED).
Aucun commit binaire n'a été publié, et la procédure prévoit
la restauration SHA-256 des deux sources.

L'inspection des tests révèle une incompatibilité de schéma :
TD0731, TD0737 (deux audits et le test de normalisation)
et TD0738 classent tout AcceptedKeyIds non vide comme legacy.
Or la transition canonique prévoit justement de conserver
AcceptedKeyItems[0] **et** de lui adjoindre un AcceptedKeyIds[0]
strictement équivalent dans deux vrais DataAssets.

Une politique pure et locale aux tests accepte seulement :
- DA_Lock_CopperWall : Key_Copper dans les deux champs ;
- DA_Lock_IronWall : Key_Iron dans les deux champs ;
- ou l'état historique avec AcceptedKeyIds vide.

Toute mauvaise clé, autre package, source manquante,
doublon ou valeur supplémentaire reste une anomalie.
Un test automatisé couvre tous ces cas. Cette correction
ne modifie ni comportement runtime, ni DataAsset, ni
scripts de transaction. Les tests post-migration devront
désormais autoriser **uniquement** cette transition connue.

Avant nouvelle tentative de migration native, exiger les
deux contrôles UE5.5.4 SUCCESS sur ce commit code.
