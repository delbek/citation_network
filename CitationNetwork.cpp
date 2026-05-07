#include "CitationNetwork.h"

CitationNetwork::CitationNetwork(std::string indexFile, std::string metaFile, std::string binaryFile, std::string edgeListFile, bool binary)
: m_papers(nullptr), m_rowPtrs(nullptr), m_colIds(nullptr), m_numPapers(0), m_numCitations(0)
{
    if (binary)
    {
        readFromBinary(binaryFile);
    }
    else
    {
        firstPass(indexFile);
        std::cout << "Index loaded." << std::endl;
        loadMeta(metaFile);
        std::cout << "Meta loaded." << std::endl;
        buildCSR(indexFile);
        std::cout << "CSR constructed." << std::endl;
        writeToBinary(binaryFile);
        std::cout << "Written to binary." << std::endl;
        generateEdgeList(edgeListFile);
        std::cout << "Edge list generated." << std::endl;
    }
}

CitationNetwork::~CitationNetwork()
{
    delete[] m_papers;
    delete[] m_rowPtrs;
    delete[] m_colIds;
    delete[] m_colPtrs;
    delete[] m_rowIds;
}

std::vector<std::string> CitationNetwork::parseCSVLine(const std::string& line)
{
    std::vector<std::string> fields;
    std::string field;
    bool inQuotes = false;
    for (char c : line)
    {
        if (c == '\r')
        {
            continue;
        }
        if (c == '"')
        {
            inQuotes = !inQuotes;
        }
        else if (c == ',' && !inQuotes)
        {
            fields.push_back(field);
            field.clear();
        }
        else
        {
            field += c;
        }
    }
    fields.push_back(field);
    return fields;
}

std::vector<Author> CitationNetwork::parseAuthors(const std::string& authorField)
{
    std::vector<std::string> entries;
    std::string entry;
    for (unsigned i = 0; i < authorField.size(); ++i)
    {
        if (authorField[i] == ';')
        {
            if (!entry.empty())
            {
                while (!entry.empty() && entry.front() == ' ')
                {
                    entry.erase(entry.begin());
                }
                entries.push_back(entry);
                entry.clear();
            }
        }
        else
        {
            entry += authorField[i];
        }
    }
    if (!entry.empty())
    {
        while (!entry.empty() && entry.front() == ' ')
        {
            entry.erase(entry.begin());
        }
        entries.push_back(entry);
    }

    std::vector<Author> authors(entries.size());
    for (unsigned i = 0; i < entries.size(); ++i)
    {
        const std::string& e = entries[i];
        unsigned bracketOpen = e.find('[');
        if (bracketOpen != std::string::npos)
        {
            authors[i].fullName = e.substr(0, bracketOpen - 1);
            unsigned bracketClose = e.find(']', bracketOpen);
            std::string idBlock = e.substr(bracketOpen + 1, bracketClose - bracketOpen - 1);
            std::istringstream ss(idBlock);
            std::string token;
            while (ss >> token)
            {
                if (token.rfind("omid:ra/", 0) == 0)
                {
                    authors[i].omid = token;
                    break;
                }
            }
        }
        else
        {
            authors[i].fullName = e;
        }
    }
    return authors;
}

std::string CitationNetwork::extractOmid(const std::string& idField)
{
    std::istringstream ss(idField);
    std::string token;
    while (ss >> token)
    {
        if (token.rfind("omid:br/", 0) == 0)
        {
            return token;
        }
    }
    return "";
}

void CitationNetwork::firstPass(const std::string& indexFile)
{
    std::ifstream f(indexFile);
    std::string line;
    std::getline(f, line);
    unsigned idx = 0;
    while (std::getline(f, line))
    {
        auto fields = parseCSVLine(line);
        if (fields.size() < 3)
        {
            continue;
        }
        const std::string& citing = fields[1];
        const std::string& cited = fields[2];
        if (m_omidToIndex.find(citing) == m_omidToIndex.end())
        {
            m_omidToIndex[citing] = idx++;
        }
        if (m_omidToIndex.find(cited) == m_omidToIndex.end())
        {
            m_omidToIndex[cited] = idx++;
        }
    }
    m_numPapers = idx;
    m_papers = new Paper[m_numPapers];
    for (auto& [omid, i]: m_omidToIndex)
    {
        m_papers[i].omid = omid;
    }
}

