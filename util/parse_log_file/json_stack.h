#ifndef JSON_STACK_H
#define JSON_STACK_H

typedef struct {
    char **items;
    int depth;
    int capacity;
} json_stack_t;

/* init stack */
json_stack_t* stack_init(int initial_capacity);

/* push item (string copy) */
int stack_push(json_stack_t *stack, const char *value);

/* pop item (caller must free return value) */
char* stack_pop(json_stack_t *stack);

/* get top item (no pop) */
char* stack_top(json_stack_t *stack);

/* free stack */
void stack_free(json_stack_t *stack);

#endif