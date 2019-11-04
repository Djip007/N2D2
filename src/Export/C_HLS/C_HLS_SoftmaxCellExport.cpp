/*
    (C) Copyright 2016 CEA LIST. All Rights Reserved.
    Contributor(s): Jean-Philippe Perois (Thales Service)

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

#include "Export/C_HLS/C_HLS_SoftmaxCellExport.hpp"
#include "Export/DeepNetExport.hpp"

N2D2::Registrar<N2D2::SoftmaxCellExport>
N2D2::C_HLS_SoftmaxCellExport::mRegistrar("C_HLS", N2D2::C_HLS_SoftmaxCellExport::generate);

N2D2::Registrar<N2D2::C_HLS_CellExport>
N2D2::C_HLS_SoftmaxCellExport::mRegistrarType(SoftmaxCell::Type, N2D2::C_HLS_SoftmaxCellExport::getInstance);

N2D2::C_HLS_SoftmaxCellExport::~C_HLS_SoftmaxCellExport() {}

void N2D2::C_HLS_SoftmaxCellExport::generate(const SoftmaxCell& cell, const std::string& dirName)
{
	Utils::createDirectories(dirName + "/include");
	const std::string fileName = dirName + "/include/" + Utils::CIdentifier(cell.getName()) + ".h";
	std::ofstream header(fileName.c_str());

	if (!header.good()) throw std::runtime_error("Could not create C header file: " + fileName);

	C_HLS_CellExport::generateHeaderBegin(cell, header);

	const std::string identifier = Utils::CIdentifier(cell.getName());
	bool isUnsigned = N2D2::DeepNetExport::isCellInputsUnsigned(cell);

	header << "void " << identifier << "_" << ((isUnsigned) ? "u" : "") << "imp( \n"
			<< "    DATA_T inputs["<<cell.getNbChannels()<<"]["<<cell.getChannelsHeight()<<"]["<<cell.getChannelsWidth()<< "],\n"
			<< "    DATA_T outputs["<<cell.getNbOutputs()<<"]["<<cell.getOutputsHeight()<<"]["<<cell.getOutputsWidth()<<"])\n{\n"
			;
	/*
    void
    softmaxcell_propagate(unsigned int nbOutputs,
                          unsigned int outputsHeight,
                          unsigned int outputsWidth,
                          DATA_T inputs[nbOutputs][outputsHeight][outputsWidth],
                          DATA_T outputs[nbOutputs][outputsHeight][outputsWidth])
    {
	 */
	if (CellExport::mPrecision > 0) {
		//header << "#if NB_BITS > 0"<< std::endl;
		for (size_t oy = 0; oy < cell.getOutputsHeight(); ++oy) {
			for (size_t ox = 0; ox < cell.getOutputsWidth(); ++ox) {
				for (size_t pos = 0; pos < cell.getNbOutputs(); ++pos) {
					// c'est ce qui est fait en C... mais en quoi c'est juste???
					header << "    outputs["<<pos<<"]["<<oy<<"]["<<ox<<"] = inputs["<<pos<<"]["<<oy<<"]["<<ox<<"];" << std::endl;
				}
			}
		}
	} else {
		std::string EXP;
		if (CellExport::mPrecision==Float16) EXP ="exp?";
		if (CellExport::mPrecision==Float32) EXP ="expf";
		if (CellExport::mPrecision==Float64) EXP ="exp";

		//header << "#else"<< std::endl;
		header << "    DATA_T maxVal;"<< std::endl;
		header << "    DATA_T sum;"   << std::endl;
		header << "    DATA_T tmp;"   << std::endl;
		for (size_t oy = 0; oy < cell.getOutputsHeight(); ++oy) {
			for (size_t ox = 0; ox < cell.getOutputsWidth(); ++ox) {
				header << std::endl;
				header << "    maxVal = inputs[0]["<<oy<<"]["<<ox<<"];" << std::endl;
				for (size_t pos = 1; pos < cell.getNbOutputs(); ++pos) {
					header << "    if(maxVal < inputs["<<pos<<"]["<<oy<<"]["<<ox<<"]) maxVal = inputs["<<pos<<"]["<<oy<<"]["<<ox<<"];" << std::endl;
				}
				header << std::endl;
				header << "    sum = 0;"   << std::endl;
				for (size_t pos = 0; pos < cell.getNbOutputs(); ++pos) {
					header << "    tmp = "<<EXP<<"(inputs["<<pos<<"]["<<oy<<"]["<<ox<<"] - maxVal);" << std::endl;
					header << "    outputs["<<pos<<"]["<<oy<<"]["<<ox<<"] = tmp" << std::endl;
					header << "    sum += tmp " << std::endl;
				}
				header << std::endl;
				for (size_t pos = 0; pos < cell.getNbOutputs(); ++pos) {
					header << "    outputs["<<pos<<"]["<<oy<<"]["<<ox<<"] /= sum" << std::endl;
				}
			}
		}
		//header << "#endif"<< std::endl;
	}

	header << "}\n\n";
	C_HLS_CellExport::generateHeaderEnd(cell, header);
}

std::unique_ptr<N2D2::C_HLS_SoftmaxCellExport>
N2D2::C_HLS_SoftmaxCellExport::getInstance(Cell& /*cell*/) {
	return std::unique_ptr<C_HLS_SoftmaxCellExport>(new C_HLS_SoftmaxCellExport);
}

N2D2::C_HLS_CellExport::TclDirectives
N2D2::C_HLS_SoftmaxCellExport::getTclDirectives(
		Cell& cell,
		const std::vector<std::shared_ptr<Cell> >& /*parentCells*/,
		bool isUnsigned)
{
	std::stringstream funcName;
	funcName << Utils::CIdentifier(cell.getName()) << "_" << ((isUnsigned) ? "u" : "") << "propagate";

	if (cell.isUnitMap()) funcName << "_unitmap";

	const SoftmaxCell& softmaxCell = dynamic_cast<const SoftmaxCell&>(cell);

	return TclDirectives(funcName.str(),
			(cell.isUnitMap()) ? "Softmax_UnitMap" : "Softmax",
					cell.getNbChannels(),
					cell.getChannelsWidth(),
					cell.getChannelsHeight(),
					1, // c'est quoi???
							1);
}

void N2D2::C_HLS_SoftmaxCellExport::generateCellPrototype(
		Cell& cell,
		const std::vector<std::shared_ptr<Cell> >& /*parentCells*/,
		const std::string& outputSizeName,
		std::ofstream& prog,
		bool isUnsigned)
{
	const std::string identifier = Utils::CIdentifier(cell.getName());
	//const std::string prefix = Utils::upperCase(identifier);
	// Utils::CIdentifier(cell->getName())

	prog << "#define " << identifier << "_" << ((isUnsigned) ? "u" : "") << "propagate(NB, HEIGHT, WIDTH, IN, OUT) "
			<< identifier << "_" << ((isUnsigned) ? "u" : "") << "imp(IN, OUT)";

	//prog << "SOFTMAXCELL_" << ((isUnsigned) ? "U" : "") << "PROPAGATE("
	//    << identifier << ", "
	//    << prefix << "_NB_CHANNELS, "
	//    << prefix << "_CHANNELS_HEIGHT, "
	//    << prefix << "_CHANNELS_WIDTH, "
	//    << outputSizeName << ", "
	//    << prefix << "_OUTPUTS_HEIGHT, "
	//    << prefix << "_OUTPUTS_WIDTH)\n";
}
