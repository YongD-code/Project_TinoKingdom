import unreal

bp = unreal.load_asset('/Game/Fab/Magic_crystal/magic_crystal/StaticMeshes/CrystalNPC')
sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
lib = unreal.SubobjectDataBlueprintFunctionLibrary
for handle in sub.k2_gather_subobject_data_for_blueprint(bp):
    obj = lib.get_object_for_blueprint(lib.get_data(handle), bp)
    if isinstance(obj, unreal.StaticMeshComponent):
        unreal.log('SCALE_CHECK mesh=' + str(obj.get_editor_property('static_mesh')) + ' scale=' + str(obj.get_editor_property('relative_scale3d')))
    if isinstance(obj, unreal.MagicStoneDestructionComponent):
        unreal.log('SCALE_CHECK mappings=' + str(obj.get_editor_property('fracture_meshes')))

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for name in ('magic_crystal', 'magic_crystal1'):
    sm = unreal.load_asset('/Game/Fab/Magic_crystal/magic_crystal/StaticMeshes/' + name)
    gc = unreal.load_asset('/Game/Fab/Magic_crystal/magic_crystal/StaticMeshes/magic_crystal1_magic_crystal1/GC_' + name)
    unreal.log('SCALE_CHECK source=' + name + ' bounds=' + str(sm.get_bounds()))
    actor = actors.spawn_actor_from_class(unreal.GeometryCollectionActor, unreal.Vector(0, 0, 0), transient=True)
    comp = actor.get_component_by_class(unreal.GeometryCollectionComponent)
    comp.set_rest_collection(gc)
    unreal.log('SCALE_CHECK gc=' + name + ' transforms=' + str(len(comp.get_initial_local_rest_transforms())) + ' bounds=' + str(actor.get_actor_bounds(False)) + ' root=' + str(comp.get_root_initial_transform()))
    unreal.log('SCALE_CHECK debug=' + comp.get_debug_info())
    actors.destroy_actor(actor)
