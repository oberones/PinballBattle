"""Validate saved Assault content in a fresh Unreal process, including sibling independence."""
import unreal

assets = unreal.EditorAssetLibrary
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
base = "/Game/Minigames/AlienAssault/"


def load(path):
    """Require a real saved Unreal package."""
    result = assets.load_asset(path)
    assert result, path
    return result


definition = load(base + "Data/DA_MG_AlienAssault")
assert definition.get_editor_property("duration_seconds") == 30
assert definition.get_editor_property("local_lives") == 3
assert not definition.get_editor_property("uses_mouse_aim")
assert {str(k): v for k, v in definition.get_editor_property("metric_bounds").items()} == {"EnemiesDestroyed": 120, "SurvivalSeconds": 30}
for prop in ("map", "runtime_class", "pawn_class", "input_context", "action_input", "hud_class"):
    assert definition.get_editor_property(prop), prop
root = unreal.get_default_object(load(base + "Blueprints/BP_AlienAssaultRuntime").generated_class())
camera = root.get_editor_property("camera")
assert abs(camera.get_editor_property("relative_rotation").yaw - 90) < .01
assert camera.get_editor_property("ortho_near_clip_plane") < 0
assert camera.get_editor_property("override_aspect_ratio_axis_constraint")
for prop in ("enemy_class", "projectile_class", "destruction_sound"):
    assert root.get_editor_property(prop), prop
pawn = unreal.get_default_object(load(base + "Blueprints/BP_AssaultShip").generated_class())
assert pawn.get_editor_property("fire_action") == load(base + "Input/IA_AssaultFire")
threat = unreal.get_default_object(load(base + "Blueprints/BP_AssaultEnemy").generated_class())
cabinet = load("/Game/Cabinets/AlienInvasion/Data/DA_AlienCabinet")
ids = [str(d.get_editor_property("mini_game_id")) for d in cabinet.get_editor_property("mini_games")]
assert set(ids) == {"AsteroidField", "PlanetaryDefense", "AlienAssault"} and len(ids) == len(set(ids))
test = load("/Game/Tests/Data/DA_AssaultTestCabinet")
assert list(test.get_editor_property("mini_games")) == [definition]
rules = test.get_editor_property("scoring_profile").get_editor_property("mini_game_rules")
rule = next(r for r in rules if str(r.get_editor_property("profile_key")) == "AlienAssault")
assert {str(k): v for k, v in rule.get_editor_property("weights").items()} == {"EnemiesDestroyed": 250, "SurvivalSeconds": 100}
assert [str(k) for k in rule.get_editor_property("floor_metrics")] == ["SurvivalSeconds"]
assert levels.load_level(base + "Maps/L_MG_AlienAssault")
roots = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.AlienAssaultRuntime)]
assert len(roots) == 1 and roots[0].get_editor_property("camera")
assert levels.load_level("/Game/Tests/Maps/L_AssaultTest")
probe = next(a for a in actors.get_all_level_actors() if isinstance(a, unreal.AlienAssaultProbe))
assert probe.get_editor_property("objective").get_editor_property("trigger").get_editor_property("definition") == definition
assert levels.load_level("/Game/Cabinets/AlienInvasion/Maps/L_AlienCabinet")
objectives = [a.get_editor_property("trigger") for a in actors.get_all_level_actors() if isinstance(a, unreal.MinigameObjective)]
assert {str(t.get_editor_property("objective_id")) for t in objectives} >= {"AsteroidObjective", "DefenseObjective", "AssaultObjective"}
# Any minigame-owned package may use shared/engine content, but must not reference a sibling folder.
registry = unreal.AssetRegistryHelpers.get_asset_registry()
options = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True)
for path in assets.list_assets(base, recursive=True, include_folder=False):
    for dependency in registry.get_dependencies(path.split(".")[0], options):
        dep = str(dependency)
        assert not dep.startswith("/Game/Minigames/") or dep.startswith(base), (path, dep)
unreal.log("PINBALL_PHASE8_ASSET_VALIDATION_PASSED")
