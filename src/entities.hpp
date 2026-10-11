#pragma once
#include "raylib.h"
#include "raymath.h"
#include "math_springs.hpp"
#include "weapons.hpp"
#include "maps.hpp"
#include "audio_synth.hpp"
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// ============================================================================
// PHANTOM FORCES ENTITIES: PLAYER CONTROLLER, AI BOTS, BALLISTICS & SHELLS
// ============================================================================

enum PostureState {
    POSTURE_STAND = 0,
    POSTURE_CROUCH,
    POSTURE_PRONE,
    POSTURE_SLIDE,
    POSTURE_DIVE
};

struct Projectile {
    bool active = false;
    int ownerId = -1;       // 0 = Player, >0 = Bot ID
    int team = 0;           // 0 = Phantoms, 1 = Ghosts
    WeaponId weaponId = WPN_M4A1;
    WeaponAttachments attachments;
    Vector3 startPos;
    Vector3 hitPos;         // Exact hitscan impact point
    Vector3 pos;
    Vector3 prevPos;
    Vector3 vel;
    float maxDistance = 100.0f;
    float penetrationLeft = 1.0f;
    float distanceTraveled = 0.0f;
    float lifeTime = 0.0f;
    bool hasWallbanged = false;
    Color tracerColor = { 255, 230, 120, 240 };
};

// 3D Spinning Ejected Brass Casing / Shotgun Hull
struct ShellCasing {
    bool active = false;
    Vector3 pos;
    Vector3 vel;
    Vector3 rotDeg;
    Vector3 rotVel;
    Color color = { 225, 185, 65, 255 };
    bool isShotgunShell = false;
    float timer = 2.8f;
};

struct ImpactDecal {
    Vector3 pos;
    Vector3 normal;
    Color color;
    float timer = 6.0f;
    bool isWallbangExit = false;
};

struct FragGrenade {
    bool active = false;
    int ownerId = 0;
    int team = 0;
    Vector3 pos;
    Vector3 vel;
    float fuseTimer = 3.2f;
    float explosionFlashTimer = 0.0f;
};

struct KillFeedEntry {
    std::string killer;
    int killerTeam;
    std::string weaponName;
    std::string victim;
    int victimTeam;
    bool headshot;
    bool wallbang;
    float timer = 6.0f;
};

struct XpPopup {
    std::string label;
    int points;
    float timer = 2.8f;
};

// Swept/Iterative AABB + Step-Up Character Collision Resolver against MapBlocks
inline void ResolveCharacterCollision(
    Vector3& footPos,
    Vector3& vel,
    float radius,
    float height,
    bool& grounded,
    const GameMap& map
) {
    grounded = false;
    // Ground floor plane at y = 0
    if (footPos.y <= 0.0f) {
        footPos.y = 0.0f;
        if (vel.y < 0.0f) vel.y = 0.0f;
        grounded = true;
    }

    const float stepHeight = 0.52f; // Smoothly climb stairs/ramps & low curbs

    for (const auto& b : map.blocks) {
        if (!b.collidable) continue;
        BoundingBox box = b.GetBounds();

        BoundingBox charBox = {
            { footPos.x - radius, footPos.y,          footPos.z - radius },
            { footPos.x + radius, footPos.y + height, footPos.z + radius }
        };

        if (!CheckCollisionBoxes(charBox, box)) continue;

        float overlapX1 = box.max.x - charBox.min.x;
        float overlapX2 = charBox.max.x - box.min.x;
        float penX = (overlapX1 < overlapX2) ? overlapX1 : -overlapX2;

        float overlapY1 = box.max.y - charBox.min.y; // landing on top of box
        float overlapY2 = charBox.max.y - box.min.y; // hitting head on bottom of box
        float penY = (overlapY1 < overlapY2) ? overlapY1 : -overlapY2;

        float overlapZ1 = box.max.z - charBox.min.z;
        float overlapZ2 = charBox.max.z - box.min.z;
        float penZ = (overlapZ1 < overlapZ2) ? overlapZ1 : -overlapZ2;

        // Check if this is a low step/stair we can step up onto
        if (overlapY1 > 0.0f && overlapY1 <= stepHeight && vel.y <= 0.5f &&
            (std::fabs(penX) > 0.01f || std::fabs(penZ) > 0.01f)) {
            footPos.y = box.max.y;
            if (vel.y < 0.0f) vel.y = 0.0f;
            grounded = true;
            continue;
        }

        float absX = std::fabs(penX);
        float absY = std::fabs(penY);
        float absZ = std::fabs(penZ);

        if (absY <= absX && absY <= absZ) {
            footPos.y += penY;
            if (penY > 0.0f) {
                grounded = true;
                if (vel.y < 0.0f) vel.y = 0.0f;
            } else if (vel.y > 0.0f) {
                vel.y = 0.0f;
            }
        } else if (absX < absZ) {
            footPos.x += penX;
            vel.x = 0.0f;
        } else {
            footPos.z += penZ;
            vel.z = 0.0f;
        }
    }
}

