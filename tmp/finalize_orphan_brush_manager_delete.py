import json
import os

import unreal


EXPECTED_WORLD = "/Game/Map/TinoKingdom_ByChanWoong.TinoKingdom_ByChanWoong"
TARGET_ACTOR = "Landscape_WaterBrushManager_1"
KEEP_ACTOR = "Landscape_WaterBrushManager_0"
KEEP_WATER_BODY = "WaterBodyRiver_0"
PACKAGE_NAME = "/Game/__ExternalActors__/Map/TinoKingdom_ByChanWoong/F/0Y/Q1RS8MWDSIEF069OM6R8SR"

editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = editor.get_editor_world()
if not world or world.get_path_name() != EXPECTED_WORLD:
    raise RuntimeError("Unexpected editor world: " + (world.get_path_name() if world else "None"))

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
actor_names = {actor.get_name() for actor in actors}
if TARGET_ACTOR in actor_names:
    raise RuntimeError(TARGET_ACTOR + " is still present")
if KEEP_ACTOR not in actor_names:
    raise RuntimeError(KEEP_ACTOR + " is missing")
if KEEP_WATER_BODY not in actor_names:
    raise RuntimeError(KEEP_WATER_BODY + " is missing")

package = unreal.find_package(PACKAGE_NAME)
if not package:
    raise RuntimeError("Target package is not loaded: " + PACKAGE_NAME)
if not unreal.EditorLoadingAndSavingUtils.save_packages([package], False):
    raise RuntimeError("Failed to finalize orphan WaterBrushManager deletion")

unreal.SystemLibrary.collect_garbage()
filename = os.path.join(
    unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()),
    "__ExternalActors__", "Map", "TinoKingdom_ByChanWoong",
    "F", "0Y", "Q1RS8MWDSIEF069OM6R8SR.uasset",
)
print("CODEX_ORPHAN_BRUSH_MANAGER_DELETE=" + json.dumps({
    "deleted_actor": TARGET_ACTOR,
    "kept_actor": KEEP_ACTOR,
    "kept_water_body": KEEP_WATER_BODY,
    "file_remaining": os.path.isfile(filename),
}, ensure_ascii=False))
