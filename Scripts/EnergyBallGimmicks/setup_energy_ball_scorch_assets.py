"""Configure the project-owned Energy Ball scorch texture and decal material."""

from __future__ import annotations

import unreal


EFFECT_ROOT = "/Game/GameAnimationSample/Gimmicks/EnergyBall/Effects"
TEXTURE_PATH = f"{EFFECT_ROOT}/T_EnergyBallScorch"
MATERIAL_PATH = f"{EFFECT_ROOT}/M_EnergyBallScorch"


def load_required(path: str):
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError(f"Required asset was not found: {path}")
    return asset


def get_or_create_material() -> unreal.Material:
    material = unreal.load_asset(MATERIAL_PATH)
    if material is None:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "M_EnergyBallScorch",
            EFFECT_ROOT,
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )
    if material is None:
        raise RuntimeError(f"Could not create material: {MATERIAL_PATH}")
    return material


def configure_texture(texture: unreal.Texture2D) -> None:
    texture.set_editor_property("srgb", False)
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    texture.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
    texture.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)


def configure_material(texture: unreal.Texture2D) -> unreal.Material:
    material = get_or_create_material()
    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

    texture_sample = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSample, -520, 80
    )
    texture_sample.set_editor_property("texture", texture)
    texture_sample.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)

    lifetime = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionDecalLifetimeOpacity, -520, 240
    )
    opacity = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -260, 130
    )
    base_color = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant3Vector, -260, -120
    )
    base_color.set_editor_property("constant", unreal.LinearColor(0.012, 0.006, 0.0025, 1.0))
    roughness = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant, -260, 330
    )
    roughness.set_editor_property("r", 0.96)

    unreal.MaterialEditingLibrary.connect_material_expressions(texture_sample, "A", opacity, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(lifetime, "", opacity, "B")
    unreal.MaterialEditingLibrary.connect_material_property(
        base_color, "", unreal.MaterialProperty.MP_BASE_COLOR
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        opacity, "", unreal.MaterialProperty.MP_OPACITY
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        roughness, "", unreal.MaterialProperty.MP_ROUGHNESS
    )

    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def main() -> None:
    texture = load_required(TEXTURE_PATH)
    configure_texture(texture)
    material = configure_material(texture)
    unreal.log(
        f"ENERGY_BALL_SCORCH_ASSET texture={texture.get_path_name()} "
        f"material={material.get_path_name()} domain={material.get_editor_property('material_domain')}"
    )
    unreal.log("ENERGY_BALL_SCORCH_ASSET_SETUP_COMPLETE")


main()
