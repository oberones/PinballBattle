"""Author real Phase 3 packages; replace only this script's tagged cabinet actors on rerun."""
import math
import os
import struct
import sys
import wave
import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cabinet_lighting import apply_cabinet_lighting
from cabinet_return import configure_return

assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
framework = "/Game/Framework/Pinball/"
cabinet = "/Game/Cabinets/AlienInvasion/"
tests = "/Game/Tests/Blueprints/"


def save(asset):
    """Persist an authored package immediately and fail if Unreal cannot save it."""
    assert assets.save_loaded_asset(asset, only_if_is_dirty=False), asset.get_path_name()


def make(path, cls, factory):
    """Reuse an existing real asset or create one through its registered Unreal factory."""
    if assets.does_asset_exist(path):
        return assets.load_asset(path)
    folder, name = path.rsplit("/", 1)
    result = tools.create_asset(name, folder, cls, factory)
    assert result, path
    return result


def bp(path, native, configure=None, widget=False):
    """Compile a native-backed Blueprint after applying explicit content defaults."""
    factory = unreal.WidgetBlueprintFactory() if widget else unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/PinballBattle." + native))
    result = make(path, unreal.WidgetBlueprint if widget else unreal.Blueprint, factory)
    if configure:
        configure(unreal.get_default_object(result.generated_class()))
    unreal.BlueprintEditorLibrary.compile_blueprint(result)
    save(result)
    return result


