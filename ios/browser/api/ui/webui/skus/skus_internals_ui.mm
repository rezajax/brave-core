// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/ui/webui/skus/skus_internals_ui.h"
//#include "brave/ios/browser/ui/webui/brave_webui_source.h"

#include <memory>
#include <string_view>
#include <utility>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/notreached.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/skus/browser/pref_names.h"
#include "brave/components/skus/browser/resources/grit/skus_internals_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_service.h"
#include "ui/base/clipboard/clipboard_ios.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service_helper.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_os_connection_api.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"

#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/web/public/webui/url_data_source_ios.h"
#import "ios/web/public/webui/web_ui_ios.h"
#import "ios/web/public/webui/web_ui_ios_data_source.h"
#import "ios/web/public/webui/web_ui_ios_message_handler.h"
#import "ios/web/public/web_state.h"
#include "ui/base/webui/web_ui_util.h"
#include "ui/base/webui/resource_path.h"
#include "ios/chrome/grit/ios_resources.h"
#include "ios/chrome/grit/ios_strings.h"

//namespace {
//
//void SaveSkusStateToFile(const base::FilePath& path,
//                         std::string_view skus_state) {
//  base::WriteFile(path, skus_state);
//}
//
//}  // namespace

namespace {

web::WebUIIOSDataSource* CreateAndAddWebUIDataSource(
    web::WebUIIOS* web_ui,
    const std::string& name,
    const webui::ResourcePath* resource_map,
    std::size_t resource_map_size,
    int html_resource_id) {
  
  web::WebUIIOSDataSource* source = web::WebUIIOSDataSource::Create(name);
  web::WebUIIOSDataSource::Add(ChromeBrowserState::FromWebUIIOS(web_ui), source);
  source->UseStringsJs();

  // Add required resources.
  source->AddResourcePaths(base::make_span(resource_map, resource_map_size));
  source->SetDefaultResource(html_resource_id);
  return source;
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
//
// SkuesInternalsUI
//
///////////////////////////////////////////////////////////////////////////////

SkusInternalsUI::SkusInternalsUI(web::WebUIIOS* web_ui, const GURL& url) : web::WebUIIOSController(web_ui, url.host()),
      local_state_(GetApplicationContext()->GetLocalState()) {

  // Set up the brave://skus-internals/ source.
  CreateAndAddWebUIDataSource(web_ui, url.host(), kSkusInternalsGenerated, kSkusInternalsGeneratedSize, IDR_SKUS_INTERNALS_HTML);

  // Bind Mojom Interface
  web_ui->GetWebState()->GetInterfaceBinderForMainFrame()->AddInterface(
          base::BindRepeating(&SkusInternalsUI::BindInterface,
                              base::Unretained(this)));
}

SkusInternalsUI::~SkusInternalsUI() {}

void SkusInternalsUI::BindInterface(
    mojo::PendingReceiver<skus::mojom::SkusInternals> pending_receiver) {
  if (skus_internals_receiver_.is_bound()) {
    skus_internals_receiver_.reset();
  }

  skus_internals_receiver_.Bind(std::move(pending_receiver));
}

void SkusInternalsUI::GetEventLog(GetEventLogCallback callback) {
  // TODO(simonhong): Ask log to SkusService
  NOTIMPLEMENTED_LOG_ONCE();
}

void SkusInternalsUI::GetSkusState(GetSkusStateCallback callback) {
  std::move(callback).Run(GetSkusStateAsString());
}

void SkusInternalsUI::GetVpnState(GetVpnStateCallback callback) {
  base::Value::Dict dict;
#if BUILDFLAG(ENABLE_BRAVE_VPN)
#if !BUILDFLAG(IS_ANDROID)
  dict.Set("Last connection error", GetLastVPNConnectionError());
#endif
  dict.Set("Order", GetVPNOrderInfo());
#endif
  std::string result;
  base::JSONWriter::Write(dict, &result);
  std::move(callback).Run(result);
}

base::Value::Dict SkusInternalsUI::GetVPNOrderInfo() const {
  base::Value::Dict dict;
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  auto* browser_state = ChromeBrowserState::FromWebUIIOS(web_ui());
  if (!brave_vpn::IsBraveVPNEnabled(browser_state->GetPrefs())) {
    return dict;
  }

  dict.Set("env",
           local_state_->GetString(brave_vpn::prefs::kBraveVPNEnvironment));

  const auto& skus_state = local_state_->GetDict(skus::prefs::kSkusState);
  for (const auto kv : skus_state) {
    if (!base::StartsWith(kv.first, "skus:")) {
      continue;
    }

    // Convert to Value as it's stored as string in local state.
    auto json_value = base::JSONReader::Read(kv.second.GetString());
    if (!json_value) {
      continue;
    }

    const auto* skus = json_value->GetIfDict();
    if (!skus) {
      continue;
    }

    const auto* orders = skus->FindDict("orders");
    if (!orders) {
      continue;
    }
    
    base::Value::Dict order_dict_output;
    for (const auto order : *orders) {
      const auto* order_dict = order.second.GetIfDict();
      if (!order_dict) {
        continue;
      }

      if (auto* location = order_dict->FindString("location")) {
        if (!base::StartsWith(*location, "vpn.")) {
          continue;
        }
        order_dict_output.Set("location", *location);
      }

      if (auto* id = order_dict->FindString("id")) {
        order_dict_output.Set("id", *id);
      }
      if (auto* expires_at = order_dict->FindString("expires_at")) {
        order_dict_output.Set("expires_at", *expires_at);
      }
    }
    
    // Set output with env like {skus:production: {...}}.
    dict.Set(kv.first, std::move(order_dict_output));
  }
#endif
  return dict;
}

void SkusInternalsUI::ResetSkusState() {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  // VPN service caches credentials. It should be cleared also
  // when skus state is reset. Otherwise, vpn service is still
  // in purchased state.
  auto* browser_state = ChromeBrowserState::FromWebUIIOS(web_ui());
  if (brave_vpn::IsBraveVPNEnabled(browser_state->GetPrefs())) {
    brave_vpn::ClearSubscriberCredential(local_state_);
    if (auto* service =
            brave_vpn::BraveVpnServiceFactory::GetForBrowserState(browser_state)) {
      service->ReloadPurchasedState();
    }
  }
#endif

  local_state_->ClearPref(skus::prefs::kSkusState);
}

void SkusInternalsUI::CopySkusStateToClipboard() {
//  ui::ScopedClipboardWriter(ui::ClipboardBuffer::kCopyPaste)
//      .WriteText(base::UTF8ToUTF16(GetSkusStateAsString()));
}

void SkusInternalsUI::DownloadSkusState() {
  
}

//void SkusInternalsUI::FileSelected(const base::FilePath& path,
//                                   int index,
//                                   void* params) {
//  // TODO:
//  base::FilePath path;
//  
//  base::ThreadPool::PostTask(
//      FROM_HERE, {base::MayBlock()},
//      base::BindOnce(&SaveSkusStateToFile, path, GetSkusStateAsString()));
//}
//
//void SkusInternalsUI::FileSelectionCanceled(void* params) {
//  
//}

std::string SkusInternalsUI::GetLastVPNConnectionError() const {
  std::string error;
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  auto* api = GetApplicationContext()->brave_vpn_os_connection_api();
  CHECK(api);
  error = api->GetLastConnectionError();
#endif
  return error;
}

std::string SkusInternalsUI::GetSkusStateAsString() const {
  const auto& skus_state = local_state_->GetDict(skus::prefs::kSkusState);
  base::Value::Dict dict;

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  auto* browser_state = ChromeBrowserState::FromWebUIIOS(web_ui());
  if (brave_vpn::IsBraveVPNEnabled(browser_state->GetPrefs())) {
    dict.Set("env",
             local_state_->GetString(brave_vpn::prefs::kBraveVPNEnvironment));
  }
#endif

  for (const auto kv : skus_state) {
    // Only shows "skus:xx" kv in webui.
    if (!base::StartsWith(kv.first, "skus:")) {
      continue;
    }

    if (auto value = base::JSONReader::Read(kv.second.GetString()); value) {
      dict.Set(kv.first, std::move(*value));
    }
  }

  std::string result;
  base::JSONWriter::Write(dict, &result);
  return result;
}