void CitationNetwork::loadMeta(const std::string& metaFile)
{
    std::ifstream f(metaFile);
    std::string line;
    std::getline(f, line);
    while (std::getline(f, line))
    {
        while (std::count(line.begin(), line.end(), '"') % 2 != 0)
        {
            std::string continuation;
            if (!std::getline(f, continuation))
            {
                break;
            }
            line += ' ' + continuation;
        }
        auto fields = parseCSVLine(line);
        if (fields.size() < 11)
        {
            continue;
        }
        std::string omid = extractOmid(fields[0]);
        if (omid.empty())
        {
            continue;
        }
        auto it = m_omidToIndex.find(omid);
        if (it == m_omidToIndex.end())
        {
            continue;
        }
        unsigned i = it->second;
        m_papers[i].omid = omid;
        m_papers[i].title = fields[1];
        m_papers[i].authors = parseAuthors(fields[2]);
        m_papers[i].pubDate = fields[7];
        m_papers[i].venue = fields[5];
        m_papers[i].type = fields[8];
        m_papers[i].publisher = fields[9];
    }
}

void CitationNetwork::buildCSR(const std::string& indexFile)
{
    m_rowPtrs = new unsigned[m_numPapers + 1]();

    std::ifstream f1(indexFile);
    std::string line;
    std::getline(f1, line);
    while (std::getline(f1, line))
    {
        auto fields = parseCSVLine(line);
        if (fields.size() < 3)
        {
            continue;
        }
        auto it = m_omidToIndex.find(fields[1]);
        if (it != m_omidToIndex.end())
        {
            m_rowPtrs[it->second + 1]++;
        }
    }
    f1.close();

    for (unsigned i = 0; i < m_numPapers; ++i)
    {
        m_rowPtrs[i + 1] += m_rowPtrs[i];
    }
    m_numCitations = m_rowPtrs[m_numPapers];
    m_colIds = new unsigned[m_numCitations];

    unsigned* fillPtr = new unsigned[m_numPapers + 1];
    for (unsigned i = 0; i <= m_numPapers; ++i)
    {
        fillPtr[i] = m_rowPtrs[i];
    }

    std::ifstream f2(indexFile);
    std::getline(f2, line);
    while (std::getline(f2, line))
    {
        auto fields = parseCSVLine(line);
        if (fields.size() < 3)
        {
            continue;
        }
        auto itSrc = m_omidToIndex.find(fields[1]);
        auto itDst = m_omidToIndex.find(fields[2]);
        if (itSrc == m_omidToIndex.end() || itDst == m_omidToIndex.end())
        {
            continue;
        }
        unsigned src = itSrc->second;
        unsigned dst = itDst->second;
        m_colIds[fillPtr[src]++] = dst;
    }
    f2.close();

    m_colPtrs = new unsigned[m_numPapers + 1]();
    m_rowIds = new unsigned[m_numCitations];
    for (unsigned i = 0; i < m_numPapers; ++i)
    {
        for (unsigned nnz = m_rowPtrs[i]; nnz < m_rowPtrs[i + 1]; ++nnz)
        {
            unsigned j = m_colIds[nnz];
            ++m_colPtrs[j + 1];
        }
    }
    for (unsigned j = 0; j < m_numPapers; ++j)
    {
        m_colPtrs[j + 1] += m_colPtrs[j];
        fillPtr[j] = m_colPtrs[j];
    }
    
    for (unsigned i = 0; i < m_numPapers; ++i)
    {
        for (unsigned nnz = m_rowPtrs[i]; nnz < m_rowPtrs[i + 1]; ++nnz)
        {
            unsigned j = m_colIds[nnz];
            m_rowIds[fillPtr[j]++] = i;
        }
    }
}

