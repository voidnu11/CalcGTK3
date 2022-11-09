#ifndef _SRC_DOUBLE_STACK_
#define _SRC_DOUBLE_STACK_

typedef struct DoubleStack {
  int capacity;
  double *data;
  int top;
} stack_d;

stack_d *init_stack_d(unsigned int capacity);
void destroy_stack_d(stack_d *s);
int is_empty_stack_d(stack_d *s);
int is_full_stack_d(stack_d *s);
double top_stack_d(stack_d *s);
double pop_stack_d(stack_d *s);
void push_stack_d(stack_d *s, double value);

#endif  // _SRC_DOUBLE_STACK_