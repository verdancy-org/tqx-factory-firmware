#pragma once

// clang-format off
/* === MODULE MANIFEST V2 ===
module_description: 事件绑定模块，支持通过构造函数参数配置事件绑定关系
constructor_args:
  - modules:
    - name: "source"
      module_ref: "@source"
    - name: "target"
      module_ref: "@target"
  - event_binding_groups:
    - bindings:
      - source_module: "source"
        source_event: 1
        target_module: "target"
        target_event: 1
required_hardware: []
depends: []
=== END MANIFEST === */
// clang-format on

#include <cstdint>
#include <cstring>
#include <initializer_list>

#include "app_framework.hpp"
#include "event.hpp"

class EventBinder : public LibXR::Application
{
 public:
  struct EventBinding
  {
    const char* source_module;
    std::uint32_t source_event;
    const char* target_module;
    std::uint32_t target_event;

    template <typename SourceEvent, typename TargetEvent>
    EventBinding(const char* source, SourceEvent source_id, const char* target,
                 TargetEvent target_id)
        : source_module(source),
          source_event(static_cast<std::uint32_t>(source_id)),
          target_module(target),
          target_event(static_cast<std::uint32_t>(target_id))
    {
    }
  };

  struct BindingGroup
  {
    std::initializer_list<EventBinding> bindings;
  };

  struct ModuleInfo
  {
    const char* name;
    LibXR::Event* event;

    template <typename Module>
    ModuleInfo(const char* module_name, Module& module)
        : name(module_name), event(&module.GetEvent())
    {
    }

    ModuleInfo(const char* module_name, LibXR::Event* module_event)
        : name(module_name), event(module_event)
    {
    }
  };

  EventBinder(LibXR::HardwareContainer& hw, LibXR::ApplicationManager& app,
              std::initializer_list<ModuleInfo> modules,
              std::initializer_list<BindingGroup> event_binding_groups)
  {
    UNUSED(hw);
    UNUSED(app);

    for (const auto& group : event_binding_groups)
    {
      for (const auto& binding : group.bindings)
      {
        LibXR::Event* source = FindEvent(modules, binding.source_module);
        LibXR::Event* target = FindEvent(modules, binding.target_module);
        if (source != nullptr && target != nullptr)
        {
          target->Bind(*source, binding.source_event, binding.target_event);
        }
      }
    }
  }

  void OnMonitor() override {}

 private:
  static LibXR::Event* FindEvent(std::initializer_list<ModuleInfo> modules,
                                 const char* name)
  {
    for (const auto& module : modules)
    {
      if (std::strcmp(module.name, name) == 0)
      {
        return module.event;
      }
    }
    return nullptr;
  }
};
