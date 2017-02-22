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

#ifndef N2D2_CONVCELLGENERATOR_H
#define N2D2_CONVCELLGENERATOR_H

#include "Cell/ConvCell.hpp"
#include "Generator/CellGenerator.hpp"
#include "Generator/FillerGenerator.hpp"
#include "Generator/KernelGenerator.hpp"
#include "Generator/MappingGenerator.hpp"
#include "utils/IniParser.hpp"

#ifdef WIN32
// For static library
#pragma comment(                                                               \
    linker,                                                                    \
    "/include:?mRegistrar@ConvCellGenerator@N2D2@@0U?$Registrar@VCellGenerator@N2D2@@@2@A")
#endif

namespace N2D2 {
class ConvCellGenerator : public CellGenerator {
public:
    static std::shared_ptr<ConvCell> generate(Network& network,
                                              StimuliProvider& sp,
                                              const std::vector
                                              <std::shared_ptr<Cell> >& parents,
                                              IniParser& iniConfig,
                                              const std::string& section);

private:
    static Registrar<CellGenerator> mRegistrar;
};
}

#endif // N2D2_CONVCELLGENERATOR_H
