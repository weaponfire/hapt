/**
 *   HAPT -- Centroid CUDA
 *
 * Maximo, Andre -- Mar, 2009
 *
 */

#include <cuda.h>
#include <cutil.h>

#include <stdio.h>
#include <sys/time.h>

#include "centroid.cuh"

typedef unsigned long long int uint_64;

#include "bitonic_sort.cu"

#include "quick_sort.cu"

#define BLOCK_X 16
#define BLOCK_Y 16
#define NTHREADS BLOCK_X*BLOCK_Y

uint3 dimBlock = { BLOCK_X, BLOCK_Y, 1 };
uint3 dimGrid = { 1, 1, 1 };

uint numCentroids;
uint szCentroidList, szUnpackedArray;

float *d_centroidList;

uint *d_unpackedArray;

texture<float4, 1, cudaReadModeElementType> centroidTex;

//Round a / b to nearest higher integer value
__host__ inline uint
iDivUp(uint a, uint b) { return (a % b != 0) ? (a / b + 1) : (a / b); }

// Align a to nearest higher multiple of b
__host__ inline uint
iAlignUp(uint a, uint b) { return (a % b != 0) ? (a - a % b + b) : a; }

/// Update centroid values inside the GPU

__global__
void updateCentroid( uint_64 *packedArray,
		     const float mvX, const float mvY, const float mvZ,
		     const uint nC ) {

	uint centroidId = __mul24( __mul24( __mul24( blockIdx.y, gridDim.x ) + blockIdx.x, blockDim.x )
				   + threadIdx.y, blockDim.y ) + threadIdx.x;

	if( centroidId >= nC ) centroidId = nC - 1;

 	float4 centroid = tex1Dfetch(centroidTex, centroidId );

	float cZ = mvX * centroid.x + mvY * centroid.y + mvZ * centroid.z;

	if( cZ < -1.0 ) cZ = -1.0;
	if( cZ >  1.0 ) cZ =  1.0;

	uint_64 pack = (uint_64)( ((cZ + 1.0) / 2.0) * 0xFFFFFFFF );
	pack <<= 32;
	pack += (uint_64) centroidId;

	packedArray[ centroidId ] = pack;

}

/// Unpack array packed in update step inside the GPU

__global__
void unpackArray( uint *unpackedArray,
		  const uint_64 *packedArray,
		  const uint nC ) {

	uint centroidId = __mul24( __mul24( __mul24( blockIdx.y, gridDim.x ) + blockIdx.x, blockDim.x )
				   + threadIdx.y, blockDim.y ) + threadIdx.x;

	if( centroidId >= nC ) centroidId = nC - 1;

	/// Just throw away centroidZ and get sorted centroidId from packedArray
	unpackedArray[ centroidId ] = (uint) ( packedArray[ centroidId ] );

}

/// Initialize General

extern "C"
__host__
void initCUDA( float *h_centroidList, uint _numCentroids ) {

	/// General
	numCentroids = _numCentroids;

	szCentroidList = numCentroids * 4 * sizeof(float);

	szUnpackedArray = numCentroids * sizeof(uint);

	dimGrid.x = iDivUp( numCentroids, NTHREADS );

	CUDA_SAFE_CALL( cudaMalloc((void**) &d_centroidList, szCentroidList) );
 	CUDA_SAFE_CALL( cudaMemcpy(d_centroidList, h_centroidList, szCentroidList, cudaMemcpyHostToDevice) );

	CUDA_SAFE_CALL( cudaMalloc((void**) &d_unpackedArray, szUnpackedArray) );

	initBitonic( numCentroids );

	initQuick( numCentroids );

}

/// Run bitonic sort using CUDA

extern "C"
__host__
void bitonicSortCUDA( uint *ids, float _mvX, float _mvY, float _mvZ ) {

	CUDA_SAFE_CALL( cudaBindTexture(0, centroidTex, d_centroidList, szCentroidList) );

	updateCentroid<<< dimGrid, dimBlock >>>( d_packedArrayBitonic, _mvX, _mvY, _mvZ, numCentroids );

	CUDA_SAFE_CALL( cudaUnbindTexture(centroidTex) );

	bitonicSort();

	unpackArray<<< dimGrid, dimBlock >>>( d_unpackedArray, d_packedArrayBitonic, numCentroids );

 	CUDA_SAFE_CALL( cudaMemcpy(ids, d_unpackedArray, szUnpackedArray, cudaMemcpyDeviceToHost) );

}

/// Run quick sort using CUDA

extern "C"
__host__
void quickSortCUDA( uint *ids, float _mvX, float _mvY, float _mvZ ) {

	CUDA_SAFE_CALL( cudaBindTexture(0, centroidTex, d_centroidList, szCentroidList) );

	// create and start timer
// 	unsigned timer;
// 	cutCreateTimer(&timer);
// 	cutStartTimer(timer);

	updateCentroid<<< dimGrid, dimBlock >>>( d_packedArrayQuick, _mvX, _mvY, _mvZ, numCentroids );

	CUDA_SAFE_CALL( cudaUnbindTexture(centroidTex) );

	quickSort();

	unpackArray<<< dimGrid, dimBlock >>>( d_unpackedArray, d_packedArrayQuick, numCentroids );

	// stop and destroy timer
//	cutStopTimer(timer);
//	printf("quicksort kernel time %f ms\n ", cutGetTimerValue(timer));
//	cutDeleteTimer(timer);

 	CUDA_SAFE_CALL( cudaMemcpy(ids, d_unpackedArray, szUnpackedArray, cudaMemcpyDeviceToHost) );

}

/// Clean CUDA memory

extern "C"
__host__
void cleanCUDA( void ) {

	CUDA_SAFE_CALL( cudaFree(d_centroidList) );
 	CUDA_SAFE_CALL( cudaFree(d_unpackedArray) );

	cleanBitonic();

	cleanQuick();

}
