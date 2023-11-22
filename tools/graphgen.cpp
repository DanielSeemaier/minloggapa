/**
 * CLI tool to generate graphs
 */
#include <iosfwd>
#include <iostream>
#include <getopt.h>
#include <data_structure/graph_access.h>
#include <functional>
#include <io/graph_io.h>

void print_usage(std::ostream &stream, const char *name);

void generate_clique(graph_access &G, int argc, char **argv);

void generate_biclique(graph_access &G, int argc, char **argv);

void generate_increasing_cliques(graph_access &G, int argc, char **argv);

void generate_cycle_with_chords(graph_access &G, int argc, char **argv);

void generate_rectangle(graph_access &G, int argc, char **argv);

void generate_cycle(graph_access &G, int argc, char **argv);

int main(int argc, char **argv) {
    option arguments[] = {
            {
                    "help",
                    no_argument,
                    nullptr,
                    'h'
            },
            {
                    "type",
                    required_argument,
                    nullptr,
                    't'
            },
            {
                    nullptr,
                    0,
                    nullptr,
                    0
            }
    };

    int value = 0;
    std::function<void(graph_access &, int, char **)> generator;
    bool generator_specified = false;
    do {
        value = getopt_long(argc, argv, "ht:", arguments, nullptr);

        switch (value) {
            case 'h': {
                print_usage(std::cout, argv[0]);
                std::exit(0);
            }

            case 't': {
                generator_specified = true;
                std::string type(optarg);

                if (type == "clique") {
                    generator = generate_clique;
                } else if (type == "biclique") {
                    generator = generate_biclique;
                } else if (type == "increasing_cliques") {
                    generator = generate_increasing_cliques;
                } else if (type == "cycle_with_chords") {
                    generator = generate_cycle_with_chords;
                } else if (type == "rect") {
                    generator = generate_rectangle;
                } else if (type == "cycle") {
                    generator = generate_cycle;
                } else {
                    std::cerr << "Error: Invalid type: " << type << std::endl;
                    print_usage(std::cerr, argv[0]);
                    std::exit(1);
                }

                break;
            }

            case '?': {
                std::cerr << "Unknown argument: " << optarg << std::endl;
                print_usage(std::cerr, argv[0]);
                std::exit(1);
            }

            default: {
                // do nothing
            }
        }
    } while (value != -1);

    if (argc - optind < 1) {
        std::cerr << "Error: No output file specified" << std::endl;
        print_usage(std::cerr, argv[0]);
        std::exit(1);
    }
    std::string filename(argv[optind]);
    ++optind;

    if (!generator_specified) {
        std::cerr << "Error: No generator function specified" << std::endl;
        print_usage(std::cerr, argv[0]);
        std::exit(1);
    }

    graph_access G;
    generator(G, argc, argv);

    graph_io::writeGraph(G, filename);
}

void print_usage(std::ostream &stream, const char *name) {
    stream << "Usage: " << name << " OPTIONS output TYPE_OPTIONS" << std::endl;
    stream << "\t-h. --help: Show this message" << std::endl;
    stream << "\t-t type, --type=type: One of the following available types:" << std::endl;
    stream << "\t\tclique: Generates a clique of the specified size; TYPE_OPTIONS: size" << std::endl;
    stream << "\t\tbiclique: Generates a graph with two cliques that have one or zero edge(s) in between; TYPE_OPTIONS: size0 size1 connect" << std::endl;
    stream << "\t\tincreasing_cliques: TYPE_OPTION: size" << std::endl;
    stream << "\t\trect: TYPE_OPTION: size" << std::endl;
    stream << "\t\tcycle: TYPE_OPTION: size" << std::endl;
    stream << "Example: " << name << " --type=clique out.graph 100" << std::endl;
}

void generate_clique(graph_access &G, int argc, char **argv) {
    if (argc - optind != 1) {
        std::cerr << "Error: You must specify the size of the clique" << std::endl;
        print_usage(std::cerr, argv[0]);
        std::exit(1);
    }

    auto size = static_cast<NodeID>(std::atoi(argv[optind]));
    std::cout << "Info: Generating clique with " << size << " nodes" << std::endl;

    G.start_construction(size, size * (size - 1));
    for (NodeID v = 0; v < G.number_of_nodes(); ++v) {
        NodeID node = G.new_node();
        G.setNodeWeight(node, 1);

        for (NodeID u = 0; u < G.number_of_nodes(); ++u) {
            if (v == u) continue;
            EdgeID edge = G.new_edge(v, u);
            G.setEdgeWeight(edge, 1);
        }
    }
    G.finish_construction();
}

void generate_biclique(graph_access &G, int argc, char **argv) {
    if (argc - optind != 3) {
        std::cerr << "Error: You must specify the size of each clique and whether they should be connected" << std::endl;
        print_usage(std::cerr, argv[0]);
        std::exit(1);
    }

    auto size0 = static_cast<NodeID>(std::stol(argv[optind]));
    ++optind;
    auto size1 = static_cast<NodeID>(std::stol(argv[optind]));
    ++optind;
    bool connected = (std::stol(argv[optind]) == 1);
    std::cout << "Info: Generating cliques with " << size0 << " and " << size1 << " nodes" << std::endl;
    if (connected) {
        std::cout << "Info: Cliques are connected" << std::endl;
    }

    G.start_construction(size0 + size1, size0 * (size0 - 1) + size1 * (size1 - 1) + (connected ? 2 : 0));
    for (NodeID v = 0; v < size0; ++v) {
        NodeID node = G.new_node();
        G.setNodeWeight(node, 1);

        for (NodeID u = 0; u < size0; ++u) {
            if (v == u) continue;
            EdgeID edge = G.new_edge(v, u);
            G.setEdgeWeight(edge, 1);
        }

        if (v == 0 && connected) {
            EdgeID edge = G.new_edge(v, size0);
            G.setEdgeWeight(edge, 1);
        }
    }
    for (NodeID v = size0; v < size0 + size1; ++v) {
        NodeID node = G.new_node();
        G.setNodeWeight(node, 1);

        for (NodeID u = size0; u < size0 + size1; ++u) {
            if (v == u) continue;
            EdgeID edge = G.new_edge(v, u);
            G.setEdgeWeight(edge, 1);
        }

        if (v == size0 && connected) {
            EdgeID edge = G.new_edge(v, 0);
            G.setEdgeWeight(edge, 1);
        }
    }
    G.finish_construction();
}

