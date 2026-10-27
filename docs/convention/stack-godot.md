# Godot stack

The Godot form of naming, scene and resource layout, GDScript comments, and import checks.

Checked against the Godot 4.7 documentation linked below. The project's Godot release and Godot-side scripting language remain unselected.

## Naming and resources

- Godot scene, resource, and folder names use `snake_case`; scene node names use `PascalCase`.
- Keep a scene's scripts and dedicated resources beside it within its owning domain. Shared resources sit in the nearest folder of their owner that serves their consumers.
- Commit authored scenes, resources, source assets, and their resource-reference metadata. Exclude `.godot/` and generated `*.translation` files. Use an empty `.gdignore` in non-resource directories inside the Godot project that the importer must skip.
- The first runnable project selects the location of `project.godot`; this document does not create a project root or prescribe a Rust binding.

These choices were checked against Godot 4.7's [project organization](https://docs.godotengine.org/en/4.7/tutorials/best_practices/project_organization.html) and [version control](https://docs.godotengine.org/en/4.7/tutorials/best_practices/version_control_systems.html) documentation.

## GDScript, when used

- Classes and enum types use `PascalCase`; functions, signals, variables, and parameters use `snake_case`; constants and enum members use `CONSTANT_CASE`.
- Indent with tabs. Use `##` for declaration documentation and `#` for ordinary comments.

Syntax and naming were checked against the Godot 4.7 [GDScript style guide](https://docs.godotengine.org/en/4.7/tutorials/scripting/gdscript/gdscript_styleguide.html). These rules apply if GDScript is introduced; they do not select it over Rust or another supported language.

## Import checks

With the first Godot project, add `godot --headless --path <project-root> --import` to `make build`, using the selected project path and pinned engine. Integrate the Rust build in dependency order once the integration mechanism is chosen. Resource import is a build check; choose a test runner and export targets with the behavior they will verify.
