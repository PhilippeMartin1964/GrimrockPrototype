# ARMEMENT01.6.3 — ARM001 Dague réaliste — essai Meshy 7.1

Date : 20 octobre 2026.
Statut : protocole artistique préparé. La génération Meshy elle-même nécessite une action dans le compte utilisateur.
Baseline UE5.5.4 : TD04.2 du 20.10.2026 à 20:47, 1 486 réussites, 0 avertissement, 0 échec.

## 1. Objectif et garde-fous

Refaire visuellement ARM001 (Dague), lié à VIS_Dagger_A, avec une géométrie et des textures PBR réalistes. Le prototype procédural et les .uasset existants sont **conservés**. Le travail artistique ne change ni dégâts, ni niveaux 1–20, ni hotbar, ni physique d'un item, ni catalogue RPG.

Le modèle final pilote sera nommé SM_Weapon_Dagger_Meshy_Pilot : il ne doit **jamais écraser** SM_Weapon_Dagger_A lors de l'exploration artistique. Les résultats éventuels du compte Meshy ne sont pas déjà présents dans Git.

## 2. Workflow actuel de Meshy

En octobre 2026, Meshy 7.1 est la version courante du mode Image to 3D ; Meshy 7 est désormais une version antérieure. Le mode Text to 3D de l'application web ne convient pas à cet essai Meshy 7. Utiliser :

1. Meshy Workspace (https://www.meshy.ai/workspace) → Image → AI Image Generation.
2. Produire une image de référence unique de la dague (prompt ci-dessous). Style Realistic, fond simple et toute l'arme visible.
3. Valider visuellement l'image, puis Send to Image to 3D.
4. 3D Model → Image to 3D → High Detail → Meshy 7.1 (ou Meshy 7 si seule version affichée), résolution Standard pour le premier essai.
5. Si les vues générées sont cohérentes, activer Multi-View (de 1 à 4 vues). Multi-View et Smart Topology ne fonctionnent pas simultanément ; les éventuels remesh/retopo viennent **après**.
6. Textures PBR activées si disponibles : BaseColor, Metallic, Roughness, Normal. Texture 2K suffisante pour le premier essai ; si l'interface propose Remove Lighting, l'activer pour éviter les reflets peints dans BaseColor.
7. Examiner les quatre côtés, récupérer GLB et, si utile, FBX plus maps PBR. Ne pas payer plusieurs générations sans avoir d'abord analysé le premier rendu.

Documentation officielle consultée :
- https://docs.meshy.ai/en/webapp/guides/image/ai-image-generation
- https://docs.meshy.ai/en/webapp/image-to-3d
- https://docs.meshy.ai/en/webapp/changelog
- https://www.meshy.ai/tutorials/multi-view-image-to-3d
- https://docs.meshy.ai/en/api/multi-image-to-3d

## 3. Direction artistique — Dague de garde ARM001

Proportions cibles à confirmer dans Blender, non garanties par Meshy : longueur 38–42 cm, lame droite double tranchant environ 25–27 cm, poignée+pommeau 11–13 cm, garde 8–10 cm. Acier forgé légèrement sombre, biseaux usés et fines rayures ; garde patinée simple ; cuir brun foncé enroulé et cousu ; pommeau métallique compact. Pas de runes lumineuses, de décoration géante, de magie, ni de métal plastique.

### Prompt de génération de la référence 2D (anglais, copier tel quel)

~~~text
One single complete late-medieval European quillon dagger, historically plausible dark fantasy game prop, photographed like a premium museum object for accurate 3D reconstruction. Vertical upright orientation, sharp blade pointing UP and pommel DOWN, fully visible from tip to pommel, centered, straight-on FRONT orthographic product view, no perspective distortion, isolated on a completely plain off-white background. Overall length approximately 40 cm. Straight double-edged hand-forged steel blade around 26 cm long, diamond cross-section with subtle central ridge, physically credible thickness, faint hammer marks, extremely fine scratches, darker patina close to the guard and naturally bright worn edges. Simple straight iron crossguard about 9 cm wide, restrained touches of aged bronze, solid brown-black leather-wrapped grip with precise small stitches, compact worn steel pommel. Subtle handcrafted imperfections, premium photorealistic PBR appearance, real metal and leather microdetail, physically plausible craftsmanship, soft even product lighting. Entire single weapon inside the frame, 10 percent clean margin on all sides. NOT illustration, NOT stylized, NOT cartoon.
~~~

### Exclusions (champ Negative Prompt s'il existe)

~~~text
No hand, no human, no sheath, no table, no stand, no skull, no glowing runes, no jewel, no magical aura, no ornate fantasy spikes, no curved blade, no extra blades, no extra objects, no floating parts, no text, no watermark, no logo, no frame, no harsh cast shadows, no clipped tip or pommel.
~~~

Si Meshy propose la création automatique des autres angles, utiliser la première image approuvée comme autorité. Vérifier la **même arme** de face, profil, dos et trois quarts : surtout ne pas mélanger des gardes ou poignées différentes.

## 4. Dossier d'échanges local

    D:\Development\GrimrockPrototype\Saved\ARMEMENT01\ARMEMENT01_6_3\Meshy\ARM001\
        01_Reference\
        02_MeshyExport\
        03_BlenderReview\
        04_UEReview\

Ne pas remplacer un fichier existant du dossier Content. Conserver le GLB brut de Meshy, ses maps, et les paramètres utilisés.

## 5. Pipeline de finition

Blender :
- importer GLB/FBX, vérifier intégrité du mesh sur 360 degrés, épaisseur réelle de la lame, normales, UV, tangentes et jonctions ;
- remettre l'échelle aux environs de 40 cm et aligner selon Z vertical UE en centimètres ;
- vérifier maps PBR et cohérence du grain acier/cuir ;
- réduire la densité si nécessaire, en gardant les détails normaux via baking : cible initiale indicative 10–25k triangles en LOD0, selon visibilité réelle ;
- conserver les fichiers originaux et sortir un export UE distinct.

UE5.5.4 :
- importer sous /Game/GrimrockPrototype/Meshes/Weapons/SM_Weapon_Dagger_Meshy_Pilot ;
- matériaux principaux dans les Material Slots, conformément à MATERIAL-OWNERSHIP01 ;
- comparer en lumière de donjon, au sol, en alcôve, et en preview d'inventaire ;
- créer **ultérieurement** l'icône 48×48 par un rendu du mesh approuvé, pas par un nouveau dessin non concordant ;
- ne remplacer la référence WorldMesh de VIS_Dagger_A qu'après validation humaine et tests Grimrock restés verts.

## 6. Décision de passage

Première validation requise : **image 2D de référence réaliste**. Seconde validation : fichier GLB/FBX ou captures de face, côté et trois quarts du résultat Meshy. Aucun remplacement automatique et aucun commit de .uasset avant cette double validation.

La création 3D dans le compte Meshy n'est pas réalisée par la publication de ce document.
