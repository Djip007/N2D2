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

#include "Target/TargetScore.hpp"
#include "StimuliProvider.hpp"
#include "Cell/Cell.hpp"
#include "Cell/Cell_Frame_Top.hpp"
#include "Cell/Cell_CSpike_Top.hpp"

N2D2::Registrar<N2D2::Target>
N2D2::TargetScore::mRegistrar("TargetScore", N2D2::TargetScore::create);

const char* N2D2::TargetScore::Type = "TargetScore";

N2D2::TargetScore::TargetScore(const std::string& name,
                               const std::shared_ptr<Cell>& cell,
                               const std::shared_ptr<StimuliProvider>& sp,
                               double targetValue,
                               double defaultValue,
                               unsigned int targetTopN,
                               const std::string& labelsMapping,
                               bool createMissingLabels)
    : Target(
          name, cell, sp, targetValue, defaultValue, targetTopN, labelsMapping, createMissingLabels),
      mConfusionRangeMin(this, "ConfusionRangeMin", 0.0),
      mConfusionRangeMax(this, "ConfusionRangeMax", 1.0),
      mConfusionQuantSteps(this, "ConfusionQuantSteps", 10U),
      mMaxValidationScore(0.0),
      mMaxValidationTopNScore(0.0)
{
    // ctor
}

bool N2D2::TargetScore::newValidationScore(double validationScore)
{
    const std::deque<double>& success = getScore(Database::Learn).success;

    mValidationScore.push_back(std::make_pair(success.size(), validationScore));

    if (validationScore > mMaxValidationScore) {
        mMaxValidationScore = validationScore;
        return true;
    } else
        return false;
}

bool N2D2::TargetScore::newValidationTopNScore(double validationTopNScore)
{
    if (!mDataAsTarget && mTargetTopN > 1) {
        const std::deque<double>& success
            = getTopNScore(Database::Learn).success;
        mValidationTopNScore.push_back(
            std::make_pair(success.size(), validationTopNScore));

        if (validationTopNScore > mMaxValidationTopNScore) {
            mMaxValidationTopNScore = validationTopNScore;
            return true;
        } else
            return false;
    } else
        return false;
}

double N2D2::TargetScore::getBatchAverageSuccess() const
{
    return (mBatchSuccess.size() > 0)
               ? std::accumulate(mBatchSuccess.begin(),
                                 mBatchSuccess.end(),
                                 0.0) / (double)mBatchSuccess.size()
               : 1.0;
}

double N2D2::TargetScore::getBatchAverageTopNSuccess() const
{
    if (!mDataAsTarget && mTargetTopN > 1)
        return (mBatchTopNSuccess.size() > 0)
                   ? std::accumulate(mBatchTopNSuccess.begin(),
                                     mBatchTopNSuccess.end(),
                                     0.0) / (double)mBatchTopNSuccess.size()
                   : 1.0;
    else
        return 0.0;
}

double N2D2::TargetScore::getAverageSuccess(Database::StimuliSet set,
                                            unsigned int avgWindow) const
{
    assert(mScoreSet.find(set) != mScoreSet.end());

    const std::deque<double>& success = (*mScoreSet.find(set)).second.success;

    if (success.empty())
        return 0.0;

    return (avgWindow > 0 && success.size() > avgWindow)
               ? std::accumulate(success.end() - avgWindow, success.end(), 0.0)
                 / (double)avgWindow
               : std::accumulate(success.begin(), success.end(), 0.0)
                 / (double)success.size();
}

double N2D2::TargetScore::getAverageTopNSuccess(Database::StimuliSet set,
                                                unsigned int avgWindow) const
{
    if (mDataAsTarget || !(mTargetTopN > 1))
        return 0.0;

    assert(mScoreTopNSet.find(set) != mScoreTopNSet.end());

    const std::deque<double>& success
        = (*mScoreTopNSet.find(set)).second.success;

    if (success.empty())
        return 0.0;

    return (avgWindow > 0 && success.size() > avgWindow)
               ? std::accumulate(success.end() - avgWindow, success.end(), 0.0)
                 / (double)avgWindow
               : std::accumulate(success.begin(), success.end(), 0.0)
                 / (double)success.size();
}

double N2D2::TargetScore::getAverageScore(Database::StimuliSet set,
                                          ConfusionTableMetric metric) const
{
    assert(mScoreSet.find(set) != mScoreSet.end());

    const ConfusionMatrix<unsigned long long int>& confusionMatrix
        = (*mScoreSet.find(set)).second.confusionMatrix;

    if (confusionMatrix.empty())
        return 0.0;

    const std::vector<ConfusionTable<unsigned long long int> > conf
        = confusionMatrix.getConfusionTables();

    double avgScore = 0.0;

    for (unsigned int target = 0; target < conf.size(); ++target)
        avgScore += conf[target].getMetric(metric);

    if (conf.size() > 0)
        avgScore /= conf.size();

    return avgScore;
}

