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

#include "Generator/PoolCellGenerator.hpp"

N2D2::Registrar<N2D2::CellGenerator>
N2D2::PoolCellGenerator::mRegistrar(PoolCell::Type,
                                    N2D2::PoolCellGenerator::generate);

std::shared_ptr<N2D2::PoolCell>
N2D2::PoolCellGenerator::generate(Network& network,
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

    std::cout << "Layer: " << section << " [Pool(" << model << ")]"
              << std::endl;

    std::vector<unsigned int> poolDims;

    if (iniConfig.isProperty("PoolDims")) {
        poolDims = iniConfig.getProperty
                                     <std::vector<unsigned int> >("PoolDims");
    }
    else if (iniConfig.isProperty("Pool"))
        poolDims.resize(2, iniConfig.getProperty<unsigned int>("Pool"));
    else {
        poolDims.push_back(iniConfig.getProperty
                                     <unsigned int>("PoolWidth"));
        poolDims.push_back(iniConfig.getProperty
                                     <unsigned int>("PoolHeight"));

        if (iniConfig.isProperty("PoolDepth")) {
            poolDims.push_back(iniConfig.getProperty
                                         <unsigned int>("PoolDepth"));
        }
    }

    const unsigned int nbChannels = iniConfig.getProperty
                                    <unsigned int>("NbChannels");

    std::vector<unsigned int> strideDims;

    if (iniConfig.isProperty("StrideDims")) {
        strideDims = iniConfig.getProperty
                                <std::vector<unsigned int> >("StrideDims");
    }
    else if (iniConfig.isProperty("Stride")) {
        strideDims.resize(poolDims.size(),
                             iniConfig.getProperty<unsigned int>("Stride"));
    }
    else {
        strideDims.push_back(iniConfig.getProperty
                                     <unsigned int>("StrideX", 1));
        strideDims.push_back(iniConfig.getProperty
                                     <unsigned int>("StrideY", 1));
        strideDims.push_back(iniConfig.getProperty
                                     <unsigned int>("StrideZ", 1));
        strideDims.resize(poolDims.size(), 1);
    }

    std::vector<unsigned int> paddingDims;

    if (iniConfig.isProperty("PaddingDims")) {
        paddingDims = iniConfig.getProperty
                                <std::vector<unsigned int> >("PaddingDims");
    }
    else if (iniConfig.isProperty("Padding")) {
        paddingDims.resize(poolDims.size(),
                           iniConfig.getProperty<int unsigned>("Padding"));
    }
    else {
        paddingDims.push_back(iniConfig.getProperty
                              <int unsigned>("PaddingX", 0));
        paddingDims.push_back(iniConfig.getProperty
                              <int unsigned>("PaddingY", 0));
        paddingDims.push_back(iniConfig.getProperty
                              <int unsigned>("PaddingZ", 0));
        paddingDims.resize(poolDims.size(), 0);
    }

    const PoolCell::Pooling pooling = iniConfig.getProperty
                                      <PoolCell::Pooling>("Pooling");

    std::shared_ptr<Activation> activation
        = ActivationGenerator::generate(
            iniConfig, section, model, dataType, "ActivationFunction");

    // Cell construction
    std::shared_ptr<PoolCell> cell = Registrar
        <PoolCell>::create(model)(network,
                                  section,
                                  poolDims,
                                  nbChannels,
                                  strideDims,
                                  paddingDims,
                                  pooling,
                                  activation);

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
    MappingGenerator::Mapping defaultMapping
        = MappingGenerator::getMapping(iniConfig, section, "Mapping");

    unsigned int nbInputChannels = 0;
    std::vector<bool> outputConnection(nbChannels, false);

    for (std::vector<std::shared_ptr<Cell> >::const_iterator it
         = parents.begin(),
         itEnd = parents.end();
         it != itEnd;
         ++it) {
        const Matrix<bool> map = MappingGenerator::generate(
            sp, (*it), nbChannels, iniConfig, section, defaultMapping);

        nbInputChannels += map.rows();

        for (unsigned int channel = 0; channel < nbChannels; ++channel) {
            const std::vector<bool>& row = map.col(channel);
            outputConnection[channel]
                = outputConnection[channel]
                  || (std::find(row.begin(), row.end(), true) != row.end());
        }

        if (!(*it))
            cell->addInput(sp, 0, 0, sp.getSizeX(), sp.getSizeY(), map);
        else if ((*it)->getOutputsWidth() > 1 || (*it)->getOutputsHeight() > 1)
            cell->addInput((*it).get(), map);
        else {
            throw std::runtime_error("2D input expected for a PoolCell (\""
                                     + section + "\"), \"" + (*it)->getName()
                                     + "\" is not,"
                                       " in configuration file: "
                                     + iniConfig.getFileName());
        }
    }

    for (unsigned int channel = 0; channel < nbChannels; ++channel) {
        if (!outputConnection[channel]) {
            std::cout << Utils::cwarning << "Warning: output map #" << channel
                      << " of \"" << section << "\" has no input connection."
                      << Utils::cdef << std::endl;
        }
    }

    std::cout << "  # Inputs dims: " << cell->getInputsDims() << std::endl;
    std::cout << "  # Outputs dims: " << cell->getOutputsDims() << std::endl;

    Utils::createDirectories("map");
    cell->writeMap("map/" + section + "_map.dat");

    return cell;
}
