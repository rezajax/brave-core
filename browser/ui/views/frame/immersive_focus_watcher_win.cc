/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/immersive_focus_watcher_win.h"

#include <set>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/views/frame/immersive_fullscreen_controller_win.h"
#include "ui/aura/client/transient_window_client.h"
#include "ui/aura/window.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/public/activation_client.h"

namespace {

// Returns the BubbleDialogDelegateView corresponding to |maybe_bubble| if
// |maybe_bubble| is a bubble.
views::BubbleDialogDelegate* AsBubbleDialogDelegate(
    aura::Window* maybe_bubble) {
  if (!maybe_bubble) {
    return nullptr;
  }
  views::Widget* widget = views::Widget::GetWidgetForNativeView(maybe_bubble);
  if (!widget) {
    return nullptr;
  }
  return widget->widget_delegate()->AsBubbleDialogDelegate();
}

views::View* GetAnchorView(aura::Window* maybe_bubble) {
  views::BubbleDialogDelegate* bubble_dialog =
      AsBubbleDialogDelegate(maybe_bubble);
  return bubble_dialog ? bubble_dialog->GetAnchorView() : nullptr;
}

// Returns true if |maybe_transient| is a transient child of |toplevel|.
bool IsWindowTransientChildOf(aura::Window* maybe_transient,
                              aura::Window* toplevel) {
  if (!maybe_transient || !toplevel) {
    return false;
  }

  aura::client::TransientWindowClient* transient_window_client =
      aura::client::GetTransientWindowClient();
  for (aura::Window* window = maybe_transient; window;
       window = transient_window_client->GetTransientParent(window)) {
    if (window == toplevel) {
      return true;
    }
  }
  return false;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////

// Class which keeps the top-of-window views revealed as long as one of the
// bubbles it is observing is visible. The logic to keep the top-of-window
// views revealed based on the visibility of bubbles anchored to
// children of |ImmersiveFullscreenController::top_container_| is separate from
// the logic related to |ImmersiveFullscreenController::focus_revealed_lock_|
// so that bubbles which are not activatable and bubbles which do not close
// upon deactivation also keep the top-of-window views revealed for the
// duration of their visibility.
class ImmersiveFocusWatcherWin::BubbleObserver : public aura::WindowObserver {
 public:
  explicit BubbleObserver(ImmersiveFullscreenControllerWin* controller);

  BubbleObserver(const BubbleObserver&) = delete;
  BubbleObserver& operator=(const BubbleObserver&) = delete;

  ~BubbleObserver() override;

  // Start / stop observing changes to |bubble|'s visibility.
  void StartObserving(aura::Window* bubble);
  void StopObserving(aura::Window* bubble);

 private:
  // Updates |revealed_lock_| based on whether any of |bubbles_| is visible.
  void UpdateRevealedLock();

  // aura::WindowObserver overrides:
  void OnWindowVisibilityChanged(aura::Window* window, bool visible) override;
  void OnWindowDestroying(aura::Window* window) override;

  raw_ptr<ImmersiveFullscreenControllerWin> controller_;

  std::set<aura::Window*> bubbles_;

  // Lock which keeps the top-of-window views revealed based on whether any of
  // |bubbles_| is visible.
  std::unique_ptr<SimpleImmersiveRevealedLock> revealed_lock_;
};

ImmersiveFocusWatcherWin::BubbleObserver::BubbleObserver(
    ImmersiveFullscreenControllerWin* controller)
    : controller_(controller) {}

ImmersiveFocusWatcherWin::BubbleObserver::~BubbleObserver() {
  for (aura::Window* bubble : bubbles_) {
    bubble->RemoveObserver(this);
  }
}

void ImmersiveFocusWatcherWin::BubbleObserver::StartObserving(
    aura::Window* bubble) {
  if (bubbles_.insert(bubble).second) {
    bubble->AddObserver(this);
    UpdateRevealedLock();
  }
}

void ImmersiveFocusWatcherWin::BubbleObserver::StopObserving(
    aura::Window* bubble) {
  if (bubbles_.erase(bubble)) {
    bubble->RemoveObserver(this);
    UpdateRevealedLock();
  }
}

void ImmersiveFocusWatcherWin::BubbleObserver::UpdateRevealedLock() {
  bool has_visible_bubble = false;
  for (aura::Window* bubble : bubbles_) {
    if (bubble->IsVisible()) {
      has_visible_bubble = true;
      break;
    }
  }

  bool was_revealed = controller_->IsRevealed();
  if (has_visible_bubble) {
    if (!revealed_lock_.get()) {
      // Reveal the top-of-window views without animating because it looks
      // weird for the top-of-window views to animate and the bubble not to
      // animate along with the top-of-window views.
      revealed_lock_.reset(controller_->GetRevealedLock(
          ImmersiveFullscreenControllerWin::ANIMATE_REVEAL_NO));
    }
  } else {
    revealed_lock_.reset();
  }

  if (!was_revealed && revealed_lock_.get()) {
    // Currently, there is no nice way for bubbles to reposition themselves
    // whenever the anchor view moves. Tell the bubbles to reposition themselves
    // explicitly instead. The hidden bubbles are also repositioned because
    // BubbleDialogDelegateView does not reposition its widget as a result of a
    // visibility change.
    for (aura::Window* bubble : bubbles_) {
      AsBubbleDialogDelegate(bubble)->OnAnchorBoundsChanged();
    }
  }
}

void ImmersiveFocusWatcherWin::BubbleObserver::OnWindowVisibilityChanged(
    aura::Window*,
    bool visible) {
  UpdateRevealedLock();
}

void ImmersiveFocusWatcherWin::BubbleObserver::OnWindowDestroying(
    aura::Window* window) {
  StopObserving(window);
}

ImmersiveFocusWatcherWin::ImmersiveFocusWatcherWin(
    ImmersiveFullscreenControllerWin* controller)
    : immersive_fullscreen_controller_(controller) {
  GetWidget()->GetFocusManager()->AddFocusChangeListener(this);
  aura::client::GetTransientWindowClient()->AddObserver(this);
  ::wm::GetActivationClient(GetWidgetWindow()->GetRootWindow())
      ->AddObserver(this);
  RecreateBubbleObserver();
}

ImmersiveFocusWatcherWin::~ImmersiveFocusWatcherWin() {
  aura::client::GetTransientWindowClient()->RemoveObserver(this);
  GetWidget()->GetFocusManager()->RemoveFocusChangeListener(this);
  auto* activation_client =
      ::wm::GetActivationClient(GetWidgetWindow()->GetRootWindow());
  if (activation_client) {
    activation_client->RemoveObserver(this);
  }
}

void ImmersiveFocusWatcherWin::UpdateFocusRevealedLock() {
  views::Widget* widget = GetWidget();
  views::View* top_container =
      immersive_fullscreen_controller_->top_container();
  bool hold_lock = false;
  if (widget->IsActive()) {
    views::View* focused_view = widget->GetFocusManager()->GetFocusedView();
    if (top_container->Contains(focused_view)) {
      hold_lock = true;
    }
  } else {
    aura::Window* native_window = GetWidgetWindow();
    aura::Window* active_window =
        ::wm::GetActivationClient(native_window->GetRootWindow())
            ->GetActiveWindow();
    if (GetAnchorView(active_window)) {
      // BubbleObserver will already have locked the top-of-window views if the
      // bubble is anchored to a child of |top_container|. Don't acquire
      // |lock_| here for the sake of simplicity.
      // Note: Instead of checking for the existence of the |anchor_view|,
      // the existence of the |anchor_widget| is performed to avoid the case
      // where the view is already gone (and the widget is still running).
    } else {
      // The currently active window is not |native_window| and it is not a
      // bubble with an anchor view. The top-of-window views should be revealed
      // if:
      // 1) The active window is a transient child of |native_window|.
      // 2) The top-of-window views are already revealed. This restriction
      //    prevents a transient window opened by the web contents while the
      //    top-of-window views are hidden from from initiating a reveal.
      // The top-of-window views will stay revealed till |native_window| is
      // reactivated.
      if (immersive_fullscreen_controller_->IsRevealed() &&
          IsWindowTransientChildOf(active_window, native_window)) {
        hold_lock = true;
      }
    }
  }

  if (hold_lock) {
    if (!lock_.get()) {
      lock_.reset(immersive_fullscreen_controller_->GetRevealedLock(
          ImmersiveFullscreenControllerWin::ANIMATE_REVEAL_YES));
    }
  } else {
    lock_.reset();
  }
}

void ImmersiveFocusWatcherWin::ReleaseLock() {
  lock_.reset();
}

views::Widget* ImmersiveFocusWatcherWin::GetWidget() {
  return immersive_fullscreen_controller_->widget();
}

aura::Window* ImmersiveFocusWatcherWin::GetWidgetWindow() {
  return GetWidget()->GetNativeWindow();
}

void ImmersiveFocusWatcherWin::RecreateBubbleObserver() {
  bubble_observer_ =
      std::make_unique<BubbleObserver>(immersive_fullscreen_controller_);
  const std::vector<aura::Window*> transient_children =
      aura::client::GetTransientWindowClient()->GetTransientChildren(
          GetWidgetWindow());
  for (size_t i = 0; i < transient_children.size(); ++i) {
    aura::Window* transient_child = transient_children[i];
    views::View* anchor_view = GetAnchorView(transient_child);
    if (anchor_view &&
        immersive_fullscreen_controller_->top_container()->Contains(
            anchor_view)) {
      bubble_observer_->StartObserving(transient_child);
    }
  }
}

void ImmersiveFocusWatcherWin::OnWillChangeFocus(views::View* focused_before,
                                                 views::View* focused_now) {}

void ImmersiveFocusWatcherWin::OnDidChangeFocus(views::View* focused_before,
                                                views::View* focused_now) {
  UpdateFocusRevealedLock();
}

void ImmersiveFocusWatcherWin::OnWindowActivated(
    ::wm::ActivationChangeObserver::ActivationReason reason,
    aura::Window* gaining_active,
    aura::Window* losing_active) {
  UpdateFocusRevealedLock();
}

void ImmersiveFocusWatcherWin::OnTransientChildWindowAdded(
    aura::Window* window,
    aura::Window* transient) {
  views::View* anchor = GetAnchorView(transient);
  if (anchor &&
      immersive_fullscreen_controller_->top_container()->Contains(anchor)) {
    // Observe the aura::Window because the BubbleDelegateView may not be
    // parented to the widget's root view yet so |bubble_delegate->GetWidget()|
    // may still return null.
    bubble_observer_->StartObserving(transient);
  }
}

void ImmersiveFocusWatcherWin::OnTransientChildWindowRemoved(
    aura::Window* window,
    aura::Window* transient) {
  bubble_observer_->StopObserving(transient);
}
