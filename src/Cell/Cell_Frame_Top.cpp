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

#include "Cell/Cell_Frame_Top.hpp"

#ifdef PYBIND
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>
#include <pybind11/numpy.h>

namespace py = pybind11;

namespace N2D2 {
void init_Cell_Frame_Top(py::module &m) {
    py::class_<Cell_Frame_Top, std::shared_ptr<Cell_Frame_Top>> cell(m, "Cell_Frame_Top");

    py::enum_<Cell_Frame_Top::Signals>(cell, "Signals")
    .value("In", Cell_Frame_Top::Signals::In)
    .value("Out", Cell_Frame_Top::Signals::Out)
    .value("InOut", Cell_Frame_Top::Signals::InOut)
    .export_values();

    cell.def("save", &Cell_Frame_Top::save, py::arg("dirName"))
    .def("load", &Cell_Frame_Top::load, py::arg("dirName"))
    .def("addInput", &Cell_Frame_Top::addInput, py::arg("inputs"), py::arg("diffOutputs"))
    .def("replaceInput", &Cell_Frame_Top::replaceInput, py::arg("oldInputs"), py::arg("newInputs"), py::arg("newDiffOutputs"))
    .def("propagate", &Cell_Frame_Top::propagate, py::arg("inference") = false)
    .def("backPropagate", &Cell_Frame_Top::backPropagate)
    .def("update", &Cell_Frame_Top::update)
    .def("checkGradient", &Cell_Frame_Top::checkGradient, py::arg("epsilon"), py::arg("maxError"))
    .def("discretizeSignals", &Cell_Frame_Top::discretizeSignals, py::arg("nbLevels"), py::arg("signals") = Cell_Frame_Top::In)
    .def("setOutputTarget", &Cell_Frame_Top::setOutputTarget, py::arg("targets"), py::arg("targetVal") = 1.0, py::arg("defaultVal") = 0.0)
    .def("setOutputTargets", (double (Cell_Frame_Top::*)(const Tensor<int>&, double, double)) &Cell_Frame_Top::setOutputTargets, py::arg("targets"), py::arg("targetVal") = 1.0, py::arg("defaultVal") = 0.0)
    .def("setOutputTargets", (double (Cell_Frame_Top::*)(const BaseTensor&)) &Cell_Frame_Top::setOutputTargets, py::arg("targets"))
    .def("setOutputErrors", &Cell_Frame_Top::setOutputErrors, py::arg("errors"))
    .def("getOutputs", (BaseTensor& (Cell_Frame_Top::*)()) &Cell_Frame_Top::getOutputs)
    .def("getDiffInputs", (BaseTensor& (Cell_Frame_Top::*)()) &Cell_Frame_Top::getDiffInputs)
    .def("getMaxOutput", &Cell_Frame_Top::getMaxOutput, py::arg("batchPos") = 0)
    .def("getActivation", &Cell_Frame_Top::getActivation)
    .def("setActivation", &Cell_Frame_Top::setActivation, py::arg("activation"))
    .def("isCuda", &Cell_Frame_Top::isCuda);
}
}
#endif
