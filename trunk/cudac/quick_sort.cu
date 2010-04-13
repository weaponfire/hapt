/**
 *   Quick Sort on GPU (using CUDA)
 *
 *   Adapted from:
 *
 *     http://www.cs.chalmers.se/~dcs/gpuqsortdcs.html
 *
 * Maximo, Andre -- Mar, 2009
 *
 **/

#include <algorithm>

#include <cutil.h>

uint szPackedArrayQuick;

uint_64 *d_packedArrayQuick, *d_auxiliaryArray;

#define MAXTHREADS 256
#define MAXBLOCKS 2048

uint threads, blockscount, sbsize, size;

// Keep tracks of the data blocks in phase one
typedef struct _BlockSize {

	unsigned int beg;
	unsigned int end;
	unsigned int orgbeg;
	unsigned int orgend;
	uint_64		 rmaxpiv;
	uint_64		 lmaxpiv;
	uint_64		 rminpiv;
	uint_64		 lminpiv;
	bool		 altered;
	bool		 flip;
	uint_64		 pivot;

} BlockSize;

// Holds parameters to the kernel in phase one
typedef struct _Params {

	unsigned int from;
	unsigned int end;
	uint_64 pivot;
	unsigned int ptr;
	bool last;

} Params;

// Used to perform a cumulative sum between blocks.
// Unnecessary for cards with atomic operations.
// Will be removed when these becomes more common
typedef struct _Length {

	uint_64 maxpiv[MAXBLOCKS];
	uint_64 minpiv[MAXBLOCKS];
	unsigned int left[MAXBLOCKS];
	unsigned int right[MAXBLOCKS];

} Length;

// Since we have divided up the kernel in to three
// we need to remember the result of the cumulative sum
// Unnecessary for cards with atomic operations.
// Will be removed when these becomes more common
typedef struct _Hist {

	unsigned int left[(MAXTHREADS)*MAXBLOCKS];
	unsigned int right[(MAXTHREADS)*MAXBLOCKS];

} Hist;

typedef struct _LQSortParams {

	unsigned int beg;
	unsigned int end;
	bool flip;
	unsigned int sbsize;

} LQSortParams;

#include "gpuqsort_kernels.cu"

#undef THREADS
#define THREADS threads

Params* params;
Params* dparams;
LQSortParams* lqparams;
LQSortParams* dlqparams;
Hist* dhists;
Length* dlength;
Length* length;
BlockSize* workset;
float TK,TM,MK,MM,SM,SK;

/// Initialize Quick

__host__
void initQuick( uint numElements ) {

	size = numElements;

	/// Quick Sort
	szPackedArrayQuick = size * sizeof(uint_64);

	CUDA_SAFE_CALL( cudaMalloc((void**) &d_packedArrayQuick, szPackedArrayQuick) );

	/// Prepare device (qsort)
	cudaDeviceProp deviceProp;
	cudaGetDeviceProperties(&deviceProp, 0);
	if( !strcmp(deviceProp.name, "GeForce 8800 GTX") ) {

		TK = 1.17125033316e-005f;
		TM = 52.855721393f;
		MK = 3.7480010661e-005f;
		MM = 476.338308458f;
		SK = 4.68500133262e-005f;
		SM = 211.422885572f;

	} else if( !strcmp(deviceProp.name, "GeForce 8600 GTS") ) {

		TK = 0.0f;
		TM = 64.0f;
		MK = 0.0000951623403898f;
		MM = 476.338308458f;
		SK = 0.0000321583081317f;
		SM = 202.666666667f;

	} else {

		TK = 0;
		TM = 128;
		MK = 0;
		MM = 512;
		SK = 0;
		SM = 512;

	}

	if( cudaMallocHost((void**)&workset,MAXBLOCKS*2*sizeof(BlockSize)) != cudaSuccess ) return;
	if( cudaMallocHost((void**)&params,MAXBLOCKS*sizeof(Params)) != cudaSuccess ) return;
	if( cudaMallocHost((void**)&length,sizeof(Length)) != cudaSuccess ) return;
	if( cudaMallocHost((void**)&lqparams,MAXBLOCKS*sizeof(LQSortParams)) != cudaSuccess ) return;
	if( cudaMalloc((void**)&dlqparams,MAXBLOCKS*sizeof(LQSortParams)) != cudaSuccess ) return;
	if( cudaMalloc((void**)&dhists,sizeof(Hist)) != cudaSuccess ) return;
	if( cudaMalloc((void**)&dlength,sizeof(Length)) != cudaSuccess ) return;
	if( cudaMalloc((void**)&dparams,MAXBLOCKS*sizeof(Params)) != cudaSuccess ) return;

	threads = 1<<(int)round(log(size * TK + TM)/log(2.0));
	blockscount = 1<<(int)round(log(size * MK + MM)/log(2.0));
	sbsize = 1<<(int)round(log(size * SK + SM)/log(2.0));

	if( threads > MAXTHREADS ) return; 
	if( blockscount > MAXBLOCKS ) return;

	if( cudaMalloc((void**)&d_auxiliaryArray, szPackedArrayQuick) != cudaSuccess ) return;

}

