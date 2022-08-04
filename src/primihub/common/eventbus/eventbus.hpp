// ref: https://github.com/DeveloperPaul123/eventbus

#pragma once

#include <any>
#include <atomic>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <typeindex>
#include <unordered_map>
#include <utility>

#include "src/primihub/common/eventbus/function_traits.hpp"

namespace primihub::common {
class event_bus;

/**
 * @brief A registration handle for a particular handler of an event type.
 * @details This class is move constructible only. It also assumed that the
 * lifespan of this object will be as long or shorter than that of the event
 * bus. This class is move constructible for that reason, but there are still
 * some cases where you can run into life time issues.
 */
class handler_registration {
    const void *handle_{nullptr};
    primihub::common::event_bus *event_bus_{nullptr};

  public:
    handler_registration(const handler_registration &other) = delete;
    handler_registration(handler_registration &&other) noexcept;
    handler_registration &operator=(const handler_registration &other) = delete;
    handler_registration &operator=(handler_registration &&other) noexcept;
    ~handler_registration();

    /**
     * @brief Pointer to the underlying handle.
     */
    [[nodiscard]] const void *handle() const;

    /**
     * @brief Unregister this handler from the event bus.
     */
    void unregister() noexcept;

  protected:
    handler_registration(const void *handle, primihub::common::event_bus *bus);
    friend class event_bus;
};

/**
 * @brief A central event handler class that connects event handlers with the
 * events.
 */
class event_bus {
  public:
    event_bus() = default;

    /**
     * @brief Register an event handler for a given event type.
     * @tparam EventType The event type
     * @tparam EventHandler The invocable event handler type.
     * @param handler A callable handler of the event type. Can accept the event
     * as param or take no params.
     * @return A handler_registration instance for the given handler.
     */
    template <typename EventType, typename EventHandler,
              typename = std::enable_if_t<
                  std::is_invocable_v<EventHandler> ||
                  std::is_invocable_v<EventHandler, EventType>>>
    [[nodiscard]] handler_registration
    register_handler(EventHandler &&handler) {
        using traits = primihub::common::function_traits<EventHandler>;
        const auto type_idx = std::type_index(typeid(EventType));
        const void *handle;
        // check if the function takes any arguments.
        if constexpr (traits::arity == 0) {
            safe_unique_registrations_access([&]() {
                auto it = handler_registrations_.emplace(
                    type_idx, [handler = std::forward<EventHandler>(handler)](
                                  auto) { handler(); });

                handle = static_cast<const void *>(&(it->second));
            });
        } else {
            safe_unique_registrations_access([&]() {
                auto it = handler_registrations_.emplace(
                    type_idx,
                    [func = std::forward<EventHandler>(handler)](auto value) {
                        func(std::any_cast<EventType>(value));
                    });

                handle = static_cast<const void *>(&(it->second));
            });
        }
        return {handle, this};
    }

    /**
     * @brief Register an event handler for a given event type.
     * @tparam EventType The event type
     * @tparam ClassType Event handler class
     * @tparam MemberFunction Event handler member function
     * @param class_instance Instance of ClassType that will handle the event.
     * @param function Pointer to the MemberFunction of the ClassType.
     * @return A handler_registration instance for the given handler.
     */
    template <typename EventType, typename ClassType, typename MemberFunction>
    [[nodiscard]] handler_registration
    register_handler(ClassType *class_instance,
                     MemberFunction &&function) noexcept {
        using traits = primihub::common::function_traits<MemberFunction>;
        static_assert(std::is_same_v<ClassType,
                                     std::decay_t<typename traits::owner_type>>,
                      "Member function pointer must match instance type.");

        const auto type_idx = std::type_index(typeid(EventType));
        const void *handle;

        if constexpr (traits::arity == 0) {
            safe_unique_registrations_access([&]() {
                auto it = handler_registrations_.emplace(
                    type_idx, [class_instance, function](auto) {
                        (class_instance->*function)();
                    });

                handle = static_cast<const void *>(&(it->second));
            });
        } else {
            safe_unique_registrations_access([&]() {
                auto it = handler_registrations_.emplace(
                    type_idx, [class_instance, function](auto value) {
                        (class_instance->*function)(
                            std::any_cast<EventType>(value));
                    });

                handle = static_cast<const void *>(&(it->second));
            });
        }
        return {handle, this};
    }

