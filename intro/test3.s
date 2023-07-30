.intel_syntax noprefix
.globl plus, main

plus:
    add rsi, rdi /* rsi + rsi -> rsi */
    mov rax, rsi /* movはmoveの略だがrsiからraxにコピーしている */
    ret

main:
    mov rdi, 3 /* RDI = 第一引数 */
    mov rsi, 4 /* RSI = 第二引数 */
    call plus  /* plusの中でraxに代入しているのでそれがmainの返り値になる */
    ret
