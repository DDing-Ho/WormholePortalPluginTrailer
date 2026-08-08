"""Apply the authored Energy Ball device visuals without moving level actors.

Set ENERGY_BALL_LEVEL_INSPECT_ONLY=1 to report the native actors and their
visual components without changing or saving the level.
"""

from __future__ import annotations

import os

import unreal


MAP_PATH = "/Game/Levels/LineTrace_Trailer"
MESH_ROOT = "/Game/GameAnimationSample/Gimmicks/EnergyBall/Meshes"
MATERIAL_ROOT = "/Game/GameAnimationSample/Gimmicks/EnergyBall/Materials"


def load_required(path: str):
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError(f"Required asset was not found: {path}")
    return asset


def load_native_class(class_name: str):
    native_class = unreal.load_class(None, f"/Script/GameAnimationSample.{class_name}")
    if native_class is None:
        raise RuntimeError(f"Native class is unavailable: {class_name}")
    return native_class


def find_component(actor: unreal.Actor, component_name: str) -> unreal.StaticMeshComponent:
    for component in actor.get_components_by_class(unreal.StaticMeshComponent):
        if component.get_name() == component_name:
            return component
    raise RuntimeError(f"{actor.get_actor_label()} does not contain {component_name}")


def describe_actor(actor: unreal.Actor, component: unreal.StaticMeshComponent) -> None:
    location = actor.get_actor_location()
    rotation = actor.get_actor_rotation()
    mesh = component.get_editor_property("static_mesh")
    mesh_path = mesh.get_path_name() if mesh is not None else "<None>"
    scale = component.get_relative_transform().scale3d
    unreal.log(
        f"ENERGY_BALL_LEVEL_ACTOR label={actor.get_actor_label()} class={actor.get_class().get_name()} "
        f"location=({location.x:.2f},{location.y:.2f},{location.z:.2f}) "
        f"rotation=({rotation.pitch:.2f},{rotation.yaw:.2f},{rotation.roll:.2f}) "
        f"mesh={mesh_path} scale=({scale.x:.2f},{scale.y:.2f},{scale.z:.2f})"
    )


def apply_visual(
    actor: unreal.Actor,
    component_name: str,
    mesh: unreal.StaticMesh,
    materials: dict[str, unreal.MaterialInterface],
) -> None:
    component = find_component(actor, component_name)
    component.set_static_mesh(mesh)
    component.set_relative_scale3d(unreal.Vector(1.0, 1.0, 1.0))

    static_materials = mesh.get_editor_property("static_materials")
    for index, entry in enumerate(static_materials):
        slot_name = str(entry.get_editor_property("material_slot_name"))
        if slot_name in materials:
            component.set_material(index, materials[slot_name])
    component.modify()
    actor.modify()
    describe_actor(actor, component)


def main() -> None:
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Could not load map: {MAP_PATH}")

    emitter_class = load_native_class("EnergyBallEmitter")
    receiver_class = load_native_class("EnergyBallReceiver")
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = list(actor_subsystem.get_all_level_actors())
    emitters = [actor for actor in actors if actor.get_class() == emitter_class]
    receivers = [actor for actor in actors if actor.get_class() == receiver_class]
    if not emitters or not receivers:
        raise RuntimeError(
            f"Expected existing Energy Ball actors; emitters={len(emitters)} receivers={len(receivers)}"
        )

    inspect_only = os.environ.get("ENERGY_BALL_LEVEL_INSPECT_ONLY", "0") == "1"
    if inspect_only:
        for actor in sorted(emitters, key=lambda value: value.get_actor_label()):
            describe_actor(actor, find_component(actor, "EmitterMesh"))
        for actor in sorted(receivers, key=lambda value: value.get_actor_label()):
            describe_actor(actor, find_component(actor, "ReceiverMesh"))
        unreal.log(
            f"ENERGY_BALL_LEVEL_INSPECTION_COMPLETE emitters={len(emitters)} receivers={len(receivers)}"
        )
        return

    materials = {
        "Shell": load_required(f"{MATERIAL_ROOT}/MI_EnergyBallShell"),
        "Mechanism": load_required(f"{MATERIAL_ROOT}/MI_EnergyBallMechanism"),
        "Optic": load_required(f"{MATERIAL_ROOT}/MI_EnergyBallOptic"),
    }
    emitter_mesh = load_required(f"{MESH_ROOT}/SM_EnergyBallEmitter")
    receiver_mesh = load_required(f"{MESH_ROOT}/SM_EnergyBallReceiver")

    for actor in emitters:
        apply_visual(actor, "EmitterMesh", emitter_mesh, materials)
    for actor in receivers:
        apply_visual(actor, "ReceiverMesh", receiver_mesh, materials)

    level_editor_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level_editor_subsystem.save_current_level():
        raise RuntimeError(f"Could not save map: {MAP_PATH}")
    unreal.log(
        f"ENERGY_BALL_LEVEL_SETUP_COMPLETE emitters={len(emitters)} receivers={len(receivers)}"
    )


main()
