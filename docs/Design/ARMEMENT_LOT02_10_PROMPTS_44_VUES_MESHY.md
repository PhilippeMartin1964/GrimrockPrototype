# ARMEMENT-LOT02.10 — Prompts des 44 vues orthographiques pour Meshy

Projet : GrimrockPrototype / Unreal Engine 5.5.4. Destination : thread « Contrôle des vues ARM20 ».

## Message de démarrage à transmettre au thread

> Nous créons les **44 images source destinées à Meshy** pour les 11 armes ci-dessous. Génère une image **individuelle** à la fois, jamais une planche. Pour chaque arme, commence par FRONT ; fais-la valider, puis utilise l'image FRONT retenue comme **référence visuelle impérative** pour BACK, LEFT et RIGHT. Respecte strictement la forme, les proportions, le nombre de pièces, les matériaux, les couleurs, les gravures, les fixations et les accessoires du même objet réel en 3D. Les côtés doivent être de vrais profils orthographiques, pas des perspectives 3/4. Ne change jamais de version du modèle entre les quatre vues. Conserve l'image de référence jointe et le prompt d'identité à chaque génération. Les images sont des références visuelles, pas une preuve de géométrie 3D parfaite.

## Convention générale (intégrée dans chacun des 44 prompts)

- **FRONT** = face principale de l'objet ; **BACK** = rotation de 180° autour de l'axe Z ; **LEFT** = rotation de +90° autour de Z depuis FRONT ; **RIGHT** = rotation de -90° autour de Z. Caméra horizontale, aucune inclinaison.
- Arme allongée : pointe/tête vers le haut, poignée/talon vers le bas, axe principal vertical Z-up. Bouclier : disque vertical, face décorée vers FRONT. Orbe : centre de la sphère au centre de l'image.
- Objet entier dans un **PNG carré 1024×1024**, centré, même cadrage et échelle **au sein des 4 vues de la même arme**, marge d'environ 8 %, aucun élément coupé.
- Rendu de studio réaliste 3D dark medieval fantasy : proportions de fabrication plausibles, géométrie nette, métal forgé, vrai bois, cuir et pierre/cristal ; petites traces d'âge sans excès.
- Fond uni gris très clair ou transparent, lumière studio douce et stable, pas d'ombre portée opaque, pas de décor.
- **Interdits :** personnage, main, flèche sur un arc, intérieur de donjon, pose dramatique, perspective, vue trois-quarts, collage, quatre objets dans une image, dessin au trait, légende, texte, chiffre, logo, watermark, cadre, explosion, fumée ou particules volumétriques.
- En cas de désaccord entre FRONT et une autre vue, corriger la vue plutôt que redessiner l'arme. Une vue latérale fine peut rester très fine : ne pas épaissir artificiellement l'objet.

## Répartition des fichiers

| Code | Arme | FRONT | BACK | LEFT | RIGHT |
|---|---|---|---|---|---|
| ARM038 | Lance courte à deux mains | `ARM038_FRONT.png` | `ARM038_BACK.png` | `ARM038_LEFT.png` | `ARM038_RIGHT.png` |
| ARM048 | Arc recourbé | `ARM048_FRONT.png` | `ARM048_BACK.png` | `ARM048_LEFT.png` | `ARM048_RIGHT.png` |
| ARM054 | Couteau de jet | `ARM054_FRONT.png` | `ARM054_BACK.png` | `ARM054_LEFT.png` | `ARM054_RIGHT.png` |
| ARM057 | Javelot | `ARM057_FRONT.png` | `ARM057_BACK.png` | `ARM057_LEFT.png` | `ARM057_RIGHT.png` |
| ARM058 | Dard de jet | `ARM058_FRONT.png` | `ARM058_BACK.png` | `ARM058_LEFT.png` | `ARM058_RIGHT.png` |
| ARM061 | Baguette arcanique | `ARM061_FRONT.png` | `ARM061_BACK.png` | `ARM061_LEFT.png` | `ARM061_RIGHT.png` |
| ARM062 | Baguette de feu | `ARM062_FRONT.png` | `ARM062_BACK.png` | `ARM062_LEFT.png` | `ARM062_RIGHT.png` |
| ARM067 | Bâton élémentaire | `ARM067_FRONT.png` | `ARM067_BACK.png` | `ARM067_LEFT.png` | `ARM067_RIGHT.png` |
| ARM069 | Sceptre sacré | `ARM069_FRONT.png` | `ARM069_BACK.png` | `ARM069_LEFT.png` | `ARM069_RIGHT.png` |
| ARM070 | Orbe arcanique | `ARM070_FRONT.png` | `ARM070_BACK.png` | `ARM070_LEFT.png` | `ARM070_RIGHT.png` |
| ARM081 | Égide sacrée | `ARM081_FRONT.png` | `ARM081_BACK.png` | `ARM081_LEFT.png` | `ARM081_RIGHT.png` |

