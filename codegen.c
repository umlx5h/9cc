#include "chibicc.h"

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
  case ND_IF:
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .L1\n");
    gen(node->rhs);
    printf(".L1:\n");
    return;
    
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
  case ND_RETURN:
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  jmp .Lreturn\n");
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
  case ND_EQ:
    // cmpはフラグレジスタという特殊なレジスタに結果がセットされる
    printf("  cmp rax, rdi\n");
    // フラグレジスタの結果をal (raxの下位8ビット)にコピーする。seteは同じ場合は1が入る (equal)
    printf("  sete al\n");
    // 下位8ビットより左の64ビットの余っている部分をゼロクリアする
    printf("  movzb rax, al\n");
    break;
  case ND_NE:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n"); // 違う場合に1がセットされる (not equal)
    printf("  movzb rax, al\n");
    break;
  case ND_LT:
    printf("  cmp rax, rdi\n");
    printf("  setl al\n"); // 小さい場合に1がセットされる (set lighter)
    printf("  movzb rax, al\n");
    break;
  case ND_LE:
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
  printf("main:\n");

  // Prologue
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, %d\n", prog->stack_size);

  // Emit code
  for (Node *node = prog->node; node; node = node->next)
    gen(node);

  // Epilogue
  printf(".Lreturn:\n");
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
}
