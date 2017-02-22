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

#ifndef N2D2_NORMALFILLERGENERATOR_H
#define N2D2_NORMALFILLERGENERATOR_H

#include "Filler/NormalFiller.hpp"
#include "FillerGenerator.hpp"

#ifdef WIN32
// For static library
#pragma comment(                                                               \
    linker,                                                                    \
    "/include:?mRegistrar@NormalFillerGenerator@N2D2@@0U?$Registrar@VFillerGenerator@N2D2@@@2@A")
#endif

namespace N2D2 {
class NormalFillerGenerator : public FillerGenerator {
public:
    static std::shared_ptr<NormalFiller<Float_T> >
    generate(IniParser& iniConfig,
             const std::string& section,
             const std::string& name);

private:
    static Registrar<FillerGenerator> mRegistrar;
};
}

#endif // N2D2_NORMALFILLERGENERATOR_H
