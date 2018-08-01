/*
    (C) Copyright 2016 CEA LIST. All Rights Reserved.
    Contributor(s): Olivier BICHLER (olivier.bichler@cea.fr)

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

#ifndef N2D2_SGDSOLVER_H
#define N2D2_SGDSOLVER_H

#include "Solver/Solver.hpp"

#ifdef WIN32
// For static library
#pragma comment(                                                               \
    linker,                                                                    \
    "/include:?mRegistrar@?$SGDSolver_Frame@M@N2D2@@0U?$Registrar@V?$SGDSolver@M@N2D2@@@2@A")
#ifdef CUDA
#pragma comment(                                                               \
    linker,                                                                    \
    "/include:?mRegistrar@?$SGDSolver_Frame_CUDA@M@N2D2@@0U?$Registrar@V?$SGDSolver@M@N2D2@@@2@A")
#endif
#endif

namespace N2D2 {
class SGDSolver : public Solver {
public:
    typedef std::function<std::shared_ptr<SGDSolver>()> RegistryCreate_T;

    static RegistryMap_T& registry()
    {
        static RegistryMap_T rMap;
        return rMap;
    }

    enum LearningRatePolicy {
        None,
        StepDecay,
        ExponentialDecay,
        InvTDecay,
        PolyDecay,
        InvDecay
    };

    SGDSolver();
    std::shared_ptr<SGDSolver> clone() const
    {
        return std::shared_ptr<SGDSolver>(doClone());
    }
    bool isNewIteration() const
    {
        return (mIterationPass == 0);
    }
    virtual ~SGDSolver() {};

protected:
    double getLearningRate(unsigned int batchSize);

    /// Initial learning rate
    Parameter<double> mLearningRate;
    /// Momentum
    Parameter<double> mMomentum;
    /// Decay
    Parameter<double> mDecay;
    /// Power
    Parameter<double> mPower;
    /// Global batch size = batch size * mIterationSize
    Parameter<unsigned int> mIterationSize;
    /// MaxIterations
    Parameter<unsigned long long int> mMaxIterations;
    /// Learning rate decay policy
    Parameter<LearningRatePolicy> mLearningRatePolicy;
    /// Learning rate step size
    Parameter<unsigned int> mLearningRateStepSize;
    /// Learning rate decay
    Parameter<double> mLearningRateDecay;
    /// If true, don't clamp the weights between -1 and 1 during learning
    Parameter<bool> mClamping;

    unsigned int mIterationPass;
    unsigned int mNbIterations;

private:
    virtual SGDSolver* doClone() const = 0;
};
}

namespace {
template <>
const char* const EnumStrings<N2D2::SGDSolver::LearningRatePolicy>::data[]
    = {"None",
       "StepDecay",
       "ExponentialDecay",
       "InvTDecay",
       "PolyDecay",
       "InvDecay"};
}

#endif // N2D2_SGDSOLVER_H
