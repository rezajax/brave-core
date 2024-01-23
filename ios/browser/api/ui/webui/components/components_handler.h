#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "base/values.h"
#include "build/build_config.h"
#include "components/component_updater/component_updater_service.h"
#include "components/update_client/update_client.h"
#include "ios/web/public/webui/web_ui_ios_message_handler.h"

// The handler for Javascript messages for the chrome://components/ page.
class ComponentsHandler : public web::WebUIIOSMessageHandler,
                          public component_updater::ServiceObserver {
 public:
  ComponentsHandler(
      component_updater::ComponentUpdateService* component_updater);
  ComponentsHandler(const ComponentsHandler&) = delete;
  ComponentsHandler& operator=(const ComponentsHandler&) = delete;
  ~ComponentsHandler() override;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

  // Callback for the "requestComponentsData" message.
  void HandleRequestComponentsData(const base::Value::List& args);

  // Callback for the "checkUpdate" message.
  void HandleCheckUpdate(const base::Value::List& args);

  // ServiceObserver implementation.
  void OnEvent(Events event, const std::string& id) override;

 private:
  static std::u16string ComponentEventToString(Events event);
  static std::u16string ServiceStatusToString(
      update_client::ComponentState state);

  base::Value::List LoadComponents();
  void OnDemandUpdate(const std::string& component_id);

  // Weak pointer; injected for testing.
  const raw_ptr<component_updater::ComponentUpdateService> component_updater_;

  base::ScopedObservation<component_updater::ComponentUpdateService,
                          component_updater::ComponentUpdateService::Observer>
      observation_{this};
};