// Check if there is a waist-high obstacle directly in front with clear space above for PF Vaulting
inline bool CheckVaultOpportunity(Vector3 footPos, Vector3 forwardDir, const GameMap& map, Vector3& outVaultTarget) {
    Vector3 flatFwd = Vector3Normalize({ forwardDir.x, 0.0f, forwardDir.z });
    Vector3 shinOrigin = { footPos.x, footPos.y + 0.55f, footPos.z };
    Vector3 headOrigin = { footPos.x, footPos.y + 1.95f, footPos.z };

    float bestDist = 1.65f;
    bool hitLow = false;
    float obstacleTopY = footPos.y + 1.1f;

    for (const auto& b : map.blocks) {
        if (!b.collidable) continue;
        BoundingBox box = b.GetBounds();
        RayBoxHit lowHit = IntersectRayAABB(shinOrigin, flatFwd, box);
        if (lowHit.hit && lowHit.tNear >= 0.0f && lowHit.tNear < bestDist) {
            float topRelative = box.max.y - footPos.y;
            if (topRelative >= 0.55f && topRelative <= 1.55f) {
                // Verify head clearance above obstacle
                RayBoxHit highHit = IntersectRayAABB(headOrigin, flatFwd, box);
                if (!highHit.hit || highHit.tNear > 1.8f) {
                    hitLow = true;
                    bestDist = lowHit.tNear;
                    obstacleTopY = box.max.y;
                }
            }
        }
    }

    if (hitLow) {
        outVaultTarget = {
            footPos.x + flatFwd.x * (bestDist + 0.85f),
            obstacleTopY + 0.05f,
            footPos.z + flatFwd.z * (bestDist + 0.85f)
        };
        return true;
    }
    return false;
}

inline float WrapAngleRad(float a) {
    while (a >  PI) a -= 2.0f * PI;
    while (a < -PI) a += 2.0f * PI;
    return a;
}

inline float SmoothTurnAngle(float current, float target, float maxTurnRate, float dt) {
    float diff = WrapAngleRad(target - current);
    float maxStep = maxTurnRate * dt;
    if (std::fabs(diff) <= maxStep) return target;
    return WrapAngleRad(current + ((diff > 0.0f) ? maxStep : -maxStep));
}

// ============================================================================
// CLASSIC ROBLOX / PHANTOM FORCES BLOCKY R6 BOT AGENT (UPGRADED TACTICAL MODEL)
// ============================================================================
struct BotAgent {
    int id = 1;
    std::string name;
    int team = 1; // 0 = Phantoms (Blue), 1 = Ghosts (Orange)
    bool alive = true;
    float health = 100.0f;
    float respawnTimer = 0.0f;

    Vector3 pos = { 0, 0, 0 }; // Foot position
    Vector3 vel = { 0, 0, 0 };
    float yaw = 0.0f;
    float pitch = 0.0f;
    bool grounded = true;
    bool crouching = false;

    WeaponId weaponId = WPN_AK47;
    WeaponAttachments attachments;
    float fireCooldown = 0.0f;
    int burstRemaining = 0;
    int continuousShots = 0;
    float timeSinceLastShot = 0.0f;
    float reactionTimer = 0.25f;
    float strafeTimer = 0.0f;
    float strafeDir = 1.0f;
    int targetWaypoint = 0;    // Immediate A* step waypoint index
    int goalWaypoint   = 0;    // High-level strategic destination waypoint index
    float walkPhase = 0.0f;
    float muzzleFlashTimer = 0.0f;

    // Pathfinding Obstacle Avoidance & Stuck Recovery State
    Vector3 lastProgressPos = { 0, 0, 0 };
    float stuckTimer = 0.0f;
    float vaultCooldown = 0.0f;
    float scanPhase = 0.0f;

