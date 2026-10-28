// Registers the Godot classes; game rules stay in gameplay.
#include "scene.hpp"

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

namespace {

void initialize(godot::ModuleInitializationLevel level) {
  if (level != godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
    return;
  }
  GDREGISTER_CLASS(neocity::AscentScene);
}

void uninitialize(godot::ModuleInitializationLevel /*level*/) {}

} // namespace

/// Entry symbol named by neocity.gdextension.
extern "C" GDExtensionBool GDE_EXPORT
neocity_init(GDExtensionInterfaceGetProcAddress get_proc_address,
             GDExtensionClassLibraryPtr library,
             GDExtensionInitialization *initialization) {
  const godot::GDExtensionBinding::InitObject init(get_proc_address, library,
                                                   initialization);
  init.register_initializer(initialize);
  init.register_terminator(uninitialize);
  init.set_minimum_library_initialization_level(
      godot::MODULE_INITIALIZATION_LEVEL_SCENE);
  return init.init();
}