---

## 44 prompts à copier individuellement


### ARM038 — Lance courte à deux mains (`ShortSpear`)

**FRONT — `ARM038_FRONT.png`**

```text
OBJECT IDENTITY: a single short two-handed spear. long, straight, slightly tapered ash-wood shaft with a narrow forged steel leaf-shaped spearhead; a short dark iron socket, discreet leather hand grips and a small iron butt cap; practical, lean and historically plausible proportions. A few tool marks and restrained iron binding around the socket; the steel point has a visible central ridge and sharpened symmetrical edges.

Front orthographic view, looking directly at the principal designed face of the object. This is the master reference for the three later views. Show the complete frontal construction clearly, emphasizing materials and real fixed attachments. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). This is the FIRST image; establish the single definitive design that must be kept unchanged in the next three views.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**BACK — `ARM038_BACK.png`**

```text
OBJECT IDENTITY: a single short two-handed spear. long, straight, slightly tapered ash-wood shaft with a narrow forged steel leaf-shaped spearhead; a short dark iron socket, discreet leather hand grips and a small iron butt cap; practical, lean and historically plausible proportions. A few tool marks and restrained iron binding around the socket; the steel point has a visible central ridge and sharpened symmetrical edges.

Rear orthographic view, exactly 180 degrees around the world vertical axis from FRONT. Show the untreated rear side of the ash shaft, continuous grip wraps and the back ridge of the forged steel spearhead; no flag, no blade on the back, no additional ornaments. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM038_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**LEFT — `ARM038_LEFT.png`**

```text
OBJECT IDENTITY: a single short two-handed spear. long, straight, slightly tapered ash-wood shaft with a narrow forged steel leaf-shaped spearhead; a short dark iron socket, discreet leather hand grips and a small iron butt cap; practical, lean and historically plausible proportions. A few tool marks and restrained iron binding around the socket; the steel point has a visible central ridge and sharpened symmetrical edges.

True LEFT orthographic side view, exactly +90 degrees around the vertical axis from FRONT, no camera tilt. The shaft must appear round and slim, and the leaf point must reveal its true tapered thickness and central ridge; do not widen the tip to look like a sword. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM038_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**RIGHT — `ARM038_RIGHT.png`**

```text
OBJECT IDENTITY: a single short two-handed spear. long, straight, slightly tapered ash-wood shaft with a narrow forged steel leaf-shaped spearhead; a short dark iron socket, discreet leather hand grips and a small iron butt cap; practical, lean and historically plausible proportions. A few tool marks and restrained iron binding around the socket; the steel point has a visible central ridge and sharpened symmetrical edges.

True RIGHT orthographic side view, exactly -90 degrees around the vertical axis from FRONT, no camera tilt. Reveal the opposite side of the same round shaft and spearhead, with matching socket and leather grips; no added blade or asymmetric extension. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM038_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```


### ARM048 — Arc recourbé (`RecurveBow`)

**FRONT — `ARM048_FRONT.png`**

```text
OBJECT IDENTITY: a single strung medieval recurve bow. one continuous laminated wooden bow with visibly recurved upper and lower tips, a centered worn leather grip, bone or horn nocks and exactly one thin taut bowstring connected to both tips; no arrow. Clearly articulated laminated wooden layers and subtle horn reinforcement; true recurved limb silhouette, not a modern compound bow.

Front orthographic view, looking directly at the principal designed face of the object. This is the master reference for the three later views. Show the complete frontal construction clearly, emphasizing materials and real fixed attachments. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). This is the FIRST image; establish the single definitive design that must be kept unchanged in the next three views.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**BACK — `ARM048_BACK.png`**

```text
OBJECT IDENTITY: a single strung medieval recurve bow. one continuous laminated wooden bow with visibly recurved upper and lower tips, a centered worn leather grip, bone or horn nocks and exactly one thin taut bowstring connected to both tips; no arrow. Clearly articulated laminated wooden layers and subtle horn reinforcement; true recurved limb silhouette, not a modern compound bow.

Rear orthographic view, exactly 180 degrees around the world vertical axis from FRONT. Show the continuous back of the same laminated bow limbs and leather-wrapped grip, with the single bowstring positioned correctly; the limb tips must curve identically to the front reference. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM048_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**LEFT — `ARM048_LEFT.png`**

```text
OBJECT IDENTITY: a single strung medieval recurve bow. one continuous laminated wooden bow with visibly recurved upper and lower tips, a centered worn leather grip, bone or horn nocks and exactly one thin taut bowstring connected to both tips; no arrow. Clearly articulated laminated wooden layers and subtle horn reinforcement; true recurved limb silhouette, not a modern compound bow.

