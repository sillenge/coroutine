.code32
section .data
	stack_offset dw 0	;Coroutine.cur_stack变量 的偏移

section .text
global asm_switch_context_32

;rsi oldCoroutine
;rdi newCoroutine
asm_switch_context_32:
	;保存当前线程 现场
    push ebp
    mov ebp, esp
    push edi
    push esi
    push ebp
    push ecx
    push edx
    push eax

    mov [esi + stack_offset], esp ;oldCoroutine->cur_stack
    ;-------------------------切换堆栈-----------------------
    mov esp, [edi + stack_offset] ;newCoroutine->cur_stack
    ;还原新线程 现场
    pop eax 
    pop edx
    pop ecx
    pop ebx
    pop esi
    pop edi
    pop ebp
    ret				;pop eip

