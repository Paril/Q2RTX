cmake_minimum_required (VERSION 3.9)
cmake_policy(SET CMP0069 NEW)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
SET(SRC_BASENAC
	basenac/g_ai.c
	basenac/g_chase.c
	basenac/g_cmds.c
	basenac/g_combat.c
	basenac/g_func.c
	basenac/g_items.c
	basenac/g_main.c
	basenac/g_misc.c
	basenac/g_monster.c
	basenac/g_phys.c
	basenac/g_ptrs.c
	basenac/g_save.c
	basenac/g_spawn.c
	basenac/g_svcmds.c
	basenac/g_target.c
	basenac/g_trigger.c
	basenac/g_turret.c
	basenac/g_utils.c
	basenac/g_weapon.c
	basenac/m_berserk.c
	basenac/m_boss2.c
	basenac/m_boss3.c
	basenac/m_boss31.c
	basenac/m_boss32.c
	basenac/m_brain.c
	basenac/m_chick.c
	basenac/m_flipper.c
	basenac/m_float.c
	basenac/m_flyer.c
	basenac/m_gladiator.c
	basenac/m_gunner.c
	basenac/m_hover.c
	basenac/m_hover.c
	basenac/m_infantry.c
	basenac/m_insane.c
	basenac/m_medic.c
	basenac/m_move.c
	basenac/m_mutant.c
	basenac/m_parasite.c
	basenac/m_soldier.c
	basenac/m_supertank.c
	basenac/m_tank.c
	basenac/p_client.c
	basenac/p_hud.c
	basenac/p_trail.c
	basenac/p_view.c
	basenac/p_weapon.c
	basenac/weapons/nac_axe.c
	basenac/weapons/nac_perf.c
	basenac/weapons/nac_shotgun.c
	basenac/weapons/nac_sshotgun.c
	basenac/weapons/nac_blaster.c
	basenac/weapons/nac_rocket.c
	basenac/weapons/nac_launch.c
	basenac/weapons/nac_thunderbolt.c
	basenac/monsters/nac_monster.c
	basenac/monsters/nac_knight.c
	basenac/monsters/nac_fiend.c
	basenac/monsters/nac_grunt.c
	basenac/monsters/nac_enforcer.c
	basenac/monsters/nac_vore.c
	basenac/monsters/nac_scrag.c
	basenac/entities/nac_env_reverb.c
)

SET(HEADERS_BASENAC
	basenac/g_local.h
	basenac/g_ptrs.h
	basenac/m_berserk.h
	basenac/m_boss2.h
	basenac/m_boss31.h
	basenac/m_boss32.h
	basenac/m_brain.h
	basenac/m_chick.h
	basenac/m_flipper.h
	basenac/m_float.h
	basenac/m_flyer.h
	basenac/m_gladiator.h
	basenac/m_gunner.h
	basenac/m_hover.h
	basenac/m_infantry.h
	basenac/m_insane.h
	basenac/m_medic.h
	basenac/m_mutant.h
	basenac/m_parasite.h
	basenac/m_player.h
	basenac/m_rider.h
	basenac/m_soldier.h
	basenac/m_supertank.h
	basenac/m_tank.h
	basenac/monsters/nac_monster.h
)


SET(SRC_CLIENT
	client/ascii.c
	client/console.c
	client/cin.c
	client/crc.c
	client/demo.c
	client/download.c
	client/effects.c
	client/entities.c
	client/input.c
	client/keys.c
	client/locs.c
	client/main.c
	client/newfx.c
#	client/null.c
	client/parse.c
	client/precache.c
	client/predict.c
	client/refresh.c
	client/screen.c
	client/tent.c
	client/view.c
	client/ui/demos.c
	client/ui/menu.c
	client/ui/playerconfig.c
	client/ui/playermodels.c
	client/ui/script.c
	client/ui/servers.c
	client/ui/ui.c
	client/sound/al.c
	client/sound/main.c
	client/sound/mem.c
	client/sound/ogg.c
	client/sound/qal/fixed.c
)

