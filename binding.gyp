{
    "targets": [
        {
            "target_name": "astrarp_node",
            "sources": [
                "core/binding.cpp"
            ],
            "include_dirs": [
                "<!@(node -p \"require('node-addon-api').include\")",
                "./core/include",
                "./core/llama.cpp/ggml/include",
                "./core/llama.cpp/include",
                "./core/pjh_json/include"
            ],
            "dependencies": [
                "<!(node -p \"require('node-addon-api').gyp\")"
            ],
            "libraries": [
                "<(module_root_dir)/core/build/llama.cpp/ggml/src/Release/ggml-base.lib",
                "<(module_root_dir)/core/build/llama.cpp/ggml/src/Release/ggml-cpu.lib",
                "<(module_root_dir)/core/build/llama.cpp/ggml/src/Release/ggml.lib",
                "<(module_root_dir)/core/build/llama.cpp/src/Release/llama.lib",
                "<(module_root_dir)/core/build/Release/astrarp_lib.lib",
                "<(module_root_dir)/core/build/Release/llama.lib"
            ],
            "cflags_cc": [
                "-std=c++20"
            ],
            "defines": [
                "NAPI_CPP_EXCEPTIONS",
                "LLAMA_SHARED"
            ],
            "msvs_settings": {
                "VCCLCompilerTool": {
                    "DebugInformationFormat": "0",
                    "ExceptionHandling": 1,
                    "AdditionalOptions": [
                        "/utf-8"
                    ]
                }
            }
        }
    ]
}