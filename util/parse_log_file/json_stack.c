#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json_stack.h"

#define STACK_INIT_CAP 32

json_stack_t* stack_init(int initial_capacity){
    json_stack_t *st = malloc(sizeof(json_stack_t));
    if(!st) return NULL;
    st->depth=0;
    if(initial_capacity<=0){
        initial_capacity=STACK_INIT_CAP;
    }
    st->capacity=initial_capacity;
    st->items=malloc(sizeof(char*)*initial_capacity);
    if(!st->items){
        free(st);
        return NULL;
    }
    return st;
}

int stack_push(json_stack_t *st,const char *value){
    if(!st||!value)return -1;
    if(st->depth>=st->capacity){
        int new_capacity=st->capacity*2;
        char **new_items=realloc(st->items,new_capacity*sizeof(char*));
        if(!new_items){
            return -1;
        }
        st->items=new_items;
        st->capacity=new_capacity;
    }
    st->items[st->depth]=strdup(value);
    st->depth++;
    return 1;
}

char *stack_pop(json_stack_t *st){
    if(!st||st->depth<=0){
        return NULL;
    }
    st->depth--;
    char *val=st->items[st->depth];
    st->items[st->depth]=NULL;

    return val;
}

void stack_free(json_stack_t *s)
{
    if (!s) return;

    for (int i = 0; i < s->depth; i++) {
        free(s->items[i]);
    }

    free(s->items);
    free(s);
}