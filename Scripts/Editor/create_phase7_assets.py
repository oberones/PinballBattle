"""Author independent Planetary Defense assets and add its production cabinet association."""
import math
import os
import struct
import wave
import unreal

assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
base = "/Game/Minigames/PlanetaryDefense/"


def native(name):
    """Resolve a compiled class explicitly."""
    result = unreal.load_class(None, "/Script/PinballBattle." + name)
    assert result, name
    return result


def save(asset):
    """Persist an actual Unreal package and fail immediately on save errors."""
    assert assets.save_loaded_asset(asset, only_if_is_dirty=False)


def make(path, cls, factory):
    """Reuse an existing asset on rerun without replacing unrelated content."""
    if assets.does_asset_exist(path):
        return assets.load_asset(path)
    folder, name = path.rsplit("/", 1)
    result = tools.create_asset(name, folder, cls, factory)
    assert result, path
    return result


def blueprint(path, parent):
    """Create a native-backed Blueprint with a compiled generated class."""
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", native(parent))
    result = make(path, unreal.Blueprint, factory)
    unreal.BlueprintEditorLibrary.compile_blueprint(result)
    save(result)
    return result


def data(path, cls):
    """Create a typed data asset with the engine factory."""
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", cls)
    return make(path, cls, factory)


def material(name, color):
    """Author original unlit colors without global lighting or sibling media dependencies."""
    result = make(base + "Art/" + name, unreal.Material, unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(result)
    result.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    expression = unreal.MaterialEditingLibrary.create_material_expression(result, unreal.MaterialExpressionConstant3Vector)
    expression.set_editor_property("constant", unreal.LinearColor(*color, 1))
    unreal.MaterialEditingLibrary.connect_material_property(expression, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(result)
    save(result)
    return result


blueprints = {}
for name in ("PlanetaryDefenseRuntime", "DefenseAimPawn", "DefenseThreat", "DefenseColony", "DefenseInterceptor", "DefenseBlastZone"):
    blueprints[name] = blueprint(base + "Blueprints/BP_" + name, name)
defaults = {name: unreal.get_default_object(asset.generated_class()) for name, asset in blueprints.items()}
root = defaults["PlanetaryDefenseRuntime"]
camera = root.get_editor_property("camera")
camera.set_editor_property("ortho_width", 2400)
camera.set_editor_property("override_aspect_ratio_axis_constraint", True)
camera.set_editor_property("aspect_ratio_axis_constraint", unreal.AspectRatioAxisConstraint.ASPECT_RATIO_MAINTAIN_XFOV)
camera.set_editor_property("auto_calculate_ortho_planes", False)
camera.set_editor_property("update_ortho_planes", False)
camera.set_editor_property("ortho_near_clip_plane", -1500)
camera.set_editor_property("ortho_far_clip_plane", 3000)
camera.set_relative_location(unreal.Vector(350, 0, 1200), False, False)
camera.set_relative_rotation(unreal.Rotator(pitch=-90, yaw=90, roll=0), False, False)
for prop, name in {"threat_class": "DefenseThreat", "colony_class": "DefenseColony",
                   "interceptor_class": "DefenseInterceptor", "blast_class": "DefenseBlastZone"}.items():
    root.set_editor_property(prop, blueprints[name].generated_class())
colors = {"Threat": (2.0, .19, .045), "Colony": (.15, .8, .35), "Interceptor": (.35, 1.7, 2.0),
          "Blast": (.015, .13, .23), "Aim": (.75, 1.8, 1.9), "Boundary": (.04, .18, .32)}
materials = {name: material("M_Defense" + name, color) for name, color in colors.items()}
for name in ("Threat", "Colony", "Interceptor", "BlastZone"):
    defaults["Defense" + name].get_editor_property("visual").set_material(0, materials["Blast" if name == "BlastZone" else name])
for mesh in defaults["DefenseAimPawn"].get_editor_property("reticle"):
    mesh.set_material(0, materials["Aim"])
for mesh in root.get_components_by_class(unreal.StaticMeshComponent):
    mesh.set_material(0, materials["Boundary"])
root.get_editor_property("launcher").set_material(0, materials["Colony"])
root.get_editor_property("barrel").set_material(0, materials["Interceptor"])
defaults["DefenseThreat"].get_editor_property("visual").set_relative_rotation(unreal.Rotator(pitch=-90, yaw=0, roll=0), False, False)
mapping = data(base + "Input/IMC_PlanetaryDefense", unreal.InputMappingContext)
mapping.unmap_all()
fire = data(base + "Input/IA_DefenseFire", unreal.InputAction)
fire.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
key = unreal.Key()
key.set_editor_property("key_name", "SpaceBar")
mapping.map_key(fire, key)
defaults["DefenseAimPawn"].set_editor_property("fire_action", fire)
save(fire)
save(mapping)

# A short original two-tone electronic interception sound is synthesized locally.
audio_dir = os.path.abspath(os.path.join(unreal.Paths.project_saved_dir(), "Phase7Audio"))
os.makedirs(audio_dir, exist_ok=True)
audio_file = os.path.join(audio_dir, "DefenseIntercept.wav")
with wave.open(audio_file, "wb") as stream:
    stream.setparams((1, 2, 22050, 0, "NONE", "not compressed"))
    stream.writeframes(b"".join(struct.pack("<h", int(13000 * (1 - i / 3087) ** 2 *
        (math.sin(2 * math.pi * 960 * i / 22050) + .25 * math.sin(2 * math.pi * 1440 * i / 22050)))) for i in range(3087)))
task = unreal.AssetImportTask()
for prop, value in {"filename": audio_file, "destination_path": base + "Audio", "automated": True,
                    "replace_existing": True, "save": True}.items():
    task.set_editor_property(prop, value)
tools.import_asset_tasks([task])
root.set_editor_property("interception_sound", assets.load_asset(base + "Audio/DefenseIntercept"))
for asset in blueprints.values():
    unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    save(asset)

hud_factory = unreal.WidgetBlueprintFactory()
hud_factory.set_editor_property("parent_class", native("PinballPresentationWidget"))
hud = make(base + "UI/WBP_DefenseHUD", unreal.WidgetBlueprint, hud_factory)
hud_defaults = unreal.get_default_object(hud.generated_class())
hud_defaults.set_editor_property("screen", unreal.PinballScreen.MINI_GAME_HUD)
hud_defaults.set_editor_property("heading", "PLANETARY\nDEFENSE")
hud_defaults.set_editor_property("status_wrap_width", 260)
unreal.BlueprintEditorLibrary.compile_blueprint(hud)
save(hud)
arena_path = base + "Maps/L_MG_PlanetaryDefense"
assert levels.load_level(arena_path) if assets.does_asset_exist(arena_path) else levels.new_level(arena_path)
roots = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.PlanetaryDefenseRuntime)]
if not roots:
    actors.spawn_actor_from_class(blueprints["PlanetaryDefenseRuntime"].generated_class(), unreal.Vector(), unreal.Rotator())