SET(SRC_CLIENT_HTTP
	client/http.c
)

SET(HEADERS_CLIENT
	client/client.h
	client/ui/ui.h
	client/sound/sound.h
	client/sound/qal/fixed.h
)

SET(SRC_SERVER
	server/commands.c
	server/entities.c
	server/game.c
	server/init.c
	server/main.c
	server/send.c
	server/user.c
	server/world.c
	server/save.c
)

SET(HEADERS_SERVER
	server/server.h
)

SET(SRC_COMMON
	common/bsp.c
	common/cmd.c
	common/cmodel.c
	common/common.c
	common/cvar.c
	common/error.c
	common/field.c
	common/fifo.c
	common/files.c
	common/math.c
	common/mdfour.c
	common/msg.c
	common/pmove.c
	common/prompt.c
	common/sizebuf.c
#	common/tests.c
	common/utils.c
	common/zone.c
	common/net/chan.c
	common/net/net.c
	system/system.c
)

SET(HEADERS_COMMON
	common/net/inet_ntop.h
	common/net/inet_pton.h
	common/net/win.h
)

SET(SRC_REFRESH
	refresh/images.c
	refresh/models.c
	refresh/model_iqm.c
	refresh/stb/stb.c
)

SET(SRC_SHARED
	shared/m_flash.c
	shared/shared.c
	shared/iqm.c
)

SET(SRC_LINUX
	unix/hunk.c
	unix/system.c
	unix/tty.c
)

SET(SRC_WINDOWS
	windows/debug.c
	windows/hunk.c
	windows/system.c
)

SET(HEADERS_WINDOWS
	windows/client.h
)

SET(SRC_VKPT
	refresh/vkpt/asvgf.c
	refresh/vkpt/bloom.c
	refresh/vkpt/bsp_mesh.c
	refresh/vkpt/draw.c
	refresh/vkpt/fog.c
	refresh/vkpt/cameras.c
	refresh/vkpt/freecam.c
	refresh/vkpt/fsr.c
	refresh/vkpt/main.c
	refresh/vkpt/material.c
	refresh/vkpt/matrix.c
	refresh/vkpt/mgpu.c
	refresh/vkpt/models.c
	refresh/vkpt/path_tracer.c
	refresh/vkpt/physical_sky.c
	refresh/vkpt/precomputed_sky.c
	refresh/vkpt/profiler.c
	refresh/vkpt/shadow_map.c
	refresh/vkpt/textures.c
	refresh/vkpt/tone_mapping.c
	refresh/vkpt/post_process.c
	refresh/vkpt/transparency.c
	refresh/vkpt/uniform_buffer.c
	refresh/vkpt/vertex_buffer.c
	refresh/vkpt/vk_util.c
	refresh/vkpt/buddy_allocator.c
	refresh/vkpt/device_memory_allocator.c
	refresh/vkpt/god_rays.c
	refresh/vkpt/conversion.c
)

SET(HEADERS_VKPT
	refresh/vkpt/vkpt.h
	refresh/vkpt/vk_util.h
	refresh/vkpt/buddy_allocator.h
	refresh/vkpt/device_memory_allocator.h
	refresh/vkpt/fog.h
	refresh/vkpt/cameras.h
	refresh/vkpt/material.h
	refresh/vkpt/physical_sky.h
	refresh/vkpt/precomputed_sky.h
	refresh/vkpt/conversion.h
)

