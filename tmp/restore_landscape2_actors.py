import json
import unreal


EXPECTED_WORLD = "/Game/Map/TinoKingdom_ByChanWoong.TinoKingdom_ByChanWoong"
PACKAGE_PATHS = [
    "/Game/__ExternalActors__/Map/TinoKingdom_ByChanWoong/E/EN/54W4AFR0MEFGMMG4PNMNOG",
    "/Game/__ExternalActors__/Map/TinoKingdom_ByChanWoong/3/EG/1H8CUH8T3XMS28T6A61HGL",
    "/Game/__ExternalActors__/Map/TinoKingdom_ByChanWoong/6/9R/0BJ34ZXROLIN8HNCPNL5TP",
    "/Game/__ExternalActors__/Map/TinoKingdom_ByChanWoong/C/UE/3PQGINNZKP8UG1OTVBWSW7",
    "/Game/__ExternalActors__/Map/TinoKingdom_ByChanWoong/D/JD/G5UMX8IDAIWJONB3W32XUM",
    "/Game/__ExternalActors__/Map/TinoKingdom_ByChanWoong/2/V5/ZP8NBUCRBXJBJMRF8TSI0O",
]
EXPECTED_LABELS = {
    "Landscape2",
    "LandscapeSplineActor5",
    "LandscapeSplineActor6",
    "LandscapeSplineActor7",
    "LandscapeSplineActor8",
    "LandscapeSplineActor9",
}


editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = editor.get_editor_world()
if not world or world.get_path_name() != EXPECTED_WORLD:
    raise RuntimeError("Unexpected editor world: " + (world.get_path_name() if world else "None"))

packages = []
for path in PACKAGE_PATHS:
    package = unreal.find_package(path) or unreal.load_package(path)
    if not package:
        raise RuntimeError("Could not find or load package: " + path)
    packages.append(package)

reloaded, error_message = unreal.EditorLoadingAndSavingUtils.reload_packages(
    packages,
    unreal.ReloadPackagesInteractionMode.ASSUME_POSITIVE,
)
if not reloaded:
    raise RuntimeError("Package reload failed: " + str(error_message))

actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
found = {}
for actor in actor_subsystem.get_all_level_actors():
    label = actor.get_actor_label()
    if label in EXPECTED_LABELS:
        found[label] = {
            "path": actor.get_path_name(),
            "package": actor.get_outermost().get_name(),
            "class": actor.get_class().get_name(),
        }

missing = sorted(EXPECTED_LABELS.difference(found))
if missing:
    raise RuntimeError("Reload completed but actors are still missing: " + ", ".join(missing))

print("CODEX_LANDSCAPE2_RESTORE_OK=" + json.dumps(found, ensure_ascii=False))
