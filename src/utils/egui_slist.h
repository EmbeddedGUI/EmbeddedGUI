/**
 * @file
 *
 * @brief Single-linked list implementation
 *
 * Single-linked list implementation using inline macros/functions.
 * This API is not thread safe, and thus if a list is used across threads,
 * calls to functions must be protected with synchronization primitives.
 */
#ifndef _EGUI_SLIST_H_
#define _EGUI_SLIST_H_

#include <stdbool.h>
#include <stddef.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

struct _egui_snode
{
    struct _egui_snode *next;
};

typedef struct _egui_snode egui_snode_t;

struct _slist
{
    egui_snode_t *head;
    egui_snode_t *tail;
};

typedef struct _slist egui_slist_t;

#ifndef egui_slist_container_of
#define egui_slist_container_of(ptr, type, member) ((type *)(((const char *)(ptr)) - offsetof(type, member)))
#endif

#define EGUI_SLIST_ENTRY(ptr, type, member) egui_slist_container_of(ptr, type, member)

/**
 * @brief Provide the primitive to iterate on a list
 * Note: the loop is unsafe and thus __sn should not be removed
 *
 * User _MUST_ add the loop statement curly braces enclosing its own code:
 *
 *     EGUI_SLIST_FOR_EACH_NODE(l, n) {
 *         <user code>
 *     }
 *
 * This and other EGUI_SLIST_*() macros are not thread safe.
 *
 * @param __sl A pointer on a egui_slist_t to iterate on
 * @param __sn A egui_snode_t pointer to peek each node of the list
 */
#define EGUI_SLIST_FOR_EACH_NODE(__sl, __sn) for (__sn = egui_slist_peek_head(__sl); __sn != NULL; __sn = egui_slist_peek_next(__sn))

/**
 * @brief Provide the primitive to iterate on a list, from a node in the list
 * Note: the loop is unsafe and thus __sn should not be removed
 *
 * User _MUST_ add the loop statement curly braces enclosing its own code:
 *
 *     EGUI_SLIST_ITERATE_FROM_NODE(l, n) {
 *         <user code>
 *     }
 *
 * Like EGUI_SLIST_FOR_EACH_NODE(), but __dn already contains a node in the list
 * where to start searching for the next entry from. If NULL, it starts from
 * the head.
 *
 * This and other EGUI_SLIST_*() macros are not thread safe.
 *
 * @param __sl A pointer on a egui_slist_t to iterate on
 * @param __sn A egui_snode_t pointer to peek each node of the list
 *             it contains the starting node, or NULL to start from the head
 */
#define EGUI_SLIST_ITERATE_FROM_NODE(__sl, __sn)                                                                                                               \
    for (__sn = __sn ? egui_slist_peek_next_no_check(__sn) : egui_slist_peek_head(__l); __sn != NULL; __sn = egui_slist_peek_next(__sn))

/**
 * @brief Provide the primitive to safely iterate on a list
 * Note: __sn can be removed, it will not break the loop.
 *
 * User _MUST_ add the loop statement curly braces enclosing its own code:
 *
 *     EGUI_SLIST_FOR_EACH_NODE_SAFE(l, n, s) {
 *         <user code>
 *     }
 *
 * This and other EGUI_SLIST_*() macros are not thread safe.
 *
 * @param __sl A pointer on a egui_slist_t to iterate on
 * @param __sn A egui_snode_t pointer to peek each node of the list
 * @param __sns A egui_snode_t pointer for the loop to run safely
 */
#define EGUI_SLIST_FOR_EACH_NODE_SAFE(__sl, __sn, __sns)                                                                                                       \
    for (__sn = (void *)egui_slist_peek_head(__sl), __sns = egui_slist_peek_next(__sn); __sn != NULL; __sn = __sns, __sns = egui_slist_peek_next(__sn))

/*
 * Required function definitions for the list_gen.h interface
 *
 * These are the only functions that do not treat the list/node pointers
 * as completely opaque types.
 */

/**
 * @brief Initialize a list
 *
 * @param list A pointer on the list to initialize
 */
static inline void egui_slist_init(egui_slist_t *list)
{
    list->head = NULL;
    list->tail = NULL;
}

#define EGUI_SLIST_STATIC_INIT(ptr_to_list)                                                                                                                    \
    {                                                                                                                                                          \
        NULL, NULL                                                                                                                                             \
    }