set(SRC_SHADERS
	refresh/vkpt/shader/animate_materials.comp
	refresh/vkpt/shader/god_rays_filter.comp
	refresh/vkpt/shader/god_rays.comp
	refresh/vkpt/shader/bloom_composite.comp
	refresh/vkpt/shader/bloom_blur.comp
	refresh/vkpt/shader/bloom_downscale.comp
	refresh/vkpt/shader/compositing.comp
	refresh/vkpt/shader/checkerboard_interleave.comp
	refresh/vkpt/shader/asvgf_atrous.comp
	refresh/vkpt/shader/asvgf_gradient_atrous.comp
	refresh/vkpt/shader/asvgf_gradient_img.comp
	refresh/vkpt/shader/asvgf_gradient_reproject.comp
	refresh/vkpt/shader/asvgf_lf.comp
	refresh/vkpt/shader/asvgf_taau.comp
	refresh/vkpt/shader/asvgf_temporal.comp
	refresh/vkpt/shader/instance_geometry.comp
	refresh/vkpt/shader/normalize_normal_map.comp
	refresh/vkpt/shader/tone_mapping_histogram.comp
	refresh/vkpt/shader/tone_mapping_curve.comp
    refresh/vkpt/shader/tone_mapping_apply.comp
    refresh/vkpt/shader/post_process.comp
	refresh/vkpt/shader/physical_sky.comp
	refresh/vkpt/shader/physical_sky_space.comp
	refresh/vkpt/shader/shadow_map.vert
	refresh/vkpt/shader/sky_buffer_resolve.comp
	refresh/vkpt/shader/stretch_pic.frag
	refresh/vkpt/shader/stretch_pic.vert
	refresh/vkpt/shader/final_blit_lanczos.frag
	refresh/vkpt/shader/final_blit.vert
	refresh/vkpt/shader/fsr_easu_fp16.comp
	refresh/vkpt/shader/fsr_easu_fp32.comp
	refresh/vkpt/shader/fsr_rcas_fp16.comp
	refresh/vkpt/shader/fsr_rcas_fp32.comp
)

set(SRC_RT_SHADERS
	refresh/vkpt/shader/primary_rays.rgen
	refresh/vkpt/shader/direct_lighting.rgen
	refresh/vkpt/shader/indirect_lighting.rgen
	refresh/vkpt/shader/path_tracer.rchit
	refresh/vkpt/shader/path_tracer.rmiss
	refresh/vkpt/shader/path_tracer_masked.rahit
	refresh/vkpt/shader/path_tracer_particle.rahit
	refresh/vkpt/shader/path_tracer_sprite.rahit
	refresh/vkpt/shader/path_tracer_beam.rahit
	refresh/vkpt/shader/path_tracer_beam.rint
	refresh/vkpt/shader/path_tracer_explosion.rahit
	refresh/vkpt/shader/reflect_refract.rgen
)

# Only build shaders in 64-bit builds, because the client
# is also only built for 64-bit systems.
IF(IS_64_BIT)
    include(../cmake/compileShaders.cmake)

    foreach(s ${SRC_RT_SHADERS})
        get_filename_component(shader_name ${s} NAME)
        get_filename_component(shader_ext ${s} EXT)
        compile_shader(SOURCE_FILE ${s} OUTPUT_FILE_LIST shader_bytecode OUTPUT_FILE_NAME "${shader_name}.pipeline")
        if (shader_ext STREQUAL ".rgen")
            compile_shader(SOURCE_FILE ${s} OUTPUT_FILE_LIST shader_bytecode OUTPUT_FILE_NAME "${shader_name}.query" DEFINES "-DKHR_RAY_QUERY" STAGE "comp")
        endif()
    endforeach()

    foreach(s ${SRC_SHADERS})
        compile_shader(SOURCE_FILE ${s} INCLUDES "-I${CMAKE_CURRENT_SOURCE_DIR}/refresh/vkpt/fsr" OUTPUT_FILE_LIST shader_bytecode)
    endforeach()

    add_custom_target(shaders DEPENDS ${shader_bytecode})

    if(TARGET glslangValidator)
        add_dependencies(shaders glslangValidator)
    endif()
ENDIF()

