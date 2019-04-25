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

#include "Target/Target.hpp"
#include "N2D2.hpp"
#include "StimuliProvider.hpp"
#include "Cell/Cell.hpp"
#include "Cell/Cell_Frame_Top.hpp"
#include "Cell/Cell_CSpike_Top.hpp"

N2D2::Registrar<N2D2::Target> N2D2::Target::mRegistrar("Target",
                                                       N2D2::Target::create);

const char* N2D2::Target::Type = "Target";

N2D2::Target::Target(const std::string& name,
                     const std::shared_ptr<Cell>& cell,
                     const std::shared_ptr<StimuliProvider>& sp,
                     double targetValue,
                     double defaultValue,
                     unsigned int targetTopN,
                     const std::string& labelsMapping_)
    : mDataAsTarget(this, "DataAsTarget", false),
      mNoDisplayLabel(this, "NoDisplayLabel", -1),
      mLabelsHueOffset(this, "LabelsHueOffset", 0),
      mEstimatedLabelsValueDisplay(this, "EstimatedLabelsValueDisplay", true),
      mMaskedLabel(this, "MaskedLabel", -1),
      mBinaryThreshold(this, "BinaryThreshold", 0.5),
      mImageLogFormat(this, "ImageLogFormat", "jpg"),
      mWeakTarget(this, "WeakTarget", -2),
      mName(name),
      mCell(cell),
      mStimuliProvider(sp),
      mTargetValue(targetValue),
      mDefaultValue(defaultValue),
      mTargetTopN(targetTopN),
      mDefaultTarget(-2),
      mPopulateTargets(true)
{
    // ctor
    Utils::createDirectories(name);

    if (!labelsMapping_.empty())
        labelsMapping(labelsMapping_);
}

unsigned int N2D2::Target::getNbTargets() const
{
    return (mCell->getNbOutputs() > 1) ? mCell->getNbOutputs() : 2;
}

void N2D2::Target::labelsMapping(const std::string& fileName)
{
    mLabelsMapping.clear();

    if (fileName.empty())
        return;

    std::ifstream clsFile(fileName.c_str());

    if (!clsFile.good())
        throw std::runtime_error("Could not open class mapping file: "
                                 + fileName);

    std::string line;

    while (std::getline(clsFile, line)) {
        // Remove optional comments
        line.erase(std::find(line.begin(), line.end(), '#'), line.end());
        // Left trim & right trim (right trim necessary for extra "!value.eof()"
        // check later)
        line.erase(
            line.begin(),
            std::find_if(line.begin(),
                         line.end(),
                         std::not1(std::ptr_fun<int, int>(std::isspace))));
        line.erase(std::find_if(line.rbegin(),
                                line.rend(),
                                std::not1(std::ptr_fun<int, int>(std::isspace)))
                       .base(),
                   line.end());

        if (line.empty())
            continue;

        std::string className;
        int output;

        std::stringstream value(line);
        std::stringstream classNameStr;

        int wordInString = std::count_if(line.begin(), line.end(), [](char ch) { return isspace(ch); });

        for(int idx = 0; idx < wordInString; ++idx)
        {
            std::string str;
            if (!(value >> Utils::quoted(str)))
                throw std::runtime_error("Unreadable class name: " + line + " in file "
                                         + fileName);
             if(idx > 0)
                classNameStr << " ";

             classNameStr << str;
        }

        className = classNameStr.str();

        //if (!(value >> Utils::quoted(className)) || !(value >> output)
        //    || (output < 0 && output != -1) || !value.eof())
        //    throw std::runtime_error("Unreadable value: " + line + " in file "
         //                            + fileName);

        if (!(value >> output) || (output < 0 && output != -1) || !value.eof())
            throw std::runtime_error("Unreadable value: " + line + " in file "
                                     + fileName);


        if (className == "default") {
            if (mDefaultTarget >= -1)
                throw std::runtime_error(
                    "Default mapping already exists in file " + fileName);

            mDefaultTarget = output;
        } else {
            std::vector<int> labels;

            if (className != "*") {
                if (mStimuliProvider->getDatabase().isLabel(className)) {
                    if (className.find_first_of("*?") != std::string::npos) {
                        throw std::runtime_error("Ambiguous use of wildcard: "
                            + line + ", because there is a label named \""
                            + className + "\" in the database, in file "
                            + fileName);
                    }

                    labels.push_back(mStimuliProvider->getDatabase()
                        .getLabelID(className));
                }
                else {
                    labels = mStimuliProvider->getDatabase()
                        .getMatchingLabelsIDs(className);
                }
            }
            else {
                if (mStimuliProvider->getDatabase().isLabel(className)) {
                    throw std::runtime_error("Ambiguous ignore wildcard *,"
                        " because there is a label named \"*\" in the database,"
                        " in file " + fileName);
                }

                labels.push_back(-1);
            }

            if (!labels.empty()) {
                for (std::vector<int>::const_iterator it = labels.begin(),
                    itEnd = labels.end(); it != itEnd; ++it)
                {
                    bool newInsert;
                    std::tie(std::ignore, newInsert)
                        = mLabelsMapping.insert(std::make_pair(*it, output));

                    if (!newInsert) {
                        throw std::runtime_error(
                            "Mapping already exists for label: " + line
                            + " in file " + fileName);
                    }
                }
            }
            else {
                std::cout
                    << Utils::cwarning
                    << "No label exists in the database with the name: "
                    << className << " in file " << fileName
                    << Utils::cdef << std::endl;
            }
        }
    }
}

