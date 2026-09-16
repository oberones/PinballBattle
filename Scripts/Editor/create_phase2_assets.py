"""Author the first playable table using real Unreal assets and explicit instance references.

Build the native module first. Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>.
Reruns update these prototype assets, replacing only actors tagged Phase2Authoring.
"""
import math
import unreal

assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
root = "/Game/Framework/Pinball/"
test_root = "/Game/Tests/Blueprints/"


def save(asset):
    assert assets.save_loaded_asset(asset, only_if_is_dirty=False), asset.get_path_name()


def make(path, cls, factory):
    if assets.does_asset_exist(path):
        return assets.load_asset(path)
    folder, name = path.rsplit("/", 1)
    result = tools.create_asset(name, folder, cls, factory)
    assert result, path
    return result


def bp(path, native, configure=lambda cdo: None, widget=False):
    factory = unreal.WidgetBlueprintFactory() if widget else unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/PinballBattle." + native))
    result = make(path, unreal.WidgetBlueprint if widget else unreal.Blueprint, factory)
    configure(unreal.get_default_object(result.generated_class()))
    unreal.BlueprintEditorLibrary.compile_blueprint(result)
    save(result)
    return result


def material(name, color, folder=test_root):
    result = make(folder + name, unreal.Material, unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(result)
    value = unreal.MaterialEditingLibrary.create_material_expression(result, unreal.MaterialExpressionConstant3Vector)
    value.set_editor_property("constant", unreal.LinearColor(*color, 1))
    unreal.MaterialEditingLibrary.connect_material_property(value, "", unreal.MaterialProperty.MP_BASE_COLOR)
    roughness = unreal.MaterialEditingLibrary.create_material_expression(result, unreal.MaterialExpressionConstant)
    roughness.set_editor_property("r", .4)
    unreal.MaterialEditingLibrary.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(result)
    save(result)
    return result


physical = make(root + "PM_Pinball", unreal.PhysicalMaterial, unreal.PhysicalMaterialFactoryNew())
physical.set_editor_property("friction", .12)
physical.set_editor_property("restitution", .65)
physical.set_editor_property("override_friction_combine_mode", True)
physical.set_editor_property("friction_combine_mode", unreal.FrictionCombineMode.MIN)
physical.set_editor_property("override_restitution_combine_mode", True)
physical.set_editor_property("restitution_combine_mode", unreal.FrictionCombineMode.MAX)
save(physical)

factory = unreal.DataAssetFactory()
tuning_class = unreal.load_class(None, "/Script/PinballBattle.PinballTuningData")
factory.set_editor_property("data_asset_class", tuning_class)
tuning = make(root + "DA_PinballTuning", tuning_class, factory)
tuning.set_editor_property("physical_material", physical)
save(tuning)

ball = bp(root + "BP_PinballBall", "PinballBall")
flipper = bp(root + "BP_Flipper", "PinballFlipper")
plunger = bp(root + "BP_Plunger", "PinballPlunger")
bumper = bp(root + "BP_Bumper", "PinballBumper")
drain = bp(root + "BP_Drain", "PinballDrain")


def controls_defaults(cdo):
    cdo.set_editor_property("heading", "PINBALL\nPRACTICE")
    cdo.set_editor_property("instructions", "\nDOWN ARROW\nHold to charge.\nRelease to launch.\n\nLEFT / RIGHT\nOperate the flippers.\n\nKeep the ball bouncing!\nA new ball appears\nafter every drain.\n\n")


widget = bp(test_root + "WBP_PrototypeControls", "PrototypeControlsWidget", controls_defaults, widget=True)


def table_defaults(cdo):
    cdo.set_editor_property("tuning", tuning)
    cdo.set_editor_property("ball_class", ball.generated_class())
    cdo.set_editor_property("controls_widget_class", widget.generated_class())


table_bp = bp(test_root + "BP_PrototypeTable", "PinballTable", table_defaults)
controller = assets.load_asset("/Game/Framework/Blueprints/BP_PinballPlayerController")
controller_cdo = unreal.get_default_object(controller.generated_class())
for field, action in (("left_flipper_action", "LeftFlipper"), ("right_flipper_action", "RightFlipper"), ("plunger_action", "Plunger")):
    controller_cdo.set_editor_property(field, assets.load_asset("/Game/Framework/Input/IA_" + action))
unreal.BlueprintEditorLibrary.compile_blueprint(controller)
save(controller)


def mode_defaults(cdo):
    cdo.set_editor_property("practice_mode", True)
    cdo.set_editor_property("player_controller_class", controller.generated_class())
    cdo.set_editor_property("default_pawn_class", assets.load_blueprint_class(root + "BP_PinballControlPawn"))


mode = bp(test_root + "BP_PracticeGameMode", "PinballGameModeBase", mode_defaults)
levels.load_level("/Game/Tests/Maps/L_PhysicsPrototype")
for actor in actors.get_all_level_actors():
    if unreal.Name("Phase2Authoring") in actor.tags:
        actors.destroy_actor(actor)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", mode.generated_class())

incline = math.radians(tuning.get_editor_property("incline_degrees"))
table_rotation = unreal.Rotator(roll=-math.degrees(incline))


def point(x, y, z):
    return unreal.Vector(x, y * math.cos(incline) - z * math.sin(incline),
                         y * math.sin(incline) + z * math.cos(incline))


def rotation(yaw=0):
    return unreal.MathLibrary.compose_rotators(unreal.Rotator(yaw=yaw), table_rotation)


def spawn(cls, name, pos, rot=None):
    actor = actors.spawn_actor_from_class(cls, pos, rot or table_rotation)
    assert actor, name
    actor.set_actor_label(name)
    actor.set_editor_property("tags", [unreal.Name("Phase2Authoring")])
    return actor


table = spawn(table_bp.generated_class(), "Prototype Table", unreal.Vector())
camera = table.get_editor_property("table_camera")
camera.set_relative_location(unreal.Vector(-20, 570, 2350), False, False)
camera.set_relative_rotation(unreal.Rotator(pitch=-90, yaw=90), False, False)
camera.set_editor_property("field_of_view", 53)
left = spawn(flipper.generated_class(), "Left Flipper", point(125, 165, 18), rotation(174.5))
right = spawn(flipper.generated_class(), "Right Flipper", point(-145, 165, 18), rotation(5.5))
left.set_editor_property("reverse_drive", True)
launcher = spawn(plunger.generated_class(), "Plunger", point(270, 38, 15), rotation(90))
drain_actor = spawn(drain.generated_class(), "Drain", point(0, -25, 15))
drain_actor.get_editor_property("drain").set_box_extent(unreal.Vector(335, 35, 60), False)
bumper_actors = [spawn(bumper.generated_class(), "Bumper " + str(i + 1), point(x, y, 27))
                 for i, (x, y) in enumerate(((-130, 780), (80, 860), (-10, 620)))]
table.set_editor_property("left_flipper", left)
table.set_editor_property("right_flipper", right)
table.set_editor_property("plunger", launcher)
table.set_editor_property("drain", drain_actor)
table.set_editor_property("bumpers", bumper_actors)

floor_mat = material("M_PrototypePlayfield", (.025, .065, .10))
wall_mat = material("M_PrototypeWall", (.16, .23, .3))
flip_mat = material("M_PrototypeFlipper", (.10, .7, .95))
bumper_mat = material("M_PrototypeBumper", (.95, .28, .12))
ball_mat = material("M_PinballBall", (1, .85, .15), folder=root)
unreal.get_default_object(ball.generated_class()).get_component_by_class(unreal.StaticMeshComponent).set_material(0, ball_mat)
unreal.BlueprintEditorLibrary.compile_blueprint(ball)
save(ball)
if assets.does_asset_exist(test_root + "M_PrototypeBall"):
    assert assets.delete_asset(test_root + "M_PrototypeBall")
cube = assets.load_asset("/Engine/BasicShapes/Cube")


def wall(name, pos, size, yaw=0, mat=wall_mat):
    actor = spawn(unreal.StaticMeshActor, name, point(*pos), rotation(yaw))
    comp = actor.static_mesh_component
    comp.set_static_mesh(cube)
    actor.set_actor_scale3d(unreal.Vector(*(v / 100 for v in size)))
    comp.set_material(0, mat)
    comp.set_phys_material_override(physical)
    comp.set_collision_profile_name("BlockAll")
    comp.set_collision_response_to_all_channels(unreal.CollisionResponseType.ECR_BLOCK)
    return actor


wall("Playfield", (0, 565, -22), (640, 1230, 44), mat=floor_mat)
wall("Left Rail", (-320, 570, 32), (35, 1230, 90))
wall("Right Rail", (320, 570, 32), (35, 1230, 90))
wall("Top Rail", (0, 1180, 32), (670, 40, 90))
wall("Launch Lane", (234, 490, 32), (25, 1050, 90))
# The ready ball is secured by the lifecycle. A ball returning down the launch lane drains.
wall("Launch Guide", (252, 1110, 35), (150, 28, 95), -40)
# Extend guide ends through the rails, so their end caps cannot form ball-sized pockets.
wall("Left Return Guide", (-235, 300, 30), (320, 26, 70), -40)
wall("Right Return Guide", (164, 280, 30), (210, 26, 70), 48)
wall("Upper Obstacle", (-155, 1015, 30), (150, 28, 70), 20)
for actor in (left, right):
    actor.get_component_by_class(unreal.StaticMeshComponent).set_material(0, flip_mat)
for actor in bumper_actors:
    actor.get_editor_property("surface").set_material(0, bumper_mat)
launcher.get_component_by_class(unreal.StaticMeshComponent).set_material(0, flip_mat)

light = spawn(unreal.DirectionalLight, "Prototype Key Light", unreal.Vector(0, 0, 1500), unreal.Rotator(pitch=-60, yaw=-35))
light.light_component.set_editor_property("intensity", 5)
light.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sky = spawn(unreal.SkyLight, "Prototype Fill", unreal.Vector(0, 0, 1200), unreal.Rotator())
sky.light_component.set_editor_property("intensity", 1)
sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
post = spawn(unreal.PostProcessVolume, "Prototype Exposure", unreal.Vector(), unreal.Rotator())
post.set_editor_property("unbound", True)
settings = post.get_editor_property("settings")
settings.set_editor_property("override_auto_exposure_min_brightness", True)
settings.set_editor_property("override_auto_exposure_max_brightness", True)
settings.set_editor_property("auto_exposure_min_brightness", 1)
settings.set_editor_property("auto_exposure_max_brightness", 1)
post.set_editor_property("settings", settings)
probe_class = unreal.load_class(None, "/Script/PinballBattle.FirstPlayableProbe")
if probe_class:
    spawn(probe_class, "First Playable Validation (command-line opt-in)", unreal.Vector(), unreal.Rotator())
assert levels.save_current_level()
unreal.log("PINBALL_PHASE2_ASSETS_CREATED")
