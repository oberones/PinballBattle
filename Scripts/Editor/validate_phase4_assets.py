"""Read saved Phase 4 packages in a fresh editor process and assert their runtime wiring."""
import unreal

assets = unreal.EditorAssetLibrary
root = "/Game/Cabinets/AlienInvasion/"


def load(path):
    """Require a real saved package, not just an in-memory object from the authoring process."""
    result = assets.load_asset(path)
    assert result, path
    return result


cabinet = load(root + "Data/DA_AlienCabinet")
assert cabinet.get_editor_property("initial_balls") == 3
assert cabinet.get_editor_property("return_trigger_protection_seconds") == 1
assert cabinet.get_editor_property("results_presentation_seconds") == 3
assert cabinet.get_editor_property("development_without_minigames")
profile = load(root + "Data/DA_AlienScoring")
assert cabinet.get_editor_property("scoring_profile") == profile
rules = profile.get_editor_property("categories")
assert len(rules) == 3
assert {r.get_editor_property("category"): r.get_editor_property("base_points") for r in rules} == {
    unreal.ScoringCategory.TARGET: 100, unreal.ScoringCategory.BUMPER: 50, unreal.ScoringCategory.LANE: 500}
assert profile.get_editor_property("minimum_multiplier") == 1
assert profile.get_editor_property("maximum_multiplier") == 10
for name, prop, screen in (
    ("WBP_Start", "start_widget", unreal.PinballScreen.START),
    ("WBP_PinballHUD", "hud_widget", unreal.PinballScreen.HUD),
    ("WBP_Pause", "pause_widget", unreal.PinballScreen.PAUSE),
    ("WBP_GameOver", "game_over_widget", unreal.PinballScreen.GAME_OVER),
):
    bp = load("/Game/Framework/UI/" + name)
    assert cabinet.get_editor_property(prop) == bp.generated_class()
    assert unreal.get_default_object(bp.generated_class()).get_editor_property("screen") == screen
pause = load("/Game/Framework/Input/IA_Pause")
assert pause.get_editor_property("trigger_when_paused")
controller = load("/Game/Framework/Blueprints/BP_PinballPlayerController")
assert unreal.get_default_object(controller.generated_class()).get_editor_property("pause_action") == pause
mode = load(root + "Blueprints/BP_AlienGameMode")
defaults = unreal.get_default_object(mode.generated_class())
assert defaults.get_editor_property("cabinet") == cabinet
assert not defaults.get_editor_property("practice_mode")
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level(root + "Maps/L_AlienCabinet")
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_world_settings().get_editor_property("default_game_mode") == mode.generated_class()
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
tables = [a for a in actors if isinstance(a, unreal.PinballTable)]
assert len(tables) == 1
assert tables[0].get_editor_property("controls_widget_class") is None
assert tables[0].get_editor_property("tuning") == cabinet.get_editor_property("tuning")
assert tables[0].get_editor_property("table_camera") is not None
assert len([a for a in actors if isinstance(a, unreal.BasicSessionProbe)]) == 1
assert assets.does_asset_exist("/Game/Tests/Maps/L_TableInteractions")
unreal.log("PINBALL_PHASE4_ASSET_VALIDATION_PASSED")
