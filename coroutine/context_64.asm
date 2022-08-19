
section .text
global asm_switch_context_64

;rsi oldCoroutine
;rdi newCoroutine
asm_switch_context_64:
	;���浱ǰ�߳� �ֳ�
    push rbp
    mov rbp, rsp
    push rdi
    push rsi
    push rbp
    push rcx
    push rdx
    push rax

    mov [rsi + 0], rsp ;oldCoroutine->cur_stack
    ;-------------------------�л���ջ-----------------------
    mov rsp, [rdi + 0] ;newCoroutine->cur_stack
    ;��ԭ���߳� �ֳ�
    pop rax 
    pop rdx
    pop rcx
    pop rbx
    pop rsi
    pop rdi
    pop rbp
    ret			;pop rip