static inline egui_snode_t *_egui_snode_next_peek(egui_snode_t *node)
{
    return node->next;
}

static inline void _egui_snode_next_set(egui_snode_t *parent, egui_snode_t *child)
{
    parent->next = child;
}

static inline void _egui_slist_head_set(egui_slist_t *list, egui_snode_t *node)
{
    list->head = node;
}

static inline void _egui_slist_tail_set(egui_slist_t *list, egui_snode_t *node)
{
    list->tail = node;
}

/**
 * @brief Peek the first node from the list
 *
 * @param list A point on the list to peek the first node from
 *
 * @return A pointer on the first node of the list (or NULL if none)
 */
static inline egui_snode_t *egui_slist_peek_head(egui_slist_t *list)
{
    return list->head;
}

/**
 * @brief Peek the last node from the list
 *
 * @param list A point on the list to peek the last node from
 *
 * @return A pointer on the last node of the list (or NULL if none)
 */
static inline egui_snode_t *egui_slist_peek_tail(egui_slist_t *list)
{
    return list->tail;
}

/*
 * Derived, generated APIs
 */

/**
 * @brief Test if the given list is empty
 *
 * @param list A pointer on the list to test
 *
 * @return a boolean, true if it's empty, false otherwise
 */
static inline bool egui_slist_is_empty(egui_slist_t *list)
{
    return (egui_slist_peek_head(list) == NULL);
}

/**
 * @brief Peek the next node from current node, node is not NULL
 *
 * Faster then egui_slist_peek_next() if node is known not to be NULL.
 *
 * @param node A pointer on the node where to peek the next node
 *
 * @return a pointer on the next node (or NULL if none)
 */
static inline egui_snode_t *egui_slist_peek_next_no_check(egui_snode_t *node)
{
    return _egui_snode_next_peek(node);
}

/**
 * @brief Peek the next node from current node
 *
 * @param node A pointer on the node where to peek the next node
 *
 * @return a pointer on the next node (or NULL if none)
 */
static inline egui_snode_t *egui_slist_peek_next(egui_snode_t *node)
{
    return node != NULL ? egui_slist_peek_next_no_check(node) : NULL;
}

/**
 * @brief Prepend a node to the given list
 *
 * This and other egui_slist_*() functions are not thread safe.
 *
 * @param list A pointer on the list to affect
 * @param node A pointer on the node to prepend
 */
static inline void egui_slist_prepend(egui_slist_t *list, egui_snode_t *node)
{
    _egui_snode_next_set(node, egui_slist_peek_head(list));
    _egui_slist_head_set(list, node);

    if (egui_slist_peek_tail(list) == NULL)
    {
        _egui_slist_tail_set(list, egui_slist_peek_head(list));
    }
}

/**
 * @brief Append a node to the given list
 *
 * This and other egui_slist_*() functions are not thread safe.
 *
 * @param list A pointer on the list to affect
 * @param node A pointer on the node to append
 */
static inline void egui_slist_append(egui_slist_t *list, egui_snode_t *node)
{
    _egui_snode_next_set(node, NULL);

    if (egui_slist_peek_tail(list) == NULL)
    {
        _egui_slist_tail_set(list, node);
        _egui_slist_head_set(list, node);
    }
    else
    {
        _egui_snode_next_set(egui_slist_peek_tail(list), node);
        _egui_slist_tail_set(list, node);
    }
}

/**
 * @brief Append a list to the given list
 *
 * Append a singly-linked, NULL-terminated list consisting of nodes containing
 * the pointer to the next node as the first element of a node, to @a list.
 * This and other egui_slist_*() functions are not thread safe.
 *
 * FIXME: Why are the element parameters void *?
 *
 * @param list A pointer on the list to affect
 * @param head A pointer to the first element of the list to append
 * @param tail A pointer to the last element of the list to append
 */
static inline void egui_slist_append_list(egui_slist_t *list, void *head, void *tail)
{
    if (egui_slist_peek_tail(list) == NULL)
    {
        _egui_slist_head_set(list, (egui_snode_t *)head);
    }
    else
    {
        _egui_snode_next_set(egui_slist_peek_tail(list), (egui_snode_t *)head);
    }
    _egui_slist_tail_set(list, (egui_snode_t *)tail);
}

/**
 * @brief merge two slists, appending the second one to the first
 *
 * When the operation is completed, the appending list is empty.
 * This and other egui_slist_*() functions are not thread safe.
 *
 * @param list A pointer on the list to affect
 * @param list_to_append A pointer to the list to append.
 */