void N2D2::Target::setLabelTarget(int label, int output)
{
    mLabelsMapping[label] = output;
}

void N2D2::Target::setDefaultTarget(int output)
{
    mDefaultTarget = output;
}

int N2D2::Target::getLabelTarget(int label) const
{
    if (mLabelsMapping.empty())
        return label;
    else {
        const std::map<int, int>::const_iterator it
            = mLabelsMapping.find(label);

        if (it != mLabelsMapping.end())
            return (*it).second;
        else if (mDefaultTarget >= -1)
            return mDefaultTarget;
        else {
            #pragma omp critical
            {
                std::stringstream labelStr;
                labelStr << label;

                const std::string labelName
                    = mStimuliProvider->getDatabase().getLabelName(label);

                throw std::runtime_error(
                    "Incomplete class mapping: no output specified for label #"
                    + labelStr.str() + " (" + labelName + ")");
            }
        }
    }
}

int N2D2::Target::getDefaultTarget() const
{
    if (mDefaultTarget >= -1)
        return mDefaultTarget;
    else
        throw std::runtime_error("No default target mapping");
}

std::vector<int> N2D2::Target::getTargetLabels(int output) const
{
    if (mLabelsMapping.empty())
        return std::vector<int>(1, output);
    else {
        std::vector<int> labels;

        for (std::map<int, int>::const_iterator it = mLabelsMapping.begin(),
                                                itEnd = mLabelsMapping.end();
             it != itEnd;
             ++it) {
            if ((*it).second == output)
                labels.push_back((*it).first);
        }

        return labels;
    }
}

std::vector<std::string> N2D2::Target::getTargetLabelsName() const
{
    const unsigned int nbTargets = getNbTargets();
    std::vector<std::string> labelsName;
    labelsName.reserve(nbTargets);

    for (int target = 0; target < (int)nbTargets; ++target) {
        std::stringstream labelName;

        if (target == mDefaultTarget)
            labelName << "default";
        else {
            const std::vector<int> cls = getTargetLabels(target);
            const Database& db = mStimuliProvider->getDatabase();

            if (!cls.empty()) {
                labelName << ((cls[0] >= 0 && cls[0] < (int)db.getNbLabels())
                                ? db.getLabelName(cls[0]) :
                              (cls[0] >= 0)
                                ? "" :
                                  "*");

                if (cls.size() > 1)
                    labelName << "...";
            }
        }

        labelsName.push_back(labelName.str());
    }

    return labelsName;
}

void N2D2::Target::logLabelsMapping(const std::string& fileName) const
{
    if (mDataAsTarget)
        return;

    const std::string dataFileName = mName + "/" + fileName + ".dat";
    std::ofstream labelsData(dataFileName);

    if (!labelsData.good())
        throw std::runtime_error("Could not save log class mapping data file: "
                                 + dataFileName);

    labelsData << "label name output\n";

    for (unsigned int label = 0,
                      size = mStimuliProvider->getDatabase().getNbLabels();
         label < size;
         ++label)
        labelsData << label << " "
                   << mStimuliProvider->getDatabase().getLabelName(label) << " "
                   << getLabelTarget(label) << "\n";
}

