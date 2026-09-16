"""Apply the cabinet's lighting revision to the saved map without recreating gameplay actors."""
import os
import sys
import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cabinet_lighting import apply_cabinet_lighting

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level("/Game/Cabinets/AlienInvasion/Maps/L_AlienCabinet")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
tables = [actor for actor in actors.get_all_level_actors() if isinstance(actor, unreal.PinballTable)]
assert len(tables) == 1, "Lighting update requires one explicit cabinet table"
apply_cabinet_lighting(tables[0])
assert levels.save_current_level()
unreal.log("PINBALL_CABINET_LIGHTING_UPDATED")