True LEFT orthographic side view, exactly +90 degrees around the vertical axis from FRONT, no camera tilt. A strict edge-on profile showing the authentic forward and backward limb curvature, thickness of the wooden laminations and the single taut bowstring in its offset plane; this should be a narrow profile. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM048_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**RIGHT — `ARM048_RIGHT.png`**

```text
OBJECT IDENTITY: a single strung medieval recurve bow. one continuous laminated wooden bow with visibly recurved upper and lower tips, a centered worn leather grip, bone or horn nocks and exactly one thin taut bowstring connected to both tips; no arrow. Clearly articulated laminated wooden layers and subtle horn reinforcement; true recurved limb silhouette, not a modern compound bow.

True RIGHT orthographic side view, exactly -90 degrees around the vertical axis from FRONT, no camera tilt. The opposite edge-on profile of precisely the same recurved limbs and single string; do not reverse the curve or introduce extra cables. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM048_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```


### ARM054 — Couteau de jet (`ThrowingKnife`)

**FRONT — `ARM054_FRONT.png`**

```text
OBJECT IDENTITY: a single balanced medieval throwing knife. a compact, slender, symmetrical double-edged forged-steel blade, a rigid full-length metal tang, a short dark leather-wrapped handle and a small weighted iron pommel; minimal guard suited for throwing, not duelling. The blade has one crisp center ridge and subtle grinder lines; simple practical geometry, clearly distinct from a broad combat dagger.

Front orthographic view, looking directly at the principal designed face of the object. This is the master reference for the three later views. Show the complete frontal construction clearly, emphasizing materials and real fixed attachments. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). This is the FIRST image; establish the single definitive design that must be kept unchanged in the next three views.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**BACK — `ARM054_BACK.png`**

```text
OBJECT IDENTITY: a single balanced medieval throwing knife. a compact, slender, symmetrical double-edged forged-steel blade, a rigid full-length metal tang, a short dark leather-wrapped handle and a small weighted iron pommel; minimal guard suited for throwing, not duelling. The blade has one crisp center ridge and subtle grinder lines; simple practical geometry, clearly distinct from a broad combat dagger.

Rear orthographic view, exactly 180 degrees around the world vertical axis from FRONT. Same short blade and handle viewed from behind; show the full tang and back of the leather binding, without adding a guard or a second blade. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM054_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**LEFT — `ARM054_LEFT.png`**

```text
OBJECT IDENTITY: a single balanced medieval throwing knife. a compact, slender, symmetrical double-edged forged-steel blade, a rigid full-length metal tang, a short dark leather-wrapped handle and a small weighted iron pommel; minimal guard suited for throwing, not duelling. The blade has one crisp center ridge and subtle grinder lines; simple practical geometry, clearly distinct from a broad combat dagger.

True LEFT orthographic side view, exactly +90 degrees around the vertical axis from FRONT, no camera tilt. Very thin steel blade profile with believable taper, flat tang, small handle thickness and weighted pommel; compact balanced outline. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM054_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**RIGHT — `ARM054_RIGHT.png`**

```text
OBJECT IDENTITY: a single balanced medieval throwing knife. a compact, slender, symmetrical double-edged forged-steel blade, a rigid full-length metal tang, a short dark leather-wrapped handle and a small weighted iron pommel; minimal guard suited for throwing, not duelling. The blade has one crisp center ridge and subtle grinder lines; simple practical geometry, clearly distinct from a broad combat dagger.

True RIGHT orthographic side view, exactly -90 degrees around the vertical axis from FRONT, no camera tilt. Opposite blade-edge profile with the same taper and thickness, same pommel and binding; do not add hooks, serrations or an extra edge. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM054_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```


### ARM057 — Javelot (`Javelin`)

**FRONT — `ARM057_FRONT.png`**

```text
OBJECT IDENTITY: a single light hand-thrown medieval javelin. a long, fine and lightweight straight wooden shaft, a very narrow elongated forged-steel point and a short socket fitted to the shaft, plus a modest leather throwing grip near the center of balance. Narrower and visually lighter than the two-handed spear ARM038; no flag, feathers or long sword-shaped head.

Front orthographic view, looking directly at the principal designed face of the object. This is the master reference for the three later views. Show the complete frontal construction clearly, emphasizing materials and real fixed attachments. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). This is the FIRST image; establish the single definitive design that must be kept unchanged in the next three views.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**BACK — `ARM057_BACK.png`**

