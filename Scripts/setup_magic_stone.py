"""Add the destruction component to the existing crystal NPC without changing its dialogue."""
from pathlib import Path
import shutil
import unreal

asset_path = "/Game/Fab/Magic_crystal/magic_crystal/StaticMeshes/CrystalNPC"
blueprint = unreal.load_asset(asset_path)
if not blueprint:
    raise RuntimeError("CrystalNPC could not be loaded")

subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
library = unreal.SubobjectDataBlueprintFunctionLibrary
handles = subsystem.k2_gather_subobject_data_for_blueprint(blueprint)
existing = []
for handle in handles:
    obj = library.get_object_for_blueprint(library.get_data(handle), blueprint)
    if isinstance(obj, unreal.MagicStoneDestructionComponent):
        existing.append(obj)
if len(existing) > 1:
    raise RuntimeError("Multiple destruction components found; review before continuing")

if not existing:
    project = Path(unreal.Paths.project_dir()).resolve()
    source = project / "Content/Fab/Magic_crystal/magic_crystal/StaticMeshes/CrystalNPC.uasset"
    backup = project / "Saved/MagicStoneBackup/CrystalNPC.uasset"
    backup.parent.mkdir(parents=True, exist_ok=True)
    if not backup.exists():
        shutil.copy2(source, backup)
    params = unreal.AddNewSubobjectParams(
        parent_handle=handles[0],
        new_class=unreal.MagicStoneDestructionComponent,
        blueprint_context=blueprint,
    )
    handle, reason = subsystem.add_new_subobject(params)
    obj = library.get_object_for_blueprint(library.get_data(handle), blueprint)
    if not isinstance(obj, unreal.MagicStoneDestructionComponent):
        raise RuntimeError("Failed to add destruction component: " + str(reason))
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint):
        raise RuntimeError("Failed to save CrystalNPC")

verified = []
for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
    obj = library.get_object_for_blueprint(library.get_data(handle), blueprint)
    if isinstance(obj, unreal.MagicStoneDestructionComponent):
        verified.append(obj)
if len(verified) != 1:
    raise RuntimeError("Expected exactly one destruction component")
component = verified[0]
for entry in component.get_editor_property("fracture_meshes"):
    for field in ("intact_mesh", "fractured_collection"):
        if not entry.get_editor_property(field):
            raise RuntimeError("Missing fracture asset: " + field)
unreal.log("MAGIC_STONE_SETUP_OK hits=" + str(component.get_editor_property("hits_to_break")))
