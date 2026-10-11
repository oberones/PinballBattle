"""Author independent Alien Assault assets and add its production cabinet association."""
import math
import os
import struct
import wave
import unreal

assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
base = "/Game/Minigames/AlienAssault/"


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
for name in ("AlienAssaultRuntime", "AssaultShipPawn", "AssaultEnemy", "AssaultProjectile"):
    blueprints[name] = blueprint(base + "Blueprints/BP_" + ("AssaultShip" if name == "AssaultShipPawn" else name), name)
defaults = {name: unreal.get_default_object(asset.generated_class()) for name, asset in blueprints.items()}
root = defaults["AlienAssaultRuntime"]
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
for prop, name in {"enemy_class": "AssaultEnemy", "projectile_class": "AssaultProjectile"}.items():
    root.set_editor_property(prop, blueprints[name].generated_class())
colors = {"Enemy": (1.8, .08, .55), "Ship": (.15, 1.2, 1.8), "Shot": (2.0, 1.2, .12), "Boundary": (.08, .14, .32)}
materials = {name: material("M_Assault" + name, color) for name, color in colors.items()}
for name, color in {"AssaultEnemy": "Enemy", "AssaultShipPawn": "Ship", "AssaultProjectile": "Shot"}.items():
    defaults[name].get_editor_property("visual").set_material(0, materials[color])
for mesh in root.get_components_by_class(unreal.StaticMeshComponent):
    mesh.set_material(0, materials["Boundary"])
mapping = data(base + "Input/IMC_AlienAssault", unreal.InputMappingContext)
mapping.unmap_all()
for label, key_name in (("Left", "Left"), ("Right", "Right"), ("Fire", "SpaceBar")):
    action = data(base + "Input/IA_Assault" + label, unreal.InputAction)
    action.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
    key = unreal.Key()
    key.set_editor_property("key_name", key_name)
    mapping.map_key(action, key)
    defaults["AssaultShipPawn"].set_editor_property(label.lower() + "_action", action)
    if label == "Fire":
        fire = action
    save(action)
save(mapping)

# A short original two-tone electronic interception sound is synthesized locally.
audio_dir = os.path.abspath(os.path.join(unreal.Paths.project_saved_dir(), "Phase8Audio"))
os.makedirs(audio_dir, exist_ok=True)
audio_file = os.path.join(audio_dir, "AssaultIntercept.wav")
with wave.open(audio_file, "wb") as stream:
    stream.setparams((1, 2, 22050, 0, "NONE", "not compressed"))
    stream.writeframes(b"".join(struct.pack("<h", int(13000 * (1 - i / 3087) ** 2 *
        (math.sin(2 * math.pi * 960 * i / 22050) + .25 * math.sin(2 * math.pi * 1440 * i / 22050)))) for i in range(3087)))
task = unreal.AssetImportTask()
for prop, value in {"filename": audio_file, "destination_path": base + "Audio", "automated": True,
                    "replace_existing": True, "save": True}.items():
    task.set_editor_property(prop, value)
tools.import_asset_tasks([task])
root.set_editor_property("destruction_sound", assets.load_asset(base + "Audio/AssaultIntercept"))
for asset in blueprints.values():
    unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    save(asset)

hud_factory = unreal.WidgetBlueprintFactory()
hud_factory.set_editor_property("parent_class", native("PinballPresentationWidget"))
hud = make(base + "UI/WBP_AssaultHUD", unreal.WidgetBlueprint, hud_factory)
hud_defaults = unreal.get_default_object(hud.generated_class())
hud_defaults.set_editor_property("screen", unreal.PinballScreen.MINI_GAME_HUD)
hud_defaults.set_editor_property("heading", "ALIEN\nASSAULT")
hud_defaults.set_editor_property("status_wrap_width", 260)
unreal.BlueprintEditorLibrary.compile_blueprint(hud)
save(hud)
arena_path = base + "Maps/L_MG_AlienAssault"
assert levels.load_level(arena_path) if assets.does_asset_exist(arena_path) else levels.new_level(arena_path)
roots = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.AlienAssaultRuntime)]
if not roots:
    actors.spawn_actor_from_class(blueprints["AlienAssaultRuntime"].generated_class(), unreal.Vector(), unreal.Rotator())
assert levels.save_current_level()
definition = data(base + "Data/DA_MG_AlienAssault", native("MiniGameDefinition"))
for prop, value in {
    "mini_game_id": "AlienAssault", "display_name": "Alien Assault",
    "instructions": "LEFT/RIGHT moves your ship along the bottom. Hold SPACE to fire upward. Each destroyed alien returns to its formation slot after three active seconds. Dodge incoming shots. Three lives; a flashing shield protects each respawn for two seconds. Survive 30 active seconds. Four kills and 10 seconds earn 2,000; 28 kills and 30 seconds earn 10,000. ESCAPE pauses.",
    "map": assets.load_asset(arena_path), "runtime_class": blueprints["AlienAssaultRuntime"].generated_class(),
    "pawn_class": blueprints["AssaultShipPawn"].generated_class(), "input_context": mapping, "action_input": fire,
    "hud_class": hud.generated_class(), "uses_mouse_aim": False,
    "arena_transform": unreal.Transform(location=unreal.Vector(30000, 0, 0)), "arena_extent": unreal.Vector(800, 430, 1500),
    "duration_seconds": 30.0, "local_lives": 3, "metric_bounds": {"EnemiesDestroyed": 120.0, "SurvivalSeconds": 30.0},
    "maximum_objectives": 120, "scoring_profile_key": "AlienAssault",
}.items():
    definition.set_editor_property(prop, value)
