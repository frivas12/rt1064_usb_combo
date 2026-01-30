#include "lut-builder/optimization.hh"

#include <vector>
#include <string>
#include <iostream>

int main (int argc, char** argv)
{
    const std::size_t EFS_PAGE_SIZES = std::stoull(argv[1]);
    
    optimization::global_lut_header gheader(2);

    std::vector<optimization::lut_entries> entries;
    for (int i = 3; i < argc; i += 2)
    {
        const std::size_t size = std::stoull(argv[i]);
        const std::size_t num = std::stoull(argv[i + 1]);
        entries.emplace_back(size, num);
    }

    optimization::lut_header header(std::stoull(argv[2]), entries.size());

    std::vector<optimization::opt_base*> pointers;
    pointers.reserve(2 + entries.size());
    pointers.push_back(&gheader);
    pointers.push_back(&header);
    for (auto itr = entries.begin(); itr != entries.end(); ++itr)
    {
        pointers.push_back(&(*itr));
    }

    const uint16_t OPTIMIZED = optimization::optimize(pointers.begin(), pointers.end());

    std::size_t acc = gheader.get_page_count(OPTIMIZED) + header.get_page_count(OPTIMIZED);
    for (auto itr = entries.begin(); itr != entries.end(); ++itr)
    {
        acc += itr->get_page_count(OPTIMIZED);
    }

    std::cout << "Optimized page size:\t" << OPTIMIZED << " bytes\n";
    std::cout << "Page count:\t\t" << acc << " pages\n";
    std::cout << "Object\t\tMin\tActual\tMax\n"
              << "Global Header\t" << gheader.min_size() << '\t' << gheader.get_page_count(OPTIMIZED)*OPTIMIZED << '\t' << gheader.max_size() << '\n'
              << "Header\t\t" << header.min_size() << '\t' << header.get_page_count(OPTIMIZED)*OPTIMIZED << '\t' << header.max_size() << '\n';

    for (auto obj : entries)
    {
        std::cout << "Entry\t\t" << obj.min_size() << '\t' << obj.get_page_count(OPTIMIZED)*OPTIMIZED << '\t' << obj.max_size() << '\n';
    }

    return 0;
}

// EOF
