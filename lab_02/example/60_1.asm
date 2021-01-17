.386
text	segment 'code' use16
		assume cs:text, ds:data

begin:	mov ax, data
		mov ds, ax
	
; Основной фрагмент программы
		mov eax, 12345678h ; Загружаем операнд 32 раз
		add eax, 87654321h
		mov dword ptr sum, EAX;	Запишем результат
		
		mov ax, 4c00h
		int 21h
text ends
data segment
	sum dd 0
data ends
	end begin
	