assert levels.save_current_level()
definition = data(base + "Data/DA_MG_PlanetaryDefense", native("MiniGameDefinition"))
for prop, value in {
    "mini_game_id": "PlanetaryDefense", "display_name": "Planetary Defense",
    "instructions": "Move the MOUSE to aim. Hold SPACE to launch unlimited interceptors. Aim ahead of orange threats: each shot creates a temporary blue blast. Protect three green colonies, each lost to one impact. Keep intercepting even if all colonies fall. The round lasts 30 seconds. Four interceptions earn 1,000; 28 with all colonies earn 10,000.",
    "map": assets.load_asset(arena_path), "runtime_class": blueprints["PlanetaryDefenseRuntime"].generated_class(),
    "pawn_class": blueprints["DefenseAimPawn"].generated_class(), "input_context": mapping, "action_input": fire,
    "hud_class": hud.generated_class(), "uses_mouse_aim": True,
    "arena_transform": unreal.Transform(location=unreal.Vector(20000, 0, 0)), "arena_extent": unreal.Vector(800, 430, 1500),
    "duration_seconds": 30.0, "local_lives": 0, "metric_bounds": {"ThreatsDestroyed": 100.0, "StructuresSurviving": 3.0},
    "maximum_objectives": 100, "scoring_profile_key": "PlanetaryDefense",
}.items():
    definition.set_editor_property(prop, value)
