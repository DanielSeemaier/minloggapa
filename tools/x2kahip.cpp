/**
 * CLI tool that converts a graph in some format to the one KaHIP uses.
 */

#include <map>
#include <functional>
#include <getopt.h>

#include <io/graph_io.h>

void print_usage(std::ostream &stream, const char *name);

void load_rmf(graph_access &G, std::string filename);

void load_colonsep_edge_list(graph_access &G, std::string filename);

int main(int argc, char *argv[]) {
    bool specified_loader = false;
    std::function<void(graph_access &, std::string)> loader;

    option arguments[] = {
            {
                    "help",
                    no_argument,
                    nullptr,
                    'h'
            },
            {
                    "format",
                    required_argument,
                    nullptr,
                    'f'
            },
            {
                    nullptr,
                    0,
                    nullptr,
                    0
            }
    };

    int value = 0;
    do {
        value = getopt_long(argc, argv, "f:h", arguments, nullptr);

        switch (value) {
            case 'h': {
                print_usage(std::cout, argv[0]);
                std::exit(0);
            }

            case 'f': {
                specified_loader = true;
                std::string format(optarg);

                if (format == "rmf") {
                    loader = load_rmf;
                } else if (format == "colonsep") {
                    loader = load_colonsep_edge_list;
                } else {
                    std::cerr << "Unknown graph format: " << format << std::endl;
                    print_usage(std::cerr, argv[0]);
                    std::exit(1);
                }
                break;
            }

            case '?': {
                print_usage(std::cerr, argv[0]);
                std::exit(1);
            }

            default: {
                // do nothing
            }
        }
    } while (value != -1);

    if (argc - optind != 2 || !specified_loader) {
        print_usage(std::cerr, argv[0]);
        std::exit(1);
    }
    std::string input_filename(argv[optind]);
    std::string output_filename(argv[optind + 1]);

    try {
        graph_access G;
        loader(G, input_filename);

        std::cout << "|V| = " << G.number_of_nodes() << std::endl;
        std::cout << "|E| = " << G.number_of_edges() << std::endl;

        graph_io::writeGraphWeighted(G, output_filename);
    } catch (const char *msg) {
        std::cerr << "Specified graph could not be loaded; the format specific loader reported: " << std::endl;
        std::cerr << "\t" << msg << std::endl;
        std::cerr << "Did you specify the appropriate file format?" << std::endl;
        std::exit(1);
    }


    return 0;
}

void print_usage(std::ostream &stream, const char *name) {
    stream << "Usage: " << name << " --format=[rmf|colonsep] input output" << std::endl;
}

/**
 * File format that specifies the number of nodes and edges in the first line:
 * <code>d ghct [number of nodes] [number of edges]</code>
 *
 * Then, every line contains an edge in the form of
 * <code>e [source] [target] 1</code>
 *
 * Node ids start at 1 instead of 0.
 *
 * @param G
 * @param filename
 */
void load_rmf(graph_access &G, std::string filename) {
    std::ifstream in(filename);

    std::string line;
    std::getline(in, line);

    // read number of nodes and edges
    if (line[0] != 'd') throw "first line must start with 'd'";

    line = line.substr(line.find(' ') + 1);
    line = line.substr(line.find(' ') + 1);
    NodeID number_of_nodes = static_cast<NodeID>(std::atoi(line.substr(0, line.find(' ')).c_str()));
    line = line.substr(line.find(' ') + 1);
    EdgeID number_of_edges = static_cast<EdgeID>(std::atoi(line.c_str()));

    G.start_construction(number_of_nodes, number_of_edges);

    for (NodeID node = 0; node < number_of_nodes; ++node) {
        NodeID new_node_id = G.new_node();
        G.setNodeWeight(new_node_id, 1);
        assert(node == new_node_id);
    }

    while (std::getline(in, line)) {
        if (line[0] != 'e') throw "edge line must start with 'e'";

        line = line.substr(line.find(' ') + 1);
        NodeID source = static_cast<NodeID>(std::atoi(line.substr(0, line.find(' ')).c_str()));
        line = line.substr(line.find(' ') + 1);
        NodeID target = static_cast<NodeID>(std::atoi(line.substr(0, line.find(' ')).c_str()));

        // starts counting at 1, hence "-1"
        EdgeID new_edge_id = G.new_edge(source - 1, target - 1);
        G.setEdgeWeight(new_edge_id, 1);
    }

    G.finish_construction();
}

/**
 * Graph format where every line specifies one edge (source and target are separated by a colon):
 * <code>[source]:[target]</code>
 *
 * This format doesn't specify the number of nodes or edges in advance. The graph is implicitly undirected, i.e. for
 * every edge source:target, another edge target:source is inserted automatically. The list of edges must be sorted.
 *
 * Node ids start at 1 instead of 0.
 *
 * @param G
 * @param filename
 */
void load_colonsep_edge_list(graph_access &G, std::string filename) {
    std::ifstream in(filename);

    std::map<NodeID, std::vector<NodeID>> graph;
    NodeID num_nodes = 0;
    NodeID num_edges = 0;

    std::string line;
    while (std::getline(in, line)) {
        size_t delimiter = line.find(':');
        NodeID from = static_cast<NodeID>(std::atoi(line.substr(0, delimiter).c_str()));
        NodeID to = static_cast<NodeID>(std::atoi(line.substr(delimiter + 1).c_str()));

        if (!graph.count(from)) {
            graph[from] = std::vector<NodeID>();
        }
        if (!graph.count(to)) {
            graph[to] = std::vector<NodeID>();
        }

        graph[from].push_back(to);
        graph[to].push_back(from);

        num_nodes = std::max(num_nodes, std::max(from, to));
        num_edges += 2;
    }
    ++num_nodes;

    G.start_construction(num_nodes, num_edges);

    for (NodeID u = 0; u < num_nodes; ++u) {
        NodeID new_node_id = G.new_node();
        G.setNodeWeight(new_node_id, 1);
        assert (u == new_node_id);

        if (graph.count(u)) {
            for (auto &v : graph[u]) {
                EdgeID new_edge_id = G.new_edge(u, v);
                G.setEdgeWeight(new_edge_id, 1);
            }
        }
    }

    G.finish_construction();
}
