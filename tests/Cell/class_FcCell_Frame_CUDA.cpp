/*
    (C) Copyright 2014 CEA LIST. All Rights Reserved.
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

#ifdef CUDA

#include "N2D2.hpp"

#include "Database/MNIST_IDX_Database.hpp"
#include "Cell/FcCell_Frame_CUDA.hpp"
#include "utils/UnitTest.hpp"

using namespace N2D2;

class FcCell_Frame_Test_CUDA : public FcCell_Frame_CUDA {
public:
    FcCell_Frame_Test_CUDA(const std::string& name,
                           unsigned int nbOutputs,
                           const std::shared_ptr
                           <Activation>& activation)
        : Cell(name, nbOutputs),
          FcCell(name, nbOutputs),
          FcCell_Frame_CUDA(name, nbOutputs, activation) {};

    friend class UnitTest_FcCell_Frame_CUDA_addInput__env;
    friend class UnitTest_FcCell_Frame_CUDA_addInput;
    friend class UnitTest_FcCell_Frame_CUDA_propagate_input_check;
    friend class UnitTest_FcCell_Frame_CUDA_propagate_2_input_check;
    friend class UnitTest_FcCell_Frame_CUDA_propagate_weight_check;
};

TEST_DATASET(FcCell_Frame_CUDA,
             FcCell_Frame_CUDA,
             (unsigned int nbOutputs),
             std::make_tuple(0U),
             std::make_tuple(1U),
             std::make_tuple(3U),
             std::make_tuple(10U),
             std::make_tuple(253U))
{
    REQUIRED(UnitTest::CudaDeviceExists(3));

    FcCell_Frame_CUDA fc1("fc1", nbOutputs);

    ASSERT_EQUALS(fc1.getName(), "fc1");
    ASSERT_EQUALS(fc1.getNbOutputs(), nbOutputs);
}

TEST_DATASET(FcCell_Frame_CUDA,
             addInput__env,
             (unsigned int nbOutputs,
              unsigned int channelsWidth,
              unsigned int channelsHeight),
             std::make_tuple(1U, 24U, 24U),
             std::make_tuple(1U, 24U, 32U),
             std::make_tuple(1U, 32U, 24U),
             std::make_tuple(3U, 24U, 24U),
             std::make_tuple(3U, 24U, 32U),
             std::make_tuple(3U, 32U, 24U),
             std::make_tuple(10U, 24U, 24U),
             std::make_tuple(10U, 24U, 32U),
             std::make_tuple(10U, 32U, 24U))
{
    REQUIRED(UnitTest::CudaDeviceExists(3));

    Network net;
    Environment env(net, EmptyDatabase, {channelsWidth, channelsHeight, 1});

    FcCell_Frame_Test_CUDA fc1("fc1",
                               nbOutputs,
                               std::make_shared
                               <TanhActivation_Frame_CUDA<Float_T> >());
    fc1.setParameter("NoBias", true);
    fc1.addInput(env);
    fc1.initialize();

    ASSERT_EQUALS(fc1.getNbChannels(), 1U);
    ASSERT_EQUALS(fc1.getChannelsWidth(), channelsWidth);
    ASSERT_EQUALS(fc1.getChannelsHeight(), channelsHeight);
    ASSERT_EQUALS(fc1.getNbOutputs(), nbOutputs);
    ASSERT_EQUALS(fc1.getOutputsWidth(), 1U);
    ASSERT_EQUALS(fc1.getOutputsHeight(), 1U);
    // ASSERT_NOTHROW_ANY(fc1.checkGradient(1.0e-3, 1.0e-3));

    // Internal state testing
    ASSERT_EQUALS(fc1.mInputs.dataSize(), channelsWidth * channelsHeight);
    ASSERT_EQUALS(fc1.mOutputs.size(), nbOutputs);
    ASSERT_EQUALS(fc1.mDiffInputs.size(), nbOutputs);
    ASSERT_EQUALS(fc1.mDiffOutputs.dataSize(), 0U);
}

TEST_DATASET(FcCell_Frame_CUDA,
             addInput,
             (unsigned int nbOutputs,
              unsigned int channelsWidth,
              unsigned int channelsHeight),
             std::make_tuple(1U, 24U, 24U),
             std::make_tuple(1U, 24U, 32U),
             std::make_tuple(1U, 32U, 24U),
             std::make_tuple(3U, 24U, 24U),
             std::make_tuple(3U, 24U, 32U),
             std::make_tuple(3U, 32U, 24U),
             std::make_tuple(10U, 24U, 24U),
             std::make_tuple(10U, 24U, 32U),
             std::make_tuple(10U, 32U, 24U))
{
    REQUIRED(UnitTest::CudaDeviceExists(3));

    Network net;
    Environment env(net, EmptyDatabase, {channelsWidth, channelsHeight, 1});

    FcCell_Frame_Test_CUDA fc1(
        "fc1", 16, std::make_shared<TanhActivation_Frame_CUDA<Float_T> >());
    FcCell_Frame_Test_CUDA fc2("fc2",
                               nbOutputs,
                               std::make_shared
                               <TanhActivation_Frame_CUDA<Float_T> >());

    fc1.addInput(env);
    fc2.addInput(&fc1);
    fc1.initialize();
    fc2.initialize();

    ASSERT_EQUALS(fc2.getNbSynapses(), (16U + 1U) * nbOutputs);
    ASSERT_EQUALS(fc2.getNbChannels(), 16U);
    ASSERT_EQUALS(fc2.getChannelsWidth(), 1U);
    ASSERT_EQUALS(fc2.getChannelsHeight(), 1U);
    ASSERT_EQUALS(fc2.getNbOutputs(), nbOutputs);
    ASSERT_EQUALS(fc2.getOutputsWidth(), 1U);
    ASSERT_EQUALS(fc2.getOutputsHeight(), 1U);
    // ASSERT_NOTHROW_ANY(fc2.checkGradient(1.0e-3, 1.0e-3));

    // Internal state testing
    ASSERT_EQUALS(fc2.mInputs.dataSize(), 16U);
    ASSERT_EQUALS(fc2.mOutputs.size(), nbOutputs);
    ASSERT_EQUALS(fc2.mDiffInputs.size(), nbOutputs);
    ASSERT_EQUALS(fc2.mDiffOutputs.dataSize(), 16U);
}

TEST_DATASET(FcCell_Frame_CUDA,
             propagate_input_check,
             (unsigned int nbOutputs,
              unsigned int channelsWidth,
              unsigned int channelsHeight),
             std::make_tuple(1U, 1U, 1U),
             std::make_tuple(1U, 1U, 2U),
             std::make_tuple(2U, 2U, 1U),
             std::make_tuple(3U, 3U, 3U),
             std::make_tuple(1U, 10U, 10U),
             std::make_tuple(2U, 25U, 25U),
             std::make_tuple(1U, 25U, 30U),
             std::make_tuple(1U, 30U, 25U),
             std::make_tuple(1U, 30U, 30U))
{
    REQUIRED(UnitTest::CudaDeviceExists(3));
    REQUIRED(UnitTest::DirExists(N2D2_DATA("mnist")));

    Network net;

    FcCell_Frame_Test_CUDA fc1(
        "fc1", nbOutputs, std::shared_ptr<Activation>());
    fc1.setParameter("NoBias", true);

    MNIST_IDX_Database database;
    database.load(N2D2_DATA("mnist"));

    Environment env(net, database, {channelsWidth, channelsHeight, 1}, 2, false);
    env.addTransformation(RescaleTransformation(channelsWidth, channelsHeight));
    env.setCachePath();

    env.readRandomBatch(Database::Test);

    Tensor<Float_T>& in = env.getData();

    ASSERT_EQUALS(in.dimZ(), 1U);
    ASSERT_EQUALS(in.dimX(), channelsWidth);
    ASSERT_EQUALS(in.dimY(), channelsHeight);

    fc1.addInput(env);
    fc1.initialize();

    const unsigned int inputSize = fc1.getNbChannels() * fc1.getChannelsWidth()
                                   * fc1.getChannelsHeight();
    const unsigned int outputSize = fc1.getNbOutputs() * fc1.getOutputsWidth()
                                    * fc1.getOutputsHeight();

    ASSERT_EQUALS(inputSize, channelsWidth * channelsHeight);
    ASSERT_EQUALS(outputSize, nbOutputs);

    for (unsigned int output = 0; output < outputSize; ++output) {
        for (unsigned int channel = 0; channel < inputSize; ++channel)
            fc1.setWeight(output, channel, 1.0);
    }

    fc1.propagate();

    const Tensor<Float_T>& out = tensor_cast<Float_T>(fc1.getOutputs());

    ASSERT_EQUALS(out.dimZ(), nbOutputs);
    ASSERT_EQUALS(out.dimX(), 1U);
    ASSERT_EQUALS(out.dimY(), 1U);

    const Float_T sum
        = std::accumulate(in.begin(),
                          in.begin() + inputSize,
                          0.0f); // Warning: 0.0 leads to wrong results!

    for (unsigned int output = 0; output < out.dimZ(); ++output) {
        ASSERT_EQUALS_DELTA(out(output, 0), sum, 1e-4);
    }
}

TEST_DATASET(FcCell_Frame_CUDA,
             propagate_2_input_check,
             (unsigned int nbOutputs,
              unsigned int channelsWidth,
              unsigned int channelsHeight),
             std::make_tuple(1U, 1U, 1U),
             std::make_tuple(1U, 1U, 2U),
             std::make_tuple(2U, 2U, 1U),
             std::make_tuple(3U, 3U, 3U),
             std::make_tuple(1U, 10U, 10U),
             std::make_tuple(2U, 25U, 25U),
             std::make_tuple(1U, 25U, 30U),
             std::make_tuple(1U, 30U, 25U),
             std::make_tuple(1U, 30U, 30U))
{
    REQUIRED(UnitTest::CudaDeviceExists(3));
    REQUIRED(UnitTest::DirExists(N2D2_DATA("mnist")));

    Network net;

    FcCell_Frame_Test_CUDA fc1(
        "fc1", nbOutputs, std::shared_ptr<Activation>());
    fc1.setParameter("NoBias", true);

    MNIST_IDX_Database database;
    database.load(N2D2_DATA("mnist"));

    Environment env(net, database, {channelsWidth, channelsHeight, 1}, 2, false);
    env.addTransformation(RescaleTransformation(channelsWidth, channelsHeight));
    env.setCachePath();

    env.readRandomBatch(Database::Test);

    fc1.addInput(env);
    fc1.addInput(env);
    fc1.initialize();

    const unsigned int inputSize = fc1.getNbChannels() * fc1.getChannelsWidth()
                                   * fc1.getChannelsHeight();
    const unsigned int outputSize = fc1.getNbOutputs() * fc1.getOutputsWidth()
                                    * fc1.getOutputsHeight();

    for (unsigned int output = 0; output < outputSize; ++output) {
        for (unsigned int channel = 0; channel < inputSize; ++channel)
            fc1.setWeight(output, channel, 1.0);
    }

    fc1.propagate();
    fc1.mInputs.synchronizeDToH();

    ASSERT_EQUALS(fc1.mInputs.dimZ(), fc1.getNbChannels());
    ASSERT_EQUALS(fc1.mInputs[0].dimX(), fc1.getChannelsWidth());
    ASSERT_EQUALS(fc1.mInputs[0].dimY(), fc1.getChannelsHeight());

    Float_T sum = 0.0;

    for (unsigned int k = 0; k < fc1.mInputs.size(); ++k) {
        const Tensor<Float_T>& input = tensor_cast<Float_T>(fc1.mInputs[k]);

        for (unsigned int channel = 0; channel < input.dimZ(); ++channel) {
            for (unsigned int y = 0; y < fc1.getChannelsHeight(); ++y) {
                for (unsigned int x = 0; x < fc1.getChannelsWidth(); ++x)
                    sum += input(x, y, channel, 0);
            }
        }
    }

    const Tensor<Float_T>& out = tensor_cast<Float_T>(fc1.getOutputs());

    ASSERT_EQUALS(out.dimZ(), fc1.getNbOutputs());
    ASSERT_EQUALS(out.dimX(), fc1.getOutputsWidth());
    ASSERT_EQUALS(out.dimY(), fc1.getOutputsHeight());

    for (unsigned int ox = 0; ox < fc1.getOutputsWidth(); ++ox) {
        for (unsigned int oy = 0; oy < fc1.getOutputsHeight(); ++oy) {
            for (unsigned int output = 0; output < fc1.getNbOutputs(); ++output)
                ASSERT_EQUALS_DELTA(out(ox, oy, output, 0), sum, 1e-3);
        }
    }
}

TEST_DATASET(FcCell_Frame_CUDA,
             propagate_weight_check,
             (unsigned int nbOutputs,
              unsigned int channelsWidth,
              unsigned int channelsHeight),
             std::make_tuple(1U, 1U, 1U),
             std::make_tuple(1U, 1U, 2U),
             std::make_tuple(2U, 2U, 1U),
             std::make_tuple(3U, 3U, 3U),
             std::make_tuple(1U, 10U, 10U),
             std::make_tuple(2U, 25U, 25U),
             std::make_tuple(1U, 25U, 30U),
             std::make_tuple(1U, 30U, 25U),
             std::make_tuple(1U, 30U, 30U))
{
    REQUIRED(UnitTest::CudaDeviceExists(3));

    Network net;
    Environment env(
        net, EmptyDatabase, {channelsWidth, channelsHeight, 1}, 2, false);

    FcCell_Frame_Test_CUDA fc1(
        "fc1", nbOutputs, std::shared_ptr<Activation>());
    fc1.setParameter("NoBias", true);

    const cv::Mat img0(
        channelsHeight, channelsWidth, CV_32FC1, cv::Scalar(1.0));
    const cv::Mat img1(
        channelsHeight, channelsWidth, CV_32FC1, cv::Scalar(0.5));

    env.streamStimulus(img0, Database::Learn, 0);
    env.streamStimulus(img1, Database::Learn, 1);

    fc1.addInput(env);
    fc1.initialize();

    const unsigned int inputSize = fc1.getNbChannels() * fc1.getChannelsWidth()
                                   * fc1.getChannelsHeight();

    fc1.propagate();

    const Tensor<Float_T>& out = tensor_cast<Float_T>(fc1.getOutputs());

    ASSERT_EQUALS(out.dimZ(), nbOutputs);
    ASSERT_EQUALS(out.dimX(), 1U);
    ASSERT_EQUALS(out.dimY(), 1U);

    for (unsigned int output = 0; output < out.dimZ(); ++output) {
        Float_T sum = 0.0;

        for (unsigned int channel = 0; channel < inputSize; ++channel)
            sum += fc1.getWeight(output, channel);

        ASSERT_EQUALS_DELTA(out(output, 0), sum, 1e-4);
    }
}

RUN_TESTS()

#else

int main()
{
    return 0;
}

#endif
