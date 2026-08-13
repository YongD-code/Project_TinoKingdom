import json
import unreal


MAP_PATH = "/Game/Map/TinoKingdom_ByChanWoong"


def prop_path(obj, name):
    try:
        value = obj.get_editor_property(name)
        return value.get_path_name() if value else None
    except Exception as exc:
        return {"error": str(exc)}


unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
world = unreal.EditorLevelLibrary.get_editor_world()
rows = []
for actor in unreal.ActorIterator(world):
    class_name = actor.get_class().get_name()
    if class_name not in ("Landscape", "LandscapeSplineActor", "LandscapeStreamingProxy"):
        continue
    rows.append({
        "class": class_name,
        "name": actor.get_name(),
        "label": actor.get_actor_label(),
        "path": actor.get_path_name(),
        "package": actor.get_outermost().get_name(),
        "landscape_actor": prop_path(actor, "landscape_actor"),
        "landscape_guid": str(actor.get_editor_property("landscape_guid")) if "landscape_guid" in dir(actor) else None,
    })

print("CODEX_SAVED_CURRENT_PACKAGES=" + json.dumps(rows, ensure_ascii=False, default=str))

explicit = []
external_splines = [
    ("/Game/__ExternalActors__/Map/TinoKingdom_ByChanWoong/3/EG/1H8CUH8T3XMS28T6A61HGL", "LandscapeSplineActor_4"),
    ("/Game/__ExternalActors__/Map/TinoKingdom_ByChanWoong/6/9R/0BJ34ZXROLIN8HNCPNL5TP", "LandscapeSplineActor_5"),
    ("/Game/__ExternalActors__/Map/TinoKingdom_ByChanWoong/C/UE/3PQGINNZKP8UG1OTVBWSW7", "LandscapeSplineActor_6"),
    ("/Game/__ExternalActors__/Map/TinoKingdom_ByChanWoong/D/JD/G5UMX8IDAIWJONB3W32XUM", "LandscapeSplineActor_7"),
    ("/Game/__ExternalActors__/Map/TinoKingdom_ByChanWoong/2/V5/ZP8NBUCRBXJBJMRF8TSI0O", "LandscapeSplineActor_8"),
]
for package_path, actor_name in external_splines:
    package = unreal.load_package(package_path)
    actor_path = MAP_PATH + ".TinoKingdom_ByChanWoong:PersistentLevel." + actor_name
    actor = unreal.find_object(None, actor_path, unreal.LandscapeSplineActor)
    explicit.append({
        "package": package_path,
        "package_loaded": bool(package),
        "actor_path": actor_path,
        "actor_found": bool(actor),
        "landscape_actor": prop_path(actor, "landscape_actor") if actor else None,
    })
print("CODEX_EXPLICIT_SPLINES=" + json.dumps(explicit, ensure_ascii=False, default=str))
