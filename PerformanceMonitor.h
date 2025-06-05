#pragma once

#include <chrono>
#include <string>

class PerformanceMonitor {
private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    
    bool enabled = false;
    TimePoint lastFileTime;
    std::string lastFileName;

public:
    void setEnabled(bool enable);
    void recordFileOperation(const std::string& fileName);
    bool isEnabled() const;
};

// Global instance declaration
extern PerformanceMonitor performanceMonitor;
