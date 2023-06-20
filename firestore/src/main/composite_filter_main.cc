/*
 * Copyright 2023 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if defined(__ANDROID__)
#error "This header should not be used on Android."
#endif

#include <utility>
#include <vector>

#include "Firestore/core/src/core/composite_filter.h"
#include "firestore/src/main/composite_filter_main.h"
#include "firestore/src/main/converter_main.h"
#include "absl/algorithm/container.h"

namespace firebase {
namespace firestore {

CompositeFilterInternal::CompositeFilterInternal(
    core::CompositeFilter::Operator op, std::vector<FilterInternal*>& filters)
    : FilterInternal(FilterType::Composite), op_(op) {
  for (FilterInternal* filter_internal : filters) {
    filters_.emplace_back(std::shared_ptr<FilterInternal>(filter_internal));
  }
}

CompositeFilterInternal* CompositeFilterInternal::clone() {
  return new CompositeFilterInternal(*this);
}

bool CompositeFilterInternal::IsEmpty() const { return filters_.empty(); }

core::Filter CompositeFilterInternal::ToCoreFilter(
    const api::Query& query,
    const firebase::firestore::UserDataConverter& user_data_converter) const {
  std::vector<core::Filter> core_filters;
  for (auto& filter : filters_) {
    core_filters.push_back(filter->ToCoreFilter(query, user_data_converter));
  }
  return core::CompositeFilter::Create(std::move(core_filters), op_);
}

bool operator==(const CompositeFilterInternal& lhs,
                const CompositeFilterInternal& rhs) {
  if (lhs.op_ != rhs.op_) return false;
  const std::vector<std::shared_ptr<FilterInternal>>& lhs_filters = lhs.filters_;
  const std::vector<std::shared_ptr<FilterInternal>>& rhs_filters = rhs.filters_;
  const unsigned long size = lhs_filters.size();
  if (size != rhs_filters.size()) return false;
  for (int i = 0; i < size; i++) {
    if (lhs_filters[i] != rhs_filters[i] && *lhs_filters[i] != *rhs_filters[i]) return false;
  }
  return true;
}

}  // namespace firestore
}  // namespace firebase
