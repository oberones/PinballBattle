"""Alien cabinet release tuning: two clear feeds over the flippers, away from the drain gap."""
import unreal

TABLE_BLUEPRINT = "/Game/Cabinets/AlienInvasion/Blueprints/BP_AlienTable"
TABLE_MAPS = (
    "/Game/Cabinets/AlienInvasion/Maps/L_AlienCabinet",
    "/Game/Tests/Maps/L_TableInteractions",
    "/Game/Tests/Maps/L_TransitionTest",
    "/Game/Tests/Maps/L_AsteroidTest",
    "/Game/Tests/Maps/L_DefenseTest",
)
PRIMARY = unreal.Vector(65, 450, 16)
BACKUP = unreal.Vector(-85, 450, 16)
VELOCITY = unreal.Vector(0, -160, 0)


def configure_return(table):
    """Keep cabinet geometry choices in content, in the table's inclined local frame."""
    table.get_editor_property("primary_return").set_editor_property("relative_location", PRIMARY)
    table.get_editor_property("backup_return").set_editor_property("relative_location", BACKUP)
    table.set_editor_property("return_velocity", VELOCITY)


def validate_return(table):
    """Require saved marker and velocity settings, including existing map instances."""
    for prop, expected in (("primary_return", PRIMARY), ("backup_return", BACKUP)):
        actual = table.get_editor_property(prop).get_editor_property("relative_location")
        assert (actual - expected).length() < .01, (table.get_path_name(), prop, actual)
    assert (table.get_editor_property("return_velocity") - VELOCITY).length() < .01
