
section .text
global asm_switch_context_64

;rsi oldCoroutine
;rdi newCoroutine
asm_switch_context_64:
	;保存当前线程 现场
    push rbp
    mov rbp, rsp
    push rdi
    push rsi
    push rbp
    push rcx
    push rdx
    push rax

    mov [rsi + 0], rsp ;oldCoroutine->cur_stack
    ;-------------------------切换堆栈-----------------------
    mov rsp, [rdi + 0] ;newCoroutine->cur_stack
    ;还原新线程 现场
    pop rax 
    pop rdx
    pop rcx
    pop rbx
    pop rsi
    pop rdi
    pop rbp
    ret			;pop rip