```text
OBJECT IDENTITY: a single light hand-thrown medieval javelin. a long, fine and lightweight straight wooden shaft, a very narrow elongated forged-steel point and a short socket fitted to the shaft, plus a modest leather throwing grip near the center of balance. Narrower and visually lighter than the two-handed spear ARM038; no flag, feathers or long sword-shaped head.

Rear orthographic view, exactly 180 degrees around the world vertical axis from FRONT. Show the reverse side of the very slim straight shaft, narrow iron socket and slender point; preserve the central leather throwing grip. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM057_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**LEFT — `ARM057_LEFT.png`**

```text
OBJECT IDENTITY: a single light hand-thrown medieval javelin. a long, fine and lightweight straight wooden shaft, a very narrow elongated forged-steel point and a short socket fitted to the shaft, plus a modest leather throwing grip near the center of balance. Narrower and visually lighter than the two-handed spear ARM038; no flag, feathers or long sword-shaped head.

True LEFT orthographic side view, exactly +90 degrees around the vertical axis from FRONT, no camera tilt. A strictly edge-on view showing a needle-thin but solid metal point, narrow socket and slender cylindrical shaft, no widening. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM057_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**RIGHT — `ARM057_RIGHT.png`**

```text
OBJECT IDENTITY: a single light hand-thrown medieval javelin. a long, fine and lightweight straight wooden shaft, a very narrow elongated forged-steel point and a short socket fitted to the shaft, plus a modest leather throwing grip near the center of balance. Narrower and visually lighter than the two-handed spear ARM038; no flag, feathers or long sword-shaped head.

True RIGHT orthographic side view, exactly -90 degrees around the vertical axis from FRONT, no camera tilt. The opposite narrow side with precisely the same point and grip, preserving a lightweight throwable silhouette. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM057_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```


### ARM058 — Dard de jet (`Dart`)

**FRONT — `ARM058_FRONT.png`**

```text
OBJECT IDENTITY: a single small hand-thrown medieval fantasy dart. a short, thin, meticulously made metal needle with a sharp narrow point, a compact straight shaft with tiny leather grip and restrained brass rings, and a very small stabilizing tail; clearly a handheld dart, not an arrow or a syringe. Its tiny size and slender silhouette must be plausible; dark steel, aged brass, minimal muted green glass or enamel marker, no visible liquid and no dripping poison.

Front orthographic view, looking directly at the principal designed face of the object. This is the master reference for the three later views. Show the complete frontal construction clearly, emphasizing materials and real fixed attachments. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). This is the FIRST image; establish the single definitive design that must be kept unchanged in the next three views.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**BACK — `ARM058_BACK.png`**

```text
OBJECT IDENTITY: a single small hand-thrown medieval fantasy dart. a short, thin, meticulously made metal needle with a sharp narrow point, a compact straight shaft with tiny leather grip and restrained brass rings, and a very small stabilizing tail; clearly a handheld dart, not an arrow or a syringe. Its tiny size and slender silhouette must be plausible; dark steel, aged brass, minimal muted green glass or enamel marker, no visible liquid and no dripping poison.

Rear orthographic view, exactly 180 degrees around the world vertical axis from FRONT. Reverse view of exactly the same slim dart and tiny stabilizing tail, including the small brass bands; never add additional fletching. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM058_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**LEFT — `ARM058_LEFT.png`**

```text
OBJECT IDENTITY: a single small hand-thrown medieval fantasy dart. a short, thin, meticulously made metal needle with a sharp narrow point, a compact straight shaft with tiny leather grip and restrained brass rings, and a very small stabilizing tail; clearly a handheld dart, not an arrow or a syringe. Its tiny size and slender silhouette must be plausible; dark steel, aged brass, minimal muted green glass or enamel marker, no visible liquid and no dripping poison.

True LEFT orthographic side view, exactly +90 degrees around the vertical axis from FRONT, no camera tilt. Extremely thin edge-on profile that accurately shows the slender needle and compact tail thickness; avoid making it into a spear. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM058_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**RIGHT — `ARM058_RIGHT.png`**

```text
OBJECT IDENTITY: a single small hand-thrown medieval fantasy dart. a short, thin, meticulously made metal needle with a sharp narrow point, a compact straight shaft with tiny leather grip and restrained brass rings, and a very small stabilizing tail; clearly a handheld dart, not an arrow or a syringe. Its tiny size and slender silhouette must be plausible; dark steel, aged brass, minimal muted green glass or enamel marker, no visible liquid and no dripping poison.

True RIGHT orthographic side view, exactly -90 degrees around the vertical axis from FRONT, no camera tilt. Exact opposite profile retaining the needle, tiny grip and tail geometry; no unexplained extra wings or blades. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM058_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```


### ARM061 — Baguette arcanique (`ArcaneWand`)

**FRONT — `ARM061_FRONT.png`**

```text
OBJECT IDENTITY: a single short arcane mage wand. a hand-sized straight dark walnut wand carved with fine arcane geometric inscriptions, understated aged silver bindings, a narrow forged-metal setting at the tip and one small faceted deep-blue crystal firmly attached at its head. A very restrained internal blue luminescence only within the crystal, no floating fragments, no external rays or spell projectile.

