#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int pos = 0;

// トークンの型を表す値
enum {
  TK_NUM = 256,
  TK_EOF,
};

typedef struct {
  int ty;	// トークンの型
  int val;	// 整数型の場合、その数値
  char *input;	// トークン文字列
} Token;

enum {
  ND_NUM = 256,
};

typedef struct Node{
  int ty;
  struct Node *lhs;
  struct Node *rhs;
  int val;
} Node;

Node* mul();
Node* term();

typedef struct {
  void **data;
  int capacity;
  int len;
} Vector;

Vector *new_vector(){
  Vector *vec = malloc(sizeof(Vector));
  vec->data = malloc(sizeof(void*) * 16);
  vec->capacity = 16;
  vec->len = 0;
  return vec;
}

void vec_push(Vector *vec, void *elem){
  if(vec->capacity == vec->len){
    vec->capacity *= 2;
    vec->data = realloc(vec->data,
                        sizeof(void*) * vec->capacity);
  }
  vec->data[vec->len++] = elem;
}

//====== vector test =======

int expect(int line, int expected, int actual){
  if(expected == actual)
    return;
  fprintf(stderr, "%d: %d expected, but got %d\n",
          line, expected, actual);
  exit(1);
}

void runtest(){
  Vector *vec = new_vector();
  expect(__LINE__, 0, vec->len);

  int i;
  for(i=0; i<100; i++)
    vec_push(vec, (void *)i);

  expect(__LINE__, 100, vec->len);
  expect(__LINE__, 0, (int)vec->data[0]);
  expect(__LINE__, 50, (int)vec->data[50]);
  expect(__LINE__, 99, (int)vec->data[99]);
  
  printf("OK\n");
}

//================

Token tokens[100];

Node *new_node(int ty, Node *lhs, Node *rhs){
  Node *node = malloc(sizeof(Node));
  node->ty = ty;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val){
  Node *node = malloc(sizeof(Node));
  node->ty = ND_NUM;
  node->val = val;
  return node;
}

int consume(int ty){
  if (tokens[pos].ty != ty)
    return 0;
  pos++;
  return 1;
}

Node *add(){
  Node *node = mul();
  
  for(;;){
    if(consume('+'))
      node = new_node('+', node, mul());
    else if(consume('-'))
      node = new_node('-', node, mul());
    else
      return node;
  }
}

Node *mul(){
  Node *node = term();

  for(;;){
    if(consume('*'))
      node = new_node('*', node, term());
    else if(consume('/'))
      node = new_node('/', node, term());
    else
      return node;
  }
}

Node *term(){
  if(consume('(')){
    Node *node = add();
    if(!consume(')'))
      printf("開きカッコに対応する閉じカッコがありません。: %s\n",
            tokens[pos].input);
    return node;
  }

  if(tokens[pos].ty == TK_NUM)
    return new_node_num(tokens[pos++].val);

  printf("数値でも開きかっこでもないトークンです: %s\n",
        tokens[pos].input);
}

void tokenize(char *p){
  int i = 0;
  while(*p){
    if (isspace(*p)){
      p++;
      continue;
    }

    if( *p=='+' || *p=='-' || *p=='*' || *p=='/' ||
        *p=='(' || *p==')' ){
      tokens[i].ty = *p;
      tokens[i].input = p;
      i++;
      p++;
      continue;
    }

    if(isdigit(*p)){
      tokens[i].ty = TK_NUM;
      tokens[i].input = p;
      tokens[i].val = strtol(p, &p, 10);
      i++;
      continue;
    }

    fprintf(stderr, "トークナイズできません: %s\n", p);
    exit(1);
  }

  tokens[i].ty = TK_EOF;
  tokens[i].input = p;
}

void gen(Node *node){
  if(node->ty == ND_NUM){
    printf("  push %d\n", node->val);
    return;
  }
  
  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch(node->ty){
  case '+':
    printf("  add rax, rdi\n");
    break;
  case '-':
    printf("  sub rax, rdi\n");
    break;
  case '*':
    printf("  mul rdi\n");
    break;
  case '/':
    printf("  mov rdx, 0\n");
    printf("  div rdi\n");
  }

  printf("  push rax\n");
}

void error(int i){
  fprintf(stderr, "予期しないトークンです: %s\n",
          tokens[i].input);
  exit(1);
}

int main(int argc, char **argv){
  if (strcmp(argv[1], "-test") == 0){
    runtest();
    return 0;
  }

  if (argc != 2){
    fprintf(stderr, "`引数の個数が正しくありません\n");
    return 1;
  }

  tokenize(argv[1]);  
  Node *node = add();

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  gen(node);

  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}
