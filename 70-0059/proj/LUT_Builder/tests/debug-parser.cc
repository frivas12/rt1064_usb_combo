#include "lut-builder/cmd-line-parser.hh"

#include <iostream>

int main(int argc, char ** argv)
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

    std::cout << "Opt page size:  " << ((args.optimize_page_size) ? "true" : "false") << '\n'
              << "LUT Page Size:  " << args.lut_page_size << '\n';

    std::cout << "Targets:\n";
    for (std::size_t i = 0; i < args.targets.size(); ++i)
    {
        std::cout << "\t" << std::to_string(i + 1) << ":  [" << args.targets[i].input_file_paths[0];
        for (std::size_t k = 1; k < args.targets[i].input_file_paths.size(); ++k)
        {
            std::cout << ", " << args.targets[i].input_file_paths[k];
        }
        std::cout << "] -> " << args.targets[i].output_file_path << '\n';
    }
    std::cout << "Keys:\n";
    for (std::size_t i = 0; i < args.key_sizes.size(); ++i)
    {
        std::cout << "\t" << std::to_string(i + 1) << ":  " << std::to_string(args.key_sizes[i]) << " byte";
        if (args.key_sizes[i] != 0)
            std::cout << "s";
        std::cout << "\n";    }

    return 0;
}
