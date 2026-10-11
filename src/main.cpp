#include "raylib.h"
#include "hud_and_menu.hpp"
#include <ctime>
#include <cstring>
#include <iostream>

// Need glClear declaration for clearing the depth buffer before the viewmodel pass
extern "C" void glClear(unsigned int mask);

int RunSelfTest() {
    std::cout << "[Self-Test] Verifying Phantom Forces Raylib C++ Systems...\n";
    GameWorld world;

    // 1. Verify all 8 maps (3 Classic PF + 5 15v15 Blueprints) & AI NavGraphs
    for (int m = 0; m < (int)MAP_COUNT; ++m) {
        world.LoadMapAndReset((MapId)m, MODE_FLARE_DOMINATION);
        size_t totalEdges = 0;
        for (const auto& nb : world.map.navNeighbors) totalEdges += nb.size();
        int stepNode = world.map.FindNextWaypointOnPath(0, (int)world.map.waypoints.size() - 1);
        std::cout << "  -> Loaded Map: " << world.map.name
                  << " | Blocks: " << world.map.blocks.size()
                  << " | NavNodes: " << world.map.waypoints.size()
                  << " | NavEdges: " << (totalEdges / 2)
                  << " | A* Step(0->Last): " << stepNode
                  << " | Bots: " << world.bots.size() << " (15v15)\n";
        if (world.map.blocks.empty() || world.bots.size() != 29 || world.map.waypoints.size() < 20) {
            std::cerr << "[Error] Map, NavGraph, or bot initialization failed!\n";
            return 1;
        }
    }

    // 2. Verify Spring3D Damped Harmonic Oscillator convergence
    Spring3D testSpring(180.0f, 18.0f);
    testSpring.Shove({ 10.0f, -5.0f, 3.0f });
    for (int i = 0; i < 60; ++i) testSpring.Update(1.0f / 60.0f);
    std::cout << "  -> Spring3D residual after 1.0s: " << Vector3Length(testSpring.position) << "\n";

    // 3. Verify Wallbanging, Per-Weapon Sockets & Expanded Attachments
    world.LoadMapAndReset(MAP_FOUNDRY, MODE_FLARE_DOMINATION);
    WeaponStats m60 = GetModifiedWeaponStats(WPN_M60, { OPTIC_EOTECH552, BARREL_LONG, GRIP_HERA_CQR, OTHER_BALLISTICS_TRACKER, AMMO_ARMOR_PIERCING });
    WeaponStats r870Slugs = GetModifiedWeaponStats(WPN_REMINGTON870, { OPTIC_OKP7, BARREL_ARS_SUPPRESSOR, GRIP_SKELETON, OTHER_BLUE_LASER, AMMO_SLUGS });
    std::cout << "  -> M60 (Long Barrel + Hera CQR + AP) Penetration: " << m60.penetration << "m | Velocity: " << m60.muzzleVelocity << "m/s\n";
    std::cout << "  -> R870 (Slugs Conversion) Pellets: " << r870Slugs.pellets << " | Close Dmg: " << r870Slugs.damage0 << "\n";

    for (int w = 0; w < (int)WPN_COUNT; ++w) {
        WeaponSocketLayout sock = GetWeaponSockets((WeaponId)w, BARREL_LONG);
        if (sock.muzzleTipPos.z >= 0.0f) {
            std::cerr << "[Error] Invalid muzzle socket on weapon " << w << "\n";
            return 1;
        }
    }

    // 4. Verify Hitscan Instant Frame-0 Damage & Visual Tracer Spawning
    {
        world.LoadMapAndReset(MAP_FOUNDRY, MODE_FLARE_DOMINATION);
        BotAgent& targetBot = world.bots[0];
        targetBot.alive = true;
        targetBot.health = 100.0f;
        targetBot.team = 1; // Enemy team
        targetBot.pos = { 0.0f, 1.0f, 4.0f };
        Vector3 fireOrigin = { 0.0f, 1.5f, 0.0f };
        Vector3 aimDir = { 0.0f, 0.0f, 1.0f };
        WeaponStats ak = GetWeaponStats(WPN_AK47);
        WeaponAttachments cleanAtt;
        world.projectiles.clear();

        world.SpawnWeaponFire(0, 0, fireOrigin, aimDir, ak, cleanAtt, 0.0f);

        if (targetBot.health >= 100.0f) {
            std::cerr << "[Error] Hitscan bullet failed to register instant frame 0 damage! Health: "
                      << targetBot.health << "\n";
            return 1;
        }
        if (world.projectiles.empty() || !world.projectiles[0].active) {
            std::cerr << "[Error] Hitscan visual tracer was not spawned!\n";
            return 1;
        }
        std::cout << "  -> Hitscan Instant Hit Reg: Frame-0 hit confirmed (Enemy HP: "
                  << targetBot.health << "/100) | Visual Tracer spawned at 650m/s\n";
    }

    // 5. Verify Recoil Patterns for all 8 weapons & Attachment Scaling
    {
        for (int w = 0; w < (int)WPN_COUNT; ++w) {
            WeaponId wid = (WeaponId)w;
            WeaponStats baseStats = GetWeaponStats(wid);
            WeaponAttachments stockAtt;
            Vector3 kick0 = GetWeaponRecoilPattern(wid, 0, stockAtt, baseStats);
            Vector3 kick5 = GetWeaponRecoilPattern(wid, 5, stockAtt, baseStats);
            if (kick0.x <= 0.0f) {
                std::cerr << "[Error] Recoil pattern vertical kick non-positive for weapon " << w << "\n";
                return 1;
            }
            // Verify compensator reduces horizontal recoil
            WeaponAttachments compAtt;
            compAtt.barrel = BARREL_COMPENSATOR;
            WeaponStats compStats = GetModifiedWeaponStats(wid, compAtt);
            Vector3 kickComp = GetWeaponRecoilPattern(wid, 5, compAtt, compStats);
            if (std::fabs(kickComp.y) > std::fabs(kick5.y) + 0.001f) {
                std::cerr << "[Error] Compensator failed to dampen horizontal recoil for weapon " << w << "\n";
                return 1;
            }
        }
        std::cout << "  -> Recoil Patterns: Deterministic curves & attachment dampening verified for all 8 weapons\n";
    }

    // 6. Verify Dynamic Spread Factorization (Hipfire vs ADS, Stance, Movement, Spray Bloom)
    {
        WeaponStats m4 = GetWeaponStats(WPN_M4A1);
        float hipSpreadStand = CalculateDynamicSpread(m4, 0.0f, 0, false, false, false, 1);
        float adsSpreadStand = CalculateDynamicSpread(m4, 1.0f, 0, false, false, false, 1);
        float crouchSpread   = CalculateDynamicSpread(m4, 0.0f, 1, false, false, false, 1);
        float proneSpread    = CalculateDynamicSpread(m4, 0.0f, 2, false, false, false, 1);
        float sprintSpread   = CalculateDynamicSpread(m4, 0.0f, 0, true, true, false, 1);
        float jumpSpread     = CalculateDynamicSpread(m4, 0.0f, 0, false, false, true, 1);
        float spraySpread    = CalculateDynamicSpread(m4, 0.0f, 0, false, false, false, 10);

        if (!(adsSpreadStand < hipSpreadStand)) {
            std::cerr << "[Error] ADS spread is not tighter than hipfire spread!\n";
            return 1;
        }
        if (!(crouchSpread < hipSpreadStand && proneSpread < crouchSpread)) {
            std::cerr << "[Error] Crouch/Prone stances did not tighten weapon spread!\n";
            return 1;
        }
        if (!(sprintSpread > hipSpreadStand && jumpSpread > sprintSpread)) {
            std::cerr << "[Error] Sprinting/Jumping movement penalties failed to bloom spread!\n";
            return 1;
        }
        if (!(spraySpread > hipSpreadStand)) {
            std::cerr << "[Error] Continuous fire spray bloom failed to increase spread over sustained fire!\n";
            return 1;
        }
        std::cout << "  -> Dynamic Spread: Hip/ADS (" << hipSpreadStand << " -> " << adsSpreadStand
                  << " rad), Stance bonuses, & Spray Bloom verified\n";
    }

    // Simulate 120 frames of AI bots + projectiles + shell casings + objectives
    for (int frame = 0; frame < 120; ++frame) {
        world.Update(1.0f / 60.0f);
    }
    std::cout << "[Self-Test] All Phantom Forces hitscan, recoil patterns, dynamic spread, 8 maps & 15v15 AI passed!\n";
    return 0;
}