double N2D2::TargetScore::getAverageTopNScore(Database::StimuliSet set,
    ConfusionTableMetric metric) const
{
    if (mDataAsTarget || !(mTargetTopN > 1))
        return 0.0;

    assert(mScoreTopNSet.find(set) != mScoreTopNSet.end());

    const ConfusionMatrix<unsigned long long int>& confusionMatrix
        = (*mScoreTopNSet.find(set)).second.confusionMatrix;

    if (confusionMatrix.empty())
        return 0.0;

    const std::vector<ConfusionTable<unsigned long long int> > conf
        = confusionMatrix.getConfusionTables();

    double avgScore = 0.0;

    for (unsigned int target = 0; target < conf.size(); ++target)
        avgScore += conf[target].getMetric(metric);

    if (conf.size() > 0)
        avgScore /= conf.size();

    return avgScore;
}

void N2D2::TargetScore::logSuccess(const std::string& fileName,
                                   Database::StimuliSet set,
                                   unsigned int avgWindow) const
{
    assert(mScoreSet.find(set) != mScoreSet.end());

    const std::string dataFileName = mName + "/Success_" + fileName + ".dat";

    if (set == Database::Validation) {
        assert(mScoreSet.find(Database::Learn) != mScoreSet.end());

        // Save validation scores file
        std::ofstream dataFile(dataFileName);

        if (!dataFile.good())
            throw std::runtime_error("Could not create data rate log file: "
                                     + dataFileName);

        for (std::vector<std::pair<unsigned int, double> >::const_iterator it
             = mValidationScore.begin(),
             itEnd = mValidationScore.end();
             it != itEnd;
             ++it) {
            dataFile << (*it).first << " " << (*it).second << "\n";
        }

        dataFile.close();

        // Plot validation
        const double lastValidation = (!mValidationScore.empty())
            ? mValidationScore.back().second : 0.0;
        const double lastLearn = (!getScore(Database::Learn).success.empty())
            ? getScore(Database::Learn).success.back() : 0.0;

        const double minFinalRate = std::min(lastValidation, lastLearn);
        const double maxFinalRate = std::max(lastValidation, lastLearn);

        std::ostringstream label;
        label << "\"Best validation: " << 100.0 * mMaxValidationScore
              << "%\" at graph 0.5, graph 0.15 front";

        Gnuplot multiplot;
        multiplot.saveToFile(dataFileName);
        multiplot.setMultiplot();

        Monitor::logDataRate((*mScoreSet.find(Database::Learn)).second.success,
                             mName + "/" + fileName + "_LearningSuccess.dat",
                             avgWindow,
                             true);

        multiplot << "clear";
        multiplot.setYrange(
            Utils::clamp(minFinalRate - (1.0 - minFinalRate), 0.0, 0.99),
            Utils::clamp(2.0 * maxFinalRate, 0.01, 1.0));
        multiplot.set("label", label.str());
        multiplot
            << ("replot \"" + dataFileName
                + "\" using 1:2 with linespoints lt 7 title \"Validation\"");
    }
    else {
        Monitor::logDataRate((*mScoreSet.find(set)).second.success,
                             dataFileName,
                             avgWindow,
                             true);
    }
}
// Top-n log success
void N2D2::TargetScore::logTopNSuccess(const std::string& fileName,
                                       Database::StimuliSet set,
                                       unsigned int avgWindow) const
{
    if (!mDataAsTarget && mTargetTopN > 1) {
        assert(mScoreTopNSet.find(set) != mScoreTopNSet.end());

        const std::string dataFileName = mName + "/SuccessTopN_" + fileName
                                         + ".dat";

        if (set == Database::Validation) {
            assert(mScoreTopNSet.find(Database::Learn) != mScoreTopNSet.end());

            // Save validation scores file
            std::ofstream dataFile(dataFileName);

            if (!dataFile.good())
                throw std::runtime_error("Could not create data rate log file: "
                                         + dataFileName);

            for (std::vector<std::pair<unsigned int, double> >::const_iterator
                     it = mValidationTopNScore.begin(),
                     itEnd = mValidationTopNScore.end();
                 it != itEnd;
                 ++it) {
                dataFile << (*it).first << " " << (*it).second << "\n";
            }

            dataFile.close();

            // Plot validation
            const double lastValidation = (!mValidationTopNScore.empty())
                ? mValidationTopNScore.back().second : 0.0;
            const double lastLearn
                = (!getTopNScore(Database::Learn).success.empty())
                    ? getTopNScore(Database::Learn).success.back() : 0.0;

            const double minFinalRate = std::min(lastValidation, lastLearn);
            const double maxFinalRate = std::max(lastValidation, lastLearn);

            std::ostringstream label;
            label << "\"Best top-" << mTargetTopN
                  << " accuracy validation: " << 100.0 * mMaxValidationTopNScore
                  << "%\" at graph 0.5, graph 0.15 front";

            Gnuplot multiplot;
            multiplot.saveToFile(dataFileName);
            multiplot.setMultiplot();

            Monitor::logDataRate(
                (*mScoreTopNSet.find(Database::Learn)).second.success,
                mName + "/" + fileName + "_LearningTopNSuccess.dat",
                avgWindow,
                true);

            multiplot << "clear";
            multiplot.setYrange(
                Utils::clamp(minFinalRate - (1.0 - minFinalRate), 0.0, 0.99),
                Utils::clamp(2.0 * maxFinalRate, 0.01, 1.0));
            multiplot.set("label", label.str());
            multiplot << ("replot \"" + dataFileName + "\" using 1:2 with "
                                                       "linespoints lt 7 title "
                                                       "\"Validation\"");
        } else
            Monitor::logDataRate((*mScoreTopNSet.find(set)).second.success,
                                 dataFileName,
                                 avgWindow,
                                 true);
    }
}

