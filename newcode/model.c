// Object Model

// This implements Objects that are containers for other Objects.
// Since references are allowed, Objects have a ref count and remain behind
// as a tombstone after they are destroyed.
// Objects have a list of active iterators, but the iterators themselves
// are Activity frames that exist outside of the object model.

typedef struct activity_t* activityp;

typedef struct activity_array_t {
    int32_t len, cap;
    struct activity_t* items;
} activity_array_t;

// Iterator Interface

// Is there a generic interface? What for?
// Instead can we use specific activity frames for this?

typedef struct iterator_t* iteratorp;
enum iterator_ops {
    iter_close,
    iter_
};
typedef struct iterator_t {
    int (*next)(iteratorp self);
    int (*ctrl)(iteratorp self, enum iterator_ops op);
} iterator_t;

// Contents Iterator

// What can we do with a contents iterator?
// 1. just build a list as a snapshot.
// 2. build a next()-able walker.

typedef struct obj_iter_t* obj_iterp;

typedef struct obj_iter_t {
    activity_t act;
    objectp obj; // holds a ref.
    int32_t pos;
} obj_iter_t;

// Get the next matching object from the iterator.
objectp obj_iter_next(obj_iterp self) {
}

typedef struct object_array_t {
    int32_t len, cap;
    struct object_t* items;
} object_array_t;

typedef struct object_t {
    int32_t refs;
    activity_array_t iterators;
} object_t;