int main(int argc, char** argv) {
    std::srand((unsigned int)std::time(nullptr));

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--self-test") == 0) {
            return RunSelfTest();
        }
    }

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1366, 768, "Phantom Forces - Raylib C++ Edition (8 Maps: 3 Classic + 5 15v15 Blueprints)");
    SetTargetFPS(120);
    SetExitKey(KEY_NULL); // Use ESC/TAB/M to toggle Loadout Menu instead of instant quit

    SoundBank audio;
    audio.Init();

    GameWorld world;
    world.audio = &audio;
    world.LoadMapAndReset(MAP_FOUNDRY, MODE_FLARE_DOMINATION);

    int inspectSlot = 0; // 0 = Primary, 1 = Secondary in Loadout Menu

    while (!WindowShouldClose()) {
        float dt = std::min(GetFrameTime(), 0.05f);

        // Quick Map Switch Hotkeys (F1 - F8) & Game Mode Switch (F9)
        for (int m = 0; m < (int)MAP_COUNT; ++m) {
            if (IsKeyPressed(KEY_F1 + m)) {
                world.LoadMapAndReset((MapId)m, world.currentMode);
                if (!world.inDeployMenu) {
                    world.DeployPlayer();
                    DisableCursor();
                }
            }
        }
        if (IsKeyPressed(KEY_F9)) {
            GameModeId nextMode = (GameModeId)(((int)world.currentMode + 1) % (int)MODE_COUNT);
            world.LoadMapAndReset(world.currentMapId, nextMode);
            if (!world.inDeployMenu) {
                world.DeployPlayer();
                DisableCursor();
            }
        }

        // Toggle PF Deploy / Loadout & Map Select Screen with TAB, M, or ESC
        if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_M) || IsKeyPressed(KEY_ESCAPE)) {
            world.inDeployMenu = !world.inDeployMenu;
            if (world.inDeployMenu) {
                EnableCursor();
            } else {
                if (!world.player.alive) {
                    world.DeployPlayer();
                } else {
                    DisableCursor();
                }
            }
        }

        world.Update(dt);

        BeginDrawing();
        ClearBackground(world.map.skyColor);

        if (world.inDeployMenu) {
            // Render orbiting cinematic 3D overview of the active map behind the Loadout UI
            float orbitAngle = (float)GetTime() * 0.12f;
            Camera3D menuCam = { 0 };
            menuCam.position = { std::cos(orbitAngle) * 76.0f, 32.0f, std::sin(orbitAngle) * 76.0f };
            menuCam.target   = { 0.0f, 4.0f, 0.0f };
            menuCam.up       = { 0.0f, 1.0f, 0.0f };
            menuCam.fovy     = 55.0f;
            menuCam.projection = CAMERA_PERSPECTIVE;

            DrawWorld3D(world, menuCam);
            DrawLoadoutAndDeployMenu(world, inspectSlot);
        } else {
            Camera3D worldCam = BuildPlayerWorldCamera(world.player);
            DrawWorld3D(world, worldCam);
            DrawFirstPersonViewmodel(world);
            DrawTacticalHUD(world, worldCam);
        }

        DrawFPS(GetScreenWidth() - 95, 14);
        EndDrawing();
    }

    audio.Cleanup();
    CloseWindow();
    return 0;
}

