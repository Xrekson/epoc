#pragma once
#include "game_world.hpp"
#include "rlgl.h"
#include <cstdio>

extern "C" void glClear(unsigned int mask);

// ============================================================================
// PHANTOM FORCES 3D RENDERER, PROCEDURAL VIEWMODEL PASS, TACTICAL HUD & DEPLOY UI
// ============================================================================

inline Camera3D BuildPlayerWorldCamera(const Player& p) {
    Camera3D cam = { 0 };
    cam.position = p.GetEyePos();
    cam.target   = Vector3Add(cam.position, p.GetForward());
    float rollAngle = p.camRecoilSpring.position.z * 0.015f;
    if (p.posture == POSTURE_SLIDE) rollAngle -= 0.065f;
    Vector3 right = p.GetRight();
    cam.up = Vector3Normalize({ right.x * std::sin(rollAngle), std::cos(rollAngle), right.z * std::sin(rollAngle) });
    cam.fovy = p.currentFov;
    cam.projection = CAMERA_PERSPECTIVE;
    return cam;
}

inline void DrawWorld3D(const GameWorld& world, const Camera3D& cam) {
    BeginMode3D(cam);

    // 1. Ground Plane + Tactical Grid Lines (Expanded for Large 15v15 Arenas)
    DrawPlane({ 0.0f, 0.0f, 0.0f }, { 420.0f, 320.0f }, world.map.groundColor);
    for (int i = -144; i <= 144; i += 8) {
        DrawLine3D({ (float)i, 0.01f, -112.0f }, { (float)i, 0.01f, 112.0f }, { 0, 0, 0, 28 });
    }
    for (int z = -112; z <= 112; z += 8) {
        DrawLine3D({ -144.0f, 0.01f, (float)z }, { 144.0f, 0.01f, (float)z }, { 0, 0, 0, 28 });
    }

    // 2. Map Geometry Blocks
    for (const auto& blk : world.map.blocks) {
        DrawPFBox(blk.pos, blk.size, blk.color);
    }

    // 3. Objective Beacons (Flare Domination Flares & King of the Hill Ring)
    auto drawObjective3D = [](const ObjectivePoint& pt) {
        Color col = { 210, 215, 225, 190 };
        if (pt.ownerTeam == 0) col = { 45, 135, 255, 210 };
        if (pt.ownerTeam == 1) col = { 255, 115, 35, 210 };
        DrawCylinderWires(pt.pos, pt.radius, pt.radius, 0.25f, 24, col);
        DrawCylinder({ pt.pos.x, pt.pos.y + 0.25f, pt.pos.z }, 0.18f, 0.18f, 0.5f, 12, { 60, 64, 70, 255 });
        Color beam = col;
        beam.a = 95;
        DrawCylinder({ pt.pos.x, pt.pos.y + 0.5f, pt.pos.z }, 0.14f, 0.35f, 6.5f, 10, beam);
    };

    if (world.currentMode == MODE_FLARE_DOMINATION) {
        for (const auto& fp : world.map.flarePoints) drawObjective3D(fp);
    } else if (world.currentMode == MODE_KING_OF_THE_HILL) {
        drawObjective3D(world.map.hillPoint);
    }

    // 4. Phantoms & Ghosts Blocky R6 Bots
    for (const auto& b : world.bots) {
        b.Draw();
    }

    // 5. Bullet Impact & Wallbang Exit Decals
    for (const auto& d : world.decals) {
        DrawCubeV(Vector3Add(d.pos, Vector3Scale(d.normal, 0.012f)), { 0.055f, 0.055f, 0.055f }, d.color);
    }

    // 6. 3D Spinning Ejected Brass Casings / Shotgun Shells
    for (const auto& sc : world.shells) {
        if (!sc.active) continue;
        rlPushMatrix();
        rlTranslatef(sc.pos.x, sc.pos.y, sc.pos.z);
        rlRotatef(sc.rotDeg.x, 1.0f, 0.0f, 0.0f);
        rlRotatef(sc.rotDeg.y, 0.0f, 1.0f, 0.0f);
        rlRotatef(sc.rotDeg.z, 0.0f, 0.0f, 1.0f);
        Vector3 sz = sc.isShotgunShell ? Vector3{ 0.018f, 0.018f, 0.055f } : Vector3{ 0.010f, 0.010f, 0.036f };
        DrawPFBox({ 0, 0, 0 }, sz, sc.color);
        rlPopMatrix();
    }

    // 7. Active Ballistic Projectiles & Very Short Glowing Tracer Shots (Every Shot = Compact Tracer)
    for (const auto& p : world.projectiles) {
        if (!p.active) continue;
        Vector3 dir = Vector3Normalize(p.vel);
        // Clamp tracer length to a very short 0.42m streak and prevent clipping behind the muzzle
        float availDist = std::max(0.06f, p.distanceTraveled - 0.22f);
        float tracerLen = std::min(0.42f, availDist);
        Vector3 head = (p.distanceTraveled < 0.22f)
            ? Vector3Add(p.startPos, Vector3Scale(dir, 0.28f))
            : p.pos;
        Vector3 tail = Vector3Subtract(head, Vector3Scale(dir, tracerLen));
        Vector3 coreTail = Vector3Subtract(head, Vector3Scale(dir, tracerLen * 0.55f));

        // Ultra-thin tapered outer tracer glow (2.2mm tail -> 5.0mm head) + hot white core + tiny 5.5mm bullet tip
        DrawCylinderEx(tail, head, 0.0022f, 0.0050f, 6, p.tracerColor);
        DrawLine3D(coreTail, head, { 255, 255, 240, 255 });
        DrawSphereEx(head, 0.0055f, 4, 4, { 255, 252, 230, 255 });
    }

    // 8. M67 Frag Grenades & Explosions
    for (const auto& g : world.grenades) {
        if (!g.active) continue;
        if (g.explosionFlashTimer > 0.0f) {
            float r = (0.35f - g.explosionFlashTimer) * 28.0f;
            DrawSphere(g.pos, r, { 255, 165, 45, 140 });
            DrawSphereWires(g.pos, r * 1.15f, 10, 10, { 255, 225, 110, 200 });
        } else {
            DrawSphere(g.pos, 0.14f, { 52, 82, 48, 255 });
            DrawCubeV({ g.pos.x, g.pos.y + 0.14f, g.pos.z }, { 0.06f, 0.08f, 0.06f }, { 255, 60, 40, 255 });
        }
    }

    EndMode3D();
}

