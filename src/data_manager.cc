#include "ads_dtf/dtf/data_manager.h"
#include "ads_dtf/dtf/data_context.h"

namespace ads_dtf {

void DataManager::ResetRepo(LifeSpan span) {
    if (span >= LifeSpan::Max) return;

    DataRepo& repo = repos_[enum_id_cast(span)];
    for (auto& pair : repo) {
        auto& dtype = pair.first;
        std::unique_ptr<DataObjectBase>& dataObjPtr = pair.second;
        dataObjPtr->Clear();
    }
}

} // namespace ads_dtf