import json
import os

import unreal


EXPECTED_WORLD = "/Game/Map/TinoKingdom_ByChanWoong.TinoKingdom_ByChanWoong"
PACKAGE_SUFFIXES = [
    "0/30/16RTZLAZ2RTH66IUVCOM5B",
    "0/B7/O0URFU99NIY7W52EZZB6LC",
    "1/58/9EL0YK0FH8WTA1ZXFOP1L6",
    "1/IJ/8IU9H2ECYLVDF1XPMX3IU6",
    "1/NX/WT5QR7LJ5YLTEXCH24UEUQ",
    "1/O4/339MPD9YEKXAA0GY2G23XV",
    "2/VF/87WX2G130H1KJVHSXU6DTT",
    "3/6R/DZDJWEKB0UEFC5JKH9GO9Z",
    "4/CU/558U6PVTP50FDJ7PSE2GMC",
    "5/17/HAGN2BK7Z1G1G51J753L63",
    "5/3I/VFTILZ66OAKOUCR6Y5IRNN",
    "5/QC/TZVYBP6WKD8ZF66YRW38OR",
    "7/2N/YHFTTUEDBZZ3CJ990H0VNN",
    "7/IL/E6CJLORULJKMMJOS9FA1OT",
    "8/I3/SK8ZKIS4EZMZGW7LI2XN96",
    "A/81/FII4JEZC5LAZC4YQQ5R72E",
    "A/ED/I6C95VKO1TQARAGAB8JUAU",
    "A/GN/UY4DJP6H9GL4TEDMZ4KWFW",
    "A/MU/XLB2WFEB5KG57BD4NSJVT9",
    "A/UX/8NDTS1ERE10LXJRNI5BLIQ",
    "B/0G/20TLPH95GJUIOZVVKGN4F8",
    "C/F3/MPW00OX8W9LZYPOYQQ5VH3",
    "D/2J/JQ2KZM45YZ5PHLGWELKW6K",
    "D/ES/7U0BZC9LJIAHRTH8SRESK0",
    "E/4U/Y9X3XQ6EJBCGJM82SZKD1F",
    "E/EN/54W4AFR0MEFGMMG4PNMNOG",
]
PACKAGE_ROOT = "/Game/__ExternalActors__/Map/TinoKingdom_ByChanWoong/"
TARGET_PACKAGE_NAMES = {PACKAGE_ROOT + suffix for suffix in PACKAGE_SUFFIXES}


editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = editor.get_editor_world()
if not world or world.get_path_name() != EXPECTED_WORLD:
    raise RuntimeError("Unexpected editor world: " + (world.get_path_name() if world else "None"))

actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = actor_subsystem.get_all_level_actors()
ecr_actors = [actor.get_path_name() for actor in actors if "ECR7GTL4QF2KPLA9FWY56F1C8" in actor.get_name()]
root_actor = [actor.get_path_name() for actor in actors if actor.get_name() == "Landscape_1"]
if ecr_actors or root_actor:
    raise RuntimeError("Landscape2 actors are still live: " + json.dumps(ecr_actors + root_actor))

loaded_packages = {
    name: unreal.find_package(name)
    for name in TARGET_PACKAGE_NAMES
}
loaded_packages = {name: package for name, package in loaded_packages.items() if package}
missing_loaded = sorted(TARGET_PACKAGE_NAMES - set(loaded_packages))
if missing_loaded:
    raise RuntimeError("Target packages are not loaded: " + ", ".join(missing_loaded))

packages = [loaded_packages[name] for name in sorted(TARGET_PACKAGE_NAMES)]
saved = unreal.EditorLoadingAndSavingUtils.save_packages(packages, False)
if not saved:
    raise RuntimeError("Failed to finalize one or more Landscape2 package deletions")

unreal.SystemLibrary.collect_garbage()
content_dir = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir())
remaining_files = []
for suffix in PACKAGE_SUFFIXES:
    filename = os.path.join(content_dir, "__ExternalActors__", "Map", "TinoKingdom_ByChanWoong", *suffix.split("/")) + ".uasset"
    if os.path.isfile(filename):
        remaining_files.append(filename)

print("CODEX_LANDSCAPE2_DELETE_SAVE=" + json.dumps({
    "target_count": len(packages),
    "remaining_files": remaining_files,
}, ensure_ascii=False))
