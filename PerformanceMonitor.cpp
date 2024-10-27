#include "PerformanceMonitor.h"
#include <iostream>

// Global instance definition
PerformanceMonitor performanceMonitor;

void PerformanceMonitor::setEnabled(bool enable) {
    enabled = enable;
    if (enable) {
        lastFileTime = Clock::now();
    }
}

void PerformanceMonitor::recordFileOperation(const std::string& fileName) {
    if (!enabled) return;

    auto currentTime = Clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        currentTime - lastFileTime);

    if (!lastFileName.empty()) {
        std::cout << "Time between " << lastFileName << " and " << fileName 
                 << ": " << duration.count() << "ms\n";
    }

    lastFileName = fileName;
    lastFileTime = currentTime;
}

bool PerformanceMonitor::isEnabled() const {
    return enabled;
}