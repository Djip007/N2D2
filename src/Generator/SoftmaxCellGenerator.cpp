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

#include "DeepNet.hpp"
#include "Generator/SoftmaxCellGenerator.hpp"
#include "third_party/half.hpp"

N2D2::Registrar<N2D2::CellGenerator>
N2D2::SoftmaxCellGenerator::mRegistrar(SoftmaxCell::Type,
                                       N2D2::SoftmaxCellGenerator::generate);

std::shared_ptr<N2D2::SoftmaxCell>
N2D2::SoftmaxCellGenerator::generate(Network& /*network*/, const DeepNet& deepNet,
                                     StimuliProvider& sp,
                                     const std::vector
                                     <std::shared_ptr<Cell> >& parents,
                                     IniParser& iniConfig,
                                     const std::string& section)
{
    if (!iniConfig.currentSection(section, false))
        throw std::runtime_error("Missing [" + section + "] section.");

    const std::string model = iniConfig.getProperty<std::string>(
        "Model", CellGenerator::mDefaultModel);
    const DataType dataType = iniConfig.getProperty<DataType>(
        "DataType", CellGenerator::mDefaultDataType);

    const unsigned int nbOutputs = iniConfig.getProperty
                                   <unsigned int>("NbOutputs");
    const unsigned int groupSize = iniConfig.getProperty
                                   <unsigned int>("GroupSize", 0U);

    const bool withLoss = iniConfig.getProperty<bool>("WithLoss", false);

    std::cout << "Layer: " << section << " [Softmax(" << model << ")]"
              << std::endl;

    // Cell construction
    std::shared_ptr<SoftmaxCell> cell
        = (dataType == Float32)
            ? Registrar<SoftmaxCell>::create<float>(model)(deepNet, section,
                                                           nbOutputs,
                                                           withLoss,
                                                           groupSize)
          : (dataType == Float16)
            ? Registrar<SoftmaxCell>::create<half_float::half>(model)(deepNet, section,
                                                                      nbOutputs,
                                                                      withLoss,
                                                                      groupSize)
            : Registrar<SoftmaxCell>::create<double>(model)(deepNet, section,
                                                           nbOutputs,
                                                           withLoss,
                                                           groupSize);

    if (!cell) {
        throw std::runtime_error(
            "Cell model \"" + model + "\" is not valid in section [" + section
            + "] in network configuration file: " + iniConfig.getFileName());
    }

    // Set configuration parameters defined in the INI file
    cell->setParameters(getConfig(model, iniConfig));

    // Load configuration file (if exists)
    cell->loadParameters(section + ".cfg", true);

    const unsigned int x0 = iniConfig.getProperty
                            <unsigned int>("InputOffsetX", 0);
    const unsigned int y0 = iniConfig.getProperty
                            <unsigned int>("InputOffsetY", 0);
    const unsigned int width = iniConfig.getProperty
                               <unsigned int>("InputWidth", 0);
    const unsigned int height = iniConfig.getProperty
                                <unsigned int>("InputHeight", 0);

    // Connect the cell to the parents
    for (std::vector<std::shared_ptr<Cell> >::const_iterator it
         = parents.begin(),
         itEnd = parents.end();
         it != itEnd;
         ++it) {
        if (!(*it))
            cell->addInput(sp, x0, y0, width, height);
        else
            cell->addInput((*it).get(), x0, y0, width, height);
    }

    std::cout << "  # Inputs dims: " << cell->getInputsDims() << std::endl;
    std::cout << "  # Outputs dims: " << cell->getOutputsDims() << std::endl;

    return cell;
}
