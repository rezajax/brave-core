// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_WEBUI_WEBUI_RESOURCES_H_
#define BRAVE_COMPONENTS_WEBUI_WEBUI_RESOURCES_H_

#include <string_view>
#include <vector>
#include "base/containers/flat_map.h"

namespace brave {
struct WebUISimpleItem {
  const std::string_view name;
  const int id;
};

const base::flat_map<std::string_view, std::vector<WebUISimpleItem>>&
GetWebUIResources();

const base::flat_map<std::string_view, std::vector<WebUISimpleItem>>&
GetWebUILocalizedStrings();

}  // namespace brave

#endif  // BRAVE_COMPONENTS_WEBUI_WEBUI_RESOURCES_H_