void N2D2::Target::process(Database::StimuliSet set)
{
    const unsigned int nbTargets = getNbTargets();

    if (mDataAsTarget) {
        if (set == Database::Learn) {
            std::shared_ptr<Cell_Frame_Top> targetCell
                = std::dynamic_pointer_cast<Cell_Frame_Top>(mCell);

            // Set targets
            mLoss.push_back(targetCell->
                            setOutputTargets(mStimuliProvider->getData()));
        }
    } else {
        const Tensor<int>& labels = mStimuliProvider->getLabelsData();

        if (mTargets.empty()) {
            mTargets.resize({mCell->getOutputsWidth(),
                            mCell->getOutputsHeight(),
                            1,
                            labels.dimB()});
            mEstimatedLabels.resize({mCell->getOutputsWidth(),
                                    mCell->getOutputsHeight(),
                                    mTargetTopN,
                                    labels.dimB()});
            mEstimatedLabelsValue.resize({mCell->getOutputsWidth(),
                                         mCell->getOutputsHeight(),
                                         mTargetTopN,
                                         labels.dimB()});
        }

        if (mPopulateTargets) {
            // Generate targets
            const size_t size = mTargets.dimB() * mTargets.dimY();

            if (mTargets.dimX() != labels.dimX()
                || mTargets.dimY() != labels.dimY())
            {
                const double xRatio = labels.dimX() / (double)mTargets.dimX();
                const double yRatio = labels.dimY() / (double)mTargets.dimY();

#if defined(_OPENMP) && _OPENMP >= 200805
#pragma omp parallel for collapse(2) if (size > 16)
#else
#pragma omp parallel for if (mTargets.dimB() > 4 && size > 16)
#endif
                for (int batchPos = 0; batchPos < (int)mTargets.dimB();
                    ++batchPos)
                {
                    for (int y = 0; y < (int)mTargets.dimY(); ++y) {
                        for (int x = 0; x < (int)mTargets.dimX(); ++x) {
                            const unsigned int xl0 = std::floor(x * xRatio);
                            const unsigned int xl1 = std::max(xl0 + 1,
                                (unsigned int)std::floor((x + 1) * xRatio));
                            const unsigned int yl0 = std::floor(y * yRatio);
                            const unsigned int yl1 = std::max(yl0 + 1,
                                (unsigned int)std::floor((y + 1) * yRatio));

                            // +1 takes into account ignore target (-1)
                            std::vector<int> targetHist(labels.dimB() + 1, 0);

                            for (unsigned int yl = yl0; yl < yl1; ++yl) {
                                for (unsigned int xl = xl0; xl < xl1; ++xl) {
                                    assert(getLabelTarget(
                                        labels(xl, yl, 0, batchPos)) >= -1);

                                    ++targetHist[getLabelTarget(
                                        labels(xl, yl, 0, batchPos)) + 1];
                                }
                            }

                            if (mWeakTarget >= -1) {
                                // initialize original index locations
                                // first index is -1 (ignore)
                                std::vector<int> targetHistIdx(
                                        targetHist.size());
                                std::iota(targetHistIdx.begin(),
                                        targetHistIdx.end(), -1); // -1 = ignore

                                // sort indexes based on comparing values in 
                                // targetHist. Sort in descending order.
                                std::partial_sort(targetHistIdx.begin(),
                                    targetHistIdx.begin() + 2,
                                    targetHistIdx.end(),
                                    [&targetHist](int i1, int i2)
                                        {return targetHist[i1 + 1]
                                                    > targetHist[i2 + 1];});

                                mTargets(x, y, 0, batchPos)
                                    = (targetHistIdx[0] == mWeakTarget
                                        && targetHistIdx[1] > 0)
                                            ? targetHistIdx[1]
                                            : targetHistIdx[0];
                            }
                            else {
                                std::vector<int>::iterator maxElem
                                    = std::max_element(targetHist.begin(),
                                                       targetHist.end());

                                mTargets(x, y, 0, batchPos)
                                    = std::distance(targetHist.begin(),
                                                    maxElem) - 1; // -1 = ignore
                            }
                        }
                    }
                }
            }
            else {
                // one-to-one mapping
#pragma omp parallel for if (mTargets.dimB() > 64 && size > 256)
                for (int batchPos = 0; batchPos < (int)mTargets.dimB();
                    ++batchPos)
                {
                    // target only has 1 channel, whereas label has as many
                    // channels as environment channels
                    const Tensor<int> label = labels[batchPos][0];
                    Tensor<int> target = mTargets[batchPos][0];

                    for (int index = 0; index < (int)label.size(); ++index)
                        target(index) = getLabelTarget(label(index));
                }
            }
        }

        std::shared_ptr<Cell_Frame_Top> targetCell = std::dynamic_pointer_cast
            <Cell_Frame_Top>(mCell);

        if (set == Database::Learn) {
            // Set targets
            if (mTargets.dimX() == 1 && mTargets.dimY() == 1) {
                for (unsigned int batchPos = 0; batchPos < mTargets.dimB();
                     ++batchPos) {
                    if (mTargets(0, batchPos) < 0) {
                        std::cout << Utils::cwarning
                                  << "Target::setTargetsValue(): ignore label "
                                     "with 1D output for stimuli ID "
                                  << mStimuliProvider->getBatch()[batchPos]
                                  << Utils::cdef << std::endl;
                    }
                }

                mLoss.push_back(targetCell->setOutputTarget(
                    mTargets, mTargetValue, mDefaultValue));
            } else
                mLoss.push_back(targetCell->setOutputTargets(
                    mTargets, mTargetValue, mDefaultValue));
        }

        // Retrieve estimated labels
        std::shared_ptr<Cell_CSpike_Top> targetCellCSpike
            = std::dynamic_pointer_cast<Cell_CSpike_Top>(mCell);

        if (targetCell)
            targetCell->getOutputs().synchronizeDToH();
        else
            targetCellCSpike->getOutputsActivity().synchronizeDToH();

        const Tensor<Float_T>& values
            = (targetCell) ? tensor_cast<Float_T>(targetCell->getOutputs())
                           : tensor_cast<Float_T>(
                                        targetCellCSpike->getOutputsActivity());
        const unsigned int nbOutputs = values.dimZ();

        if (mTargetTopN > nbOutputs) {
            throw std::runtime_error("Target::process(): target 'TopN' "
                                    "parameter must be <= to the network "
                                    "output size");
        }

        // Find batchSize to ignore invalid stimulus in batch (can occur for the 
        // last batch of the set)
        int batchSize = 0;

        if (mStimuliProvider->getBatch().back() >= 0)
            batchSize = (int)mTargets.dimB();
        else {
            for (; batchSize < (int)mTargets.dimB(); ++batchSize) {
                const int id = mStimuliProvider->getBatch()[batchSize];

                if (id < 0)
                    break;
            }
        }

        const size_t size = values.dimY() * batchSize;

        std::vector<int> outputsIdx(nbOutputs);

        if (nbOutputs > 1 && mTargetTopN > 1)
            std::iota(outputsIdx.begin(), outputsIdx.end(), 0);

#if defined(_OPENMP) && _OPENMP >= 200805
#pragma omp parallel for collapse(2) if (size > 16) schedule(dynamic)
#else
#pragma omp parallel for if (batchSize > 4 && size > 16)
#endif
        for (int batchPos = 0; batchPos < (int)batchSize; ++batchPos)
        {
            for (int oy = 0; oy < (int)values.dimY(); ++oy) {
                for (int ox = 0; ox < (int)values.dimX(); ++ox) {
                    const int id = mStimuliProvider->getBatch()[batchPos];
                    assert(id >= 0);

                    if (mTargets(ox, oy, 0, batchPos) >= (int)nbTargets) {
#pragma omp critical(Target__process)
                        {
                            std::cout << Utils::cwarning << "Stimulus #"
                                << id << " has target "
                                << mTargets(ox, oy, 0, batchPos)
                                << " @ (" << ox << "," << oy << ") but "
                                "number of output target is " << nbTargets
                                << Utils::cdef << std::endl;

                            throw std::runtime_error("Target::process(): "
                                                        "target out of "
                                                        "range.");
                        }
                    }

                    if (nbOutputs > 1 && mTargetTopN > 1) {
                        // initialize original index locations
                        std::vector<int> sortedLabelsIdx(outputsIdx.begin(),
                                                         outputsIdx.end());

                        // sort indexes based on comparing values
                        std::partial_sort(sortedLabelsIdx.begin(),
                            sortedLabelsIdx.begin() + mTargetTopN,
                            sortedLabelsIdx.end(),
                            [&values, &ox, &oy, &batchPos](int i1, int i2)
                                {return values(ox, oy, i1, batchPos)
                                            > values(ox, oy, i2, batchPos);});

                        for (unsigned int i = 0; i < mTargetTopN; ++i) {
                            mEstimatedLabels(ox, oy, i, batchPos)
                                = sortedLabelsIdx[i];
                            mEstimatedLabelsValue(ox, oy, i, batchPos)
                                = values(ox, oy, sortedLabelsIdx[i], batchPos);
                        }
                    }
                    else if (nbOutputs > 1) {
                        size_t maxIdx = 0;
                        Float_T maxVal = values(ox, oy, 0, batchPos);

                        for (size_t i = 1; i < nbOutputs; ++i) {
                            if (values(ox, oy, i, batchPos) > maxVal) {
                                maxIdx = i;
                                maxVal = values(ox, oy, i, batchPos);
                            }
                        }

                        mEstimatedLabels(ox, oy, 0, batchPos) = maxIdx;
                        mEstimatedLabelsValue(ox, oy, 0, batchPos) = maxVal;
                    }
                    else {
                        mEstimatedLabels(ox, oy, 0, batchPos)
                            = (values(ox, oy, 0, batchPos) > mBinaryThreshold);
                        mEstimatedLabelsValue(ox, oy, 0, batchPos)
                            = (mEstimatedLabels(ox, oy, 0, batchPos) == 1)
                                    ? values(ox, oy, 0, batchPos)
                                    : (1.0 - values(ox, oy, 0, batchPos));
                    }

                    if (values.dimX() == 1 && values.dimY() == 1) {
                        static bool display = true;

                        if (set == Database::Test && batchPos == 0 && display) {
                            std::cout << "[";

                            for (int i = 0; i < (int)nbOutputs; ++i) {
                                if (i == mEstimatedLabels(ox, oy, 0, batchPos))
                                {
                                    std::cout << std::setprecision(2)
                                        << std::fixed
                                        << "(" << values(ox, oy, i, batchPos)
                                        << ") ";
                                }
                                else {
                                    std::cout << std::setprecision(2)
                                        << std::fixed
                                        << values(ox, oy, i, batchPos) << " ";
                                }
                            }

                            std::cout << "]" << std::endl;
                            display = false;
                        }
                    }
                }
            }
        }
    }
}

