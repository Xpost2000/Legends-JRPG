local void handle_specialfx_sequence_action(s32 id) {
    switch (id) {
        case SPECIAL_EFFECT_INVERSION_1: {
            special_effect_start_inversion();
        } break;
        default: {
            _debugprintf("FX does not exist!");
        } break;
    }
}
