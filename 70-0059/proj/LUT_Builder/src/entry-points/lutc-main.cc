// main.cc

/**************************************************************************//**
 * \file main.cc
 * \author Sean Benish
 *****************************************************************************/
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <fstream>
#include <cerrno>
#include <cstring>

#include "lut-builder/cmd-line-parser.hh"
#include "lut-builder/intermediate-formats.hh"
#include "lut-builder/cmd-line-parser.hh"
#include "lut-builder/lut-tree.hh"
#include "lut-builder/lut-tree.hh"
#include "lut-builder/optimization.hh"
#include "lut-builder/compiler.hh"
#include "lut-builder/duplicate-searcher.hh"

/*****************************************************************************
 * Constants
 *****************************************************************************/
constexpr uint8_t LUT_VERSION = 1;

/*****************************************************************************
 * Macros
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/

/*****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/
static std::vector<uint8_t> write_lut(const structured_arguments& args, lut_tree& r_tree);
static std::vector<uint8_t> write_basic_lut(const structured_arguments& args, lut_tree_node * const p_root);
static std::vector<uint8_t> write_indirect_lut(const structured_arguments& args, lut_tree_node * const p_root, const std::size_t indirection);
static void construct_optimization_objects(std::vector<optimization::lut_header>& r_headers,
    std::vector<optimization::lut_entries>& r_entries, 
    const structured_arguments& r_args,
    lut_tree_node * const p_node,
    const std::size_t DEPTH = 0);

static void read_in_complied_file(compiled_entry_slim& r_slim, const std::filesystem::path& path);

/*****************************************************************************
 * Static Data
 *****************************************************************************/

