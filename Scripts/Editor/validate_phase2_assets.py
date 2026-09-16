"""Read saved first-playable content back in a fresh Unreal Editor process."""
import unreal

assets = unreal.EditorAssetLibrary
root = "/Game/Framework/Pinball/"
tests = "/Game/Tests/Blueprints/"
for path in ("BP_PinballBall", "BP_Flipper", "BP_Plunger", "BP_Bumper", "BP_Drain", "DA_PinballTuning", "PM_Pinball"):
    assert assets.load_asset(root + path), path
assert assets.load_asset(tests + "WBP_PrototypeControls").generated_class()
assert assets.load_asset(tests + "BP_PrototypeTable").generated_class()
assert not unreal.get_default_object(assets.load_blueprint_class(
    "/Game/Framework/Blueprints/BP_PinballGameMode")).get_editor_property("practice_mode")
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level("/Game/Tests/Maps/L_PhysicsPrototype")
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
mode = world.get_world_settings().get_editor_property("default_game_mode")
assert mode == assets.load_blueprint_class(tests + "BP_PracticeGameMode")
assert unreal.get_default_object(mode).get_editor_property("practice_mode")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()


def native(name):
    return getattr(unreal, name)


def of_type(name):
    return [actor for actor in actors if isinstance(actor, native(name))]


tables = of_type("PinballTable")
assert len(tables) == 1
table = tables[0]
assert len(of_type("PinballFlipper")) == 2
assert len(of_type("PinballBumper")) == 3
assert len(of_type("PinballPlunger")) == 1
assert len(of_type("PinballDrain")) == 1
assert len(of_type("PinballBall")) == 0, "Ball must be owned and spawned by lifecycle"
assert len([a for a in actors if isinstance(a, unreal.PlayerStart)]) == 1
left = table.get_editor_property("left_flipper")
right = table.get_editor_property("right_flipper")
assert left != right and left in actors and right in actors
assert left.get_editor_property("reverse_drive") and not right.get_editor_property("reverse_drive")
assert table.get_editor_property("plunger") in actors
assert table.get_editor_property("drain") in actors
assert set(table.get_editor_property("bumpers")) == set(of_type("PinballBumper"))
assert table.get_editor_property("tuning") == assets.load_asset(root + "DA_PinballTuning")
assert table.get_editor_property("controls_widget_class") == assets.load_blueprint_class(tests + "WBP_PrototypeControls")
assert table.get_editor_property("ball_class") == assets.load_blueprint_class(root + "BP_PinballBall")
assert table.get_actor_transform().transform_location(unreal.Vector(0, 100, 0)).z > table.get_actor_location().z
controller = unreal.get_default_object(assets.load_blueprint_class("/Game/Framework/Blueprints/BP_PinballPlayerController"))
for field, action in (("left_flipper_action", "LeftFlipper"), ("right_flipper_action", "RightFlipper"), ("plunger_action", "Plunger")):
    assert controller.get_editor_property(field) == assets.load_asset("/Game/Framework/Input/IA_" + action)
ball_cdo = unreal.get_default_object(table.get_editor_property("ball_class"))
assert isinstance(ball_cdo.root_component, unreal.SphereComponent)
assert ball_cdo.root_component.get_editor_property("body_instance").get_editor_property("use_ccd")
ball_channel = ball_cdo.root_component.get_collision_object_type()
for actor in actors:
    if isinstance(actor, unreal.StaticMeshActor):
        assert actor.static_mesh_component.get_collision_response_to_channel(ball_channel) == unreal.CollisionResponseType.ECR_BLOCK
tuning = table.get_editor_property("tuning")
assert tuning.get_editor_property("trap_window_seconds") == 10
assert tuning.get_editor_property("exempt_launch_and_capture_areas")
assert tuning.get_editor_property("physical_material") == assets.load_asset(root + "PM_Pinball")
dependencies = unreal.AssetRegistryDependencyOptions(include_hard_package_references=True, include_soft_package_references=True)
registry = unreal.AssetRegistryHelpers.get_asset_registry()
for name in ("BP_PinballBall", "BP_Flipper", "BP_Plunger", "BP_Bumper", "BP_Drain", "DA_PinballTuning"):
    assert not any(str(path).startswith("/Game/Tests/") for path in registry.get_dependencies(root + name, dependencies)), name
unreal.log("PINBALL_PHASE2_ASSET_VALIDATION_PASSED")
