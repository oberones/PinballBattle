"""Trim the cabinet-side return guide end in the cabinet and preserved practice maps."""
import unreal


def fix_map(path):
    """Preserve the flipper-side endpoint and verify the shortened guide ends inside the side rail."""
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    assert levels.load_level(path), path
    # Labels are an editor-only authoring selector, never a gameplay lookup.
    guides = [a for a in actors.get_all_level_actors() if a.get_actor_label() == "Left Return Guide"]
    assert len(guides) == 1, path
    guide = guides[0]
    old_scale = guide.get_actor_scale3d()
    assert abs(old_scale.x - 3.2) < .001 or abs(old_scale.x - 2.5) < .001
    endpoint = guide.get_actor_location() + guide.get_actor_forward_vector() * (old_scale.x * 50)
    guide.set_actor_location(endpoint - guide.get_actor_forward_vector() * 125, False, False)
    guide.set_actor_scale3d(unreal.Vector(2.5, old_scale.y, old_scale.z))
    tip = guide.get_actor_location() + guide.get_actor_forward_vector() * 125
    assert (tip - endpoint).length() < .001
    origin, extent = guide.get_actor_bounds(False)
    # The side rail occupies world X -337.5..-302.5. The end must overlap it without protruding.
    assert -337.5 < origin.x - extent.x < -302.5, (origin, extent)
    assert levels.save_current_level(), path
    unreal.log("RETURN_GUIDE_FIXED " + path + " outer_x=" + str(origin.x - extent.x))


fix_map("/Game/Cabinets/AlienInvasion/Maps/L_AlienCabinet")
fix_map("/Game/Tests/Maps/L_TableInteractions")