save(definition)
profile = assets.load_asset("/Game/Cabinets/AlienInvasion/Data/DA_AlienScoring")
rules = [r for r in profile.get_editor_property("mini_game_rules") if str(r.get_editor_property("profile_key")) != "AlienAssault"]
rule = unreal.MiniGameScoreRule()
rule.set_editor_property("profile_key", "AlienAssault")
rule.set_editor_property("weights", {"EnemiesDestroyed": 250.0, "SurvivalSeconds": 100.0})
rule.set_editor_property("floor_metrics", ["SurvivalSeconds"])
rules.append(rule)
profile.set_editor_property("mini_game_rules", rules)
profile.set_editor_property("revision", "Phase8-1")
save(profile)
cabinet = assets.load_asset("/Game/Cabinets/AlienInvasion/Data/DA_AlienCabinet")
games = [d for d in cabinet.get_editor_property("mini_games") if str(d.get_editor_property("mini_game_id")) != "AlienAssault"]
cabinet.set_editor_property("mini_games", games + [definition])
cabinet.set_editor_property("development_without_minigames", True)
# Individual definitions now select their own HUD; the cabinet retains a shared fallback.
cabinet.set_editor_property("mini_game_hud_widget", assets.load_blueprint_class("/Game/Framework/UI/WBP_MinigameHUD"))
save(cabinet)
test_path = "/Game/Tests/Data/DA_AssaultTestCabinet"
if not assets.does_asset_exist(test_path):
    assert assets.duplicate_asset(cabinet.get_path_name(), test_path)
test_cabinet = assets.load_asset(test_path)
test_cabinet.set_editor_property("mini_games", [definition])
test_cabinet.set_editor_property("mini_game_hud_widget", hud.generated_class())
test_cabinet.set_editor_property("cabinet_id", "AssaultTest")
save(test_cabinet)
mode = blueprint("/Game/Tests/Blueprints/BP_AssaultGameMode", "PinballGameModeBase")
mode_defaults = unreal.get_default_object(mode.generated_class())
mode_defaults.set_editor_property("cabinet", test_cabinet)
mode_defaults.set_editor_property("player_controller_class", assets.load_blueprint_class("/Game/Framework/Blueprints/BP_PinballPlayerController"))
mode_defaults.set_editor_property("default_pawn_class", cabinet.get_editor_property("control_pawn_class"))
unreal.BlueprintEditorLibrary.compile_blueprint(mode)
save(mode)
test_map = "/Game/Tests/Maps/L_AssaultTest"
if not assets.does_asset_exist(test_map):
    assert assets.duplicate_asset("/Game/Tests/Maps/L_TransitionTest", test_map)
for map_path in (test_map, "/Game/Cabinets/AlienInvasion/Maps/L_AlienCabinet"):
    assert levels.load_level(map_path)
    table = next(a for a in actors.get_all_level_actors() if isinstance(a, unreal.PinballTable))
    objectives = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.MinigameObjective)]
    if map_path == test_map:
        objective = objectives[0]
    else:
        matches = [a for a in objectives if str(a.get_editor_property("trigger").get_editor_property("objective_id")) == "AssaultObjective"]
        objective = matches[0] if matches else actors.spawn_actor_from_class(assets.load_blueprint_class("/Game/Framework/Pinball/BP_MinigameObjective"), unreal.Vector(), unreal.Rotator())
    objective.set_actor_location(table.get_actor_transform().transform_location(unreal.Vector(190, 540, 20)), False, False)
    trigger = objective.get_editor_property("trigger")
    trigger.set_editor_property("table", table)
    trigger.set_editor_property("definition", definition)
    trigger.set_editor_property("objective_id", "AssaultObjective")
    objective.set_actor_label("Alien Assault Objective")
    objective.get_component_by_class(unreal.StaticMeshComponent).set_material(0, materials["Enemy"])
    if map_path == test_map:
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        world.get_world_settings().set_editor_property("default_game_mode", mode.generated_class())
        for actor in actors.get_all_level_actors():
            if isinstance(actor, unreal.PinballTransitionFunctionalTest):
                actors.destroy_actor(actor)
        probes = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.AlienAssaultProbe)]
        probe = probes[0] if probes else actors.spawn_actor_from_class(native("AlienAssaultProbe"), unreal.Vector(), unreal.Rotator())
        probe.set_editor_property("objective", objective)
    assert levels.save_current_level()
test_cabinet.set_editor_property("persistent_map", assets.load_asset(test_map))
save(test_cabinet)
unreal.log("PINBALL_PHASE8_ASSETS_CREATED")
