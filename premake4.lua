-- A solution contains projects, and defines the available configurations
solution "ckw"
configurations { "Release", "Debug" }

configuration "gmake Debug"
do
    buildoptions { "-g" }
    linkoptions { "-g" }
end

configuration "gmake"
do
  buildoptions { 
      "-Wall", 
      --"-std=c++0x",
      "-std=gnu++0x",
  }
  linkoptions {
      --"-Wl,--entry,_wWinMain,--enable-stdcall-fixup",
      --"-s -mwindows -nostartfiles",
  }
end

configuration "gmake windows"
do
  buildoptions { 
      "-U__CYGWIN__", 
  }
end

configuration "vs*"
do
    flags {
        'WinMain',
    }
    defines {
        '_CRT_SECURE_NO_WARNINGS',
    }
    linkoptions {}
end

configuration "windows*"
do
    defines {
        'WIN32',
        '_WIN32',
        '_WINDOWS',
    }
end

configuration "Debug"
do
  defines { "DEBUG" }
  flags { "Symbols" }
  targetdir "debug"
end

configuration "Release"
do
  defines { "NDEBUG" }
  flags { "Optimize" }
  targetdir "release"
end

configuration {}

------------------------------------------------------------------------------
-- ckw child
------------------------------------------------------------------------------
project "ckwc"
--language "C"
language "C++"
--kind "StaticLib"
--kind "DynamicLib"
--kind "ConsoleApp"
kind "WindowedApp"
flags {
    "Unicode",
}
files {
    "*.cpp", "*.h",
}
excludes {
    "main.cpp",
}
defines {
    "UNICODE",
    "_UNICODE",
    "_WIN32_WINNT=0x0500",
}
includedirs {
}
libdirs {
}
links {
    'Shlwapi',
}


------------------------------------------------------------------------------
-- ckw
------------------------------------------------------------------------------
project "ckw"
--language "C"
language "C++"
--kind "StaticLib"
--kind "DynamicLib"
--kind "ConsoleApp"
kind "WindowedApp"
flags {
    "Unicode",
}
files {
    "*.cpp", "*.h", "*.rc",
}
excludes {
    "mainc.cpp",
}
defines {
    "UNICODE",
    "_UNICODE",
    "_WIN32_WINNT=0x0500",
}
includedirs {
}
libdirs {
}
links {
    'Shlwapi',
}