Front orthographic view, looking directly at the principal designed face of the object. This is the master reference for the three later views. Show the complete frontal construction clearly, emphasizing materials and real fixed attachments. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). This is the FIRST image; establish the single definitive design that must be kept unchanged in the next three views.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**BACK — `ARM061_BACK.png`**

```text
OBJECT IDENTITY: a single short arcane mage wand. a hand-sized straight dark walnut wand carved with fine arcane geometric inscriptions, understated aged silver bindings, a narrow forged-metal setting at the tip and one small faceted deep-blue crystal firmly attached at its head. A very restrained internal blue luminescence only within the crystal, no floating fragments, no external rays or spell projectile.

Rear orthographic view, exactly 180 degrees around the world vertical axis from FRONT. Show the backside of the same wooden wand with continuous engraved lines, matching silver fittings and the rear facets of its single secured blue crystal. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM061_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**LEFT — `ARM061_LEFT.png`**

```text
OBJECT IDENTITY: a single short arcane mage wand. a hand-sized straight dark walnut wand carved with fine arcane geometric inscriptions, understated aged silver bindings, a narrow forged-metal setting at the tip and one small faceted deep-blue crystal firmly attached at its head. A very restrained internal blue luminescence only within the crystal, no floating fragments, no external rays or spell projectile.

True LEFT orthographic side view, exactly +90 degrees around the vertical axis from FRONT, no camera tilt. Side profile displaying the carved wooden shaft thickness, fixed crystal setting and slender proportions, with no floating magic attachments. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM061_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**RIGHT — `ARM061_RIGHT.png`**

```text
OBJECT IDENTITY: a single short arcane mage wand. a hand-sized straight dark walnut wand carved with fine arcane geometric inscriptions, understated aged silver bindings, a narrow forged-metal setting at the tip and one small faceted deep-blue crystal firmly attached at its head. A very restrained internal blue luminescence only within the crystal, no floating fragments, no external rays or spell projectile.

True RIGHT orthographic side view, exactly -90 degrees around the vertical axis from FRONT, no camera tilt. Opposite profile of the identical attached crystal and silver fittings; preserve every band at the same height. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM061_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```


### ARM062 — Baguette de feu (`FireWand`)

**FRONT — `ARM062_FRONT.png`**

```text
OBJECT IDENTITY: a single short fire-aligned mage wand. a slender hand-sized blackened hardwood wand with copper and aged brass bindings, subtle burned grain, fine ember-red runic engravings and one small warm amber-red crystal secured by a simple metal crown at the tip. Faint ember glow confined to grooves and crystal only, no actual flames, no smoke, no projectile and no explosive effects.

Front orthographic view, looking directly at the principal designed face of the object. This is the master reference for the three later views. Show the complete frontal construction clearly, emphasizing materials and real fixed attachments. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). This is the FIRST image; establish the single definitive design that must be kept unchanged in the next three views.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**BACK — `ARM062_BACK.png`**

```text
OBJECT IDENTITY: a single short fire-aligned mage wand. a slender hand-sized blackened hardwood wand with copper and aged brass bindings, subtle burned grain, fine ember-red runic engravings and one small warm amber-red crystal secured by a simple metal crown at the tip. Faint ember glow confined to grooves and crystal only, no actual flames, no smoke, no projectile and no explosive effects.

Rear orthographic view, exactly 180 degrees around the world vertical axis from FRONT. Reverse side of the same blackened shaft, the exact same copper bands and red crystal setting, with realistic heat patina; no new flames. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM062_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**LEFT — `ARM062_LEFT.png`**

```text
OBJECT IDENTITY: a single short fire-aligned mage wand. a slender hand-sized blackened hardwood wand with copper and aged brass bindings, subtle burned grain, fine ember-red runic engravings and one small warm amber-red crystal secured by a simple metal crown at the tip. Faint ember glow confined to grooves and crystal only, no actual flames, no smoke, no projectile and no explosive effects.

True LEFT orthographic side view, exactly +90 degrees around the vertical axis from FRONT, no camera tilt. A narrow side profile showing the tip crown clamping the amber-red crystal and the actual carved wood thickness; no large fiery plume. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM062_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**RIGHT — `ARM062_RIGHT.png`**

```text
OBJECT IDENTITY: a single short fire-aligned mage wand. a slender hand-sized blackened hardwood wand with copper and aged brass bindings, subtle burned grain, fine ember-red runic engravings and one small warm amber-red crystal secured by a simple metal crown at the tip. Faint ember glow confined to grooves and crystal only, no actual flames, no smoke, no projectile and no explosive effects.

