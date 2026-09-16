"""Read Phase 3 content back in a fresh process and verify explicit inventory and isolation."""
import unreal

assets = unreal.EditorAssetLibrary
framework = "/Game/Framework/Pinball/"
cabinet = "/Game/Cabinets/AlienInvasion/"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level(cabinet + "Maps/L_AlienCabinet")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()


def instances(cls):
    """Filter editor actors by native type solely for saved-content validation."""
    return [actor for actor in actors if isinstance(actor, cls)]


assert len(instances(unreal.PinballTable)) == 1
table = instances(unreal.PinballTable)[0]
assert table.get_editor_property("require_complete_inventory")
assert len(instances(unreal.PinballBall)) == 0
for field, cls, count in (("targets", unreal.PinballScoringTarget, 4), ("lanes", unreal.PinballLane, 2),
                          ("bumpers", unreal.PinballBumper, 3)):
    referenced = table.get_editor_property(field)
    assert len(referenced) == count and set(referenced) == set(instances(cls)), field
    for actor in referenced:
        feedback = actor.get_editor_property("feedback")
        assert feedback.get_editor_property("hit_sound"), actor.get_actor_label()
        assert actor.get_editor_property("flash"), actor.get_actor_label()
assert len(instances(unreal.PinballDrain)) == 1
assert table.get_editor_property("drain") == instances(unreal.PinballDrain)[0]
assert len(instances(unreal.PinballFlipper)) == 2
assert table.get_editor_property("primary_return").get_editor_property("relative_location") != table.get_editor_property("backup_return").get_editor_property("relative_location")
assert table.get_editor_property("escape_bounds").get_unscaled_box_extent().y > 600
assert table.get_editor_property("tuning").get_editor_property("trap_window_seconds") == 10
for name in ("BP_ScoringTarget", "BP_Lane", "BP_Bumper"):
    blueprint = assets.load_asset(framework + name)
    assert blueprint and blueprint.generated_class(), name
for name in ("S_Target", "S_Bumper", "S_Lane"):
    audio = assets.load_asset(framework + "Audio/" + name)
    assert audio and audio.get_editor_property("duration") > .1, name
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
mode = world.get_world_settings().get_editor_property("default_game_mode")
assert mode == assets.load_blueprint_class("/Game/Tests/Blueprints/BP_PracticeGameMode")
assert not unreal.get_default_object(assets.load_blueprint_class("/Game/Framework/Blueprints/BP_PinballGameMode")).get_editor_property("practice_mode")
assert len([a for a in instances(unreal.TextRenderActor) if "PREVIEW" in str(a.get_component_by_class(unreal.TextRenderComponent).get_editor_property("text"))]) == 3
registry = unreal.AssetRegistryHelpers.get_asset_registry()
options = unreal.AssetRegistryDependencyOptions(include_hard_package_references=True, include_soft_package_references=True)
for path in (framework + "BP_ScoringTarget", framework + "BP_Lane", framework + "BP_Bumper", cabinet + "Blueprints/BP_AlienTable"):
    dependencies = registry.get_dependencies(path, options)
    assert not any(str(dep).startswith("/Game/Tests/") for dep in dependencies), path
    if path.startswith(framework):
        assert not any(str(dep).startswith(cabinet) for dep in dependencies), path
unreal.log("PINBALL_PHASE3_ASSET_VALIDATION_PASSED")
