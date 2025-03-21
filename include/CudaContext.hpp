/*
    (C) Copyright 2015 CEA LIST. All Rights Reserved.
    Contributor(s): Olivier BICHLER (olivier.bichler@cea.fr)
                    Victor GACOIN

    This software is governed by the CeCILL-C license under French law and
    abiding by the rules of distribution of free software.  You can  use,
    modify and/ or redistribute the software under the terms of the CeCILL-C
    license as circulated by CEA, CNRS and INRIA at the following URL
    "http://www.cecill.info".

    As a counterpart to the access to the source code and  rights to copy,
    modify and redistribute granted by the license, users are provided only
    with a limited warranty  and the software's author,  the holder of the
    economic rights,  and the successive licensors  have only  limited
    liability.

    The fact that you are presently reading this means that you have had
    knowledge of the CeCILL-C license and that you accept its terms.
*/

#ifndef N2D2_CUDA_CONTEXT_H
#define N2D2_CUDA_CONTEXT_H

#include <iostream>

#include "CudaUtils.hpp"

#include "third_party/half.hpp"

namespace N2D2 {
class CudaContext {
public:
    static void setDevice(int device)
    {
        CHECK_CUDA_STATUS(cudaSetDevice(device));
    }

    static const cudaDeviceProp& getDeviceProp()
    {
        static cudaDeviceProp deviceProp;
        static bool init = false;

        if (!init) {
            int device;
            CHECK_CUDA_STATUS(cudaGetDevice(&device));
            CHECK_CUDA_STATUS(cudaGetDeviceProperties(&deviceProp, device));
            init = true;
        }

        return deviceProp;
    }

    // Declare cublas handle
    static cublasHandle_t& cublasHandle()
    {
        static cublasHandle_t cublas_h = NULL;

        if (!cublas_h) {
            CHECK_CUBLAS_STATUS(cublasCreate(&cublas_h));
            std::cout << "CUBLAS initialized" << std::endl;
        }

        return cublas_h;
    }

    // Declare cudnn handle
    static cudnnHandle_t& cudnnHandle()
    {
        static cudnnHandle_t cudnn_h = NULL;

        if (!cudnn_h) {
            CHECK_CUDNN_STATUS(cudnnCreate(&cudnn_h));
            std::cout << "CUDNN initialized" << std::endl;
        }

        return cudnn_h;
    }

    template <class T>
    struct data_type {
        static const cudnnDataType_t value = CUDNN_DATA_FLOAT;
                                            // Dummy value by default
    };
};
}

namespace N2D2 {
    template <>
    struct CudaContext::data_type<half_float::half> {
        static const cudnnDataType_t value = CUDNN_DATA_HALF;
    };

    template <>
    struct CudaContext::data_type<float> {
        static const cudnnDataType_t value = CUDNN_DATA_FLOAT;
    };

    template <>
    struct CudaContext::data_type<double> {
        static const cudnnDataType_t value = CUDNN_DATA_DOUBLE;
    };
}

#endif // N2D2_CUDA_CONTEXT_H
