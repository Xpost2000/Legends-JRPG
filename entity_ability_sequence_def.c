#ifndef ENTITY_ABILITY_SEQUENCE_DEF_C
#define ENTITY_ABILITY_SEQUENCE_DEF_C

#define ENTITY_ABILITY_SEQUENCE_MAX_PARTICIPANTS (32)

/*
  I'm going to just interpret lisp forms.

  It just means I have practically speaking no serialization code and I can entirely
  focus on the scary part which is just the implementation of all this crap.

  The amount of state I'm going to be keeping track of is a bit intimidating.
*/
struct entity_ability_sequence {
    struct lisp_form* actions;
    s32 sequence_action_count;

    /*
     *  runtime data
     */

    entity_id participants[ENTITY_ABILITY_SEQUENCE_MAX_PARTICIPANTS];
};

/*
  NOTE:
  asdlflasdfjasdfl
*/
#if 0
void skip_current_ability_animation(void) {
    for each anim form; {
        if (damage causing) {
            // do all shit
        }

        if (movement causing) {
            // do all shit
        }
    }

    okay done!
}
#endif

#endif
