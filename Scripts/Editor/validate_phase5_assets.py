"""Read saved Phase 5 packages in a fresh editor process and verify explicit content wiring."""
import unreal

assets = unreal.EditorAssetLibrary
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def load(path):
    """Require a real saved Unreal asset, not a path-only placeholder."""
    result = assets.load_asset(path)
    assert result, path
    return result


definition = load("/Game/Tests/Data/DA_MG_Test")
assert definition.get_editor_property("duration_seconds") == 30
assert definition.get_editor_property("local_lives") == 3
assert definition.get_editor_property("minimum_result_multiplier") == 1
assert definition.get_editor_property("maximum_result_multiplier") == 1
for name in ("map", "runtime_class", "pawn_class", "input_context", "action_input", "hud_class"):
    assert definition.get_editor_property(name), name
assert definition.get_editor_property("metric_bounds")["Actions"] == 100
cabinet = load("/Game/Tests/Data/DA_TransitionTestCabinet")
assert cabinet.get_editor_property("development_without_minigames")
assert list(cabinet.get_editor_property("mini_games")) == [definition]
assert cabinet.get_editor_property("return_trigger_protection_seconds") == 1
assert cabinet.get_editor_property("results_presentation_seconds") == 3
for name in ("instructions_widget", "mini_game_hud_widget", "results_widget", "recovery_widget"):
    assert cabinet.get_editor_property(name), name
assert levels.load_level("/Game/Tests/Maps/L_MG_Test")
roots = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.MiniGameRuntimeBase)]
assert len(roots) == 1 and roots[0].get_editor_property("camera")
assert levels.load_level("/Game/Tests/Maps/L_TransitionTest")
tables = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.PinballTable)]
objectives = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.MinigameObjective)]
fixtures = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.PinballTransitionFunctionalTest)]
assert len(tables) == len(objectives) == len(fixtures) == 1
trigger = objectives[0].get_editor_property("trigger")
assert trigger.get_editor_property("table") == tables[0]
assert trigger.get_editor_property("definition") == definition
assert fixtures[0].get_editor_property("objective") == objectives[0]
assert tables[0].get_editor_property("primary_return") != tables[0].get_editor_property("backup_return")
unreal.log("PINBALL_PHASE5_ASSET_VALIDATION_PASSED")
