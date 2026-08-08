"""Create laser material instances and bind them to the imported Static Mesh slots.

Run through UnrealEditor-Cmd with the PythonScript commandlet. This script is
idempotent so reimported source meshes can be configured again safely.
"""

from __future__ import annotations

import unreal


MESH_ROOT = "/Game/GameAnimationSample/Gimmicks/Laser/Meshes"
MATERIAL_ROOT = "/Game/GameAnimationSample/Gimmicks/Laser/Materials"


def load_required(path: str):
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError(f"Required asset was not found: {path}")
    return asset


def get_or_create_material_instance(name: str, parent_path: str) -> unreal.MaterialInstanceConstant:
    path = f"{MATERIAL_ROOT}/{name}"
    instance = unreal.load_asset(path)
    if instance is None:
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        factory = unreal.MaterialInstanceConstantFactoryNew()
        instance = tools.create_asset(name, MATERIAL_ROOT, unreal.MaterialInstanceConstant, factory)
    if instance is None:
        raise RuntimeError(f"Could not create material instance: {path}")

    parent = load_required(parent_path)
    unreal.MaterialEditingLibrary.set_material_instance_parent(instance, parent)
    return instance


def configure_materials() -> dict[str, unreal.MaterialInterface]:
    shell = get_or_create_material_instance(
        "MI_LaserShell",
        "/Game/Levels/LevelPrototyping/Materials/M_Solid",
    )
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        shell, "BaseColor", unreal.LinearColor(0.82, 0.84, 0.82, 1.0)
    )
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(shell, "Metallic", 0.05)
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(shell, "Roughness", 0.38)

    mechanism = get_or_create_material_instance(
        "MI_LaserMechanism",
        "/Game/Levels/LevelPrototyping/Materials/M_Solid",
    )
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        mechanism, "BaseColor", unreal.LinearColor(0.018, 0.024, 0.032, 1.0)
    )
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(mechanism, "Metallic", 0.78)
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(mechanism, "Roughness", 0.24)

    optic = get_or_create_material_instance(
        "MI_LaserOptic",
        "/Game/WormholePortal/VFX/Materials/M_PortalEnergy_Additive",
    )
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        optic, "EffectColor", unreal.LinearColor(0.04, 0.09, 0.12, 1.0)
    )
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(optic, "EmissiveStrength", 0.35)

    for material in (shell, mechanism, optic):
        unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)

    return {"Shell": shell, "Mechanism": mechanism, "Optic": optic}


def configure_mesh(mesh_name: str, materials: dict[str, unreal.MaterialInterface]) -> None:
    mesh = load_required(f"{MESH_ROOT}/{mesh_name}")
    static_materials = mesh.get_editor_property("static_materials")
    slot_names = [str(entry.get_editor_property("material_slot_name")) for entry in static_materials]
    expected = {"Shell", "Mechanism", "Optic"}
    if set(slot_names) != expected or len(slot_names) != 3:
        raise RuntimeError(f"{mesh_name} has invalid material slots: {slot_names}; expected {sorted(expected)}")

    for entry in static_materials:
        slot_name = str(entry.get_editor_property("material_slot_name"))
        entry.set_editor_property("material_interface", materials[slot_name])
    mesh.set_editor_property("static_materials", static_materials)

    unreal.log(f"LASER_ASSET {mesh_name}: slots={slot_names}")
    unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)


def main() -> None:
    materials = configure_materials()
    for mesh_name in ("SM_LaserEmitter", "SM_LaserReceiver", "SM_LaserRedirectorCube"):
        configure_mesh(mesh_name, materials)
    unreal.log("LASER_ASSET_SETUP_COMPLETE")


main()