void N2D2::Target::logEstimatedLabels(const std::string& dirName) const
{
    const std::string dirPath = mName + "/" + dirName;
    Utils::createDirectories(dirPath);

    if (mTargets.dimX() == 1 && mTargets.dimY() == 1) {
#if !defined(WIN32) && !defined(__CYGWIN__) && !defined(_WIN32)
        const int ret = symlink(N2D2_PATH("tools/roc.py"),
                                (dirPath + "_roc.py").c_str());
        if (ret < 0) {
        } // avoid ignoring return value warning
#endif

        std::shared_ptr<Cell_Frame_Top> targetCell = std::dynamic_pointer_cast
            <Cell_Frame_Top>(mCell);
        std::shared_ptr<Cell_CSpike_Top> targetCellCSpike
            = std::dynamic_pointer_cast<Cell_CSpike_Top>(mCell);

        if (targetCell)
            targetCell->getOutputs().synchronizeDToH();
        else
            targetCellCSpike->getOutputsActivity().synchronizeDToH();

        const Tensor<Float_T>& values
            = (targetCell) ? tensor_cast_nocopy<Float_T>(targetCell->getOutputs())
                           : tensor_cast_nocopy<Float_T>
                                (targetCellCSpike->getOutputsActivity());
        const unsigned int nbOutputs = values.dimZ();

        const std::string fileName = dirPath + "/classif.log";

        std::ofstream data(fileName, std::ofstream::app);

        if (!data.good()) {
            throw std::runtime_error("Could not save log classif data file: "
                                    + fileName);
        }

        for (int batchPos = 0; batchPos < (int)mTargets.dimB(); ++batchPos) {
            const int id = mStimuliProvider->getBatch()[batchPos];

            if (id < 0) {
                // Invalid stimulus in batch (can occur for the last batch of the
                // set)
                continue;
            }

            const Tensor<Float_T> value = values[batchPos];
            const Tensor<int> target = mTargets[batchPos][0];
            const Tensor<int> estimatedLabels = mEstimatedLabels[batchPos][0];
            const Tensor<Float_T> estimatedLabelsValue
                = mEstimatedLabelsValue[batchPos][0];

            const std::string imgFile
                = mStimuliProvider->getDatabase().getStimulusName(id);

            data << id
                << " " << imgFile
                << " " << target(0)
                << " " << estimatedLabels(0)
                << " " << estimatedLabelsValue(0);

            for (int i = 0; i < (int)nbOutputs; ++i)
                data << " " << value(i);

            data << "\n";
        }

        return;
    }

#if !defined(WIN32) && !defined(__CYGWIN__) && !defined(_WIN32)
    const int ret = symlink(N2D2_PATH("tools/target_viewer.py"),
                            (dirPath + ".py").c_str());
    if (ret < 0) {
    } // avoid ignoring return value warning
#endif

    if (mDataAsTarget) {
        std::shared_ptr<Cell_Frame_Top> targetCell = std::dynamic_pointer_cast
            <Cell_Frame_Top>(mCell);
        std::shared_ptr<Cell_CSpike_Top> targetCellCSpike
            = std::dynamic_pointer_cast<Cell_CSpike_Top>(mCell);
        const int size = mStimuliProvider->getBatch().size();

#pragma omp parallel for if (size > 4)
        for (int batchPos = 0; batchPos < size; ++batchPos) {
            const int id = mStimuliProvider->getBatch()[batchPos];

            if (id < 0) {
                // Invalid stimulus in batch (can occur for the last batch of
                // the set)
                continue;
            }

            // Retrieve estimated labels
            if (targetCell)
                targetCell->getOutputs().synchronizeDToH();
            else
                targetCellCSpike->getOutputsActivity().synchronizeDToH();

            const Tensor<Float_T>& values
                = (targetCell) ? tensor_cast_nocopy<Float_T>(targetCell->getOutputs())
                               : tensor_cast_nocopy<Float_T>
                                    (targetCellCSpike->getOutputsActivity());
            const std::string imgFile
                = mStimuliProvider->getDatabase().getStimulusName(id);
            const std::string baseName = Utils::baseName(imgFile);
            const std::string fileBaseName = Utils::fileBaseName(baseName);
            std::string fileExtension = Utils::fileExtension(baseName);

            if (!((std::string)mImageLogFormat).empty()) {
                // Keep "[x,y]" after file extension, appended by
                // getStimulusName() in case of slicing
                fileExtension.replace(0, fileExtension.find_first_of('['),
                                      mImageLogFormat);
            }

            // Input image
            cv::Mat inputImg = (cv::Mat)mStimuliProvider->getData(0, batchPos);
            cv::Mat inputImg8U;
            inputImg.convertTo(inputImg8U, CV_8U, 255.0);

            std::string fileName = dirPath + "/" + fileBaseName + "_target."
                                   + fileExtension;

            if (!cv::imwrite(fileName, inputImg8U)) {
#pragma omp critical(Target__logEstimatedLabels)
                throw std::runtime_error("Unable to write image: " + fileName);
            }

            // Output image
            const cv::Mat outputImg = (cv::Mat)values[batchPos][0];
            cv::Mat outputImg8U;
            outputImg.convertTo(outputImg8U, CV_8U, 255.0);

            fileName = dirPath + "/" + fileBaseName + "_estimated."
                       + fileExtension;

            if (!cv::imwrite(fileName, outputImg8U)) {
#pragma omp critical(Target__logEstimatedLabels)
                throw std::runtime_error("Unable to write image: " + fileName);
            }
        }

        return;
    }

    const unsigned int nbTargets = getNbTargets();

#pragma omp parallel for if (mTargets.dimB() > 4)
    for (int batchPos = 0; batchPos < (int)mTargets.dimB(); ++batchPos) {
        const int id = mStimuliProvider->getBatch()[batchPos];

        if (id < 0) {
            // Invalid stimulus in batch (can occur for the last batch of the
            // set)
            continue;
        }

        const Tensor<int> target = mTargets[batchPos][0];
        const Tensor<int> estimatedLabels = mEstimatedLabels[batchPos][0];
        const Tensor<Float_T> estimatedLabelsValue
            = mEstimatedLabelsValue[batchPos][0];

        cv::Mat targetImgHsv(cv::Size(mTargets.dimX(), mTargets.dimY()),
                             CV_8UC3,
                             cv::Scalar(0, 0, 0));
        cv::Mat estimatedImgHsv(cv::Size(mTargets.dimX(), mTargets.dimY()),
                                CV_8UC3,
                                cv::Scalar(0, 0, 0));

        const Tensor<int> mask = (mMaskLabelTarget && mMaskedLabel >= 0)
            ? mMaskLabelTarget->getEstimatedLabels()[batchPos][0]
            : Tensor<int>();

        if (!mask.empty() && mask.dims() != target.dims()) {
            std::ostringstream errorStr;
            errorStr << "Mask dims (" << mask.dims() << ") from MaskLabelTarget"
                " does not match target dims (" << target.dims() << ") for"
                " target \"" << mName << "\"";

#pragma omp critical(Target__logEstimatedLabels)
            throw std::runtime_error(errorStr.str());
        }

        for (unsigned int oy = 0; oy < mTargets.dimY(); ++oy) {
            for (unsigned int ox = 0; ox < mTargets.dimX(); ++ox) {
                const int targetHue = (180 * target(ox, oy) / nbTargets
                                       + mLabelsHueOffset) % 180;

                targetImgHsv.at<cv::Vec3b>(oy, ox)
                    = (target(ox, oy) >= 0)
                        ? ((target(ox, oy) != mNoDisplayLabel)
                            ? cv::Vec3f(targetHue, 255, 255)
                            : cv::Vec3f(targetHue, 10, 127)) // no color
                        : cv::Vec3f(0, 0, 127); // ignore = no color

                const int estimatedHue = (180 * estimatedLabels(ox, oy)
                                          / nbTargets + mLabelsHueOffset) % 180;

                estimatedImgHsv.at<cv::Vec3b>(oy, ox)
                    = (mask.empty() || mask(ox, oy) == mMaskedLabel)
                        ? ((estimatedLabels(ox, oy) != mNoDisplayLabel)
                            ? cv::Vec3f(estimatedHue, 255,
                                       (mEstimatedLabelsValueDisplay)
                                           ? 255 * estimatedLabelsValue(ox, oy)
                                           : 255)
                            : cv::Vec3f(estimatedHue, 10, 127)) // no color
                        : cv::Vec3f(0, 0, 127); // not masked = no color
            }
        }

        const double alpha = 0.75;
        const std::string imgFile
            = mStimuliProvider->getDatabase().getStimulusName(id);
        const std::string baseName = Utils::baseName(imgFile);
        const std::string fileBaseName = Utils::fileBaseName(baseName);
        std::string fileExtension = Utils::fileExtension(baseName);

        if (!((std::string)mImageLogFormat).empty()) {
            // Keep "[x,y]" after file extension, appended by
            // getStimulusName() in case of slicing
            fileExtension.replace(0, fileExtension.find_first_of('['),
                                  mImageLogFormat);
        }

        // Input image
        cv::Mat inputImg = (cv::Mat)mStimuliProvider->getData(0, batchPos);
        cv::Mat inputImg8U;
        // inputImg.convertTo(inputImg8U, CV_8U, 255.0);

        // Normalize image
        cv::Mat inputImgNorm;
        cv::normalize(
            inputImg.reshape(1), inputImgNorm, 0, 255, cv::NORM_MINMAX);
        inputImg = inputImgNorm.reshape(inputImg.channels());
        inputImg.convertTo(inputImg8U, CV_8U);

        cv::Mat inputImgColor;
#if CV_MAJOR_VERSION >= 3
        cv::cvtColor(inputImg8U, inputImgColor, cv::COLOR_GRAY2BGR);
#else
        cv::cvtColor(inputImg8U, inputImgColor, CV_GRAY2BGR);
#endif

        // Target image
        cv::Mat imgColor;
#if CV_MAJOR_VERSION >= 3
        cv::cvtColor(targetImgHsv, imgColor, cv::COLOR_HSV2BGR);
#else
        cv::cvtColor(targetImgHsv, imgColor, CV_HSV2BGR);
#endif

        cv::Mat imgSampled;
        cv::resize(imgColor,
                   imgSampled,
                   cv::Size(mStimuliProvider->getSizeX(),
                            mStimuliProvider->getSizeY()),
                   0.0,
                   0.0,
                   cv::INTER_NEAREST);

        cv::Mat imgBlended;
        cv::addWeighted(
            inputImgColor, alpha, imgSampled, 1 - alpha, 0.0, imgBlended);

        std::string fileName = dirPath + "/" + fileBaseName + "_target."
                               + fileExtension;

        if (!cv::imwrite(fileName, imgBlended)) {
#pragma omp critical(Target__logEstimatedLabels)
            throw std::runtime_error("Unable to write image: " + fileName);
        }

        // Estimated image
#if CV_MAJOR_VERSION >= 3
        cv::cvtColor(estimatedImgHsv, imgColor, cv::COLOR_HSV2BGR);
#else
        cv::cvtColor(estimatedImgHsv, imgColor, CV_HSV2BGR);
#endif

        cv::resize(imgColor,
                   imgSampled,
                   cv::Size(mStimuliProvider->getSizeX(),
                            mStimuliProvider->getSizeY()),
                   0.0,
                   0.0,
                   cv::INTER_NEAREST);

        cv::addWeighted(
            inputImgColor, alpha, imgSampled, 1 - alpha, 0.0, imgBlended);

        fileName = dirPath + "/" + fileBaseName + "_estimated." + fileExtension;

        if (!cv::imwrite(fileName, imgBlended)) {
#pragma omp critical(Target__logEstimatedLabels)
            throw std::runtime_error("Unable to write image: " + fileName);
        }
    }
}

