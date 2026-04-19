#pragma once

#include <bitset>
#include <concepts>
#include <cstdint>
#include <span>
#include <tuple>
#include <type_traits>

#include <print>

namespace alex::ecs {

namespace helper {

template <class T, class... Ts>
constexpr std::size_t index_of(const std::tuple<Ts...> &) {
  int found{}, count{};
  ((!found ? (++count, found = std::is_same_v<T, Ts>) : 0), ...);
  return found ? count - 1 : count;
}

} // namespace helper

template <typename t_component, std::uint32_t v_max_count>
struct component_policy_t {
  using component_t = std::remove_cvref_t<t_component>;
  static constexpr std::uint32_t max_count{v_max_count};
};

template <typename t_policy>
concept component_policy = requires(t_policy p) {
  { std::is_destructible_v<typename t_policy::component_t> };
  { std::movable<typename t_policy::component_t> };
  { t_policy::max_count } -> std::convertible_to<std::uint32_t>;
};

static_assert(component_policy<component_policy_t<char, 50>>);

template <typename... component_policies> struct manager_t {
  using entity_id_t = std::uint64_t;
  using memory_index_plus1_t = std::uint32_t;
  using component_indices_t =
      std::array<memory_index_plus1_t, sizeof...(component_policies)>;

  using component_indices_pointer_t = component_indices_t *;

  static constexpr entity_id_t invalid_entity_id_v{0};
  static constexpr memory_index_plus1_t invalid_component_index_v{0};
  struct initialize_t {};
  struct entity_t {
    entity_id_t id{invalid_entity_id_v};
    component_indices_t indices;
  };
  using entity_pointer_t = entity_t *;
  using entity_memory_t = std::span<entity_t>;
  using component_memories_t =
      std::tuple<std::span<typename component_policies::component_t>...>;

  using component_memory_activeflags_t =
      std::tuple<std::bitset<component_policies::max_count>...>;

  explicit constexpr manager_t(
      entity_memory_t entity_memory,
      std::span<typename component_policies::
                    component_t>... component_memories) noexcept
      : entity_memory{entity_memory},
        component_memories{std::make_tuple(component_memories...)} {}

  [[nodiscard]]
  entity_id_t generate_next_entity_id() noexcept {
    entity_id_t const entity = next_entity;
    next_entity++;
    return entity;
  }

  [[nodiscard]]
  constexpr auto new_entity() noexcept -> entity_id_t {
    for (entity_t &correlation : entity_memory) {
      if (correlation.id == invalid_entity_id_v) {
        correlation.id = generate_next_entity_id();
        for (memory_index_plus1_t &index : correlation.indices) {
          index = invalid_component_index_v;
        }

        return correlation.id;
      }
    }
    return invalid_entity_id_v;
  }

  [[nodiscard]]
  constexpr auto find_component_indices(entity_id_t entity) const noexcept
      -> component_indices_pointer_t {

    if (entity == invalid_entity_id_v) {
      return nullptr;
    }

    for (entity_t &correlation : entity_memory) {
      if (correlation.id == entity) {
        return &correlation.indices;
      }
    }

    return nullptr;
  }

  template <typename t_component>
  [[nodiscard]]
  constexpr auto get_component_index() const noexcept -> std::size_t {
    using component_memory_t = std::span<t_component>;
    return helper::index_of<component_memory_t>(component_memories);
  }

  template <typename t_component>
  constexpr auto has_component(entity_id_t entity) const noexcept -> bool {
    std::size_t const component_index = get_component_index<t_component>();
    component_indices_t *const indices = find_component_indices(entity);
    if (!indices) {
      return false;
    }

    if ((*indices)[component_index] == invalid_component_index_v) {
      return false;
    }

    return true;
  }

  template <typename t_component>
  [[nodiscard]]
  constexpr auto get_component(entity_id_t entity) const noexcept
      -> t_component * {
    std::size_t const component_index = get_component_index<t_component>();
    component_indices_t *indices = find_component_indices(entity);
    if (!indices) {
      return nullptr;
    }

    memory_index_plus1_t const memory_index_plus1 = (*indices)[component_index];
    if (memory_index_plus1 == invalid_component_index_v) {
      return nullptr;
    }

    return &(
        std::get<component_index>(component_memories)[memory_index_plus1 - 1]);
  }

  template <typename t_component>
  [[nodiscard]]
  constexpr auto add_component(entity_id_t entity) noexcept -> t_component * {
    std::size_t const component_index = get_component_index<t_component>();
    component_indices_t *const indices = find_component_indices(entity);
    if (!indices) {
      return nullptr;
    }

    memory_index_plus1_t const memory_index_plus1 = (*indices)[component_index];
    if (memory_index_plus1 != invalid_component_index_v) {
      return &(std::get<component_index>(
          component_memories)[memory_index_plus1 - 1]);
    }

    std::size_t const max_components =
        std::get<component_index>(component_memories).size();
    for (memory_index_plus1_t i = 0; i < max_components; i++) {
      if (std::get<component_index>(activeflags)[i] == 0) {
        std::get<component_index>(activeflags).set(i);
        (*indices)[component_index] = i + 1;
        return &(std::get<component_index>(component_memories)[i]);
      }
    }

    return nullptr;
  }

  [[nodiscard]]
  auto constexpr get_entities(
      std::span<entity_pointer_t> out_entities) const noexcept -> std::size_t {
    std::size_t entity_count{0};
    for (entity_t &correlation : entity_memory) {
      if (entity_count >= out_entities.size()) {
        return entity_count;
      }

      if (correlation.id == invalid_entity_id_v) {
        continue;
      }

      out_entities[entity_count] = &correlation;
      entity_count++;
    }

    return entity_count;
  }

  entity_id_t next_entity{1};
  entity_memory_t entity_memory;
  component_memories_t component_memories;
  component_memory_activeflags_t activeflags;
};

} // namespace alex::ecs
