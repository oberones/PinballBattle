"""Update existing cabinet release settings without recreating maps or unrelated actors."""
import os
import sys
import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cabinet_return import TABLE_BLUEPRINT, TABLE_MAPS, configure_return, validate_return

assets = unreal.EditorAssetLibrary
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
blueprint = assets.load_asset(TABLE_BLUEPRINT)
assert blueprint
configure_return(unreal.get_default_object(blueprint.generated_class()))
unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
assert assets.save_loaded_asset(blueprint, only_if_is_dirty=False)
for path in TABLE_MAPS:
    assert levels.load_level(path), path
    table = next(a for a in actors.get_all_level_actors() if isinstance(a, unreal.PinballTable))
    configure_return(table)
    validate_return(table)
    assert levels.save_current_level(), path
unreal.log("PINBALL_RETURN_ASSETS_CONFIGURED")