void generate_increasing_cliques(graph_access &G, int argc, char **argv) {
    if (argc - optind != 1) {
        std::cerr << "Error: You must specify the number of cliques that are to be generated" << std::endl;
        print_usage(std::cerr, argv[0]);
        std::exit(1);
    }

    auto size = static_cast<NodeID>(std::stol(argv[optind]));
    if (size == 0) {
        std::cerr << "Error: Size must be greater than zero" << std::endl;
        print_usage(std::cerr, argv[0]);
        std::exit(1);
    }

    NodeID number_of_nodes = (size * (size + 1)) / 2;
    EdgeID number_of_edges = 2 * (size - 1);
    for (NodeID clique = 1; clique <= size; ++clique) {
        number_of_edges += clique * (clique - 1);
    }

    G.start_construction(number_of_nodes, number_of_edges);

    NodeID offset = 0;
    for (NodeID clique = 1; clique <= size; ++clique) {
        for (NodeID v = offset; v < offset + clique; ++v) {
            NodeID node = G.new_node();
            G.setNodeWeight(node, 1);
            assert (v == node);

            if (v == offset && clique > 1) {
                EdgeID edge = G.new_edge(v, v - 1);
                G.setEdgeWeight(edge, 1);
            }
            if (v == offset + clique - 1 && clique < size) {
                EdgeID edge = G.new_edge(v, v + 1);
                G.setEdgeWeight(edge, 1);
            }

            for (NodeID u = offset; u < offset + clique; ++u) {
                if (v == u) continue;
                EdgeID edge = G.new_edge(v, u);
                G.setEdgeWeight(edge, 1);
            }
        }

        offset += clique;
    }

    G.finish_construction();
}

void generate_cycle_with_chords(graph_access &G, int argc, char **argv) {
    if (argc - optind != 1) {
        std::cerr << "Error: You must specify the number of nodes" << std::endl;
        std::exit(1);
    }

    auto size = static_cast<NodeID>(std::stol(argv[optind]));
    if (size % 2 != 0) {
        std::cerr << "Error: Only even numbers of nodes are supported" << std::endl;
        std::exit(1);
    }

    G.start_construction(size, size * 3);

    for (NodeID v = 0; v < size; ++v) {
        NodeID node = G.new_node();
        G.setNodeWeight(node, 1);
        assert (v == node);

        G.setEdgeWeight(G.new_edge(v, (v == 0) ? size - 1 : v - 1), 1);
        G.setEdgeWeight(G.new_edge(v, (v + 1) % size), 1);
        if (v < size / 2) {
            G.setEdgeWeight(G.new_edge(v, v + size / 2), 1);
        } else {
            G.setEdgeWeight(G.new_edge(v, v - size / 2), 1);
        }
    }

    G.finish_construction();
}

void generate_rectangle(graph_access &G, int argc, char **argv) {
    if (argc - optind != 1) {
        std::cerr << "Error: You must specify the number of nodes" << std::endl;
        std::exit(1);
    }

    auto size = static_cast<NodeID>(std::stol(argv[optind]));

    G.start_construction(size * size, 4 * (size - 1) * size);
    for (NodeID v = 0; v < size * size; ++v) {
        NodeID node = G.new_node();
        G.setNodeWeight(node, 1);
        assert (v == node);

        NodeID x = v % size;
        NodeID y = v / size;

        if (x > 0) G.setEdgeWeight(G.new_edge(v, (x - 1) + y * size), 1);
        if (x < size - 1) G.setEdgeWeight(G.new_edge(v, (x + 1) + y * size), 1);
        if (y > 0) G.setEdgeWeight(G.new_edge(v, x + (y - 1) * size), 1);
        if (y < size - 1) G.setEdgeWeight(G.new_edge(v, x + (y + 1) * size), 1);
    }
    G.finish_construction();
}

void generate_cycle(graph_access &G, int argc, char **argv) {
    if (argc - optind != 1) {
        std::cerr << "Error: You must specify the number of nodes" << std::endl;
        std::exit(1);
    }

    auto size = static_cast<NodeID>(std::stol(argv[optind]));

    if (size < 3) {
        std::cerr << "Error: Size must be bigger than 2" << std::endl;
        std::exit(1);
    }

    G.start_construction(size, 2 * size);
    for (NodeID v = 0; v < size; ++v) {
        NodeID node = G.new_node();
        G.setNodeWeight(node, 1);
        assert (v == node);

        G.setEdgeWeight(G.new_edge(v, (v + 1) % size), 1);
        if (v == 0) {
            G.setEdgeWeight(G.new_edge(v, size - 1), 1);
        } else {
            G.setEdgeWeight(G.new_edge(v, v - 1), 1);
        }
    }
    G.finish_construction();
}