void N2D2::Target::logLabelsLegend(const std::string& fileName) const
{
    if (mDataAsTarget)
        return;

    if (mTargets.dimX() == 1 && mTargets.dimY() == 1)
        return;

    // Legend image
    const unsigned int margin = 5;
    const unsigned int labelWidth = 300;
    const unsigned int cellWidth = 50;
    const unsigned int cellHeight = 50;

    const unsigned int nbTargets = getNbTargets();
    const std::vector<std::string>& labelsName = getTargetLabelsName();

    cv::Mat legendImg(cv::Size(cellWidth + labelWidth, cellHeight * nbTargets),
                      CV_8UC3,
                      cv::Scalar(0, 0, 0));

    for (unsigned int target = 0; target < nbTargets; ++target) {
        cv::rectangle(
            legendImg,
            cv::Point(margin, target * cellHeight + margin),
            cv::Point(cellWidth - margin, (target + 1) * cellHeight - margin),
            cv::Scalar((180 * target / nbTargets + mLabelsHueOffset) % 180,
                        255, 255),
#if CV_MAJOR_VERSION >= 3
            cv::FILLED);
#else
            CV_FILLED);
#endif

        std::stringstream legendStr;
        legendStr << target << " " << labelsName[target];

        int baseline = 0;
        const cv::Size textSize = cv::getTextSize(
            legendStr.str(), cv::FONT_HERSHEY_SIMPLEX, 1.0, 2, &baseline);
        cv::putText(legendImg,
                    legendStr.str(),
                    cv::Point(cellWidth + margin,
                              (target + 1) * cellHeight
                              - (cellHeight - textSize.height) / 2.0),
                    cv::FONT_HERSHEY_SIMPLEX,
                    1.0,
                    cv::Scalar(0, 0, 255),
                    2);
    }

    cv::Mat imgColor;