# Embed version number
ADD_DEFINITIONS(-DVERSION_MAJOR=${Q2RTX_VERSION_MAJOR})
ADD_DEFINITIONS(-DVERSION_MINOR=${Q2RTX_VERSION_MINOR})
ADD_DEFINITIONS(-DVERSION_POINT=${Q2RTX_VERSION_POINT})
ADD_DEFINITIONS(-DVERSION_SHA=${Q2RTX_VERSION_SHA})
ADD_DEFINITIONS(-DVERSION_BRANCH=${Q2RTX_VERSION_BRANCH})

ADD_DEFINITIONS(-DHAVE_CONFIG_H=1 -DCURL_STATICLIB -DAL_LIBTYPE_STATIC)

IF(WIN32)
	ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
ENDIF()

add_compile_definitions($<$<CONFIG:Debug>:USE_DEBUG>)

if(NOT WIN32)
    add_compile_definitions(_GNU_SOURCE)
endif()

# Check whether compiler supports 'flagstr', and if so, set ${flagvar} to that value
function(CCompilerFlagString flagvar flagstr)
    CHECK_C_COMPILER_FLAG("${flagstr}" ${flagvar}_FOUND)
    if(${flagvar}_FOUND)
        set(${flagvar} "${flagstr}" PARENT_SCOPE)
    endif()
endfunction()
# Check whether compiler supports 'flagstr', and if so, append that value to ${flagvar}
function(CCompilerFlagStringAppend flagvar flagstr)
    string(TOUPPER "${flagstr}" auto_flag_var)
    string(REGEX REPLACE "[^A-Z0-9]" "_" auto_flag_var "${auto_flag_var}")
    CCompilerFlagString (C_${auto_flag_var} "${flagstr}")
    if(NOT "${C_${auto_flag_var}}" EQUAL "")
        set(${flagvar} "${${flagvar}} ${C_${auto_flag_var}} " PARENT_SCOPE)
    endif()
endfunction()

if(NOT MSVC)
    CCompilerFlagStringAppend(CMAKE_C_FLAGS "-fsigned-char")
    CCompilerFlagStringAppend(CMAKE_C_FLAGS "-Wall")
    CCompilerFlagStringAppend(CMAKE_C_FLAGS "-Wpointer-arith")
    CCompilerFlagStringAppend(CMAKE_C_FLAGS "-Wformat-security")
    CCompilerFlagStringAppend(CMAKE_C_FLAGS "-Werror=vla")
    CCompilerFlagStringAppend(CMAKE_C_FLAGS "-Wstrict-prototypes")
    # CCompilerFlagString(WARN_MISSING_PROTOTYPES "-Wmissing-prototypes")
endif()

ADD_LIBRARY(basenac SHARED ${SRC_BASENAC} ${HEADERS_BASENAC} ${SRC_SHARED})
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    # Workaround for GCC 10 linking shared libgcc by default
    target_link_options(baseq2 PRIVATE "-static-libgcc")
endif()

set(FAKE_TARGET fakeTarget)

add_custom_target(fakeTarget
    cd "${CMAKE_CURRENT_SOURCE_DIR}/basenac/" && py genptr.py > g_ptrs.c
)

add_dependencies(basenac fakeTarget)

IF(WIN32)
    IF(IS_64_BIT)
        ADD_EXECUTABLE(client WIN32 
            ${SRC_CLIENT} ${HEADERS_CLIENT} 
            ${SRC_COMMON} ${HEADERS_COMMON} 
            ${SRC_REFRESH} ${SRC_SHADERS} 
            ${SRC_SHARED} 
            ${SRC_WINDOWS} ${HEADERS_WINDOWS}
            ${SRC_SERVER} ${HEADERS_SERVER}
            windows/res/q2rtx.rc
        )
    ENDIF()
    ADD_EXECUTABLE(server 
        ${SRC_COMMON} ${HEADERS_COMMON} 
        ${SRC_SHARED} 
        ${SRC_WINDOWS} ${HEADERS_WINDOWS}
        ${SRC_SERVER} ${HEADERS_SERVER}
        client/null.c
        windows/res/q2rtxded.rc
    )
