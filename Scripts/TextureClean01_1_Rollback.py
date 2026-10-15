import unreal
import os
import csv
from datetime import datetime

# ============================================================================
# TEXTURE-CLEAN01.1 - Recovery rollback
# Unreal Engine 5.5.x
#
# Restores ONLY the metadata changed by TextureClean01_1_SafeMetadata.py.
# Each asset is reverted only if its current state still matches the exact
# post-apply state recorded by TEXTURE-CLEAN01.1.
#
# No source image is reimported or resized.
# ============================================================================

OUTPUT_SUBDIR = "TextureClean01.1"

timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
output_dir = os.path.join(unreal.Paths.project_saved_dir(), OUTPUT_SUBDIR)
os.makedirs(output_dir, exist_ok=True)

csv_path = os.path.join(
    output_dir,
    "TextureClean01_1_ROLLBACK_{}.csv".format(timestamp)
)
summary_path = os.path.join(
    output_dir,
    "TextureClean01_1_ROLLBACK_{}_summary.txt".format(timestamp)
)


def enum_name(value):
    if value is None:
        return ""

    name = getattr(value, "name", None)
    if name:
        return str(name)

    text = str(value)
    if "." in text:
        text = text.split(".")[-1]
    if ":" in text:
        text = text.split(":")[0]
    return text.replace(">", "").strip()


def state_of(texture):
    return {
        "srgb": bool(texture.get_editor_property("srgb")),
        "lod_group": texture.get_editor_property("lod_group"),
        "compression_settings": texture.get_editor_property("compression_settings"),
        "mip_gen_settings": texture.get_editor_property("mip_gen_settings"),
    }


def state_text(state):
    return {
        "SRGB": str(state["srgb"]),
        "LODGroup": enum_name(state["lod_group"]),
        "Compression": enum_name(state["compression_settings"]),
        "MipGen": enum_name(state["mip_gen_settings"]),
    }


def mismatches(current, expected):
    labels = {
        "srgb": "sRGB",
        "lod_group": "LODGroup",
        "compression_settings": "Compression",
        "mip_gen_settings": "MipGen",
    }

    out = []

    for key, expected_value in expected.items():
        current_value = current[key]
        if current_value == expected_value:
            continue

        if isinstance(current_value, bool):
            current_text = str(current_value)
            expected_text = str(expected_value)
        else:
            current_text = enum_name(current_value)
            expected_text = enum_name(expected_value)

        out.append(
            "{} expected {} but found {}".format(
                labels[key],
                expected_text,
                current_text
            )
        )

    return out


WORLD = unreal.TextureGroup.TEXTUREGROUP_WORLD
UI = unreal.TextureGroup.TEXTUREGROUP_UI

TC_DEFAULT = unreal.TextureCompressionSettings.TC_DEFAULT
TC_MASKS = unreal.TextureCompressionSettings.TC_MASKS
TC_GRAYSCALE = unreal.TextureCompressionSettings.TC_GRAYSCALE

MIPS_FROM_GROUP = unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP
NO_MIPS = unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS


