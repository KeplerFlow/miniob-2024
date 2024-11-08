#include "sql/operator/update_logical_operator.h"

UpdateLogicalOperator::UpdateLogicalOperator(Table *table, std::vector<const FieldMeta *>field_meta, std::map<int,Value> value,std::map<int,int>select_map)
        :table_(table), field_meta_(field_meta), value_map_(value),select_map_(select_map)
{}