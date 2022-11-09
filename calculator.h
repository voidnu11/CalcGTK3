#ifndef SRC_CALCULATOR_H_
#define SRC_CALCULATOR_H_

#include "double_stack.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FUNCTIONS "cos sin tan acos asin atan ln sqrt log"
#define OPERATORS "+-*/^%socialtnq"
#define BINARY_OP "+-*/^%"

#define FUNCTIONS_CHARS "csalt"
#define OPERATORS_CHARS "+~*/^%()socialtnq#"

enum operators_map {
  UNK = '\0',
  OBT = '(',
  ADD = '+',
  SUB = '-',
  DIV = '/',
  MUL = '*',
  POW = '^',
  MOD = '%',
  SIN = 's',
  COS = 'c',
  TAN = 't',
  ASIN = 'i',
  ACOS = 'o',
  ATAN = 'a',
  LN = 'n',
  LOG = 'l',
  SQRT = 'q',
  CBT = ')',
};

void do_math(char operation);
double calculate(char *rpn, const double *x);

char mask_function(char *function);
double mask_operator(char operator);
int need_release(double put, double top);
void preformat_infix_string(char *input);
void infix_to_postfix(char *infix, char *postfix);
void append_operator_to_postfix(char *postfix, int *pos, stack_d *operators);
void append_char_to_postfix(char *postfix, int *pos, char ch);
void append_str_to_postfix(char *postfix, int *pos, char *str);

#endif // SRC_CALCULATOR_H_
