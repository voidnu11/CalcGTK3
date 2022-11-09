#include "calculator.h"

stack_d *operands = NULL;
static double operand = (unsigned)~0;
static int error = 0;

int need_release(double put, double top) {
  return put - (long)put <= top - (long)top;
}

void infix_to_postfix(char *infix, char *postfix) {
  memset(postfix, '\0', 1024);
  preformat_infix_string(infix);
  stack_d *operators = init_stack_d(256);
  char *tmp_str = malloc(256);
  int idx = 0;
  while (1) {
    // write operand
    memset(tmp_str, '\0', 256);
    int tmp_idx = 0;
    while (*infix && strchr("-0123456789.x", *infix) != NULL)
      tmp_str[tmp_idx++] = *(infix++);
    append_str_to_postfix(postfix, &idx, tmp_str);

    if (!*infix) {
      while (!is_empty_stack_d(operators))
        append_operator_to_postfix(postfix, &idx, operators);
      break;
    }

    double masked = mask_operator(*infix), buf = 0;
    if ((int)masked == CBT) {
      while (!is_empty_stack_d(operators) &&
             (int)(buf = pop_stack_d(operators)) != OBT) {
        tmp_str[0] = buf;
        tmp_str[1] = '\0';
        append_str_to_postfix(postfix, &idx, tmp_str);
      }
    } else {
      while (!is_empty_stack_d(operators) && (int)masked != OBT &&
             need_release(masked, top_stack_d(operators)))
        append_operator_to_postfix(postfix, &idx, operators);
      push_stack_d(operators, masked);
    }
    infix++;
  }
  destroy_stack_d(operators);
}

void append_operator_to_postfix(char *postfix, int *pos, stack_d *operators) {
  char tmp_str[0];
  tmp_str[0] = (char)pop_stack_d(operators);
  if (tmp_str[0] == OBT)
    return;
  append_str_to_postfix(postfix, pos, tmp_str);
}

void append_char_to_postfix(char *postfix, int *pos, char ch) {
  postfix[(*pos)++] = ch;
  postfix++;
}

void append_str_to_postfix(char *postfix, int *pos, char *str) {
  if (*str && *pos != 0)
    append_char_to_postfix(postfix, pos, ' ');
  for (; *str; str++)
    append_char_to_postfix(postfix, pos, *str);
}

void preformat_infix_string(char *input) {
  char temp[512] = "\0", *output = input;
  size_t pos = 0;
  while (*input++) {
    // rewrite binary minus to tilda
    if (*(input - 1) != '(' && *input == '-')
      *input = '~';
    // masks functions
    if (strchr(FUNCTIONS_CHARS, *(input - 1)) != NULL) {
      char func[8] = "\0";
      size_t end = strchr(input, '(') - input + 1;
      snprintf(func, end, "%s", input - 1);
      temp[pos++] = mask_function(func);
      input += end;
    }
    temp[pos++] = *(input - 1);
  }
  // rewrite input
  strcpy(output, temp);
}

char mask_function(char *function) {
  char mask = '?';
  int num = strstr(FUNCTIONS, function) - FUNCTIONS;
  if (num == 27 && *(function + num + 1) == 'n')
    num = 31;
  switch (strstr(FUNCTIONS, function) - FUNCTIONS) {
  case 0:
    mask = 'c';
    break;
  case 4:
    mask = 's';
    break;
  case 8:
    mask = 't';
    break;
  case 12:
    mask = 'o';
    break;
  case 17:
    mask = 'i';
    break;
  case 22:
    mask = 'a';
    break;
  case 35:
    mask = 'l';
    break;
  case 27:
    mask = 'n';
    break;
  case 30:
    mask = 'q';
    break;
  }
  return mask;
}

double mask_operator(char operator) {
  double mask = UNK;
  switch (operator) {
  case '(':
    mask = OBT + 0 / 10.;
    break;
  case ')':
    mask = CBT + 0 / 10.;
    break;
  case '+':
    mask = ADD + 3 / 10.;
    break;
  case '~':
    mask = SUB + 3 / 10.;
    break;
  case '/':
    mask = DIV + 4 / 10.;
    break;
  case '*':
    mask = MUL + 4 / 10.;
    break;
  case '%':
    mask = MOD + 4 / 10.;
    break;
  case '^':
    mask = POW + 5 / 10.;
    break;
  case 's':
    mask = SIN + 6 / 10.;
    break;
  case 'c':
    mask = COS + 6 / 10.;
    break;
  case 't':
    mask = TAN + 6 / 10.;
    break;
  case 'i':
    mask = ASIN + 6 / 10.;
    break;
  case 'o':
    mask = ACOS + 6 / 10.;
    break;
  case 'a':
    mask = ATAN + 6 / 10.;
    break;
  case 'n':
    mask = LN + 6 / 10.;
    break;
  case 'l':
    mask = LOG + 6 / 10.;
    break;
  case 'q':
    mask = SQRT + 6 / 10.;
    break;
  }
  return mask;
}

void do_math(char operation) {
  double a = pop_stack_d(operands), b = 0;
  if (strchr(BINARY_OP, operation))
    b = pop_stack_d(operands);
  switch (operation) {
  case '+':
    push_stack_d(operands, a + b);
    break;
  case '-':
    push_stack_d(operands, b - a);
    break;
  case '*':
    push_stack_d(operands, a * b);
    break;
  case '/':
    push_stack_d(operands, b / a);
    break;
  case '%':
    push_stack_d(operands, fmod(b, a));
    break;
  case '^':
    push_stack_d(operands, pow(b, a));
    break;
  case 's':
    push_stack_d(operands, sin(a));
    break;
  case 'o':
    push_stack_d(operands, acos(a));
    break;
  case 'c':
    push_stack_d(operands, cos(a));
    break;
  case 'i':
    push_stack_d(operands, asin(a));
    break;
  case 'a':
    push_stack_d(operands, atan(a));
    break;
  case 'l':
    push_stack_d(operands, log10(a));
    break;
  case 't':
    push_stack_d(operands, tan(a));
    break;
  case 'n':
    push_stack_d(operands, log(a));
    break;
  case 'q':
    push_stack_d(operands, sqrt(a));
    break;
  }
}

double calculate(char *rpn, const double *x) {
  char *token, tmp[512];
  operands = init_stack_d(512);
  strcpy(tmp, rpn);
  while ((token = strtok(tmp, " "))) {
    if (strlen(token) == 1 && strchr(OPERATORS, *token)) {
      do_math(*token);
    } else {
      sscanf(token, "%lf", &operand);
      if (*token == 'x' && x)
        operand = *x;
      push_stack_d(operands, operand);
    }
  }
  operand = pop_stack_d(operands);
  destroy_stack_d(operands);
  strcpy(rpn, tmp);
  return operand;
}