void N2D2::TargetScore::logConfusionMatrix(const std::string& fileName,
                                           Database::StimuliSet set) const
{
    assert(mScoreSet.find(set) != mScoreSet.end());

    const std::vector<std::string>& labelsName = (!mDataAsTarget)
        ? getTargetLabelsName()
        : std::vector<std::string>();

    (*mScoreSet.find(set)).second.confusionMatrix.log(
        mName + "/ConfusionMatrix_" + fileName + ".dat", labelsName);
}

void N2D2::TargetScore::logMisclassified(const std::string& fileName,
                                         Database::StimuliSet set) const
{
    assert(mScoreSet.find(set) != mScoreSet.end());

    if (mDataAsTarget)
        return;

    std::ofstream data((mName + "/Misclassified_" + fileName + ".dat").c_str());

    if (!data.good())
        throw std::runtime_error("Could not log misclassified stimuli file.");

    data << "# name target estimated\n";

    const std::vector<std::pair<unsigned int, unsigned int> >& misclassified
        = (*mScoreSet.find(set)).second.misclassified;

    for (std::vector<std::pair<unsigned int, unsigned int> >::const_iterator it
         = misclassified.begin(),
         itEnd = misclassified.end();
         it != itEnd;
         ++it) {
        const std::vector<int> cls = getTargetLabels((*it).second);
        const std::string name
            = mStimuliProvider->getDatabase().getStimulusName((*it).first);
        const int label
            = mStimuliProvider->getDatabase().getStimulusLabel((*it).first);

        data << (*it).first << " " << Utils::quoted(name) << " ";

        if (label >= 0) {
            if (label < (int)mStimuliProvider->getDatabase().getNbLabels())
                data << mStimuliProvider->getDatabase().getLabelName(label);
            else
                data << label;
        }
        else
            data << "*";

        data << " ";

        if (!cls.empty()) {
            if (cls[0] < (int)mStimuliProvider->getDatabase().getNbLabels())
                data << mStimuliProvider->getDatabase().getLabelName(cls[0]);
            else
                data << cls[0];

            if (cls.size() > 1)
                data << "...";
        }
        data << "\n";
    }
}

void N2D2::TargetScore::clearSuccess(Database::StimuliSet set)
{
    mScoreSet[set].success.clear();
    mScoreTopNSet[set].success.clear();
}

void N2D2::TargetScore::clearConfusionMatrix(Database::StimuliSet set)
{
    mScoreSet[set].confusionMatrix.clear();
}

void N2D2::TargetScore::clearMisclassified(Database::StimuliSet set)
{
    mScoreSet[set].misclassified.clear();
}

