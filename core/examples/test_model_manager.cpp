#include <iostream>
#include <string>
#include <cassert>

#include "core/model_manager.hpp"
#include "core/model.hpp"
#include "core/model_params.hpp"

int main()
{
    using namespace astra_rp::core;
    std::cout << "Test started." << std::endl;

    auto &manager = ModelManager::instance();
    std::cout << "Model Manager successfully initialized." << std::endl;

    astra_rp::Str path(std::getenv("LLAMA_MODEL_DIR"));
    std::cout << path << std::endl;
    astra_rp::Str name("test_model");
    ModelParams params(name);

    auto model = manager.load(path, params);
    std::cout << "Model successfully loaded." << std::endl;

    assert(model != nullptr);
    assert(model->name() == name);
    assert(model->raw() != nullptr);
    std::cout << "Model check passed." << std::endl;

    manager.unload(name);
    std::cout << "Model successfully unloaded." << std::endl;
}