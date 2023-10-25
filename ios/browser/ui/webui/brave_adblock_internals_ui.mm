// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/brave_adblock_internals_ui.h"

#import <mach/mach.h>
#include <memory>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/process/process.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_adblock/adblock_internals/resources/grit/brave_adblock_internals_generated_map.h"
#include "components/grit/brave_components_resources.h"
#import "ios/web/public/webui/web_ui_ios.h"
#import "ios/web/public/webui/web_ui_ios_controller.h"
#import "ios/web/public/webui/web_ui_ios_data_source.h"
#import "ios/web/public/webui/web_ui_ios_message_handler.h"

#include "base/values.h"
#include "brave/ios/browser/ui/webui/brave_webui_source.h"

namespace {

// See chrome/browser/metrics/process_memory_metrics_emitter.cc.
struct MemoryMetric {
  // The root dump name that represents the required metric.
  const char* const dump_name;
  // The type of metric that is measured, usually size in bytes or object count.
  const char* const metric;
} const kCollectedMemoryMetrics[] = {
    {"malloc", "size"},
};

// Class acting as a controller of the brave://adblock-internals WebUI.
class BraveAdblockInternalsMessageHandler : public web::WebUIIOSMessageHandler {
 public:
  BraveAdblockInternalsMessageHandler() = default;
  ~BraveAdblockInternalsMessageHandler() override = default;

 private:
  // WebUIMessageHandler implementation.
  void RegisterMessages() override {
    web_ui()->RegisterMessageCallback(
        "brave_adblock_internals.getDebugInfo",
        base::BindRepeating(&BraveAdblockInternalsMessageHandler::GetDebugInfo,
                            base::Unretained(this)));
    web_ui()->RegisterMessageCallback(
        "brave_adblock_internals.discardRegex",
        base::BindRepeating(&BraveAdblockInternalsMessageHandler::DiscardRegex,
                            base::Unretained(this)));
  }

  void GetDebugInfo(const base::Value::List& args) {
    CHECK_EQ(1U, args.size());
    const auto& callback_id = args[0].GetString();

    struct task_basic_info info = {0};
    mach_msg_type_number_t size = TASK_BASIC_INFO_COUNT;
    kern_return_t kerr =
        task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &size);
    if (kerr != KERN_SUCCESS) {
      web_ui()->RejectJavascriptCallback(base::Value(callback_id),
                                         base::Value("failed to get dump"));
      return;
    }

    base::Value::Dict mem_info;
    mem_info.Set(std::string(kCollectedMemoryMetrics[0].dump_name) + "/" +
                     kCollectedMemoryMetrics[0].metric + "_kb",
                 base::NumberToString(info.resident_size / 1024));

    mem_info.Set("private_footprint_kb",
                 static_cast<int>(info.resident_size / 1024));

    base::Value::List regex_list;
    base::Value::Dict regex_info;
    regex_info.Set("id", base::NumberToString(1024));
    regex_info.Set("regex", std::string("Test Regex"));
    regex_info.Set("unused_sec", static_cast<int>(2048));
    regex_info.Set("usage_count", static_cast<int>(4096));
    regex_list.Append(std::move(regex_info));

    base::Value::Dict default_engine_info;
    default_engine_info.Set("compiled_regex_count", static_cast<int>(256));
    default_engine_info.Set("regex_data", std::move(regex_list));

    base::Value::List regex_list2;

    base::Value::Dict additional_engine_info;
    additional_engine_info.Set("compiled_regex_count", static_cast<int>(512));
    additional_engine_info.Set("regex_data", std::move(regex_list2));

    base::Value::Dict res;
    res.Set("default_engine", std::move(default_engine_info));
    res.Set("additional_engine", std::move(additional_engine_info));
    res.Set("memory", std::move(mem_info));
    web_ui()->ResolveJavascriptCallback(base::Value(callback_id), res);
  }

  void DiscardRegex(const base::Value::List& args) {
    CHECK_EQ(1U, args.size());
    uint64_t regex_id = 0U;
    if (!base::StringToUint64(args[0].GetString(), &regex_id)) {
      return;
    }
    // GetApplicationContext()->ad_block_service()->DiscardRegex(regex_id);
  }

  base::WeakPtrFactory<BraveAdblockInternalsMessageHandler> weak_ptr_factory_{
      this};
};

}  // namespace

BraveAdblockInternalsUI::BraveAdblockInternalsUI(web::WebUIIOS* web_ui,
                                                 const std::string& host)
    : web::WebUIIOSController(web_ui, host) {
  brave::CreateAndAddWebUIDataSource(
      web_ui, host, kBraveAdblockInternalsGenerated,
      kBraveAdblockInternalsGeneratedSize, IDR_BRAVE_ADBLOCK_INTERNALS_HTML);

  web_ui->AddMessageHandler(
      std::make_unique<BraveAdblockInternalsMessageHandler>());
}

BraveAdblockInternalsUI::~BraveAdblockInternalsUI() = default;