void N2D2::TargetScore::clearScore(Database::StimuliSet set)
{
    clearSuccess(set);
    clearConfusionMatrix(set);
    clearMisclassified(set);
}
void N2D2::TargetScore::computeScore(Database::StimuliSet set)
{
    if (mDataAsTarget) {
        ConfusionMatrix<unsigned long long int>& confusionMatrix
            = mScoreSet[set].confusionMatrix;

        if (confusionMatrix.empty()) {
            confusionMatrix.resize(mConfusionQuantSteps,
                                   mConfusionQuantSteps, 0);
        }

        std::shared_ptr<Cell_Frame_Top> targetCell 
            = std::dynamic_pointer_cast<Cell_Frame_Top>(mCell);
        std::shared_ptr<Cell_CSpike_Top> targetCellCSpike
            = std::dynamic_pointer_cast<Cell_CSpike_Top>(mCell);

        if (targetCell)
            targetCell->getOutputs().synchronizeDToH();
        else
            targetCellCSpike->getOutputsActivity().synchronizeDToH();

        const Tensor<Float_T>& values
            = (targetCell) ? tensor_cast<Float_T>(targetCell->getOutputs())
                           : tensor_cast<Float_T>
                                (targetCellCSpike->getOutputsActivity());

        mBatchSuccess.assign(values.dimB(), -1.0);

#pragma omp parallel for if (values.dimB() > 4 && values[0].size() > 1)
        for (int batchPos = 0; batchPos < (int)values.dimB(); ++batchPos) {
            const int id = mStimuliProvider->getBatch()[batchPos];

            if (id < 0) {
                // Invalid stimulus in batch (can occur for the last batch of the
                // set)
                continue;
            }

            const Tensor<Float_T> target = mStimuliProvider->getTargetData()[batchPos];
            const Tensor<Float_T> estimated = values[batchPos];

            ConfusionMatrix<unsigned long long int> confusion(
                mConfusionQuantSteps, mConfusionQuantSteps, 0);

            double mse = 0.0;

            for (size_t index = 0; index < target.size(); ++index) {
                const double err = target(index) - estimated(index);
                mse += err * err;

                const unsigned int t = Utils::clamp<unsigned int>(
                    Utils::round((mConfusionQuantSteps - 1)
                        * (target(index) - mConfusionRangeMin)
                            / (double)(mConfusionRangeMax - mConfusionRangeMin)),
                    0U, mConfusionQuantSteps - 1);
                const unsigned int e = Utils::clamp<unsigned int>(
                    Utils::round((mConfusionQuantSteps - 1)
                        * (estimated(index) - mConfusionRangeMin)
                            / (double)(mConfusionRangeMax - mConfusionRangeMin)),
                    0U, mConfusionQuantSteps - 1);

                confusion(t, e) += 1ULL;
            }

            if (target.size() > 0)
                mse /= target.size();

            mBatchSuccess[batchPos] = mse;

#pragma omp critical(TargetScore__process)
            std::transform(confusionMatrix.begin(), confusionMatrix.end(),
                        confusion.begin(),
                        confusionMatrix.begin(),
                        std::plus<unsigned long long int>());
        }
    }
    else {
        const unsigned int nbTargets = getNbTargets();

        ConfusionMatrix<unsigned long long int>& confusionMatrix
            = mScoreSet[set].confusionMatrix;
        std::vector<std::pair<unsigned int, unsigned int> >& misclassified
            = mScoreSet[set].misclassified;

        if (confusionMatrix.empty())
            confusionMatrix.resize(nbTargets, nbTargets, 0);

        mBatchSuccess.assign(mTargets.dimB(), -1.0);

        mEstimatedLabels.synchronizeDBasedToH();
        mEstimatedLabelsValue.synchronizeDBasedToH();

        if (mTargetTopN > 1)
            mBatchTopNSuccess.assign(mTargets.dimB(), -1.0);

#pragma omp parallel for if (mTargets.dimB() > 4 && mTargets[0].size() > 1)
        for (int batchPos = 0; batchPos < (int)mTargets.dimB(); ++batchPos) {
            const int id = mStimuliProvider->getBatch()[batchPos];

            if (id < 0) {
                // Invalid stimulus in batch (can occur for the last batch of the
                // set)
                continue;
            }

            const Tensor<int> target = mTargets[batchPos];
            const Tensor<int> estimatedLabels = mEstimatedLabels[batchPos];

            if (target.size() == 1) {
                if (target(0) >= 0) {
#pragma omp atomic
                    confusionMatrix(target(0), estimatedLabels(0)) += 1ULL;

                    mBatchSuccess[batchPos] = (estimatedLabels(0) == target(0));

                    if (!mBatchSuccess[batchPos]) {
#pragma omp critical(TargetScore__process)
                        misclassified.push_back(
                            std::make_pair(id, estimatedLabels(0)));
                    }

                    // Top-N case :
                    if (mTargetTopN > 1) {
                        unsigned int topNscore = 0;

                        for (unsigned int n = 0; n < mTargetTopN; ++n) {
                            if (estimatedLabels(n) == target(0))
                                ++topNscore;
                        }

                        mBatchTopNSuccess[batchPos] = (topNscore > 0);
                    }
                }
            } else {
                ConfusionMatrix<unsigned long long int> confusion(nbTargets,
                                                                nbTargets,
                                                                0);

                std::vector<unsigned int> nbHits(nbTargets, 0);
                std::vector<unsigned int> nbHitsTopN(nbTargets, 0);
                std::vector<unsigned int> nbLabels(nbTargets, 0);

                for (unsigned int oy = 0; oy < mTargets.dimY(); ++oy) {
                    for (unsigned int ox = 0; ox < mTargets.dimX(); ++ox) {
                        if (target(ox, oy, 0) >= 0) {
                            confusion(target(ox, oy, 0),
                                            estimatedLabels(ox, oy, 0)) += 1;

                            ++nbLabels[target(ox, oy, 0)];

                            if (target(ox, oy, 0)
                                == (int)estimatedLabels(ox, oy, 0))
                                ++nbHits[target(ox, oy, 0)];

                            // Top-N case :
                            if (mTargetTopN > 1) {
                                unsigned int topNscore = 0;

                                for (unsigned int n = 0; n < mTargetTopN; ++n) {
                                    if (estimatedLabels(ox, oy, n)
                                        == target(ox, oy, 0))
                                        ++topNscore;
                                }

                                if (topNscore > 0)
                                    ++nbHitsTopN[target(ox, oy, 0)];
                            }
                        }
                    }
                }

                double success = 0.0;
                double successTopN = 0.0;
                unsigned int nbValidTargets = 0;

                for (unsigned int t = 0; t < nbTargets; ++t) {
                    if (nbLabels[t] > 0) {
                        success += nbHits[t] / (double)nbLabels[t];
                        successTopN += nbHitsTopN[t] / (double)nbLabels[t];
                        ++nbValidTargets;
                    }
                }

                mBatchSuccess[batchPos] = (nbValidTargets > 0) ?
                    (success / nbValidTargets) : 1.0;

                if (mTargetTopN > 1) {
                    mBatchTopNSuccess[batchPos] = (nbValidTargets > 0) ?
                        (successTopN / nbValidTargets) : 1.0;
                }

#pragma omp critical(TargetScore__process)
                std::transform(confusionMatrix.begin(), confusionMatrix.end(),
                            confusion.begin(),
                            confusionMatrix.begin(),
                            std::plus<unsigned long long int>());
            }
        }
    }

    // Remove invalid/ignored batch positions before computing the score
    correctLastBatch(mBatchSuccess, mScoreSet[set].success);
    mScoreSet[set].success.push_back(getBatchAverageSuccess());

    if (!mDataAsTarget && mTargetTopN > 1) {
        correctLastBatch(mBatchTopNSuccess, mScoreTopNSet[set].success);
        mScoreTopNSet[set].success.push_back(getBatchAverageTopNSuccess());
    }
}