    // General Awareness, Senses & Target Memory State
    bool hasTargetMemory = false;
    Vector3 lastKnownTargetPos = { 0, 0, 0 };
    Vector3 lastKnownTargetVel = { 0, 0, 0 };
    float memoryTimer = 0.0f;      // Time remaining before bot forgets last known threat
    float losLostTimer = 99.0f;    // Time since direct visual LOS was broken (for wallbang suppression)
    float combatAlertTimer = 0.0f; // Heightened 360-degree combat alertness after gunfire/damage
    float targetLockTimer = 0.0f;  // Continuous visual lock time (triggers team 3D spotting callout)

    // Radar & 3D Spotting timers
    float spottedTimer = 0.0f;   // >0 means marked by 'E' 3D diamond
    float radarPingTimer = 0.0f; // >0 means visible on minimap from unsuppressed gunfire

    int kills = 0;
    int deaths = 0;

    void AlertToThreat(Vector3 threatEyePos, Vector3 threatVel = { 0, 0, 0 }, float duration = 4.8f) {
        hasTargetMemory = true;
        lastKnownTargetPos = threatEyePos;
        lastKnownTargetVel = threatVel;
        memoryTimer = std::max(memoryTimer, duration);
        combatAlertTimer = std::max(combatAlertTimer, duration + 1.5f);
    }

    Vector3 GetFlatForward() const {
        return Vector3Normalize({ -std::sin(yaw), 0.0f, -std::cos(yaw) });
    }

    // Per-Limb Hitboxes in World Space (for Headshot / Torso / Limb multipliers)
    BoundingBox GetHeadBox() const {
        float h = crouching ? 1.05f : 1.45f;
        return {
            { pos.x - 0.24f, pos.y + h,        pos.z - 0.24f },
            { pos.x + 0.24f, pos.y + h + 0.44f, pos.z + 0.24f }
        };
    }

    BoundingBox GetTorsoBox() const {
        float bottom = crouching ? 0.42f : 0.68f;
        float top    = crouching ? 1.05f : 1.45f;
        return {
            { pos.x - 0.34f, pos.y + bottom, pos.z - 0.24f },
            { pos.x + 0.34f, pos.y + top,    pos.z + 0.24f }
        };
    }

    BoundingBox GetLimbsBox() const {
        float top = crouching ? 0.95f : 1.38f;
        return {
            { pos.x - 0.48f, pos.y,       pos.z - 0.35f },
            { pos.x + 0.48f, pos.y + top, pos.z + 0.35f }
        };
    }

    Vector3 GetEyePos() const {
        return { pos.x, pos.y + (crouching ? 1.22f : 1.62f), pos.z };
    }