True RIGHT orthographic side view, exactly -90 degrees around the vertical axis from FRONT, no camera tilt. The exact opposite side profile with all copper bands and red engraving still consistent, with no invented side fins or spikes. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM062_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```


### ARM067 — Bâton élémentaire (`ElementalStaff`)

**FRONT — `ARM067_FRONT.png`**

```text
OBJECT IDENTITY: a single two-handed elemental mage staff. one tall, straight, dark seasoned wooden staff with believable wear from weather and heat, engraved bronze collars and one substantial securely mounted faceted crystal at the top with subtle blue, ember-orange and jade-green inclusions. Elemental symbolism is carved or inlaid into the real surface; all pieces are attached to the staff, no detached orbiting crystals, no spell effects or terrain surfaces.

Front orthographic view, looking directly at the principal designed face of the object. This is the master reference for the three later views. Show the complete frontal construction clearly, emphasizing materials and real fixed attachments. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). This is the FIRST image; establish the single definitive design that must be kept unchanged in the next three views.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**BACK — `ARM067_BACK.png`**

```text
OBJECT IDENTITY: a single two-handed elemental mage staff. one tall, straight, dark seasoned wooden staff with believable wear from weather and heat, engraved bronze collars and one substantial securely mounted faceted crystal at the top with subtle blue, ember-orange and jade-green inclusions. Elemental symbolism is carved or inlaid into the real surface; all pieces are attached to the staff, no detached orbiting crystals, no spell effects or terrain surfaces.

Rear orthographic view, exactly 180 degrees around the world vertical axis from FRONT. Show the real back of the same staff, with matching bronze collars and the opposite facets of the single attached tri-colored crystal; wooden grain remains continuous. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM067_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**LEFT — `ARM067_LEFT.png`**

```text
OBJECT IDENTITY: a single two-handed elemental mage staff. one tall, straight, dark seasoned wooden staff with believable wear from weather and heat, engraved bronze collars and one substantial securely mounted faceted crystal at the top with subtle blue, ember-orange and jade-green inclusions. Elemental symbolism is carved or inlaid into the real surface; all pieces are attached to the staff, no detached orbiting crystals, no spell effects or terrain surfaces.

True LEFT orthographic side view, exactly +90 degrees around the vertical axis from FRONT, no camera tilt. True side profile of long wooden staff and top crystal mount, showing believable thickness and the fixed connection between crystal and shaft. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM067_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**RIGHT — `ARM067_RIGHT.png`**

```text
OBJECT IDENTITY: a single two-handed elemental mage staff. one tall, straight, dark seasoned wooden staff with believable wear from weather and heat, engraved bronze collars and one substantial securely mounted faceted crystal at the top with subtle blue, ember-orange and jade-green inclusions. Elemental symbolism is carved or inlaid into the real surface; all pieces are attached to the staff, no detached orbiting crystals, no spell effects or terrain surfaces.

True RIGHT orthographic side view, exactly -90 degrees around the vertical axis from FRONT, no camera tilt. Opposite profile with the same height and crystal silhouette; do not change the shaft to a spear or add any magical blade. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM067_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```


### ARM069 — Sceptre sacré (`HolyScepter`)

**FRONT — `ARM069_FRONT.png`**

```text
OBJECT IDENTITY: a single one-handed sacred fantasy scepter. a medium-length ivory-toned carved wooden or bone-inlaid shaft, slender aged gold filigree, a small symmetrical ceremonial crown holding one pale opalescent crystal at the top, and modest abstract sacred geometric motifs. Worn, solemn and ancient rather than bright or ornate; faint warm white glow confined to the crystal, no healing particles, no crosses or real-world denominational symbols.

Front orthographic view, looking directly at the principal designed face of the object. This is the master reference for the three later views. Show the complete frontal construction clearly, emphasizing materials and real fixed attachments. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). This is the FIRST image; establish the single definitive design that must be kept unchanged in the next three views.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**BACK — `ARM069_BACK.png`**

```text
OBJECT IDENTITY: a single one-handed sacred fantasy scepter. a medium-length ivory-toned carved wooden or bone-inlaid shaft, slender aged gold filigree, a small symmetrical ceremonial crown holding one pale opalescent crystal at the top, and modest abstract sacred geometric motifs. Worn, solemn and ancient rather than bright or ornate; faint warm white glow confined to the crystal, no healing particles, no crosses or real-world denominational symbols.

Rear orthographic view, exactly 180 degrees around the world vertical axis from FRONT. Back of the same sacred crown, opalescent crystal facets, gold collars and continuous carved shaft; no weapon blade or new symbolic imagery. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM069_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**LEFT — `ARM069_LEFT.png`**

