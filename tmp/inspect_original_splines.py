import json
import unreal


MAP_PATH = "/Game/Map/TinoKingdomMap_test"


def safe_property(obj, name):
    try:
        value = obj.get_editor_property(name)
        if isinstance(value, unreal.Object):
            return value.get_path_name()
        if isinstance(value, (list, tuple, unreal.Array)):
            return [str(item) for item in value]
        return value
    except Exception as exc:
        return {"error": str(exc)}


unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
world = unreal.EditorLevelLibrary.get_editor_world()
actors = list(unreal.ActorIterator(world))
print("CODEX_ACTOR_COUNT=" + str(len(actors)))
result = []

for actor in actors:
    if actor.get_class().get_name() != "LandscapeSplineActor":
        continue
    components = actor.get_components_by_class(unreal.ActorComponent)
    spline_components = [
        component for component in components
        if component.get_class().get_name() == "LandscapeSplinesComponent"
    ]
    origin, extent = actor.get_actor_bounds(False)
    entry = {
        "name": actor.get_name(),
        "label": actor.get_actor_label(),
        "path": actor.get_path_name(),
        "landscape_actor": safe_property(actor, "landscape_actor"),
        "bounds_origin": [origin.x, origin.y, origin.z],
        "bounds_extent": [extent.x, extent.y, extent.z],
        "spline_components": [],
    }
    for component in spline_components:
        entry["spline_components"].append({
            "name": component.get_name(),
            "control_points": safe_property(component, "control_points"),
            "segments": safe_property(component, "segments"),
        })
    result.append(entry)

print("CODEX_SPLINE_DUMP=" + json.dumps(result, ensure_ascii=False, default=str))

landscapes = []
for actor in actors:
    if actor.get_class().get_name() != "Landscape":
        continue
    components = actor.get_components_by_class(unreal.ActorComponent)
    spline_components = [
        component for component in components
        if component.get_class().get_name() == "LandscapeSplinesComponent"
    ]
    entry = {
        "name": actor.get_name(),
        "label": actor.get_actor_label(),
        "path": actor.get_path_name(),
        "spline_components": [],
    }
    for component in spline_components:
        entry["spline_components"].append({
            "name": component.get_name(),
            "control_points": safe_property(component, "control_points"),
            "segments": safe_property(component, "segments"),
        })
    landscapes.append(entry)
print("CODEX_LANDSCAPE_SPLINES=" + json.dumps(landscapes, ensure_ascii=False, default=str))
