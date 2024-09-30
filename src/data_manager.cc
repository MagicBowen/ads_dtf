#include "ads_dtf/dtf/data_manager.h"
#include "ads_dtf/dtf/data_context.h"

namespace ads_dtf {

DataContext DataManager::GetContext() {
    return DataContext(*this);
}
    
} // namespace ads_dtf