    /**
     * @brief Fire an event to notify event handlers.
     * @tparam EventType The event type
     * @param evt The event to pass to all event handlers.
     */
    template <typename EventType,
              typename = std::enable_if_t<!std::is_pointer_v<EventType>>>
    void fire_event(EventType &&evt) noexcept {
        safe_shared_registrations_access(
            [this, local_event = std::forward<EventType>(evt)]() {
                // only call the functions we need to
                auto [begin_evt_id, end_evt_id] =
                    handler_registrations_.equal_range(
                        std::type_index(typeid(EventType)));
                for (; begin_evt_id != end_evt_id; ++begin_evt_id) {
                    begin_evt_id->second(local_event);
                }
            });
    }

    /**
     * @brief Remove a given handler from the event bus.
     * @param registration The registration object returned by register_handler.
     * @return true is handler removal was successful, false otherwise.
     */
    bool remove_handler(const handler_registration &registration) noexcept {
        if (!registration.handle()) {
            return false;
        }

        auto result = false;
        safe_unique_registrations_access([this, &result, &registration]() {
            for (auto it = handler_registrations_.begin();
                 it != handler_registrations_.end(); ++it) {
                if (static_cast<const void *>(&(it->second)) ==
                    registration.handle()) {
                    handler_registrations_.erase(it);
                    result = true;
                    break;
                }
            }
        });
        return result;
    }

    /**
     * @brief Remove all handlers from event bus.
     */
    void remove_handlers() noexcept {
        safe_unique_registrations_access(
            [this]() { handler_registrations_.clear(); });
    }

    /**
     * @brief Get the number of handlers registered with the event bus.
     * @return The total number of handlers.
     */
    [[nodiscard]] std::size_t handler_count() noexcept {
        std::shared_lock<mutex_type> lock(registration_mutex_);
        std::size_t count{};
        safe_shared_registrations_access(
            [this, &count]() { count = handler_registrations_.size(); });
        return count;
    }

  private:
    using mutex_type = std::shared_mutex;
    mutable mutex_type registration_mutex_;
    std::unordered_multimap<std::type_index, std::function<void(std::any)>>
        handler_registrations_;

    template <typename Callable>
    void safe_shared_registrations_access(Callable &&callable) {
        try {
            std::shared_lock<mutex_type> lock(registration_mutex_);
            callable();
        } catch (std::system_error &) {
        }
    }
    template <typename Callable>
    void safe_unique_registrations_access(Callable &&callable) {
        try {
            // if(registration_mutex_.locked_by_caller()) return;
            // if this fails, an exception may be thrown.
            std::unique_lock<mutex_type> lock(registration_mutex_);
            callable();
        } catch (std::system_error &) {
            // do nothing
        }
    }
};

inline const void *handler_registration::handle() const { return handle_; }

inline void handler_registration::unregister() noexcept {
    if (event_bus_ && handle_) {
        event_bus_->remove_handler(*this);
        handle_ = nullptr;
    }
}

inline handler_registration::handler_registration(const void *handle,
                                                  primihub::common::event_bus *bus)
    : handle_(handle), event_bus_(bus) {}

inline handler_registration::handler_registration(
    handler_registration &&other) noexcept
    : handle_(std::exchange(other.handle_, nullptr)),
      event_bus_(std::exchange(other.event_bus_, nullptr)) {}

inline handler_registration &
handler_registration::operator=(handler_registration &&other) noexcept {
    handle_ = std::exchange(other.handle_, nullptr);
    event_bus_ = std::exchange(other.event_bus_, nullptr);
    return *this;
}

inline handler_registration::~handler_registration() { unregister(); }

} // namespace primihub::common