#if CV_MAJOR_VERSION >= 3
    cv::cvtColor(legendImg, imgColor, cv::COLOR_HSV2BGR);
#else
    cv::cvtColor(legendImg, imgColor, CV_HSV2BGR);
#endif

    if (!cv::imwrite(mName + "/" + fileName, imgColor))
        throw std::runtime_error("Unable to write image: " + mName + "/"
                                 + fileName);
}

std::pair<int, N2D2::Float_T>
N2D2::Target::getEstimatedLabel(const std::shared_ptr<ROI>& roi,
                                unsigned int batchPos) const
{
    const Tensor<int>& labels = mStimuliProvider->getLabelsData();
    const double xRatio = labels.dimX() / (double)mCell->getOutputsWidth();
    const double yRatio = labels.dimY() / (double)mCell->getOutputsHeight();

    const cv::Rect rect = roi->getBoundingRect();
    const unsigned int x0 = rect.tl().x / xRatio;
    const unsigned int y0 = rect.tl().y / yRatio;
    const unsigned int x1 = rect.br().x / xRatio;
    const unsigned int y1 = rect.br().y / yRatio;

    std::shared_ptr<Cell_Frame_Top> targetCell = std::dynamic_pointer_cast
        <Cell_Frame_Top>(mCell);
    std::shared_ptr<Cell_CSpike_Top> targetCellCSpike
        = std::dynamic_pointer_cast<Cell_CSpike_Top>(mCell);
    
    if (targetCell)
        targetCell->getOutputs().synchronizeDToH();
    else
        targetCellCSpike->getOutputsActivity().synchronizeDToH();

    const Tensor<Float_T>& value
        = (targetCell) ? tensor_cast_nocopy<Float_T>(targetCell->getOutputs())[batchPos]
                       : tensor_cast_nocopy<Float_T>
                        (targetCellCSpike->getOutputsActivity())[batchPos];

    if (x1 >= value.dimX() || y1 >= value.dimY())
        throw std::runtime_error(
            "Target::getEstimatedLabel(): bounding box out of range");

    const unsigned int nbOutputs = value.dimZ();
    std::vector<Float_T> bbLabels((nbOutputs > 1) ? nbOutputs : 2, 0.0);

    for (unsigned int oy = y0; oy <= y1; ++oy) {
        for (unsigned int ox = x0; ox <= x1; ++ox) {
            if (nbOutputs > 1) {
                for (unsigned int index = 0; index < nbOutputs; ++index)
                    bbLabels[index] += value(ox, oy, index);
            } else {
                bbLabels[0] += 1.0 - value(ox, oy, 0);
                bbLabels[1] += value(ox, oy, 0);
            }
        }
    }

    const std::vector<Float_T>::const_iterator it
        = std::max_element(bbLabels.begin(), bbLabels.end());
    return std::make_pair(it - bbLabels.begin(),
                          (*it) / ((x1 - x0 + 1) * (y1 - y0 + 1)));
}

void N2D2::Target::clear(Database::StimuliSet /*set*/)
{
    mLoss.clear();
}
