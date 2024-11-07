/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef ACCESS_MODE_H
#define ACCESS_MODE_H

namespace ads_dtf {

enum class AccessMode {
    Read,
    Write,
    Create,
    CreateSync,
    None,
};

}

#endif
