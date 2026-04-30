#pragma once

#include "helpers.h"

class CitationNetwork
{
public:
    CitationNetwork(std::string indexFile, std::string metaFile, std::string binaryFile, std::string edgeListFile, bool binary = false);
    CitationNetwork(const CitationNetwork& other) = delete;
    CitationNetwork(CitationNetwork&& other) = delete;
    CitationNetwork& operator=(const CitationNetwork& other) = delete;
    CitationNetwork& operator=(CitationNetwork&& other) = delete;
    ~CitationNetwork();

    unsigned getNumPapers() const {return m_numPapers;};
    unsigned getNumCitations() const {return m_numCitations;};

    Paper* getPapers() const {return m_papers;}
    std::unordered_map<std::string, unsigned>& getOmitToIndex() {return m_omidToIndex;}
    unsigned* getRowPtrs() const {return m_rowPtrs;}
    unsigned* getColIds() const {return m_colIds;}

private:
    std::vector<std::string> parseCSVLine(const std::string& line);
    std::vector<Author> parseAuthors(const std::string& authorField);
    std::string extractOmid(const std::string& idField);
    void firstPass(const std::string& indexFile);
    void loadMeta(const std::string& metaFile);
    void buildCSR(const std::string& indexFile);
    void readFromBinary(const std::string& binaryFile);
    void writeToBinary(const std::string& binaryFile);
    void generateEdgeList(const std::string& edgeListFile);

private:
    unsigned m_numPapers;
    unsigned m_numCitations;

    Paper* m_papers;
    std::unordered_map<std::string, unsigned> m_omidToIndex;
    unsigned* m_rowPtrs;
    unsigned* m_colIds;
};