```text
OBJECT IDENTITY: a single one-handed sacred fantasy scepter. a medium-length ivory-toned carved wooden or bone-inlaid shaft, slender aged gold filigree, a small symmetrical ceremonial crown holding one pale opalescent crystal at the top, and modest abstract sacred geometric motifs. Worn, solemn and ancient rather than bright or ornate; faint warm white glow confined to the crystal, no healing particles, no crosses or real-world denominational symbols.

True LEFT orthographic side view, exactly +90 degrees around the vertical axis from FRONT, no camera tilt. A credible slim ceremonial side profile showing thickness of the crown and crystal socket, with all gold fittings attached. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM069_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**RIGHT — `ARM069_RIGHT.png`**

```text
OBJECT IDENTITY: a single one-handed sacred fantasy scepter. a medium-length ivory-toned carved wooden or bone-inlaid shaft, slender aged gold filigree, a small symmetrical ceremonial crown holding one pale opalescent crystal at the top, and modest abstract sacred geometric motifs. Worn, solemn and ancient rather than bright or ornate; faint warm white glow confined to the crystal, no healing particles, no crosses or real-world denominational symbols.

True RIGHT orthographic side view, exactly -90 degrees around the vertical axis from FRONT, no camera tilt. Opposite ceremonial profile, fixed ornament layout and crystal setting; do not enlarge the head into a mace or wand fireball. Place the shaft/blade vertical, tip or head at the top, handle/butt at the bottom (world Z up). IMPORTANT: the previously approved `ARM069_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```


### ARM070 — Orbe arcanique (`ArcaneOrb`)

**FRONT — `ARM070_FRONT.png`**

```text
OBJECT IDENTITY: a single hand-sized passive arcane focus orb. one palm-sized spherical, faceted midnight-blue crystalline orb captured by two finely wrought, connected tarnished-silver and bronze stabilizing bands; discreet runes etched into the bands; a completely self-contained handheld object with no pedestal. The sphere is the principal solid mass; very subtle internal blue light, no spellcasting, no rays, no hovering detached parts and no disembodied hand.

Front orthographic view, looking directly at the principal designed face of the object. This is the master reference for the three later views. Show the complete frontal construction clearly, emphasizing materials and real fixed attachments. Keep the spherical orb centered with its connected bands in the same physical orientation. This is the FIRST image; establish the single definitive design that must be kept unchanged in the next three views.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**BACK — `ARM070_BACK.png`**

```text
OBJECT IDENTITY: a single hand-sized passive arcane focus orb. one palm-sized spherical, faceted midnight-blue crystalline orb captured by two finely wrought, connected tarnished-silver and bronze stabilizing bands; discreet runes etched into the bands; a completely self-contained handheld object with no pedestal. The sphere is the principal solid mass; very subtle internal blue light, no spellcasting, no rays, no hovering detached parts and no disembodied hand.

Rear orthographic view, exactly 180 degrees around the world vertical axis from FRONT. Opposite hemisphere of exactly the same solid orb and connected metal bands; reveal continuous rear clasps and subtly worn metal engraving; no rear handle or pedestal. Keep the spherical orb centered with its connected bands in the same physical orientation. IMPORTANT: the previously approved `ARM070_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**LEFT — `ARM070_LEFT.png`**

```text
OBJECT IDENTITY: a single hand-sized passive arcane focus orb. one palm-sized spherical, faceted midnight-blue crystalline orb captured by two finely wrought, connected tarnished-silver and bronze stabilizing bands; discreet runes etched into the bands; a completely self-contained handheld object with no pedestal. The sphere is the principal solid mass; very subtle internal blue light, no spellcasting, no rays, no hovering detached parts and no disembodied hand.

True LEFT orthographic side view, exactly +90 degrees around the vertical axis from FRONT, no camera tilt. True side view of the spherical crystal volume and the thickness of its metal bands; a sphere must remain a circle, not elongate. Keep the spherical orb centered with its connected bands in the same physical orientation. IMPORTANT: the previously approved `ARM070_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**RIGHT — `ARM070_RIGHT.png`**

```text
OBJECT IDENTITY: a single hand-sized passive arcane focus orb. one palm-sized spherical, faceted midnight-blue crystalline orb captured by two finely wrought, connected tarnished-silver and bronze stabilizing bands; discreet runes etched into the bands; a completely self-contained handheld object with no pedestal. The sphere is the principal solid mass; very subtle internal blue light, no spellcasting, no rays, no hovering detached parts and no disembodied hand.

True RIGHT orthographic side view, exactly -90 degrees around the vertical axis from FRONT, no camera tilt. Opposite 90-degree side view of the same sphere, revealing the opposing band and clamp without introducing a stand or extra gems. Keep the spherical orb centered with its connected bands in the same physical orientation. IMPORTANT: the previously approved `ARM070_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```


### ARM081 — Égide sacrée (`HolyAegis`)

**FRONT — `ARM081_FRONT.png`**

