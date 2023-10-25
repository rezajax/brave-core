// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_WEBUI_RESOURCES_WEBUI_RESOURCES_H_
#define BRAVE_COMPONENTS_WEBUI_RESOURCES_WEBUI_RESOURCES_H_

#include <map>
#include <string>
#include <vector>

namespace brave {
struct WebUISimpleItem {
  const char* name;
  int id;
};

const std::map<std::string, std::vector<WebUISimpleItem>>& GetWebUIResources();

const std::map<std::string, std::vector<WebUISimpleItem>>&
GetWebUILocalizedStrings();

}  // namespace brave

#endif  // BRAVE_COMPONENTS_WEBUI_RESOURCES_WEBUI_RESOURCES_H_
