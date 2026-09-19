"""Validate saved Defense content in a fresh Unreal process, including sibling independence."""
import unreal

assets = unreal.EditorAssetLibrary
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
base = "/Game/Minigames/PlanetaryDefense/"


def load(path):
    """Require a real saved Unreal package."""
    result = assets.load_asset(path)
    assert result, path
    return result


definition = load(base + "Data/DA_MG_PlanetaryDefense")
assert definition.get_editor_property("duration_seconds") == 30
assert definition.get_editor_property("local_lives") == 0
assert definition.get_editor_property("uses_mouse_aim")
assert {str(k): v for k, v in definition.get_editor_property("metric_bounds").items()} == {"ThreatsDestroyed": 100, "StructuresSurviving": 3}
for prop in ("map", "runtime_class", "pawn_class", "input_context", "action_input", "hud_class"):
    assert definition.get_editor_property(prop), prop
root = unreal.get_default_object(load(base + "Blueprints/BP_PlanetaryDefenseRuntime").generated_class())
assert len(root.get_editor_property("colony_anchors")) == 3
camera = root.get_editor_property("camera")
assert abs(camera.get_editor_property("relative_rotation").yaw - 90) < .01
assert camera.get_editor_property("ortho_near_clip_plane") < 0
assert camera.get_editor_property("override_aspect_ratio_axis_constraint")
for prop in ("threat_class", "colony_class", "interceptor_class", "blast_class", "interception_sound"):
    assert root.get_editor_property(prop), prop
pawn = unreal.get_default_object(load(base + "Blueprints/BP_DefenseAimPawn").generated_class())
assert pawn.get_editor_property("fire_action") == load(base + "Input/IA_DefenseFire")
assert len(pawn.get_editor_property("reticle")) == 4
threat = unreal.get_default_object(load(base + "Blueprints/BP_DefenseThreat").generated_class())
assert abs(threat.get_editor_property("visual").get_editor_property("relative_rotation").pitch + 90) < .01
cabinet = load("/Game/Cabinets/AlienInvasion/Data/DA_AlienCabinet")
ids = [str(d.get_editor_property("mini_game_id")) for d in cabinet.get_editor_property("mini_games")]
assert "AsteroidField" in ids and "PlanetaryDefense" in ids and len(ids) == len(set(ids))
test = load("/Game/Tests/Data/DA_DefenseTestCabinet")
assert list(test.get_editor_property("mini_games")) == [definition]
rules = test.get_editor_property("scoring_profile").get_editor_property("mini_game_rules")
rule = next(r for r in rules if str(r.get_editor_property("profile_key")) == "PlanetaryDefense")
assert {str(k): v for k, v in rule.get_editor_property("weights").items()} == {"ThreatsDestroyed": 250, "StructuresSurviving": 1000}
assert levels.load_level(base + "Maps/L_MG_PlanetaryDefense")
roots = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.PlanetaryDefenseRuntime)]
assert len(roots) == 1 and roots[0].get_editor_property("camera")
assert len(roots[0].get_editor_property("colony_anchors")) == 3
assert levels.load_level("/Game/Tests/Maps/L_DefenseTest")
probe = next(a for a in actors.get_all_level_actors() if isinstance(a, unreal.PlanetaryDefenseProbe))
assert probe.get_editor_property("objective").get_editor_property("trigger").get_editor_property("definition") == definition
assert levels.load_level("/Game/Cabinets/AlienInvasion/Maps/L_AlienCabinet")
objectives = [a.get_editor_property("trigger") for a in actors.get_all_level_actors() if isinstance(a, unreal.MinigameObjective)]
assert {str(t.get_editor_property("objective_id")) for t in objectives} >= {"AsteroidObjective", "DefenseObjective"}
# Any minigame-owned package may use shared/engine content, but must not reference a sibling folder.
registry = unreal.AssetRegistryHelpers.get_asset_registry()
options = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True)
for path in assets.list_assets(base, recursive=True, include_folder=False):
    for dependency in registry.get_dependencies(path.split(".")[0], options):
        dep = str(dependency)
        assert not dep.startswith("/Game/Minigames/") or dep.startswith(base), (path, dep)
unreal.log("PINBALL_PHASE7_ASSET_VALIDATION_PASSED")
