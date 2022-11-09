#include "double_stack.h"

#include <stdlib.h>

int is_full_stack_d(stack_d *s) { return s->top == s->capacity - 1; }
int is_empty_stack_d(stack_d *s) { return s->top == -1; }

stack_d *init_stack_d(unsigned int capacity) {
  stack_d *s = (stack_d *)malloc(sizeof(stack_d));
  s->capacity = capacity;
  s->top = -1;
  s->data = (double *)malloc(s->capacity * sizeof(double));
  return s;
}

void destroy_stack_d(stack_d *s) {
  free(s->data);
  free(s);
}

double top_stack_d(stack_d *s) {
  return !is_empty_stack_d(s) ? s->data[s->top] : ((unsigned)~0);
}

double pop_stack_d(stack_d *s) {
  return !is_empty_stack_d(s) ? s->data[s->top--] : ((unsigned)~0);
}

void push_stack_d(stack_d *s, double value) {
  if (!is_full_stack_d(s))
    s->data[++s->top] = value;
}
