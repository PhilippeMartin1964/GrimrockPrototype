import unreal
import os
import csv
from datetime import datetime

# ============================================================================
# TEXTURE-CLEAN01.1 - Safe Metadata Fixes
# Unreal Engine 5.5.x
#
# Default mode is DRY RUN.
# Set APPLY_CHANGES = True only after validating the dry-run report.
#
# Scope is intentionally explicit: exactly 14 audited Texture2D assets.
# This script does NOT resize/reimport textures and does NOT touch:
# - Teleporter/Vines world textures with non-POT sources / NoMipmaps
# - legacy UI mip cleanup
# - Flame 8K textures
# - UI textures using VectorDisplacement compression
# ============================================================================

APPLY_CHANGES = False

OUTPUT_SUBDIR = "TextureClean01.1"

timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
output_dir = os.path.join(unreal.Paths.project_saved_dir(), OUTPUT_SUBDIR)
os.makedirs(output_dir, exist_ok=True)

mode_name = "APPLY" if APPLY_CHANGES else "DRYRUN"
csv_path = os.path.join(
    output_dir,
    "TextureClean01_1_{}_{}.csv".format(mode_name, timestamp)
)
summary_path = os.path.join(
    output_dir,
    "TextureClean01_1_{}_{}_summary.txt".format(mode_name, timestamp)
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


def values_equal(left, right):
    return left == right


def describe_changes(before, target):
    changes = []

    labels = {
        "srgb": "sRGB",
        "lod_group": "LODGroup",
        "compression_settings": "Compression",
        "mip_gen_settings": "MipGen",
    }

    for key, target_value in target.items():
        before_value = before[key]

        if values_equal(before_value, target_value):
            continue

        if isinstance(before_value, bool):
            before_text = str(before_value)
            target_text = str(target_value)
        else:
            before_text = enum_name(before_value)
            target_text = enum_name(target_value)

        changes.append(
            "{}: {} -> {}".format(
                labels[key],
                before_text,
                target_text
            )
        )

    return changes


def matches_expected(current, expected):
    mismatches = []

    labels = {
        "srgb": "sRGB",
        "lod_group": "LODGroup",
        "compression_settings": "Compression",
        "mip_gen_settings": "MipGen",
    }

    for key, expected_value in expected.items():
        current_value = current[key]

        if values_equal(current_value, expected_value):
            continue

        if isinstance(current_value, bool):
            current_text = str(current_value)
            expected_text = str(expected_value)
        else:
            current_text = enum_name(current_value)
            expected_text = enum_name(expected_value)

        mismatches.append(
            "{} expected {} but found {}".format(
                labels[key],
                expected_text,
                current_text
            )
        )

    return mismatches


WORLD = unreal.TextureGroup.TEXTUREGROUP_WORLD
UI = unreal.TextureGroup.TEXTUREGROUP_UI

TC_DEFAULT = unreal.TextureCompressionSettings.TC_DEFAULT
TC_MASKS = unreal.TextureCompressionSettings.TC_MASKS
TC_GRAYSCALE = unreal.TextureCompressionSettings.TC_GRAYSCALE

MIPS_FROM_GROUP = unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP
NO_MIPS = unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS


# Each entry contains:
# - exact audited asset path
# - exact V2 precondition
# - only the metadata fields allowed to change in this ticket
FIXES = [
    # ------------------------------------------------------------------------
    # 4 talent illustrations: only Texture Group World -> UI.
    # Compression/default, NoMipmaps and sRGB are intentionally preserved.
    # ------------------------------------------------------------------------
    {
        "label": "Talent Alchemist Grenadier -> UI group",
        "asset": "/Game/GrimrockPrototype/Blueprints/UI/Skills/Images/Alchemist/T_Talent_AlchemistGrenadier.T_Talent_AlchemistGrenadier",
        "expected": {
            "srgb": True,
            "lod_group": WORLD,
            "compression_settings": TC_DEFAULT,
            "mip_gen_settings": NO_MIPS,
        },
        "target": {
            "lod_group": UI,
        },
    },
    {
        "label": "Talent Ranger Hunter -> UI group",
        "asset": "/Game/GrimrockPrototype/Blueprints/UI/Skills/Images/Ranger/T_Talent_RangerHunter.T_Talent_RangerHunter",
        "expected": {
            "srgb": True,
            "lod_group": WORLD,
            "compression_settings": TC_DEFAULT,
            "mip_gen_settings": NO_MIPS,
        },
        "target": {
            "lod_group": UI,
        },
    },
    {
        "label": "Talent Ranger Marksman -> UI group",
        "asset": "/Game/GrimrockPrototype/Blueprints/UI/Skills/Images/Ranger/T_Talent_RangerMarksman.T_Talent_RangerMarksman",
        "expected": {
            "srgb": True,
            "lod_group": WORLD,
            "compression_settings": TC_DEFAULT,
            "mip_gen_settings": NO_MIPS,
        },
        "target": {
            "lod_group": UI,
        },
    },
    {
        "label": "Talent Ranger Scout -> UI group",
        "asset": "/Game/GrimrockPrototype/Blueprints/UI/Skills/Images/Ranger/T_Talent_RangerScout.T_Talent_RangerScout",
        "expected": {
            "srgb": True,
            "lod_group": WORLD,
            "compression_settings": TC_DEFAULT,
            "mip_gen_settings": NO_MIPS,
        },
        "target": {
            "lod_group": UI,
        },
    },

    # ------------------------------------------------------------------------
    # 3 packed ORM maps: linear data + Masks compression.
    # Existing World group and mip policy are intentionally preserved.
    # ------------------------------------------------------------------------
    {
        "label": "Granite ORM -> linear Masks",
        "asset": "/Game/GrimrockPrototype/Art/Textures/Final/Assets/Granite_01/T_Granite_01_ORM.T_Granite_01_ORM",
        "expected": {
            "srgb": True,
            "lod_group": WORLD,
            "compression_settings": TC_DEFAULT,
            "mip_gen_settings": MIPS_FROM_GROUP,
        },
        "target": {
            "srgb": False,
            "compression_settings": TC_MASKS,
        },
    },
    {
        "label": "Wood 03 ORM -> linear Masks",
        "asset": "/Game/GrimrockPrototype/Art/Textures/Final/Door/Wood_03/T_Wood_03_ORM.T_Wood_03_ORM",
        "expected": {
            "srgb": True,
            "lod_group": WORLD,
            "compression_settings": TC_DEFAULT,
            "mip_gen_settings": MIPS_FROM_GROUP,
        },
        "target": {
            "srgb": False,
            "compression_settings": TC_MASKS,
        },
    },
    {
        "label": "Marble ORM -> linear Masks",
        "asset": "/Game/GrimrockPrototype/Art/Textures/Final/Wall/Marble_01/T_Marble_01_ORM.T_Marble_01_ORM",
        "expected": {
            "srgb": True,
            "lod_group": WORLD,
            "compression_settings": TC_DEFAULT,
            "mip_gen_settings": MIPS_FROM_GROUP,
        },
        "target": {
            "srgb": False,
            "compression_settings": TC_MASKS,
        },
    },

    # ------------------------------------------------------------------------
    # 7 scalar/data maps: only sRGB True -> False.
    # Compression is intentionally preserved to minimize impact.
    # ------------------------------------------------------------------------
    {
        "label": "Goblin Body Roughness -> linear",
        "asset": "/Game/GrimrockPrototype/Monsters/GoblinThrower/Textures/Body/T_Goblin_Bomber_Roughness.T_Goblin_Bomber_Roughness",
        "expected": {
            "srgb": True,
            "lod_group": WORLD,
            "compression_settings": TC_GRAYSCALE,
            "mip_gen_settings": MIPS_FROM_GROUP,
        },
        "target": {
            "srgb": False,
        },
    },
    {
        "label": "Goblin Cloth Metallic -> linear",
        "asset": "/Game/GrimrockPrototype/Monsters/GoblinThrower/Textures/Cloth/T_Cloth_Bomber_Metallic.T_Cloth_Bomber_Metallic",
        "expected": {
            "srgb": True,
            "lod_group": WORLD,
            "compression_settings": TC_GRAYSCALE,
            "mip_gen_settings": MIPS_FROM_GROUP,
        },
        "target": {
            "srgb": False,
        },
    },
    {
        "label": "Goblin Cloth Opacity -> linear",
        "asset": "/Game/GrimrockPrototype/Monsters/GoblinThrower/Textures/Cloth/T_Cloth_Bomber_Opacity.T_Cloth_Bomber_Opacity",
        "expected": {
            "srgb": True,
            "lod_group": WORLD,
            "compression_settings": TC_DEFAULT,
            "mip_gen_settings": MIPS_FROM_GROUP,
        },
        "target": {
            "srgb": False,
        },
    },
    {
        "label": "Goblin Cloth Roughness -> linear",
        "asset": "/Game/GrimrockPrototype/Monsters/GoblinThrower/Textures/Cloth/T_Cloth_Bomber_Roughness.T_Cloth_Bomber_Roughness",
        "expected": {
            "srgb": True,
            "lod_group": WORLD,
            "compression_settings": TC_GRAYSCALE,
            "mip_gen_settings": MIPS_FROM_GROUP,
        },
        "target": {
            "srgb": False,
        },
    },
    {
        "label": "Goblin Hair Alpha -> linear",
        "asset": "/Game/GrimrockPrototype/Monsters/GoblinThrower/Textures/Hair/T_alpha.T_alpha",
        "expected": {
            "srgb": True,
            "lod_group": WORLD,
            "compression_settings": TC_DEFAULT,
            "mip_gen_settings": MIPS_FROM_GROUP,
        },
        "target": {
            "srgb": False,
        },
    },
    {
        "label": "WereRat Body AO -> linear",
        "asset": "/Game/GrimrockPrototype/Monsters/RatGiant/Textures/T_RatGiant_Body_AO.T_RatGiant_Body_AO",
        "expected": {
            "srgb": True,
            "lod_group": WORLD,
            "compression_settings": TC_DEFAULT,
            "mip_gen_settings": MIPS_FROM_GROUP,
        },
        "target": {
            "srgb": False,
        },
    },
    {
        "label": "WereRat Fur AO -> linear",
        "asset": "/Game/GrimrockPrototype/Monsters/RatGiant/Textures/T_RatGiant_Fur_AO.T_RatGiant_Fur_AO",
        "expected": {
            "srgb": True,
            "lod_group": WORLD,
            "compression_settings": TC_DEFAULT,
            "mip_gen_settings": MIPS_FROM_GROUP,
        },
        "target": {
            "srgb": False,
        },
    },
]


def run():
    unreal.log("======================================================")
    unreal.log("TEXTURE-CLEAN01.1 - Safe Metadata Fixes")
    unreal.log("Mode = " + mode_name)
    unreal.log("Explicit targets = " + str(len(FIXES)))
    unreal.log("======================================================")

    results = []

    planned = 0
    applied = 0
    already_target = 0
    skipped_precondition = 0
    missing = 0
    save_failures = 0

    for fix in FIXES:
        asset_path = fix["asset"]

        texture = unreal.load_asset(asset_path)

        row = {
            "AssetPath": asset_path,
            "Label": fix["label"],
            "Mode": mode_name,
            "Status": "",
            "BeforeSRGB": "",
            "BeforeLODGroup": "",
            "BeforeCompression": "",
            "BeforeMipGen": "",
            "TargetChanges": "",
            "AfterSRGB": "",
            "AfterLODGroup": "",
            "AfterCompression": "",
            "AfterMipGen": "",
            "Message": "",
        }

        if not isinstance(texture, unreal.Texture2D):
            row["Status"] = "MISSING_OR_NOT_TEXTURE2D"
            row["Message"] = "Asset could not be loaded as Texture2D."
            results.append(row)
            missing += 1
            unreal.log_error(
                "[TEXTURE-CLEAN01.1] Missing/not Texture2D: " + asset_path
            )
            continue

        before = state_of(texture)
        before_text = state_text(before)

        row["BeforeSRGB"] = before_text["SRGB"]
        row["BeforeLODGroup"] = before_text["LODGroup"]
        row["BeforeCompression"] = before_text["Compression"]
        row["BeforeMipGen"] = before_text["MipGen"]

        mismatches = matches_expected(before, fix["expected"])
        if mismatches:
            row["Status"] = "SKIPPED_PRECONDITION"
            row["Message"] = " | ".join(mismatches)
            results.append(row)
            skipped_precondition += 1
            unreal.log_warning(
                "[TEXTURE-CLEAN01.1] Precondition mismatch; skipped: {} :: {}".format(
                    asset_path,
                    row["Message"]
                )
            )
            continue

        target_state = dict(before)
        target_state.update(fix["target"])

        changes = describe_changes(before, target_state)
        row["TargetChanges"] = " | ".join(changes)

        if not changes:
            row["Status"] = "ALREADY_TARGET"
            row["Message"] = "No metadata change required."
            after_text = state_text(before)
            row["AfterSRGB"] = after_text["SRGB"]
            row["AfterLODGroup"] = after_text["LODGroup"]
            row["AfterCompression"] = after_text["Compression"]
            row["AfterMipGen"] = after_text["MipGen"]
            results.append(row)
            already_target += 1
            unreal.log(
                "[TEXTURE-CLEAN01.1] Already target: " + asset_path
            )
            continue

        planned += 1

        if not APPLY_CHANGES:
            row["Status"] = "PLANNED"
            row["Message"] = row["TargetChanges"]
            after_text = state_text(target_state)
            row["AfterSRGB"] = after_text["SRGB"]
            row["AfterLODGroup"] = after_text["LODGroup"]
            row["AfterCompression"] = after_text["Compression"]
            row["AfterMipGen"] = after_text["MipGen"]
            results.append(row)
            unreal.log(
                "[TEXTURE-CLEAN01.1] PLAN {} :: {}".format(
                    asset_path,
                    row["TargetChanges"]
                )
            )
            continue

        # APPLY mode: only fields present in target may be modified.
        for key, value in fix["target"].items():
            texture.set_editor_property(key, value)

        after = state_of(texture)

        target_mismatches = []
        for key, expected_target in fix["target"].items():
            if not values_equal(after[key], expected_target):
                target_mismatches.append(
                    "{} did not reach target".format(key)
                )

        if target_mismatches:
            row["Status"] = "APPLY_VERIFY_FAILED"
            row["Message"] = " | ".join(target_mismatches)
            after_text = state_text(after)
            row["AfterSRGB"] = after_text["SRGB"]
            row["AfterLODGroup"] = after_text["LODGroup"]
            row["AfterCompression"] = after_text["Compression"]
            row["AfterMipGen"] = after_text["MipGen"]
            results.append(row)
            unreal.log_error(
                "[TEXTURE-CLEAN01.1] Apply verification failed: {} :: {}".format(
                    asset_path,
                    row["Message"]
                )
            )
            continue

        save_ok = unreal.EditorAssetLibrary.save_loaded_asset(texture)
        after_text = state_text(after)

        row["AfterSRGB"] = after_text["SRGB"]
        row["AfterLODGroup"] = after_text["LODGroup"]
        row["AfterCompression"] = after_text["Compression"]
        row["AfterMipGen"] = after_text["MipGen"]

        if save_ok:
            row["Status"] = "APPLIED"
            row["Message"] = row["TargetChanges"]
            applied += 1
            unreal.log(
                "[TEXTURE-CLEAN01.1] APPLIED {} :: {}".format(
                    asset_path,
                    row["TargetChanges"]
                )
            )
        else:
            row["Status"] = "SAVE_FAILED"
            row["Message"] = "Metadata changed in memory but save_loaded_asset returned False."
            save_failures += 1
            unreal.log_error(
                "[TEXTURE-CLEAN01.1] Save failed: " + asset_path
            )

        results.append(row)

    fieldnames = [
        "AssetPath",
        "Label",
        "Mode",
        "Status",
        "BeforeSRGB",
        "BeforeLODGroup",
        "BeforeCompression",
        "BeforeMipGen",
        "TargetChanges",
        "AfterSRGB",
        "AfterLODGroup",
        "AfterCompression",
        "AfterMipGen",
        "Message",
    ]

    with open(csv_path, "w", newline="", encoding="utf-8-sig") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=fieldnames,
            delimiter=";"
        )
        writer.writeheader()
        writer.writerows(results)

    summary_lines = [
        "TEXTURE-CLEAN01.1 - Safe Metadata Fixes",
        "=" * 60,
        "",
        "Mode                 : " + mode_name,
        "Explicit targets     : " + str(len(FIXES)),
        "Planned changes      : " + str(planned),
        "Applied              : " + str(applied),
        "Already target       : " + str(already_target),
        "Skipped precondition : " + str(skipped_precondition),
        "Missing/not Texture2D: " + str(missing),
        "Save failures        : " + str(save_failures),
        "",
        "CSV                  : " + csv_path,
    ]

    with open(summary_path, "w", encoding="utf-8") as handle:
        handle.write("\n".join(summary_lines))

    unreal.log("")
    unreal.log("TEXTURE-CLEAN01.1 completed.")
    unreal.log("Mode = " + mode_name)
    unreal.log("Explicit targets = " + str(len(FIXES)))
    unreal.log("Planned changes = " + str(planned))
    unreal.log("Applied = " + str(applied))
    unreal.log("Already target = " + str(already_target))
    unreal.log("Skipped precondition = " + str(skipped_precondition))
    unreal.log("Missing/not Texture2D = " + str(missing))
    unreal.log("Save failures = " + str(save_failures))
    unreal.log("CSV = " + csv_path)
    unreal.log("Summary = " + summary_path)


run()
