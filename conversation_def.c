#ifndef CONVERSIONATION_DEF_C
#define CONVERSIONATION_DEF_C

/* 
   for now this is extremely simple and as needs evolve we'll update everything
*/

/* allow this to be associated to an actor */
/* NOTE Does not allow conditional dialogue yet. */
/* NOTE Does not allow anything to happen other than dialogue... */
#define CONVERSATION_CURRENT_VERSION (0)
#define MAX_CONVERSATION_CHOICES (16)
#define MAX_CONVERSATION_NODES   (128)
struct conversation_choice {
    string text;
    /* does not count bartering */
    u32    target; /* 0 == END_CONVERSATION */
};
struct conversation_node {
    image_id portrait;
    string   speaker_name;
    string   text;

    struct conversation_choice choices[MAX_CONVERSATION_CHOICES];
};
/* simple? */
struct conversation {
    u32 version
    /* assume 0 is the start of the node always */
    u32 node_count;
    struct conversation_node nodes[MAX_CONVERSATION_NODES];
};

#endif
