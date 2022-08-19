.code32
section .data
	stack_offset dw 0	;Coroutine.cur_stack���� ��ƫ��

section .text
global asm_switch_context_32

;rsi oldCoroutine
;rdi newCoroutine
asm_switch_context_32:
	;���浱ǰ�߳� �ֳ�
    push ebp
    mov ebp, esp
    push edi
    push esi
    push ebp
    push ecx
    push edx
    push eax

    mov [esi + stack_offset], esp ;oldCoroutine->cur_stack
    ;-------------------------�л���ջ-----------------------
    mov esp, [edi + stack_offset] ;newCoroutine->cur_stack
    ;��ԭ���߳� �ֳ�
    pop eax 
    pop edx
    pop ecx
    pop ebx
    pop esi
    pop edi
    pop ebp
    ret				;pop eip

