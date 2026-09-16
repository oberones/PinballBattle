"""Read saved Phase 1 content and verify actual class, mapping and empty-map wiring."""

import unreal


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def load(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    require(asset is not None, "Missing asset: " + path)
    return asset


def mappings(context):
    data = context.get_editor_property("default_key_mappings")
    return {
        (entry.get_editor_property("action").get_name(), str(entry.get_editor_property("key").get_editor_property("key_name")))
        for entry in data.get_editor_property("mappings")
    }


root = "/Game/Framework/"
common = load(root + "Input/IMC_Common")
pinball = load(root + "Input/IMC_Pinball")
require(mappings(common) == {("IA_Pause", "Escape")}, "Incorrect common bindings")
require(mappings(pinball) == {
    ("IA_LeftFlipper", "Left"), ("IA_RightFlipper", "Right"), ("IA_Plunger", "Down")
}, "Incorrect pinball bindings")
for name in ("LeftFlipper", "RightFlipper", "Plunger", "Pause"):
    action = load(root + "Input/IA_" + name)
    require(action.get_editor_property("value_type") == unreal.InputActionValueType.BOOLEAN, name + " must be Boolean")
    require(not action.get_editor_property("triggers"), name + " must preserve ordinary press/hold/release phases")
    require(action.get_editor_property("trigger_when_paused") == (name == "Pause"), "Incorrect pause execution: " + name)

controller = load(root + "Blueprints/BP_PinballPlayerController")
pawn = load(root + "Pinball/BP_PinballControlPawn")
mode = load(root + "Blueprints/BP_PinballGameMode")
for asset in (controller, pawn, mode):
    require(asset.generated_class() is not None, "Blueprint has no generated class")
defaults = unreal.get_default_object(controller.generated_class())
require(defaults.get_editor_property("common_mapping_context") == common, "Common context not assigned")
require(defaults.get_editor_property("pinball_mapping_context") == pinball, "Pinball context not assigned")
mode_defaults = unreal.get_default_object(mode.generated_class())
require(mode_defaults.get_editor_property("player_controller_class") == controller.generated_class(), "Controller class mismatch")
require(mode_defaults.get_editor_property("default_pawn_class") == pawn.generated_class(), "Pawn class mismatch")
require(mode_defaults.get_editor_property("game_state_class") == unreal.load_class(None, "/Script/PinballBattle.PinballGameStateBase"), "GameState class mismatch")

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
require(levels.load_level("/Game/Tests/Maps/L_PhysicsPrototype"), "Cannot load saved prototype map")
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
require(world.get_world_settings().get_editor_property("default_game_mode") == mode.generated_class(), "Map GameMode override mismatch")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
require(sum(isinstance(actor, unreal.PlayerStart) for actor in actors) == 1, "Expected one authored PlayerStart")
require(not any(isinstance(actor, unreal.Pawn) for actor in actors), "Pawn must be spawned once by GameMode")
unreal.log("PINBALL_PHASE1_ASSET_VALIDATION_PASSED: Blueprint classes, four Boolean actions, two contexts, map and spawn wiring")
