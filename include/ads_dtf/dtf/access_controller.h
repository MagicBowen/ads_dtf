/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef ACCESS_CONTROLLER_H
#define ACCESS_CONTROLLER_H

#include "ads_dtf/dtf/access_mode.h"
#include "ads_dtf/dtf/data_type.h"
#include "ads_dtf/dtf/life_span.h"
#include "ads_dtf/dtf/user.h"
#include <unordered_map>

namespace ads_dtf {

struct AccessController {
    bool Register(UserId user, DataType data, LifeSpan span, AccessMode mode) {
        auto dataKey = LifeDataType(data, span);
        auto& accessorInfo = accessors_[user];
        auto result = accessorInfo.find(dataKey);
        if (result != accessorInfo.end()) {
            return false;
        }
        accessorInfo[dataKey] = mode;
        return true;
    }

    AccessMode GetAccessMode(UserId user, DataType data, LifeSpan span) const {
        auto accessor = accessors_.find(user);
        if (accessor == accessors_.end()) {
            return AccessMode::None;
        }
        auto dataAccessIter = accessor->second.find(LifeDataType(data, span));
        if (dataAccessIter == accessor->second.end()) {
            return AccessMode::None;
        }
        return dataAccessIter->second;
    }

private:
    struct LifeDataType {
        LifeDataType(DataType dtype, LifeSpan span) 
        : dtype_(dtype), span_(span) {}

        bool operator==(const LifeDataType& other) const {
            return dtype_ == other.dtype_ && span_ == other.span_;
        }

        std::size_t Hash() const {
            return std::hash<DataType>()(dtype_) ^ std::hash<int>()(enum_id_cast(span_));
        }
    private:
        DataType dtype_;
        LifeSpan span_;
    };

    struct LifeDataTypeHash {
        std::size_t operator()(const LifeDataType& key) const {
            return key.Hash();
        }
    };

private:
    using DataAccessMap = std::unordered_map<LifeDataType, AccessMode, LifeDataTypeHash>;
    std::unordered_map<UserId, DataAccessMap> accessors_;
};

}

#endif
