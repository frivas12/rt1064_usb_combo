// preprocessor.hh

/**************************************************************************//**
 * \file preprocessor.hh
 * \author Sean Benish
 * \brief XML preprocessing.
 *****************************************************************************/
#pragma once

#include <filesystem>

#include "tinyxml2.h"

/*****************************************************************************
 * Defines
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/**
 * Preprocesses an XML by replacing include and import tags with their proper XML block.
 * \param[in]       r_doc Document to modify if preprocessors are needed.
 * \param[in]       path_to_file The path to the file referenced by r_doc.
 * \throw std::runtime_exception Throws a runtime exception when the XMLs to includ were not found.
 */
void preprocess_xml(tinyxml2::XMLDocument& r_doc, std::filesystem::path path_to_file);

//EOF
