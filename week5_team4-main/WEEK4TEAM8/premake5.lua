project "WEEK4TEAM8"
    kind "WindowedApp"
    language "C++"
    cppdialect "C++20"
    systemversion "latest"
    characterset "Unicode"

    targetdir ("bin/%{cfg.platform}/%{cfg.buildcfg}")
    objdir ("bin-int/%{cfg.platform}/%{cfg.buildcfg}")
    debugdir "%{wks.location}"

    files {
        "**.cpp",
        "**.h",
        "../Assets/**",
    }

    -- 에셋은 탐색기에 보이기만 하고 빌드에는 걸리지 않게 한다
    vpaths {
        ["Assets/*"] = "../Assets/**",
    }

    filter "files:**.hlsl"
        buildaction "None"

    filter {}

    includedirs {
        ".",
        "ImGui",
        "Json",
        "%{wks.location}/Vendor/include",
    }

    libdirs {
        "%{wks.location}/Vendor/lib/%{cfg.buildcfg}",
    }

    links {
        "d3d11",
        "d3dcompiler",
        "dwrite",
        "d2d1",
        "dxgi",
        "dwmapi",
        "gdi32",
        "imm32",
        "user32",
        "comdlg32",
        "Ole32"
    }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        links { 
            "freetyped" 
        }

    filter "configurations:Release"
        runtime "Release"
        optimize "Off"
        symbols "On"
        links { 
            "freetype" 
        }

    filter "configurations:ObjViewerRelease"
        runtime "Release"
        optimize "Off"
        symbols "On"
        -- ObjViewerRelease는 Release용 외부 라이브러리를 재사용한다.
        libdirs { "%{wks.location}/Vendor/lib/Release" }
        defines { "IS_OBJ_VIEWER=1" }
        links {
            "freetype" 
        }



    filter "system:windows"
        defines {
            "UNICODE",
            "_UNICODE",
            "WIN32_LEAN_AND_MEAN"
        }

filter "toolset:msc*"
    buildoptions { "/utf-8" }

filter {}