ELSE()
    IF(IS_64_BIT)
        ADD_EXECUTABLE(client
            ${SRC_CLIENT} ${HEADERS_CLIENT} 
            ${SRC_COMMON} ${HEADERS_COMMON} 
            ${SRC_REFRESH} ${SRC_SHADERS} 
            ${SRC_SHARED} 
            ${SRC_LINUX}
            ${SRC_SERVER} ${HEADERS_SERVER}
        )
    ENDIF()
    ADD_EXECUTABLE(server
        ${SRC_COMMON} ${HEADERS_COMMON} 
        ${SRC_SHARED} 
        ${SRC_LINUX}
        ${SRC_SERVER} ${HEADERS_SERVER}
        client/null.c
    )
ENDIF()

TARGET_COMPILE_DEFINITIONS(server PRIVATE USE_SERVER=1)
target_compile_options(server PRIVATE "${WARN_MISSING_PROTOTYPES}")

IF (TARGET client)
    TARGET_COMPILE_DEFINITIONS(client PRIVATE USE_CLIENT=1 USE_FIXED_LIBAL=1 USE_SDL=1)
    target_compile_options(client PRIVATE "${WARN_MISSING_PROTOTYPES}")

    IF(CONFIG_USE_CURL)
        TARGET_SOURCES(client PRIVATE ${SRC_CLIENT_HTTP})
        TARGET_COMPILE_DEFINITIONS(client PRIVATE USE_CURL=1)
        TARGET_LINK_LIBRARIES(client libcurl)
    ENDIF()

    add_dependencies(client shaders)

    IF (CONFIG_VKPT_RENDERER)
        TARGET_SOURCES(client PRIVATE ${SRC_VKPT} ${HEADERS_VKPT})
        TARGET_INCLUDE_DIRECTORIES(client PRIVATE ${CMAKE_SOURCE_DIR}/extern/Vulkan-Headers/include)
        LINK_DIRECTORIES(client PRIVATE ${CMAKE_SOURCE_DIR})
        TARGET_COMPILE_DEFINITIONS(client PRIVATE REF_VKPT=1 USE_REF=1)
        IF (CONFIG_VKPT_ENABLE_DEVICE_GROUPS)
            TARGET_COMPILE_DEFINITIONS(client PRIVATE VKPT_DEVICE_GROUPS)
        ENDIF()
        if (CONFIG_VKPT_ENABLE_IMAGE_DUMPS)
            TARGET_COMPILE_DEFINITIONS(client PRIVATE VKPT_IMAGE_DUMPS)
        ENDIF()
        IF (WIN32)
        TARGET_LINK_LIBRARIES(client vulkan-1)
        ELSE ()
        TARGET_LINK_LIBRARIES(client vulkan)
        ENDIF()
    ENDIF()

    TARGET_LINK_LIBRARIES(client stb)
    TARGET_LINK_LIBRARIES(client tinyobjloader)
    TARGET_LINK_LIBRARIES(client OpenAL)
ENDIF()

SOURCE_GROUP("basenac\\sources" FILES ${SRC_BASENAC})
SOURCE_GROUP("basenac\\headers" FILES ${HEADERS_BASENAC})
SOURCE_GROUP("client\\sources" FILES ${SRC_CLIENT})
SOURCE_GROUP("client\\headers" FILES ${HEADERS_CLIENT})
SOURCE_GROUP("server\\sources" FILES ${SRC_SERVER})
SOURCE_GROUP("server\\headers" FILES ${HEADERS_SERVER})
SOURCE_GROUP("common\\sources" FILES ${SRC_COMMON})
SOURCE_GROUP("common\\headers" FILES ${HEADERS_COMMON})
SOURCE_GROUP("refresh\\sources" FILES ${SRC_REFRESH} ${SRC_VKPT})
SOURCE_GROUP("refresh\\headers" FILES ${HEADERS_VKPT})
SOURCE_GROUP("refresh\\shaders" FILES ${SRC_SHADERS})
SOURCE_GROUP("shared\\sources" FILES ${SRC_SHARED})
SOURCE_GROUP("windows\\sources" FILES ${SRC_WINDOWS})
SOURCE_GROUP("windows\\headers" FILES ${HEADERS_WINDOWS})

