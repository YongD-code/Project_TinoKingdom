import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
print("CODEX_LIVE_PYTHON_OK=" + (world.get_path_name() if world else "None"))
print("CODEX_RELOAD_DOC=" + str(unreal.EditorLoadingAndSavingUtils.reload_packages.__doc__))
enum_type = getattr(unreal, "ReloadPackagesInteractionMode", None)
print("CODEX_RELOAD_ENUM=" + str([name for name in dir(enum_type) if name.isupper()]) if enum_type else "CODEX_RELOAD_ENUM=None")
for name in ("find_package", "load_package", "find_object", "get_objects_of_class"):
    value = getattr(unreal, name, None)
    print("CODEX_API_" + name.upper() + "=" + str(value.__doc__ if value else None))
print("CODEX_SAVE_PACKAGES_DOC=" + str(unreal.EditorLoadingAndSavingUtils.save_packages.__doc__))