// ============================================================================
// FIRST-PERSON PROCEDURAL SPRING VIEWMODEL RENDER PASS
// Rendered in View Space with a cleared Depth Buffer so the gun never clips walls
// ============================================================================
inline void DrawFirstPersonViewmodel(const GameWorld& world) {
    const Player& p = world.player;
    if (!p.alive || world.inDeployMenu) return;

    WeaponStats stats = p.GetActiveStats();
    const WeaponAttachments& att = p.GetActiveAttachments();

    OpticType effOptic = att.optic;
    if (stats.id == WPN_INTERVENTION && effOptic == OPTIC_IRON) effOptic = OPTIC_PMII_SCOPE;
    if (effOptic == OPTIC_PMII_SCOPE && !p.useCantedSight && p.adsAlpha > 0.88f) {
        return;
    }

    rlDrawRenderBatchActive();
    glClear(0x00000100); // GL_DEPTH_BUFFER_BIT

    Camera3D vmCam = { 0 };
    vmCam.position = { 0.0f, 0.0f, 0.0f };
    vmCam.target   = { 0.0f, 0.0f, -1.0f };
    vmCam.up       = { 0.0f, 1.0f, 0.0f };
    vmCam.fovy     = 62.0f;
    vmCam.projection = CAMERA_PERSPECTIVE;

    BeginMode3D(vmCam);

    // 1. Compute Hipfire vs. Exact ADS Optic Alignment Position
    float sightHeight = GetOpticSightHeight(stats.id, effOptic);
    WeaponSocketLayout sock = GetWeaponSockets(stats.id, att.barrel);
    Vector3 hipPos = { 0.165f, -0.145f, -0.34f };
    Vector3 adsPos = { 0.0f,   -sightHeight, -0.24f };
    if (p.useCantedSight && att.other == OTHER_CANTED_DELTA) {
        float cx = 0.044f;
        float cy = sock.opticMountPos.y + 0.015f;
        float rad = 42.0f * DEG2RAD;
        float rx = cx * std::cos(rad) - cy * std::sin(rad);
        float ry = cx * std::sin(rad) + cy * std::cos(rad);
        adsPos = { -rx, -ry, -0.22f };
    }

    Vector3 basePos = Vector3Lerp(hipPos, adsPos, p.adsAlpha);

    // 2. Add Procedural Walk Cycle Bob & Breath Sway
    float bobAmp = (1.0f - 0.82f * p.adsAlpha) * (p.sprinting ? 0.018f : 0.008f);
    float bobX = std::cos(p.walkCycleTime * 0.5f) * bobAmp;
    float bobY = std::fabs(std::sin(p.walkCycleTime)) * bobAmp;

    float breathScale = p.holdingBreath ? 0.05f : 1.0f;
    float idleSwayX = std::sin((float)GetTime() * 1.6f) * 0.0018f * breathScale;
    float idleSwayY = std::cos((float)GetTime() * 3.2f) * 0.0018f * breathScale;

    Vector3 finalPos = Vector3Add(basePos, { bobX + idleSwayX, -bobY + idleSwayY, 0.0f });
    finalPos = Vector3Add(finalPos, p.gunPosSpring.position);
    finalPos = Vector3Add(finalPos, Vector3Scale(p.swaySpring.position, 1.0f - 0.65f * p.adsAlpha));

    // 3. Compute Procedural Rotations (Sprint Tuck, Canted Roll, Slide Tilt, Inspect 'H')
    Vector3 rotDeg = p.gunRotSpring.position;
    if (p.sprinting && p.adsAlpha < 0.2f) {
        rotDeg.x -= 14.0f;
        rotDeg.y += 28.0f;
        rotDeg.z -= 12.0f;
        finalPos.y -= 0.035f;
    }
    if (p.posture == POSTURE_SLIDE) {
        rotDeg.z -= 18.0f;
    }
    if (p.useCantedSight && att.other == OTHER_CANTED_DELTA) {
        rotDeg.z += 42.0f * p.adsAlpha;
    }

    // PF Weapon Inspect Animation ('H' key)
    if (p.inspectTimer > 0.0f) {
        float t = 1.0f - (p.inspectTimer / p.inspectDuration); // 0 -> 1
        float env = std::sin(t * PI);
        finalPos.x -= 0.085f * env;
        finalPos.y += 0.045f * env;
        finalPos.z -= 0.040f * env;
        // Tilt left to show right receiver/ejection port, then tilt right to show left side!
        float sideSwing = std::sin(t * PI * 2.0f);
        rotDeg.y += 38.0f * sideSwing * env;
        rotDeg.z += 32.0f * sideSwing * env;
        rotDeg.x += 12.0f * env;
    }

    float reloadFrac = p.isReloading ? std::clamp(p.reloadTimer / std::max(0.1f, p.reloadDuration), 0.0f, 1.0f) : 0.0f;

    rlPushMatrix();
    rlTranslatef(finalPos.x, finalPos.y, finalPos.z);
    rlRotatef(rotDeg.z, 0.0f, 0.0f, 1.0f);
    rlRotatef(rotDeg.y, 0.0f, 1.0f, 0.0f);
    rlRotatef(rotDeg.x, 1.0f, 0.0f, 0.0f);

    if (p.meleeSwingTimer > 0.0f) {
        // Upgraded Tactical Combat Knife with Guard, Fuller & Serrated Spine
        DrawPFBox({ 0.0f, -0.025f, -0.04f }, { 0.026f, 0.036f, 0.12f }, { 28, 30, 34, 255 });
        DrawPFBox({ 0.0f, -0.010f, -0.10f }, { 0.016f, 0.068f, 0.012f }, { 55, 60, 68, 255 }); // Crossguard
        DrawPFBox({ 0.0f,  0.002f, -0.19f }, { 0.007f, 0.032f, 0.17f }, { 205, 214, 224, 255 }); // Blade
        DrawPFAngledBox({ 0.0f, 0.010f, -0.27f }, { 0.007f, 0.022f, 0.045f }, -25.0f, 0, 0, { 220, 228, 236, 255 });
    } else {
        DrawProceduralWeaponModel(stats, att, reloadFrac, p.boltAnimTimer, p.muzzleFlashTimer, true, p.adsAlpha);
    }

    rlPopMatrix();
    EndMode3D();
}