/// Quick Sort

__host__
void quickSort( void ) {

#ifdef HASATOMICS
	uint_64* doh;
	uint_64 oh;

	cudaGetSymbolAddress( (void**)&doh, "ohtotal" );
	oh=0;
	cudaMemcpy( doh, &oh, 4, cudaMemcpyHostToDevice );
#endif

	//d_auxiliaryArray = 0;
	//if( cudaMalloc((void**)&d_auxiliaryArray, szPackedArrayQuick) != cudaSuccess ) return;

	cudaThreadSynchronize();

	// We start with a set containg only the sequence to be sorted
	// This will grow as we partition the data
	workset[0].beg = 0;
	workset[0].end = size;
	workset[0].orgbeg = 0;
	workset[0].orgend = size;
	workset[0].altered = false;
	workset[0].flip = false;

	// Get a starting pivot
	workset[0].pivot = 0x7fffffffffffffff;

	unsigned int worksize = 1;

	unsigned int blocks = blockscount/2;
	unsigned totsize = size;
	unsigned int maxlength = (size/blocks)/4;

	unsigned int iterations = 0;
	bool flip = true;

	// Partition the sequences until we have enough
	while( worksize < blocks ) {

		unsigned int ws = totsize / blocks;
		unsigned int paramsize = 0;

		// Go through the sequences we have and divide them into sections
		// and assign thread blocks according to their size
		for(unsigned int i = 0; i < worksize; i++) {

			if( (workset[i].end-workset[i].beg) < maxlength) continue;

			// Larger sequences gets more thread blocks assigned to them
			unsigned int blocksassigned = max( (workset[i].end-workset[i].beg) / ws, 1 );
			for(unsigned int q = 0; q < blocksassigned; q++) {

				params[paramsize].from = workset[i].beg + ws*q;
				params[paramsize].end = params[paramsize].from + ws;
				params[paramsize].pivot = workset[i].pivot;
				params[paramsize].ptr = i;
				params[paramsize].last = false;
				paramsize++;
				
			}

			params[paramsize-1].last = true;
			params[paramsize-1].end = workset[i].end;

			workset[i].lmaxpiv=0;
			workset[i].lminpiv=0xffffffffffffffff;
			workset[i].rmaxpiv=0;
			workset[i].rminpiv=0xffffffffffffffff;
		}

		if( paramsize == 0 ) break;

		// Copy the block assignment to the GPU
		if( cudaMemcpy(dparams, params, paramsize*sizeof(Params), cudaMemcpyHostToDevice) != cudaSuccess ) return;

		// Do the cumulative sum
		if( flip ) part1<<< paramsize, THREADS, 2*( (THREADS+1)*2*4+THREADS*2*4 ) >>>(d_packedArrayQuick, dparams, dhists, dlength);
		else part1<<< paramsize, THREADS, 2*( (THREADS+1)*2*4+THREADS*2*4 ) >>>(d_auxiliaryArray, dparams, dhists, dlength);

		if( cudaMemcpy(length, dlength, sizeof(Length), cudaMemcpyDeviceToHost) != cudaSuccess ) return;

		// Do the block cumulative sum. Done on the CPU since not all cards have support for atomic operations yet. 
		for(unsigned int i = 0; i < paramsize; i++) {

			unsigned int l = length->left[i];
			unsigned int r = length->right[i];
			
			length->left[i] = workset[params[i].ptr].beg;
			length->right[i] = workset[params[i].ptr].end;
			
			workset[params[i].ptr].beg+=l;
			workset[params[i].ptr].end-=r;
			workset[params[i].ptr].altered = true;

			workset[params[i].ptr].rmaxpiv = max(length->maxpiv[i],workset[params[i].ptr].rmaxpiv);
			workset[params[i].ptr].lminpiv = min(length->minpiv[i],workset[params[i].ptr].lminpiv);

			workset[params[i].ptr].lmaxpiv = min(workset[params[i].ptr].pivot,workset[params[i].ptr].rmaxpiv); 
			workset[params[i].ptr].rminpiv = max(workset[params[i].ptr].pivot,workset[params[i].ptr].lminpiv); 

		}

		// Copy the result of the block cumulative sum to the GPU
		if( cudaMemcpy(dlength, length, sizeof(Length), cudaMemcpyHostToDevice) != cudaSuccess ) return;

		// Move the elements to their correct position
		if( flip ) part2<<< paramsize, THREADS >>>(d_packedArrayQuick, d_auxiliaryArray, dparams, dhists, dlength);
		else part2<<< paramsize, THREADS >>>(d_auxiliaryArray, d_packedArrayQuick, dparams, dhists, dlength);

		// Fill in the pivot value between the left and right blocks
		part3<<< paramsize, THREADS >>>(d_packedArrayQuick, dparams, dhists, dlength);

		flip = !flip;

		// Add the sequences resulting from the partitioning to set
		unsigned int oldworksize = worksize;
		totsize = 0;
		for(unsigned int i = 0; i < oldworksize; i++) {

			if( workset[i].altered ) {

				if(workset[i].beg-workset[i].orgbeg>=maxlength)
					totsize += workset[i].beg-workset[i].orgbeg;
				if(workset[i].orgend-workset[i].end>=maxlength)
					totsize += workset[i].orgend-workset[i].end;

				workset[worksize].beg = workset[worksize].orgbeg = workset[i].orgbeg;
				workset[worksize].end = workset[worksize].orgend = workset[i].beg;
				workset[worksize].flip=flip;
				workset[worksize].altered = false;
				workset[worksize].pivot = (workset[i].lminpiv/2+workset[i].lmaxpiv/2);

				worksize++;

				workset[i].orgbeg = workset[i].beg = workset[i].end;
				workset[i].end = workset[i].orgend;
				workset[i].flip = flip;
				workset[i].pivot = (workset[i].rminpiv/2+workset[i].rmaxpiv/2);
				workset[i].altered = false;

			}
		}

		iterations++;

	}

	// Due to the poor scheduler on some graphics card
	// we need to sort the order in which the blocks
	// are sorted to avoid poor scheduling decisions
	unsigned int sortblocks[MAXBLOCKS*2];
	for(int i = 0; i < worksize; i++)
		sortblocks[i] = ( (workset[i].end-workset[i].beg) << (int)round( log((float)(MAXBLOCKS*4.0f))/log(2.0f) ) ) + i;

	std::sort(&sortblocks[0], &sortblocks[worksize]);

	if( worksize !=0 ) {

		// Copy the block assignments to the GPU
		for(int i = 0; i < worksize; i++) {

		 	unsigned int q = ( worksize - 1 ) - ( sortblocks[i] & (MAXBLOCKS*4 - 1) );

			lqparams[i].beg = workset[q].beg;
			lqparams[i].end = workset[q].end;
			lqparams[i].flip = workset[q].flip;
			lqparams[i].sbsize = sbsize;

		}

		if( cudaMemcpy(dlqparams, lqparams, worksize*sizeof(LQSortParams), cudaMemcpyHostToDevice) != cudaSuccess ) return;

		// Run the local quicksort, the one that doesn't need inter-block synchronization
		lqsort<<< worksize, THREADS, 2*max((THREADS+1)*2*4,sbsize*4) >>>(d_packedArrayQuick, d_auxiliaryArray, dlqparams, 0);

	}

	cudaThreadSynchronize();

	//cudaFree(d_auxiliaryArray);

}

/// Clean Quick

__host__
void cleanQuick( void ) {

 	CUDA_SAFE_CALL( cudaFree(d_packedArrayQuick) );

	/// Free (puqsort)
	cudaFreeHost( workset );
	cudaFreeHost( params );
	cudaFreeHost( length );
	cudaFreeHost( lqparams );
	cudaFree( dparams );
	cudaFree( dlqparams );
	cudaFree( dhists );
	cudaFree( dlength );

	cudaFree(d_auxiliaryArray);

}