```text
OBJECT IDENTITY: a single round off-hand sacred fantasy shield. a compact round shield of dark hammered steel over a subtle wooden backing, strong bronze-gold reinforced rim, a central shallow raised boss with one small pale sacred stone and restrained engraved geometric protective symbols; battle-worn but structurally sound. Physically plausible defensive shield sized for one arm, not a giant decorative wall plaque; curved thickness is visible in side views.

Front orthographic view, looking directly at the principal designed face of the object. This is the master reference for the three later views. Show the complete frontal construction clearly, emphasizing materials and real fixed attachments. Keep the circular shield upright, its decorated face defining FRONT. This is the FIRST image; establish the single definitive design that must be kept unchanged in the next three views.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**BACK — `ARM081_BACK.png`**

```text
OBJECT IDENTITY: a single round off-hand sacred fantasy shield. a compact round shield of dark hammered steel over a subtle wooden backing, strong bronze-gold reinforced rim, a central shallow raised boss with one small pale sacred stone and restrained engraved geometric protective symbols; battle-worn but structurally sound. Physically plausible defensive shield sized for one arm, not a giant decorative wall plaque; curved thickness is visible in side views.

Rear orthographic view, exactly 180 degrees around the world vertical axis from FRONT. Show the actual functional BACK of the exact same round shield: worn wooden planks, leather-lined forearm strap, sturdy hand grip, rivets and metal bracing, all mechanically attached to the front plate; no magical front emblem duplicated onto the back. Keep the circular shield upright, its decorated face defining FRONT. IMPORTANT: the previously approved `ARM081_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**LEFT — `ARM081_LEFT.png`**

```text
OBJECT IDENTITY: a single round off-hand sacred fantasy shield. a compact round shield of dark hammered steel over a subtle wooden backing, strong bronze-gold reinforced rim, a central shallow raised boss with one small pale sacred stone and restrained engraved geometric protective symbols; battle-worn but structurally sound. Physically plausible defensive shield sized for one arm, not a giant decorative wall plaque; curved thickness is visible in side views.

True LEFT orthographic side view, exactly +90 degrees around the vertical axis from FRONT, no camera tilt. A strict 90-degree left-side profile clearly showing shield curvature, layered rim thickness, front boss protrusion and rear leather straps; do not render a full front-facing disc. Keep the circular shield upright, its decorated face defining FRONT. IMPORTANT: the previously approved `ARM081_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```

**RIGHT — `ARM081_RIGHT.png`**

```text
OBJECT IDENTITY: a single round off-hand sacred fantasy shield. a compact round shield of dark hammered steel over a subtle wooden backing, strong bronze-gold reinforced rim, a central shallow raised boss with one small pale sacred stone and restrained engraved geometric protective symbols; battle-worn but structurally sound. Physically plausible defensive shield sized for one arm, not a giant decorative wall plaque; curved thickness is visible in side views.

True RIGHT orthographic side view, exactly -90 degrees around the vertical axis from FRONT, no camera tilt. The true 90-degree right-side profile showing the same curvature and opposite strap position, no enlargement or invented secondary layers. Keep the circular shield upright, its decorated face defining FRONT. IMPORTANT: the previously approved `ARM081_FRONT.png` MUST be supplied as an image reference. Depict the EXACT SAME physical 3D object, not a new interpretation or a flipped duplicate. Preserve all proportions, colors, construction, ornament counts, fastening positions and wear; only rotate the camera/object to the requested view.

Create exactly ONE high-end photorealistic 3D studio product image for a serious dark medieval fantasy game, orthographic camera only, camera level at the object's center (no elevation or 3/4 perspective), physically credible metal/wood/leather/crystal materials and subtle age, clean soft neutral studio lighting, plain light neutral grey or transparent background. Square 1024x1024 PNG, entire object centered and fully visible with approximately 8% margin; maintain identical object scale, camera distance, canvas and illumination across all four views. No character, hands, environment, labels, written text, logos, watermarks, framing border, split layout, extra object, smoke, flames, sparks or dramatic VFX. Do not create a contact sheet or show more than one view.
```


---

## Contrôle final avant Meshy

Pour chaque série de quatre images, contrôler la continuité du modèle : même silhouette, longueur, largeur, épaisseur, montures, nombre de cristaux, bandes métalliques et décors ; mêmes dimensions de cadrage et direction d'éclairage. Une image latérale doit montrer l'épaisseur réelle, pas un faux trois-quarts. Vérifier spécialement la corde de ARM048, la finesse des projectiles ARM054/057/058, les cristaux fixés ARM061/062/067/069 et la face arrière fonctionnelle de ARM081. Dans le cas d'ARM070, les quatre vues d'une sphère peuvent se ressembler : ce n'est pas une erreur.

Une fois les 44 vues acceptées, utiliser les quatre images **du même code ARM** ensemble dans Meshy, puis contrôler le mesh obtenu dans Blender avant import Unreal. Ces prompts décrivent le visuel ; ils n'ajoutent aucun sort, talent, poison, soin ou effet gameplay.
