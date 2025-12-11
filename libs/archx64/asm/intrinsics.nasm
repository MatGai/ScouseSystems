%define MAGIC_HIDWORD       0xbaad0000
%define MAGIC_LODWORD       0xdeadc0de  
	
	BITS 64
    DEFAULT REL

	section .text
	align 8

; -----------------------------------------------------------------------------------------------
; uint64_t 
; _scouse_readmsr(
;   _In_  uint32_t,    // rax
;   _Out_ uint32_t*    // rdx
; );
;
; -----------------------------------------------------------------------------------------------


	global _scouse_readmsr
	_scouse_readmsr:

	xor r10d, r10d

	readmsr

	mov r8, cr2
	cmp r8d, MAGIC_LODWORD
	jne readmsr_success

	xor r8, r8
	mov cr2, r8
	mov r10d, 1

readmsr_success:
 
	shl rdx, 32
	or  rax, rdx

	test edx, edx
	jz readmsr_ret

	mov dword [edx], r10d

readmsr_ret:

	ret

; -----------------------------------------------------------------------------------------------
; 
; void 
; _scouse_cpuid( 
;    _In_  unsigned __int32,    // rax 
;    _Out_ unsigned __int32*    // rbx , pointer to buffer { eax, ebx, ecx, edx }
; );                     
;
; -----------------------------------------------------------------------------------------------

	global _scouse_cpuid
	_scouse_cpuid:

	push rbx

	mov  r10, rdx
	mov  eax, ecx
	xor  ecx, ecx
	push rax

	cpuid	

	mov [r10],		eax
	mov [r10 + 4 ], ebx
	mov [r10 + 8 ], ecx
	mov [r10 + 12], edx

	pop rax
	pop rbx

	ret


; -----------------------------------------------------------------------------------------------
;
; unsigned __int64	// rax
; _scouse_readcr0(
;	
; );
;
; -----------------------------------------------------------------------------------------------

	global _scouse_readcr0
	_scouse_readcr0:

	mov rax, cr0
	ret

; -----------------------------------------------------------------------------------------------
;
; unsigned __int64	// rax
; _scouse_writecr0(
;	_In_ unsigned __int64	// rcx
; );
;
; -----------------------------------------------------------------------------------------------

	global _scouse_writecr0
	_scouse_writecr0:

	mov rax, rcx
	mov cr0, rax
	ret

; -----------------------------------------------------------------------------------------------
;
; unsigned __int64	// rax
; _scouse_readcr2(
;	
; );
;
; -----------------------------------------------------------------------------------------------

	global _scouse_readcr2
	_scouse_readcr2:

	mov rax, cr2
	ret

; -----------------------------------------------------------------------------------------------
;
; unsigned __int64	// rax
; _scouse_writecr2(
;	_In_ unsigned __int64	// rcx
; );
;
; -----------------------------------------------------------------------------------------------

	global _scouse_writecr2
	_scouse_writecr2:

	mov rax, rcx
	mov cr2, rax
	ret


; -----------------------------------------------------------------------------------------------
;
; unsigned __int64	// rax
; _scouse_readcr3(
;
; );
;
; -----------------------------------------------------------------------------------------------

	global _scouse_readcr3
	_scouse_readcr3:

	mov rax, cr3
	ret

; -----------------------------------------------------------------------------------------------
;
; unsigned __int64	// rax
; _scouse_writecr3(
;	_In_ unsigned __int64	// rcx
; );
;
; -----------------------------------------------------------------------------------------------

	global _scouse_writecr3
	_scouse_writecr3:

	mov rax, rcx
	mov cr3, rax
	ret

; -----------------------------------------------------------------------------------------------
;
; unsigned __int64	// rax
; _scouse_readcr4(
;
; );
;
; -----------------------------------------------------------------------------------------------

	global _scouse_readcr4
	_scouse_readcr4:

	mov rax, cr4
	ret

; -----------------------------------------------------------------------------------------------
;
; unsigned __int64	// rax
; _scouse_writecr4(
;	_In_ unsigned __int64	// rcx
; );
;
; -----------------------------------------------------------------------------------------------

	global _scouse_writecr4
	_scouse_writecr4:

	mov rax, rcx
	mov cr4, rax
	ret

