{
    "targets": [
        {
            "target_name": "astrarp_node",
            "sources": ["core/binding.cpp"],
            "include_dirs": [
                "<!@(node -p \"require('node-addon-api').include\")",
                "./core/include",
                "./core/llama.cpp/ggml/include",
                "./core/llama.cpp/include",
                "./core/pjh_json/include",
            ],
            "dependencies": ["<!(node -p \"require('node-addon-api').gyp\")"],
            "libraries": ["../core/build/Release/astrarp_lib.lib"],
            "cflags_cc": ["-std=c++20"],
            "defines": ["NAPI_CPP_EXCEPTIONS"],
            "msvs_settings": {
                "VCCLCompilerTool": {
                    "DebugInformationFormat": "0",
                    "ExceptionHandling": 1,
                }
            },
        }
    ]
}
