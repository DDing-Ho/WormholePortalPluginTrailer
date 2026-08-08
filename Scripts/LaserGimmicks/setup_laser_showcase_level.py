"""Inspect or idempotently add the laser redirect showcase to LineTrace_Trailer.

Set LASER_LEVEL_INSPECT_ONLY=1 to load and report the existing laser actors
without changing or saving the level.
"""

from __future__ import annotations

import os

import unreal


MAP_PATH = "/Game/Levels/LineTrace_Trailer"
REDIRECTOR_LABEL = "LaserRedirector_Showcase"
RECEIVER_LABEL = "LaserReceiver_Showcase"
INCOMING_DISTANCE = 420.0
OUTPUT_DISTANCE = 420.0


def load_native_class(class_name: str):
    native_class = unreal.load_class(None, f"/Script/GameAnimationSample.{class_name}")
    if native_class is None:
        raise RuntimeError(f"Native class is unavailable: {class_name}")
    return native_class


def actor_label(actor: unreal.Actor) -> str:
    return actor.get_actor_label() if actor is not None else "<None>"


def is_exact_class(actor: unreal.Actor, native_class) -> bool:
    return actor is not None and actor.get_class() == native_class


def log_actor(actor: unreal.Actor) -> None:
    location = actor.get_actor_location()
    rotation = actor.get_actor_rotation()
    unreal.log(
        f"LASER_LEVEL_ACTOR label={actor_label(actor)} class={actor.get_class().get_name()} "
        f"location=({location.x:.2f},{location.y:.2f},{location.z:.2f}) "
        f"rotation=({rotation.pitch:.2f},{rotation.yaw:.2f},{rotation.roll:.2f})"
    )


def find_by_label(actors: list[unreal.Actor], label: str):
    for actor in actors:
        if actor_label(actor) == label:
            return actor
    return None


def safe_lateral_direction(forward: unreal.Vector, fallback: unreal.Vector) -> unreal.Vector:
    world_up = unreal.Vector(0.0, 0.0, 1.0)
    lateral = forward.cross(world_up)
    if lateral.length() < 0.1:
        lateral = fallback
    return lateral.normal()


def set_transform(actor: unreal.Actor, location: unreal.Vector, rotation: unreal.Rotator) -> None:
    if not actor.set_actor_location_and_rotation(location, rotation, False, False):
        raise RuntimeError(f"Could not set showcase transform for {actor_label(actor)}")


def main() -> None:
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Could not load map: {MAP_PATH}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = list(actor_subsystem.get_all_level_actors())
    emitter_class = load_native_class("LaserEmitter")
    receiver_class = load_native_class("LaserReceiver")
    redirector_class = load_native_class("LaserRedirectorCube")

    relevant_classes = {emitter_class, receiver_class, redirector_class}
    relevant_actors = [actor for actor in actors if actor.get_class() in relevant_classes]
    for actor in sorted(relevant_actors, key=actor_label):
        log_actor(actor)

    emitters = [actor for actor in actors if is_exact_class(actor, emitter_class)]
    if not emitters:
        raise RuntimeError("LineTrace_Trailer does not contain a LaserEmitter")

    emitter = sorted(emitters, key=actor_label)[0]
    if os.environ.get("LASER_LEVEL_INSPECT_ONLY", "0") == "1":
        unreal.log(f"LASER_LEVEL_INSPECTION_COMPLETE emitter={actor_label(emitter)}")
        return

    emitter_location = emitter.get_actor_location()
    forward = emitter.get_actor_forward_vector().normal()
    lateral = safe_lateral_direction(forward, emitter.get_actor_right_vector())

    redirector_location = emitter_location + forward * INCOMING_DISTANCE
    receiver_location = redirector_location + lateral * OUTPUT_DISTANCE
    redirector_rotation = unreal.MathLibrary.make_rot_from_x(lateral)
    receiver_rotation = unreal.MathLibrary.make_rot_from_x(lateral * -1.0)

    redirector = find_by_label(actors, REDIRECTOR_LABEL)
    if redirector is None:
        redirector = actor_subsystem.spawn_actor_from_class(
            redirector_class, redirector_location, redirector_rotation, transient=False
        )
        if redirector is None:
            raise RuntimeError("Could not spawn LaserRedirectorCube")
        redirector.set_actor_label(REDIRECTOR_LABEL, mark_dirty=True)
    elif not is_exact_class(redirector, redirector_class):
        raise RuntimeError(f"Actor label is already used by another class: {REDIRECTOR_LABEL}")
    set_transform(redirector, redirector_location, redirector_rotation)

    actors = list(actor_subsystem.get_all_level_actors())
    receiver = find_by_label(actors, RECEIVER_LABEL)
    if receiver is None:
        receiver = actor_subsystem.spawn_actor_from_class(
            receiver_class, receiver_location, receiver_rotation, transient=False
        )
        if receiver is None:
            raise RuntimeError("Could not spawn LaserReceiver")
        receiver.set_actor_label(RECEIVER_LABEL, mark_dirty=True)
    elif not is_exact_class(receiver, receiver_class):
        raise RuntimeError(f"Actor label is already used by another class: {RECEIVER_LABEL}")
    set_transform(receiver, receiver_location, receiver_rotation)

    log_actor(emitter)
    log_actor(redirector)
    log_actor(receiver)
    level_editor_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level_editor_subsystem.save_current_level():
        raise RuntimeError(f"Could not save map: {MAP_PATH}")
    unreal.log("LASER_LEVEL_SETUP_COMPLETE")


main()
