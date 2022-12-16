/*
  Test to parse for the meta generator
*/

SERIALIZE typedef struct test1 {
    SERIALIZE s32 x;
#if 0
    SERIALIZE(1,2,3,4,5)s32 z;
#endif
    SERIALIZE(4 to CURRENT)s32 y;
    s32 z; /* runtime data */
};

SERIALIZE struct test {
    SERIALIZE test1 id;
    SERIALIZE s32 x;
    SERIALIZE char name[255];
};

SERIALIZE struct sad UNPACK_INTO(struct test) {
    SERIALIZE s32 x UNPACK_INTO(x);
    SERIALIZE char name[255] UNPACK_INTO(name);
};
