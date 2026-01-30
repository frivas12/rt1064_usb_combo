// preprocessor.cc

/**************************************************************************//**
 * \file preprocessor.cc
 * \author Sean Benish
 *****************************************************************************/
#include "lut-builder/preprocessor.hh"

#include <string.h>
#include <utility>
#include <vector>

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Macros
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/
struct context_t
{
    std::filesystem::path path;
};

/*****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/
static tinyxml2::XMLElement * xpath_search(tinyxml2::XMLElement * const p_elem, const std::string XPATH);

static void preprocess_includes(tinyxml2::XMLDocument& r_doc, tinyxml2::XMLElement * const p_elem, context_t context);
static context_t preprocess_context(tinyxml2::XMLElement * const p_elem, context_t context);
static void preprocess_nodes(tinyxml2::XMLDocument& r_doc, tinyxml2::XMLElement * const p_elem, context_t context);

/*****************************************************************************
 * Static Data
 *****************************************************************************/

/******************************************************************************
 * Interrupt Handlers
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
void preprocess_xml(tinyxml2::XMLDocument& r_doc, std::filesystem::path path_to_file)
{
    context_t context;
    context.path = path_to_file.parent_path();
    preprocess_nodes(r_doc, r_doc.FirstChildElement(), context);
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/
static tinyxml2::XMLElement * xpath_search(tinyxml2::XMLElement * const p_elem, const std::string XPATH)
{
    const std::size_t SLASH_FIND = XPATH.find_first_of('/');
    const std::string TARGET_NAME = (SLASH_FIND == std::string::npos) ? XPATH : (
        XPATH.substr(0, SLASH_FIND)
    );

    if (p_elem->Name() == TARGET_NAME)
    {
        if (SLASH_FIND == std::string::npos)
        {
            return p_elem;
        }

        tinyxml2::XMLElement * p_rt = nullptr;
        const std::string NEW_XPATH = XPATH.substr(SLASH_FIND + 1);
        auto itr = p_elem->FirstChildElement();
        
        while (p_rt == nullptr && itr != nullptr)
        {
            p_rt = xpath_search(itr, NEW_XPATH);
            itr = itr->NextSiblingElement();
        }

        return p_rt;
    }

    return nullptr;
}

static void preprocess_includes(tinyxml2::XMLDocument& r_doc, tinyxml2::XMLElement * const p_elem, context_t context)
{
    // Replacement 
    std::vector<std::pair<tinyxml2::XMLNode*, tinyxml2::XMLNode*>> swap;

    auto itr = p_elem->FirstChildElement();
    while (itr != nullptr)
    {
        if (strcmp(itr->Name(), "Include") == 0 || strcmp(itr->Name(), "Import") == 0) {
            std::filesystem::path path = context.path / itr->Attribute("href");
            std::filesystem::path path_cwd = path.parent_path();

            tinyxml2::XMLDocument doc;
            if (doc.LoadFile(path.string().c_str()) != tinyxml2::XMLError::XML_SUCCESS)
            {
                throw std::runtime_error(std::string("preprocessor failed to open ") + path.string());
            }
            context_t new_context = context;
            new_context.path = path_cwd;
            preprocess_nodes(doc, doc.FirstChildElement(), new_context);


            tinyxml2::XMLElement * p_found = doc.FirstChildElement();
            if (strcmp(itr->Name(), "Import") == 0)
            {
                std::string xpath(itr->Attribute("xpath"));
                xpath = (xpath.size() > 0 && xpath[0] == '/') ? xpath.substr(1) : xpath;
                p_found = xpath_search(p_found, xpath);
            }
            // p_found may be null here

            // Clone into parent document
            swap.push_back(std::pair(
                itr,
                p_found->DeepClone(&r_doc)  
            ));

        } else {
            preprocess_includes(r_doc, itr, context);
        }

        itr = itr->NextSiblingElement();
    }

    // Remove any element to be removed
    for (auto pair : swap)
    {
        p_elem->DeleteChild(pair.first);
    }

    // Add cloned elements
    for (auto pair : swap)
    {
        p_elem->InsertEndChild(pair.second);
    }
}

static context_t preprocess_context(tinyxml2::XMLElement * const p_elem, context_t context)
{
    // <Name, New Value>
    std::vector<std::pair<std::string, std::string>> changes;

    auto begin = p_elem->FirstAttribute();
    while (begin != nullptr)
    {
        const std::size_t LEN = strlen(begin->Value());
        if (LEN > 0 && begin->Value()[0] == '$')
        {
            if (LEN > 6 && memcmp(begin->Value() + 1, "PATH:", 6) == 0) {
                // Path operator
                changes.push_back(std::pair(
                    std::string(begin->Name()),
                    (context.path / std::string(begin->Value()).substr(6)).string()
                ));
            } else if (LEN > 1 && begin->Value()[1] == '$') {
                // $ Literal
                changes.push_back(std::pair(
                    std::string(begin->Name()),
                    std::string(begin->Value()).substr(1)
                ));
            }
        }

        begin = begin->Next();
    }

    for (auto item : changes)
    {
        p_elem->SetAttribute(item.first.c_str(), item.second.c_str());
    }

    auto itr = p_elem->FirstChildElement();
    while (itr != nullptr)
    {
        context = preprocess_context(itr, context);

        itr = itr->NextSiblingElement();
    }

    return context;
}

static void preprocess_nodes(tinyxml2::XMLDocument& r_doc, tinyxml2::XMLElement * const p_elem, context_t context)
{
    context = preprocess_context(p_elem, context);
    preprocess_includes(r_doc, p_elem, context);
}
// EOF
