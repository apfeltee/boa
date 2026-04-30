
    #define LIT_GROW_ARRAY(state, previous, type, oldcount, count) (type*)lit_reallocate(state, previous, sizeof(type) * (oldcount), sizeof(type) * (count))

    #define LIT_FREE_ARRAY(state, type, pointer, oldcount) lit_reallocate(state, pointer, sizeof(type) * (oldcount), 0)

    #define LIT_ALLOCATE(state, type, count) (type*)lit_reallocate(state, NULL, 0, sizeof(type) * (count))
    #define LIT_FREE(state, type, pointer) lit_reallocate(state, pointer, sizeof(type), 0)

        array->values = LIT_GROW_ARRAY(state, array->values, uint8_t, oldcapacity, array->capacity);