/******************************************************************************
 * Interrupt Handlers
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
int main (int argc, char** argv)
{
    structured_arguments args;

    try
    {
        args = parse_args(argc, argv);
    }
    catch (std::exception& r_e)
    {
        std::cerr << r_e.what() << std::endl;

        return 1;
    }

    if (args.need_help)
    {
        print_usage(argv[0]);
        return 0;
    }

    lut_tree tree(args.key_sizes);
    std::unordered_map<std::filesystem::path, compiled_entry_slim> input_parsed_mapper;

    // Compile the LUT files into intermediates.
    for (auto itr = args.targets.begin(); itr != args.targets.end(); ++itr)
    {
        for (auto file = itr->input_file_paths.begin(); file != itr->input_file_paths.end(); ++file)
        {
            compiled_entry_slim compiled;
            if (file->extension() == LUT_OBJECT_EXTENSION) {
                try
                {
                    read_in_complied_file(compiled, *file);
                }
                catch (std::exception& r_e)
                {
                    std::cerr << "Exception while reading object file " << file->string() << ":  " << r_e.what() << ".\n";
                    return 1;
                }
            } else {
                try
                {
                    compile(compiled, *file);
                }
                catch (std::exception& r_e)
                {
                    std::cerr << "Exception while compiling file " << file->string() << ":  " << r_e.what() << ".\n";
                    return 1;
                }
            }

            if (compiled.keys.size() != args.key_sizes.size())
            {
                std::cerr << "Key mismatch on object file " << file->string() << '\n';
                return 1;
            }
            for (std::size_t i = 0; i < args.key_sizes.size(); ++i)
            {
                if (compiled.keys[i].size() != args.key_sizes[i])
                {
                    std::cerr << "Key mismatch on object file " << file->string() << '\n';
                    return 1;
                }
            }

            input_parsed_mapper[*file] = compiled;
        }
    }

    // If compiling to LUT, join the files.
    if (args.targets[0].target_type == target_types_e::LUT)
    {
        for (auto itr = input_parsed_mapper.begin(); itr != input_parsed_mapper.end(); ++itr)
        {
            tree.add_struct(itr->second); 
        }
    }

    if (args.targets[0].target_type == target_types_e::LUT)
    {
        auto duplicates = find_duplicate_ids(tree);

        if (!duplicates.empty())
        {
            std::cerr << "Duplicate keys found: \n";
            for (std::vector<uint8_t> i : duplicates) {
                std::cerr << "[ ";
                for (uint8_t item : i) {
                    std::cerr << std::right
                              << std::setfill('0')
                              << std::hex
                              << std::setw(2)
                              << static_cast<unsigned int>(item)
                              << "h ";
                }

                std::cerr << "]\n";
            }

        return 1;
        }
    }

    // Do size optimization if enabled
    // Check that size does not violate the minimum size restriction.
    if (args.targets[0].target_type == target_types_e::LUT)
    {
        optimization::global_lut_header gheader(args.key_sizes.size());
        std::vector<optimization::lut_header> headers;
        std::vector<optimization::lut_entries> entries;

        construct_optimization_objects(headers, entries, args, &tree, 0);

        std::vector<optimization::opt_base*> pointers;
        pointers.reserve(headers.size() + entries.size() + 1);
        pointers.push_back(&gheader);
        for (auto itr = headers.begin(); itr != headers.end(); ++itr)
        {
            pointers.push_back(&(*itr));
        }
        for (auto itr = entries.begin(); itr != entries.end(); ++itr)
        {
            pointers.push_back(&(*itr));
        }
        
        auto [min, max] = optimization::get_bounds(pointers.begin(), pointers.end());
        if (args.lut_page_size < min)
        {
            std::cerr << "LUT page size set to " << args.lut_page_size << ". Mimimum page sizes is " << min << '\n';
            return 1;
        }

        if (args.optimize_page_size && args.targets[0].target_type == target_types_e::LUT)
        {
            args.lut_page_size = optimization::optimize(pointers.begin(), pointers.end());
            args.optimize_page_size = false;
        }
    }

    // Write output files.
    for (auto itr = args.targets.begin(); itr != args.targets.end(); ++itr)
    {
        
        std::fstream out_file(itr->output_file_path, std::ios::binary | std::ios::out | std::ios::trunc);
        if (!out_file.is_open())
        {
            std::cerr << "Failed to open output file \"" << itr->output_file_path.string() << "\".  Error no:  "<< std::strerror(errno) <<"\n";
            return 1;
        }

        if (itr->target_type == target_types_e::LUT) {
            LUT_Header header;
            header.lut_version = LUT_VERSION;
            header.lut_page_size = args.lut_page_size;
            header.indirection_count = args.key_sizes.size() - 1;
            header.key_sizes.reserve(args.key_sizes.size());
            for (std::size_t i = 0; i < args.key_sizes.size(); ++i)
            {
                header.key_sizes[i] = static_cast<uint8_t>(args.key_sizes[i]);
            }
            header.keys_in_header = 0;

            std::vector<uint8_t> lut = write_lut(args, tree);

            out_file.write(reinterpret_cast<char*>(lut.data()), static_cast<std::streamsize>(lut.size()));
            
        } else if (itr->target_type == target_types_e::LUT_OBJECT) {
            const compiled_entry_slim& r_slim = input_parsed_mapper[itr->input_file_paths[0]];

            // Calculate entry size.
            uint32_t entry_size = sizeof(r_slim.crc) + r_slim.structure.size() + (r_slim.keys.size() + 1) / 2;
            for (std::size_t i = 0; i < r_slim.keys.size(); ++i)
            {
                entry_size += r_slim.keys[i].size();
            }

            compiled_entry entry;
            entry.lut_version = LUT_VERSION;
            entry.key_count = r_slim.keys.size();
            entry.struct_size = r_slim.structure.size();
            entry.crc = r_slim.crc;

            // Output header
            out_file.write(reinterpret_cast<char*>(&entry.lut_version), sizeof(entry.lut_version));
            out_file.write(reinterpret_cast<char*>(&entry.key_count), sizeof(entry.key_count));
            out_file.write(reinterpret_cast<char*>(&entry.struct_size), sizeof(entry.struct_size));

            // Output keys
            for (std::size_t i = 0; i < args.key_sizes.size(); ++i)
            {
                const uint8_t SIZE = static_cast<uint8_t>(args.key_sizes[i]);
                out_file.write(reinterpret_cast<const char*>(&SIZE), sizeof(SIZE));
            }

            // Output signature
            for (std::size_t i = 0; i < r_slim.keys.size(); ++i)
            {
                for (std::size_t j = 0; j < r_slim.keys[i].size(); ++j)
                {
                    out_file.write(reinterpret_cast<const char*>(&r_slim.keys[i][j]), sizeof(r_slim.keys[i][j]));
                }
            }

            // Output structure
            for (std::size_t i = 0; i < r_slim.structure.size(); ++i)
            {
                out_file.write(reinterpret_cast<const char*>(&r_slim.structure[i]), sizeof(r_slim.structure[i]));
            }

            out_file.write(reinterpret_cast<char*>(&entry.crc), sizeof(entry.crc));
        }


        out_file.close();
    }

    return 0;
}


/*****************************************************************************
 * Private Functions
 *****************************************************************************/
