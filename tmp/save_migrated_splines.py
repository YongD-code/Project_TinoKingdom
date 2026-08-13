import json
import unreal


EXPECTED_WORLD = "/Game/Map/TinoKingdom_ByChanWoong.TinoKingdom_ByChanWoong"
TARGET_LABEL = "Landscape"
SOURCE_LABELS = [
    "LandscapeSplineActor5",
    "LandscapeSplineActor6",
    "LandscapeSplineActor7",
    "LandscapeSplineActor8",
    "LandscapeSplineActor9",
]


editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = editor.get_editor_world()
if not world or world.get_path_name() != EXPECTED_WORLD:
    raise RuntimeError("Unexpected editor world: " + (world.get_path_name() if world else "None"))

actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = actor_subsystem.get_all_level_actors()
target = next(
    (actor for actor in actors if actor.get_class().get_name() == "Landscape" and actor.get_actor_label() == TARGET_LABEL),
    None,
)
if not target:
    raise RuntimeError("Target Landscape actor was not found")

sources_by_label = {
    actor.get_actor_label(): actor
    for actor in actors
    if actor.get_class().get_name() == "LandscapeSplineActor" and actor.get_actor_label() in SOURCE_LABELS
}
missing = [label for label in SOURCE_LABELS if label not in sources_by_label]
if missing:
    raise RuntimeError("Missing spline actors: " + ", ".join(missing))

packages = []
result = {}
for label in SOURCE_LABELS:
    actor = sources_by_label[label]
    actor.modify()
    actor.set_editor_property("landscape_actor", target)
    owner = actor.get_editor_property("landscape_actor")
    if owner != target:
        raise RuntimeError(label + " did not accept the target Landscape")
    package = actor.get_outermost()
    packages.append(package)
    result[label] = {
        "actor": actor.get_path_name(),
        "owner": owner.get_path_name(),
        "package": package.get_name(),
    }

if not unreal.EditorLoadingAndSavingUtils.save_packages(packages, False):
    raise RuntimeError("Failed to save one or more migrated spline packages")

dirty_after = [package.get_name() for package in packages if package.is_dirty()]
if dirty_after:
    raise RuntimeError("Spline packages are still dirty after save: " + ", ".join(dirty_after))

print("CODEX_SPLINE_MIGRATION_SAVED=" + json.dumps(result, ensure_ascii=False))
