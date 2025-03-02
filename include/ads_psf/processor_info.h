/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef PROCESSOR_INFO_H
#define PROCESSOR_INFO_H

#include "processor_id.h"
#include <string>

namespace ads_psf {

struct ProcessorInfo {
    ProcessorInfo(const std::string& name, ProcessorId id)
    : name(name), id(id) {}

    const std::string name;
    const ProcessorId id;
};

}

#endif
