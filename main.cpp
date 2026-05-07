#include "CitationNetwork.h"
#include "omp.h"

#define BIGGEST_COMPONENT 64
#define PSEUDO_DIAMETER 52

void authorLog(const CitationNetwork& citationNetwork, double* closenessValues, const std::vector<std::string>& authors)
{
    #pragma omp parallel for num_threads(omp_get_max_threads())
    for (unsigned i = 0; i < authors.size(); ++i)
    {
        std::string authorName = authors[i];
        std::ofstream authorFile(std::string("results/") + authorName + std::string(".txt"));
        double average = 0;
        unsigned total = 0;
        std::vector<std::pair<unsigned, double>> papers;
        for (unsigned i = 0; i < citationNetwork.getNumPapers(); ++i)
        {
            if (isAuthorOf(citationNetwork.getPapers()[i], authorName))
            {
                double closeness = closenessValues[i];
                average += closeness;
                ++total;
                papers.emplace_back(i, closeness);
            }
        }
        std::sort(papers.begin(), papers.end(), [](const auto& a, const auto& b)
        {
            return a.second > b.second;
        });
        for (const auto& paper: papers)
        {
            authorFile << '[' << paper.first << "] " << "Paper: " << citationNetwork.getPapers()[paper.first].title << " - Cited By: " << citationNetwork.getColPtrs()[paper.first + 1] - citationNetwork.getColPtrs()[paper.first] << " - Closeness: " << closenessValues[paper.first] << std::endl;
            
        }
        authorFile << authorName << " total closeness centrality on the largest component: " << average << std::endl;
        authorFile << authorName << " average closeness centrality on the largest component: " << (average / total) << std::endl;
        authorFile.close();
    }
}

void readData(const CitationNetwork& citationNetwork, unsigned long long*& distances, unsigned long long*& contributions, unsigned*& components, unsigned& componentSize, double*& closenessValues)
{
    distances = new unsigned long long[citationNetwork.getNumPapers()];
    contributions = new unsigned long long[citationNetwork.getNumPapers()];
    components = new unsigned[citationNetwork.getNumPapers()];

    std::ifstream closenessFile("data/citation_network_closeness-contributions_results.txt");
    unsigned long long distance;
    unsigned long long contribution;
    unsigned idx = 0;
    while (closenessFile >> distance >> contribution)
    {
        distances[idx] = distance;
        contributions[idx] = contribution;
        ++idx;
    }
    closenessFile.close();

    std::ifstream wccFile("data/citation_network_weakly_components.txt");
    unsigned component;
    componentSize = 0;
    idx = 0;
    while (wccFile >> component)
    {
        components[idx++] = component;
        if (component == BIGGEST_COMPONENT)
        {
            ++componentSize;
        }
    }
    wccFile.close();

    closenessValues = new double[citationNetwork.getNumPapers()];
    for (unsigned i = 0; i < citationNetwork.getNumPapers(); ++i)
    {
        double closeness = 0;
        if (components[i] == BIGGEST_COMPONENT)
        {
            double rawPseudoDistance = static_cast<double>(distances[i]) + static_cast<double>(PSEUDO_DIAMETER) * (componentSize - contributions[i]);
            double worstCase = static_cast<double>(PSEUDO_DIAMETER) * (componentSize);
            closeness = (worstCase / rawPseudoDistance) - 1;
        }
        closenessValues[i] = closeness;
    }
}

unsigned* sortByCloseness(const double* closenessValues, unsigned N, unsigned numBuckets = 65536)
{
    double minVal = std::numeric_limits<double>::max();
    double maxVal = 0;

    #pragma omp parallel for reduction(min:minVal) reduction(max:maxVal) num_threads(omp_get_max_threads())
    for (unsigned i = 0; i < N; ++i)
    {
        double v = closenessValues[i];
        minVal = std::min(minVal, v);
        maxVal = std::max(maxVal, v);
    }

    double range = (maxVal - minVal);
    double invRange = (numBuckets - 1) / range;

    int nThreads = omp_get_max_threads();
    std::vector<std::vector<unsigned>> threadBuckets(nThreads, std::vector<unsigned>(numBuckets, 0));

    #pragma omp parallel num_threads(nThreads)
    {
        int tid = omp_get_thread_num();
        auto& localCount = threadBuckets[tid];

        #pragma omp for schedule(static)
        for (unsigned i = 0; i < N; ++i)
        {
            double v = closenessValues[i];
            ++localCount[static_cast<unsigned>((maxVal - v) * invRange)];
        }
    }

    std::vector<unsigned> bucketCount(numBuckets, 0);
    for (int t = 0; t < nThreads; ++t)
    {
        for (unsigned b = 0; b < numBuckets; ++b)
        {
            bucketCount[b] += threadBuckets[t][b];
        }
    }

    std::vector<unsigned> bucketOffset(numBuckets + 1, 0);
    for (unsigned b = 0; b < numBuckets; ++b)
    {
        bucketOffset[b + 1] = bucketOffset[b] + bucketCount[b];
    }

    unsigned* permutation = new unsigned[N];
    std::vector<unsigned> fillPtr(bucketOffset.begin(), bucketOffset.begin() + numBuckets);
    for (unsigned i = 0; i < N; ++i)
    {
        unsigned b = static_cast<unsigned>((maxVal - closenessValues[i]) * invRange);
        permutation[fillPtr[b]++] = i;
    }

    #pragma omp parallel for schedule(dynamic) num_threads(omp_get_max_threads())
    for (unsigned b = 0; b < numBuckets; ++b)
    {
        unsigned begin = bucketOffset[b];
        unsigned end = bucketOffset[b + 1];
        std::sort(permutation + begin, permutation + end,
        [&](unsigned old1, unsigned old2)
        {
            return closenessValues[old1] > closenessValues[old2];
        });
    }

    return permutation;
}

int main()
{
    CitationNetwork citationNetwork(
        "data/merged_citations.csv",
        "data/merged_meta.csv",
        "data/binary.bin",
        "data/citation_network.mtx",
        true
    );

    std::cout << "Number of papers: " << citationNetwork.getNumPapers() << std::endl;
    std::cout << "Number of edges: " << citationNetwork.getNumCitations() << std::endl;

    unsigned long long* distances;
    unsigned long long* contributions;
    unsigned* components;
    unsigned componentSize;
    double* closenessValues;
    readData(citationNetwork, distances, contributions, components, componentSize, closenessValues);

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
        "Goktas, Polat",
        "Yenigun, Husnu"
    };
    authorLog(citationNetwork, closenessValues, authors);

    unsigned* permutation = sortByCloseness(closenessValues, citationNetwork.getNumPapers());
    unsigned top = 100;
    std::cout << "Top " << top << " papers:" << std::endl;
    for (unsigned r = 0; r < top; ++r)
    {
        unsigned old = permutation[r];
        std::cout << '[' << old << "] " << "Paper: " << citationNetwork.getPapers()[old].title << " - Cited By: " << citationNetwork.getColPtrs()[old + 1] - citationNetwork.getColPtrs()[old] << " - Closeness: " << closenessValues[old] << std::endl;
    }
    

    delete[] permutation;
    delete[] distances;
    delete[] contributions;
    delete[] components;
    delete[] closenessValues;

    return 0;
}
