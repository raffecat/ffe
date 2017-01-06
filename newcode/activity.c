// Activities


// An array of structures.

typedef struct item_array_t {
    int32_t len, cap;
    void* items;
} item_array_t;

void ia_init_empty(item_array_t* ia) {
    ia->len = ia->cap = 0;
    ia->items = 0;
}

void ia_init(item_array_t* ia, size_t elem_size) {
    ia->len = 0;
    ia->cap = CARD_SIZE / elem_size;
    ia->items = malloc( ia->cap * elem_size );
}

#define ELEM(T,P,I) ( ((T*)((P).items))[I] )
#define PUSH(T,P,V) do{ if ((P).len < (P).cap) { ELEM(T,P,(P).len++)=(V); } else { ia_grow(&(P), sizeof(T), &(V)); } while(0)



// Activities are small card-allocated structures that form a stack.

// Some examples of activities:

typedef struct activity_t* activityp;

typedef struct activity_t {
    activityp prev;
} activity_t;


// CommandList: runs a list of pre-parsed commands; has local vars map.
// On start, if verbs are modified (seq.no), re-compiles the command list.

typedef struct action_t {
    // A pre-compiled command action.
} action_t;

typedef struct action_array_t {
    int32_t len, cap;
    action_t* items;
} action_array_t;

typedef struct act_cmd_list_t {
    // An activity that runs a sequence of commands.
    activity_t act;
    action_array_t actions;
    str_array_t lines; // original command lines.
} act_cmd_list_t;


// Iterator: iterate over lines in a cmd_list   (yields strings?)
// Iterator: iterate over contents of an object (yields objects)
// Iterator: iterate over verbs in a verb_set   (yields ?)

// Template render: walks over template items,
//   append literal text to the buffer
//   lift a var, and to_string it
//   if: lift a var, and look up an attribute, and to_string it
//   each: lift a var, and begin a TemplateStringIterator or TemplateObjectIterator
