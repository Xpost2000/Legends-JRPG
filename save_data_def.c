#ifndef SAVE_DATA_DEF_C
#define SAVE_DATA_DEF_C

/* delta based saving */
/* it's meant to be basically directly serializable as a stream of bytes. */

/*
  However that's a bit painful to keep in memory.
  
  Right now it's planned to be a bunch of fixed size linked list nodes,
  that go in basically a free list.
  
  we only need to write to it, and read from it very sparingly so this is just easier
  all around...
*/

struct save_data {
    /* TODO */
};

#endif
