#define NOMINMAX
#include "Interpolator.h"

#include "Dialog.h"

#include <algorithm>
#include <vector>

void Interpolator::InterpolateChain(const std::vector<ArcNote> &chain) {
    std::vector<MP_NOTEINFO> notes;

    for (size_t i = 0; i < chain.size(); ++i) {
        MP_NOTEINFO n{};
        const auto &arc = chain[i];

        n.type = arc.trace ? MP_NOTETYPE_AIRCRUSH : MP_NOTETYPE_AIRSLIDE;
        n.x = arc.x;
        n.tick = arc.t;
        n.height = arc.y;

        if (i == 0) {
            n.longAttr = MP_NOTELONGATTR_BEGIN;
            notes.push_back(n);
        }

        n.longAttr = MP_NOTELONGATTR_CONTROL;

        const double dX = arc.toX - arc.x;
        const double dT = arc.toT - arc.t;
        const double dY = arc.toY - arc.y;

        if (dX == 0 && arc.eY != EasingMode::Linear) {
            const int sY = arc.toY >= arc.y ? 1 : -1;
            const int fromY = arc.y + sY;
            const int toY = arc.toY - sY;

            for (int y = fromY; sY > 0 ? y <= toY : y >= toY; y += sY) {
                const double pY = (y - arc.y) / dY;
                const double fPY = m_solver->InverseSolve(pY, arc.eY);
                const auto fT = static_cast<int>(std::round(arc.t + fPY * dT));

                if (!notes.empty() && notes.back().tick == fT) {
                    continue;
                }

                n.x = arc.x;
                n.height = y;
                n.tick = fT;
                notes.push_back(n);
            }
        } else if (arc.eX != EasingMode::Linear || arc.eY != EasingMode::Linear) {
            const int sX = arc.toX >= arc.x ? 1 : -1;
            const int fromX = arc.x + sX;
            const int toX = arc.toX - sX;

            for (int x = fromX; sX > 0 ? x <= toX : x >= toX; x += sX) {
                const double pX = (x - arc.x) / dX;
                const double fPX = m_solver->InverseSolve(pX, arc.eX);
                const auto fT = static_cast<int>(std::round(arc.t + fPX * dT));

                if (!notes.empty() && notes.back().tick == fT) {
                    continue;
                }

                n.x = x;
                n.tick = fT;

                if (dY != 0) {
                    const double pT = (fT - arc.t) / dT;
                    const double fPT = m_solver->Solve(pT, arc.eY);
                    const auto fY = static_cast<int>(std::round(arc.y + fPT * dY));

                    n.height = fY;
                }

                notes.push_back(n);
            }
        }

        if (i == chain.size() - 1) {
            n.longAttr = MP_NOTELONGATTR_END;
        }

        n.x = arc.toX;
        n.height = arc.toY;
        n.tick = arc.toT;
        notes.push_back(n);
    }

    m_notes.push_back(std::move(notes));
}

Interpolator::Interpolator(const ConfigContext &cctx) : m_cctx(cctx) {
    switch (m_cctx.esType) {
        using enum EasingKind;
        case Sine:
            m_solver = std::make_unique<SineSolver>();
            break;
        case Power:
            m_solver = std::make_unique<PowerSolver>(m_cctx.esExponent);
            break;
        case Circular:
            m_solver = std::make_unique<CircularSolver>(m_cctx.esEpsilon);
            break;
        default:
            m_solver = std::make_unique<PowerSolver>(1);
    }
}

void Interpolator::Interpolate(const std::vector<std::vector<ArcNote>> &arcs) {
    for (const auto &chain: arcs) {
        InterpolateChain(chain);
    }
}

void Interpolator::PostProcess(const ConfigContext &cctx, const int tOffset) {
    for (auto &notes: m_notes) {
        for (auto &note: notes) {
            note.tick *= cctx.snap;
            note.tick += tOffset;
            note.height += static_cast<int>(10.0 * cctx.yOffset);
            note.width = cctx.width;
            note.timelineId = cctx.til;
            if (cctx.clamp) {
                Clamp(note);
            }
        }
        std::ranges::sort(notes, [](const MP_NOTEINFO &a, const MP_NOTEINFO &b) { return a.tick < b.tick; });
    }
}

void Interpolator::Clamp(MP_NOTEINFO &note) {
    note.height = std::clamp(note.height, 0, 360);
    note.x = std::clamp(note.x, 0, 15);
    note.width = std::max(1, std::min(note.width, 16 - note.x));
}

void Interpolator::PutIntoChart(const MargreteComPtr<IMargretePluginChart> &p_chart) const {
    if (!p_chart) {
        throw std::runtime_error("IMargretePluginChart is null");
    }
    for (const auto &chain: m_notes) {
        PutChain(p_chart, chain);
    }
}

void Interpolator::PutChain(const MargreteComPtr<IMargretePluginChart> &p_chart,
                            const std::vector<MP_NOTEINFO> &chain) {
    const auto &airHead = chain.front();

    const auto longBegin = CreateNote(p_chart);
    longBegin->setInfo(&airHead);
    for (size_t i = 1; i < chain.size(); ++i) {
        auto airControl = CreateNote(p_chart);
        airControl->setInfo(&chain[i]);
        longBegin->appendChild(airControl.get());
    }

    if (airHead.type == MP_NOTETYPE_AIRSLIDE) {
        MP_NOTEINFO info = airHead;
        info.type = MP_NOTETYPE_TAP;
        info.longAttr = MP_NOTELONGATTR_NONE;
        info.direction = MP_NOTEDIR_NONE;

        const auto tap = CreateNote(p_chart);
        tap->setInfo(&info);

        info.type = MP_NOTETYPE_AIR;
        info.direction = MP_NOTEDIR_UP;

        const auto air = CreateNote(p_chart);
        air->setInfo(&info);

        air->appendChild(longBegin.get());
        tap->appendChild(air.get());

        p_chart->appendNote(tap.get());
    } else {
        p_chart->appendNote(longBegin.get());
    }
}

inline MargreteComPtr<IMargretePluginNote> Interpolator::CreateNote(
        const MargreteComPtr<IMargretePluginChart> &p_chart) {
    MargreteComPtr<IMargretePluginNote> note;
    if (!p_chart->createNote(note.put())) {
        throw std::runtime_error("Failed to create IMargretePluginNote from IMargretePluginChart");
    }
    return note;
}
