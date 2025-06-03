#pragma once

#include <string>
#include <vector>

#include "ArcNote.h"

class ArcParser {
public:
    explicit ArcParser(const ConfigContext &m_cctx) : m_cctx(m_cctx) {
    }

    void ParseFile(const std::string &filePath);
    void Parse(const std::string &str);
    void ParseSingle(const std::string &str);

    std::vector<std::vector<ArcNote>> m_chains;

private:
    double m_bpm{100};
    ConfigContext m_cctx;

    std::vector<ArcNote> m_arcs;
    std::vector<bool> m_handled;

    bool FindLink(std::vector<ArcNote> &chain);
    bool HasPreceding(const ArcNote &arc);
    std::vector<ArcNote> BuildChain(size_t startIndex);
    void ParseBpm(const std::string &token);
    void ParseString(const std::string &str);
    void FinalizeArc(const std::string_view &str, ArcNote &arc);

    int ParseT(const std::string &str) const;
    static int ParseX(const std::string &str);
    static int ParseY(const std::string &str);
};
