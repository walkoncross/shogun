/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Written (W) 2009 Soeren Sonnenburg
 * Copyright (C) 2009 Fraunhofer Institute FIRST and Max-Planck-Society
 */

#include "features/ImplicitWeightedSpecFeatures.h"
#include "lib/io.h"

CImplicitWeightedSpecFeatures::CImplicitWeightedSpecFeatures(CStringFeatures<uint16_t>* str, bool normalize) : CDotFeatures()
{
	ASSERT(str);
	SG_REF(strings)
	strings=str;
	use_normalization=normalize;
	num_strings = str->get_num_vectors();
	spec_size = str->get_num_symbols();
	degree=str->get_order();
	set_wd_weights();

	SG_DEBUG("SPEC size=%d, num_str=%d\n", spec_size, num_strings);
}

bool CImplicitWeightedSpecFeatures::set_wd_weights()
{
	spec_weights=new float64_t[degree];

	int32_t i;
	float64_t sum=0;
	for (i=0; i<degree; i++)
	{
		spec_weights[i]=degree-i;
		sum+=spec_weights[i];
	}
	for (i=0; i<degree; i++)
		spec_weights[i]/=sum;

	return spec_weights!=NULL;
}

bool CImplicitWeightedSpecFeatures::set_weights(float64_t* w, int32_t d)
{
	ASSERT(d==degree);

	delete[] spec_weights;
	spec_weights=new float64_t[degree];
	for (int32_t i=0; i<degree; i++)
		spec_weights[i]=w[i];
	return true;
}

CImplicitWeightedSpecFeatures::CImplicitWeightedSpecFeatures(const CImplicitWeightedSpecFeatures& orig) : CDotFeatures(orig), 
	num_strings(orig.num_strings), 
	alphabet_size(orig.alphabet_size), spec_size(orig.spec_size)
{
	SG_NOTIMPLEMENTED;
	SG_REF(strings);
}

CImplicitWeightedSpecFeatures::~CImplicitWeightedSpecFeatures()
{
	SG_UNREF(strings);
}

float64_t CImplicitWeightedSpecFeatures::dot(int32_t vec_idx1, int32_t vec_idx2)
{
	ASSERT(vec_idx1 < num_strings);
	ASSERT(vec_idx2 < num_strings);

	int32_t len1=-1;
	int32_t len2=-1;
	uint16_t* vec1=strings->get_feature_vector(vec_idx1, len1);
	uint16_t* vec2=strings->get_feature_vector(vec_idx2, len2);

	float64_t result=0;
	uint8_t mask=0;

	for (int32_t d=0; d<degree; d++)
	{
		mask = mask | (1 << (degree-d-1));
		uint16_t masked=strings->get_masked_symbols(0xffff, mask);

		int32_t left_idx=0;
		int32_t right_idx=0;

		while (left_idx < len1 && right_idx < len2)
		{
			uint16_t lsym=vec1[left_idx] & masked;
			uint16_t rsym=vec2[right_idx] & masked;

			if (lsym == rsym)
			{
				int32_t old_left_idx=left_idx;
				int32_t old_right_idx=right_idx;

				while (left_idx<len1 && (vec1[left_idx] & masked) ==lsym)
					left_idx++;

				while (right_idx<len2 && (vec2[right_idx] & masked) ==lsym)
					right_idx++;

				result+=spec_weights[d]*(left_idx-old_left_idx)*(right_idx-old_right_idx);
			}
			else if (lsym<rsym)
				left_idx++;
			else
				right_idx++;
		}
	}

	return result;
}

float64_t CImplicitWeightedSpecFeatures::dense_dot(int32_t vec_idx1, const float64_t* vec2, int32_t vec2_len)
{
	ASSERT(vec2_len == spec_size);
	ASSERT(vec_idx1 < num_strings);

	float64_t result=0;
	int32_t len1=-1;
	uint16_t* vec=strings->get_feature_vector(vec_idx1, len1);

	if (vec && len1>0)
	{
		for (int32_t j=0; j<len1; j++)
		{
			uint8_t mask=0;
			int32_t offs=0;
			for (int32_t d=0; d<degree; d++)
			{
				mask = mask | (1 << (degree-d-1));
				int32_t idx=strings->get_masked_symbols(vec[j], mask);
				idx=strings->shift_symbol(idx, degree-d-1);
				result += vec2[offs + idx];
				offs+=strings->shift_offset(1,d+1);
			}
		}
	}
	return result;
}

void CImplicitWeightedSpecFeatures::add_to_dense_vec(float64_t alpha, int32_t vec_idx1, float64_t* vec2, int32_t vec2_len, bool abs_val)
{
	int32_t len1=-1;
	uint16_t* vec=strings->get_feature_vector(vec_idx1, len1);

	if (vec && len1>0)
	{
		for (int32_t j=0; j<len1; j++)
		{
			uint8_t mask=0;
			int32_t offs=0;
			for (int32_t d=0; d<degree; d++)
			{
				mask = mask | (1 << (degree-d-1));
				int32_t idx=strings->get_masked_symbols(vec[j], mask);
				idx=strings->shift_symbol(idx, degree-d-1);
				if (abs_val)
					vec2[offs + idx] += CMath::abs(alpha*spec_weights[d]);
				else
					vec2[offs + idx] += alpha*spec_weights[d];
				offs+=strings->shift_offset(1,d+1);
			}
		}
	}
}

CFeatures* CImplicitWeightedSpecFeatures::duplicate() const
{
	return new CImplicitWeightedSpecFeatures(*this);
}
