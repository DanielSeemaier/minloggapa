/**
 * CLI tool that applies a linear layout to a graph
 */

#include <getopt.h>
#include <data_structure/graph_access.h>
#include <utils.h>
#include <ctime>
#include <io/graph_io.h>

using namespace bathesis;

void error(const std::string &message, const char *app_name);
void print_usage(std::ostream &stream, const char *name);

int main(int argc, char **argv) {
    option arguments[] = {
            {
                    "help",
                    no_argument,
                    nullptr,
                    'h'
            },
            {
                    "randomly",
                    no_argument,
                    nullptr,
                    'r'
            },
            {
                    "seed",
                    required_argument,
                    nullptr,
                    's'
            },
            {
                    "layout",
                    required_argument,
                    nullptr,
                    'l'
            },
            {
                    nullptr,
                    0,
                    nullptr,
                    0
            }
    };

    auto value = 0;
    auto seed = static_cast<uint>(std::time(nullptr));
    auto permute_randomly = false;
    std::vector<NodeID> linear_layout;
    std::string linear_layout_filename;
    do {
        value = getopt_long(argc, argv, "hrs:", arguments, nullptr);
        switch (value) {

            case 'r': {
                permute_randomly = true;
                break;
            }

            case 's': {
                seed = static_cast<uint>(std::stoul(optarg));
                break;
            }

            case 'l': {
                linear_layout_filename = optarg;
                break;
            }

            case '?': {
                error(std::string("Unknown argument: ") + optarg, argv[0]);
            }

            default: {
                // do nothing
            }
        }
    } while (value != -1);

    if (argc - optind != 2) {
        error("Expecting two arguments: input and output filename", argv[0]);
    }

    std::string input_filename(argv[optind++]);
    std::string output_filename(argv[optind++]);

    graph_access original;
    graph_io::readGraphWeighted(original, input_filename);

    if (permute_randomly) {
        std::cout << "Info: Applying random permutation generated with seed " << seed << std::endl;
        std::srand(seed);
        linear_layout = utils::create_random_layout(original);
    } else {
        std::cout << "Info: Applying linear layout read from " << linear_layout_filename << std::endl;
        linear_layout.resize(original.number_of_nodes());
        graph_io::readVector(linear_layout, linear_layout_filename);
    }

    graph_access reordered;
    utils::apply_linear_layout(original, reordered, linear_layout);
    graph_io::writeGraph(reordered, output_filename);

    return EXIT_SUCCESS;
}

void error(const std::string &message, const char *app_name) {
    std::cerr << "Error: " << message << std::endl;
    print_usage(std::cerr, app_name);
    std::exit(1);
}

void print_usage(std::ostream &stream, const char *name) {
    stream << "Usage: " << name << " OPTIONS input output" << std::endl;
    stream << "\t-l file, --layout=file: Linear layout" << std::endl;
    stream << "\t-h, --help: Show this message" << std::endl;
    stream << "\t-r, --randomly: Apply a random linear layout" << std::endl;
    stream << "\t-s seed, --seed=seed: Seed to use for random linear layout; default: time based seed" << std::endl;
    stream << "Example: " << name << " -r input.graph output.graph" << std::endl;
}

