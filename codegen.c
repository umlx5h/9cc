#include "chibicc.h"

char *cur_func;
int labelseq = 0;
char *argreg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

// Pushes the given node's address to the stack.
void gen_addr(Node *node) {
  if (node->kind == ND_VAR) {
    printf("  lea rax, [rbp-%d]\n", node->var->offset);
    printf("  push rax\n");
    return;
  }

  error("not an lvalue");
}

void load() {
  printf("  pop rax\n");
  printf("  mov rax, [rax]\n");
  printf("  push rax\n");
}

void store() {
  printf("  pop rdi\n");
  printf("  pop rax\n");
  printf("  mov [rax], rdi\n");
  printf("  push rdi\n");
}

// Generate code for a given node.
void gen(Node *node) {
  switch (node->kind) {
  case ND_NUM:
    printf("  push %d\n", node->val);
    return;
  case ND_EXPR_STMT:
    gen(node->lhs);
    printf("  add rsp, 8\n"); // 式の評価結果としてスタックに一つの値が残っているのでポップしておく (rspを加算する)
    return;
  case ND_VAR: // 右辺に変数が現れた時にメモリからレジスタにコピーして1つの値にしてスタックにpush
    gen_addr(node);
    load();
    return;
  case ND_ASSIGN:
    gen_addr(node->lhs);
    gen(node->rhs);
    store();
    return;
  case ND_IF: {
    int seq = labelseq++;
    if (node->els) {
      gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je  .Lelse%d\n", seq);
      gen(node->then);
      printf("  jmp .Lend%d\n", seq);
      printf(".Lelse%d:\n", seq);
      gen(node->els);
      printf(".Lend%d:\n", seq);
    } else {
      gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je  .Lend%d\n", seq);
      gen(node->then);
      printf(".Lend%d:\n", seq);
    }
    return;
  }
  case ND_WHILE: {
    int seq = labelseq++;
    printf(".Lbegin%d:\n", seq);
    gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je  .Lend%d\n", seq);
    gen(node->then);
    printf("  jmp .Lbegin%d\n", seq);
    printf(".Lend%d:\n", seq);
    return;
  }
  case ND_FOR: {
    int seq = labelseq++;
    if (node->init)
      gen(node->init);
    printf(".Lbegin%d:\n", seq);
    if (node->cond) {
      gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je  .Lend%d\n", seq);
    }
    gen(node->then);
    if (node->inc)
      gen(node->inc);
    printf("  jmp .Lbegin%d\n", seq);
    printf(".Lend%d:\n", seq);
    return;
  }
  case ND_BLOCK:
    for (Node *n = node->body; n; n = n->next)
      gen(n);
    return;
  case ND_FUNCALL: {
    int nargs = 0;
    for (Node *arg = node->args; arg; arg = arg->next) {
      gen(arg);
      nargs++;
    }
    for (int i = nargs - 1; i >= 0; i--)
      printf("  pop %s\n", argreg[i]);
    
    // スタックポインタを調整する
    // pushとpopは8バイトRSPを増減させるが、関数呼び出し時にRSPは16の倍数になってないといけない
    // このため16の倍数であるかを確認し、そうでない場合は8の倍数と仮定しRSPを8バイト分押し下げてからcallする
    // We need to align RSP to a 16 byte boundary before
    // calling a function because it is an ABI requirement.
    // RAX is set to 0 for variadic function.
    int seq = labelseq++;
    printf("  mov rax, rsp\n");
    printf("  and rax, 15\n"); // 0x1111 とANDを取り下位4ビットを取り出す
    printf("  jnz .Lcall%d\n", seq); // rspの下位4ビットがゼロではなかったら、スタックポインタの調整をするためジャンプ
    printf("  mov rax, 0\n");
    printf("  call %s\n", node->funcname);
    printf("  jmp .Lend%d\n", seq);
    printf(".Lcall%d:\n", seq);
    printf("  sub rsp, 8\n"); // 8バイト押し下げておく
    printf("  mov rax, 0\n");
    printf("  call %s\n", node->funcname);
    printf("  add rsp, 8\n"); // 戻す
    printf(".Lend%d:\n", seq);
    printf("  push rax\n"); // 関数の返り値をスタックに積む
    return;
  }
  case ND_RETURN:
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  jmp .Lreturn_%s\n", cur_func);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
  case ND_ADD:
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf("  sub rax, rdi\n");
    break;
  case ND_MUL:
    printf("  imul rax, rdi\n");
    break;
  case ND_DIV:
    // RAXに入っている64ビット値を128ビットに伸ばしてRDXとRAXセット
    printf("  cqo\n");
    // RDX+RAXの128ビット整数をRDIで割り、商をRAXに, 余りをRDXにセット
    printf("  idiv rdi\n");
    break;
  case ND_EQ: // ==
    // cmpはフラグレジスタという特殊なレジスタに結果がセットされる
    printf("  cmp rax, rdi\n");
    // フラグレジスタの結果をal (raxの下位8ビット)にコピーする。seteは同じ場合は1が入る (equal)
    printf("  sete al\n");
    // 下位8ビットより左の64ビットの余っている部分をゼロクリアする
    printf("  movzb rax, al\n");
    break;
  case ND_NE: // !=
    printf("  cmp rax, rdi\n");
    printf("  setne al\n"); // 違う場合に1がセットされる (not equal)
    printf("  movzb rax, al\n");
    break;
  case ND_LT: // <
    printf("  cmp rax, rdi\n");
    printf("  setl al\n"); // 小さい場合に1がセットされる (set lighter)
    printf("  movzb rax, al\n");
    break;
  case ND_LE: // <=
    printf("  cmp rax, rdi\n");
    printf("  setle al\n"); // 小さい場合に1がセットされる (set lighter or equal)
    printf("  movzb rax, al\n");
    break;
  }

  printf("  push rax\n");
}

void codegen(Program *prog) {
  // Print out the first half of assembly.
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");

  for (Func *func = prog->funcs; func; func = func->next) {
    // function name label
    printf("%s:\n", func->name);

    cur_func = func->name;

    // Prologue
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n", func->stack_size);

    // Copy argument register to local stack variables.
    int i = 0;
    for (Var *var = func->args; var && i < 6; var = var->next) {
      if (var->offset > 0) {
        printf("  mov [rbp-%d], %s\n", var->offset, argreg[i]);
      }
      i++;
    }

    // Emit code
    for (Node *node = func->node; node; node = node->next)
      gen(node);

    // Epilogue
    printf(".Lreturn_%s:\n", cur_func);
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
  }
}
