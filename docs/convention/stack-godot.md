# Godot stack

The Godot form of naming, scene and resource layout, GDScript comments, and import checks.

Checked against the Godot 4.7 documentation linked below. The bootstrap host uses the standard Godot 4.7.2 editor, pinned by `GODOT_VERSION` in `Makefile`. GDScript is used for verification harnesses only. The `gameplay` library and the `neocity` presentation extension are C++ libraries connected through godot-cpp GDExtension; Godot supplies host and canvas services. A standalone production graphics backend remains open.

## Naming and resources

- Godot scene, resource, and folder names use `snake_case`; scene node names use `PascalCase`.
- Keep a scene's scripts and dedicated resources beside it in its own folder: `project/levels/<level>/` for a main-game level, `project/experiment/<id>/` for a developer-page experiment. Shared resources sit in the nearest folder of their owner that serves their consumers.
- Commit authored scenes, resources, source assets, and their resource-reference metadata. Exclude `.godot/` and generated `*.translation` files. Use an empty `.gdignore` in non-resource directories inside the Godot project that the importer must skip.
- The Godot project is `project/`: `project/project.godot` starts the C++ AscentScene in `project/levels/ascent/ascent.tscn`. The scene uses the `neocity` C++ extension, which `project/bin/neocity.gdextension` loads from beside it. Its Godot canvas adapter does not select a standalone production graphics backend.

These choices were checked against Godot 4.7's [project organization](https://docs.godotengine.org/en/4.7/tutorials/best_practices/project_organization.html) and [version control](https://docs.godotengine.org/en/4.7/tutorials/best_practices/version_control_systems.html) documentation.

## GDScript, when used

- Classes and enum types use `PascalCase`; functions, signals, variables, and parameters use `snake_case`; constants and enum members use `CONSTANT_CASE`.
- Indent with tabs. Use `##` for declaration documentation and `#` for ordinary comments.

Syntax and naming were checked against the Godot 4.7 [GDScript style guide](https://docs.godotengine.org/en/4.7/tutorials/scripting/gdscript/gdscript_styleguide.html). These rules govern the existing GDScript startup harness and any later scoped GDScript; the harness does not select GDScript for custom engine or visual implementation.

## Import checks

`make build` verifies the pinned Godot version and runs `godot --headless --path project --import`. `make test` imports first, parses `project/tests/godot_startup.gd`, and observes the configured main scene through the native startup harness; both successful process status and its readiness report are required. `make ci` runs lint and format followed by these import/startup checks.

Build the C++ extension library before Godot import, as `make build` does. `make test` also runs the gameplay unit tests and actual move/jump/retry input checks against the configured C++ scene. Resource import and scene readiness alone do not verify game rules or visual behavior. Select export targets and further test coverage with the behavior they will verify.