static inline void egui_slist_merge_slist(egui_slist_t *list, egui_slist_t *list_to_append)
{
    egui_snode_t *head, *tail;
    head = egui_slist_peek_head(list_to_append);
    tail = egui_slist_peek_tail(list_to_append);
    egui_slist_append_list(list, head, tail);
    egui_slist_init(list_to_append);
}

/**
 * @brief Insert a node to the given list
 *
 * This and other egui_slist_*() functions are not thread safe.
 *
 * @param list A pointer on the list to affect
 * @param prev A pointer on the previous node
 * @param node A pointer on the node to insert
 */
static inline void egui_slist_insert(egui_slist_t *list, egui_snode_t *prev, egui_snode_t *node)
{
    if (prev == NULL)
    {
        egui_slist_prepend(list, node);
    }
    else if (_egui_snode_next_peek(prev) == NULL)
    {
        egui_slist_append(list, node);
    }
    else
    {
        _egui_snode_next_set(node, _egui_snode_next_peek(prev));
        _egui_snode_next_set(prev, node);
    }
}

/**
 * @brief Fetch and remove the first node of the given list
 *
 * List must be known to be non-empty.
 * This and other egui_slist_*() functions are not thread safe.
 *
 * @param list A pointer on the list to affect
 *
 * @return A pointer to the first node of the list
 */
static inline egui_snode_t *egui_slist_get_not_empty(egui_slist_t *list)
{
    egui_snode_t *node = egui_slist_peek_head(list);

    _egui_slist_head_set(list, _egui_snode_next_peek(node));
    if (egui_slist_peek_tail(list) == node)
    {
        _egui_slist_tail_set(list, egui_slist_peek_head(list));
    }

    return node;
}

/**
 * @brief Fetch and remove the first node of the given list
 *
 * This and other egui_slist_*() functions are not thread safe.
 *
 * @param list A pointer on the list to affect
 *
 * @return A pointer to the first node of the list (or NULL if empty)
 */
static inline egui_snode_t *egui_slist_get(egui_slist_t *list)
{
    return egui_slist_is_empty(list) ? NULL : egui_slist_get_not_empty(list);
}

/**
 * @brief Remove a node
 *
 * This and other egui_slist_*() functions are not thread safe.
 *
 * @param list A pointer on the list to affect
 * @param prev_node A pointer on the previous node
 *        (can be NULL, which means the node is the list's head)
 * @param node A pointer on the node to remove
 */
static inline void egui_slist_remove(egui_slist_t *list, egui_snode_t *prev_node, egui_snode_t *node)
{
    if (prev_node == NULL)
    {
        _egui_slist_head_set(list, _egui_snode_next_peek(node));

        /* Was node also the tail? */
        if (egui_slist_peek_tail(list) == node)
        {
            _egui_slist_tail_set(list, egui_slist_peek_head(list));
        }
    }
    else
    {
        _egui_snode_next_set(prev_node, _egui_snode_next_peek(node));

        /* Was node the tail? */
        if (egui_slist_peek_tail(list) == node)
        {
            _egui_slist_tail_set(list, prev_node);
        }
    }

    _egui_snode_next_set(node, NULL);
}

/**
 * @brief Find and remove a node from a list
 *
 * This and other egui_slist_*() functions are not thread safe.
 *
 * @param list A pointer on the list to affect
 * @param node A pointer on the node to remove from the list
 *
 * @return true if node was removed
 */
static inline bool egui_slist_find_and_remove(egui_slist_t *list, egui_snode_t *node)
{
    egui_snode_t *prev = NULL;
    egui_snode_t *test;

    for (test = egui_slist_peek_head(list); test != NULL; test = egui_slist_peek_next(test))
    {
        if (test == node)
        {
            egui_slist_remove(list, prev, node);
            return true;
        }

        prev = test;
    }

    return false;
}

/**
 * @brief get list size
 *
 * @param list A pointer on the list to affect
 *
 * @return the list size
 */
static inline int egui_slist_size(egui_slist_t *list)
{
    int cnt = 0;
    egui_snode_t *test;

    for (test = egui_slist_peek_head(list); test != NULL; test = egui_slist_peek_next(test))
    {
        cnt++;
    }

    return cnt;
}

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_SLIST_H_ */