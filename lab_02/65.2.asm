mov gdt_data.lim, 0FFFFh
mov gdt_code.lim, 0FFFFh
mov gdt_stack.lim, 0FFFFh
mov gdt_screen.lim, 0FFFFh

push DS
pop DS
push SS
pop SS
push ES
pop ES

db 0EAh
dw offset go
dw 16

go:
	mov EAX, CR0
	and EAX, 0FFFFFFFEh
	mov CR0, EAX
	db 0EAh
	dw offset return
	dw text
	
return:
	mov AX, data
	mov DS, AX
	mov AX, stk
	mov SS, AX
	sti