static std::vector<uint8_t> write_lut(const structured_arguments& args, lut_tree& r_tree)
{
    std::vector<uint8_t> rt(args.lut_page_size);

    rt[0] = static_cast<uint8_t>(LUT_VERSION);
    rt[1] = static_cast<uint8_t>(r_tree.HEIGHT - 2);
    rt[2] = static_cast<uint8_t>(args.lut_page_size);
    rt[3] = static_cast<uint8_t>(args.lut_page_size >> 8);
    for (std::size_t i = 0; i < args.key_sizes.size(); ++i)
    {
        rt[4 + i] = static_cast<uint8_t>(args.key_sizes[i]);
    }

    std::vector<uint8_t> to_join = write_indirect_lut(args, &r_tree, r_tree.HEIGHT - 2);

    rt.resize(rt.size() + to_join.size());
    memcpy(rt.data() + args.lut_page_size, to_join.data(), to_join.size());

    return rt;
}

static std::vector<uint8_t> write_basic_lut(const structured_arguments& args, lut_tree_node * const p_root)
{
    const std::size_t STRUCTURE_KEY_SIZE = args.key_sizes[args.key_sizes.size() - 2];
    const std::size_t NUMBER_OF_CHILDREN = p_root->children_size();
    const std::size_t STATIC_OFFSET = STRUCTURE_KEY_SIZE; // Offset from the first header entry
    const std::size_t HEADER_ENTRY_SIZE = STRUCTURE_KEY_SIZE + sizeof(uint16_t);
    const std::size_t ENTRIES_ON_FIRST_HEADER_PAGE = (args.lut_page_size - STATIC_OFFSET) / HEADER_ENTRY_SIZE;
    const std::size_t ENTIRES_ON_OTHER_HEADER_PAGE = args.lut_page_size / HEADER_ENTRY_SIZE;
    const std::size_t NUMBER_OF_HEADER_PAGES = ((NUMBER_OF_CHILDREN + 1) <= ENTRIES_ON_FIRST_HEADER_PAGE) ? 1 : (
        (NUMBER_OF_CHILDREN - ENTRIES_ON_FIRST_HEADER_PAGE + ENTIRES_ON_OTHER_HEADER_PAGE) / ENTIRES_ON_OTHER_HEADER_PAGE
    );
    std::vector<uint8_t> header_vector(args.lut_page_size * NUMBER_OF_HEADER_PAGES, 0xFF);

    // Copy the number of children into the header.
    header_vector[0] = static_cast<uint8_t>(NUMBER_OF_CHILDREN);
    header_vector[1] = static_cast<uint8_t>(NUMBER_OF_CHILDREN >> 8);

    // Page offset will have to be calculated after.

    std::vector<std::vector<uint8_t>> children_pages(NUMBER_OF_CHILDREN, std::vector<uint8_t>());
    std::size_t index = 0;
    std::size_t pages_allocated = 0;

    // Create the pages for each child.
    auto begin = p_root->begin_children();
    auto end = p_root->end_children();
    while (begin != end)
    {
        lut_tree_node * const p_struct_node = *begin;


        entry_node * p_first = reinterpret_cast<entry_node*>(*p_struct_node->begin_children());
        const std::size_t CHILDREN = (*begin)->children_size();
        const std::size_t DISCRIMINATOR_KEY_SIZE = args.key_sizes[args.key_sizes.size() - 1];
        const std::size_t STRUCTURE_SIZE = p_first->get_entry().structure.size();
        constexpr std::size_t CRC_SIZE = sizeof(p_first->get_entry().crc);
        const std::size_t ENTRY_SIZE = DISCRIMINATOR_KEY_SIZE + STRUCTURE_SIZE + CRC_SIZE;
        const std::size_t ENTIRES_PER_PAGE = (args.lut_page_size) / ENTRY_SIZE;
        const std::size_t TOTAL_PAGES = (CHILDREN + ENTIRES_PER_PAGE - 1) / ENTIRES_PER_PAGE;

        // Reserve space for child pages.
        children_pages[index].resize(TOTAL_PAGES * args.lut_page_size);
        // Copy ID into LUT header.
        {
            auto id = p_struct_node->get_id();
            const std::size_t PAGE = (index < ENTRIES_ON_FIRST_HEADER_PAGE) ? 0 : (
                (index - ENTRIES_ON_FIRST_HEADER_PAGE) / ENTIRES_ON_OTHER_HEADER_PAGE + 1
            );
            const std::size_t OFFSET = PAGE*args.lut_page_size + HEADER_ENTRY_SIZE*index + ((PAGE == 0) ? STATIC_OFFSET : 0);
            for (std::size_t i = 0; i < id.size(); ++i)
            {
                header_vector[OFFSET + i] = id[i];
            }
            const uint8_t pages = static_cast<uint8_t>(NUMBER_OF_HEADER_PAGES + pages_allocated);
            header_vector[OFFSET + id.size()] = pages;
            header_vector[OFFSET + id.size() + 1] = pages >> 8;

            pages_allocated += TOTAL_PAGES;
        }
        auto ent_begin = p_struct_node->begin_children();
        auto ent_end = p_struct_node->end_children();
        for (std::size_t page = 0; page < TOTAL_PAGES; ++page)
        {
            // Iterate over each page that will be used.
            for (std::size_t entry = 0; entry < ENTIRES_PER_PAGE && ent_begin != ent_end; ++entry, ++ent_begin)
            {
                // Copy the entry's data onto the page.
                const compiled_entry_slim& r_entry = reinterpret_cast<entry_node*>(*ent_begin)->get_entry();
                uint8_t * const p_data = children_pages[index].data() + ENTRY_SIZE*entry + page*args.lut_page_size;
                memcpy(p_data, r_entry.keys[r_entry.keys.size() - 1].data(), DISCRIMINATOR_KEY_SIZE);
                memcpy(p_data + DISCRIMINATOR_KEY_SIZE, r_entry.structure.data(), STRUCTURE_SIZE);
                memcpy(p_data + DISCRIMINATOR_KEY_SIZE + STRUCTURE_SIZE,
                    &r_entry.crc, CRC_SIZE);
            }
        }

        ++begin;
        ++index;
    }

    // Write the end deliminter into header
    {
        const std::size_t PAGE = (index < ENTRIES_ON_FIRST_HEADER_PAGE) ? 0 : (
            (index - ENTRIES_ON_FIRST_HEADER_PAGE) / ENTIRES_ON_OTHER_HEADER_PAGE + 1
        );
        const std::size_t OFFSET = PAGE*args.lut_page_size + HEADER_ENTRY_SIZE*index + ((PAGE == 0) ? STATIC_OFFSET : 0);
        for (std::size_t i = 0; i < STRUCTURE_KEY_SIZE; ++i)
        {
            header_vector[OFFSET + i] = 0xFF;
        }
        const uint8_t pages = static_cast<uint8_t>(NUMBER_OF_HEADER_PAGES + pages_allocated);
        header_vector[OFFSET + STRUCTURE_KEY_SIZE] = pages;
        header_vector[OFFSET + STRUCTURE_KEY_SIZE + 1] = pages >> 8;
    }

    // Calculate the needed capacity.
    std::size_t additional_cap = 0;
    for (std::size_t i = 0; i < children_pages.size(); ++i)
    {
        std::vector<uint8_t>& data_vector = children_pages[i];
        additional_cap += data_vector.size();
    }

    // Reserve capacity and copy over data.
    std::size_t offset = header_vector.size();
    header_vector.resize(header_vector.size() + additional_cap);
    for (std::size_t i = 0; i < children_pages.size(); ++i)
    {
        std::vector<uint8_t>& data_vector = children_pages[i];
        memcpy(header_vector.data() + offset, data_vector.data(), data_vector.size());
        offset += data_vector.size();
    }
    return header_vector;
}
static std::vector<uint8_t> write_indirect_lut(const structured_arguments& args, lut_tree_node * const p_root, const std::size_t indirection)
{
    if (indirection == 0)
    {
        return write_basic_lut(args, p_root);
    }

    const std::size_t INDIRECTION_KEY_SIZE = args.key_sizes[args.key_sizes.size() - 2 - indirection];
    const std::size_t NUMBER_OF_CHILDREN = p_root->children_size();
    const std::size_t STATIC_OFFSET = INDIRECTION_KEY_SIZE; // Offset from the first header entry
    const std::size_t HEADER_ENTRY_SIZE = INDIRECTION_KEY_SIZE + sizeof(uint16_t);
    const std::size_t ENTRIES_ON_FIRST_HEADER_PAGE = (args.lut_page_size - STATIC_OFFSET) / HEADER_ENTRY_SIZE;
    const std::size_t ENTIRES_ON_OTHER_HEADER_PAGE = args.lut_page_size / HEADER_ENTRY_SIZE;
    const std::size_t NUMBER_OF_HEADER_PAGES = ((NUMBER_OF_CHILDREN + 1) <= ENTRIES_ON_FIRST_HEADER_PAGE) ? 1 : (
        (NUMBER_OF_CHILDREN - ENTRIES_ON_FIRST_HEADER_PAGE + ENTIRES_ON_OTHER_HEADER_PAGE) / ENTIRES_ON_OTHER_HEADER_PAGE
    );
    std::vector<uint8_t> header_vector(args.lut_page_size * NUMBER_OF_HEADER_PAGES, 0xFF);
    header_vector[0] = static_cast<uint8_t>(NUMBER_OF_CHILDREN);
    header_vector[1] = static_cast<uint8_t>(NUMBER_OF_CHILDREN >> 8);

    {
        std::size_t acc = p_root->children_size();
        for (std::size_t i = 0; i < INDIRECTION_KEY_SIZE; ++i)
        {
            header_vector[i] = static_cast<uint8_t>(acc);
            acc >>= 8;
        }
    }


    std::vector<uint8_t> page_vector;
    std::size_t index = 0;
    std::size_t pages_allocated = 0;


    // Allocate pages for each sub-LUT
    auto begin = p_root->begin_children();
    auto end = p_root->end_children();
    while (begin != end)
    {
        lut_tree_node * const p_lut_node = *begin;

        const std::vector<uint8_t> data = write_indirect_lut(args, p_lut_node, indirection - 1);
        const std::size_t TOTAL_PAGES = data.size() / args.lut_page_size;

        // Copy ID into LUT header.
        {
            auto id = p_lut_node->get_id();
            const std::size_t PAGE = (index < ENTRIES_ON_FIRST_HEADER_PAGE) ? 0 : (
                (index - ENTRIES_ON_FIRST_HEADER_PAGE) / ENTIRES_ON_OTHER_HEADER_PAGE + 1
            );
            const std::size_t OFFSET = PAGE*args.lut_page_size + HEADER_ENTRY_SIZE*index + ((PAGE == 0) ? STATIC_OFFSET : 0);
            for (std::size_t i = 0; i < id.size(); ++i)
            {
                header_vector[OFFSET + i] = id[i];
            }
            const uint8_t pages = static_cast<uint8_t>(NUMBER_OF_HEADER_PAGES + pages_allocated);
            header_vector[OFFSET + id.size()] = pages;
            header_vector[OFFSET + id.size() + 1] = pages >> 8;

            pages_allocated += TOTAL_PAGES;
        }

        // Copy data into page vector
        const std::size_t OFFSET = page_vector.size();
        page_vector.resize(page_vector.size() + data.size());
        memcpy(page_vector.data() + OFFSET, data.data(), data.size());

        ++begin;
        ++index;
    }

    // Write end index into LUT header
    {
        const std::size_t PAGE = (index < ENTRIES_ON_FIRST_HEADER_PAGE) ? 0 : (
            (index - ENTRIES_ON_FIRST_HEADER_PAGE) / ENTIRES_ON_OTHER_HEADER_PAGE + 1
        );
        const std::size_t OFFSET = PAGE*args.lut_page_size + HEADER_ENTRY_SIZE*index + ((PAGE == 0) ? STATIC_OFFSET : 0);
        for (std::size_t i = 0; i < INDIRECTION_KEY_SIZE; ++i)
        {
            header_vector[OFFSET + i] = 0xFF;
        }
        const uint8_t pages = static_cast<uint8_t>(NUMBER_OF_HEADER_PAGES + pages_allocated);
        header_vector[OFFSET + INDIRECTION_KEY_SIZE] = pages;
        header_vector[OFFSET + INDIRECTION_KEY_SIZE + 1] = pages >> 8;
    }


    const std::size_t OFFSET = header_vector.size();
    header_vector.resize(header_vector.size() + page_vector.size());
    memcpy(header_vector.data() + OFFSET, page_vector.data(), page_vector.size());

    return header_vector;
}

