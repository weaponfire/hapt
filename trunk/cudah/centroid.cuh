/**
 *   HAPT -- Centroid CUDA
 *
 * Maximo, Andre -- Mar, 2009
 *
 */

extern "C"
void initCUDA( float *h_centroidList, unsigned _numCentroids );

extern "C"
void cleanCUDA( void );

extern "C"
void bitonicSortCUDA( uint *ids, float _mvX, float _mvY, float _mvZ );

extern "C"
void quickSortCUDA( uint *ids, float _mvX, float _mvY, float _mvZ );
