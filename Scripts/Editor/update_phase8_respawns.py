"""Migrate existing Assault content to per-slot three-second respawns without rewriting cabinet maps."""
import unreal
assets = unreal.EditorAssetLibrary
base = "/Game/Minigames/AlienAssault/"
definition = assets.load_asset(base + "Data/DA_MG_AlienAssault")
assert definition
definition.set_editor_property("metric_bounds", {"EnemiesDestroyed": 120.0, "SurvivalSeconds": 30.0})
definition.set_editor_property("maximum_objectives", 120)
definition.set_editor_property("instructions", "LEFT/RIGHT moves your ship along the bottom. Hold SPACE to fire upward. Each destroyed alien returns to its formation slot after three active seconds. Dodge incoming shots. Three lives; a flashing shield protects each respawn for two seconds. Survive 30 active seconds. Four kills and 10 seconds earn 2,000; 28 kills and 30 seconds earn 10,000. ESCAPE pauses.")
assert assets.save_loaded_asset(definition, only_if_is_dirty=False)
root = assets.load_asset(base + "Blueprints/BP_AlienAssaultRuntime")
assert root
unreal.BlueprintEditorLibrary.compile_blueprint(root)
assert assets.save_loaded_asset(root, only_if_is_dirty=False)
unreal.log("PINBALL_ASSAULT_RESPAWN_ASSETS_UPDATED")
