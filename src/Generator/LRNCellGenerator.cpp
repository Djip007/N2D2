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
#include "Generator/LRNCellGenerator.hpp"
#include "StimuliProvider.hpp"
#include "third_party/half.hpp"

N2D2::Registrar<N2D2::CellGenerator>
N2D2::LRNCellGenerator::mRegistrar(LRNCell::Type,
                                   N2D2::LRNCellGenerator::generate);

std::shared_ptr<N2D2::LRNCell>
N2D2::LRNCellGenerator::generate(Network& /*network*/, const DeepNet& deepNet,
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

    std::cout << "Layer: " << section << " [LRN(" << model << ")]" << std::endl;

    // Cell construction
    std::shared_ptr<LRNCell> cell
        = (dataType == Float32)
            ? Registrar<LRNCell>::create<float>(model)(deepNet, section, nbOutputs)
          : (dataType == Float16)
            ? Registrar<LRNCell>::create<half_float::half>(model)(deepNet, section,
                                                                  nbOutputs)
            : Registrar<LRNCell>::create<double>(model)(deepNet, section, nbOutputs);

    if (!cell) {
        throw std::runtime_error(
            "Cell model \"" + model + "\" is not valid in section [" + section
            + "] in network configuration file: " + iniConfig.getFileName());
    }

    // Set configuration parameters defined in the INI file
    cell->setParameters(getConfig(model, iniConfig));

    // Load configuration file (if exists)
    cell->loadParameters(section + ".cfg", true);

    // Connect the cell to the parents
    for (std::vector<std::shared_ptr<Cell> >::const_iterator it
         = parents.begin(),
         itEnd = parents.end();
         it != itEnd;
         ++it) {
        if (!(*it))
            cell->addInput(sp, 0, 0, sp.getSizeX(), sp.getSizeY());
        else if ((*it)->getOutputsWidth() > 1 || (*it)->getOutputsHeight() > 1)
            cell->addInput((*it).get());
        else {
            throw std::runtime_error("2D input expected for a LRNCell (\""
                                     + section + "\"), \"" + (*it)->getName()
                                     + "\" is not,"
                                       " in configuration file: "
                                     + iniConfig.getFileName());
        }
    }

    std::cout << "  # Inputs dims: " << cell->getInputsDims() << std::endl;
    std::cout << "  # Outputs dims: " << cell->getOutputsDims() << std::endl;

    return cell;
}
