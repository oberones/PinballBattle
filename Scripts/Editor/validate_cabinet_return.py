"""Verify release settings in saved packages from a fresh Unreal process."""
import os
import sys
import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cabinet_return import TABLE_BLUEPRINT, TABLE_MAPS, validate_return

blueprint = unreal.EditorAssetLibrary.load_asset(TABLE_BLUEPRINT)
validate_return(unreal.get_default_object(blueprint.generated_class()))
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for path in TABLE_MAPS:
    assert levels.load_level(path), path
    table = next(a for a in actors.get_all_level_actors() if isinstance(a, unreal.PinballTable))
    validate_return(table)
unreal.log("PINBALL_RETURN_ASSETS_VALIDATED")
