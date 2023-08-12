#include "chibicc.h"

void gen(Node *node) {
  if (node->kind == ND_NUM) {
    printf("  push %d\n", node->val);
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

void codegen(Node *node) {
  // Print out the first half of assembly.
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // Traverse the AST to emit assembly.
  gen(node);

  // A result must be at the top of the stack, so pop it
  // to RAX to make it a program exit code.
  printf("  pop rax\n");
  printf("  ret\n");
}