    // Draw Upgraded Roblox / Phantom Forces R6 Soldier (Tactical Rig, Mag Pouches, NVG Mount & Backpack)
    void Draw() const {
        if (!alive) return;

        Color skinCol   = { 235, 202, 162, 255 };
        Color vestCol   = (team == 0) ? Color{ 38, 86, 168, 255 } : Color{ 208, 102, 34, 255 };
        Color uniformCol= (team == 0) ? Color{ 48, 62, 86, 255 }  : Color{ 88, 72, 58, 255 };
        Color helmetCol = (team == 0) ? Color{ 32, 54, 96, 255 }  : Color{ 135, 74, 32, 255 };
        Color pouchCol  = { 32, 35, 40, 255 };
        Color visorCol  = { 20, 22, 26, 255 };

        float legSwing = std::sin(walkPhase) * 24.0f;
        float crouchOff = crouching ? -0.35f : 0.0f;

        rlPushMatrix();
        rlTranslatef(pos.x, pos.y, pos.z);
        rlRotatef(yaw * RAD2DEG, 0.0f, 1.0f, 0.0f);

        // Left & Right Blocky Legs + Knee Pads + Combat Boots
        for (int s = -1; s <= 1; s += 2) {
            rlPushMatrix();
            rlTranslatef(s * 0.15f, 0.68f + crouchOff * 0.5f, 0.0f);
            rlRotatef(s * legSwing, 1.0f, 0.0f, 0.0f);
            DrawPFBox({ 0.0f, -0.34f, 0.0f }, { 0.25f, 0.68f + crouchOff * 0.4f, 0.26f }, uniformCol);
            DrawPFBox({ 0.0f, -0.32f, -0.135f }, { 0.18f, 0.14f, 0.03f }, pouchCol); // Knee pad
            DrawPFBox({ 0.0f, -0.62f, -0.02f }, { 0.26f, 0.14f, 0.30f }, pouchCol);  // Combat boot
            rlPopMatrix();
        }

        // Blocky Torso + Plate Carrier Vest + 3 Mag Pouches + Tactical Backpack
        float torsoY = 1.06f + crouchOff;
        DrawPFBox({ 0.0f, torsoY, 0.0f }, { 0.54f, 0.72f, 0.28f }, uniformCol);
        DrawPFBox({ 0.0f, torsoY + 0.02f, 0.0f }, { 0.58f, 0.52f, 0.33f }, vestCol);
        for (int p = -1; p <= 1; ++p) {
            DrawPFBox({ p * 0.14f, torsoY - 0.06f, -0.18f }, { 0.10f, 0.18f, 0.05f }, pouchCol);
        }
        DrawPFBox({ 0.0f, torsoY + 0.04f, 0.22f }, { 0.42f, 0.46f, 0.16f }, pouchCol); // Tactical backpack

        // Blocky Head + Balaclava + FAST Helmet + NVG Shroud & Side Rails
        float headY = 1.64f + crouchOff;
        DrawPFBox({ 0.0f, headY, 0.0f }, { 0.40f, 0.40f, 0.40f }, skinCol);
        DrawPFBox({ 0.0f, headY - 0.08f, -0.01f }, { 0.41f, 0.22f, 0.41f }, pouchCol); // Balaclava mask
        DrawPFBox({ 0.0f, headY + 0.13f, 0.01f }, { 0.45f, 0.22f, 0.46f }, helmetCol);
        DrawPFBox({ 0.0f, headY + 0.14f, -0.225f }, { 0.12f, 0.10f, 0.03f }, pouchCol); // NVG mount
        DrawPFBox({ 0.0f, headY + 0.03f, -0.205f }, { 0.36f, 0.11f, 0.03f }, visorCol); // Tactical goggles

        // Arms & Equipped Bespoke 3D Weapon Aiming Forward
        rlPushMatrix();
        rlTranslatef(0.0f, torsoY + 0.16f, 0.0f);
        rlRotatef(pitch * RAD2DEG, 1.0f, 0.0f, 0.0f);

        DrawPFBox({  0.34f, -0.05f, -0.22f }, { 0.20f, 0.20f, 0.54f }, uniformCol);
        DrawPFBox({ -0.24f, -0.05f, -0.24f }, { 0.20f, 0.20f, 0.52f }, uniformCol);

        rlPushMatrix();
        rlTranslatef(0.16f, 0.0f, -0.38f);
        WeaponStats ws = GetModifiedWeaponStats(weaponId, attachments);
        DrawProceduralWeaponModel(ws, attachments, 0.0f, 0.0f, muzzleFlashTimer, false, 0.0f);
        rlPopMatrix();

        rlPopMatrix();
        rlPopMatrix();
    }
};

// ============================================================================
// PHANTOM FORCES FIRST-PERSON PLAYER CONTROLLER
// ============================================================================
struct Player {
    Vector3 pos = { 0.0f, 1.0f, -35.0f }; // Foot position
    Vector3 vel = { 0.0f, 0.0f, 0.0f };
    float yaw   = 0.0f; // Radians (0 = looking toward -Z)
    float pitch = 0.0f; // Radians

    bool alive = true;
    float health = 100.0f;
    float timeSinceDamage = 10.0f; // PF auto-regen starts after 5.5s out of combat
    int kills = 0;
    int deaths = 0;
    int score = 0;
    int rank = 12;

    // Movement & Posture State Machine
    PostureState posture = POSTURE_STAND;
    bool grounded = false;
    bool sprinting = false;
    float eyeHeightCurrent = 1.62f;
    float slideTimer = 0.0f;
    Vector3 slideDir = { 0, 0, -1 };
    float diveRecoveryTimer = 0.0f;
    bool usedAirDive = false;

    // Vaulting Interpolation State
    bool isVaulting = false;
    float vaultTimer = 0.0f;
    Vector3 vaultStart = { 0, 0, 0 };
    Vector3 vaultTarget = { 0, 0, 0 };

    // Loadout (Slot 0 = Primary, Slot 1 = Secondary)
    WeaponId primaryId = WPN_M4A1;
    WeaponId secondaryId = WPN_DEAGLE;
    WeaponAttachments primaryAtt;
    WeaponAttachments secondaryAtt;
    int activeSlot = 0; // 0 = Primary, 1 = Secondary

