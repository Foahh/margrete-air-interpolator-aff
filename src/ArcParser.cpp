#include <fstream>
#include <sstream>

#include "ArcNote.h"
#include "ArcParser.h"

void ArcParser::ParseSingle(const std::string &str) {
    const size_t start = str.find('(');
    const size_t end = str.rfind(')');
    if (start == std::string::npos || end == std::string::npos) {
        throw std::invalid_argument("Invalid arc format - missing parentheses");
    }

    const std::string content = str.substr(start + 1, end - start - 1);
    std::vector<std::string> parts;
    std::stringstream ss(content);
    std::string part;

    while (getline(ss, part, ',')) {
        parts.push_back(part);
    }

    if (parts.size() < 10) {
        throw std::invalid_argument("Invalid arc format - not enough parameters");
    }

    ArcNote first;
    first.t = ParseT(parts[0]);
    first.toT = ParseT(parts[1]);
    if (first.Length < 0) {
        throw std::invalid_argument("Invalid arc format - time length must be greater than 0");
    }
    first.x = ParseX(parts[2]);
    first.toX = ParseX(parts[3]);
    first.y = ParseY(parts[5]);
    first.toY = ParseY(parts[6]);
    first.type = stoi(parts[7]);
    first.trace = parts[9] != "false";

    FinalizeArc(parts[4], first);
}

void ArcParser::FinalizeArc(const std::string_view &str, ArcNote &arc) {
    using enum EasingMode;

    if (arc.Length >= 2 && str == "b") {
        ArcNote first = arc;
        first.toT = arc.t + arc.Length / 2;
        first.toX = (arc.x + arc.toX) / 2;
        first.toY = (arc.y + arc.toY) / 2;
        first.eX = Out;
        first.eY = Out;

        ArcNote second = arc;
        second.t = first.toT;
        second.x = first.toX;
        second.y = first.toY;
        second.eX = In;
        second.eY = In;

        m_arcs.push_back(first);
        m_arcs.push_back(second);
    } else {
        if (str == "si") {
            arc.eX = In;
            arc.eY = Linear;
        } else if (str == "so") {
            arc.eX = Out;
            arc.eY = Linear;
        } else if (str == "sisi") {
            arc.eX = In;
            arc.eY = In;
        } else if (str == "soso") {
            arc.eX = Out;
            arc.eY = Out;
        } else if (str == "siso") {
            arc.eX = In;
            arc.eY = Out;
        } else if (str == "sosi") {
            arc.eX = Out;
            arc.eY = In;
        } else {
            arc.eX = Linear;
            arc.eY = Linear;
        }

        m_arcs.push_back(arc);
    }
}

int ArcParser::ParseT(const std::string &str) const {
    const int time = std::stoi(str);
    double ticks = time / (60000.0 / m_bpm) * MG_BEAT;
    if (m_cctx.snap > 1) {
        ticks = std::round(ticks / m_cctx.snap);
    }
    return static_cast<int>(std::round(ticks));
}

int ArcParser::ParseX(const std::string &str) {
    const double x = std::stod(str);
    return static_cast<int>(std::round(x * 15.0));
}

int ArcParser::ParseY(const std::string &str) {
    const double y = std::stod(str);
    return static_cast<int>(std::round(y * 80.0));
}


void ArcParser::Parse(const std::string &str) {
    m_chains.clear();
    m_arcs.clear();
    m_handled.clear();

    ParseString(str);
    if (m_arcs.empty()) {
        throw std::runtime_error("No arcs found in the chart");
    }

    for (size_t i = 0; i < m_arcs.size(); ++i) {
        if (m_handled[i]) {
            continue;
        }
        if (auto chain = BuildChain(i); !chain.empty()) {
            m_chains.push_back(std::move(chain));
        }
    }
}

void ArcParser::ParseFile(const std::string &filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::invalid_argument("Could not open file: " + filePath);
    }

    const std::string content((std::istreambuf_iterator(file)), (std::istreambuf_iterator<char>()));
    return Parse(content);
}

bool ArcParser::FindLink(std::vector<ArcNote> &chain) {
    const ArcNote &lastArc = chain.back();
    size_t linkCount = 0;
    size_t linkIndex = m_arcs.size();

    for (size_t j = 0; j < m_arcs.size(); ++j) {
        if (m_handled[j]) {
            continue;
        }

        if (const ArcNote &candidate = m_arcs[j]; lastArc.CanLinkWith(candidate)) {
            linkCount++;
            if (linkIndex == m_arcs.size()) {
                linkIndex = j;
            }
        }
    }

    if (linkCount > 1) {
        return false;
    }

    if (linkCount == 1) {
        chain.push_back(m_arcs[linkIndex]);
        m_handled[linkIndex] = true;
        return true;
    }

    return false;
}

bool ArcParser::HasPreceding(const ArcNote &arc) {
    for (size_t j = 0; j < m_arcs.size(); ++j) {
        if (m_handled[j])
            continue;
        if (m_arcs[j].CanLinkWith(arc)) {
            return true;
        }
    }
    return false;
}

std::vector<ArcNote> ArcParser::BuildChain(const size_t startIndex) {
    if (HasPreceding(m_arcs[startIndex])) {
        return {};
    }

    std::vector<ArcNote> chain;
    chain.push_back(m_arcs[startIndex]);
    m_handled[startIndex] = true;

    bool found;
    do {
        found = FindLink(chain);
    } while (found);

    return chain;
}

void ArcParser::ParseBpm(const std::string &token) {
    if (token.rfind("timing(", 0) == 0) {
        std::string content = token.substr(7);
        content.erase(content.find(')'));

        std::stringstream ss(content);
        std::string param;
        std::vector<double> params;

        while (getline(ss, param, ',')) {
            params.push_back(std::stod(param));
        }

        if (params.size() >= 3 && params[0] == 0) {
            m_bpm = params[1];
        }
    }
}

void ArcParser::ParseString(const std::string &str) {
    std::stringstream ss(str);
    std::string token;

    while (getline(ss, token, ';')) {
        std::erase_if(token, [](const auto c) { return ::isspace(c); });
        ParseBpm(token);
    }

    ss.clear();
    ss.str(str);

    while (getline(ss, token, ';')) {
        std::erase_if(token, [](const auto c) { return ::isspace(c); });

        if (token.rfind("arc(", 0) == 0) {
            try {
                ParseSingle(token);
            } catch (const std::exception &) {
                continue;
            }
        }
    }
    m_handled.resize(m_arcs.size(), false);
}
