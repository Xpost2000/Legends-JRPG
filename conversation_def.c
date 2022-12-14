#ifndef CONVERSIONATION_DEF_C
#define CONVERSIONATION_DEF_C

/* 
   for now this is extremely simple and as needs evolve we'll update everything
*/

/* allow this to be associated to an actor */
/* NOTE Does not allow conditional dialogue yet. */
/* NOTE Does not allow anything to happen other than dialogue... */
#define CONVERSATION_CURRENT_VERSION (0)
#define MAX_CONVERSATION_CHOICES (8)
#define MAX_CONVERSATION_NODES   (128)
struct conversation_choice {
    string text;
    /* does not count bartering */
    s32    target; /* 0 == END_CONVERSATION */

    /* will override target! */
    string script_code;
};
struct conversation_node {
    s32      id;
    string   speaker_name;
    string   text;

    s32 choice_count;
    struct conversation_choice choices[MAX_CONVERSATION_CHOICES];

    /* use if no choices, and using the default continue option */
    s32 target;

    /* will override target! */
    /* note you cannot have script + choices. */
    string script_code;
};
/* simple? */
struct conversation {
    u32 version;
    /* assume 0 is the start of the node always */
    u32 node_count;
    struct conversation_node nodes[MAX_CONVERSATION_NODES];
};

/* code construction utils, not needed for runtime... */
struct conversation_node* conversation_push_node(struct conversation* c, string speaker_name, string text, s32 target, s32 id) {
    struct conversation_node* n = &c->nodes[c->node_count++];
    n->id           = id;
    n->speaker_name = speaker_name;
    n->text         = text;
    n->target       = target;
    return n;
}
struct conversation_choice* conversation_push_choice(struct conversation_node* c, string text, s32 target) {
    struct conversation_choice* choice = &c->choices[c->choice_count++];
    choice->text   = text;
    choice->target = target;
    return choice;
}

void dialogue_ui_set_override_next_target(s32 new_override);

#endif
