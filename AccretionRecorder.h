#ifndef ACCRETION_RECORDER_H
#define ACCRETION_RECORDER_H

#include <utility>
#include <vector>

#include "structs.h"  // dust, planet

/**
 * @brief Passive recorder for the accretion process, used by the viewer's
 * "formation" visualization. It is OPT-IN: accrete only captures frames when a
 * recorder pointer is set (default null), so the normal generation path
 * (stargen, the harness, goldens) is byte-identical and determinism is unchanged.
 * It only READS state and copies it -- no RNG, no mutation of the accretion.
 */
struct AccretionBand {
    double inner;   // AU
    double outer;   // AU
    bool hasDust;
    bool hasGas;
};

struct AccretionBody {
    double a;     // semi-major axis, AU
    double e;     // eccentricity
    double mass;  // solar masses
};

struct AccretionFrame {
    std::vector<AccretionBand> bands;
    std::vector<AccretionBody> bodies;
};

class AccretionRecorder {
public:
    std::vector<AccretionFrame> frames;

    /** Append a snapshot of the current dust lanes and planetesimal list. */
    void capture(const std::vector<dust>& dust_bands, planet* head) {
        AccretionFrame f;
        f.bands.reserve(dust_bands.size());
        for (const auto& b : dust_bands) {
            // peekInnerEdge() (not getInnerEdge()) -- the latter mutates the band
            // when inner>=outer, which would perturb the deterministic accretion.
            double inner = static_cast<double>(b.peekInnerEdge());
            double outer = static_cast<double>(b.getOuterEdge());
            if (inner >= outer) {
                continue;  // degenerate/empty band; nothing to draw
            }
            f.bands.push_back({inner, outer, b.getDustPresent(), b.getGasPresent()});
        }
        for (planet* p = head; p != nullptr; p = p->next_planet) {
            f.bodies.push_back({static_cast<double>(p->getA()), static_cast<double>(p->getE()),
                                static_cast<double>(p->getMass())});
        }
        frames.push_back(std::move(f));
    }
};

#endif  // ACCRETION_RECORDER_H
