use godot::classes::Input;
use godot::obj::Singleton;
use neocity_ascent_engine::PlayerCommand;

pub(crate) fn sample() -> PlayerCommand {
    let input = Input::singleton();
    PlayerCommand {
        horizontal: input.get_axis("move_left", "move_right"),
        jump_pressed: input.is_action_just_pressed("jump"),
        jump_held: input.is_action_pressed("jump"),
        switch_character: input.is_action_just_pressed("switch_character"),
        pause_pressed: input.is_action_just_pressed("pause"),
        retry_pressed: input.is_action_just_pressed("retry"),
    }
}
