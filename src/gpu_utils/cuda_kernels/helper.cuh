#ifndef CUDA_HELPER_KERNELS_CUH_
#define CUDA_HELPER_KERNELS_CUH_

#include <cuda_runtime.h>
#include <vector>
#include <iostream>
#include <fstream>
#include "src/gpu_utils/cuda_utils.cuh"

__global__ void cuda_kernel_sumweightCoarse(  FLOAT *g_pdf_orientation,
									    	  FLOAT *g_pdf_offset,
									    	  FLOAT *g_Mweight,
									    	  FLOAT *g_thisparticle_sumweight,
									    	  FLOAT min_diff2,
									    	  int oversamples_orient,
									    	  int oversamples_trans,
									    	  int coarse_trans);

__global__ void cuda_kernel_sumweightFine(    FLOAT *g_pdf_orientation,
											  FLOAT *g_pdf_offset,
											  FLOAT *g_weights,
											  FLOAT *g_thisparticle_sumweight,
											  FLOAT min_diff2,
											  int oversamples_orient,
											  int oversamples_trans,
									     	  unsigned long *d_rot_id,
											  unsigned long *d_trans_idx,
											  unsigned long *d_job_idx,
											  unsigned long *d_job_num,
									     	  long int job_num);

__global__ void cuda_kernel_collect2(	FLOAT *g_oo_otrans_x,
										FLOAT *g_oo_otrans_y,
										FLOAT *g_myp_oo_otrans_x2y2z2,
										FLOAT *g_Mweight,
										FLOAT op_significant_weight,
										FLOAT op_sum_weight,
										int   coarse_trans,
										int   oversamples_trans,
										int   oversamples_orient,
										int   oversamples,
										bool  do_ignore_pdf_direction,
										FLOAT *g_weights,
										FLOAT *g_thr_wsum_prior_offsetx_class,
										FLOAT *g_thr_wsum_prior_offsety_class,
										FLOAT *g_thr_wsum_sigma2_offset
										);

__global__ void cuda_kernel_collect2jobs(	FLOAT *g_oo_otrans_x,          // otrans-size -> make const
										FLOAT *g_oo_otrans_y,          // otrans-size -> make const
										FLOAT *g_myp_oo_otrans_x2y2z2, // otrans-size -> make const
										FLOAT *g_i_weights,
										FLOAT op_significant_weight,    // TODO Put in const
										FLOAT op_sum_weight,            // TODO Put in const
										int   coarse_trans,
										int   oversamples_trans,
										int   oversamples_orient,
										int   oversamples,
										bool  do_ignore_pdf_direction,
										FLOAT *g_o_weights,
										FLOAT *g_thr_wsum_prior_offsetx_class,
										FLOAT *g_thr_wsum_prior_offsety_class,
										FLOAT *g_thr_wsum_sigma2_offset,
								     	unsigned long *d_rot_idx,
								     	unsigned long *d_trans_idx,
								     	unsigned long *d_job_idx,
								     	unsigned long *d_job_num
								     	);

// Stacks images in place, reducing at most 2*gridDim.x images down to gridDim.x images.
// Ex; 19 -> 16 or 32 -> 16,
__global__ void cuda_kernel_reduce_wdiff2s(FLOAT *g_wdiff2s_parts,
										   long int orientation_num,
										   int image_size,
										   int current_block_num);

#endif /* CUDA_HELPER_KERNELS_CUH_ */