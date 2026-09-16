"""Author the independent Phase 5 arena, transition cabinet and passive framework UI."""
import unreal

assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def native(name):
    """Resolve a compiled shared or fixture class by its script class path."""
    result = unreal.load_class(None, "/Script/PinballBattle." + name)
    assert result, name
    return result


def save(asset):
    """Persist a real package and fail authoring immediately if the save fails."""
    assert assets.save_loaded_asset(asset, only_if_is_dirty=False)


def make(path, cls, factory):
    """Reuse existing packages on reruns instead of replacing unrelated content."""
    if assets.does_asset_exist(path):
        return assets.load_asset(path)
    folder, name = path.rsplit("/", 1)
    result = tools.create_asset(name, folder, cls, factory)
    assert result, path
    return result


def blueprint(path, parent):
    """Create and compile an explicitly selected native-backed Blueprint."""
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", native(parent))
    asset = make(path, unreal.Blueprint, factory)
    unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    save(asset)
    return asset


def data(path, name):
    """Create a typed definition asset through Unreal's DataAsset factory."""
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", native(name))
    return make(path, native(name), factory)


def widget(name, screen, title):
    """Author passive presentation variants with no Blueprint timers or score mutation."""
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", native("PinballPresentationWidget"))
    asset = make("/Game/Framework/UI/" + name, unreal.WidgetBlueprint, factory)
    defaults = unreal.get_default_object(asset.generated_class())
    defaults.set_editor_property("screen", screen)
    defaults.set_editor_property("heading", title)
    unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    save(asset)
    return asset.generated_class()


instructions = widget("WBP_Instructions", unreal.PinballScreen.INSTRUCTIONS, "ARCADE ROUND")
hud = widget("WBP_MinigameHUD", unreal.PinballScreen.MINI_GAME_HUD, "SPACE ACTION")
results = widget("WBP_Results", unreal.PinballScreen.RESULTS, "RESULTS")
recovery = widget("WBP_Recovery", unreal.PinballScreen.RECOVERY, "TABLE SECURED")
runtime = blueprint("/Game/Tests/Blueprints/BP_MG_TestRuntime", "MiniGameTestRuntime")
pawn = blueprint("/Game/Tests/Blueprints/BP_MG_TestPawn", "MiniGameTestPawn")
objective_class = blueprint("/Game/Framework/Pinball/BP_MinigameObjective", "MinigameObjective")
mapping_factory = unreal.DataAssetFactory()
mapping_factory.set_editor_property("data_asset_class", unreal.InputMappingContext)
mapping = make("/Game/Tests/Input/IMC_MG_Test", unreal.InputMappingContext, mapping_factory)
action_factory = unreal.DataAssetFactory()
action_factory.set_editor_property("data_asset_class", unreal.InputAction)
action = make("/Game/Tests/Input/IA_MG_TestAction", unreal.InputAction, action_factory)
action.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
mapping.unmap_all()
key = unreal.Key()
key.set_editor_property("key_name", "SpaceBar")
mapping.map_key(action, key)
save(action)
save(mapping)

arena_path = "/Game/Tests/Maps/L_MG_Test"
if not assets.does_asset_exist(arena_path):
    assert levels.new_level(arena_path)
else:
    assert levels.load_level(arena_path)
if not any(isinstance(a, unreal.MiniGameRuntimeBase) for a in actors.get_all_level_actors()):
    actors.spawn_actor_from_class(runtime.generated_class(), unreal.Vector(), unreal.Rotator())
assert levels.save_current_level()
definition = data("/Game/Tests/Data/DA_MG_Test", "MiniGameDefinition")
for key, value in {
    "mini_game_id": "FrameworkStub", "display_name": "Space Action", "instructions": "Press SPACE to light the beacon. Each fresh press earns local progress.",
    "map": assets.load_asset(arena_path), "runtime_class": runtime.generated_class(), "pawn_class": pawn.generated_class(),
    "input_context": mapping, "action_input": action, "hud_class": hud, "arena_transform": unreal.Transform(location=unreal.Vector(10000, 0, 0)),
    "arena_extent": unreal.Vector(500, 500, 1500), "metric_bounds": {"Actions": 100.0}, "scoring_profile_key": "FrameworkStub",
}.items():
    definition.set_editor_property(key, value)
save(definition)

profile_path = "/Game/Tests/Data/DA_TransitionScoring"
if not assets.does_asset_exist(profile_path):
    assert assets.duplicate_asset("/Game/Cabinets/AlienInvasion/Data/DA_AlienScoring", profile_path)
profile = assets.load_asset(profile_path)
rule = unreal.MiniGameScoreRule()
rule.set_editor_property("profile_key", "FrameworkStub")
rule.set_editor_property("weights", {"Actions": 500.0})
profile.set_editor_property("mini_game_rules", [rule])
profile.set_editor_property("revision", "Phase5-1")
save(profile)
cabinet_path = "/Game/Tests/Data/DA_TransitionTestCabinet"
if not assets.does_asset_exist(cabinet_path):
    assert assets.duplicate_asset("/Game/Cabinets/AlienInvasion/Data/DA_AlienCabinet", cabinet_path)
cabinet = assets.load_asset(cabinet_path)
for key, value in {"cabinet_id": "FrameworkTransitionTest", "mini_games": [definition], "scoring_profile": profile,
                   "instructions_widget": instructions, "mini_game_hud_widget": hud, "results_widget": results,
                   "recovery_widget": recovery, "development_without_minigames": True}.items():
    cabinet.set_editor_property(key, value)
mode = blueprint("/Game/Tests/Blueprints/BP_TransitionGameMode", "PinballGameModeBase")
defaults = unreal.get_default_object(mode.generated_class())
defaults.set_editor_property("cabinet", cabinet)
defaults.set_editor_property("player_controller_class", assets.load_blueprint_class("/Game/Framework/Blueprints/BP_PinballPlayerController"))
defaults.set_editor_property("default_pawn_class", cabinet.get_editor_property("control_pawn_class"))
unreal.BlueprintEditorLibrary.compile_blueprint(mode)
save(mode)
test_path = "/Game/Tests/Maps/L_TransitionTest"
if not assets.does_asset_exist(test_path):
    assert assets.duplicate_asset("/Game/Cabinets/AlienInvasion/Maps/L_AlienCabinet", test_path)
assert levels.load_level(test_path)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", mode.generated_class())
table = next(a for a in actors.get_all_level_actors() if isinstance(a, unreal.PinballTable))
objectives = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.MinigameObjective)]
objective = objectives[0] if objectives else actors.spawn_actor_from_class(objective_class.generated_class(), unreal.Vector(), unreal.Rotator())
objective.set_actor_location(table.get_actor_transform().transform_location(unreal.Vector(0, 650, 20)), False, False)
objective.set_actor_rotation(table.get_actor_rotation(), False)
trigger = objective.get_editor_property("trigger")
trigger.set_editor_property("table", table)
trigger.set_editor_property("definition", definition)
trigger.set_editor_property("objective_id", "StubObjective")
probes = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.PinballTransitionFunctionalTest)]
probe = probes[0] if probes else actors.spawn_actor_from_class(native("PinballTransitionFunctionalTest"), unreal.Vector(), unreal.Rotator())
probe.set_editor_property("objective", objective)
assert levels.save_current_level()
cabinet.set_editor_property("persistent_map", assets.load_asset(test_path))
save(cabinet)
unreal.log("PINBALL_PHASE5_ASSETS_CREATED")