void N2D2::TargetScore::correctLastBatch(
    std::vector<double>& batchSuccess,
    const std::deque<double>& success)
{
    const size_t batchSize = batchSuccess.size();
    batchSuccess.erase(std::remove(batchSuccess.begin(),
                                    batchSuccess.end(), -1.0),
                        batchSuccess.end());

    if (batchSuccess.size() < batchSize) {
        // Compute the correct cumulative score sum
        const double scoreSum = batchSize
                * std::accumulate(success.begin(), success.end(), 0.0)
            + std::accumulate(batchSuccess.begin(), batchSuccess.end(), 0.0);

        // Compute the true average
        const double x = scoreSum
            / (success.size() * batchSize + batchSuccess.size());

        // Fill batchSuccess with true average
        batchSuccess.resize(batchSize, x);
    }
}

void N2D2::TargetScore::process(Database::StimuliSet set)
{
    Target::process(set);
    computeScore(set);
}

void N2D2::TargetScore::log(const std::string& fileName,
                            Database::StimuliSet set)
{
    logConfusionMatrix(fileName, set);
    logMisclassified(fileName, set);
}

void N2D2::TargetScore::clear(Database::StimuliSet set)
{
    Target::clear(set);
    clearConfusionMatrix(set);
    clearMisclassified(set);
}


#ifdef PYBIND
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

namespace N2D2 {
void init_TargetScore(py::module &m) {
    py::class_<TargetScore, std::shared_ptr<TargetScore>, Target>(m, "TargetScore", py::multiple_inheritance());
}
}
#endif