IF (WIN32)
    IF(TARGET client)
        TARGET_INCLUDE_DIRECTORIES(client PRIVATE ../VC/inc)
        TARGET_LINK_LIBRARIES(client winmm ws2_32)
        SET_TARGET_PROPERTIES(client PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
        # macro redefinition, deprecation
        TARGET_COMPILE_OPTIONS(client PRIVATE /wd4005 /wd4996)
    ENDIF()

    TARGET_INCLUDE_DIRECTORIES(server PRIVATE ../VC/inc)
    TARGET_INCLUDE_DIRECTORIES(basenac PRIVATE ../VC/inc)
    TARGET_SOURCES(basenac PRIVATE windows/res/baseq2.rc)

    TARGET_LINK_LIBRARIES(server winmm ws2_32)

    SET_TARGET_PROPERTIES(server PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

    # macro redefinition, deprecation
    TARGET_COMPILE_OPTIONS(server PRIVATE /wd4005 /wd4996)
    TARGET_COMPILE_OPTIONS(basenac PRIVATE /wd4005 /wd4996)
ENDIF()

TARGET_INCLUDE_DIRECTORIES(basenac PRIVATE ../inc)

IF(TARGET client)
    TARGET_INCLUDE_DIRECTORIES(client PRIVATE ../inc)
    TARGET_INCLUDE_DIRECTORIES(client PRIVATE "${ZLIB_INCLUDE_DIRS}")
ENDIF()

TARGET_INCLUDE_DIRECTORIES(server PRIVATE ../inc)
TARGET_INCLUDE_DIRECTORIES(server PRIVATE "${ZLIB_INCLUDE_DIRS}")

# Use dynamic zlib for steam runtime
if (CONFIG_LINUX_STEAM_RUNTIME_SUPPORT)
    IF(TARGET client)
        TARGET_LINK_LIBRARIES(client SDL2main SDL2-static z)
    ENDIF()
    TARGET_LINK_LIBRARIES(server SDL2main SDL2-static z)
else()
    IF(TARGET client)
        TARGET_LINK_LIBRARIES(client SDL2main SDL2-static zlibstatic)
    ENDIF()
    TARGET_LINK_LIBRARIES(server SDL2main SDL2-static zlibstatic)
endif()

IF(UNIX)
    TARGET_LINK_LIBRARIES(server dl rt m pthread)
ENDIF()

IF(TARGET client)
    SET_TARGET_PROPERTIES(client
        PROPERTIES
        OUTPUT_NAME "nac"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}"
        DEBUG_POSTFIX ""
    )
ENDIF()

SET_TARGET_PROPERTIES(server
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}"
    DEBUG_POSTFIX ""
)

IF (IS_64_BIT)
    SET_TARGET_PROPERTIES(server PROPERTIES OUTPUT_NAME "nacded")
ELSE()
    SET_TARGET_PROPERTIES(server PROPERTIES OUTPUT_NAME "nacded-x86")
ENDIF()

# specify both LIBRARY and RUNTIME because one works only on Windows and another works only on Linux

SET_TARGET_PROPERTIES(basenac
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/basenac"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/basenac"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/basenac"
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/basenac"
    LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/basenac"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/basenac"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/basenac"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/basenac"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/basenac"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/basenac"
    PREFIX ""
    DEBUG_POSTFIX ""
)