def material(name, color, emission=0):
    """Create a simple original cabinet material with optional low-level emissive color."""
    result = make(cabinet + "Art/" + name, unreal.Material, unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(result)
    value = unreal.MaterialEditingLibrary.create_material_expression(result, unreal.MaterialExpressionConstant3Vector)
    value.set_editor_property("constant", unreal.LinearColor(*color, 1))
    unreal.MaterialEditingLibrary.connect_material_property(value, "", unreal.MaterialProperty.MP_BASE_COLOR)
    if emission:
        glow = unreal.MaterialEditingLibrary.create_material_expression(result, unreal.MaterialExpressionConstant3Vector)
        glow.set_editor_property("constant", unreal.LinearColor(*(v * emission for v in color), 1))
        unreal.MaterialEditingLibrary.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(result)
    save(result)
    return result


def physical(name, friction, restitution, combine):
    """Separate low-bounce playfield contact from elastic rails and target faces."""
    result = make(cabinet + "Data/" + name, unreal.PhysicalMaterial, unreal.PhysicalMaterialFactoryNew())
    result.set_editor_property("friction", friction)
    result.set_editor_property("restitution", restitution)
    result.set_editor_property("override_restitution_combine_mode", True)
    result.set_editor_property("restitution_combine_mode", combine)
    save(result)
    return result


def sound(name, frequency, duration):
    """Synthesize an original decaying two-partial chime and import it as a SoundWave."""
    directory = os.path.join(unreal.Paths.project_saved_dir(), "Phase3Audio")
    os.makedirs(directory, exist_ok=True)
    path = os.path.abspath(os.path.join(directory, name + ".wav"))
    rate = 44100
    frames = bytearray()
    for i in range(int(rate * duration)):
        t = i / rate
        envelope = min(1, t / .003) * math.exp(-7 * t / duration)
        value = .55 * envelope * (math.sin(2 * math.pi * frequency * t) + .25 * math.sin(2 * math.pi * frequency * 2.7 * t))
        frames.extend(struct.pack("<h", round(32767 * value)))
    with wave.open(path, "wb") as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(rate)
        output.writeframes(frames)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", path)
    task.set_editor_property("destination_path", framework + "Audio")
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tools.import_asset_tasks([task])
    result = assets.load_asset(framework + "Audio/" + name)
    assert result, name
    return result


target_sound = sound("S_Target", 760, .18)
bumper_sound = sound("S_Bumper", 310, .13)
lane_sound = sound("S_Lane", 1100, .32)


def feedback_defaults(cdo, audio, color):
    """Set content-owned feedback without adding score or UI logic to the actor."""
    feedback = cdo.get_editor_property("feedback")
    feedback.set_editor_property("hit_sound", audio)
    feedback.set_editor_property("color", unreal.LinearColor(*color, 1))


target = bp(framework + "BP_ScoringTarget", "PinballScoringTarget",
            lambda cdo: feedback_defaults(cdo, target_sound, (.15, 1, .4)))
lane = bp(framework + "BP_Lane", "PinballLane",
          lambda cdo: feedback_defaults(cdo, lane_sound, (.15, .5, 1)))
bumper = bp(framework + "BP_Bumper", "PinballBumper",
            lambda cdo: feedback_defaults(cdo, bumper_sound, (1, .3, .05)))
ball = assets.load_asset(framework + "BP_PinballBall")
flipper = assets.load_asset(framework + "BP_Flipper")
plunger = assets.load_asset(framework + "BP_Plunger")
drain = assets.load_asset(framework + "BP_Drain")

ball_physics = physical("PM_Ball", .12, .3, unreal.FrictionCombineMode.AVERAGE)
floor_physics = physical("PM_Playfield", .18, .08, unreal.FrictionCombineMode.MIN)
rubber_physics = physical("PM_Rubber", .3, .65, unreal.FrictionCombineMode.MAX)
target_physics = physical("PM_Target", .18, .4, unreal.FrictionCombineMode.MAX)
factory = unreal.DataAssetFactory()
tuning_class = unreal.load_class(None, "/Script/PinballBattle.PinballTuningData")
factory.set_editor_property("data_asset_class", tuning_class)
tuning = make(cabinet + "Data/DA_AlienPhysics", tuning_class, factory)
tuning.set_editor_property("physical_material", ball_physics)
save(tuning)


def controls(cdo):
    """Explain repeat-ball controls and event counters without claiming a scored session."""
    cdo.set_editor_property("heading", "ALIEN INVASION\nTABLE PRACTICE")
    cdo.set_editor_property("instructions", "\nDOWN ARROW\nHold, then release to launch.\n\nLEFT / RIGHT ARROWS\nOperate the flippers.\n\nLight the four green targets.\nBlue lanes count in the\narrow direction.\n\nObjectives are previews.\nUnlimited practice balls.\n\n")


widget = bp(tests + "WBP_TableInteractions", "PrototypeControlsWidget", controls, widget=True)


def table_defaults(cdo):
    """Keep cabinet inventory/tuning on the cabinet Blueprint and practice UI on its map instance."""
    cdo.set_editor_property("tuning", tuning)
    cdo.set_editor_property("ball_class", ball.generated_class())
    cdo.set_editor_property("require_complete_inventory", True)
    configure_return(cdo)


table_bp = bp(cabinet + "Blueprints/BP_AlienTable", "PinballTable", table_defaults)
map_path = cabinet + "Maps/L_AlienCabinet"
if assets.does_asset_exist(map_path):
    assert levels.load_level(map_path)
else:
    assert levels.new_level(map_path)
for actor in actors.get_all_level_actors():
    if unreal.Name("Phase3Authoring") in actor.tags:
        actors.destroy_actor(actor)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", assets.load_blueprint_class(tests + "BP_PracticeGameMode"))
incline = math.radians(tuning.get_editor_property("incline_degrees"))
table_rotation = unreal.Rotator(roll=-math.degrees(incline))


def point(x, y, z):
    """Convert authored playfield centimetres into the six-degree inclined world plane."""
    return unreal.Vector(x, y * math.cos(incline) - z * math.sin(incline), y * math.sin(incline) + z * math.cos(incline))


def rotation(yaw=0):
    """Compose a local playfield heading with the cabinet incline."""
    return unreal.MathLibrary.compose_rotators(unreal.Rotator(yaw=yaw), table_rotation)


def spawn(cls, label, pos, rot=None):
    """Spawn one explicitly referenced actor and tag it for safe repeat authoring."""
    actor = actors.spawn_actor_from_class(cls, pos, rot or table_rotation)
    assert actor, label
    actor.set_actor_label(label)
    actor.set_editor_property("tags", [unreal.Name("Phase3Authoring")])
    return actor


table = spawn(table_bp.generated_class(), "Alien Table", unreal.Vector())
table.set_editor_property("controls_widget_class", widget.generated_class())
camera = table.get_editor_property("table_camera")
camera.set_relative_location(unreal.Vector(-20, 570, 2350), False, False)
camera.set_relative_rotation(unreal.Rotator(pitch=-90, yaw=90), False, False)
left = spawn(flipper.generated_class(), "Left Flipper", point(125, 165, 18), rotation(174.5))
right = spawn(flipper.generated_class(), "Right Flipper", point(-145, 165, 18), rotation(5.5))
left.set_editor_property("reverse_drive", True)
launcher = spawn(plunger.generated_class(), "Plunger", point(270, 38, 15), rotation(90))
drain_actor = spawn(drain.generated_class(), "Drain", point(0, -25, 15))
drain_actor.get_editor_property("drain").set_box_extent(unreal.Vector(335, 35, 60), False)
bumper_actors = [spawn(bumper.generated_class(), "Bumper " + str(i + 1), point(x, y, 27))
                 for i, (x, y) in enumerate(((-130, 780), (80, 860), (-10, 620)))]
# Four faces project into reachable open space; no narrow pockets behind stand-up targets.
target_actors = [spawn(target.generated_class(), "Target " + str(i + 1), point(x, y, 27), rotation(yaw))
                 for i, (x, y, yaw) in enumerate(((-280, 540, 90), (-280, 900, 90), (190, 710, 90), (80, 1135, 0)))]
# Lane A is the upward shooter lane; Lane B is an open left return lane directed downhill.
lane_actors = [spawn(lane.generated_class(), "Launch Lane UP", point(274, 680, 14)),
               spawn(lane.generated_class(), "Return Lane DOWN", point(-238, 640, 14), rotation(180))]
lane_actors[0].get_editor_property("corridor").set_box_extent(unreal.Vector(28, 90, 40), False)
lane_actors[0].get_editor_property("insert").set_relative_scale3d(unreal.Vector(.45, 1.8, .01))
for prop, value in (("left_flipper", left), ("right_flipper", right), ("plunger", launcher),
                    ("drain", drain_actor), ("bumpers", bumper_actors), ("targets", target_actors), ("lanes", lane_actors)):
    table.set_editor_property(prop, value)
for actor in bumper_actors:
    actor.get_editor_property("response").set_editor_property("surface_material", rubber_physics)
for actor in target_actors:
    actor.get_editor_property("scoring").set_editor_property("surface_material", target_physics)

floor_mat = material("M_Playfield", (.015, .035, .07))
wall_mat = material("M_Rails", (.14, .23, .3))
target_mat = material("M_Target", (.08, .75, .25), .4)
lane_mat = material("M_Lane", (.05, .3, .75), .3)
flip_mat = material("M_Flipper", (.1, .7, .95))
bumper_mat = material("M_Bumper", (.85, .2, .04), .3)
objective_mat = material("M_ObjectivePreview", (.5, .16, .8), .4)
cube = assets.load_asset("/Engine/BasicShapes/Cube")


def wall(label, pos, size, yaw=0, mat=None, physics=None):
    """Author thick simple collision with an explicit surface material and no custom solver."""
    actor = spawn(unreal.StaticMeshActor, label, point(*pos), rotation(yaw))
    comp = actor.static_mesh_component
    comp.set_static_mesh(cube)
    actor.set_actor_scale3d(unreal.Vector(*(v / 100 for v in size)))
    comp.set_material(0, mat or wall_mat)
    comp.set_phys_material_override(physics or rubber_physics)
    comp.set_collision_profile_name("BlockAll")
    comp.set_collision_response_to_all_channels(unreal.CollisionResponseType.ECR_BLOCK)
    return actor


wall("Playfield", (0, 565, -22), (640, 1230, 44), mat=floor_mat, physics=floor_physics)
wall("Left Rail", (-320, 570, 32), (35, 1230, 90))
wall("Right Rail", (320, 570, 32), (35, 1230, 90))
wall("Top Rail", (0, 1180, 32), (670, 40, 90))
wall("Launch Divider", (234, 490, 32), (25, 1050, 90))
wall("Launch Guide", (252, 1110, 35), (150, 28, 95), -40)
# Trim the rail-side end while preserving the original flipper-side tip.
wall("Left Return Guide", (-235 + 35 * math.cos(math.radians(40)),
                           300 - 35 * math.sin(math.radians(40)), 30), (250, 26, 70), -40)
wall("Right Return Guide", (164, 280, 30), (210, 26, 70), 48)
wall("Upper Obstacle", (-155, 1015, 30), (150, 28, 70), 20)
for actor in (left, right):
    actor.get_component_by_class(unreal.StaticMeshComponent).set_material(0, flip_mat)
for actor in bumper_actors:
    actor.get_editor_property("surface").set_material(0, bumper_mat)
for actor in target_actors:
    actor.get_editor_property("surface").set_material(0, target_mat)
for actor in lane_actors:
    actor.get_editor_property("insert").set_material(0, lane_mat)
launcher.get_component_by_class(unreal.StaticMeshComponent).set_material(0, flip_mat)


def label(text, pos, size=18):
    """Place a readable playfield label facing the fixed overhead player camera."""
    actor = spawn(unreal.TextRenderActor, text, point(*pos), rotation())
    component = actor.get_component_by_class(unreal.TextRenderComponent)
    component.set_text(text)
    component.set_world_size(size)
    component.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    component.set_relative_rotation(unreal.MathLibrary.compose_rotators(
        unreal.Rotator(pitch=90, yaw=-90), table_rotation), False, False)
    return actor


for title, x, y in (("ASTEROID FIELD", -110, 1090), ("PLANETARY DEFENSE", 75, 980), ("ALIEN ASSAULT", 60, 490)):
    pad = wall(title + " preview", (x, y, .6), (130, 62, 1), mat=objective_mat)
    pad.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    label(title + "\nPREVIEW", (x, y, 2), 13)
label("^", (274, 690, 2), 25)
label("v", (-238, 640, 2), 25)
label("ALIEN INVASION", (0, 380, 2), 23)
spawn(unreal.PlayerStart, "Player Start", point(0, -180, 120))
post = spawn(unreal.PostProcessVolume, "Exposure", unreal.Vector(), unreal.Rotator())
post.set_editor_property("unbound", True)
settings = post.get_editor_property("settings")
settings.set_editor_property("override_auto_exposure_min_brightness", True)
settings.set_editor_property("override_auto_exposure_max_brightness", True)
settings.set_editor_property("auto_exposure_min_brightness", 1)
settings.set_editor_property("auto_exposure_max_brightness", 1)
post.set_editor_property("settings", settings)
apply_cabinet_lighting(table)
probe = unreal.load_class(None, "/Script/PinballBattle.TableInteractionProbe")
if probe:
    spawn(probe, "Table Validation (command-line opt-in)", unreal.Vector(), unreal.Rotator())
assert levels.save_current_level()
unreal.log("PINBALL_PHASE3_ASSETS_CREATED")