static void construct_optimization_objects(std::vector<optimization::lut_header>& r_headers,
    std::vector<optimization::lut_entries>& r_entries, const structured_arguments& r_args,
    lut_tree_node * const p_node, const std::size_t depth)
{
    // If the node has children and its child has no children.
    const bool IS_ENTRY_NODE = p_node->children_size() > 0 && (*p_node->begin_children())->children_size() == 0;

    if (IS_ENTRY_NODE) {
        entry_node * const p_entry = reinterpret_cast<entry_node*>(
            *p_node->begin_children()
        );
        const compiled_entry_slim& r_entry = p_entry->get_entry();

        r_entries.emplace_back(
            sizeof(r_entry.crc) + r_entry.keys[r_entry.keys.size() - 1].size()
                + r_entry.structure.size(),
            p_node->children_size()
        );
    } else {
        r_headers.emplace_back(
            r_args.key_sizes[depth],
            p_node->children_size()
        );


        for (auto itr = p_node->begin_children(); itr != p_node->end_children(); ++itr)
        {
            construct_optimization_objects(r_headers, r_entries, r_args, *itr, depth + 1);
        }
    }
}

static void read_in_complied_file(compiled_entry_slim& r_slim, const std::filesystem::path& r_path)
{
    std::fstream file(r_path, std::ios::binary | std::ios::in);
    if (!file.is_open())
    {
        throw std::logic_error(std::string("failed to open file"));
    }

    compiled_entry entry;

    std::vector<uint8_t> key_sizes;
    std::vector<uint8_t> keys;
    std::vector<uint8_t> structure;

    // Read in compiled entry from file.
    file.read(reinterpret_cast<char*>(&entry.lut_version), sizeof(entry.lut_version));

    if (entry.lut_version != LUT_VERSION)
    {
        throw std::logic_error(std::string("object file has unsupported version"));
    }

    file.read(reinterpret_cast<char*>(&entry.key_count), sizeof(entry.key_count));
    file.read(reinterpret_cast<char*>(&entry.struct_size), sizeof(entry.struct_size));

    key_sizes.resize(entry.key_count);
    structure.resize(entry.struct_size);

    file.read(reinterpret_cast<char*>(key_sizes.data()), entry.key_count);

    {
        std::size_t acc = 0;
        for (auto itr = key_sizes.begin(); itr != key_sizes.end(); ++itr)
        {
            acc += *itr;
        }
        keys.resize(acc);
        file.read(reinterpret_cast<char*>(keys.data()), static_cast<std::streamsize>(acc));
    }

    file.read(reinterpret_cast<char*>(structure.data()), entry.struct_size);
    file.read(reinterpret_cast<char*>(&entry.crc), sizeof(entry.crc));

    // Translate into slim.
    r_slim.keys.resize(entry.key_count);
    std::size_t offset = 0;
    for (std::size_t i = 0; i < key_sizes.size(); ++i)
    {
        const uint8_t SIZE = key_sizes[i];
        r_slim.keys[i].resize(SIZE);
        for (std::size_t k = 0; k < SIZE; ++k)
        {
            r_slim.keys[i][k] = keys[offset + k];
        }
        offset += SIZE;
    }
    r_slim.structure = std::move(structure);
    r_slim.crc = entry.crc;
}

// EOF