    // Active Weapon Runtime State
    int magAmmo[2]     = { 30, 8 };
    int reserveAmmo[2] = { 150, 48 };
    FireMode currentFireMode[2] = { FIRE_AUTO, FIRE_SEMI };
    int burstShotsQueued = 0;

    float fireCooldown = 0.0f;
    bool isReloading = false;
    float reloadTimer = 0.0f;
    float reloadDuration = 2.0f;
    float boltAnimTimer = 0.0f;
    float muzzleFlashTimer = 0.0f;
    int continuousShots = 0;
    float timeSinceLastShot = 0.0f;

    // Weapon Inspect ('H' key)
    float inspectTimer = 0.0f;
    float inspectDuration = 3.2f;

    // Quick Melee Knife ('F') & Grenade ('G')
    float meleeSwingTimer = 0.0f;
    int grenadesLeft = 3;
    float grenadeThrowCooldown = 0.0f;
    float spotCooldown = 0.0f;
    float radarPingTimer = 0.0f;

    // Aiming, Canted Sights & Sniper Steady Breath
    bool isAiming = false;
    bool useCantedSight = false;
    float adsAlpha = 0.0f; // 0.0 = hipfire, 1.0 = full ADS
    float currentFov = 80.0f;
    float baseHipFov = 85.0f;
    float steadyBreathStamina = 5.0f;
    bool holdingBreath = false;

    // Procedural Spring Solvers (Camera Recoil, Gun Pos, Gun Rot, Walk/Sway)
    Spring3D camRecoilSpring  = Spring3D(150.0f, 16.0f);
    Spring3D gunPosSpring     = Spring3D(190.0f, 18.0f);
    Spring3D gunRotSpring     = Spring3D(170.0f, 16.0f);
    Spring3D swaySpring       = Spring3D(120.0f, 14.0f);
    float walkCycleTime       = 0.0f;

    // Hitmarker Feedback
    float hitmarkerTimer = 0.0f;
    bool hitmarkerHeadshot = false;
    bool hitmarkerKill = false;
    Vector3 lastDamageFromPos = { 0, 0, 0 };
    float damageIndicatorTimer = 0.0f;

    WeaponId GetActiveWeaponId() const {
        return (activeSlot == 0) ? primaryId : secondaryId;
    }

    const WeaponAttachments& GetActiveAttachments() const {
        return (activeSlot == 0) ? primaryAtt : secondaryAtt;
    }

    WeaponStats GetActiveStats() const {
        return GetModifiedWeaponStats(GetActiveWeaponId(), GetActiveAttachments());
    }

    void ResetLoadoutAmmo() {
        WeaponStats p = GetModifiedWeaponStats(primaryId, primaryAtt);
        WeaponStats s = GetModifiedWeaponStats(secondaryId, secondaryAtt);
        magAmmo[0] = p.magSize;
        reserveAmmo[0] = p.reserveAmmo;
        currentFireMode[0] = p.defaultFireMode;

        magAmmo[1] = s.magSize;
        reserveAmmo[1] = s.reserveAmmo;
        currentFireMode[1] = s.defaultFireMode;

        isReloading = false;
        fireCooldown = 0.0f;
        inspectTimer = 0.0f;
        grenadesLeft = 3;
    }

    void SpawnAt(Vector3 spawnPos, float spawnYaw) {
        pos = spawnPos;
        vel = { 0, 0, 0 };
        yaw = spawnYaw;
        pitch = 0.0f;
        alive = true;
        health = 100.0f;
        timeSinceDamage = 10.0f;
        posture = POSTURE_STAND;
        eyeHeightCurrent = 1.62f;
        isVaulting = false;
        usedAirDive = false;
        activeSlot = 0;
        adsAlpha = 0.0f;
        inspectTimer = 0.0f;
        ResetLoadoutAmmo();
    }

    Vector3 GetForward() const {
        float cp = std::cos(pitch);
        return Vector3Normalize({
            -std::sin(yaw) * cp,
             std::sin(pitch),
            -std::cos(yaw) * cp
        });
    }

    Vector3 GetFlatForward() const {
        return Vector3Normalize({ -std::sin(yaw), 0.0f, -std::cos(yaw) });
    }

    Vector3 GetRight() const {
        return Vector3Normalize({ std::cos(yaw), 0.0f, -std::sin(yaw) });
    }

    Vector3 GetEyePos() const {
        return { pos.x, pos.y + eyeHeightCurrent, pos.z };
    }
};
