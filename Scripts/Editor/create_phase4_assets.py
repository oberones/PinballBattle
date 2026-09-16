"""Author Phase 4 data/UI and configure the existing cabinet without rebuilding its geometry."""
import unreal

assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def save(asset):
    """Persist an actual Unreal package and fail immediately if saving fails."""
    assert assets.save_loaded_asset(asset, only_if_is_dirty=False), asset.get_path_name()


def native(name):
    """Resolve a compiled project class rather than relying on Python wrapper generation."""
    result = unreal.load_class(None, "/Script/PinballBattle." + name)
    assert result, name
    return result


def make(path, cls, factory):
    """Reuse a package on reruns or create a real asset with the appropriate Unreal factory."""
    if assets.does_asset_exist(path):
        return assets.load_asset(path)
    folder, name = path.rsplit("/", 1)
    result = tools.create_asset(name, folder, cls, factory)
    assert result, path
    return result


def data(path, name):
    """Create a native-backed Data Asset with editable validated configuration."""
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", native(name))
    return make(path, native(name), factory)


def widget(name, screen, heading):
    """Compile a reusable presentation Blueprint with no gameplay authority."""
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", native("PinballPresentationWidget"))
    result = make("/Game/Framework/UI/" + name, unreal.WidgetBlueprint, factory)
    defaults = unreal.get_default_object(result.generated_class())
    defaults.set_editor_property("screen", screen)
    defaults.set_editor_property("heading", heading)
    unreal.BlueprintEditorLibrary.compile_blueprint(result)
    save(result)
    return result.generated_class()


start = widget("WBP_Start", unreal.PinballScreen.START, "PINBALL\nREADY TO PLAY")
hud = widget("WBP_PinballHUD", unreal.PinballScreen.HUD, "PINBALL")
pause = widget("WBP_Pause", unreal.PinballScreen.PAUSE, "PAUSED")
game_over = widget("WBP_GameOver", unreal.PinballScreen.GAME_OVER, "GAME OVER")
root = "/Game/Cabinets/AlienInvasion/"
profile = data(root + "Data/DA_AlienScoring", "ScoringProfile")
profile.set_editor_property("profile_id", "AlienTable")
profile.set_editor_property("revision", "Phase4-1")
rules = []
for category, points in ((unreal.ScoringCategory.TARGET, 100), (unreal.ScoringCategory.BUMPER, 50), (unreal.ScoringCategory.LANE, 500)):
    rule = unreal.TableScoreRule()
    rule.set_editor_property("category", category)
    rule.set_editor_property("base_points", points)
    rules.append(rule)
profile.set_editor_property("categories", rules)
save(profile)
cabinet = data(root + "Data/DA_AlienCabinet", "CabinetDefinition")
for prop, value in {
    "cabinet_id": "AlienInvasion", "display_title": "Alien Invasion",
    "persistent_map": assets.load_asset(root + "Maps/L_AlienCabinet"),
    "table_class": assets.load_blueprint_class(root + "Blueprints/BP_AlienTable"),
    "control_pawn_class": assets.load_blueprint_class("/Game/Framework/Pinball/BP_PinballControlPawn"),
    "scoring_profile": profile, "tuning": assets.load_asset(root + "Data/DA_AlienPhysics"),
    "initial_balls": 3, "return_trigger_protection_seconds": 1.0, "results_presentation_seconds": 3.0,
    "development_without_minigames": True,
    "start_widget": start, "hud_widget": hud, "pause_widget": pause, "game_over_widget": game_over,
}.items():
    cabinet.set_editor_property(prop, value)
save(cabinet)

controller = assets.load_asset("/Game/Framework/Blueprints/BP_PinballPlayerController")
unreal.get_default_object(controller.generated_class()).set_editor_property(
    "pause_action", assets.load_asset("/Game/Framework/Input/IA_Pause"))
unreal.BlueprintEditorLibrary.compile_blueprint(controller)
save(controller)

factory = unreal.BlueprintFactory()
factory.set_editor_property("parent_class", native("PinballGameModeBase"))
mode = make(root + "Blueprints/BP_AlienGameMode", unreal.Blueprint, factory)
defaults = unreal.get_default_object(mode.generated_class())
defaults.set_editor_property("cabinet", cabinet)
defaults.set_editor_property("practice_mode", False)
defaults.set_editor_property("player_controller_class", controller.generated_class())
defaults.set_editor_property("default_pawn_class", cabinet.get_editor_property("control_pawn_class"))
unreal.BlueprintEditorLibrary.compile_blueprint(mode)
save(mode)

# Preserve the previous repeat-ball cabinet as a test map before changing the production map.
practice_path = "/Game/Tests/Maps/L_TableInteractions"
if not assets.does_asset_exist(practice_path):
    assert assets.duplicate_asset(root + "Maps/L_AlienCabinet", practice_path)
assert levels.load_level(practice_path)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", assets.load_blueprint_class("/Game/Tests/Blueprints/BP_PracticeGameMode"))
assert levels.save_current_level()

assert levels.load_level(root + "Maps/L_AlienCabinet")
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", mode.generated_class())
tables = [actor for actor in actors.get_all_level_actors() if isinstance(actor, unreal.PinballTable)]
assert len(tables) == 1
tables[0].set_editor_property("controls_widget_class", None)
if not any(isinstance(actor, unreal.BasicSessionProbe) for actor in actors.get_all_level_actors()):
    actor = actors.spawn_actor_from_class(native("BasicSessionProbe"), unreal.Vector(), unreal.Rotator())
    actor.set_actor_label("Session Validation (command-line opt-in)")
assert levels.save_current_level()
unreal.log("PINBALL_PHASE4_ASSETS_CREATED")