// ============================================================================
// PHANTOM FORCES TACTICAL HUD (MINIMAP RADAR, HITMARKERS, BALLISTICS TRACKER)
// ============================================================================
inline void DrawTacticalHUD(const GameWorld& world, const Camera3D& cam) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    const Player& p = world.player;
    WeaponStats stats = p.GetActiveStats();
    const WeaponAttachments& att = p.GetActiveAttachments();

    // 1. 3D Overhead Markers (Teammates, Spotted Enemies & Ballistics Tracker Drop Diamonds!)
    Vector3 camFwd = p.GetForward();
    bool hasBallisticsTracker = (att.other == OTHER_BALLISTICS_TRACKER) && (p.adsAlpha > 0.55f);

    for (const auto& b : world.bots) {
        if (!b.alive) continue;
        Vector3 headWorld = { b.pos.x, b.pos.y + (b.crouching ? 1.28f : 1.68f), b.pos.z };
        Vector3 toHead = Vector3Subtract(headWorld, cam.position);
        float dist = Vector3Length(toHead);
        if (Vector3DotProduct(Vector3Normalize(toHead), camFwd) <= 0.1f) continue;

        Vector2 scr = GetWorldToScreen({ headWorld.x, headWorld.y + 0.45f, headWorld.z }, cam);
        if (b.team == 0) {
            DrawTriangle({ scr.x, scr.y + 6 }, { scr.x + 6, scr.y - 4 }, { scr.x - 6, scr.y - 4 }, { 65, 165, 255, 220 });
            if (dist < 28.0f) {
                int tw = MeasureText(b.name.c_str(), 12);
                DrawText(b.name.c_str(), (int)scr.x - tw / 2, (int)scr.y - 18, 12, { 135, 205, 255, 220 });
            }
        } else {
            if (b.spottedTimer > 0.0f) {
                DrawPoly(scr, 4, 7.5f, 45.0f, { 255, 55, 45, 235 });
                DrawPolyLinesEx(scr, 4, 8.5f, 45.0f, 1.5f, WHITE);
            }
            // PHANTOM FORCES BALLISTICS TRACKER: Compute exact bullet drop compensation above enemy head!
            if (hasBallisticsTracker && world.HasLineOfSight(cam.position, headWorld)) {
                float flightTime = dist / std::max(10.0f, stats.muzzleVelocity);
                float bulletDrop = 0.5f * 9.81f * flightTime * flightTime;
                Vector3 aimPointWorld = { headWorld.x, headWorld.y + bulletDrop, headWorld.z };
                Vector2 btScr = GetWorldToScreen(aimPointWorld, cam);
                DrawPoly(btScr, 4, 6.0f, 45.0f, { 255, 230, 45, 245 });
                DrawPolyLinesEx(btScr, 4, 7.5f, 45.0f, 1.5f, { 25, 25, 25, 255 });
            }
        }
    }

    // Objective 3D HUD Icons
    auto drawObjHud = [&](const ObjectivePoint& pt) {
        Vector3 worldPos = { pt.pos.x, pt.pos.y + 2.4f, pt.pos.z };
        Vector3 toObj = Vector3Subtract(worldPos, cam.position);
        if (Vector3DotProduct(Vector3Normalize(toObj), camFwd) <= 0.1f) return;
        Vector2 scr = GetWorldToScreen(worldPos, cam);
        Color c = (pt.ownerTeam == 0) ? Color{ 45, 145, 255, 230 }
                : (pt.ownerTeam == 1) ? Color{ 255, 115, 35, 230 }
                : Color{ 200, 205, 215, 210 };
        DrawPoly(scr, 4, 15.0f, 45.0f, { 20, 24, 30, 200 });
        DrawPolyLinesEx(scr, 4, 15.0f, 45.0f, 2.5f, c);
        int tw = MeasureText(pt.label, 14);
        DrawText(pt.label, (int)scr.x - tw / 2, (int)scr.y - 7, 14, WHITE);
    };
    if (world.currentMode == MODE_FLARE_DOMINATION) {
        for (const auto& fp : world.map.flarePoints) drawObjHud(fp);
    } else if (world.currentMode == MODE_KING_OF_THE_HILL) {
        drawObjHud(world.map.hillPoint);
    }

    // 2. Sniper Ballistic Scope Overlay (when fully ADS with PM II Scope)
    OpticType effOptic = att.optic;
    if (stats.id == WPN_INTERVENTION && effOptic == OPTIC_IRON) effOptic = OPTIC_PMII_SCOPE;
    if (effOptic == OPTIC_PMII_SCOPE && !p.useCantedSight && p.adsAlpha > 0.88f) {
        int cx = sw / 2;
        int cy = sh / 2;
        float radius = sh * 0.465f; // Generous, wide viewing aperture with lean outer black mask
        // Outer matte masking with smooth transition
        DrawRectangle(0, 0, cx - (int)radius, sh, { 10, 12, 15, 252 });
        DrawRectangle(cx + (int)radius, 0, sw - (cx + (int)radius), sh, { 10, 12, 15, 252 });
        DrawRing({ (float)cx, (float)cy }, radius, (float)std::max(sw, sh) * 0.9f, 0.0f, 360.0f, 64, { 10, 12, 15, 252 });
        // Sleek lean scope bezel: dual ultra-thin metallic border rings + subtle shadow
        DrawRing({ (float)cx, (float)cy }, radius - 4.0f, radius, 0.0f, 360.0f, 64, { 18, 22, 28, 220 });
        DrawCircleLines(cx, cy, radius, { 65, 72, 85, 255 });
        DrawCircleLines(cx, cy, radius - 4.0f, { 25, 28, 35, 180 });

        // Ultra-Fine Precision Ballistic Reticle (Hairline Crosshairs, Unobstructed Center)
        Color reticleCol = { 20, 22, 26, 235 };
        Color milCol     = { 30, 34, 40, 220 };

        // Outer Duplex Thicker Framing Posts (starting at 90px from center, extending out to rim)
        int duplexStart = 90;
        DrawLineEx({ (float)(cx - (int)radius), (float)cy }, { (float)(cx - duplexStart), (float)cy }, 2.5f, reticleCol);
        DrawLineEx({ (float)(cx + duplexStart), (float)cy }, { (float)(cx + (int)radius), (float)cy }, 2.5f, reticleCol);
        DrawLineEx({ (float)cx, (float)(cy - (int)radius) }, { (float)cx, (float)(cy - duplexStart) }, 2.5f, reticleCol);
        DrawLineEx({ (float)cx, (float)(cy + duplexStart) }, { (float)cx, (float)(cy + (int)radius) }, 2.5f, reticleCol);

        // Thin Hairline Crosshairs (from 5px away from center to duplex posts - center remains 100% unobstructed!)
        int centerGap = 5;
        DrawLine(cx - duplexStart, cy, cx - centerGap, cy, reticleCol);
        DrawLine(cx + centerGap, cy, cx + duplexStart, cy, reticleCol);
        DrawLine(cx, cy - duplexStart, cx, cy - centerGap, reticleCol);
        DrawLine(cx, cy + centerGap, cx, cy + duplexStart, reticleCol);

        // Precision Mil-Dot / Hash Elevation Marks on Lower Vertical Post
        for (int m = 1; m <= 6; ++m) {
            int ty = cy + m * 24;
            if (ty < cy + (int)radius - 10) {
                int tickLen = (m % 2 == 0) ? 6 : 3;
                DrawLine(cx - tickLen, ty, cx + tickLen, ty, milCol);
                // Clean numbered distance indicators on alternating mil hashes (2, 4, 6)
                if (m % 2 == 0) {
                    DrawText(TextFormat("%d", m), cx + tickLen + 4, ty - 5, 10, { 45, 52, 62, 200 });
                }
            }
        }
        // Horizontal Windage Mil Hash Marks (subtle 2-4px ticks at +/- 1 to 4 mils)
        for (int w = 1; w <= 4; ++w) {
            int txL = cx - w * 22;
            int txR = cx + w * 22;
            int tickH = (w % 2 == 0) ? 4 : 2;
            DrawLine(txL, cy - tickH, txL, cy + tickH, milCol);
            DrawLine(txR, cy - tickH, txR, cy + tickH, milCol);
        }

        // Pinpoint Floating 1.2px Center Micro-Dot (instead of bulky 5px blob!)
        DrawCircle(cx, cy, 1.2f, { 255, 45, 45, 255 });

        // Steady Breath indicator text with clean tactical typography
        int textW = MeasureText(TextFormat("HOLD [L-SHIFT] STEADY BREATH (%.1fs)", p.steadyBreathStamina), 14);
        DrawText(TextFormat("HOLD [L-SHIFT] STEADY BREATH (%.1fs)", p.steadyBreathStamina),
                 cx - textW / 2, cy + (int)radius + 14, 14, { 200, 215, 230, 220 });
    }

    // 3. Dynamic Crosshair & PF Hitmarker Cross
    int cx = sw / 2;
    int cy = sh / 2;
    if (p.adsAlpha < 0.55f) {
        float gap = 8.0f + stats.hipSpread * 240.0f + Vector2Length({ p.vel.x, p.vel.z }) * 1.2f;
        int g = (int)gap;
        int len = 9;
        Color chCol = { 235, 245, 255, (unsigned char)((1.0f - p.adsAlpha) * 210) };
        DrawLine(cx - g - len, cy, cx - g, cy, chCol);
        DrawLine(cx + g, cy, cx + g + len, cy, chCol);
        DrawLine(cx, cy - g - len, cx, cy - g, chCol);
        DrawLine(cx, cy + g, cx, cy + g + len, chCol);
    }

    if (p.hitmarkerTimer > 0.0f) {
        Color hmCol = (p.hitmarkerHeadshot || p.hitmarkerKill) ? Color{ 255, 50, 50, 255 } : WHITE;
        int d1 = 5, d2 = 13;
        DrawLineEx({ (float)(cx - d2), (float)(cy - d2) }, { (float)(cx - d1), (float)(cy - d1) }, 2.2f, hmCol);
        DrawLineEx({ (float)(cx + d1), (float)(cy - d1) }, { (float)(cx + d2), (float)(cy - d2) }, 2.2f, hmCol);
        DrawLineEx({ (float)(cx - d2), (float)(cy + d2) }, { (float)(cx - d1), (float)(cy + d1) }, 2.2f, hmCol);
        DrawLineEx({ (float)(cx + d1), (float)(cy + d1) }, { (float)(cx + d2), (float)(cy + d2) }, 2.2f, hmCol);
    }

    if (p.damageIndicatorTimer > 0.0f) {
        unsigned char a = (unsigned char)std::min(140.0f, p.damageIndicatorTimer * 110.0f);
        DrawRectangleLinesEx({ 0, 0, (float)sw, (float)sh }, 18.0f, { 225, 35, 35, a });
    }

    // 4. Bottom-Left Phantom Forces Live Radar Minimap
    int mapX = 24;
    int mapY = sh - 194;
    int mapSize = 170;
    DrawRectangle(mapX, mapY, mapSize, mapSize, { 16, 20, 26, 215 });
    DrawRectangleLinesEx({ (float)mapX, (float)mapY, (float)mapSize, (float)mapSize }, 2.0f, { 70, 82, 98, 255 });

    BeginScissorMode(mapX + 2, mapY + 2, mapSize - 4, mapSize - 4);
    Vector2 radarCenter = { mapX + mapSize * 0.5f, mapY + mapSize * 0.5f };
    float radarScale = 1.85f;
    float cosY = std::cos(-p.yaw);
    float sinY = std::sin(-p.yaw);

    auto worldToRadar = [&](Vector3 wPos) -> Vector2 {
        float dx = wPos.x - p.pos.x;
        float dz = wPos.z - p.pos.z;
        float rx = dx * cosY - dz * sinY;
        float rz = dx * sinY + dz * cosY;
        return { radarCenter.x + rx * radarScale, radarCenter.y + rz * radarScale };
    };

    for (const auto& b : world.bots) {
        if (!b.alive) continue;
        if (b.team == 0 || b.spottedTimer > 0.0f || b.radarPingTimer > 0.0f) {
            Vector2 bp = worldToRadar(b.pos);
            Color dotCol = (b.team == 0) ? Color{ 55, 165, 255, 255 } : Color{ 255, 65, 55, 255 };
            DrawCircleV(bp, 4.2f, dotCol);
        }
    }

    if (world.currentMode == MODE_FLARE_DOMINATION) {
        for (const auto& fp : world.map.flarePoints) {
            Vector2 op = worldToRadar(fp.pos);
            DrawCircleLines((int)op.x, (int)op.y, 5.5f, YELLOW);
        }
    }

    DrawLineV(radarCenter, { radarCenter.x - 24, radarCenter.y - 42 }, { 255, 255, 255, 60 });
    DrawLineV(radarCenter, { radarCenter.x + 24, radarCenter.y - 42 }, { 255, 255, 255, 60 });
    DrawCircleV(radarCenter, 4.5f, WHITE);
    EndScissorMode();
    DrawText(world.map.name, mapX + 6, mapY + 5, 12, { 165, 180, 195, 230 });

    // 5. Top-Center Scoreboard & Match Timer Banner
    int sbW = 360;
    int sbX = cx - sbW / 2;
    DrawRectangle(sbX, 12, sbW, 48, { 18, 22, 28, 215 });
    DrawRectangleLinesEx({ (float)sbX, 12.0f, (float)sbW, 48.0f }, 1.5f, { 68, 78, 92, 255 });

    DrawRectangle(sbX + 6, 18, 118, 36, { 34, 88, 175, 230 });
    DrawText("PHANTOMS", sbX + 14, 21, 12, { 200, 225, 255, 255 });
    DrawText(TextFormat("%d", world.teamScore[0]), sbX + 82, 24, 24, WHITE);

    int mins = (int)world.matchTimer / 60;
    int secs = (int)world.matchTimer % 60;
    const char* modeShort = (world.currentMode == MODE_FLARE_DOMINATION) ? "FLARE DOM"
                          : (world.currentMode == MODE_KING_OF_THE_HILL) ? "KOTH" : "TDM";
    DrawText(modeShort, cx - MeasureText(modeShort, 12) / 2, 17, 12, { 185, 195, 210, 255 });
    DrawText(TextFormat("%02d:%02d", mins, secs), cx - 26, 32, 20, WHITE);

    DrawRectangle(sbX + sbW - 124, 18, 118, 36, { 205, 96, 32, 230 });
    DrawText("GHOSTS", sbX + sbW - 114, 21, 12, { 255, 230, 200, 255 });
    DrawText(TextFormat("%d", world.teamScore[1]), sbX + sbW - 52, 24, 24, WHITE);

    // 6. Bottom-Right Authentic Phantom Forces Weapon, Ammo, Firemode & Health Panel
    int hudW = 290;
    int hudH = 112;
    int hudX = sw - hudW - 24;
    int hudY = sh - hudH - 24;
    DrawRectangle(hudX, hudY, hudW, hudH, { 16, 20, 26, 215 });
    DrawRectangleLinesEx({ (float)hudX, (float)hudY, (float)hudW, (float)hudH }, 2.0f, { 68, 80, 96, 255 });

    int curMag = p.magAmmo[p.activeSlot];
    int curRes = p.reserveAmmo[p.activeSlot];
    Color ammoCol = (curMag <= stats.magSize / 4) ? Color{ 255, 75, 65, 255 } : WHITE;
    DrawText(TextFormat("%d", curMag), hudX + 16, hudY + 12, 42, ammoCol);
    DrawText(TextFormat("/ %d", curRes), hudX + 78, hudY + 26, 22, { 165, 175, 190, 255 });

    FireMode fm = p.currentFireMode[p.activeSlot];
    const char* fmStr = (fm == FIRE_AUTO) ? "[AUTO]" : (fm == FIRE_BURST) ? "[3x BURST]"
                      : (fm == FIRE_SEMI) ? "[SEMI]" : "[BOLT/PUMP]";
    DrawText(stats.name, hudX + 155, hudY + 14, 18, { 110, 195, 255, 255 });
    DrawText(fmStr, hudX + 155, hudY + 35, 14, { 200, 210, 225, 255 });

    const char* postStr = (p.posture == POSTURE_STAND)  ? (p.sprinting ? "SPRINT" : "STAND")
                        : (p.posture == POSTURE_CROUCH) ? "CROUCH"
                        : (p.posture == POSTURE_PRONE)  ? "PRONE"
                        : (p.posture == POSTURE_SLIDE)  ? "SLIDE" : "DOLPHIN DIVE";
    DrawText(TextFormat("G:%d | %s", p.grenadesLeft, postStr), hudX + 16, hudY + 58, 14, { 185, 205, 225, 240 });

    if (p.isReloading) {
        DrawText("RELOADING...", cx - 55, cy + 45, 18, { 255, 215, 75, 255 });
    }

    float hpFrac = std::clamp(p.health / 100.0f, 0.0f, 1.0f);
    Color hpCol = (hpFrac > 0.5f) ? Color{ 65, 220, 110, 255 }
                : (hpFrac > 0.25f) ? Color{ 245, 185, 50, 255 }
                : Color{ 245, 55, 55, 255 };
    DrawRectangle(hudX + 14, hudY + 82, hudW - 28, 18, { 35, 40, 48, 255 });
    DrawRectangle(hudX + 14, hudY + 82, (int)((hudW - 28) * hpFrac), 18, hpCol);
    DrawText(TextFormat("HP %d", (int)std::ceil(p.health)), hudX + 22, hudY + 84, 14, { 15, 20, 25, 255 });

    // 7. Top-Left Killfeed
    int kfY = 20;
    for (const auto& kf : world.killFeed) {
        Color kCol = (kf.killerTeam == 0) ? Color{ 95, 185, 255, 255 } : Color{ 255, 145, 65, 255 };
        Color vCol = (kf.victimTeam == 0) ? Color{ 95, 185, 255, 255 } : Color{ 255, 145, 65, 255 };
        std::string tag = std::string("[") + kf.weaponName + (kf.headshot ? " HS" : "") + (kf.wallbang ? " WB" : "") + "]";

        DrawRectangle(20, kfY, 310, 22, { 15, 18, 24, 185 });
        int xCursor = 26;
        DrawText(kf.killer.c_str(), xCursor, kfY + 4, 14, kCol);
        xCursor += MeasureText(kf.killer.c_str(), 14) + 8;
        DrawText(tag.c_str(), xCursor, kfY + 4, 14, { 225, 230, 240, 255 });
        xCursor += MeasureText(tag.c_str(), 14) + 8;
        DrawText(kf.victim.c_str(), xCursor, kfY + 4, 14, vCol);
        kfY += 25;
    }

    // 8. Bottom-Center Phantom Forces XP Feed
    int xpY = sh - 155;
    for (const auto& xp : world.xpPopups) {
        int tw = MeasureText(xp.label.c_str(), 18);
        DrawText(xp.label.c_str(), cx - tw / 2, xpY, 18, { 115, 225, 255, 245 });
        xpY -= 22;
    }

    DrawText("[TAB/M] Loadout | [H] Inspect Gun | [Shift+C] Slide | [Space+Z] Superjump | [E] Spot | [F] Knife | [G] Frag | [V] Firemode | [T] Canted",
             sw - 835, sh - 16, 12, { 180, 195, 215, 190 });
}

