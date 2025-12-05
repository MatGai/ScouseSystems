#ifndef ARCHX64_INTRINSICS_H
#define ARCHX64_INTRINSICS_H

#ifdef __cplusplus
extern "C" {
#endif

	void
	_scouse_cpuid(
		/*in*/ unsigned int function,
		/*out*/ unsigned int out[4]
	);




#ifdef __cplusplus
}
#endif

#endif // !ARCHX64_INTRINSICS_H