"""Author Phase 1 assets with UE 5.8 editor Python after building the native module.

Run once using UnrealEditor-Cmd -run=pythonscript -script=<this file>.
Existing assets are preserved; validation reports configuration drift instead of overwriting it.
"""

import unreal


ASSETS = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()


def save(asset):
    if not ASSETS.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError("Could not save " + asset.get_path_name())


def data_asset(path, asset_class, configure):
    existing = ASSETS.load_asset(path) if ASSETS.does_asset_exist(path) else None
    if existing:
        return existing
    folder, name = path.rsplit("/", 1)
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", asset_class)
    asset = TOOLS.create_asset(name, folder, asset_class, factory)
    if not asset:
        raise RuntimeError("Could not create " + path)
    configure(asset)
    save(asset)
    return asset


def blueprint(path, parent, configure):
    if ASSETS.does_asset_exist(path):
        return ASSETS.load_asset(path)
    folder, name = path.rsplit("/", 1)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent)
    asset = TOOLS.create_asset(name, folder, unreal.Blueprint, factory)
    if not asset:
        raise RuntimeError("Could not create " + path)
    configure(unreal.get_default_object(asset.generated_class()))
    unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    save(asset)
    return asset


def configure_action(action, paused=False):
    action.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
    action.set_editor_property("trigger_when_paused", paused)


input_root = "/Game/Framework/Input/"
actions = {}
for name in ("LeftFlipper", "RightFlipper", "Plunger", "Pause"):
    actions[name] = data_asset(
        input_root + "IA_" + name,
        unreal.InputAction,
        lambda action, paused=(name == "Pause"): configure_action(action, paused),
    )


def configure_pinball(context):
    for action, key in (("LeftFlipper", "Left"), ("RightFlipper", "Right"), ("Plunger", "Down")):
        context.map_key(actions[action], input_key(key))


def input_key(name):
    key = unreal.Key()
    key.set_editor_property("key_name", name)
    return key


common = data_asset(
    input_root + "IMC_Common", unreal.InputMappingContext,
    lambda context: context.map_key(actions["Pause"], input_key("Escape")),
)
pinball = data_asset(input_root + "IMC_Pinball", unreal.InputMappingContext, configure_pinball)

pawn = blueprint(
    "/Game/Framework/Pinball/BP_PinballControlPawn",
    unreal.load_class(None, "/Script/PinballBattle.PinballControlPawn"),
    lambda defaults: None,
)


def configure_controller(defaults):
    defaults.set_editor_property("common_mapping_context", common)
    defaults.set_editor_property("pinball_mapping_context", pinball)


controller = blueprint(
    "/Game/Framework/Blueprints/BP_PinballPlayerController",
    unreal.load_class(None, "/Script/PinballBattle.PinballPlayerController"),
    configure_controller,
)


def configure_game_mode(defaults):
    defaults.set_editor_property("player_controller_class", controller.generated_class())
    defaults.set_editor_property("default_pawn_class", pawn.generated_class())


game_mode = blueprint(
    "/Game/Framework/Blueprints/BP_PinballGameMode",
    unreal.load_class(None, "/Script/PinballBattle.PinballGameModeBase"),
    configure_game_mode,
)

map_path = "/Game/Tests/Maps/L_PhysicsPrototype"
if not ASSETS.does_asset_exist(map_path):
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.new_level(map_path, is_partitioned_world=False):
        raise RuntimeError("Could not create prototype level")
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", game_mode.generated_class())
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 100))
    if not start:
        raise RuntimeError("Could not author the prototype PlayerStart")
    if not levels.save_current_level():
        raise RuntimeError("Could not save prototype level")

unreal.log("PINBALL_PHASE1_ASSETS_CREATED")
