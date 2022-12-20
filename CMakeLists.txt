cmake_minimum_required(VERSION 3.11)
project(alias_q2 LANGUAGES C)

add_subdirectory(ext/alias)
add_subdirectory(ext/alias/ext/glfw)
add_subdirectory(ext/libuv)
add_library(sqlite3 ext/sqlite3.c)
add_subdirectory(tools)

add_library(shared STATIC
    game/q_shared.c
    game/monster/m_flash.c
)
target_include_directories(shared INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(shared alias sqlite3)

add_library(game STATIC
    game/q_shared.c
    game/player/p_weapon.c
    game/player/p_view.c
    game/player/p_trail.c
    game/player/p_hud.c
    game/player/p_client.c
    game/monster/m_tank.c
    game/monster/m_supertank.c
    game/monster/m_soldier.c
    game/monster/m_parasite.c
    game/monster/m_mutant.c
    game/monster/m_move.c
    game/monster/m_medic.c
    game/monster/m_insane.c
    game/monster/m_infantry.c
    game/monster/m_hover.c
    game/monster/m_gunner.c
    game/monster/m_gladiator.c
    game/monster/m_flyer.c
    game/monster/m_float.c
    game/monster/m_flipper.c
    game/monster/m_chick.c
    game/monster/m_brain.c
    game/monster/m_boss32.c
    game/monster/m_boss31.c
    game/monster/m_boss3.c
    game/monster/m_boss2.c
    game/monster/m_berserk.c
    game/monster/m_actor.c
    game/g_weapon.c
    game/g_utils.c
    game/g_turret.c
    game/g_trigger.c
    game/g_target.c
    game/g_svcmds.c
    game/g_spawn.c
    game/g_save.c
    game/g_phys.c
    game/g_monster.c
    game/g_misc.c
    game/g_main.c
    game/g_items.c
    game/g_func.c
    game/g_combat.c
    game/g_cmds.c
    game/g_chase.c
    game/g_ai.c
)
target_link_libraries(game shared)
target_compile_definitions(game PRIVATE GAME_HARD_LINKED=1)

add_library(common STATIC
    qcommon/cmd.c
    qcommon/cmodel.c
    qcommon/common.c
    qcommon/crc.c
    qcommon/cvar.c
    qcommon/files.c
    qcommon/md4.c
    qcommon/net_chan.c
    # qcommon/net_libuv.c
    qcommon/pmove.c
    qcommon/sql.c
)
target_link_libraries(common shared uv_a)

add_library(client STATIC
    client/cl_cin.C
    client/cl_ents.c
    client/cl_fx.c
    client/cl_input.c
    client/cl_inv.c
    client/cl_main.c
    client/cl_newfx.c
    client/cl_parse.c
    client/cl_pred.c
    client/cl_scrn.c
    client/cl_tent.c
    client/cl_view.c
    client/console.c
    client/keys.c
    client/menu.c
    client/qmenu.c
    client/snd_dma.c
    client/snd_mem.c
    client/snd_mix.c
    client/x86.c
    client/in_glfw.c
    client/snd_miniaudio.c
    client/snd_miniaudio_imp.c
)
target_link_libraries(client common glfw)

add_library(server STATIC
    server/sv_ccmds.c
    server/sv_ents.c
    server/sv_game.c
    server/sv_init.c
    server/sv_main.c
    # server/sv_null.c
    server/sv_send.c
    server/sv_user.c
    server/sv_world.c
)
target_link_libraries(server common)

add_library(refresh STATIC
    ref_gl/gl_draw.c
    ref_gl/gl_light.c
    ref_gl/gl_model.c
    ref_gl/gl_particle.c
    ref_gl/gl_rmisc.c
    ref_gl/gl_warp.c
    ref_gl/gl_image.c
    ref_gl/gl_mesh.c
    ref_gl/gl_rmain.c
    ref_gl/gl_rsurf.c
    ref_gl/gl_sky.c
    ref_gl/gl_thin.c
    ref_gl/glimp_glfw.c
    ref_gl/qgl_glfw.c
    ref_gl/render_mesh.c
)
target_link_libraries(refresh common glfw)
target_compile_definitions(refresh PRIVATE REF_HARD_LINKED=1)

add_executable(quake2)
target_link_libraries(quake2
    client server
    refresh
    game
)

if(WIN32)
    target_sources(quake2
        PRIVATE
            # win32/cd_win.c
            win32/conproc.c
            # win32/glw_imp.c
            # win32/in_win.c
            win32/net_wins.c
            win32/q_shwin.c
            # win32/qgl_win.c
            # win32/rw_ddraw.c
            # win32/rw_dib.c
            # win32/rw_imp.c
            # win32/snd_win.c
            win32/sys_win.c
            win32/vid_dll.c
            win32/vid_menu.c
    )

    target_link_libraries(quake2 winmm wsock32)
endif()
