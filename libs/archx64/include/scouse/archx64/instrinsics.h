#ifndef ARCHX64_INTRINSICS_H
#define ARCHX64_INTRINSICS_H

#ifdef __cplusplus
extern "C" {
#endif

	void
	_scouse_cpuid(
		__int32,
		__int32*
	);

	unsigned __int64
	_scouse_readmsr(
		unsigned __int32,
		unsigned __int32*
	);

	unsigned __int64
	_scouse_readcr0(
		void
	);

	unsigned __int64
	_scouse_writecr0(
		unsigned __int64
	);

	unsigned __int64
	_scouse_readcr2(
		void
	);

	unsigned __int64
	_scouse_writecr2(
		unsigned __int64
	);

	unsigned __int64
	_scouse_readcr3(
		void
	);

	unsigned __int64
	_scouse_writecr3(
		unsigned __int64
	);

	unsigned __int64
	_scouse_readcr4(
		void
	);

	unsigned __int64
	_scouse_writecr4(
		unsigned __int64
	);

#ifdef __cplusplus
}
#endif

#endif // !ARCHX64_INTRINSICS_H