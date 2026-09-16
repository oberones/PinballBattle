# Phase 3 graybox assets

The cabinet uses Unreal Engine basic cube/cylinder/sphere meshes, default fonts and original
flat-color materials authored by `Scripts/Editor/create_phase3_assets.py`. The layout and
objective preview inserts were created for this project. No paid or third-party art is used.

`/Game/Framework/Pinball/Audio/S_Target`, `S_Bumper` and `S_Lane` are original synthesized
two-partial decaying chimes. Their complete generation algorithm is in the same script;
temporary PCM WAV sources are regenerated under `Saved/Phase3Audio`. The versioned SoundWave
packages contain the imported audio. They do not contain sampled music or commercial effects.

The purple Asteroid Field, Planetary Defense and Alien Assault inserts are nonblocking
placeholders for later objective implementation. They do not currently activate minigames.