IF(IS_64_BIT)
	IF(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
		SET_TARGET_PROPERTIES(basenac
			PROPERTIES
			LIBRARY_OUTPUT_NAME "gameaarch64"
			RUNTIME_OUTPUT_NAME "gameaarch64"
		)
	ELSEIF(CMAKE_SYSTEM_PROCESSOR STREQUAL "ppc64le")
		SET_TARGET_PROPERTIES(baseq2
			PROPERTIES
			LIBRARY_OUTPUT_NAME "gameppc64le"
			RUNTIME_OUTPUT_NAME "gameppc64le"
                )
	ELSE()
		SET_TARGET_PROPERTIES(basenac
			PROPERTIES
			LIBRARY_OUTPUT_NAME "gamex86_64"
			RUNTIME_OUTPUT_NAME "gamex86_64"
		)
	ENDIF()
ELSE()
    SET_TARGET_PROPERTIES(basenac
        PROPERTIES
        LIBRARY_OUTPUT_NAME "gamex86"
        RUNTIME_OUTPUT_NAME "gamex86"
    )
ENDIF()

IF(CONFIG_LINUX_PACKAGING_SUPPORT)
    # Put the real game binary in /usr/share so we can have a wrapper in /usr/bin
    IF(TARGET client)
        INSTALL(TARGETS client DESTINATION share/quake2rtx/bin COMPONENT shareware)
    ENDIF()
    INSTALL(TARGETS server DESTINATION games COMPONENT shareware)

    IF(NOT CONFIG_LINUX_PACKAGING_SKIP_PKZ)
	    # Compress NVIDIA custom content
	    INSTALL(CODE "set(SOURCE \"${CMAKE_SOURCE_DIR}\")"
	        SCRIPT "${CMAKE_SOURCE_DIR}/setup/package_media.cmake"
	        SCRIPT "${CMAKE_SOURCE_DIR}/setup/package_shaders.cmake"
	        COMPONENT shareware)
	ENDIF()
    INSTALL(TARGETS basenac DESTINATION share/quake2rtx/baseq2 COMPONENT shareware)

    # Package data files, including the shareware pak0 for demo levels
    set (SHAREWARE_DATA_FILES_LIST
        "${CMAKE_SOURCE_DIR}/baseq2/q2rtx_media.pkz"
        "${CMAKE_SOURCE_DIR}/baseq2/shaders.pkz"
        "${CMAKE_SOURCE_DIR}/baseq2/blue_noise.pkz"
        "${CMAKE_SOURCE_DIR}/baseq2/shareware/pak0.pak"
    )
    INSTALL(FILES
        ${SHAREWARE_DATA_FILES_LIST}
        DESTINATION share/quake2rtx/baseq2
        COMPONENT shareware)

    # Package shareware player model
    INSTALL(DIRECTORY
        "${CMAKE_SOURCE_DIR}/baseq2/shareware/players"
        DESTINATION share/quake2rtx/baseq2
        COMPONENT shareware)

    # Package the legal and info files
    INSTALL(FILES
        "${CMAKE_SOURCE_DIR}/license.txt"
        "${CMAKE_SOURCE_DIR}/notice.txt"
        "${CMAKE_SOURCE_DIR}/readme.md"
        "${CMAKE_SOURCE_DIR}/changelog.md"
        DESTINATION share/quake2rtx/
        COMPONENT shareware)

    # Package the icon/desktop file
    INSTALL(FILES
        "${CMAKE_SOURCE_DIR}/setup/q2rtx.desktop"
        DESTINATION share/applications/
        COMPONENT shareware)
    INSTALL(FILES
        "${CMAKE_SOURCE_DIR}/setup/q2rtx.png"
        DESTINATION share/pixmaps/
        COMPONENT shareware)

    # Install Client Wrapper
    INSTALL(FILES
        "${CMAKE_SOURCE_DIR}/setup/q2rtx.sh"
        DESTINATION games/
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COMPONENT shareware
	RENAME nac)

    # Install PAK file finder
    INSTALL(FILES
        "${CMAKE_SOURCE_DIR}/setup/find-retail-paks.sh"
        DESTINATION share/quake2rtx/bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COMPONENT shareware)
ENDIF()

IF(MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D_CRT_DECLARE_NONSTDC_NAMES")
ENDIF()