ROLLBACKS = [
    # 4 Talent illustrations: UI group -> original World group.
    {
        "asset": "/Game/GrimrockPrototype/Blueprints/UI/Skills/Images/Alchemist/T_Talent_AlchemistGrenadier.T_Talent_AlchemistGrenadier",
        "applied": {
            "srgb": True,
            "lod_group": UI,
            "compression_settings": TC_DEFAULT,
            "mip_gen_settings": NO_MIPS,
        },
        "restore": {
            "lod_group": WORLD,
        },
    },
    {
        "asset": "/Game/GrimrockPrototype/Blueprints/UI/Skills/Images/Ranger/T_Talent_RangerHunter.T_Talent_RangerHunter",
        "applied": {
            "srgb": True,
            "lod_group": UI,
            "compression_settings": TC_DEFAULT,
            "mip_gen_settings": NO_MIPS,
        },
        "restore": {
            "lod_group": WORLD,
        },
    },
    {
        "asset": "/Game/GrimrockPrototype/Blueprints/UI/Skills/Images/Ranger/T_Talent_RangerMarksman.T_Talent_RangerMarksman",
        "applied": {
            "srgb": True,
            "lod_group": UI,
            "compression_settings": TC_DEFAULT,
            "mip_gen_settings": NO_MIPS,
        },
        "restore": {
            "lod_group": WORLD,
        },
    },
    {
        "asset": "/Game/GrimrockPrototype/Blueprints/UI/Skills/Images/Ranger/T_Talent_RangerScout.T_Talent_RangerScout",
        "applied": {
            "srgb": True,
            "lod_group": UI,
            "compression_settings": TC_DEFAULT,
            "mip_gen_settings": NO_MIPS,
        },
        "restore": {
            "lod_group": WORLD,
        },
    },

    # 3 ORM maps: restore the exact pre-clean metadata.
    {
        "asset": "/Game/GrimrockPrototype/Art/Textures/Final/Assets/Granite_01/T_Granite_01_ORM.T_Granite_01_ORM",
        "applied": {
            "srgb": False,
            "lod_group": WORLD,
            "compression_settings": TC_MASKS,
            "mip_gen_settings": MIPS_FROM_GROUP,
        },
        "restore": {
            "srgb": True,
            "compression_settings": TC_DEFAULT,
        },
    },
    {
        "asset": "/Game/GrimrockPrototype/Art/Textures/Final/Door/Wood_03/T_Wood_03_ORM.T_Wood_03_ORM",
        "applied": {
            "srgb": False,
            "lod_group": WORLD,
            "compression_settings": TC_MASKS,
            "mip_gen_settings": MIPS_FROM_GROUP,
        },
        "restore": {
            "srgb": True,
            "compression_settings": TC_DEFAULT,
        },
    },
    {
        "asset": "/Game/GrimrockPrototype/Art/Textures/Final/Wall/Marble_01/T_Marble_01_ORM.T_Marble_01_ORM",
        "applied": {
            "srgb": False,
            "lod_group": WORLD,
            "compression_settings": TC_MASKS,
            "mip_gen_settings": MIPS_FROM_GROUP,
        },
        "restore": {
            "srgb": True,
            "compression_settings": TC_DEFAULT,
        },
    },

    # 7 scalar/data maps: restore sRGB exactly as it was.
    {
        "asset": "/Game/GrimrockPrototype/Monsters/GoblinThrower/Textures/Body/T_Goblin_Bomber_Roughness.T_Goblin_Bomber_Roughness",
        "applied": {
            "srgb": False,
            "lod_group": WORLD,
            "compression_settings": TC_GRAYSCALE,
            "mip_gen_settings": MIPS_FROM_GROUP,
        },
        "restore": {"srgb": True},
    },
    {
        "asset": "/Game/GrimrockPrototype/Monsters/GoblinThrower/Textures/Cloth/T_Cloth_Bomber_Metallic.T_Cloth_Bomber_Metallic",
        "applied": {
            "srgb": False,
            "lod_group": WORLD,
            "compression_settings": TC_GRAYSCALE,
            "mip_gen_settings": MIPS_FROM_GROUP,
        },
        "restore": {"srgb": True},
    },
    {
        "asset": "/Game/GrimrockPrototype/Monsters/GoblinThrower/Textures/Cloth/T_Cloth_Bomber_Opacity.T_Cloth_Bomber_Opacity",
        "applied": {
            "srgb": False,
            "lod_group": WORLD,
            "compression_settings": TC_DEFAULT,
            "mip_gen_settings": MIPS_FROM_GROUP,
        },
        "restore": {"srgb": True},
    },
    {
        "asset": "/Game/GrimrockPrototype/Monsters/GoblinThrower/Textures/Cloth/T_Cloth_Bomber_Roughness.T_Cloth_Bomber_Roughness",
        "applied": {
            "srgb": False,
            "lod_group": WORLD,
            "compression_settings": TC_GRAYSCALE,
            "mip_gen_settings": MIPS_FROM_GROUP,
        },
        "restore": {"srgb": True},
    },
    {
        "asset": "/Game/GrimrockPrototype/Monsters/GoblinThrower/Textures/Hair/T_alpha.T_alpha",
        "applied": {
            "srgb": False,
            "lod_group": WORLD,
            "compression_settings": TC_DEFAULT,
            "mip_gen_settings": MIPS_FROM_GROUP,
        },
        "restore": {"srgb": True},
    },
    {
        "asset": "/Game/GrimrockPrototype/Monsters/RatGiant/Textures/T_RatGiant_Body_AO.T_RatGiant_Body_AO",
        "applied": {
            "srgb": False,
            "lod_group": WORLD,
            "compression_settings": TC_DEFAULT,
            "mip_gen_settings": MIPS_FROM_GROUP,
        },
        "restore": {"srgb": True},
    },
    {
        "asset": "/Game/GrimrockPrototype/Monsters/RatGiant/Textures/T_RatGiant_Fur_AO.T_RatGiant_Fur_AO",
        "applied": {
            "srgb": False,
            "lod_group": WORLD,
            "compression_settings": TC_DEFAULT,
            "mip_gen_settings": MIPS_FROM_GROUP,
        },
        "restore": {"srgb": True},
    },
]


