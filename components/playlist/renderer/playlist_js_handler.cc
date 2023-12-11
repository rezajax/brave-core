/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/renderer/playlist_js_handler.h"

#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "base/json/values_util.h"
#include "base/logging.h"
#include "content/public/renderer/render_frame.h"
#include "gin/converter.h"
#include "gin/function_template.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

namespace {

template <typename Signature>
void BindFunctionToObject(v8::Isolate* isolate,
                          v8::Local<v8::Object> javascript_object,
                          const std::string& function_name,
                          const base::RepeatingCallback<Signature>& callback) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  // Get the isolate associated with this object.
  javascript_object
      ->Set(context, gin::StringToSymbol(isolate, function_name),
            gin::CreateFunctionTemplate(isolate, callback)
                ->GetFunction(context)
                .ToLocalChecked())
      .Check();
}

}  // namespace

namespace playlist {

PlaylistJSHandler::PlaylistJSHandler(content::RenderFrame* render_frame,
                                     const int32_t isolated_world_id)
    : render_frame_(render_frame), isolated_world_id_(isolated_world_id) {
  EnsureConnectedToMediaHandler();
}

PlaylistJSHandler::~PlaylistJSHandler() {}

void PlaylistJSHandler::AddWorkerObjectToFrame(v8::Local<v8::Context> context) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  if (context.IsEmpty()) {
    return;
  }

  CreateWorkerObject(isolate, context);
}

void PlaylistJSHandler::SetDetectorScript(const blink::WebString& script) {
  script_ = script;
}

bool PlaylistJSHandler::EnsureConnectedToMediaHandler() {
  if (!media_handler_.is_bound()) {
    render_frame_->GetBrowserInterfaceBroker()->GetInterface(
        media_handler_.BindNewPipeAndPassReceiver());
    media_handler_.set_disconnect_handler(
        base::BindOnce(&PlaylistJSHandler::OnMediaHandlerDisconnect,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  return media_handler_.is_bound();
}

void PlaylistJSHandler::OnMediaHandlerDisconnect() {
  media_handler_.reset();
  EnsureConnectedToMediaHandler();
}

void PlaylistJSHandler::CreateWorkerObject(v8::Isolate* isolate,
                                           v8::Local<v8::Context> context) {
  DVLOG(2) << __FUNCTION__;
  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Value> pl_worker;
  if (!global->Get(context, gin::StringToV8(isolate, "pl_worker"))
           .ToLocal(&pl_worker) ||
      !pl_worker->IsObject()) {
    v8::Local<v8::Object> pl_worker_object;
    pl_worker_object = v8::Object::New(isolate);
    global
        ->Set(context, gin::StringToSymbol(isolate, "pl_worker"),
              pl_worker_object)
        .Check();
    BindFunctionsToWorkerObject(isolate, pl_worker_object);
  }
}

void PlaylistJSHandler::BindFunctionsToWorkerObject(
    v8::Isolate* isolate,
    v8::Local<v8::Object> worker_object) {
  DVLOG(2) << __FUNCTION__;
  BindFunctionToObject(isolate, worker_object, "onMediaUpdated",
                       base::BindRepeating(&PlaylistJSHandler::OnMediaUpdated,
                                           weak_ptr_factory_.GetWeakPtr()));
}

void PlaylistJSHandler::OnMediaUpdated(const std::string& src) {
  if (!GURL(src).SchemeIsHTTPOrHTTPS()) {
    return;
  }

  DVLOG(2) << __FUNCTION__ << " " << src;

  if (!EnsureConnectedToMediaHandler()) {
    return;
  }

  auto* web_frame = render_frame_->GetWebFrame();
  std::vector sources = {blink::WebScriptSource(script_)};
  web_frame->RequestExecuteScript(
      allow_to_run_script_on_main_world_ ? 0 : isolated_world_id_, sources,
      blink::mojom::UserActivationOption::kActivate,
      blink::mojom::EvaluationTiming::kAsynchronous,
      blink::mojom::LoadEventBlockingOption::kBlock,
      base::BindOnce(&PlaylistJSHandler::OnFindMedia,
                     weak_ptr_factory_.GetWeakPtr(), GURL(src)),
      blink::BackForwardCacheAware::kAllow,
      blink::mojom::WantResultOption::kWantResult,
      blink::mojom::PromiseResultOption::kAwait);
}

void PlaylistJSHandler::OnFindMedia(GURL requested_url,
                                    absl::optional<base::Value> value,
                                    base::TimeTicks time_ticks) {
  if (!value) {
    LOG(ERROR) << *value;
    return;
  }

  media_handler_->OnMediaUpdatedFromRenderFrame(requested_url,
                                                std::move(*value));
}

}  // namespace playlist
