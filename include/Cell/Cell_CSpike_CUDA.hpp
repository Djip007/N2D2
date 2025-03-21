/*
    (C) Copyright 2016 CEA LIST. All Rights Reserved.
    Contributor(s): Olivier BICHLER (olivier.bichler@cea.fr)
                    Johannes THIELE (johannes.thiele@cea.fr)

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

#ifndef N2D2_CELL_CSPIKE_CUDA_H
#define N2D2_CELL_CSPIKE_CUDA_H

#include "CEnvironment.hpp"
#include "Cell/Cell.hpp"
#include "Cell/Cell_CSpike_CUDA_Kernels.hpp"
#include "Cell/Cell_CSpike_Top.hpp"
#include "containers/CudaTensor.hpp"
#include "controler/CudaInterface.hpp"

namespace N2D2 {

class DeepNet;

class Cell_CSpike_CUDA : public virtual Cell, public Cell_CSpike_Top {
public:
    Cell_CSpike_CUDA(const DeepNet& deepNet, const std::string& name, unsigned int nbOutputs);
    virtual void addInput(StimuliProvider& sp,
                          unsigned int channel,
                          unsigned int x0,
                          unsigned int y0,
                          unsigned int width,
                          unsigned int height,
                          const Tensor<bool>& mapping = Tensor<bool>());
    virtual void addInput(StimuliProvider& sp,
                          unsigned int x0 = 0,
                          unsigned int y0 = 0,
                          unsigned int width = 0,
                          unsigned int height = 0,
                          const Tensor<bool>& mapping = Tensor<bool>());
    virtual void addInput(Cell* cell,
                          const Tensor<bool>& mapping = Tensor<bool>());
    virtual void addInput(Cell* cell,
                          unsigned int x0,
                          unsigned int y0,
                          unsigned int width = 0,
                          unsigned int height = 0);
    virtual bool tick(Time_T timestamp);
    virtual void reset(Time_T timestamp);
    virtual Tensor<int>& getOutputsActivity();
    virtual Tensor<int>& getOutputs();
    bool isCuda() const
    {
        return true;
    }
    virtual ~Cell_CSpike_CUDA() {};

protected:
    // Forward
    CudaInterface<int> mInputs;

    CudaTensor<int> mOutputs;
    CudaTensor<int> mOutputsActivity;

private:
    template <class T>
    void accumulate(CudaTensor<T>* outputsActivity,
                    CudaTensor<int>* outputs);
};
}

namespace N2D2 {
template <>
void Cell_CSpike_CUDA::accumulate
    <int>(CudaTensor<int>* outputsActivity, CudaTensor<int>* outputs);

template <>
void Cell_CSpike_CUDA::accumulate
    <float>(CudaTensor<float>* outputsActivity, CudaTensor<int>* outputs);

template <>
void Cell_CSpike_CUDA::accumulate<double>(CudaTensor<double>* outputsActivity,
                                          CudaTensor<int>* outputs);
}

#endif // N2D2_CELL_CSPIKE_CUDA_H
