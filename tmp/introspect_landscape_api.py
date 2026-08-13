import json
import unreal


def names_containing(obj, terms):
    names = []
    for name in dir(obj):
        lowered = name.lower()
        if any(term in lowered for term in terms):
            names.append(name)
    return sorted(names)


classes = {}
for class_name in (
    "LandscapeInfo",
    "LandscapeSplineActor",
    "LandscapeSplinesComponent",
    "EditorLevelLibrary",
    "EditorActorSubsystem",
    "EditorLoadingAndSavingUtils",
):
    value = getattr(unreal, class_name, None)
    classes[class_name] = {
        "exists": value is not None,
        "matching_names": names_containing(value, ("spline", "landscape", "undo", "transaction", "save")) if value else [],
    }

print("CODEX_LANDSCAPE_API=" + json.dumps(classes, ensure_ascii=False))