save(definition)
profile = assets.load_asset("/Game/Cabinets/AlienInvasion/Data/DA_AlienScoring")
rules = [r for r in profile.get_editor_property("mini_game_rules") if str(r.get_editor_property("profile_key")) != "PlanetaryDefense"]
rule = unreal.MiniGameScoreRule()
rule.set_editor_property("profile_key", "PlanetaryDefense")
rule.set_editor_property("weights", {"ThreatsDestroyed": 250.0, "StructuresSurviving": 1000.0})
rules.append(rule)
profile.set_editor_property("mini_game_rules", rules)
profile.set_editor_property("revision", "Phase7-1")
save(profile)
cabinet = assets.load_asset("/Game/Cabinets/AlienInvasion/Data/DA_AlienCabinet")
games = [d for d in cabinet.get_editor_property("mini_games") if str(d.get_editor_property("mini_game_id")) != "PlanetaryDefense"]
cabinet.set_editor_property("mini_games", games + [definition])
cabinet.set_editor_property("development_without_minigames", True)
# Individual definitions now select their own HUD; the cabinet retains a shared fallback.
cabinet.set_editor_property("mini_game_hud_widget", assets.load_blueprint_class("/Game/Framework/UI/WBP_MinigameHUD"))
save(cabinet)
test_path = "/Game/Tests/Data/DA_DefenseTestCabinet"
if not assets.does_asset_exist(test_path):
    assert assets.duplicate_asset(cabinet.get_path_name(), test_path)
test_cabinet = assets.load_asset(test_path)
test_cabinet.set_editor_property("mini_games", [definition])
test_cabinet.set_editor_property("mini_game_hud_widget", hud.generated_class())
test_cabinet.set_editor_property("cabinet_id", "DefenseTest")
save(test_cabinet)
mode = blueprint("/Game/Tests/Blueprints/BP_DefenseGameMode", "PinballGameModeBase")
mode_defaults = unreal.get_default_object(mode.generated_class())
mode_defaults.set_editor_property("cabinet", test_cabinet)
mode_defaults.set_editor_property("player_controller_class", assets.load_blueprint_class("/Game/Framework/Blueprints/BP_PinballPlayerController"))
mode_defaults.set_editor_property("default_pawn_class", cabinet.get_editor_property("control_pawn_class"))
unreal.BlueprintEditorLibrary.compile_blueprint(mode)
save(mode)
test_map = "/Game/Tests/Maps/L_DefenseTest"
if not assets.does_asset_exist(test_map):
    assert assets.duplicate_asset("/Game/Tests/Maps/L_TransitionTest", test_map)
for map_path in (test_map, "/Game/Cabinets/AlienInvasion/Maps/L_AlienCabinet"):
    assert levels.load_level(map_path)
    table = next(a for a in actors.get_all_level_actors() if isinstance(a, unreal.PinballTable))
    objectives = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.MinigameObjective)]
    if map_path == test_map:
        objective = objectives[0]
    else:
        matches = [a for a in objectives if str(a.get_editor_property("trigger").get_editor_property("objective_id")) == "DefenseObjective"]
        objective = matches[0] if matches else actors.spawn_actor_from_class(assets.load_blueprint_class("/Game/Framework/Pinball/BP_MinigameObjective"), unreal.Vector(), unreal.Rotator())
    objective.set_actor_location(table.get_actor_transform().transform_location(unreal.Vector(-190, 540, 20)), False, False)
    trigger = objective.get_editor_property("trigger")
    trigger.set_editor_property("table", table)
    trigger.set_editor_property("definition", definition)
    trigger.set_editor_property("objective_id", "DefenseObjective")
    objective.set_actor_label("Planetary Defense Objective")
    objective.get_component_by_class(unreal.StaticMeshComponent).set_material(0, materials["Colony"])
    if map_path == test_map:
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        world.get_world_settings().set_editor_property("default_game_mode", mode.generated_class())
        for actor in actors.get_all_level_actors():
            if isinstance(actor, unreal.PinballTransitionFunctionalTest):
                actors.destroy_actor(actor)
        probes = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.PlanetaryDefenseProbe)]
        probe = probes[0] if probes else actors.spawn_actor_from_class(native("PlanetaryDefenseProbe"), unreal.Vector(), unreal.Rotator())
        probe.set_editor_property("objective", objective)
    assert levels.save_current_level()
test_cabinet.set_editor_property("persistent_map", assets.load_asset(test_map))
save(test_cabinet)
unreal.log("PINBALL_PHASE7_ASSETS_CREATED")
