"""Author the independent Asteroid Field and configure the existing shipping transition path."""
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


def material(name, color):
    """Create original unlit emissive colors readable without arena-wide lighting."""
    result = make(base + "Art/" + name, unreal.Material, unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(result)
    result.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    expression = unreal.MaterialEditingLibrary.create_material_expression(result, unreal.MaterialExpressionConstant3Vector)
    expression.set_editor_property("constant", unreal.LinearColor(*color, 1))
    unreal.MaterialEditingLibrary.connect_material_property(expression, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(result)
    save(result)
    return result


def action(name, key_name):
    """Create a Boolean Enhanced Input action and a single explicit default key mapping."""
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.InputAction)
    result = make(base + "Input/IA_Asteroid" + name, unreal.InputAction, factory)
    result.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
    key = unreal.Key()
    key.set_editor_property("key_name", key_name)
    mapping.map_key(result, key)
    save(result)
    return result


base = "/Game/Minigames/AsteroidField/"
hud_factory = unreal.WidgetBlueprintFactory()
hud_factory.set_editor_property("parent_class", native("PinballPresentationWidget"))
hud = make(base + "UI/WBP_AsteroidHUD", unreal.WidgetBlueprint, hud_factory)
hud_defaults = unreal.get_default_object(hud.generated_class())
hud_defaults.set_editor_property("screen", unreal.PinballScreen.MINI_GAME_HUD)
hud_defaults.set_editor_property("heading", "ASTEROID FIELD")
unreal.BlueprintEditorLibrary.compile_blueprint(hud)
save(hud)
runtime = blueprint(base + "Blueprints/BP_AsteroidFieldRuntime", "AsteroidFieldRuntime")
pawn = blueprint(base + "Blueprints/BP_AsteroidShip", "AsteroidShipPawn")
rock = blueprint(base + "Blueprints/BP_AsteroidObstacle", "AsteroidObstacle")
shot = blueprint(base + "Blueprints/BP_AsteroidProjectile", "AsteroidProjectile")
factory = unreal.DataAssetFactory()
factory.set_editor_property("data_asset_class", unreal.InputMappingContext)
mapping = make(base + "Input/IMC_AsteroidField", unreal.InputMappingContext, factory)
mapping.unmap_all()
inputs = {"left_action": action("Left", "Left"), "right_action": action("Right", "Right"),
          "thrust_action": action("Thrust", "Up"), "fire_action": action("Fire", "SpaceBar")}
save(mapping)
ship_defaults = unreal.get_default_object(pawn.generated_class())
for prop, value in inputs.items():
    ship_defaults.set_editor_property(prop, value)
ship_defaults.get_editor_property("visual").set_material(0, material("M_Ship", (.15, 1.0, 2.0)))
# Python Rotator positional arguments differ from C++ FRotator; pitch aligns the cone tip with firing +X.
ship_defaults.get_editor_property("visual").set_relative_rotation(unreal.Rotator(pitch=-90, yaw=0, roll=0), False, False)
unreal.get_default_object(rock.generated_class()).get_editor_property("visual").set_material(0, material("M_Rock", (1.0, .35, .08)))
unreal.get_default_object(shot.generated_class()).get_component_by_class(unreal.StaticMeshComponent).set_material(0, material("M_Shot", (.7, 2.0, 1.0)))
root_defaults = unreal.get_default_object(runtime.generated_class())
root_defaults.set_editor_property("obstacle_class", rock.generated_class())
root_defaults.set_editor_property("projectile_class", shot.generated_class())

# Synthesize a short original chirp; no downloaded or sibling-game media is required.
import math
import os
import struct
import wave
audio_dir = os.path.abspath(os.path.join(unreal.Paths.project_saved_dir(), "Phase6Audio"))
os.makedirs(audio_dir, exist_ok=True)
audio_file = os.path.join(audio_dir, "AsteroidBurst.wav")
with wave.open(audio_file, "wb") as stream:
    stream.setparams((1, 2, 22050, 0, "NONE", "not compressed"))
    stream.writeframes(b"".join(struct.pack("<h", int(16000 * (1 - i / 2646) ** 2 * math.sin(2 * math.pi * (850 * i / 22050 - 1900 * (i / 22050) ** 2)))) for i in range(2646)))
task = unreal.AssetImportTask()
task.set_editor_property("filename", audio_file)
task.set_editor_property("destination_path", base + "Audio")
task.set_editor_property("automated", True)
task.set_editor_property("replace_existing", True)
task.set_editor_property("save", True)
tools.import_asset_tasks([task])
root_defaults.set_editor_property("destruction_sound", assets.load_asset(base + "Audio/AsteroidBurst"))
rails = material("M_Boundary", (.04, .25, .5))
for mesh in root_defaults.get_components_by_class(unreal.StaticMeshComponent):
    mesh.set_material(0, rails)
for asset in (pawn, rock, shot, runtime):
    unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    save(asset)

arena_path = base + "Maps/L_MG_AsteroidField"
assert levels.load_level(arena_path) if assets.does_asset_exist(arena_path) else levels.new_level(arena_path)
if not any(isinstance(a, unreal.AsteroidFieldRuntime) for a in actors.get_all_level_actors()):
    actors.spawn_actor_from_class(runtime.generated_class(), unreal.Vector(), unreal.Rotator())
assert levels.save_current_level()
definition = data(base + "Data/DA_MG_AsteroidField", "MiniGameDefinition")
for prop, value in {
    "mini_game_id": "AsteroidField", "display_name": "Asteroid Field",
    "instructions": "LEFT/RIGHT rotate. UP thrust. Hold SPACE to fire. Destroy rocks; rebound from the arena edges. Three lives, two-second shield after respawn. Survive 30 seconds. Two rocks earn 1,000 bonus; twenty earn 10,000.",
    "map": assets.load_asset(arena_path), "runtime_class": runtime.generated_class(), "pawn_class": pawn.generated_class(),
    "input_context": mapping, "action_input": inputs["fire_action"],
    "hud_class": hud.generated_class(),
    "arena_transform": unreal.Transform(location=unreal.Vector(10000, 0, 0)), "arena_extent": unreal.Vector(800, 430, 1500),
    "duration_seconds": 30.0, "local_lives": 3, "metric_bounds": {"ObjectsDestroyed": 100.0},
    "maximum_objectives": 100, "scoring_profile_key": "AsteroidField",
}.items():
    definition.set_editor_property(prop, value)
save(definition)
profile = assets.load_asset("/Game/Cabinets/AlienInvasion/Data/DA_AlienScoring")
rules = [r for r in profile.get_editor_property("mini_game_rules") if str(r.get_editor_property("profile_key")) != "AsteroidField"]
rule = unreal.MiniGameScoreRule()
rule.set_editor_property("profile_key", "AsteroidField")
rule.set_editor_property("weights", {"ObjectsDestroyed": 500.0})
rules.append(rule)
profile.set_editor_property("mini_game_rules", rules)
profile.set_editor_property("revision", "Phase6-1")
save(profile)
cabinet = assets.load_asset("/Game/Cabinets/AlienInvasion/Data/DA_AlienCabinet")
cabinet.set_editor_property("mini_games", [definition])
cabinet.set_editor_property("development_without_minigames", True)
for prop, name in {"instructions_widget": "WBP_Instructions", "mini_game_hud_widget": "WBP_MinigameHUD",
                   "results_widget": "WBP_Results", "recovery_widget": "WBP_Recovery"}.items():
    cabinet.set_editor_property(prop, assets.load_blueprint_class("/Game/Framework/UI/" + name))
cabinet.set_editor_property("mini_game_hud_widget", hud.generated_class())
save(cabinet)
test_path = "/Game/Tests/Data/DA_AsteroidTestCabinet"
if not assets.does_asset_exist(test_path):
    assert assets.duplicate_asset(cabinet.get_path_name(), test_path)
test_cabinet = assets.load_asset(test_path)
test_cabinet.set_editor_property("mini_games", [definition])
test_cabinet.set_editor_property("mini_game_hud_widget", hud.generated_class())
test_cabinet.set_editor_property("cabinet_id", "AsteroidTest")
save(test_cabinet)
mode = blueprint("/Game/Tests/Blueprints/BP_AsteroidGameMode", "PinballGameModeBase")
defaults = unreal.get_default_object(mode.generated_class())
defaults.set_editor_property("cabinet", test_cabinet)
defaults.set_editor_property("player_controller_class", assets.load_blueprint_class("/Game/Framework/Blueprints/BP_PinballPlayerController"))
defaults.set_editor_property("default_pawn_class", cabinet.get_editor_property("control_pawn_class"))
unreal.BlueprintEditorLibrary.compile_blueprint(mode)
save(mode)
test_map = "/Game/Tests/Maps/L_AsteroidTest"
if not assets.does_asset_exist(test_map):
    assert assets.duplicate_asset("/Game/Tests/Maps/L_TransitionTest", test_map)
for map_path in ("/Game/Cabinets/AlienInvasion/Maps/L_AlienCabinet", test_map):
    assert levels.load_level(map_path)
    table = next(a for a in actors.get_all_level_actors() if isinstance(a, unreal.PinballTable))
    objectives = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.MinigameObjective)]
    objective = objectives[0] if objectives else actors.spawn_actor_from_class(assets.load_blueprint_class("/Game/Framework/Pinball/BP_MinigameObjective"), unreal.Vector(), unreal.Rotator())
    objective.set_actor_location(table.get_actor_transform().transform_location(unreal.Vector(0, 650, 20)), False, False)
    trigger = objective.get_editor_property("trigger")
    trigger.set_editor_property("table", table)
    trigger.set_editor_property("definition", definition)
    trigger.set_editor_property("objective_id", "AsteroidObjective")
    objective.set_actor_label("Asteroid Field Objective")
    if map_path == test_map:
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        world.get_world_settings().set_editor_property("default_game_mode", mode.generated_class())
        for actor in actors.get_all_level_actors():
            if isinstance(actor, unreal.PinballTransitionFunctionalTest):
                actors.destroy_actor(actor)
        probes = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.AsteroidFieldProbe)]
        probe = probes[0] if probes else actors.spawn_actor_from_class(native("AsteroidFieldProbe"), unreal.Vector(), unreal.Rotator())
        probe.set_editor_property("objective", objective)
    assert levels.save_current_level()
test_cabinet.set_editor_property("persistent_map", assets.load_asset(test_map))
save(test_cabinet)
unreal.log("PINBALL_PHASE6_ASSETS_CREATED")
