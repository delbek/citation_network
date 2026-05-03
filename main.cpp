#include "CitationNetwork.h"

int main()
{
    CitationNetwork citationNetwork(
        "merged_citations.csv",
        "merged_meta.csv",
        "binary.bin",
        "citation_network.mtx",
        true
    );

    std::cout << "Number of papers: " << citationNetwork.getNumPapers() << std::endl;
    std::cout << "Number of edges: " << citationNetwork.getNumCitations() << std::endl;

    unsigned long long* distances = new unsigned long long[citationNetwork.getNumPapers()];
    std::ifstream closenessFile("citation_network_closeness_results.txt");
    unsigned long long distance;
    unsigned idx = 0;
    while (closenessFile >> distance)
    {
        distances[idx++] = distance;
    }
    closenessFile.close();

    unsigned* components = new unsigned[citationNetwork.getNumPapers()];
    std::ifstream wccFile("citation_network_weakly_components.txt");
    unsigned component;
    idx = 0;
    while (wccFile >> component)
    {
        components[idx++] = component;
    }
    wccFile.close();

    std::vector<std::string> authors = 
    {
        "Varol, Onur",
        "Saygin, Yucel",
        "Ozturk, Ozcan",
        "Savas, Erkay",
        "Yanikoglu, Berrin",
        "Erdem, Esra",
        "Aptoula, Erchan",
        "Atilgan, Canan",
        "Kaya, Kamer",
        "Yilmaz, Cemal",
        "Levi, Albert",
        "Balcisoy, Selim",
        "Tastan, Oznur",
        "Kocuk, Burak",
        "Goktas, Polat",
        "Yenigun, Husnu",
        "Arin, Inanc"
    };
    for (const auto& authorName: authors)
    {
        std::ofstream authorFile(authorName + std::string(".txt"));
        double average = 0;
        unsigned total = 0;
        std::vector<std::pair<unsigned, unsigned long long>> papers;
        for (unsigned i = 0; i < citationNetwork.getNumPapers(); ++i)
        {
            if (isAuthorOf(citationNetwork.getPapers()[i], authorName))
            {
                if (components[i] == 64 && distances[i] != 0)
                {
                    total += 1;
                    average += (static_cast<double>(1) / distances[i]);
                }
                papers.emplace_back(i, distances[i]);
            }
        }
        for (const auto& paper: papers)
        {
            authorFile << "Paper: " << citationNetwork.getPapers()[paper.first].title << " - Closeness: " << (static_cast<double>(1) / paper.second) << std::endl;
            
        }
        authorFile << authorName << " total closeness centrality on the largest component: " << average << std::endl;
        authorFile << authorName << " average closeness centrality on the largest component: " << average / total << std::endl;
        authorFile.close();
    }
    
    delete[] distances;
    delete[] components;

    return 0;
}
