import json
import unreal

base = "/Game/Fab/Magic_crystal/magic_crystal/StaticMeshes/magic_crystal1_magic_crystal1/"
for name in ("GC_magic_crystal", "GC_magic_crystal1"):
    asset = unreal.load_asset(base + name)
    if not asset:
        unreal.log_error("FRACTURE_CHECK missing " + name)
        continue
    component = unreal.GeometryCollectionComponent()
    component.set_rest_collection(asset)
    result = {"asset": name, "transform_count": len(component.get_initial_local_rest_transforms())}
    for prop in ("object_type", "enable_clustering", "damage_threshold", "cluster_group_index"):
        try:
            result[prop] = str(component.get_editor_property(prop))
        except Exception as exc:
            result[prop] = str(exc)
    result["debug"] = component.get_debug_info()
    unreal.log("FRACTURE_CHECK " + json.dumps(result))
