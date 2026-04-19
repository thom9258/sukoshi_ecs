#include "../../ecs.hpp"
#include <libtester/libtester.h>

#include <array>

struct component_name_t {
  std::string name;
};

struct component_pos_t {
  float x;
  float y;
  float z;
};

struct component_health_t {
  float health;
};

void test_added_components_has_correct_memory_layout() {
  /*
   * Expected Component Configuration & the Corresponding Memory Indices Layout:
   * note that actual memory_index = component_index - 1
   * ------------------------
   *    | name | pos | health
   * ------------------------
   * a  | 1    | 1   |
   * b  | 2    |     |
   * c  | 3    | 2   | 1
   */
  static constexpr std::size_t max_entities = 10;
  using manager_t = alex::ecs::manager_t<
      alex::ecs::component_policy_t<component_name_t, max_entities>,
      alex::ecs::component_policy_t<component_pos_t, max_entities>,
      alex::ecs::component_policy_t<component_health_t, max_entities>>;
  using entity_id_t = manager_t::entity_id_t;
  using component_indices_pointer_t = manager_t::component_indices_pointer_t;

  std::array<manager_t::entity_t, max_entities> entity_memory;
  std::array<component_name_t, max_entities> name_components;
  std::array<component_pos_t, max_entities> pos_components;
  std::array<component_health_t, max_entities> health_components;

  manager_t manager(entity_memory, name_components, pos_components,
                    health_components);
  entity_id_t a = manager.new_entity();
  {
    auto *name = manager.add_component<component_name_t>(a);
    TEST(name != nullptr);
    TEST(manager.has_component<component_name_t>(a) == true);

    auto *pos = manager.add_component<component_pos_t>(a);
    TEST(pos != nullptr);
    TEST(manager.has_component<component_pos_t>(a) == true);
  }

  entity_id_t b = manager.new_entity();
  {
    auto *name = manager.add_component<component_name_t>(b);
    TEST(name != nullptr);
    TEST(manager.has_component<component_name_t>(b) == true);
  }

  entity_id_t c = manager.new_entity();
  {
    auto *name = manager.add_component<component_name_t>(c);
    TEST(name != nullptr);
    TEST(manager.has_component<component_name_t>(c) == true);

    auto *pos = manager.add_component<component_pos_t>(c);
    TEST(pos != nullptr);
    TEST(manager.has_component<component_pos_t>(c) == true);

    auto *health = manager.add_component<component_health_t>(c);
    TEST(health != nullptr);
    TEST(manager.has_component<component_health_t>(c) == true);
  }

  {
    component_indices_pointer_t indices = manager.find_component_indices(a);
    TEST(indices != nullptr);
    TEST((*indices)[manager.get_component_index<component_name_t>()] == 1);
    TEST((*indices)[manager.get_component_index<component_pos_t>()] == 1);
    TEST((*indices)[manager.get_component_index<component_health_t>()] ==
         manager_t::invalid_component_index_v);
  }

  {
    component_indices_pointer_t indices = manager.find_component_indices(b);
    TEST(indices != nullptr);
    TEST((*indices)[manager.get_component_index<component_name_t>()] == 2);
    TEST((*indices)[manager.get_component_index<component_pos_t>()] ==
         manager_t::invalid_component_index_v);
    TEST((*indices)[manager.get_component_index<component_health_t>()] ==
         manager_t::invalid_component_index_v);
  }

  {
    component_indices_pointer_t indices = manager.find_component_indices(c);
    TEST(indices != nullptr);
    TEST((*indices)[manager.get_component_index<component_name_t>()] == 3);
    TEST((*indices)[manager.get_component_index<component_pos_t>()] == 2);
    TEST((*indices)[manager.get_component_index<component_health_t>()] == 1);
  }
}

int main(int argc, char **argv) {
  ltcontext_begin(argc, argv);

  TEST_UNIT(test_added_components_has_correct_memory_layout());

  ltcontext_end();
  return 0;
}
