"""Verify saved asteroid content independently in a fresh Unreal process."""
import unreal

assets = unreal.EditorAssetLibrary
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def load(path):
    """Require a real persisted asset package."""
    result = assets.load_asset(path)
    assert result, path
    return result


base = "/Game/Minigames/AsteroidField/"
definition = load(base + "Data/DA_MG_AsteroidField")
assert definition.get_editor_property("duration_seconds") == 30
assert definition.get_editor_property("local_lives") == 3
assert definition.get_editor_property("metric_bounds")["ObjectsDestroyed"] == 100
for prop in ("map", "runtime_class", "pawn_class", "input_context", "action_input", "hud_class"):
    assert definition.get_editor_property(prop), prop
pawn = unreal.get_default_object(load(base + "Blueprints/BP_AsteroidShip").generated_class())
# The cone's +Z nose must point along the pawn's +X thrust and projectile direction.
ship_rotation = pawn.get_editor_property("visual").get_editor_property("relative_rotation")
assert abs(ship_rotation.pitch + 90) < .01 and abs(ship_rotation.yaw) < .01 and abs(ship_rotation.roll) < .01, ship_rotation
for prop in ("left_action", "right_action", "thrust_action", "fire_action"):
    assert pawn.get_editor_property(prop), prop
root = unreal.get_default_object(load(base + "Blueprints/BP_AsteroidFieldRuntime").generated_class())
for prop in ("obstacle_class", "projectile_class", "destruction_sound"):
    assert root.get_editor_property(prop), prop
for path in ("/Game/Tests/Data/DA_AsteroidTestCabinet", "/Game/Cabinets/AlienInvasion/Data/DA_AlienCabinet"):
    cabinet = load(path)
    assert list(cabinet.get_editor_property("mini_games")) == [definition]
    assert cabinet.get_editor_property("development_without_minigames")
    rules = cabinet.get_editor_property("scoring_profile").get_editor_property("mini_game_rules")
    rule = next(r for r in rules if str(r.get_editor_property("profile_key")) == "AsteroidField")
    assert rule.get_editor_property("weights")["ObjectsDestroyed"] == 500
assert levels.load_level(base + "Maps/L_MG_AsteroidField")
roots = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.AsteroidFieldRuntime)]
assert len(roots) == 1 and roots[0].get_editor_property("camera")
assert levels.load_level("/Game/Tests/Maps/L_AsteroidTest")
probe = next(a for a in actors.get_all_level_actors() if isinstance(a, unreal.AsteroidFieldProbe))
assert probe.get_editor_property("objective").get_editor_property("trigger").get_editor_property("definition") == definition
unreal.log("PINBALL_PHASE6_ASSET_VALIDATION_PASSED")
