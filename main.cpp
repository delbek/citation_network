#include "CitationNetwork.h"

int main()
{
    CitationNetwork citationNetwork(
        DIRECTORY + std::string("merged_citations.csv"),
        DIRECTORY + std::string("merged_meta.csv"),
        DIRECTORY + std::string("binary.bin"),
        DIRECTORY + std::string("citation_network.mtx"),
        true
    );

    std::cout << "Number of papers: " << citationNetwork.getNumPapers() << std::endl;
    std::cout << "Number of edges: " << citationNetwork.getNumCitations() << std::endl;

    return 0;
}
