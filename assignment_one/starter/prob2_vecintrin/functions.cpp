#include <stdio.h>
#include <algorithm>
#include <math.h>
#include "CMU418intrin.h"
#include "logger.h"
using namespace std;


void absSerial(float* values, float* output, int N) {
    for (int i=0; i<N; i++) {
	float x = values[i];
	if (x < 0) {
	    output[i] = -x;
	} else {
	    output[i] = x;
	}
    }
}

// implementation of absolute value using 15418 instrinsics
void absVector(float* values, float* output, int N) {
    __cmu418_vec_float x;
    __cmu418_vec_float result;
    __cmu418_vec_float zero = _cmu418_vset_float(0.f);
    __cmu418_mask maskAll, maskIsNegative, maskIsNotNegative;

    //  Note: Take a careful look at this loop indexing.  This example
    //  code is not guaranteed to work when (N % VECTOR_WIDTH) != 0.
    //  Why is that the case?
    for (int i=0; i<N; i+=VECTOR_WIDTH) {

	// All ones
	maskAll = _cmu418_init_ones();

	// All zeros
	maskIsNegative = _cmu418_init_ones(0);

	// Load vector of values from contiguous memory addresses
	_cmu418_vload_float(x, values+i, maskAll);               // x = values[i];

	// Set mask according to predicate
	_cmu418_vlt_float(maskIsNegative, x, zero, maskAll);     // if (x < 0) {

	// Execute instruction using mask ("if" clause)
	_cmu418_vsub_float(result, zero, x, maskIsNegative);      //   output[i] = -x;

	// Inverse maskIsNegative to generate "else" mask
	maskIsNotNegative = _cmu418_mask_not(maskIsNegative);     // } else {

	// Execute instruction ("else" clause)
	_cmu418_vload_float(result, values+i, maskIsNotNegative); //   output[i] = x; }

	// Write results back to memory
	_cmu418_vstore_float(output+i, result, maskAll);
    }
}

// Accepts an array of values and an array of exponents
// For each element, compute values[i]^exponents[i] and clamp value to
// 4.18.  Store result in outputs.
// Uses iterative squaring, so that total iterations is proportional
// to the log_2 of the exponent
void clampedExpSerial(float* values, int* exponents, float* output, int N) {
    for (int i=0; i<N; i++) {
		float x = values[i];
		float result = 1.f;
		int y = exponents[i];
		float xpower = x;
		while (y > 0) {
			if (y & 0x1) {
				result *= xpower;
				if (result > 4.18f) {
					result = 4.18f;
					break;
				}
			}
			xpower = xpower * xpower;
			y >>= 1;
		}
		output[i] = result;
    }
}
// N = 16
void clampedExpVector(float* values, int* exponents, float* output, int N) {
	__cmu418_vec_float x;
	__cmu418_vec_int y;
	__cmu418_vec_float result;
	__cmu418_vec_int vecIsOdd;
	__cmu418_vec_float clampValue = _cmu418_vset_float(4.18f);
    __cmu418_vec_int zero = _cmu418_vset_int(0);
	__cmu418_vec_int ones = _cmu418_vset_int(1);
    __cmu418_mask maskAll, maskIsPositive,maskIsOdd,maskIsNotFinished,maskIsFinished;
	// All ones
	maskAll = _cmu418_init_ones();
	// All zeros
	maskIsPositive = _cmu418_init_ones(0);
    for (int i=0; i<N; i+=VECTOR_WIDTH) {
		
		// Initialize result to 1.0
		result = _cmu418_vset_float(1.f);
		// Load vector of values from contiguous memory addresses
		_cmu418_vload_float(x, values+i, maskAll);               // x = values[i];
		_cmu418_vload_int(y, exponents+i, maskAll);              // y = exponents[i];

		// Set mask according to predicate
		_cmu418_vgt_int(maskIsPositive, y, zero, maskAll);     // while (y > 0) {
		
		while(_cmu418_cntbits(maskIsPositive)!=0){

			_cmu418_vbitand_int(vecIsOdd,ones, y,maskAll); // if (y & 0x1) {
			_cmu418_vgt_int(maskIsOdd, vecIsOdd, zero, maskAll);

				_cmu418_vmult_float(result, result, x, maskIsOdd);      //   result *= xpower;

				maskIsNotFinished = _cmu418_init_ones();
				_cmu418_vlt_float(maskIsNotFinished, result, clampValue, maskAll); // if (result < 4.18f) {
				maskIsFinished = _cmu418_mask_not(maskIsNotFinished);

					_cmu418_vset_float(result, 4.18f, maskIsFinished);	// result = 4.18f;

			_cmu418_vmult_float(x, x, x, maskIsNotFinished); // xpower = xpower * xpower;

			_cmu418_vshiftright_int(y,y,ones,maskAll);             // y >>= 1;

			_cmu418_vgt_int(maskIsPositive, y, zero, maskAll);	 // }
		}
		// Write results back to memory
		_cmu418_vstore_float(output+i, result, maskAll);
		if(N < VECTOR_WIDTH || N%VECTOR_WIDTH!=0){
			__cmu418_vec_float zerofloat = _cmu418_vset_float(0.0f);
			_cmu418_vstore_float(output+N,zerofloat,maskAll);
		}
    }
}


float arraySumSerial(float* values, int N) {
    float sum = 0;
    for (int i=0; i<N; i++) {
	sum += values[i];
    }

    return sum;
}
#include <iostream>
// Assume N % VECTOR_WIDTH == 0
// Assume VECTOR_WIDTH is a power of 2
float arraySumVector(float* values, int N) {
	float output = 0.f;
	__cmu418_vec_float x;
	__cmu418_vec_float result;
    __cmu418_vec_int zero = _cmu418_vset_int(0);
	__cmu418_vec_int ones = _cmu418_vset_int(1);
    __cmu418_mask maskAll = _cmu418_init_ones();
	__cmu418_mask maskLeftHalf = _cmu418_init_ones(4);
	__cmu418_mask maskRightHalf = _cmu418_mask_not(maskLeftHalf);
	result = _cmu418_vset_float(0.f);
	// All zeros
    for (int i=0; i<N; i+=VECTOR_WIDTH) {
		x = _cmu418_vset_float(0.f);
		// Initialize result to 0.0
	
		// Load vector of values from contiguous memory addresses
		_cmu418_vload_float(x, values+i, maskAll);               // x = values[i];
		
		_cmu418_hadd_float(x,x);		// sum += values[i];
		_cmu418_interleave_float(x,x);

		_cmu418_vadd_float(result,result,x,maskRightHalf);
	
    }


	for(int i = 0 ; i < log2(VECTOR_WIDTH)-1; i++){
		_cmu418_hadd_float(result,result);		
		_cmu418_interleave_float(result,result);
	}
	output = result.value[VECTOR_WIDTH-1];
    return output;
}