void CitationNetwork::writeToBinary(const std::string& binaryFile)
{
    std::ofstream f(binaryFile, std::ios::binary);

    f.write(reinterpret_cast<const char*>(&m_numPapers), sizeof(unsigned));
    f.write(reinterpret_cast<const char*>(&m_numCitations), sizeof(unsigned));
    f.write(reinterpret_cast<const char*>(m_rowPtrs), sizeof(unsigned) * (m_numPapers + 1));
    f.write(reinterpret_cast<const char*>(m_colIds), sizeof(unsigned) * m_numCitations);
    f.write(reinterpret_cast<const char*>(m_colPtrs), sizeof(unsigned) * (m_numPapers + 1));
    f.write(reinterpret_cast<const char*>(m_rowIds), sizeof(unsigned) * m_numCitations);

    for (unsigned i = 0; i < m_numPapers; ++i)
    {
        const std::string* stringFields[6] =
        {
            &m_papers[i].omid,
            &m_papers[i].title,
            &m_papers[i].pubDate,
            &m_papers[i].venue,
            &m_papers[i].type,
            &m_papers[i].publisher
        };
        for (const std::string* s: stringFields)
        {
            unsigned len = s->size();
            f.write(reinterpret_cast<const char*>(&len), sizeof(unsigned));
            f.write(s->data(), len);
        }

        unsigned numAuthors = m_papers[i].authors.size();
        f.write(reinterpret_cast<const char*>(&numAuthors), sizeof(unsigned));
        for (unsigned j = 0; j < numAuthors; j++)
        {
            const std::string* authorFields[2] =
            {
                &m_papers[i].authors[j].omid,
                &m_papers[i].authors[j].fullName
            };
            for (const std::string* s: authorFields)
            {
                unsigned len = s->size();
                f.write(reinterpret_cast<const char*>(&len), sizeof(unsigned));
                f.write(s->data(), len);
            }
        }
    }
}

void CitationNetwork::readFromBinary(const std::string& binaryFile)
{
    std::ifstream f(binaryFile, std::ios::binary);

    f.read(reinterpret_cast<char*>(&m_numPapers), sizeof(unsigned));
    f.read(reinterpret_cast<char*>(&m_numCitations), sizeof(unsigned));

    m_rowPtrs = new unsigned[m_numPapers + 1];
    m_colIds = new unsigned[m_numCitations];
    m_colPtrs = new unsigned[m_numPapers + 1];
    m_rowIds = new unsigned[m_numCitations];
    m_papers = new Paper[m_numPapers];

    f.read(reinterpret_cast<char*>(m_rowPtrs), sizeof(unsigned) * (m_numPapers + 1));
    f.read(reinterpret_cast<char*>(m_colIds), sizeof(unsigned) * m_numCitations);
    f.read(reinterpret_cast<char*>(m_colPtrs), sizeof(unsigned) * (m_numPapers + 1));
    f.read(reinterpret_cast<char*>(m_rowIds), sizeof(unsigned) * m_numCitations);

    for (unsigned i = 0; i < m_numPapers; ++i)
    {
        std::string* stringFields[6] =
        {
            &m_papers[i].omid,
            &m_papers[i].title,
            &m_papers[i].pubDate,
            &m_papers[i].venue,
            &m_papers[i].type,
            &m_papers[i].publisher
        };
        for (std::string* s: stringFields)
        {
            unsigned len = 0;
            f.read(reinterpret_cast<char*>(&len), sizeof(unsigned));
            s->resize(len);
            f.read(&(*s)[0], len);
        }

        unsigned numAuthors = 0;
        f.read(reinterpret_cast<char*>(&numAuthors), sizeof(unsigned));
        m_papers[i].authors.resize(numAuthors);
        for (unsigned j = 0; j < numAuthors; j++)
        {
            std::string* authorFields[2] =
            {
                &m_papers[i].authors[j].omid,
                &m_papers[i].authors[j].fullName
            };
            for (std::string* s: authorFields)
            {
                unsigned len = 0;
                f.read(reinterpret_cast<char*>(&len), sizeof(unsigned));
                s->resize(len);
                f.read(&(*s)[0], len);
            }
        }

        m_omidToIndex[m_papers[i].omid] = i;
    }
}

void CitationNetwork::generateEdgeList(const std::string& edgeListFile)
{
    std::ofstream file(edgeListFile);

    const std::string header = 
    R"(%%MatrixMarket matrix coordinate pattern general
%-------------------------------------------------------------------------------
% name: citation_network_2026
% date: 2026
% author: Open Citations
% ed: D. Elbek, K. Kaya
% kind: directed graph
%-------------------------------------------------------------------------------)";

    file << header << std::endl;
    file << m_numPapers << ' ' << m_numPapers << ' ' << m_numCitations << std::endl;
    for (unsigned i = 0; i < m_numPapers; ++i)
    {
        for (unsigned ptr = m_rowPtrs[i]; ptr < m_rowPtrs[i + 1]; ++ptr)
        {
            unsigned j = m_colIds[ptr];
            file << i + 1 << ' ' << j + 1 << std::endl;
        }
    }

    file.close();
}
