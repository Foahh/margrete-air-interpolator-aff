#pragma once
#include <MargretePlugin.h>
#include <memory>
#include <vector>

#include "ArcNote.h"
#include "Common.hpp"

class Interpolator {
public:
    explicit Interpolator(const ConfigContext &cctx);

    void Interpolate(const std::vector<std::vector<ArcNote>> &arcs);
    void PostProcess(const ConfigContext &cctx, int tOffset);
    void PutIntoChart(const MargreteComPtr<IMargretePluginChart> &p_chart) const;

    std::vector<std::vector<MP_NOTEINFO>> m_notes;

private:
    ConfigContext m_cctx;
    std::unique_ptr<EasingSolver> m_solver;

    static void Clamp(MP_NOTEINFO &note);
    void InterpolateChain(const std::vector<ArcNote> &chain);
    static void PutChain(const MargreteComPtr<IMargretePluginChart> &p_chart, const std::vector<MP_NOTEINFO> &chain);
    static MargreteComPtr<IMargretePluginNote> CreateNote(const MargreteComPtr<IMargretePluginChart> &p_chart);
};
