import json
import unreal

asset_path = "/Game/Fab/Magic_crystal/magic_crystal/StaticMeshes/CrystalNPC"
blueprint = unreal.load_asset(asset_path)
if not blueprint:
    raise RuntimeError("CrystalNPC could not be loaded")

subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
library = unreal.SubobjectDataBlueprintFunctionLibrary
components = []
for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
    data = library.get_data(handle)
    obj = library.get_object_for_blueprint(data, blueprint)
    if not obj:
        continue
    entry = {"name": obj.get_name(), "class": obj.get_class().get_name()}
    if isinstance(obj, unreal.StaticMeshComponent):
        mesh = obj.get_editor_property("static_mesh")
        entry["mesh"] = mesh.get_path_name() if mesh else None
    components.append(entry)

report = {"asset": asset_path, "components": components}
unreal.log("MAGIC_STONE_INSPECTION " + json.dumps(report))
