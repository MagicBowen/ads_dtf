/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef PROCESSOR_ID_H
#define PROCESSOR_ID_H

#include <cstdint>
#include <string>
#include <sstream>

namespace ads_psf {

struct ProcessorId {
    static constexpr uint32_t MAX_DEPTH = 8;       // 最多支持8层嵌套
    static constexpr uint32_t BITS_PER_LEVEL = 8;  // 每层使用8位，每层最多支持255个处理器
    static constexpr uint64_t LEVEL_MASK = 0xFF;   // 每层的掩码
    
    explicit ProcessorId(uint64_t value) : value_(value) {}
    
    static ProcessorId CreateChild(const ProcessorId& parent, uint32_t childIndex) {
        uint64_t parentValue = parent.value_;
        uint32_t depth = GetDepthFromValue(parentValue);
        
        if (depth >= MAX_DEPTH) {
            return parent;
        }
        
        uint32_t newDepth = depth + 1;
        uint64_t newValue = (parentValue << BITS_PER_LEVEL) | (childIndex & LEVEL_MASK);
        newValue = (newValue & ~(LEVEL_MASK << 56)) | (static_cast<uint64_t>(newDepth) << 56);
        return ProcessorId(newValue);
    }
    
    static ProcessorId Root() {
        return ProcessorId((static_cast<uint64_t>(1) << 56) | 1);
    }
    
    ProcessorId GetParent() const {
        uint32_t depth = GetDepth();
        if (depth <= 1) {
            return ProcessorId{0};
        }
        
        uint64_t parentValue = value_ >> BITS_PER_LEVEL;
        parentValue = (parentValue & ~(LEVEL_MASK << 56)) | (static_cast<uint64_t>(depth - 1) << 56);
        
        return ProcessorId(parentValue);
    }
    
    uint32_t GetDepth() const {
        return GetDepthFromValue(value_);
    }
    
    uint64_t GetValue() const {
        return value_;
    }
    
    uint8_t GetLevelValue(uint32_t level) const {
        if (level >= GetDepth()) {
            return 0;
        }
        
        uint32_t shift = level * BITS_PER_LEVEL;
        return (value_ >> shift) & LEVEL_MASK;
    }
    
    std::string ToString() const {
        uint32_t depth = GetDepth();
        if (depth == 0) return "null";
        
        std::ostringstream oss;
        for (int i = depth - 1; i >= 0; --i) {
            if (i < depth - 1) {
                oss << ".";
            }
            oss << static_cast<int>(GetLevelValue(i));
        }
        return oss.str();
    }
    
    bool operator==(const ProcessorId& other) const {
        return value_ == other.value_;
    }
    
    bool operator!=(const ProcessorId& other) const {
        return value_ != other.value_;
    }
    
private:
    static uint32_t GetDepthFromValue(uint64_t value) {
        return (value >> 56) & LEVEL_MASK;
    }
    
private:
    uint64_t value_;
};

} // namespace ads_psf

namespace std {
    template<>
    struct hash<ads_psf::ProcessorId> {
        size_t operator()(const ads_psf::ProcessorId& id) const {
            return std::hash<uint64_t>{}(id.GetValue());
        }
    };
}

#endif
