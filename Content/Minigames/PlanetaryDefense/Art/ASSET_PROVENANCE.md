# Planetary Defense assets

Created locally for Phase 7 on 2026-09-18. No downloaded or paid assets are required.

- Colony domes, missiles, launcher, reticle, boundaries and blast disks use Unreal's
  `/Engine/BasicShapes` sphere, cone and cube meshes with original arrangements and colors.
- `M_Defense*` materials are original unlit emissive colors authored by
  `Scripts/Editor/create_phase7_assets.py`. They require no textures or external references.
- `DefenseIntercept` is an original 0.14-second, mono 22,050 Hz PCM sound synthesized by that
  script using two sine tones and a decaying envelope. The source WAV is reproducibly generated
  under ignored `Saved/Phase7Audio`; the imported Unreal SoundWave is versioned.
- The HUD uses the shared native presentation widget with a narrow content-configured column.
- No artwork, audio or class/content references come from another minigame.