// ============================================================================
// PHANTOM FORCES PRE-SPAWN LOADOUT, ATTACHMENT CUSTOMIZER & MAP SELECT MENU
// ============================================================================
inline bool DrawMenuButton(Rectangle rect, const char* text, bool selected, Color accent = { 55, 155, 255, 255 }) {
    Vector2 mp = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mp, rect);
    Color bg = selected ? Color{ 38, 62, 96, 240 } : (hovered ? Color{ 34, 42, 54, 235 } : Color{ 22, 26, 34, 225 });
    DrawRectangleRec(rect, bg);
    DrawRectangleLinesEx(rect, selected ? 2.0f : 1.0f, selected ? accent : Color{ 68, 78, 94, 220 });
    DrawText(text, (int)rect.x + 10, (int)(rect.y + (rect.height - 14) * 0.5f), 14, selected ? WHITE : Color{ 200, 210, 225, 255 });
    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

inline void DrawLoadoutAndDeployMenu(GameWorld& world, int& inspectSlot) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    Player& p = world.player;

    // Interactive Mouse Drag Orbit & Scroll Zoom for the 3D Weapon Showcase!
    static float previewYaw = 45.0f;
    static float previewPitch = 10.0f;
    static float previewZoom = 0.68f;
    static bool autoRotate = true;

    Vector2 mp = GetMousePosition();
    if (mp.x > 340 && mp.x < sw - 360) {
        float wheel = GetMouseWheelMove();
        if (std::fabs(wheel) > 0.01f) {
            previewZoom = std::clamp(previewZoom - wheel * 0.06f, 0.34f, 1.15f);
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            autoRotate = false;
            Vector2 md = GetMouseDelta();
            previewYaw   += md.x * 0.45f;
            previewPitch = std::clamp(previewPitch + md.y * 0.35f, -45.0f, 45.0f);
        }
    }
    if (autoRotate) {
        previewYaw = (float)GetTime() * 28.0f;
    }

    rlDrawRenderBatchActive();
    glClear(0x00000100); // Clear Depth Buffer for crisp 3D weapon preview

    Camera3D previewCam = { 0 };
    previewCam.position = { 0.0f, 0.03f, previewZoom };
    previewCam.target   = { -0.05f, 0.0f, 0.0f };
    previewCam.up       = { 0.0f, 1.0f, 0.0f };
    previewCam.fovy     = 45.0f;
    previewCam.projection = CAMERA_PERSPECTIVE;

    WeaponId previewId = (inspectSlot == 0) ? p.primaryId : p.secondaryId;
    WeaponAttachments& previewAtt = (inspectSlot == 0) ? p.primaryAtt : p.secondaryAtt;
    WeaponStats modStats = GetModifiedWeaponStats(previewId, previewAtt);

    BeginMode3D(previewCam);
    rlPushMatrix();
    rlTranslatef(0.02f, 0.01f, 0.0f);
    rlRotatef(previewPitch, 1.0f, 0.0f, 0.0f);
    rlRotatef(previewYaw, 0.0f, 1.0f, 0.0f);
    DrawProceduralWeaponModel(modStats, previewAtt, 0.0f, 0.0f, 0.0f, false, 0.0f);
    rlPopMatrix();
    EndMode3D();

    DrawText("DRAG MOUSE TO ORBIT 3D GUN  |  SCROLL TO ZOOM", sw / 2 - 165, sh - 60, 13, { 175, 195, 220, 210 });

    // 2. Dark Semi-Transparent Overlay Panel on Left & Right
    DrawRectangle(0, 0, sw, 68, { 12, 16, 22, 240 });
    DrawLine(0, 68, sw, 68, { 55, 155, 255, 200 });
    DrawText("PHANTOM FORCES // RAYLIB C++ EDITION", 24, 16, 26, WHITE);
    DrawText(TextFormat("RANK %d  |  SCORE: %d  |  K: %d  D: %d", p.rank, p.score, p.kills, p.deaths),
             26, 44, 14, { 115, 195, 255, 255 });

    // DEPLOY BUTTON (Spacebar or Click)
    Rectangle deployBtn = { 24.0f, 84.0f, 300.0f, 48.0f };
    if (DrawMenuButton(deployBtn, ">> DEPLOY TO COMBAT [SPACE] <<", true, { 65, 225, 120, 255 }) ||
        IsKeyPressed(KEY_SPACE)) {
        world.DeployPlayer();
        return;
    }

    // Slot Tabs: PRIMARY vs SECONDARY
    if (DrawMenuButton({ 24.0f, 144.0f, 145.0f, 32.0f }, "PRIMARY WEAPON", inspectSlot == 0)) inspectSlot = 0;
    if (DrawMenuButton({ 179.0f, 144.0f, 145.0f, 32.0f }, "SECONDARY SIDEARM", inspectSlot == 1)) inspectSlot = 1;

    // Weapon List
    float y = 186.0f;
    DrawText("SELECT WEAPON:", 24, (int)y, 13, { 150, 170, 195, 255 });
    y += 18.0f;
    const auto& db = GetBaseWeaponDatabase();
    for (const auto& w : db) {
        if ((inspectSlot == 0 && !w.isPrimary) || (inspectSlot == 1 && w.isPrimary)) continue;
        bool sel = (previewId == w.id);
        if (DrawMenuButton({ 24.0f, y, 300.0f, 28.0f }, TextFormat("%s  [%s]", w.name, w.category), sel)) {
            if (inspectSlot == 0) p.primaryId = w.id;
            else p.secondaryId = w.id;
            p.ResetLoadoutAmmo();
        }
        y += 32.0f;
    }

    // 5 Attachment Customization Slots (Click to cycle through all 31 attachments!)
    y += 8.0f;
    DrawText("CUSTOMIZE ATTACHMENTS (CLICK TO CYCLE):", 24, (int)y, 13, { 150, 170, 195, 255 });
    y += 18.0f;

    if (DrawMenuButton({ 24.0f, y, 300.0f, 28.0f }, TextFormat("OPTICS: %s", GetOpticName(previewAtt.optic)), previewAtt.optic != OPTIC_IRON)) {
        previewAtt.optic = (OpticType)(((int)previewAtt.optic + 1) % (int)OPTIC_COUNT);
        p.ResetLoadoutAmmo();
    }
    y += 32.0f;
    if (DrawMenuButton({ 24.0f, y, 300.0f, 28.0f }, TextFormat("BARREL: %s", GetBarrelName(previewAtt.barrel)), previewAtt.barrel != BARREL_NONE)) {
        previewAtt.barrel = (BarrelType)(((int)previewAtt.barrel + 1) % (int)BARREL_COUNT);
        p.ResetLoadoutAmmo();
    }
    y += 32.0f;
    if (DrawMenuButton({ 24.0f, y, 300.0f, 28.0f }, TextFormat("UNDERBARREL: %s", GetGripName(previewAtt.grip)), previewAtt.grip != GRIP_NONE)) {
        previewAtt.grip = (UnderbarrelType)(((int)previewAtt.grip + 1) % (int)GRIP_COUNT);
        p.ResetLoadoutAmmo();
    }
    y += 32.0f;
    if (DrawMenuButton({ 24.0f, y, 300.0f, 28.0f }, TextFormat("OTHER: %s", GetOtherName(previewAtt.other)), previewAtt.other != OTHER_NONE)) {
        previewAtt.other = (OtherAttachType)(((int)previewAtt.other + 1) % (int)OTHER_COUNT);
        p.ResetLoadoutAmmo();
    }
    y += 32.0f;
    if (DrawMenuButton({ 24.0f, y, 300.0f, 28.0f }, TextFormat("AMMO: %s", GetAmmoName(previewAtt.ammo)), previewAtt.ammo != AMMO_STANDARD)) {
        previewAtt.ammo = (AmmoType)(((int)previewAtt.ammo + 1) % (int)AMMO_COUNT);
        p.ResetLoadoutAmmo();
    }

    // Game Mode Selector at Bottom-Left
    y += 36.0f;
    DrawText("GAME MODE SELECTION:", 24, (int)y, 13, { 150, 170, 195, 255 });
    y += 18.0f;
    const char* modeNames[3] = { "TDM", "FLARE DOM", "KOTH" };
    for (int md = 0; md < 3; ++md) {
        if (DrawMenuButton({ 24.0f + md * 102.0f, y, 96.0f, 28.0f }, modeNames[md], (int)world.currentMode == md, { 65, 215, 155, 255 })) {
            world.LoadMapAndReset(world.currentMapId, (GameModeId)md);
        }
    }

    // 3. Right-Side Phantom Forces Advanced Stat Panel & Damage Falloff Graph
    int statW = 340;
    int statX = sw - statW - 24;
    int statY = 84;
    DrawRectangle(statX, statY, statW, 430, { 14, 18, 24, 230 });
    DrawRectangleLinesEx({ (float)statX, (float)statY, (float)statW, 430.0f }, 1.5f, { 65, 80, 98, 255 });

    DrawText(modStats.name, statX + 16, statY + 14, 24, WHITE);
    DrawText(modStats.category, statX + 16, statY + 40, 14, { 95, 185, 255, 255 });

    // Piecewise Linear Damage Graph
    int gx = statX + 20;
    int gy = statY + 68;
    int gw = statW - 40;
    int gh = 95;
    DrawRectangle(gx, gy, gw, gh, { 22, 28, 36, 255 });
    DrawRectangleLines(gx, gy, gw, gh, { 55, 65, 80, 255 });

    float maxGraphRange = 150.0f;
    float xR0 = gx + (modStats.range0 / maxGraphRange) * gw;
    float xR1 = gx + std::min(1.0f, modStats.range1 / maxGraphRange) * gw;
    float yD0 = gy + gh - std::clamp(modStats.damage0 / 100.0f, 0.0f, 1.0f) * (gh - 16);
    float yD1 = gy + gh - std::clamp(modStats.damage1 / 100.0f, 0.0f, 1.0f) * (gh - 16);

    DrawLineEx({ (float)gx, yD0 }, { xR0, yD0 }, 2.5f, { 75, 215, 130, 255 });
    DrawLineEx({ xR0, yD0 }, { xR1, yD1 }, 2.5f, { 75, 215, 130, 255 });
    DrawLineEx({ xR1, yD1 }, { (float)(gx + gw), yD1 }, 2.5f, { 75, 215, 130, 255 });
    DrawText(TextFormat("%.0f DMG (%.0fm)", modStats.damage0, modStats.range0), gx + 6, (int)yD0 - 15, 12, { 180, 245, 200, 255 });
    DrawText(TextFormat("%.0f DMG (%.0fm)", modStats.damage1, modStats.range1), (int)xR1 - 45, (int)yD1 - 15, 12, { 180, 245, 200, 255 });

    int sy = gy + gh + 18;
    auto drawStatRow = [&](const char* label, const char* val) {
        DrawText(label, statX + 18, sy, 14, { 165, 180, 198, 255 });
        int vw = MeasureText(val, 14);
        DrawText(val, statX + statW - 18 - vw, sy, 14, WHITE);
        sy += 24;
    };

    drawStatRow("RATE OF FIRE:", TextFormat("%.0f RPM", modStats.rpm));
    drawStatRow("MUZZLE VELOCITY:", TextFormat("%.0f m/s", modStats.rpm > 0 ? modStats.muzzleVelocity : 0));
    drawStatRow("WALL PENETRATION:", TextFormat("%.2f meters", modStats.penetration));
    drawStatRow("PELLETS / PROJECTILES:", TextFormat("%d", modStats.pellets));
    drawStatRow("HEAD / TORSO MULT:", TextFormat("%.2fx / %.2fx", modStats.multHead, modStats.multTorso));
    drawStatRow("MAGAZINE / RESERVE:", TextFormat("%d / %d", modStats.magSize, modStats.reserveAmmo));
    drawStatRow("WALKSPEED MULTIPLIER:", TextFormat("%.2fx", modStats.walkSpeedMult));
    drawStatRow("VERTICAL RECOIL:", TextFormat("%.2f", modStats.camRecoil.x));
    drawStatRow("HORIZONTAL RECOIL:", TextFormat("%.2f", modStats.camRecoil.y));

    // 4. Right-Bottom 8-Map Selector Grid (3 Classic PF Maps + 5 New 15v15 Blueprint Maps!)
    int mapPanelY = statY + 440;
    DrawText("SELECT MAP (3 CLASSIC + 5 15v15 BLUEPRINTS):", statX, mapPanelY, 13, { 255, 195, 95, 255 });
    mapPanelY += 18;
    for (int m = 0; m < (int)MAP_COUNT; ++m) {
        int col = m % 2;
        int row = m / 2;
        float bx = (float)statX + col * 174.0f;
        float by = (float)mapPanelY + row * 31.0f;
        Color accent = (m >= (int)MAP_FOUNDRY) ? Color{ 255, 175, 45, 255 } : Color{ 85, 185, 255, 255 };
        if (DrawMenuButton({ bx, by, 166.0f, 27.0f }, GetMapShortName((MapId)m), (int)world.currentMapId == m, accent)) {
            world.LoadMapAndReset((MapId)m, world.currentMode);
        }
    }

    int subW = MeasureText(world.map.subtitle, 14);
    DrawText(world.map.subtitle, sw / 2 - subW / 2, sh - 32, 14, { 215, 225, 240, 235 });
}