def run():
    unreal.log("======================================================")
    unreal.log("TEXTURE-CLEAN01.1 - RECOVERY ROLLBACK")
    unreal.log("Explicit targets = " + str(len(ROLLBACKS)))
    unreal.log("======================================================")

    rows = []
    restored = 0
    skipped = 0
    missing = 0
    save_failures = 0

    for item in ROLLBACKS:
        asset_path = item["asset"]
        texture = unreal.load_asset(asset_path)

        row = {
            "AssetPath": asset_path,
            "Status": "",
            "BeforeSRGB": "",
            "BeforeLODGroup": "",
            "BeforeCompression": "",
            "BeforeMipGen": "",
            "AfterSRGB": "",
            "AfterLODGroup": "",
            "AfterCompression": "",
            "AfterMipGen": "",
            "Message": "",
        }

        if not isinstance(texture, unreal.Texture2D):
            row["Status"] = "MISSING_OR_NOT_TEXTURE2D"
            row["Message"] = "Asset could not be loaded as Texture2D."
            rows.append(row)
            missing += 1
            unreal.log_error("[ROLLBACK] Missing/not Texture2D: " + asset_path)
            continue

        before = state_of(texture)
        before_text = state_text(before)

        row["BeforeSRGB"] = before_text["SRGB"]
        row["BeforeLODGroup"] = before_text["LODGroup"]
        row["BeforeCompression"] = before_text["Compression"]
        row["BeforeMipGen"] = before_text["MipGen"]

        mismatch_list = mismatches(before, item["applied"])
        if mismatch_list:
            row["Status"] = "SKIPPED_PRECONDITION"
            row["Message"] = " | ".join(mismatch_list)
            rows.append(row)
            skipped += 1
            unreal.log_warning(
                "[ROLLBACK] Current state no longer matches CLEAN01.1 apply; skipped: {} :: {}".format(
                    asset_path,
                    row["Message"]
                )
            )
            continue

        for key, value in item["restore"].items():
            texture.set_editor_property(key, value)

        after = state_of(texture)
        after_text = state_text(after)

        row["AfterSRGB"] = after_text["SRGB"]
        row["AfterLODGroup"] = after_text["LODGroup"]
        row["AfterCompression"] = after_text["Compression"]
        row["AfterMipGen"] = after_text["MipGen"]

        save_ok = unreal.EditorAssetLibrary.save_loaded_asset(texture)

        if save_ok:
            row["Status"] = "RESTORED"
            row["Message"] = "Restored only metadata changed by TEXTURE-CLEAN01.1."
            restored += 1
            unreal.log("[ROLLBACK] RESTORED " + asset_path)
        else:
            row["Status"] = "SAVE_FAILED"
            row["Message"] = "Restore applied in memory but save_loaded_asset returned False."
            save_failures += 1
            unreal.log_error("[ROLLBACK] Save failed: " + asset_path)

        rows.append(row)

    fieldnames = [
        "AssetPath",
        "Status",
        "BeforeSRGB",
        "BeforeLODGroup",
        "BeforeCompression",
        "BeforeMipGen",
        "AfterSRGB",
        "AfterLODGroup",
        "AfterCompression",
        "AfterMipGen",
        "Message",
    ]

    with open(csv_path, "w", newline="", encoding="utf-8-sig") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames, delimiter=";")
        writer.writeheader()
        writer.writerows(rows)

    summary_lines = [
        "TEXTURE-CLEAN01.1 - Recovery rollback",
        "=" * 60,
        "",
        "Explicit targets     : " + str(len(ROLLBACKS)),
        "Restored             : " + str(restored),
        "Skipped precondition : " + str(skipped),
        "Missing/not Texture2D: " + str(missing),
        "Save failures        : " + str(save_failures),
        "",
        "CSV                  : " + csv_path,
    ]

    with open(summary_path, "w", encoding="utf-8") as handle:
        handle.write("\n".join(summary_lines))

    unreal.log("")
    unreal.log("TEXTURE-CLEAN01.1 rollback completed.")
    unreal.log("Restored = " + str(restored))
    unreal.log("Skipped precondition = " + str(skipped))
    unreal.log("Missing/not Texture2D = " + str(missing))
    unreal.log("Save failures = " + str(save_failures))
    unreal.log("CSV = " + csv_path)
    unreal.log("Summary = " + summary_path)


run()
