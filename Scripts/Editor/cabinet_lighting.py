"""Author cabinet-local fill and gentle grounding shadows for readable pinball play."""
import unreal

AUTHORING_TAG = unreal.Name("Phase3Authoring")
INTERIOR_TAG = unreal.Name("CabinetInteriorLighting")


def apply_cabinet_lighting(table):
    """Update only generated lights/exposure, preserving the cabinet's geometry and references."""
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    generated = [actor for actor in actors.get_all_level_actors() if AUTHORING_TAG in actor.tags]
    transform = table.get_actor_transform()

    def spawn_light(cls, label, position, rotation):
        """Create a tagged presentation-only light for idempotent cabinet authoring."""
        actor = actors.spawn_actor_from_class(cls, position, rotation)
        assert actor, label
        actor.set_actor_label(label)
        actor.set_editor_property("tags", [AUTHORING_TAG])
        return actor

    key_rotation = unreal.MathLibrary.compose_rotators(
        unreal.Rotator(pitch=-84, yaw=-35), table.get_actor_rotation())
    keys = [actor for actor in generated if isinstance(actor, unreal.DirectionalLight)]
    key = keys[0] if keys else spawn_light(
        unreal.DirectionalLight, "Soft Overhead Light", transform.transform_location(unreal.Vector(0, 550, 1500)), key_rotation)
    key.set_actor_rotation(key_rotation, False)
    key.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    key.light_component.set_editor_property("intensity", 2.5)
    key.light_component.set_editor_property("light_source_angle", 10)
    key.light_component.set_editor_property("shadow_amount", .35)

    skies = [actor for actor in generated if isinstance(actor, unreal.SkyLight)]
    sky = skies[0] if skies else spawn_light(
        unreal.SkyLight, "Fill Light", transform.transform_location(unreal.Vector(0, 550, 1200)), unreal.Rotator())
    sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property("intensity", 1)

    # Shadowless fill approximates light reflected inside the cabinet without requiring Lumen.
    # Keep its influence near the playfield and avoid a second set of moving ball shadows.
    for actor in actors.get_all_level_actors():
        if INTERIOR_TAG in actor.tags:
            actors.destroy_actor(actor)
    for x in (-270, 270):
        for y in (300, 900):
            position = transform.transform_location(unreal.Vector(x, y, 150))
            aim = transform.transform_location(unreal.Vector(0, y, 0))
            light = spawn_light(unreal.RectLight, "Cabinet Interior Fill",
                                position, unreal.MathLibrary.find_look_at_rotation(position, aim))
            light.set_editor_property("tags", [AUTHORING_TAG, INTERIOR_TAG])
            component = light.light_component
            component.set_mobility(unreal.ComponentMobility.MOVABLE)
            component.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
            component.set_editor_property("intensity", 40)
            component.set_editor_property("attenuation_radius", 650)
            component.set_editor_property("source_width", 70)
            component.set_editor_property("source_height", 250)
            component.set_editor_property("cast_shadows", False)
            component.set_editor_property("use_temperature", True)
            component.set_editor_property("temperature", 5000)

    for actor in generated:
        if isinstance(actor, unreal.PostProcessVolume):
            settings = actor.get_editor_property("settings")
            settings.set_editor_property("override_ambient_occlusion_intensity", True)
            settings.set_editor_property("ambient_occlusion_intensity", .25)
            settings.set_editor_property("override_ambient_occlusion_radius", True)
            settings.set_editor_property("ambient_occlusion_radius", 30)
            actor.set_editor_property("settings", settings)
