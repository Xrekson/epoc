#pragma once
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

// ============================================================================
// PHANTOM FORCES WEAPON, SOCKET & EXPANDED ATTACHMENT SYSTEM (V2 OVERHAUL)
// ============================================================================

enum WeaponId {
    WPN_M4A1 = 0,
    WPN_AK47,
    WPN_INTERVENTION,
    WPN_MP7,
    WPN_REMINGTON870,
    WPN_M60,
    WPN_G17,
    WPN_DEAGLE,
    WPN_COUNT
};

enum OpticType {
    OPTIC_IRON = 0,
    OPTIC_ZPOINT,
    OPTIC_COYOTE,
    OPTIC_EOTECH552,
    OPTIC_OKP7,
    OPTIC_ACOG,
    OPTIC_PMII_SCOPE,
    OPTIC_COUNT
};

enum BarrelType {
    BARREL_NONE = 0,
    BARREL_SUPPRESSOR,
    BARREL_ARS_SUPPRESSOR,
    BARREL_MUZZLE_BRAKE,
    BARREL_COMPENSATOR,
    BARREL_LONG,
    BARREL_COUNT
};

enum UnderbarrelType {
    GRIP_NONE = 0,
    GRIP_STUBBY,
    GRIP_ANGLED,
    GRIP_VERTICAL,
    GRIP_FOLDING,
    GRIP_SKELETON,
    GRIP_HERA_CQR,
    GRIP_COUNT
};

enum OtherAttachType {
    OTHER_NONE = 0,
    OTHER_RED_LASER,
    OTHER_GREEN_LASER,
    OTHER_BLUE_LASER,
    OTHER_CANTED_DELTA,
    OTHER_BALLISTICS_TRACKER,
    OTHER_COUNT
};

enum AmmoType {
    AMMO_STANDARD = 0,
    AMMO_ARMOR_PIERCING,
    AMMO_HOLLOW_POINT,
    AMMO_PLUS_P,
    AMMO_FLECHETTE,
    AMMO_SLUGS,
    AMMO_COUNT
};

enum FireMode {
    FIRE_AUTO = 0,
    FIRE_BURST,
    FIRE_SEMI,
    FIRE_BOLT_OR_PUMP
};

struct WeaponAttachments {
    OpticType optic = OPTIC_IRON;
    BarrelType barrel = BARREL_NONE;
    UnderbarrelType grip = GRIP_NONE;
    OtherAttachType other = OTHER_NONE;
    AmmoType ammo = AMMO_STANDARD;
};

// Per-weapon 3D socket layout in weapon local space (0,0,0 = receiver center, -Z = forward)
struct WeaponSocketLayout {
    Vector3 opticMountPos;       // Top Picatinny rail mount center
    Vector3 muzzleTipPos;        // End of standard outer barrel
    Vector3 underbarrelMountPos; // Bottom handguard rail mount
    Vector3 sideRailPos;         // Right side rail for Lasers / Canted / Ballistics Tracker
    Vector3 ejectionPortPos;     // Right side chamber for bolt & ejected brass shells
    Vector3 magWellPos;          // Magazine insertion origin
    float ironSightY;            // Height of default iron sight line-of-sight
};

struct WeaponStats {
    WeaponId id;
    const char* name;
    const char* category;
    bool isPrimary;

    // Damage & Range (in meters / studs-scaled)
    float damage0;       // Max damage (close)
    float damage1;       // Min damage (far)
    float range0;        // Start dropoff distance
    float range1;        // End dropoff distance
    float multHead;      // Headshot multiplier
    float multTorso;     // Torso multiplier

    // Ballistics & Rate of Fire
    float rpm;           // Rounds per minute
    float muzzleVelocity;// m/s
    float penetration;   // Wallbang depth in meters
    int pellets;         // 1 for normal guns, 8 for shotguns
    float hipSpread;     // Hipfire spread angle (radians)
    float adsSpread;     // ADS spread angle (radians)

    // Magazine & Reload
    int magSize;
    int reserveAmmo;
    float reloadTactical;// Seconds when bullet in chamber
    float reloadEmpty;   // Seconds when completely empty

    // Recoil & Mobility
    Vector3 camRecoil;   // Pitch (x), Yaw (y), Roll (z) impulse
    Vector3 modelKick;   // Positional kickback (x, y, z)
    Vector3 modelRot;    // Rotational kick (pitch, yaw, roll)
    float adsSpeed;      // Speed of ADS transition
    float adsFov;        // Base FOV when aiming
    float walkSpeedMult; // 1.0 = standard 14 stud/s

    // Visual styling
    Color bodyColor;
    Color accentColor;
    float barrelLen;
    FireMode defaultFireMode;
};

inline const char* GetOpticName(OpticType t) {
    switch (t) {
        case OPTIC_IRON:       return "Default Iron Sights";
        case OPTIC_ZPOINT:     return "Z-Point Red Dot (1.3x)";
        case OPTIC_COYOTE:     return "Coyote Reflex (1.5x)";
        case OPTIC_EOTECH552:  return "EOTech 552 Holo (1.4x)";
        case OPTIC_OKP7:       return "OKP-7 Green Chevron (1.5x)";
        case OPTIC_ACOG:       return "ACOG 4x Scope (2.8x)";
        case OPTIC_PMII_SCOPE: return "PM II Ballistic Scope (6.0x)";
        default:               return "Iron Sights";
    }
}

inline const char* GetBarrelName(BarrelType t) {
    switch (t) {
        case BARREL_NONE:           return "Standard Barrel";
        case BARREL_SUPPRESSOR:     return "PBS-4 Suppressor (Stealth)";
        case BARREL_ARS_SUPPRESSOR: return "ARS Suppressor (Light Stealth)";
        case BARREL_MUZZLE_BRAKE:   return "Muzzle Brake (-28% Vert Recoil)";
        case BARREL_COMPENSATOR:    return "Compensator (-30% Horiz Recoil)";
        case BARREL_LONG:           return "Long Barrel (+15% Vel & Range)";
        default:                    return "None";
    }
}

inline const char* GetGripName(UnderbarrelType t) {
    switch (t) {
        case GRIP_NONE:     return "Default Handguard";
        case GRIP_STUBBY:   return "Stubby Grip (-22% Model Recoil)";
        case GRIP_ANGLED:   return "Angled Grip (-18% Cam Recoil)";
        case GRIP_VERTICAL: return "Vertical Grip (-30% Hip Spread)";
        case GRIP_FOLDING:  return "Folding Grip (-26% Pitch Kick)";
        case GRIP_SKELETON: return "Skeleton Grip (+20% ADS & Speed)";
        case GRIP_HERA_CQR: return "Hera CQR Thumbhole (-20% All Recoil)";
        default:            return "None";
    }
}

inline const char* GetOtherName(OtherAttachType t) {
    switch (t) {
        case OTHER_NONE:               return "None";
        case OTHER_RED_LASER:          return "Red Laser (-15% Hip Spread)";
        case OTHER_GREEN_LASER:        return "Green Laser (+18% Recoil Recovery)";
        case OTHER_BLUE_LASER:         return "Blue Laser (-15% Recoil Kick)";
        case OTHER_CANTED_DELTA:       return "Canted Delta Sight ([T] Toggle)";
        case OTHER_BALLISTICS_TRACKER: return "Ballistics Tracker (Head Drop HUD)";
        default:                       return "None";
    }
}

inline const char* GetAmmoName(AmmoType t) {
    switch (t) {
        case AMMO_STANDARD:       return "Standard FMJ";
        case AMMO_ARMOR_PIERCING: return "Armor Piercing (+65% Wallbang)";
        case AMMO_HOLLOW_POINT:   return "Hollow Point (+18% Close Dmg)";
        case AMMO_PLUS_P:         return "+P Overpressure (+12% Range/Vel)";
        case AMMO_FLECHETTE:      return "Flechette Sabot (3.0m Wallbang)";
        case AMMO_SLUGS:          return "Solid Slugs (1x High-Dmg Slug)";
        default:                  return "Standard";
    }
}

// Return custom 3D socket positions tailored flush to each weapon's geometry
inline WeaponSocketLayout GetWeaponSockets(WeaponId id, BarrelType barrel) {
    float longExt = (barrel == BARREL_LONG) ? 0.12f : 0.0f;
    switch (id) {
        case WPN_M4A1:
            return {
                { 0.0f, 0.052f, -0.02f },                 // opticMountPos (flush on top rail y=0.052)
                { 0.0f, 0.020f, -0.68f - longExt },       // muzzleTipPos
                { 0.0f, -0.008f, -0.27f },                // underbarrelMountPos (flush on bottom of handguard)
                { 0.030f, 0.020f, -0.30f },               // sideRailPos (flush on right handguard rail)
                { 0.028f, 0.026f, -0.03f },               // ejectionPortPos
                { 0.0f, -0.078f, -0.055f },               // magWellPos
                0.072f                                    // ironSightY (low-profile flush iron sight line)
            };
        case WPN_AK47:
            return {
                { 0.0f, 0.046f, -0.03f },                 // opticMountPos (flush on AK dust cover)
                { 0.0f, 0.016f, -0.74f - longExt },       // muzzleTipPos
                { 0.0f, -0.012f, -0.26f },                // underbarrelMountPos (flush under wooden handguard)
                { 0.029f, 0.016f, -0.31f },               // sideRailPos
                { 0.028f, 0.026f, -0.04f },               // ejectionPortPos
                { 0.0f, -0.078f, -0.065f },               // magWellPos
                0.066f                                    // ironSightY
            };
        case WPN_INTERVENTION:
            return {
                { 0.0f, 0.056f, -0.04f },                 // opticMountPos (flush on M200 top rail)
                { 0.0f, 0.024f, -0.98f - longExt },       // muzzleTipPos
                { 0.0f, -0.008f, -0.34f },                // underbarrelMountPos
                { 0.033f, 0.024f, -0.36f },               // sideRailPos
                { 0.032f, 0.032f, -0.02f },               // ejectionPortPos
                { 0.0f, -0.075f, -0.06f },                // magWellPos
                0.076f                                    // ironSightY
            };
        case WPN_MP7:
            return {
                { 0.0f, 0.050f, -0.04f },                 // opticMountPos (flush on MP7 top rail)
                { 0.0f, 0.016f, -0.36f - longExt * 0.6f },// muzzleTipPos
                { 0.0f, -0.010f, -0.20f },                // underbarrelMountPos
                { 0.027f, 0.016f, -0.21f },               // sideRailPos
                { 0.026f, 0.024f, -0.04f },               // ejectionPortPos
                { 0.0f, -0.075f, 0.04f },                 // Grip-fed magazine!
                0.068f                                    // ironSightY
            };
        case WPN_REMINGTON870:
            return {
                { 0.0f, 0.046f, -0.03f },                 // opticMountPos (flush on shotgun receiver top)
                { 0.0f, 0.026f, -0.78f - longExt },       // muzzleTipPos
                { 0.0f, -0.016f, -0.34f },                // underbarrelMountPos (flush under pump forend)
                { 0.028f, 0.012f, -0.36f },               // sideRailPos
                { 0.027f, 0.022f, -0.05f },               // ejectionPortPos
                { 0.0f, -0.008f, -0.25f },                // magWellPos
                0.062f                                    // ironSightY
            };
        case WPN_M60:
            return {
                { 0.0f, 0.058f, -0.02f },                 // opticMountPos (flush on feed tray cover)
                { 0.0f, 0.022f, -0.82f - longExt },       // muzzleTipPos
                { 0.0f, -0.018f, -0.30f },                // underbarrelMountPos
                { 0.036f, 0.020f, -0.34f },               // sideRailPos
                { 0.034f, 0.030f, -0.05f },               // ejectionPortPos
                { -0.062f, -0.055f, -0.05f },             // magWellPos
                0.078f                                    // ironSightY
            };
        case WPN_G17:
            return {
                { 0.0f, 0.040f, -0.01f },                 // opticMountPos (flush on Glock slide top)
                { 0.0f, 0.024f, -0.22f - longExt * 0.4f },// muzzleTipPos
                { 0.0f, -0.004f, -0.12f },                // underbarrelMountPos
                { 0.0f, -0.004f, -0.13f },                // sideRailPos (under-barrel pistol rail)
                { 0.020f, 0.034f, -0.04f },               // ejectionPortPos
                { 0.0f, -0.058f, 0.04f },                 // magWellPos
                0.052f                                    // ironSightY
            };
        case WPN_DEAGLE:
        default:
            return {
                { 0.0f, 0.048f, -0.04f },                 // opticMountPos (flush on Deagle rib)
                { 0.0f, 0.028f, -0.28f - longExt * 0.5f },// muzzleTipPos
                { 0.0f, -0.004f, -0.15f },                // underbarrelMountPos
                { 0.0f, -0.004f, -0.16f },                // sideRailPos
                { 0.024f, 0.040f, -0.04f },               // ejectionPortPos
                { 0.0f, -0.062f, 0.045f },                // magWellPos
                0.062f                                    // ironSightY
            };
    }
}

inline const std::vector<WeaponStats>& GetBaseWeaponDatabase() {
    static const std::vector<WeaponStats> db = {
        {
            WPN_M4A1, "M4A1", "CARBINE", true,
            34.0f, 19.0f, 22.0f, 65.0f, 1.4f, 1.0f,
            780.0f, 260.0f, 1.2f, 1, 0.036f, 0.0025f,
            30, 150, 2.1f, 2.7f,
            { 1.75f, 0.70f, 0.35f }, { 0.0f, 0.012f, 0.095f }, { 7.5f, 1.5f, 2.0f },
            14.0f, 52.0f, 1.00f,
            { 52, 56, 62, 255 }, { 32, 35, 40, 255 }, 0.58f, FIRE_AUTO
        },
        {
            WPN_AK47, "AK-47", "ASSAULT RIFLE", true,
            42.0f, 30.0f, 28.0f, 80.0f, 1.4f, 1.05f,
            600.0f, 235.0f, 1.9f, 1, 0.045f, 0.0035f,
            30, 120, 2.4f, 3.1f,
            { 2.65f, 1.15f, 0.65f }, { 0.0f, 0.018f, 0.130f }, { 12.0f, 2.8f, 3.5f },
            12.0f, 52.0f, 0.95f,
            { 56, 58, 64, 255 }, { 148, 82, 36, 255 }, 0.65f, FIRE_AUTO
        },
        {
            WPN_INTERVENTION, "INTERVENTION", "SNIPER RIFLE", true,
            90.0f, 75.0f, 50.0f, 140.0f, 3.0f, 1.50f,
            50.0f, 380.0f, 3.2f, 1, 0.085f, 0.0002f,
            7, 42, 3.0f, 4.1f,
            { 7.50f, 1.20f, 1.50f }, { 0.0f, 0.035f, 0.220f }, { 22.0f, 3.0f, 4.5f },
            8.5f, 16.0f, 0.85f,
            { 46, 50, 54, 255 }, { 126, 114, 90, 255 }, 0.92f, FIRE_BOLT_OR_PUMP
        },
        {
            WPN_MP7, "MP7", "PDW / SMG", true,
            29.0f, 17.0f, 16.0f, 50.0f, 1.4f, 1.0f,
            950.0f, 210.0f, 0.75f, 1, 0.026f, 0.0032f,
            30, 150, 1.9f, 2.4f,
            { 1.35f, 0.85f, 0.30f }, { 0.0f, 0.009f, 0.070f }, { 6.0f, 2.0f, 1.8f },
            16.5f, 56.0f, 1.08f,
            { 42, 46, 52, 255 }, { 25, 28, 32, 255 }, 0.42f, FIRE_AUTO
        },
        {
            WPN_REMINGTON870, "REMINGTON 870", "SHOTGUN", true,
            33.0f, 13.0f, 14.0f, 42.0f, 1.2f, 1.0f,
            105.0f, 195.0f, 0.85f, 8, 0.052f, 0.038f,
            8, 48, 2.6f, 3.3f,
            { 5.80f, 1.40f, 1.10f }, { 0.0f, 0.028f, 0.185f }, { 18.0f, 3.2f, 4.0f },
            12.5f, 55.0f, 0.98f,
            { 55, 58, 62, 255 }, { 136, 78, 38, 255 }, 0.72f, FIRE_BOLT_OR_PUMP
        },
        {
            WPN_M60, "M60", "LMG", true,
            40.0f, 28.0f, 34.0f, 95.0f, 1.4f, 1.1f,
            550.0f, 275.0f, 2.6f, 1, 0.058f, 0.0040f,
            100, 200, 4.4f, 5.2f,
            { 2.35f, 1.25f, 0.70f }, { 0.0f, 0.016f, 0.120f }, { 10.5f, 3.0f, 3.8f },
            8.0f, 52.0f, 0.82f,
            { 48, 52, 50, 255 }, { 68, 84, 58, 255 }, 0.78f, FIRE_AUTO
        },
        {
            WPN_G17, "GLOCK 17", "PISTOL", false,
            34.0f, 20.0f, 15.0f, 45.0f, 1.5f, 1.0f,
            780.0f, 185.0f, 0.55f, 1, 0.028f, 0.003f,
            17, 85, 1.5f, 1.9f,
            { 1.90f, 0.65f, 0.40f }, { 0.0f, 0.018f, 0.085f }, { 11.0f, 1.5f, 2.0f },
            18.0f, 58.0f, 1.12f,
            { 38, 42, 46, 255 }, { 24, 26, 30, 255 }, 0.28f, FIRE_SEMI
        },
        {
            WPN_DEAGLE, "DEAGLE L5", "REVOLVER / HEAVY PISTOL", false,
            58.0f, 36.0f, 24.0f, 65.0f, 2.5f, 1.15f,
            400.0f, 240.0f, 1.65f, 1, 0.042f, 0.0025f,
            8, 48, 2.0f, 2.5f,
            { 4.80f, 1.30f, 1.20f }, { 0.0f, 0.032f, 0.160f }, { 20.0f, 3.0f, 4.5f },
            15.0f, 55.0f, 1.05f,
            { 195, 202, 210, 255 }, { 34, 36, 40, 255 }, 0.34f, FIRE_SEMI
        }
    };
    return db;
}

inline WeaponStats GetWeaponStats(WeaponId id) {
    return GetBaseWeaponDatabase()[(int)id];
}

// Compute effective weapon stats after applying all 5 Phantom Forces attachment slots
inline WeaponStats GetModifiedWeaponStats(WeaponId id, const WeaponAttachments& att) {
    WeaponStats s = GetBaseWeaponDatabase()[(int)id];

    // 1. Optic FOV & ADS speed modifiers
    switch (att.optic) {
        case OPTIC_IRON:
            if (id == WPN_INTERVENTION) s.adsFov = 15.0f;
            break;
        case OPTIC_ZPOINT:
            s.adsFov = 48.0f;
            s.adsSpeed *= 1.06f;
            break;
        case OPTIC_COYOTE:
            s.adsFov = 44.0f;
            break;
        case OPTIC_EOTECH552:
            s.adsFov = 46.0f;
            s.camRecoil = Vector3Scale(s.camRecoil, 0.96f);
            break;
        case OPTIC_OKP7:
            s.adsFov = 43.0f;
            s.adsSpeed *= 1.04f;
            break;
        case OPTIC_ACOG:
            s.adsFov = 25.0f;
            s.adsSpeed *= 0.90f;
            break;
        case OPTIC_PMII_SCOPE:
            s.adsFov = 12.0f;
            s.adsSpeed *= 0.80f;
            break;
        default: break;
    }

    // 2. Barrel modifiers
    if (att.barrel == BARREL_SUPPRESSOR) {
        s.muzzleVelocity *= 0.85f;
        s.range0 *= 0.88f;
        s.camRecoil = Vector3Scale(s.camRecoil, 0.90f);
    } else if (att.barrel == BARREL_ARS_SUPPRESSOR) {
        s.muzzleVelocity *= 0.94f;
        s.range0 *= 0.92f;
        s.camRecoil.x *= 0.88f;
    } else if (att.barrel == BARREL_MUZZLE_BRAKE) {
        s.camRecoil.x *= 0.72f;
        s.modelRot.x  *= 0.72f;
        s.camRecoil.y *= 1.12f;
    } else if (att.barrel == BARREL_COMPENSATOR) {
        s.camRecoil.y *= 0.70f;
        s.modelRot.y  *= 0.70f;
        s.camRecoil.x *= 1.10f;
    } else if (att.barrel == BARREL_LONG) {
        s.muzzleVelocity *= 1.15f;
        s.range0 *= 1.15f;
        s.range1 *= 1.15f;
        s.adsSpeed *= 0.90f;
        s.walkSpeedMult *= 0.96f;
    }

    // 3. Underbarrel Grip modifiers
    if (att.grip == GRIP_STUBBY) {
        s.modelRot = Vector3Scale(s.modelRot, 0.78f);
        s.modelKick = Vector3Scale(s.modelKick, 0.82f);
        s.adsSpeed *= 0.93f;
    } else if (att.grip == GRIP_ANGLED) {
        s.camRecoil = Vector3Scale(s.camRecoil, 0.82f);
        s.adsSpeed *= 1.08f;
    } else if (att.grip == GRIP_VERTICAL) {
        s.hipSpread *= 0.70f;
        s.adsSpeed *= 0.92f;
    } else if (att.grip == GRIP_FOLDING) {
        s.camRecoil.x *= 0.74f;
        s.modelRot.x *= 0.82f;
        s.hipSpread *= 1.10f;
    } else if (att.grip == GRIP_SKELETON) {
        s.adsSpeed *= 1.22f;
        s.walkSpeedMult *= 1.04f;
        s.camRecoil = Vector3Scale(s.camRecoil, 1.08f);
    } else if (att.grip == GRIP_HERA_CQR) {
        s.camRecoil = Vector3Scale(s.camRecoil, 0.80f);
        s.modelRot  = Vector3Scale(s.modelRot, 0.80f);
        s.adsSpeed *= 0.86f;
    }

    // 4. Other modifiers
    if (att.other == OTHER_RED_LASER) {
        s.hipSpread *= 0.82f;
    } else if (att.other == OTHER_BLUE_LASER) {
        s.camRecoil = Vector3Scale(s.camRecoil, 0.86f);
        s.modelKick = Vector3Scale(s.modelKick, 0.85f);
    }

    // 5. Ammo / Conversion modifiers
    if (att.ammo == AMMO_ARMOR_PIERCING) {
        s.penetration *= 1.65f;
        s.range0 *= 0.82f;
    } else if (att.ammo == AMMO_HOLLOW_POINT) {
        s.damage0 *= 1.18f;
        s.damage1 *= 0.82f;
        s.penetration *= 0.50f;
        s.multTorso *= 1.05f;
    } else if (att.ammo == AMMO_PLUS_P) {
        s.muzzleVelocity *= 1.12f;
        s.range0 *= 1.12f;
        s.range1 *= 1.12f;
        s.penetration *= 1.15f;
        s.camRecoil = Vector3Scale(s.camRecoil, 1.15f);
    } else if (att.ammo == AMMO_FLECHETTE) {
        s.penetration = std::max(3.0f, s.penetration * 2.4f);
        s.damage0 *= 0.88f;
        s.damage1 *= 1.08f;
    } else if (att.ammo == AMMO_SLUGS) {
        if (s.pellets > 1) {
            s.pellets = 1;
            s.damage0 = 92.0f;
            s.damage1 = 48.0f;
            s.range0 = 32.0f;
            s.range1 = 85.0f;
            s.multHead = 1.8f;
            s.multTorso = 1.25f;
            s.adsSpread = 0.0028f;
            s.hipSpread = 0.032f;
            s.penetration = 2.1f;
            s.muzzleVelocity = 290.0f;
        } else {
            s.damage0 *= 1.12f;
            s.muzzleVelocity *= 0.90f;
        }
    }

    return s;
}

// Compute Phantom Forces piecewise linear damage at distance
inline float CalculatePFDamage(const WeaponStats& s, float distanceMeters, int hitZone) {
    float baseDmg = s.damage0;
    if (distanceMeters <= s.range0) {
        baseDmg = s.damage0;
    } else if (distanceMeters >= s.range1) {
        baseDmg = s.damage1;
    } else {
        float t = (distanceMeters - s.range0) / std::max(0.01f, s.range1 - s.range0);
        baseDmg = s.damage0 + (s.damage1 - s.damage0) * t;
    }

    if (hitZone == 2) { // Head
        return baseDmg * s.multHead;
    } else if (hitZone == 1) { // Torso
        return baseDmg * s.multTorso;
    }
    return baseDmg; // Limbs (1.0x)
}

// ============================================================================
// BESPOKE RECOIL PATTERNS & DYNAMIC SPREAD SYSTEM
// ============================================================================

// Compute deterministic procedural recoil impulse (pitch, yaw, roll) for shot 'shotIndex' in a continuous spray
inline Vector3 GetWeaponRecoilPattern(
    WeaponId id,
    int shotIndex,
    const WeaponAttachments& att,
    const WeaponStats& stats
) {
    (void)att;
    Vector3 pattern = { 1.5f, 0.0f, 0.2f };

    switch (id) {
        case WPN_M4A1: {
            // Disciplined 5.56mm carbine: smooth initial vertical climb, gentle right pull, then snaking left/right
            if (shotIndex < 4) {
                pattern = { 1.80f + shotIndex * 0.12f, 0.08f, 0.18f };
            } else if (shotIndex < 10) {
                pattern = { 1.45f, 0.65f, 0.35f };
            } else if (shotIndex < 18) {
                pattern = { 1.15f, -0.72f, -0.28f };
            } else {
                float osc = (shotIndex % 2 == 0) ? 0.48f : -0.48f;
                pattern = { 0.85f, osc, osc * 0.45f };
            }
            break;
        }
        case WPN_AK47: {
            // Aggressive 7.62x39mm: violent upward climb, hard pull to the right, then sweeping sharp left (CS "7" shape)
            if (shotIndex < 5) {
                pattern = { 2.85f + shotIndex * 0.14f, -0.18f, 0.45f };
            } else if (shotIndex < 12) {
                pattern = { 2.10f, 1.65f, 0.85f };
            } else if (shotIndex < 22) {
                pattern = { 1.30f, -1.85f, -0.95f };
            } else {
                float osc = (shotIndex % 2 == 0) ? 0.65f : -0.65f;
                pattern = { 0.95f, -0.85f + osc, -0.45f };
            }
            break;
        }
        case WPN_MP7: {
            // 950 RPM buzzing PDW: fast linear vertical climb, then tight high-frequency zig-zag
            if (shotIndex < 8) {
                pattern = { 1.25f + shotIndex * 0.035f, 0.05f, 0.15f };
            } else if (shotIndex < 16) {
                pattern = { 0.90f, 0.95f, 0.35f };
            } else if (shotIndex < 24) {
                pattern = { 0.75f, -1.05f, -0.40f };
            } else {
                float s = (shotIndex % 2 == 0) ? 0.75f : -0.75f;
                pattern = { 0.65f, s, s * 0.40f };
            }
            break;
        }
        case WPN_M60: {
            // Heavy 7.62 NATO LMG: massive initial hammer blow, wide wandering horizontal wave under sustained fire
            if (shotIndex < 5) {
                pattern = { 3.10f, 0.40f, 0.50f };
            } else if (shotIndex < 15) {
                pattern = { 2.20f, 1.40f, 0.75f };
            } else if (shotIndex < 32) {
                pattern = { 1.60f, -1.55f, -0.80f };
            } else {
                float wave = std::sin((float)shotIndex * 0.42f) * 1.65f;
                pattern = { 1.35f, wave, wave * 0.35f };
            }
            break;
        }
        case WPN_INTERVENTION: {
            // Heavy .408 CheyTac bolt-action: single massive vertical impulse + right rotational torque
            pattern = { 7.80f, 0.85f, 1.60f };
            break;
        }
        case WPN_REMINGTON870: {
            // 12-Gauge pump shotgun: heavy muzzle rise & backward kick, slight left roll
            pattern = { 6.20f, -0.65f, 1.25f };
            break;
        }
        case WPN_G17: {
            // Snappy 9mm semi-auto: quick vertical pop, climbing with right drift if spammed
            if (shotIndex < 3) {
                pattern = { 1.85f, 0.25f, 0.30f };
            } else if (shotIndex < 8) {
                pattern = { 2.35f, 0.55f, 0.45f };
            } else {
                float osc = (shotIndex % 2 == 0) ? 0.60f : -0.60f;
                pattern = { 2.60f, osc, 0.50f };
            }
            break;
        }
        case WPN_DEAGLE: {
            // .50 Action Express hand cannon: brutal vertical muzzle flip and camera torque
            if (shotIndex < 2) {
                pattern = { 5.20f, 1.20f, 2.00f };
            } else {
                float s = (shotIndex % 2 == 0) ? 1.60f : -1.20f;
                pattern = { 5.80f, s, 2.20f };
            }
            break;
        }
        default:
            pattern = stats.camRecoil;
            break;
    }

    // Scale pattern based on weapon attachment modifiers (Muzzle Brake, Compensator, Grips, Lasers)
    const WeaponStats& base = GetBaseWeaponDatabase()[(int)id];
    float vertScale  = (base.camRecoil.x > 0.001f) ? (stats.camRecoil.x / base.camRecoil.x) : 1.0f;
    float horizScale = (base.camRecoil.y > 0.001f) ? (stats.camRecoil.y / base.camRecoil.y) : 1.0f;
    pattern.x *= vertScale;
    pattern.y *= horizScale;
    pattern.z *= (vertScale * 0.5f + horizScale * 0.5f);

    return pattern;
}

// Compute dynamic bloom spread angle (radians) factoring stance, movement, ADS, and continuous spray bloom
inline float CalculateDynamicSpread(
    const WeaponStats& stats,
    float adsAlpha,
    int posture,
    bool isMoving,
    bool isSprinting,
    bool isAirborne,
    int continuousShots
) {
    // 1. Base spread interpolated between hipfire and ADS
    float spread = LerpFloat(stats.hipSpread, stats.adsSpread, adsAlpha);

    // 2. Posture stance modifiers
    if (posture == 1 /*POSTURE_CROUCH*/) spread *= 0.75f;
    else if (posture == 2 /*POSTURE_PRONE*/)  spread *= 0.55f;
    else if (posture == 3 /*POSTURE_SLIDE*/ || posture == 4 /*POSTURE_DIVE*/) spread *= 1.15f;

    // 3. Movement and airborne penalties
    float movePenalty = 1.0f;
    if (isAirborne) {
        movePenalty = 2.10f;
    } else if (isSprinting) {
        movePenalty = 1.70f;
    } else if (isMoving) {
        movePenalty = 1.35f;
    }

    // ADS dampens movement spread penalty by 60%
    float effectiveMovePenalty = LerpFloat(movePenalty, 1.0f + (movePenalty - 1.0f) * 0.40f, adsAlpha);
    spread *= effectiveMovePenalty;

    // 4. Continuous fire spread bloom (accumulation over sustained spray)
    if (continuousShots > 1 && stats.defaultFireMode != FIRE_BOLT_OR_PUMP) {
        float bloomPerShot = 0.0020f;
        float maxBloom = 0.028f;
        if (stats.id == WPN_AK47) {
            bloomPerShot = 0.0028f;
            maxBloom = 0.038f;
        } else if (stats.id == WPN_M4A1) {
            bloomPerShot = 0.0018f;
            maxBloom = 0.024f;
        } else if (stats.id == WPN_MP7) {
            bloomPerShot = 0.0016f;
            maxBloom = 0.026f;
        } else if (stats.id == WPN_M60) {
            bloomPerShot = 0.0022f;
            maxBloom = 0.045f;
        } else if (stats.id == WPN_DEAGLE) {
            bloomPerShot = 0.0060f;
            maxBloom = 0.035f;
        } else if (stats.id == WPN_G17) {
            bloomPerShot = 0.0025f;
            maxBloom = 0.022f;
        }

        // ADS tightens spray bloom accumulation by 40%
        bloomPerShot *= (1.0f - 0.40f * adsAlpha);
        maxBloom     *= (1.0f - 0.40f * adsAlpha);

        float sprayBloom = std::min((continuousShots - 1) * bloomPerShot, maxBloom);
        spread += sprayBloom;
    }

    return spread;
}

// ============================================================================
// CSG PRIMITIVE HELPERS: BOXES, PITCHED/ANGLED BOXES & CYLINDERS
// ============================================================================
inline void DrawPFBox(Vector3 pos, Vector3 size, Color fill, Color wire = { 18, 20, 24, 155 }) {
    DrawCubeV(pos, size, fill);
    DrawCubeWiresV(pos, size, wire);
}

inline void DrawPFAngledBox(Vector3 pos, Vector3 size, float pitchDeg, float yawDeg, float rollDeg, Color fill) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    if (rollDeg  != 0.0f) rlRotatef(rollDeg,  0.0f, 0.0f, 1.0f);
    if (yawDeg   != 0.0f) rlRotatef(yawDeg,   0.0f, 1.0f, 0.0f);
    if (pitchDeg != 0.0f) rlRotatef(pitchDeg, 1.0f, 0.0f, 0.0f);
    DrawPFBox({ 0, 0, 0 }, size, fill);
    rlPopMatrix();
}

inline void DrawPFCylinder(Vector3 start, Vector3 end, float rStart, float rEnd, int sides, Color fill) {
    DrawCylinderEx(start, end, rStart, rEnd, sides, fill);
    DrawCylinderWiresEx(start, end, rStart, rEnd, sides, { 18, 20, 24, 130 });
}

// Helper: Render a MIL-STD-1913 Picatinny rail spine with individual raised cross-slot teeth
inline void DrawPFPicatinnyRail(Vector3 center, float width, float height, float length, int numTeeth, Color baseCol, Color toothCol) {
    DrawPFBox({ center.x, center.y - height * 0.22f, center.z }, { width * 0.78f, height * 0.56f, length }, baseCol);
    if (numTeeth <= 1) return;
    float toothLen = (length / (float)numTeeth) * 0.52f;
    float startZ   = center.z - length * 0.5f + toothLen * 0.7f;
    float endZ     = center.z + length * 0.5f - toothLen * 0.7f;
    for (int i = 0; i < numTeeth; ++i) {
        float t = (float)i / (float)(numTeeth - 1);
        float z = startZ + (endZ - startZ) * t;
        DrawPFBox({ center.x, center.y + height * 0.22f, z }, { width, height * 0.56f, toothLen }, toothCol);
    }
}

// Helper: Render an enclosed trigger guard bow + angled curved trigger blade + axis pin
inline void DrawPFTriggerAssembly(Vector3 guardCenter, float width, float length, Color guardCol, Color triggerCol) {
    // Bottom bow + front angled strut
    DrawPFBox(guardCenter, { width, 0.006f, length }, guardCol);
    DrawPFAngledBox({ guardCenter.x, guardCenter.y + 0.010f, guardCenter.z - length * 0.46f },
                    { width, 0.022f, 0.006f }, 24.0f, 0.0f, 0.0f, guardCol);
    // Two-segment curved trigger blade
    DrawPFAngledBox({ guardCenter.x, guardCenter.y + 0.020f, guardCenter.z + 0.004f },
                    { width * 0.42f, 0.016f, 0.006f }, -18.0f, 0.0f, 0.0f, triggerCol);
    DrawPFAngledBox({ guardCenter.x, guardCenter.y + 0.010f, guardCenter.z + 0.001f },
                    { width * 0.42f, 0.014f, 0.005f },  22.0f, 0.0f, 0.0f, triggerCol);
}

// Helper: Render receiver cross-pins / rivets protruding slightly from both receiver walls
inline void DrawPFReceiverPin(Vector3 pos, float receiverWidth, float radius, Color col) {
    DrawPFCylinder({ pos.x - receiverWidth * 0.54f, pos.y, pos.z },
                   { pos.x + receiverWidth * 0.54f, pos.y, pos.z }, radius, radius, 6, col);
}

// Helper: Knurled cylinder — a body cylinder with equally-spaced raised grip rings along its length
inline void DrawPFKnurledCylinder(Vector3 start, Vector3 end, float radius, int sides,
                                   int grooves, Color bodyCol, Color grooveCol) {
    DrawPFCylinder(start, end, radius, radius, sides, bodyCol);
    if (grooves <= 0) return;
    float dx = end.x - start.x, dy = end.y - start.y, dz = end.z - start.z;
    for (int g = 0; g < grooves; ++g) {
        float t  = (float)(g + 0.5f) / (float)grooves;
        float t2 = t + 0.015f / (float)grooves;
        Vector3 ga = { start.x + dx * t,  start.y + dy * t,  start.z + dz * t  };
        Vector3 gb = { start.x + dx * t2, start.y + dy * t2, start.z + dz * t2 };
        float gr = radius * 1.06f;
        DrawPFCylinder(ga, gb, gr, gr, sides, grooveCol);
    }
}

// Helper: Tactical scope ring — two angled half-shells + dual vertical thumb-nut bolt stubs
inline void DrawPFScopeRing(Vector3 center, float tubeRadius, Color ringCol, Color boltCol) {
    float br = tubeRadius + 0.0035f;
    DrawPFAngledBox({ -br * 0.80f, center.y + br * 0.50f, center.z }, { 0.009f, br * 1.10f, 0.016f }, 0.0f, 0.0f,  30.0f, ringCol);
    DrawPFAngledBox({  br * 0.80f, center.y + br * 0.50f, center.z }, { 0.009f, br * 1.10f, 0.016f }, 0.0f, 0.0f, -30.0f, ringCol);
    DrawPFKnurledCylinder({ -br * 0.84f, center.y + br * 1.02f, center.z },
                          { -br * 0.84f, center.y + br * 1.38f, center.z }, 0.0055f, 6, 2, boltCol, ringCol);
    DrawPFKnurledCylinder({  br * 0.84f, center.y + br * 1.02f, center.z },
                          {  br * 0.84f, center.y + br * 1.38f, center.z }, 0.0055f, 6, 2, boltCol, ringCol);
    DrawSphere({ -br * 0.84f, center.y + br * 1.40f, center.z }, 0.0065f, boltCol);
    DrawSphere({  br * 0.84f, center.y + br * 1.40f, center.z }, 0.0065f, boltCol);
}

// Return the exact optical reticle height above receiver origin (0,0,0) for ADS alignment
inline float GetOpticSightHeight(WeaponId id, OpticType optic) {
    WeaponSocketLayout sock = GetWeaponSockets(id, BARREL_NONE);
    OpticType eff = optic;
    if (id == WPN_INTERVENTION && optic == OPTIC_IRON) eff = OPTIC_PMII_SCOPE;
    switch (eff) {
        case OPTIC_IRON:       return sock.ironSightY;
        case OPTIC_ZPOINT:     return sock.opticMountPos.y + 0.024f;
        case OPTIC_COYOTE:     return sock.opticMountPos.y + 0.026f;
        case OPTIC_EOTECH552:  return sock.opticMountPos.y + 0.026f;
        case OPTIC_OKP7:       return sock.opticMountPos.y + 0.021f;
        case OPTIC_ACOG:       return sock.opticMountPos.y + 0.030f;
        case OPTIC_PMII_SCOPE: return sock.opticMountPos.y + 0.032f;
        default:               return sock.ironSightY;
    }
}

// ============================================================================
// BESPOKE MULTI-PART CSG WEAPON + ATTACHMENT + OPTIC + DYNAMIC ARM RENDERER
// ============================================================================
inline void DrawProceduralWeaponModel(
    const WeaponStats& stats,
    const WeaponAttachments& att,
    float reloadProgress, // 0.0 = not reloading, (0..1) = reloading cycle
    float boltProgress,   // (0..1) bolt/slide/pump cycle
    float muzzleFlashTimer,
    bool drawArms,
    float adsAlpha        // 0 = hipfire, 1 = full ADS
) {
    Color cBody   = stats.bodyColor;
    Color cAccent = stats.accentColor;
    Color cDark   = { 26, 28, 32, 255 };
    Color cPolymer= { 38, 41, 46, 255 };
    Color cSteel  = { 116, 122, 130, 255 };
    Color cChrome = { 168, 174, 182, 255 };
    Color cBrass  = { 225, 185, 65, 255 };
    Color cCopper = { 198, 108, 58, 255 };

    WeaponSocketLayout sock = GetWeaponSockets(stats.id, att.barrel);

    // Compute Magazine Reload Animation Offset & Rocking Angle
    Vector3 magOffset = { 0.0f, 0.0f, 0.0f };
    float magPitch = 0.0f;
    if (reloadProgress > 0.001f) {
        float phase = 0.0f;
        if (reloadProgress < 0.35f) {
            phase = reloadProgress / 0.35f;
        } else if (reloadProgress < 0.65f) {
            phase = 1.0f;
        } else {
            phase = 1.0f - (reloadProgress - 0.65f) / 0.35f;
        }
        magOffset.y = -0.26f * phase;
        magOffset.x = -0.05f * phase;
        magPitch = (stats.id == WPN_AK47) ? (-28.0f * phase) : (10.0f * phase);
    }

    // Magazine tint for custom Ammo types
    Color magTint = cDark;
    if (att.ammo == AMMO_ARMOR_PIERCING) magTint = { 42, 78, 124, 255 };
    else if (att.ammo == AMMO_HOLLOW_POINT) magTint = { 128, 42, 42, 255 };
    else if (att.ammo == AMMO_PLUS_P) magTint = { 145, 115, 38, 255 };
    else if (att.ammo == AMMO_FLECHETTE) magTint = { 48, 118, 88, 255 };
    else if (att.ammo == AMMO_SLUGS) magTint = { 145, 55, 125, 255 };

    // ------------------------------------------------------------------------
    // 1. BESPOKE PER-WEAPON CSG GEOMETRY (High-Detail Multi-Part Assemblies)
    // ------------------------------------------------------------------------
    if (stats.id == WPN_M4A1) {
        // Stepped Forged AR-15 Upper & Lower Receiver + Beveled Magwell Lip
        DrawPFBox({ 0.0f, 0.022f, 0.00f }, { 0.058f, 0.052f, 0.24f }, cBody);
        DrawPFAngledBox({ -0.024f, 0.042f, 0.00f }, { 0.012f, 0.014f, 0.238f }, 0.0f, 0.0f, -35.0f, cBody);
        DrawPFAngledBox({  0.024f, 0.042f, 0.00f }, { 0.012f, 0.014f, 0.238f }, 0.0f, 0.0f,  35.0f, cBody);
        DrawPFBox({ 0.0f, -0.018f, -0.02f }, { 0.054f, 0.044f, 0.19f }, cBody);
        DrawPFBox({ 0.0f, -0.030f, -0.055f }, { 0.058f, 0.052f, 0.082f }, cBody);
        DrawPFBox({ 0.0f, -0.054f, -0.055f }, { 0.062f, 0.008f, 0.086f }, cDark); // Flared magwell lip
        // Magwell strengthening ribs + fencing around mag catch
        DrawPFBox({  0.029f, -0.022f, -0.045f }, { 0.005f, 0.036f, 0.055f }, cDark);
        DrawPFBox({  0.031f, -0.016f, -0.025f }, { 0.006f, 0.012f, 0.018f }, cSteel); // Mag release button
        DrawPFBox({ -0.029f,  0.002f, -0.015f }, { 0.006f, 0.022f, 0.012f }, cSteel); // Bolt catch paddle
        // Front Pivot Pin & Rear Takedown Pin + Fire Selector Lever
        DrawPFReceiverPin({ 0.0f, -0.004f, -0.095f }, 0.058f, 0.0045f, cDark);
        DrawPFReceiverPin({ 0.0f, -0.004f,  0.065f }, 0.058f, 0.0045f, cDark);
        DrawPFReceiverPin({ 0.0f, -0.018f,  0.022f }, 0.056f, 0.0030f, cSteel);
        DrawPFAngledBox({ -0.029f, -0.010f, 0.048f }, { 0.006f, 0.007f, 0.024f }, 15.0f, 0.0f, 0.0f, cDark); // Fire selector

        // T-Shaped AR Charging Handle + Latch at rear of upper receiver
        DrawPFBox({ 0.0f, 0.038f, 0.118f }, { 0.048f, 0.010f, 0.016f }, cDark);
        DrawPFBox({ -0.024f, 0.038f, 0.114f }, { 0.008f, 0.012f, 0.012f }, cSteel);
        // Brass Deflector Pyramid + Angled Forward Assist Plunger on right side
        DrawPFAngledBox({ 0.030f, 0.026f, 0.028f }, { 0.016f, 0.022f, 0.026f }, 0.0f, 24.0f, 0.0f, cBody);
        DrawPFCylinder({ 0.026f, 0.022f, 0.055f }, { 0.038f, 0.022f, 0.088f }, 0.007f, 0.008f, 8, cDark);

        // Ergonomic A2 Pistol Grip with Finger Swell + Trigger Assembly
        DrawPFAngledBox({ 0.0f, -0.075f, 0.08f }, { 0.040f, 0.110f, 0.048f }, -18.0f, 0.0f, 0.0f, cAccent);
        DrawPFAngledBox({ 0.0f, -0.068f, 0.056f }, { 0.042f, 0.014f, 0.012f }, -18.0f, 0.0f, 0.0f, cDark); // A2 finger shelf
        DrawPFAngledBox({ 0.0f, -0.125f, 0.096f }, { 0.042f, 0.010f, 0.052f }, -18.0f, 0.0f, 0.0f, cDark); // Grip baseplate
        DrawPFTriggerAssembly({ 0.0f, -0.046f, 0.024f }, 0.018f, 0.054f, cDark, cSteel);

        // Castle Nut + Receiver Endplate + 6-Position Buffer Tube + SOPMOD Stock
        DrawPFCylinder({ 0.0f, 0.020f, 0.118f }, { 0.0f, 0.020f, 0.138f }, 0.023f, 0.023f, 8, cDark);
        DrawPFCylinder({ 0.0f, 0.020f, 0.138f }, { 0.0f, 0.020f, 0.36f }, 0.018f, 0.018f, 10, cDark);
        DrawPFBox({ 0.0f, 0.014f, 0.28f }, { 0.054f, 0.052f, 0.17f }, cAccent); // Cheek weld
        DrawPFAngledBox({ -0.022f, 0.024f, 0.28f }, { 0.016f, 0.032f, 0.165f }, 0.0f, 0.0f,  32.0f, cAccent); // SOPMOD battery tubes
        DrawPFAngledBox({  0.022f, 0.024f, 0.28f }, { 0.016f, 0.032f, 0.165f }, 0.0f, 0.0f, -32.0f, cAccent);
        DrawPFAngledBox({ 0.0f, -0.030f, 0.31f }, { 0.036f, 0.072f, 0.12f }, 28.0f, 0.0f, 0.0f, cAccent);
        DrawPFBox({ 0.0f, -0.048f, 0.27f }, { 0.022f, 0.014f, 0.075f }, cDark); // Stock adjustment latch lever
        DrawPFBox({ 0.0f, -0.008f, 0.37f }, { 0.054f, 0.122f, 0.024f }, cDark); // Ribbed rubber buttpad
        DrawPFBox({ 0.0f, -0.058f, 0.352f }, { 0.010f, 0.022f, 0.022f }, cSteel); // Rear sling loop

        // Delta Ring + Ventilated RIS Quad-Rail Handguard + Stainless Gas Tube Inside
        DrawPFCylinder({ 0.0f, 0.020f, -0.12f }, { 0.0f, 0.020f, -0.155f }, 0.030f, 0.026f, 12, cDark);
        DrawPFCylinder({ 0.0f, 0.036f, -0.12f }, { 0.0f, 0.036f, -0.41f }, 0.0045f, 0.0045f, 6, cChrome); // Gas tube
        DrawPFBox({ 0.0f, 0.020f, -0.27f }, { 0.052f, 0.054f, 0.23f }, cAccent);
        for (int v = 0; v < 5; ++v) {
            float vz = -0.175f - v * 0.044f;
            DrawPFBox({ 0.0f, 0.020f, vz }, { 0.056f, 0.018f, 0.022f }, cDark); // Side vent cutouts
            DrawPFBox({ 0.0f, 0.020f, vz }, { 0.020f, 0.058f, 0.022f }, cDark); // Top/bottom vent cutouts
        }
        // Left, Right & Bottom Picatinny Accessory Rails + Full-Length Top Picatinny Rail (flush at y = 0.052f)
        DrawPFPicatinnyRail({ -0.028f, 0.020f, -0.27f }, 0.008f, 0.014f, 0.22f, 9, cDark, cPolymer);
        DrawPFPicatinnyRail({  0.028f, 0.020f, -0.27f }, 0.008f, 0.014f, 0.22f, 9, cDark, cPolymer);
        DrawPFPicatinnyRail({  0.0f,  -0.009f, -0.27f }, 0.028f, 0.008f, 0.22f, 9, cDark, cPolymer);
        DrawPFPicatinnyRail({  0.0f,   0.050f, -0.13f }, 0.034f, 0.010f, 0.48f, 18, cDark, cPolymer);

        // Stepped M4 Barrel Profile (M203 step cut) + A2 Birdcage Flash Hider with Wrench Flats
        DrawPFCylinder({ 0.0f, 0.020f, -0.38f }, { 0.0f, 0.020f, -0.49f }, 0.012f, 0.012f, 10, cSteel);
        DrawPFCylinder({ 0.0f, 0.020f, -0.49f }, { 0.0f, 0.020f, -0.52f }, 0.0095f, 0.0095f, 10, cSteel); // M203 step-down
        DrawPFCylinder({ 0.0f, 0.020f, -0.52f }, sock.muzzleTipPos, 0.012f, 0.012f, 10, cSteel);
        if (att.barrel == BARREL_NONE || att.barrel == BARREL_LONG) {
            Vector3 fhEnd = Vector3Add(sock.muzzleTipPos, { 0, 0, -0.046f });
            DrawPFCylinder(sock.muzzleTipPos, Vector3Add(sock.muzzleTipPos, { 0, 0, -0.010f }), 0.013f, 0.013f, 8, cSteel); // Crush washer
            DrawPFCylinder(Vector3Add(sock.muzzleTipPos, { 0, 0, -0.010f }), fhEnd, 0.015f, 0.015f, 10, cDark);
            DrawPFBox(Vector3Add(sock.muzzleTipPos, { 0, 0.004f, -0.028f }), { 0.032f, 0.006f, 0.020f }, cSteel); // Birdcage ports
        }

        // Fully-Grounded Iconic A2 Front Sight Gas Block Tower + Rail-Mounted Rear Peep Sight
        if (att.optic == OPTIC_IRON) {
            // 1. Gas Block Collar pinned around barrel (y = 0.020f, r = 0.012f) with dual cross taper pins
            DrawPFCylinder({ 0.0f, 0.020f, -0.402f }, { 0.0f, 0.020f, -0.442f }, 0.0150f, 0.0150f, 10, cDark);
            DrawPFReceiverPin({ 0.0f, 0.020f, -0.412f }, 0.032f, 0.0032f, cSteel);
            DrawPFReceiverPin({ 0.0f, 0.020f, -0.432f }, 0.032f, 0.0032f, cSteel);
            // Stainless gas tube entering upper receiver through delta ring
            DrawPFCylinder({ 0.0f, 0.036f, -0.422f }, { 0.0f, 0.036f, -0.380f }, 0.0035f, 0.0035f, 6, cChrome);

            // 2. Underslung Bayonet Lug & Sling Swivel Bracket
            DrawPFBox({ 0.0f, 0.003f, -0.422f }, { 0.014f, 0.010f, 0.026f }, cDark); // Lug neck
            DrawPFBox({ 0.0f, -0.003f, -0.422f }, { 0.018f, 0.004f, 0.020f }, cDark); // T-shelf
            DrawPFBox({ 0.0f, -0.009f, -0.414f }, { 0.018f, 0.010f, 0.012f }, cSteel); // Swivel bracket
            DrawPFReceiverPin({ 0.0f, -0.007f, -0.414f }, 0.022f, 0.0024f, cDark);

            // 3. Solid Triangular A2 Tower Body (zero gap beneath — continuous solid web from barrel to deck)
            DrawPFBox({ 0.0f, 0.047f, -0.422f }, { 0.013f, 0.030f, 0.024f }, cDark); // Central solid web
            DrawPFAngledBox({ 0.0f, 0.048f, -0.435f }, { 0.013f, 0.032f, 0.010f }, -22.0f, 0.0f, 0.0f, cDark); // Angled front leg
            DrawPFBox({ 0.0f, 0.048f, -0.412f }, { 0.013f, 0.032f, 0.008f }, cDark); // Rear upright leg

            // 4. Horizontal Sight Platform Deck & Elevation Post Well
            DrawPFBox({ 0.0f, 0.062f, -0.422f }, { 0.018f, 0.004f, 0.020f }, cDark);
            DrawPFCylinder({ 0.0f, 0.062f, -0.422f }, { 0.0f, 0.064f, -0.422f }, 0.0045f, 0.0045f, 8, cSteel);

            // 5. Curved Protective Sight Wings (Ears)
            DrawPFAngledBox({ -0.0068f, 0.069f, -0.422f }, { 0.0022f, 0.014f, 0.016f }, 0.0f, 0.0f, -15.0f, cDark);
            DrawPFAngledBox({ -0.0084f, 0.076f, -0.422f }, { 0.0020f, 0.006f, 0.014f }, 0.0f, 0.0f,  20.0f, cDark);
            DrawPFAngledBox({  0.0068f, 0.069f, -0.422f }, { 0.0022f, 0.014f, 0.016f }, 0.0f, 0.0f,  15.0f, cDark);
            DrawPFAngledBox({  0.0084f, 0.076f, -0.422f }, { 0.0020f, 0.006f, 0.014f }, 0.0f, 0.0f, -20.0f, cDark);

            // 6. Precision Front Sight Post & Luminous Tritium Aiming Bead (at exact ironSightY = 0.072f)
            DrawPFBox({ 0.0f, 0.0675f, -0.422f }, { 0.0018f, 0.009f, 0.0018f }, cDark);
            DrawCubeV({ 0.0f, 0.072f, -0.422f }, { 0.0016f, 0.0016f, 0.0016f }, Color{ 70, 255, 110, 255 });

            // 7. Rail-Mounted Flip-Up Rear BUIS (Backup Iron Sight) on Receiver Top Rail
            DrawPFBox({ 0.0f, 0.054f, 0.075f }, { 0.026f, 0.006f, 0.022f }, cDark); // Picatinny clamp base
            DrawPFCylinder({ -0.013f, 0.054f, 0.075f }, { -0.017f, 0.054f, 0.075f }, 0.0042f, 0.0042f, 6, cSteel); // Clamp screw
            DrawPFReceiverPin({ 0.0f, 0.057f, 0.070f }, 0.024f, 0.0028f, cSteel); // Pivot pin
            DrawPFKnurledCylinder({ 0.012f, 0.068f, 0.075f }, { 0.017f, 0.068f, 0.075f }, 0.005f, 6, 2, cDark, cSteel); // Windage drum
            DrawPFBox({ 0.0f, 0.061f, 0.075f }, { 0.008f, 0.014f, 0.005f }, cDark); // Vertical stem
            // Rear Peep Ring centered at (0.0f, 0.072f, 0.075f) — co-linear with front post and view ray
            DrawPFBox({ -0.0055f, 0.072f, 0.075f }, { 0.0028f, 0.009f, 0.004f }, cDark);
            DrawPFBox({  0.0055f, 0.072f, 0.075f }, { 0.0028f, 0.009f, 0.004f }, cDark);
            DrawPFBox({  0.0f, 0.0675f, 0.075f }, { 0.009f, 0.0022f, 0.004f }, cDark);
            DrawPFBox({  0.0f, 0.0765f, 0.075f }, { 0.009f, 0.0022f, 0.004f }, cDark);
        } else {
            // Low-Profile Gas Block on barrel when equipped with optical sights + folded rear BUIS
            DrawPFBox({ 0.0f, 0.024f, -0.422f }, { 0.024f, 0.020f, 0.032f }, cDark);
            DrawPFReceiverPin({ 0.0f, 0.020f, -0.422f }, 0.028f, 0.003f, cSteel);
            DrawPFCylinder({ 0.0f, 0.034f, -0.422f }, { 0.0f, 0.034f, -0.380f }, 0.0035f, 0.0035f, 6, cChrome);
            DrawPFBox({ 0.0f, 0.054f, 0.075f }, { 0.024f, 0.005f, 0.022f }, cDark); // Folded rear BUIS
        }

        // Ejection Port Hinged Dust Cover + Reciprocating Bolt Carrier Group
        float boltZ = sock.ejectionPortPos.z + boltProgress * 0.075f;
        DrawPFBox({ 0.028f, 0.026f, boltZ }, { 0.012f, 0.020f, 0.078f }, cChrome);
        // Gas key on top of BCG + cam pin hole + firing pin retaining pin
        DrawPFBox({ 0.0f, 0.036f, boltZ - 0.005f }, { 0.010f, 0.009f, 0.038f }, cDark);        // Gas key body
        DrawPFBox({ 0.0f, 0.040f, boltZ + 0.010f }, { 0.006f, 0.005f, 0.010f }, cSteel);       // Gas key cap
        DrawPFReceiverPin({ 0.0f, 0.018f, boltZ + 0.022f }, 0.012f, 0.0025f, cDark);          // Cam pin
        DrawPFReceiverPin({ 0.0f, 0.018f, boltZ - 0.028f }, 0.012f, 0.0022f, cChrome);        // Firing pin ret pin
        DrawPFAngledBox({ 0.032f, 0.012f, sock.ejectionPortPos.z }, { 0.004f, 0.018f, 0.076f }, 0.0f, 0.0f, 25.0f, cDark);

        // Barrel indexing band (prevents rotation under suppressor torque) + muzzle end marking ring
        DrawPFCylinder({ 0.0f, 0.020f, -0.385f }, { 0.0f, 0.020f, -0.400f }, 0.013f, 0.013f, 8, cDark);
        DrawPFCylinder({ 0.0f, 0.020f, -0.386f }, { 0.0f, 0.020f, -0.388f }, 0.0145f, 0.0145f, 8, cSteel);

        // 6-Position buffer tube position notches (cosmetic ridges along the tube)
        for (int n = 0; n < 6; ++n) {
            DrawPFBox({ 0.020f, 0.020f, 0.160f + n * 0.030f }, { 0.001f, 0.008f, 0.006f }, cDark);
        }

        // QD end-plate sling socket flanges (one either side of the receiver endplate)
        DrawPFBox({ -0.028f, 0.020f, 0.138f }, { 0.006f, 0.016f, 0.014f }, cSteel);
        DrawPFBox({  0.028f, 0.020f, 0.138f }, { 0.006f, 0.016f, 0.014f }, cSteel);

        // Textured A2 grip checkering line grooves (3 horizontal bands)
        for (int ck = 0; ck < 4; ++ck) {
            DrawPFBox({ 0.0f, -0.055f - ck * 0.018f, 0.075f + ck * 0.004f },
                      { 0.042f, 0.003f, 0.040f }, cDark);
        }

        // Safety selector detent spring bump (right side, just forward of grip)
        DrawPFBox({ -0.029f, -0.006f, 0.030f }, { 0.003f, 0.004f, 0.012f }, cSteel);

        // 2-Segment Curved STANAG 30-Round Magazine with Stamped Ribs & Floorplate
        rlPushMatrix();
        rlTranslatef(sock.magWellPos.x + magOffset.x, sock.magWellPos.y + magOffset.y, sock.magWellPos.z + magOffset.z);
        rlRotatef(magPitch, 1.0f, 0.0f, 0.0f);
        Color stanagCol = (att.ammo == AMMO_STANDARD) ? Color{ 68, 72, 78, 255 } : magTint;
        DrawPFBox({ 0.0f, 0.02f, 0.0f }, { 0.034f, 0.080f, 0.064f }, stanagCol);
        DrawPFAngledBox({ 0.0f, -0.050f, -0.012f }, { 0.034f, 0.085f, 0.062f }, 12.0f, 0.0f, 0.0f, stanagCol);
        // Stamped vertical reinforcement ribs + witness holes + Ranger floorplate pull tab
        for (int r = -1; r <= 1; ++r) {
            DrawPFAngledBox({ 0.0f, -0.048f, -0.012f + r * 0.016f }, { 0.037f, 0.072f, 0.005f }, 12.0f, 0.0f, 0.0f, cDark);
        }
        // Witness holes (translucent square cutouts showing round count indicator)
        DrawPFBox({ 0.0f, -0.020f, -0.030f }, { 0.037f, 0.022f, 0.004f }, { 18, 22, 26, 255 });
        DrawPFBox({ 0.0f, -0.064f, -0.026f }, { 0.037f, 0.018f, 0.004f }, { 18, 22, 26, 255 });
        // Feed lips (two steel tabs at the top of the mag body)
        DrawPFBox({  0.012f, 0.064f, 0.004f }, { 0.008f, 0.012f, 0.052f }, cSteel);
        DrawPFBox({ -0.012f, 0.064f, 0.004f }, { 0.008f, 0.012f, 0.052f }, cSteel);
        // Ranger floorplate
        DrawPFAngledBox({ 0.0f, -0.092f, -0.021f }, { 0.038f, 0.010f, 0.068f }, 12.0f, 0.0f, 0.0f, cDark);
        DrawPFBox({ 0.0f, -0.096f, -0.040f }, { 0.040f, 0.010f, 0.012f }, cSteel); // Pull tab
        rlPopMatrix();

    } else if (stats.id == WPN_AK47) {
        // Stamped Steel AKM Receiver + Magwell Dimples + Trunnion Rivets + Ribbed Dust Cover
        DrawPFBox({ 0.0f, 0.006f, 0.00f }, { 0.054f, 0.058f, 0.25f }, cBody);
        DrawPFBox({ 0.0f, 0.002f, -0.055f }, { 0.057f, 0.020f, 0.048f }, cDark); // Stamped magwell dimple depression
        DrawPFReceiverPin({ 0.0f,  0.018f, -0.105f }, 0.056f, 0.0040f, cSteel); // Front trunnion rivet
        DrawPFReceiverPin({ 0.0f, -0.010f, -0.098f }, 0.056f, 0.0040f, cSteel);
        DrawPFReceiverPin({ 0.0f,  0.014f,  0.105f }, 0.056f, 0.0040f, cSteel); // Rear trunnion rivet
        // Sheet-metal AK fire selector lever on right side
        DrawPFAngledBox({ 0.029f, 0.010f, 0.035f }, { 0.005f, 0.014f, 0.095f }, -8.0f, 0.0f, 0.0f, cDark);
        // Ribbed Top Dust Cover + Rear Return Spring Button
        DrawPFAngledBox({ 0.0f, 0.038f, 0.00f }, { 0.046f, 0.020f, 0.24f }, 2.0f, 0.0f, 0.0f, cDark);
        for (int r = 0; r < 4; ++r) {
            DrawPFBox({ 0.0f, 0.042f, -0.06f + r * 0.042f }, { 0.050f, 0.012f, 0.008f }, cBody);
        }
        DrawPFBox({ 0.0f, 0.032f, 0.126f }, { 0.014f, 0.014f, 0.012f }, cSteel); // Recoil spring guide button

        // Rear Tangent Leaf Sight Block with Precision U-Notch & Range Slide
        DrawPFBox({ 0.0f, 0.038f, -0.125f }, { 0.036f, 0.024f, 0.052f }, cDark); // Tangent sight block base
        DrawPFReceiverPin({ 0.0f, 0.044f, -0.145f }, 0.030f, 0.0032f, cSteel); // Leaf hinge pin
        DrawPFAngledBox({ 0.0f, 0.051f, -0.122f }, { 0.022f, 0.006f, 0.046f }, -6.0f, 0.0f, 0.0f, cSteel); // Sight leaf ramp
        // Elevation slider with knurled grip buttons on sides
        DrawPFBox({ 0.0f, 0.054f, -0.125f }, { 0.028f, 0.008f, 0.012f }, cDark);
        DrawPFKnurledCylinder({ -0.014f, 0.054f, -0.125f }, { -0.018f, 0.054f, -0.125f }, 0.004f, 6, 2, cDark, cSteel);
        DrawPFKnurledCylinder({  0.014f, 0.054f, -0.125f }, {  0.018f, 0.054f, -0.125f }, 0.004f, 6, 2, cDark, cSteel);
        // Rear U-Notch Sight Blade at exact ironSightY = 0.066f
        DrawPFBox({ 0.0f, 0.0615f, -0.104f }, { 0.020f, 0.005f, 0.004f }, cDark); // Blade base
        DrawPFBox({ -0.0060f, 0.066f, -0.104f }, { 0.0045f, 0.007f, 0.004f }, cDark); // Left notch ear
        DrawPFBox({  0.0060f, 0.066f, -0.104f }, { 0.0045f, 0.007f, 0.004f }, cDark); // Right notch ear

        // If an optic is mounted on the AK, render its angled left-side Warsaw Pact dovetail rail bracket!
        if (att.optic != OPTIC_IRON) {
            DrawPFBox({ -0.028f, 0.008f, sock.opticMountPos.z }, { 0.006f, 0.018f, 0.068f }, cSteel);
            DrawPFAngledBox({ -0.024f, 0.026f, sock.opticMountPos.z }, { 0.008f, 0.044f, 0.055f }, 0.0f, 0.0f, -18.0f, cSteel);
            DrawPFPicatinnyRail({ 0.0f, sock.opticMountPos.y - 0.002f, sock.opticMountPos.z }, 0.038f, 0.008f, 0.105f, 6, cDark, cPolymer);
        }
        // Checkered Wooden Pistol Grip + Steel Ferrule + Trigger Guard + Mag Catch Paddle
        DrawPFBox({ 0.0f, -0.025f, 0.068f }, { 0.040f, 0.012f, 0.048f }, cSteel);
        DrawPFAngledBox({ 0.0f, -0.070f, 0.085f }, { 0.038f, 0.105f, 0.046f }, -16.0f, 0.0f, 0.0f, cAccent);
        DrawPFTriggerAssembly({ 0.0f, -0.040f, 0.022f }, 0.018f, 0.052f, cDark, cSteel);
        DrawPFAngledBox({ 0.0f, -0.036f, -0.014f }, { 0.016f, 0.022f, 0.008f }, 18.0f, 0.0f, 0.0f, cSteel); // Mag catch paddle

        // Iconic Angled Wooden AK Buttstock + Sling Swivel + Ribbed Metal Buttplate
        DrawPFAngledBox({ 0.0f, -0.012f, 0.24f }, { 0.046f, 0.075f, 0.24f }, -9.0f, 0.0f, 0.0f, cAccent);
        DrawPFBox({ -0.025f, -0.028f, 0.31f }, { 0.008f, 0.016f, 0.018f }, cSteel); // Side sling loop
        DrawPFBox({ 0.0f, -0.028f, 0.36f }, { 0.048f, 0.102f, 0.018f }, cDark);

        // Two-Piece Laminated Wood Handguard with Finger Swell Grooves & Steel Retaining Caps
        DrawPFBox({ 0.0f, 0.002f, -0.23f }, { 0.052f, 0.044f, 0.21f }, cAccent);
        DrawPFBox({ 0.0f, 0.008f, -0.23f }, { 0.056f, 0.016f, 0.14f }, cAccent); // Palm swell bulge
        DrawPFBox({ 0.0f, 0.004f, -0.338f }, { 0.054f, 0.046f, 0.014f }, cDark); // Front lower handguard retainer cap
        DrawPFBox({ 0.0f, 0.032f, -0.22f }, { 0.040f, 0.024f, 0.18f }, cAccent);
        DrawPFBox({ 0.0f, 0.020f, -0.20f }, { 0.048f, 0.008f, 0.032f }, cDark); // Vent slot between upper/lower handguard
        DrawPFBox({ 0.0f, 0.020f, -0.25f }, { 0.048f, 0.008f, 0.032f }, cDark);

        // Cylindrical Rifle Barrel + Under-Barrel Cleaning Rod + Upper Gas Tube + Angled Gas Block
        DrawPFCylinder({ 0.0f, 0.016f, -0.33f }, sock.muzzleTipPos, 0.012f, 0.011f, 10, cDark);
        DrawPFCylinder({ 0.0f, 0.002f, -0.34f }, { 0.0f, 0.002f, sock.muzzleTipPos.z + 0.03f }, 0.0035f, 0.0035f, 6, cSteel); // Cleaning rod!
        DrawPFCylinder({ 0.0f, 0.034f, -0.31f }, { 0.0f, 0.032f, -0.45f }, 0.010f, 0.010f, 8, cDark);
        DrawPFAngledBox({ 0.0f, 0.024f, -0.45f }, { 0.020f, 0.036f, 0.030f }, -22.0f, 0.0f, 0.0f, cDark);

        // Classic AK Angled Front Sight Block + Solid Barrel Sleeve + Protective Ears
        float fsbZ = sock.muzzleTipPos.z + 0.05f;
        // 1. Solid sleeve wrapping around barrel (y = 0.016f, r = 0.011f) with dual pins (zero air gap!)
        DrawPFCylinder({ 0.0f, 0.016f, fsbZ - 0.018f }, { 0.0f, 0.016f, fsbZ + 0.018f }, 0.0135f, 0.0135f, 10, cDark);
        DrawPFReceiverPin({ 0.0f, 0.016f, fsbZ - 0.008f }, 0.029f, 0.0028f, cSteel);
        DrawPFReceiverPin({ 0.0f, 0.016f, fsbZ + 0.008f }, 0.029f, 0.0028f, cSteel);
        // 2. Solid triangular upright tower body from sleeve up to sight deck (no gap)
        DrawPFBox({ 0.0f, 0.042f, fsbZ }, { 0.014f, 0.034f, 0.022f }, cDark);
        DrawPFAngledBox({ 0.0f, 0.044f, fsbZ + 0.005f }, { 0.013f, 0.036f, 0.012f }, 16.0f, 0.0f, 0.0f, cDark); // Angled rear strut
        // 3. Platform deck & cylindrical sight post drum well
        DrawPFBox({ 0.0f, 0.058f, fsbZ - 0.002f }, { 0.018f, 0.004f, 0.016f }, cDark);
        DrawPFCylinder({ -0.009f, 0.058f, fsbZ - 0.002f }, { 0.009f, 0.058f, fsbZ - 0.002f }, 0.0045f, 0.0045f, 8, cSteel); // Windage drum
        // 4. Protective curved ears flaring up to y = 0.072f
        DrawPFAngledBox({ -0.0078f, 0.065f, fsbZ - 0.002f }, { 0.0020f, 0.014f, 0.015f }, 0.0f, 0.0f, -14.0f, cDark);
        DrawPFAngledBox({  0.0078f, 0.065f, fsbZ - 0.002f }, { 0.0020f, 0.014f, 0.015f }, 0.0f, 0.0f,  14.0f, cDark);
        // 5. Cylindrical front sight post & luminous tritium bead (exact ironSightY = 0.066f)
        DrawPFCylinder({ 0.0f, 0.058f, fsbZ - 0.002f }, { 0.0f, 0.066f, fsbZ - 0.002f }, 0.0018f, 0.0018f, 6, cDark);
        DrawCubeV({ 0.0f, 0.066f, fsbZ - 0.002f }, { 0.0016f, 0.0016f, 0.0016f }, Color{ 70, 255, 110, 255 });
        // Bayonet lug bracket under the FSB + cleaning rod guide hole
        DrawPFBox({ 0.0f, 0.008f, fsbZ - 0.010f }, { 0.016f, 0.012f, 0.024f }, cDark);
        DrawPFBox({ 0.0f, 0.002f, fsbZ - 0.016f }, { 0.014f, 0.008f, 0.010f }, cSteel);
        // Muzzle Attachment: AK slant compensator (only when no barrel attachment)
        if (att.barrel == BARREL_NONE || att.barrel == BARREL_LONG) {
            DrawPFAngledBox(Vector3Add(sock.muzzleTipPos, { 0, -0.002f, -0.018f }), { 0.018f, 0.018f, 0.032f }, 28.0f, 0.0f, 0.0f, cDark);
            // Slant ports cut through the device body
            DrawPFBox(Vector3Add(sock.muzzleTipPos, { 0.0f, 0.004f, -0.022f }), { 0.020f, 0.008f, 0.008f }, cBody);
        }
        // Gas piston rod (visible above handguard when cycling)
        float pistonOffset = boltProgress * 0.06f;
        DrawPFCylinder({ 0.0f, 0.034f, -0.31f + pistonOffset }, { 0.0f, 0.034f, -0.24f + pistonOffset }, 0.008f, 0.008f, 7, cChrome);
        // Cleaning rod tip keeper (small cap at the muzzle end of the cleaning rod)
        DrawPFCylinder({ 0.0f, 0.002f, sock.muzzleTipPos.z + 0.028f }, { 0.0f, 0.002f, sock.muzzleTipPos.z + 0.020f }, 0.005f, 0.005f, 6, cDark);

        // Reciprocating Right-Side Polished Steel AK Bolt Carrier & Charging Handle Knob
        float akBoltZ = -0.05f + boltProgress * 0.095f;
        DrawPFBox({ 0.024f, 0.024f, akBoltZ }, { 0.016f, 0.018f, 0.082f }, cChrome);
        DrawPFCylinder({ 0.028f, 0.024f, akBoltZ - 0.025f }, { 0.048f, 0.024f, akBoltZ - 0.025f }, 0.006f, 0.005f, 8, cChrome);
        // Knurled charging handle ball
        DrawPFKnurledCylinder({ 0.042f, 0.024f, akBoltZ - 0.025f }, { 0.052f, 0.024f, akBoltZ - 0.025f }, 0.007f, 8, 3, cDark, cChrome);

        // 3-Segment Iconic Curved AK Banana Magazine with Spine Rib & Baseplate!
        rlPushMatrix();
        rlTranslatef(sock.magWellPos.x + magOffset.x, sock.magWellPos.y + magOffset.y, sock.magWellPos.z + magOffset.z);
        rlRotatef(magPitch, 1.0f, 0.0f, 0.0f);
        Color bakelite = (att.ammo == AMMO_STANDARD) ? Color{ 176, 82, 28, 255 } : magTint;
        DrawPFAngledBox({ 0.0f,  0.026f,  0.000f }, { 0.036f, 0.064f, 0.066f },  6.0f, 0.0f, 0.0f, bakelite);
        DrawPFAngledBox({ 0.0f, -0.028f, -0.012f }, { 0.036f, 0.068f, 0.066f }, 18.0f, 0.0f, 0.0f, bakelite);
        DrawPFAngledBox({ 0.0f, -0.080f, -0.036f }, { 0.036f, 0.068f, 0.066f }, 30.0f, 0.0f, 0.0f, bakelite);
        // Rear locking lug spine + stamped side ribs
        DrawPFAngledBox({ 0.0f, -0.028f,  0.022f }, { 0.020f, 0.145f, 0.008f }, 18.0f, 0.0f, 0.0f, cDark);
        // Cartridge stack hint — visible brass tips at the top of the magazine
        for (int cx = -1; cx <= 1; cx += 2) {
            DrawPFBox({ cx * 0.008f, 0.046f, 0.018f }, { 0.008f, 0.008f, 0.016f }, cBrass);
        }
        // Floorplate
        DrawPFAngledBox({ 0.0f, -0.112f, -0.054f }, { 0.038f, 0.010f, 0.070f }, 30.0f, 0.0f, 0.0f, cDark);
        rlPopMatrix();

    } else if (stats.id == WPN_INTERVENTION) {
        // Heavy CNC-Machined CheyTac M200 Chassis, Flared Magwell & Top Picatinny Rail
        DrawPFBox({ 0.0f, 0.014f, 0.00f }, { 0.066f, 0.078f, 0.32f }, cBody);
        DrawPFBox({ 0.0f, -0.012f, -0.04f }, { 0.070f, 0.042f, 0.13f }, cAccent); // Reinforced magwell block
        for (int p = 0; p < 4; ++p) {
            DrawPFReceiverPin({ 0.0f, -0.008f, -0.09f + p * 0.045f }, 0.070f, 0.004f, cDark);
        }
        DrawPFPicatinnyRail({ 0.0f, 0.055f, -0.05f }, 0.038f, 0.010f, 0.36f, 14, cDark, cPolymer);
        DrawPFAngledBox({ 0.0f, -0.072f, 0.11f }, { 0.042f, 0.110f, 0.050f }, -14.0f, 0.0f, 0.0f, cDark);
        DrawPFTriggerAssembly({ 0.0f, -0.042f, 0.052f }, 0.020f, 0.056f, cDark, cChrome);

        // Skeletonized Dual-Rod Collapsible Sniper Stock + Thumbscrew Cheek Rest + Rear Monopod
        DrawPFCylinder({ -0.018f, 0.024f, 0.16f }, { -0.018f, 0.024f, 0.42f }, 0.008f, 0.008f, 8, cSteel);
        DrawPFCylinder({  0.018f, 0.024f, 0.16f }, {  0.018f, 0.024f, 0.42f }, 0.008f, 0.008f, 8, cSteel);
        DrawPFBox({ 0.0f, 0.042f, 0.29f }, { 0.050f, 0.028f, 0.14f }, cAccent); // Cheek rest
        DrawPFReceiverPin({ 0.0f, 0.032f, 0.25f }, 0.056f, 0.006f, cDark); // Cheek adjustment thumbscrews
        DrawPFReceiverPin({ 0.0f, 0.032f, 0.33f }, 0.056f, 0.006f, cDark);
        DrawPFBox({ 0.0f, 0.002f, 0.42f }, { 0.054f, 0.130f, 0.028f }, cDark);  // Thick recoil pad
        DrawPFCylinder({ 0.0f, -0.02f, 0.39f }, { 0.0f, -0.105f, 0.39f }, 0.010f, 0.010f, 8, cDark);
        DrawPFCylinder({ 0.0f, -0.06f, 0.39f }, { 0.0f, -0.08f, 0.39f }, 0.014f, 0.014f, 8, cSteel); // Monopod locking collar
        DrawPFBox({ 0.0f, -0.108f, 0.39f }, { 0.026f, 0.010f, 0.026f }, cDark); // Monopod foot

        // Tubular Perforated Cooling Shroud + Offset Carry Handle + Detailed Folding Bipod
        DrawPFCylinder({ 0.0f, 0.022f, -0.16f }, { 0.0f, 0.022f, -0.46f }, 0.032f, 0.032f, 12, cAccent);
        for (int v = 0; v < 5; ++v) {
            float vz = -0.20f - v * 0.052f;
            DrawPFBox({ 0.0f, 0.028f, vz }, { 0.066f, 0.012f, 0.026f }, cDark); // Shroud vent slots
        }
        // Additional hex-pattern side vent holes on shroud (left and right faces)
        for (int hv = 0; hv < 4; ++hv) {
            float hz = -0.215f - hv * 0.052f;
            DrawPFBox({  0.033f, 0.022f, hz }, { 0.001f, 0.014f, 0.016f }, cDark);
            DrawPFBox({ -0.033f, 0.022f, hz }, { 0.001f, 0.014f, 0.016f }, cDark);
        }
        DrawPFAngledBox({ 0.030f, 0.052f, -0.25f }, { 0.010f, 0.034f, 0.10f }, 0.0f, 0.0f, -24.0f, cDark);
        // Bipod Yoke + Dual Notched Legs + External Return Springs + Claw Feet with Rubber Pads
        DrawPFBox({ 0.0f, -0.010f, -0.44f }, { 0.064f, 0.020f, 0.028f }, cDark);
        for (int s = -1; s <= 1; s += 2) {
            float bx = s * 0.028f;
            DrawPFCylinder({ bx, -0.014f, -0.44f }, { bx, -0.018f, -0.20f }, 0.009f, 0.008f, 8, cDark);
            DrawPFKnurledCylinder({ bx, -0.018f, -0.28f }, { bx, -0.018f, -0.17f }, 0.006f, 8, 4, cSteel, cDark); // Notched spring section
            DrawPFBox({ bx, -0.018f, -0.17f }, { 0.018f, 0.022f, 0.014f }, cDark); // Bipod ski foot
            DrawPFBox({ bx, -0.019f, -0.166f }, { 0.018f, 0.018f, 0.012f }, { 22, 22, 22, 255 }); // Rubber pad on foot
        }
        // Bipod cant angle lock screw (center of yoke)
        DrawPFCylinder({ 0.0f, -0.006f, -0.44f }, { 0.0f, -0.006f, -0.468f }, 0.005f, 0.005f, 6, cSteel);

        // Long Heavy Fluted Free-Float Barrel + 3-Chamber Tank-Style Muzzle Brake
        DrawPFCylinder({ 0.0f, 0.024f, -0.46f }, sock.muzzleTipPos, 0.016f, 0.013f, 12, cSteel);
        // Fluted barrel longitudinal grooves
        DrawPFBox({ -0.013f, 0.026f, -0.66f }, { 0.004f, 0.006f, 0.34f }, cDark);
        DrawPFBox({  0.013f, 0.026f, -0.66f }, { 0.004f, 0.006f, 0.34f }, cDark);
        DrawPFBox({  0.0f,   0.037f, -0.66f }, { 0.006f, 0.004f, 0.34f }, cDark);
        if (att.barrel == BARREL_NONE || att.barrel == BARREL_LONG) {
            Vector3 mbCenter = Vector3Add(sock.muzzleTipPos, { 0, 0, -0.042f });
            DrawPFBox(mbCenter, { 0.040f, 0.032f, 0.088f }, cDark);
            for (int b = -1; b <= 1; ++b) {
                DrawPFAngledBox({ 0.0f, mbCenter.y, mbCenter.z + b * 0.024f },
                                { 0.044f, 0.018f, 0.014f }, 0.0f, 16.0f, 0.0f, cBody);
            }
        }

        // Animated Bolt-Action Fluted Cylinder + Swept Handle + Oversized Tactical Knob
        float boltLiftDeg = std::sin(boltProgress * PI) * 55.0f;
        float boltSlideZ  = 0.06f + boltProgress * 0.11f;
        rlPushMatrix();
        rlTranslatef(0.0f, 0.028f, boltSlideZ);
        rlRotatef(boltLiftDeg, 0.0f, 0.0f, 1.0f);
        DrawPFKnurledCylinder({ 0.0f, 0.0f, -0.06f }, { 0.0f, 0.0f, 0.04f }, 0.011f, 8, 5, cChrome, cDark);
        DrawPFCylinder({ 0.0f, 0.0f, 0.0f }, { 0.058f, -0.016f, 0.012f }, 0.007f, 0.007f, 8, cSteel);
        DrawSphere({ 0.062f, -0.018f, 0.014f }, 0.014f, cDark);
        DrawPFBox({ 0.062f, -0.006f, 0.014f }, { 0.016f, 0.005f, 0.016f }, cDark); // Flat machined spot
        rlPopMatrix();

        // Heavy .408 CheyTac Ribbed Steel Box Magazine + Carry Handle D-Ring
        rlPushMatrix();
        rlTranslatef(sock.magWellPos.x + magOffset.x, sock.magWellPos.y + magOffset.y, sock.magWellPos.z + magOffset.z);
        Color cheyMag = (att.ammo == AMMO_STANDARD) ? cAccent : magTint;
        DrawPFBox({ 0.0f, 0.0f, 0.0f }, { 0.040f, 0.135f, 0.092f }, cheyMag);
        for (int r = -1; r <= 1; ++r) {
            DrawPFBox({ 0.0f, 0.0f, r * 0.024f }, { 0.043f, 0.120f, 0.008f }, cDark);
        }
        DrawPFBox({ 0.0f, -0.068f, 0.0f }, { 0.044f, 0.010f, 0.098f }, cDark);
        rlPopMatrix();

    } else if (stats.id == WPN_MP7) {
        // Compact HK MP7A1 Polymer PDW Body + Ventilated Nose Cone + Side Picatinny Rails
        DrawPFBox({ 0.0f, 0.016f, -0.06f }, { 0.050f, 0.064f, 0.34f }, cBody);
        DrawPFAngledBox({ 0.0f, 0.032f, -0.22f }, { 0.046f, 0.032f, 0.055f }, -26.0f, 0.0f, 0.0f, cBody);
        for (int v = 0; v < 3; ++v) {
            DrawPFBox({ 0.0f, 0.030f, -0.14f - v * 0.032f }, { 0.053f, 0.010f, 0.018f }, cDark); // Cooling vents
        }
        // Full-Length Top Picatinny Rail + Dual Side Accessory Rails
        DrawPFPicatinnyRail({  0.0f,   0.050f, -0.06f }, 0.034f, 0.010f, 0.33f, 14, cDark, cPolymer);
        DrawPFPicatinnyRail({ -0.027f, 0.016f, -0.16f }, 0.008f, 0.014f, 0.11f, 5,  cDark, cPolymer);
        DrawPFPicatinnyRail({  0.027f, 0.016f, -0.16f }, 0.008f, 0.014f, 0.11f, 5,  cDark, cPolymer);
        // Ambidextrous Fire Selector + Bolt Release Lever + Rear HK Latch Charging Handle
        DrawPFAngledBox({ -0.027f, -0.004f, 0.02f }, { 0.006f, 0.010f, 0.022f }, -20.0f, 0.0f, 0.0f, cSteel);
        DrawPFAngledBox({  0.027f, -0.004f, 0.02f }, { 0.006f, 0.010f, 0.022f }, -20.0f, 0.0f, 0.0f, cSteel);
        DrawPFBox({ 0.0f, 0.036f, 0.115f + boltProgress * 0.045f }, { 0.042f, 0.012f, 0.018f }, cDark);

        // Center-Balanced Textured Pistol Grip + Trigger Guard + Extended 40-Round Mag Basepad
        DrawPFAngledBox({ 0.0f, -0.060f, 0.04f }, { 0.040f, 0.118f, 0.052f }, -8.0f, 0.0f, 0.0f, cAccent);
        DrawPFTriggerAssembly({ 0.0f, -0.028f, -0.006f }, 0.016f, 0.046f, cDark, cSteel);
        rlPushMatrix();
        rlTranslatef(sock.magWellPos.x + magOffset.x, sock.magWellPos.y + magOffset.y, sock.magWellPos.z + magOffset.z);
        DrawPFAngledBox({ 0.0f, -0.048f, 0.0f }, { 0.030f, 0.105f, 0.040f }, -8.0f, 0.0f, 0.0f, magTint);
        DrawPFAngledBox({ 0.0f, -0.098f, 0.007f }, { 0.035f, 0.016f, 0.046f }, -8.0f, 0.0f, 0.0f, cDark); // Extended bumper pad
        rlPopMatrix();

        // Collapsible Dual-Rail Strut Stock + Release Latches + Curved Buttpad
        DrawPFBox({ -0.026f, 0.016f, 0.19f }, { 0.006f, 0.012f, 0.18f }, cSteel);
        DrawPFBox({  0.026f, 0.016f, 0.19f }, { 0.006f, 0.012f, 0.18f }, cSteel);
        DrawPFBox({ 0.0f, 0.016f, 0.105f }, { 0.056f, 0.036f, 0.022f }, cDark); // Stock locking cap
        DrawPFBox({ 0.0f, 0.004f, 0.28f }, { 0.046f, 0.112f, 0.024f }, cDark);
        // Stock strut locking detent pins (two push-button releases in the cap)
        DrawPFReceiverPin({ 0.0f, 0.016f, 0.118f }, 0.060f, 0.0045f, cSteel);

        // MP7 Flip-Up Diopter Rear Sight & Hooded Front Sight on Top Rail
        if (att.optic == OPTIC_IRON) {
            // 1. Rear Flip-Up Peep Sight clamped to top rail at z = 0.085f
            DrawPFBox({ 0.0f, 0.054f, 0.085f }, { 0.024f, 0.006f, 0.020f }, cDark); // Clamp base
            DrawPFCylinder({ -0.012f, 0.054f, 0.085f }, { -0.016f, 0.054f, 0.085f }, 0.004f, 0.004f, 6, cSteel); // Clamp bolt
            DrawPFReceiverPin({ 0.0f, 0.056f, 0.078f }, 0.022f, 0.0025f, cSteel); // Flip hinge
            DrawPFKnurledCylinder({ 0.011f, 0.065f, 0.085f }, { 0.016f, 0.065f, 0.085f }, 0.0045f, 6, 2, cDark, cSteel); // Windage dial
            DrawPFBox({ 0.0f, 0.059f, 0.085f }, { 0.007f, 0.012f, 0.005f }, cDark); // Vertical stem
            // Rear Peep Ring centered at (0.0f, 0.068f, 0.085f)
            DrawPFBox({ -0.0050f, 0.068f, 0.085f }, { 0.0025f, 0.008f, 0.004f }, cDark);
            DrawPFBox({  0.0050f, 0.068f, 0.085f }, { 0.0025f, 0.008f, 0.004f }, cDark);
            DrawPFBox({  0.0f, 0.0642f, 0.085f }, { 0.008f, 0.0022f, 0.004f }, cDark);
            DrawPFBox({  0.0f, 0.0718f, 0.085f }, { 0.008f, 0.0022f, 0.004f }, cDark);

            // 2. Front Flip-Up Hooded Sight clamped to top rail at z = -0.220f
            DrawPFBox({ 0.0f, 0.054f, -0.220f }, { 0.022f, 0.006f, 0.020f }, cDark); // Clamp base
            DrawPFReceiverPin({ 0.0f, 0.056f, -0.212f }, 0.020f, 0.0025f, cSteel); // Flip hinge
            DrawPFBox({ 0.0f, 0.058f, -0.220f }, { 0.008f, 0.010f, 0.014f }, cDark); // Riser body
            // Curved HK protective hood ears
            DrawPFAngledBox({ -0.0062f, 0.067f, -0.220f }, { 0.0018f, 0.014f, 0.014f }, 0.0f, 0.0f, -16.0f, cDark);
            DrawPFAngledBox({  0.0062f, 0.067f, -0.220f }, { 0.0018f, 0.014f, 0.014f }, 0.0f, 0.0f,  16.0f, cDark);
            // Precision front needle post & luminous tritium bead (exact ironSightY = 0.068f)
            DrawPFBox({ 0.0f, 0.0645f, -0.220f }, { 0.0016f, 0.007f, 0.0016f }, cDark);
            DrawCubeV({ 0.0f, 0.068f, -0.220f }, { 0.0015f, 0.0015f, 0.0015f }, Color{ 70, 255, 110, 255 });
        } else {
            // Folded flat iron sights on top rail when optical sights are mounted
            DrawPFBox({ 0.0f, 0.054f, 0.085f }, { 0.022f, 0.004f, 0.024f }, cDark);
            DrawPFBox({ 0.0f, 0.054f, -0.220f }, { 0.020f, 0.004f, 0.024f }, cDark);
        }
        // Left side receiver serial number plate (decorative block)
        DrawPFBox({ -0.026f, 0.016f, 0.030f }, { 0.001f, 0.020f, 0.052f }, { 28, 32, 36, 255 });

        // Built-in Folding Front Vertical Grip (with hinge pin) when no custom underbarrel grip is attached
        if (att.grip == GRIP_NONE) {
            DrawPFReceiverPin({ 0.0f, -0.010f, -0.19f }, 0.046f, 0.005f, cSteel);
            DrawPFAngledBox({ 0.0f, -0.045f, -0.19f }, { 0.028f, 0.076f, 0.032f }, 10.0f, 0.0f, 0.0f, cAccent);
            // Grip checkering panels
            for (int gck = 0; gck < 4; ++gck) {
                DrawPFBox({ 0.0f, -0.028f - gck * 0.014f, -0.185f }, { 0.030f, 0.004f, 0.030f }, cDark);
            }
        }
        DrawPFCylinder({ 0.0f, 0.016f, -0.23f }, sock.muzzleTipPos, 0.011f, 0.011f, 10, cSteel);
        if (att.barrel == BARREL_NONE || att.barrel == BARREL_LONG) {
            DrawPFCylinder(sock.muzzleTipPos, Vector3Add(sock.muzzleTipPos, { 0, 0, -0.032f }), 0.013f, 0.013f, 8, cDark);
        }

    } else if (stats.id == WPN_REMINGTON870) {
        // Steel 870 Receiver + Angled Rear Tang + Ejection Port + Side-Saddle 4-Shell Carrier!
        DrawPFBox({ 0.0f, 0.014f, -0.01f }, { 0.052f, 0.064f, 0.24f }, cBody);
        DrawPFAngledBox({ 0.0f, 0.034f, 0.095f }, { 0.050f, 0.028f, 0.055f }, -26.0f, 0.0f, 0.0f, cBody);
        Color shellCol = (att.ammo == AMMO_SLUGS) ? Color{ 165, 45, 140, 255 }
                       : (att.ammo == AMMO_FLECHETTE) ? Color{ 45, 165, 95, 255 }
                       : Color{ 215, 38, 38, 255 };
        // Chrome bolt carrier + chambered shell visible inside ejection port
        DrawPFBox({ 0.025f, 0.020f, -0.04f + boltProgress * 0.055f }, { 0.010f, 0.018f, 0.064f }, cChrome);
        DrawPFBox({ 0.026f, 0.018f, -0.03f + boltProgress * 0.055f }, { 0.010f, 0.014f, 0.044f }, shellCol);
        // Left-side tactical shell saddle holding 4 spare 12-gauge shells with brass bases!
        DrawPFBox({ -0.028f, 0.014f, -0.01f }, { 0.006f, 0.042f, 0.145f }, cDark);
        for (int s = 0; s < 4; ++s) {
            float sz = -0.062f + s * 0.034f;
            DrawPFCylinder({ -0.034f, 0.038f, sz }, { -0.034f, 0.024f, sz }, 0.009f, 0.009f, 8, cBrass);
            DrawPFCylinder({ -0.034f, 0.024f, sz }, { -0.034f, -0.018f, sz }, 0.0085f, 0.0085f, 8, shellCol);
        }

        // Trigger Guard + Cross-Bolt Safety + Classic Angled Walnut Stock & Recoil Pad
        DrawPFTriggerAssembly({ 0.0f, -0.028f, 0.075f }, 0.016f, 0.050f, cDark, cSteel);
        DrawPFAngledBox({ 0.0f, -0.028f, 0.14f }, { 0.042f, 0.058f, 0.12f }, -22.0f, 0.0f, 0.0f, cAccent);
        DrawPFAngledBox({ 0.0f, -0.038f, 0.29f }, { 0.044f, 0.090f, 0.24f }, -8.0f, 0.0f, 0.0f, cAccent);
        DrawPFBox({ 0.0f, -0.048f, 0.41f }, { 0.046f, 0.112f, 0.024f }, cDark);

        // Dual Stacked Upper Barrel + Ventilated Top Rib + Lower Magazine Tube + Barrel Clamp
        DrawPFCylinder({ 0.0f, 0.026f, -0.13f }, sock.muzzleTipPos, 0.013f, 0.013f, 12, cBody);
        DrawPFBox({ 0.0f, 0.042f, -0.44f }, { 0.008f, 0.005f, 0.62f }, cDark); // Ventilated top rib spine
        for (int p = 0; p < 8; ++p) {
            DrawPFBox({ 0.0f, 0.038f, -0.17f - p * 0.076f }, { 0.006f, 0.008f, 0.014f }, cBody); // Vent rib posts
        }
        // Tactical Ghost Ring Rear Sight & Elevated Front Pedestal Ramp (ironSightY = 0.062f)
        if (att.optic == OPTIC_IRON) {
            // 1. Receiver-Mounted Ghost Ring Rear Sight at z = 0.035f
            DrawPFBox({ 0.0f, 0.048f, 0.035f }, { 0.024f, 0.006f, 0.022f }, cDark); // Steel clamp base
            DrawPFCylinder({ -0.012f, 0.048f, 0.035f }, { -0.016f, 0.048f, 0.035f }, 0.0035f, 0.0035f, 6, cSteel); // Windage screw
            DrawPFAngledBox({ -0.0075f, 0.059f, 0.035f }, { 0.0022f, 0.016f, 0.014f }, 0.0f, 0.0f, -14.0f, cDark); // Protective wings
            DrawPFAngledBox({  0.0075f, 0.059f, 0.035f }, { 0.0022f, 0.016f, 0.014f }, 0.0f, 0.0f,  14.0f, cDark);
            // Steel Ghost Ring Aperture centered at (0.0f, 0.062f, 0.035f)
            DrawPFBox({ 0.0f, 0.0578f, 0.035f }, { 0.008f, 0.0024f, 0.004f }, cDark);
            DrawPFBox({ 0.0f, 0.0662f, 0.035f }, { 0.008f, 0.0024f, 0.004f }, cDark);
            DrawPFBox({ -0.0045f, 0.062f, 0.035f }, { 0.0024f, 0.008f, 0.004f }, cDark);
            DrawPFBox({  0.0045f, 0.062f, 0.035f }, { 0.0024f, 0.008f, 0.004f }, cDark);

            // 2. Elevated Front Sight Pedestal & High-Viz Bead welded to rib (zero air gap)
            DrawPFBox({ 0.0f, 0.048f, sock.muzzleTipPos.z + 0.025f }, { 0.010f, 0.016f, 0.026f }, cDark); // Solid riser
            DrawPFAngledBox({ 0.0f, 0.052f, sock.muzzleTipPos.z + 0.034f }, { 0.009f, 0.018f, 0.012f }, 24.0f, 0.0f, 0.0f, cDark); // Front ramp
            DrawPFAngledBox({ -0.006f, 0.061f, sock.muzzleTipPos.z + 0.025f }, { 0.0018f, 0.012f, 0.018f }, 0.0f, 0.0f, -12.0f, cDark); // Wings
            DrawPFAngledBox({  0.006f, 0.061f, sock.muzzleTipPos.z + 0.025f }, { 0.0018f, 0.012f, 0.018f }, 0.0f, 0.0f,  12.0f, cDark);
            DrawPFBox({ 0.0f, 0.059f, sock.muzzleTipPos.z + 0.025f }, { 0.0020f, 0.008f, 0.014f }, cDark); // Center blade
            DrawCubeV({ 0.0f, 0.062f, sock.muzzleTipPos.z + 0.025f }, { 0.0018f, 0.0018f, 0.008f }, Color{ 70, 255, 110, 255 }); // Fiber-optic pip
        } else {
            // Receiver Picatinny Rail when optic is attached
            DrawPFPicatinnyRail({ 0.0f, sock.opticMountPos.y - 0.002f, sock.opticMountPos.z }, 0.032f, 0.008f, 0.12f, 7, cDark, cSteel);
        }
        DrawPFCylinder({ 0.0f, -0.002f, -0.13f }, { 0.0f, -0.002f, -0.68f }, 0.012f, 0.012f, 12, cSteel);
        DrawPFCylinder({ 0.0f, -0.002f, -0.68f }, { 0.0f, -0.002f, -0.705f }, 0.0135f, 0.0135f, 10, cDark); // Knurled mag tube cap
        DrawPFBox({ 0.0f, 0.012f, -0.62f }, { 0.028f, 0.044f, 0.020f }, cDark); // Figure-8 barrel/mag clamp

        // Animated Sliding Ribbed Corncob Wooden Pump Forend + Dual Action Bars
        float pumpZ = -0.34f + boltProgress * 0.11f;
        DrawPFBox({ -0.016f, -0.002f, pumpZ + 0.14f }, { 0.004f, 0.008f, 0.18f }, cChrome);
        DrawPFBox({  0.016f, -0.002f, pumpZ + 0.14f }, { 0.004f, 0.008f, 0.18f }, cChrome);
        DrawPFCylinder({ 0.0f, -0.004f, pumpZ + 0.095f }, { 0.0f, -0.004f, pumpZ - 0.095f }, 0.023f, 0.023f, 12, cAccent);
        // Deep corncob knurling — alternating wide & narrow contrast rings
        for (int g = -4; g <= 4; ++g) {
            float gz = pumpZ + g * 0.022f;
            Color ringC = (g % 2 == 0) ? cDark : Color{ 165, 88, 42, 255 };
            DrawPFCylinder({ 0.0f, -0.004f, gz + 0.005f }, { 0.0f, -0.004f, gz - 0.005f }, 0.025f, 0.025f, 12, ringC);
        }
        // Barrel-to-receiver cross pin bolt (visible on right side of receiver)
        DrawPFReceiverPin({ 0.0f, 0.014f, -0.06f }, 0.056f, 0.0040f, cDark);
        // Dual extractor claws inside ejection port
        DrawPFBox({  0.022f, 0.034f, -0.040f + boltProgress * 0.055f }, { 0.006f, 0.006f, 0.018f }, cChrome);
        DrawPFBox({ -0.022f, 0.034f, -0.040f + boltProgress * 0.055f }, { 0.006f, 0.006f, 0.018f }, cChrome);
        // Cross-bolt safety indicator (red/white ring visible in button)
        DrawPFCylinder({ 0.0f, -0.004f, 0.052f }, { 0.0f, -0.004f, 0.066f }, 0.007f, 0.007f, 8, cChrome);
        DrawPFBox({ 0.0f, -0.004f, 0.065f }, { 0.006f, 0.006f, 0.004f }, { 215, 38, 38, 255 }); // Red = FIRE indicator

    } else if (stats.id == WPN_M60) {
        // Heavy Boxy M60 General Purpose Machine Gun Receiver + Riveted Side Plates + Hinged Feed Cover
        DrawPFBox({ 0.0f, 0.014f, -0.02f }, { 0.070f, 0.080f, 0.32f }, cBody);
        DrawPFBox({ 0.0f, 0.016f, -0.02f }, { 0.074f, 0.044f, 0.26f }, cDark); // Stamped side reinforcement plate
        DrawPFBox({ 0.0f, 0.056f, -0.04f }, { 0.060f, 0.016f, 0.24f }, cDark); // Top feed tray cover
        DrawPFBox({ 0.0f, 0.065f, -0.04f }, { 0.036f, 0.008f, 0.21f }, cBody); // Stamped top cover ridge
        DrawPFReceiverPin({ 0.0f, 0.054f, -0.155f }, 0.064f, 0.005f, cSteel);  // Feed cover front hinge pin
        DrawPFReceiverPin({ 0.0f, 0.054f,  0.075f }, 0.068f, 0.006f, cSteel);  // Feed cover rear latch
        // Folding Ladder Leaf Rear Sight on Feed Cover (ironSightY = 0.078f)
        if (att.optic == OPTIC_IRON) {
            DrawPFBox({ 0.0f, 0.058f, 0.055f }, { 0.026f, 0.008f, 0.016f }, cDark); // Hinge base
            DrawPFReceiverPin({ 0.0f, 0.058f, 0.050f }, 0.028f, 0.0035f, cSteel); // Hinge pin
            DrawPFKnurledCylinder({ 0.013f, 0.072f, 0.055f }, { 0.018f, 0.072f, 0.055f }, 0.0045f, 6, 2, cDark, cSteel); // Thumb knob
            // Vertical ladder stanchions
            DrawPFBox({ -0.0085f, 0.076f, 0.055f }, { 0.0035f, 0.028f, 0.006f }, cDark);
            DrawPFBox({  0.0085f, 0.076f, 0.055f }, { 0.0035f, 0.028f, 0.006f }, cDark);
            DrawPFBox({  0.0f,    0.089f, 0.055f }, { 0.020f, 0.003f, 0.006f }, cDark); // Top arch
            // Sliding battle sight notch bar centered at (0.0f, 0.078f, 0.055f)
            DrawPFBox({ -0.0055f, 0.078f, 0.055f }, { 0.0035f, 0.008f, 0.004f }, cSteel);
            DrawPFBox({  0.0055f, 0.078f, 0.055f }, { 0.0035f, 0.008f, 0.004f }, cSteel);
            DrawPFBox({  0.0f,   0.0745f, 0.055f }, { 0.014f, 0.0025f, 0.004f }, cSteel);
        } else {
            // Folded ladder sight flat on feed tray cover
            DrawPFBox({ 0.0f, 0.058f, 0.055f }, { 0.022f, 0.005f, 0.028f }, cDark);
        }
        // Pistol Grip + Trigger Guard + Heavy Buffered Shoulder Stock with Hinged Buttplate Rest
        DrawPFAngledBox({ 0.0f, -0.076f, 0.09f }, { 0.042f, 0.110f, 0.050f }, -14.0f, 0.0f, 0.0f, cDark);
        DrawPFTriggerAssembly({ 0.0f, -0.042f, 0.038f }, 0.020f, 0.054f, cDark, cSteel);
        DrawPFBox({ 0.0f, 0.008f, 0.27f }, { 0.058f, 0.092f, 0.26f }, cDark);
        DrawPFBox({ 0.0f, 0.058f, 0.37f }, { 0.060f, 0.010f, 0.068f }, cSteel); // Hinged shoulder rest flap

        // Ribbed Polymer Forend + Perforated Barrel Heat Shield + Top Carry Handle!
        DrawPFBox({ 0.0f, 0.002f, -0.28f }, { 0.064f, 0.056f, 0.24f }, cAccent);
        DrawPFBox({ 0.0f, 0.038f, -0.28f }, { 0.054f, 0.024f, 0.23f }, cDark); // Perforated metal heat shield
        for (int h = 0; h < 5; ++h) {
            DrawPFBox({ 0.0f, 0.042f, -0.19f - h * 0.042f }, { 0.058f, 0.010f, 0.020f }, cBody);
        }
        // Offset Carry Handle Loop above center of gravity
        DrawPFAngledBox({ 0.028f, 0.070f, -0.20f }, { 0.008f, 0.044f, 0.010f }, 0.0f, 0.0f, -20.0f, cSteel);
        DrawPFAngledBox({ 0.028f, 0.070f, -0.28f }, { 0.008f, 0.044f, 0.010f }, 0.0f, 0.0f, -20.0f, cSteel);
        DrawPFCylinder({ 0.036f, 0.088f, -0.19f }, { 0.036f, 0.088f, -0.29f }, 0.011f, 0.011f, 8, cDark);

        // Heavy Barrel + Under-Slung Gas Cylinder + Folded Bipod Legs + Iconic Triangular Front Sight
        DrawPFCylinder({ 0.0f, 0.022f, -0.40f }, sock.muzzleTipPos, 0.015f, 0.014f, 12, cSteel);
        // Barrel fluting grooves (4 longitudinal flutes for weight reduction & cooling)
        for (int fl = 0; fl < 4; ++fl) {
            float fa = fl * 0.0f; // axial offset for each flute (all parallel)
            DrawPFBox({ (fl < 2 ? -0.016f : 0.016f), (fl % 2 == 0 ? 0.022f : 0.022f),
                        -0.60f + (fl < 2 ? 0.0f : 0.0f) }, { 0.003f, 0.005f, 0.32f }, cDark);
            (void)fa;
        }
        DrawPFBox({ -0.016f, 0.026f, -0.60f }, { 0.003f, 0.004f, 0.30f }, cDark); // Left flute
        DrawPFBox({  0.016f, 0.026f, -0.60f }, { 0.003f, 0.004f, 0.30f }, cDark); // Right flute
        DrawPFCylinder({ 0.0f, -0.008f, -0.40f }, { 0.0f, -0.008f, -0.68f }, 0.013f, 0.013f, 10, cDark);
        // Bipod legs with knurled adjustment section
        DrawPFKnurledCylinder({ -0.028f, -0.004f, -0.68f }, { -0.028f, -0.008f, -0.52f }, 0.009f, 8, 4, cDark, cSteel);
        DrawPFKnurledCylinder({  0.028f, -0.004f, -0.68f }, {  0.028f, -0.008f, -0.52f }, 0.009f, 8, 4, cDark, cSteel);
        DrawPFCylinder({ -0.028f, -0.008f, -0.52f }, { -0.028f, -0.008f, -0.42f }, 0.009f, 0.009f, 8, cDark);
        DrawPFCylinder({  0.028f, -0.008f, -0.52f }, {  0.028f, -0.008f, -0.42f }, 0.009f, 0.009f, 8, cDark);
        // Bipod rubber feet
        DrawPFBox({ -0.028f, -0.008f, -0.682f }, { 0.014f, 0.016f, 0.010f }, { 22, 22, 22, 255 });
        DrawPFBox({  0.028f, -0.008f, -0.682f }, { 0.014f, 0.016f, 0.010f }, { 22, 22, 22, 255 });
        // Heat shield rivets (decorative row of rivets on perforated shield)
        for (int hv = 0; hv < 4; ++hv) {
            float hz = -0.185f - hv * 0.042f;
            DrawPFCylinder({ -0.030f, 0.042f, hz }, { -0.030f, 0.046f, hz }, 0.003f, 0.003f, 5, cSteel);
            DrawPFCylinder({  0.030f, 0.042f, hz }, {  0.030f, 0.046f, hz }, 0.003f, 0.003f, 5, cSteel);
        }
        // Feed chute exit ramp + case deflector on right side of receiver
        DrawPFAngledBox({ 0.038f, 0.022f, -0.085f }, { 0.004f, 0.024f, 0.038f }, 0.0f, 18.0f, 0.0f, cDark);
        // Iconic M60 Triangular Front Sight Tower (fully grounded to barrel, zero air gap)
        if (att.optic == OPTIC_IRON) {
            // 1. Heavy cylindrical collar clamped around barrel (y = 0.022f, r = 0.015f)
            DrawPFCylinder({ 0.0f, 0.022f, -0.725f }, { 0.0f, 0.022f, -0.755f }, 0.017f, 0.017f, 10, cDark);
            DrawPFReceiverPin({ 0.0f, 0.022f, -0.740f }, 0.036f, 0.0035f, cSteel);
            // 2. Solid triangular upright body rising from sleeve to sight deck
            DrawPFBox({ 0.0f, 0.052f, -0.740f }, { 0.014f, 0.034f, 0.022f }, cDark);
            DrawPFAngledBox({ 0.0f, 0.053f, -0.750f }, { 0.013f, 0.036f, 0.010f }, -18.0f, 0.0f, 0.0f, cDark);
            // 3. Platform deck & knurled elevation screw well
            DrawPFBox({ 0.0f, 0.069f, -0.740f }, { 0.018f, 0.004f, 0.020f }, cDark);
            DrawPFKnurledCylinder({ 0.0f, 0.069f, -0.740f }, { 0.0f, 0.072f, -0.740f }, 0.0045f, 6, 2, cDark, cSteel);
            // 4. Protective curved ears flaring up to y = 0.084f
            DrawPFAngledBox({ -0.0075f, 0.076f, -0.740f }, { 0.0024f, 0.016f, 0.016f }, 0.0f, 0.0f, -14.0f, cDark);
            DrawPFAngledBox({  0.0075f, 0.076f, -0.740f }, { 0.0024f, 0.016f, 0.016f }, 0.0f, 0.0f,  14.0f, cDark);
            // 5. Adjustable square sight blade & luminous tritium bead (exact ironSightY = 0.078f)
            DrawPFBox({ 0.0f, 0.074f, -0.740f }, { 0.0020f, 0.008f, 0.004f }, cDark);
            DrawCubeV({ 0.0f, 0.078f, -0.740f }, { 0.0016f, 0.0016f, 0.0016f }, Color{ 70, 255, 110, 255 });
        }

        // Side Canvas Ammo Box + Steel Hanger Bracket + Detailed 7.62mm Brass Belt with M13 Links & Copper Tips!
        rlPushMatrix();
        rlTranslatef(sock.magWellPos.x + magOffset.x, sock.magWellPos.y + magOffset.y, sock.magWellPos.z + magOffset.z);
        DrawPFBox({ 0.0f, 0.0f, 0.0f }, { 0.085f, 0.135f, 0.125f }, (att.ammo == AMMO_STANDARD) ? cAccent : magTint);
        DrawPFBox({ 0.0f, 0.056f, 0.0f }, { 0.090f, 0.018f, 0.130f }, cDark); // Canvas pouch lid
        for (int b = 0; b < 7; ++b) {
            float fy = 0.048f + b * 0.0105f;
            float fx = 0.022f + b * 0.0055f;
            float fz = -0.024f + b * 0.007f;
            DrawPFBox({ fx, fy, fz }, { 0.032f, 0.007f, 0.008f }, cBrass);              // Brass casing
            DrawPFBox({ fx + 0.004f, fy, fz }, { 0.014f, 0.0085f, 0.009f }, cDark);     // Black M13 disintegrating link
            DrawPFBox({ fx + 0.020f, fy, fz }, { 0.012f, 0.0055f, 0.006f }, cCopper);   // Copper FMJ bullet tip
        }
        rlPopMatrix();

    } else if (stats.id == WPN_G17) {
        // Polymer Frame + Finger Grooves + Thumb Rest + Picatinny Dust Cover + Safe-Action Trigger
        DrawPFAngledBox({ 0.0f, -0.052f, 0.042f }, { 0.038f, 0.110f, 0.050f }, -14.0f, 0.0f, 0.0f, cAccent);
        DrawPFAngledBox({ 0.0f, -0.054f, 0.046f }, { 0.041f, 0.082f, 0.032f }, -14.0f, 0.0f, 0.0f, cDark); // Textured grip panels
        for (int g = 0; g < 3; ++g) {
            DrawPFBox({ 0.0f, -0.032f - g * 0.024f, 0.014f + g * 0.006f }, { 0.039f, 0.008f, 0.010f }, cDark); // Finger grooves
        }
        DrawPFBox({ 0.0f, 0.004f, -0.05f }, { 0.038f, 0.022f, 0.19f }, cAccent);
        DrawPFPicatinnyRail({ 0.0f, -0.006f, -0.095f }, 0.034f, 0.006f, 0.075f, 3, cDark, cPolymer);
        DrawPFTriggerAssembly({ 0.0f, -0.022f, -0.014f }, 0.016f, 0.048f, cAccent, cDark);
        DrawPFReceiverPin({ 0.0f, 0.006f, -0.022f }, 0.040f, 0.003f, cDark); // Trigger & locking block pins
        DrawPFReceiverPin({ 0.0f, 0.000f, -0.012f }, 0.040f, 0.003f, cDark);

        // Reciprocating Serrated Blocky Glock Slide + Ejection Port Hood + Extractor Claw
        float slideZ = -0.05f + boltProgress * 0.055f;
        DrawPFCylinder({ 0.0f, 0.024f, -0.02f }, sock.muzzleTipPos, 0.009f, 0.009f, 10, cSteel);
        DrawPFCylinder({ 0.0f, 0.010f, -0.04f }, { 0.0f, 0.010f, -0.145f }, 0.006f, 0.006f, 8, cDark); // Recoil spring guide rod
        DrawPFBox({ 0.0f, 0.025f, slideZ }, { 0.040f, 0.034f, 0.21f }, cBody);
        // Front slide serrations (aggressive forward-cocking cuts)
        for (int fs = 0; fs < 5; ++fs) {
            DrawPFBox({ 0.0f, 0.024f, slideZ - 0.068f - fs * 0.008f }, { 0.043f, 0.025f, 0.0038f }, cDark);
        }
        DrawPFBox({ 0.010f, 0.032f, -0.03f }, { 0.024f, 0.014f, 0.046f }, cDark); // Rectangular barrel hood in port
        // Loaded chamber indicator — a small raised pin on top of slide above barrel hood
        DrawPFBox({ 0.0f, 0.043f, -0.042f + boltProgress * 0.055f }, { 0.006f, 0.006f, 0.008f },
                  boltProgress < 0.05f ? Color{ 48, 155, 85, 255 } : Color{ 215, 42, 42, 255 }); // green=empty, red=loaded
        // Rear slide serrations (standard Glock pattern)
        for (int s = 0; s < 6; ++s) {
            DrawPFBox({ 0.0f, 0.024f, slideZ + 0.048f + s * 0.008f }, { 0.043f, 0.026f, 0.0035f }, cDark);
        }
        // Slide-Mounted 3-Dot Combat Iron Sights (ironSightY = 0.052f, reciprocates with slide)
        // 1. Dovetailed Rear U-Notch Sight Block at slideZ + 0.095f
        DrawPFBox({ 0.0f, 0.046f, slideZ + 0.095f }, { 0.024f, 0.009f, 0.014f }, cDark);
        DrawPFBox({ -0.0065f, 0.051f, slideZ + 0.095f }, { 0.0045f, 0.006f, 0.012f }, cDark); // Left notch blade
        DrawPFBox({  0.0065f, 0.051f, slideZ + 0.095f }, { 0.0045f, 0.006f, 0.012f }, cDark); // Right notch blade
        DrawPFBox({  0.0f,    0.0482f, slideZ + 0.095f }, { 0.016f, 0.0022f, 0.012f }, cDark); // Notch floor
        DrawCubeV({ -0.0065f, 0.050f, slideZ + 0.102f }, { 0.0022f, 0.0022f, 0.0016f }, Color{ 255, 255, 255, 240 }); // Rear white dot
        DrawCubeV({  0.0065f, 0.050f, slideZ + 0.102f }, { 0.0022f, 0.0022f, 0.0016f }, Color{ 255, 255, 255, 240 });

        // 2. Slide-Pinned Front Sight Dot Post at slideZ - 0.092f (zero gap, firmly anchored on slide)
        DrawPFBox({ 0.0f, 0.047f, slideZ - 0.092f }, { 0.0035f, 0.010f, 0.014f }, cDark);
        DrawCubeV({ 0.0f, 0.050f, slideZ - 0.088f }, { 0.0022f, 0.0022f, 0.0018f }, Color{ 255, 255, 255, 250 }); // Front white dot
        DrawCubeV({ 0.0f, 0.052f, slideZ - 0.092f }, { 0.0016f, 0.0016f, 0.0016f }, Color{ 70, 255, 110, 255 });  // Tritium bead tip
        // Trigger shoe stippling detail (Glock OEM serrated trigger face)
        DrawPFAngledBox({ 0.0f, -0.026f, -0.022f }, { 0.014f, 0.003f, 0.006f }, -18.0f, 0.0f, 0.0f, cPolymer);

        rlPushMatrix();
        rlTranslatef(sock.magWellPos.x + magOffset.x, sock.magWellPos.y + magOffset.y, sock.magWellPos.z + magOffset.z);
        DrawPFAngledBox({ 0.0f, 0.0f, 0.0f }, { 0.030f, 0.100f, 0.040f }, -14.0f, 0.0f, 0.0f, magTint);
        DrawPFAngledBox({ 0.0f, -0.052f, 0.013f }, { 0.038f, 0.012f, 0.048f }, -14.0f, 0.0f, 0.0f, cDark); // Polymer mag baseplate
        rlPopMatrix();

    } else if (stats.id == WPN_DEAGLE) {
        // Heavy Two-Tone Desert Eagle Mark XIX Frame + Wraparound Rubber Grips + Beavertail Tang
        DrawPFAngledBox({ 0.0f, -0.058f, 0.048f }, { 0.044f, 0.120f, 0.058f }, -14.0f, 0.0f, 0.0f, cAccent);
        DrawPFAngledBox({ 0.0f, -0.060f, 0.052f }, { 0.047f, 0.095f, 0.042f }, -14.0f, 0.0f, 0.0f, cDark); // Rubberized grip panels
        DrawPFBox({ 0.0f, 0.004f, -0.06f }, { 0.042f, 0.026f, 0.24f }, cAccent);
        DrawPFAngledBox({ 0.0f, 0.008f, 0.072f }, { 0.040f, 0.016f, 0.034f }, 16.0f, 0.0f, 0.0f, cAccent); // Beavertail tang
        DrawPFTriggerAssembly({ 0.0f, -0.024f, -0.018f }, 0.018f, 0.056f, cAccent, cDark);
        // External Skeletonized Spur Hammer (cocks back with slide!)
        DrawPFAngledBox({ 0.0f, 0.022f, 0.078f + boltProgress * 0.025f },
                        { 0.010f, 0.024f, 0.018f }, -32.0f - boltProgress * 35.0f, 0.0f, 0.0f, cDark);

        // Fixed Angled Triangular Barrel Block + Integrated Top Weaver Rail Rib + Side Lightning Cuts
        DrawPFBox({ 0.0f, 0.028f, -0.13f }, { 0.044f, 0.040f, 0.16f }, cBody);
        DrawPFAngledBox({ -0.016f, 0.038f, -0.13f }, { 0.014f, 0.022f, 0.158f }, 0.0f, 0.0f, -32.0f, cBody);
        DrawPFAngledBox({  0.016f, 0.038f, -0.13f }, { 0.014f, 0.022f, 0.158f }, 0.0f, 0.0f,  32.0f, cBody);
        DrawPFPicatinnyRail({ 0.0f, 0.048f, -0.13f }, 0.026f, 0.006f, 0.14f, 6, cDark, cBody);
        for (int c = 0; c < 3; ++c) {
            DrawPFBox({ 0.0f, 0.026f, -0.175f + c * 0.032f }, { 0.048f, 0.012f, 0.018f }, cDark); // Barrel flutes/cuts
        }
        // Reciprocating Open-Top Rear Slide + Ambidextrous Safety Levers + Deep Slide Serrations
        float deagleSlideZ = 0.01f + boltProgress * 0.065f;
        DrawPFBox({ 0.0f, 0.028f, deagleSlideZ }, { 0.046f, 0.040f, 0.13f }, cBody);
        for (int s = 0; s < 5; ++s) {
            DrawPFBox({ 0.0f, 0.026f, deagleSlideZ + 0.018f + s * 0.009f }, { 0.049f, 0.028f, 0.004f }, cDark);
        }
        DrawPFAngledBox({ -0.025f, 0.032f, deagleSlideZ + 0.048f }, { 0.006f, 0.012f, 0.022f }, -15.0f, 0.0f, 0.0f, cDark);
        DrawPFAngledBox({  0.025f, 0.032f, deagleSlideZ + 0.048f }, { 0.006f, 0.012f, 0.022f }, -15.0f, 0.0f, 0.0f, cDark);

        // Heavy Combat 3-Dot Iron Sights (ironSightY = 0.062f)
        // 1. Slide-Mounted Dovetailed Rear Square Notch Sight at deagleSlideZ + 0.055f
        DrawPFBox({ 0.0f, 0.054f, deagleSlideZ + 0.055f }, { 0.026f, 0.012f, 0.016f }, cDark);
        DrawPFBox({ -0.0075f, 0.060f, deagleSlideZ + 0.055f }, { 0.0050f, 0.008f, 0.014f }, cDark); // Left blade
        DrawPFBox({  0.0075f, 0.060f, deagleSlideZ + 0.055f }, { 0.0050f, 0.008f, 0.014f }, cDark); // Right blade
        DrawPFBox({  0.0f,    0.0565f, deagleSlideZ + 0.055f }, { 0.018f, 0.003f, 0.014f }, cDark); // Notch shelf
        DrawCubeV({ -0.0075f, 0.060f, deagleSlideZ + 0.063f }, { 0.0022f, 0.0022f, 0.0016f }, Color{ 255, 255, 255, 240 }); // Rear white dot
        DrawCubeV({  0.0075f, 0.060f, deagleSlideZ + 0.063f }, { 0.0022f, 0.0022f, 0.0016f }, Color{ 255, 255, 255, 240 });

        // 2. Fixed Barrel Front Combat Ramp Sight Blade at sock.muzzleTipPos.z + 0.024f (zero air gap)
        DrawPFBox({ 0.0f, 0.054f, sock.muzzleTipPos.z + 0.024f }, { 0.008f, 0.012f, 0.022f }, cDark); // Riser base
        DrawPFAngledBox({ 0.0f, 0.056f, sock.muzzleTipPos.z + 0.032f }, { 0.007f, 0.014f, 0.012f }, 28.0f, 0.0f, 0.0f, cDark); // Ramp face
        DrawPFBox({ 0.0f, 0.060f, sock.muzzleTipPos.z + 0.022f }, { 0.0024f, 0.006f, 0.014f }, cDark); // Blade
        DrawCubeV({ 0.0f, 0.060f, sock.muzzleTipPos.z + 0.028f }, { 0.0022f, 0.0022f, 0.0016f }, Color{ 255, 255, 255, 250 }); // Front white dot
        DrawCubeV({ 0.0f, 0.062f, sock.muzzleTipPos.z + 0.022f }, { 0.0016f, 0.0016f, 0.0016f }, Color{ 70, 255, 110, 255 }); // Tritium pip

        rlPushMatrix();
        rlTranslatef(sock.magWellPos.x + magOffset.x, sock.magWellPos.y + magOffset.y, sock.magWellPos.z + magOffset.z);
        DrawPFAngledBox({ 0.0f, 0.0f, 0.0f }, { 0.034f, 0.110f, 0.046f }, -14.0f, 0.0f, 0.0f, (att.ammo == AMMO_STANDARD) ? cChrome : magTint);
        DrawPFAngledBox({ 0.0f, -0.056f, 0.014f }, { 0.042f, 0.012f, 0.054f }, -14.0f, 0.0f, 0.0f, cDark);
        rlPopMatrix();
    }

    // ------------------------------------------------------------------------
    // 2. BARREL ATTACHMENTS (Multi-Part Muzzle Devices & Suppressors)
    // ------------------------------------------------------------------------
    Vector3 flashOrigin = sock.muzzleTipPos;
    if (att.barrel == BARREL_SUPPRESSOR) {
        // Quick-Detach Tactical Suppressor with Wrench Flats, Cooling Ribs & End-Cap Bore
        Vector3 collar = Vector3Add(sock.muzzleTipPos, { 0, 0, -0.028f });
        Vector3 mid    = Vector3Add(sock.muzzleTipPos, { 0, 0, -0.055f });
        Vector3 end    = Vector3Add(sock.muzzleTipPos, { 0, 0, -0.175f });
        DrawPFCylinder(sock.muzzleTipPos, collar, 0.016f, 0.019f, 8, cSteel); // QD wrench-flat collar
        DrawPFCylinder(collar, mid, 0.022f, 0.022f, 12, { 38, 42, 48, 255 });
        DrawPFCylinder(mid, end, 0.021f, 0.021f, 12, cDark);
        for (int r = 0; r < 4; ++r) {
            Vector3 rz = Vector3Add(mid, { 0, 0, -0.018f - r * 0.024f });
            DrawPFCylinder(rz, Vector3Add(rz, { 0, 0, -0.008f }), 0.0225f, 0.0225f, 12, { 46, 50, 56, 255 });
        }
        DrawPFCylinder(end, Vector3Add(end, { 0, 0, -0.004f }), 0.008f, 0.008f, 8, { 10, 10, 12, 255 }); // Bore hole
        flashOrigin = end;
    } else if (att.barrel == BARREL_ARS_SUPPRESSOR) {
        // Short K-Can ARS Suppressor with Knurled Locking Ring & Fluted Body
        Vector3 ring = Vector3Add(sock.muzzleTipPos, { 0, 0, -0.022f });
        Vector3 end  = Vector3Add(sock.muzzleTipPos, { 0, 0, -0.115f });
        DrawPFCylinder(sock.muzzleTipPos, ring, 0.019f, 0.019f, 8, cSteel);
        DrawPFCylinder(ring, end, 0.0175f, 0.0175f, 12, { 42, 46, 52, 255 });
        DrawPFCylinder(Vector3Add(end, { 0, 0, 0.016f }), end, 0.0185f, 0.016f, 10, cDark);
        flashOrigin = end;
    } else if (att.barrel == BARREL_MUZZLE_BRAKE) {
        // Dual-Chamber Competition Muzzle Brake with Angled Side Baffle Ports & Crush Washer
        Vector3 center = Vector3Add(sock.muzzleTipPos, { 0, 0, -0.030f });
        DrawPFCylinder(sock.muzzleTipPos, Vector3Add(sock.muzzleTipPos, { 0, 0, -0.008f }), 0.014f, 0.014f, 8, cChrome);
        DrawPFBox(center, { 0.034f, 0.026f, 0.056f }, { 68, 72, 78, 255 });
        DrawPFAngledBox(Vector3Add(center, { 0, 0,  0.010f }), { 0.038f, 0.014f, 0.014f }, 0.0f, 18.0f, 0.0f, cDark);
        DrawPFAngledBox(Vector3Add(center, { 0, 0, -0.012f }), { 0.038f, 0.014f, 0.014f }, 0.0f, 18.0f, 0.0f, cDark);
        flashOrigin = Vector3Add(sock.muzzleTipPos, { 0, 0, -0.058f });
    } else if (att.barrel == BARREL_COMPENSATOR) {
        // Ported Tactical Compensator with 3 Upward Gas Expansion Vents
        Vector3 end = Vector3Add(sock.muzzleTipPos, { 0, 0, -0.056f });
        DrawPFCylinder(sock.muzzleTipPos, end, 0.0155f, 0.0155f, 10, { 92, 84, 66, 255 });
        for (int p = 0; p < 3; ++p) {
            DrawPFBox(Vector3Add(sock.muzzleTipPos, { 0, 0.012f, -0.014f - p * 0.014f }), { 0.014f, 0.010f, 0.008f }, cDark);
        }
        flashOrigin = end;
    }

    // ------------------------------------------------------------------------
    // 3. UNDERBARREL GRIPS (Flush Rail Clamps, Thumbscrews & Textured Profiles)
    // ------------------------------------------------------------------------
    Vector3 leftHandGripPos = Vector3Add(sock.underbarrelMountPos, { -0.018f, -0.012f, 0.0f });
    if (stats.id == WPN_REMINGTON870) {
        leftHandGripPos.z += boltProgress * 0.11f;
    }

    if (stats.isPrimary && att.grip != GRIP_NONE) {
        Vector3 gp = sock.underbarrelMountPos;
        // Flush low-profile Picatinny rail clamp + dual steel cross-bolt thumbscrews
        DrawPFBox({ gp.x, gp.y - 0.002f, gp.z }, { 0.036f, 0.012f, 0.054f }, cDark);
        DrawPFReceiverPin({ gp.x, gp.y - 0.002f, gp.z - 0.014f }, 0.042f, 0.0040f, cSteel);
        DrawPFReceiverPin({ gp.x, gp.y - 0.002f, gp.z + 0.014f }, 0.042f, 0.0040f, cSteel);

        if (att.grip == GRIP_STUBBY) {
            // TangoDown Stubby Grip with Horizontal Grip Ribs & Waterproof Battery Cap
            DrawPFAngledBox({ gp.x, gp.y - 0.034f, gp.z }, { 0.032f, 0.056f, 0.036f }, -6.0f, 0.0f, 0.0f, cDark);
            for (int r = 0; r < 3; ++r) {
                DrawPFAngledBox({ gp.x, gp.y - 0.022f - r * 0.013f, gp.z }, { 0.035f, 0.005f, 0.039f }, -6.0f, 0.0f, 0.0f, cPolymer);
            }
            DrawPFAngledBox({ gp.x, gp.y - 0.064f, gp.z + 0.003f }, { 0.034f, 0.010f, 0.038f }, -6.0f, 0.0f, 0.0f, cSteel);
            leftHandGripPos = { gp.x - 0.012f, gp.y - 0.038f, gp.z };
        } else if (att.grip == GRIP_ANGLED) {
            // Magpul AFG-2 Angled Foregrip with Finger Shelf, Serrated Ramp & Cross Bolts
            DrawPFBox({ gp.x, gp.y - 0.006f, gp.z }, { 0.032f, 0.012f, 0.110f }, cDark);
            DrawPFAngledBox({ gp.x, gp.y - 0.022f, gp.z - 0.018f }, { 0.030f, 0.024f, 0.078f }, 28.0f, 0.0f, 0.0f, cDark);
            DrawPFAngledBox({ gp.x, gp.y - 0.018f, gp.z - 0.052f }, { 0.032f, 0.018f, 0.012f }, -18.0f, 0.0f, 0.0f, cPolymer); // Front finger stop
            DrawPFAngledBox({ gp.x, gp.y - 0.024f, gp.z + 0.032f }, { 0.030f, 0.028f, 0.042f }, -34.0f, 0.0f, 0.0f, cDark);
            for (int r = 0; r < 4; ++r) {
                DrawPFAngledBox({ gp.x, gp.y - 0.016f - r * 0.006f, gp.z - 0.034f + r * 0.012f },
                                { 0.033f, 0.005f, 0.006f }, 28.0f, 0.0f, 0.0f, cPolymer);
            }
            leftHandGripPos = { gp.x - 0.014f, gp.y - 0.022f, gp.z };
        } else if (att.grip == GRIP_VERTICAL) {
            // KAC Broomhandle Vertical Foregrip with Ribbed Collar & Knurled Bottom Cap
            DrawPFCylinder({ gp.x, gp.y, gp.z }, { gp.x, gp.y - 0.022f, gp.z }, 0.018f, 0.016f, 10, cPolymer);
            DrawPFCylinder({ gp.x, gp.y - 0.022f, gp.z }, { gp.x, gp.y - 0.082f, gp.z }, 0.015f, 0.014f, 10, cDark);
            for (int r = 0; r < 4; ++r) {
                float ry = gp.y - 0.030f - r * 0.013f;
                DrawPFCylinder({ gp.x, ry, gp.z }, { gp.x, ry - 0.005f, gp.z }, 0.0165f, 0.0165f, 10, cPolymer);
            }
            DrawPFCylinder({ gp.x, gp.y - 0.082f, gp.z }, { gp.x, gp.y - 0.094f, gp.z }, 0.016f, 0.013f, 10, cSteel);
            leftHandGripPos = { gp.x - 0.010f, gp.y - 0.052f, gp.z };
        } else if (att.grip == GRIP_FOLDING) {
            // Folding Foregrip with Steel Hinge Knuckle, Push-Button Lock & Contoured Finger Grooves
            DrawPFBox({ gp.x, gp.y - 0.008f, gp.z + 0.015f }, { 0.034f, 0.018f, 0.036f }, cSteel);
            DrawPFReceiverPin({ gp.x, gp.y - 0.010f, gp.z + 0.006f }, 0.040f, 0.0055f, cDark); // Pivot button
            DrawPFAngledBox({ gp.x, gp.y - 0.048f, gp.z }, { 0.026f, 0.082f, 0.032f }, 10.0f, 0.0f, 0.0f, cDark);
            for (int g = 0; g < 3; ++g) {
                DrawPFAngledBox({ gp.x, gp.y - 0.032f - g * 0.018f, gp.z - 0.004f }, { 0.028f, 0.007f, 0.035f }, 10.0f, 0.0f, 0.0f, cPolymer);
            }
            leftHandGripPos = { gp.x - 0.010f, gp.y - 0.048f, gp.z };
        } else if (att.grip == GRIP_SKELETON) {
            // Lightweight CNC-Machined Aluminum Skeleton Grip with Cross-Webbing Struts
            DrawPFAngledBox({ gp.x, gp.y - 0.042f, gp.z - 0.016f }, { 0.020f, 0.078f, 0.009f }, -16.0f, 0.0f, 0.0f, cSteel);
            DrawPFAngledBox({ gp.x, gp.y - 0.042f, gp.z + 0.016f }, { 0.020f, 0.078f, 0.009f }, -16.0f, 0.0f, 0.0f, cSteel);
            DrawPFAngledBox({ gp.x, gp.y - 0.042f, gp.z }, { 0.018f, 0.008f, 0.034f },  22.0f, 0.0f, 0.0f, cDark); // Diagonal truss web
            DrawPFAngledBox({ gp.x, gp.y - 0.076f, gp.z }, { 0.022f, 0.012f, 0.044f }, -16.0f, 0.0f, 0.0f, cSteel);
            leftHandGripPos = { gp.x - 0.010f, gp.y - 0.045f, gp.z };
        } else if (att.grip == GRIP_HERA_CQR) {
            // Futuristic Hera Arms CQR Thumbhole Foregrip with Side Fastener Bolts & Sling QD Socket
            DrawPFBox({ gp.x, gp.y - 0.012f, gp.z }, { 0.034f, 0.022f, 0.138f }, cDark);
            DrawPFAngledBox({ gp.x, gp.y - 0.044f, gp.z - 0.040f }, { 0.032f, 0.064f, 0.032f }, -26.0f, 0.0f, 0.0f, cDark);
            DrawPFAngledBox({ gp.x, gp.y - 0.044f, gp.z + 0.044f }, { 0.030f, 0.064f, 0.026f },  22.0f, 0.0f, 0.0f, cDark);
            DrawPFBox({ gp.x, gp.y - 0.068f, gp.z }, { 0.032f, 0.016f, 0.108f }, cDark);
            DrawPFBox({ gp.x, gp.y - 0.042f, gp.z - 0.044f }, { 0.035f, 0.040f, 0.020f }, cPolymer); // Textured grip insert
            DrawPFReceiverPin({ gp.x, gp.y - 0.014f, gp.z - 0.048f }, 0.038f, 0.004f, cSteel);
            DrawPFReceiverPin({ gp.x, gp.y - 0.014f, gp.z + 0.048f }, 0.038f, 0.004f, cSteel);
            leftHandGripPos = { gp.x - 0.012f, gp.y - 0.044f, gp.z - 0.025f };
        }
    }

    // ------------------------------------------------------------------------
    // 4. SIDE RAIL / OTHER ATTACHMENTS (AN/PEQ Laser Box, Canted Delta, Tracker)
    // ------------------------------------------------------------------------
    Vector3 sp = sock.sideRailPos;
    if (att.other == OTHER_RED_LASER || att.other == OTHER_GREEN_LASER || att.other == OTHER_BLUE_LASER) {
        // AN/PEQ-15 Tactical Laser Aiming Module with Dual Emitters, Rotary Mode Dial & Activation Button
        DrawPFBox(sp, { 0.024f, 0.022f, 0.062f }, cDark);
        DrawPFBox({ sp.x + 0.004f, sp.y + 0.008f, sp.z }, { 0.020f, 0.012f, 0.054f }, cPolymer);
        // Rotary mode selector dial + blue safety screw + pressure switch plug
        DrawPFCylinder({ sp.x + 0.012f, sp.y + 0.014f, sp.z + 0.006f },
                       { sp.x + 0.018f, sp.y + 0.018f, sp.z + 0.006f }, 0.006f, 0.006f, 8, cSteel);
        DrawPFBox({ sp.x + 0.014f, sp.y + 0.004f, sp.z - 0.012f }, { 0.006f, 0.008f, 0.010f }, { 185, 52, 42, 255 }); // Fire button
        // Dual front laser & IR illuminator lens bezels
        DrawPFCylinder(Vector3Add(sp, { 0.0f,  0.004f, -0.028f }), Vector3Add(sp, { 0.0f,  0.004f, -0.040f }), 0.0055f, 0.0055f, 8, cSteel);
        DrawPFCylinder(Vector3Add(sp, { 0.0f, -0.005f, -0.028f }), Vector3Add(sp, { 0.0f, -0.005f, -0.036f }), 0.0045f, 0.0045f, 8, cDark);
        Color beamCol = (att.other == OTHER_RED_LASER)   ? Color{ 255, 38, 38, 195 }
                      : (att.other == OTHER_GREEN_LASER) ? Color{ 38, 255, 78, 195 }
                      :                                    Color{ 45, 195, 255, 210 };
        Vector3 beamStart = Vector3Add(sp, { 0.0f, 0.004f, -0.040f });
        DrawCubeV(beamStart, { 0.004f, 0.004f, 0.004f }, beamCol);
        DrawLine3D(beamStart, Vector3Add(beamStart, { 0.0f, 0.0f, -20.0f }), beamCol);

    } else if (stats.isPrimary && att.other == OTHER_CANTED_DELTA) {
        // 45-Degree Offset Rail Mount + Ultra-Lean Mini-Delta Reflex Sight
        float cz = sock.opticMountPos.z - 0.065f;
        float cy = sock.opticMountPos.y - 0.006f;
        DrawPFBox({ 0.012f, cy - 0.002f, cz }, { 0.024f, 0.006f, 0.038f }, cDark); // Low-profile rail clamp
        DrawPFAngledBox({ 0.026f, cy - 0.003f, cz }, { 0.032f, 0.008f, 0.036f }, 0.0f, 0.0f, -42.0f, cDark);
        DrawPFAngledBox({ 0.038f, cy + 0.008f, cz }, { 0.022f, 0.008f, 0.038f }, 0.0f, 0.0f, -42.0f, cPolymer);
        // Ultra-lean protective delta hood wings (0.0016f thin titanium rim, wide open window)
        DrawPFAngledBox({ 0.033f, cy + 0.022f, cz }, { 0.0016f, 0.020f, 0.026f }, -14.0f, 0.0f, -22.0f, cDark);
        DrawPFAngledBox({ 0.052f, cy + 0.004f, cz }, { 0.0016f, 0.020f, 0.026f }, -14.0f, 0.0f, -62.0f, cDark);
        DrawPFAngledBox({ 0.044f, cy + 0.015f, cz }, { 0.020f, 0.018f, 0.001f }, 0.0f, 0.0f, -42.0f, { 80, 210, 255, 24 });
        // Crisp 1.5 MOA micro red dot (pinpoint precision, never covers targets)
        DrawCubeV({ 0.044f, cy + 0.015f, cz }, { 0.0013f, 0.0013f, 0.001f }, { 255, 45, 45, 255 });

    } else if (att.other == OTHER_BALLISTICS_TRACKER) {
        // Ruggedized Tactical Ballistics Tracker Smartphone with Protective Bumper, Antenna & Telemetry UI
        float bz = sock.opticMountPos.z - 0.055f;
        float by = sock.opticMountPos.y - 0.008f;
        DrawPFAngledBox({ 0.028f, by - 0.008f, bz }, { 0.034f, 0.009f, 0.024f }, 0.0f, 0.0f, -20.0f, cDark);
        DrawPFAngledBox({ 0.048f, by, bz }, { 0.048f, 0.032f, 0.010f }, 0.0f, 0.0f, -15.0f, cDark); // Rugged bumper case
        DrawPFCylinder({ 0.066f, by + 0.010f, bz - 0.002f }, { 0.070f, by + 0.026f, bz - 0.002f }, 0.003f, 0.0025f, 6, cDark); // Rangefinder antenna
        DrawPFCylinder({ 0.054f, by + 0.006f, bz - 0.004f }, { 0.054f, by + 0.006f, bz - 0.012f }, 0.006f, 0.006f, 8, cSteel); // Forward camera lens
        // Bright yellow telemetry screen + radar grid & target indicator
        DrawPFAngledBox({ 0.048f, by, bz + 0.0052f }, { 0.040f, 0.024f, 0.001f }, 0.0f, 0.0f, -15.0f, { 245, 205, 35, 255 });
        DrawPFAngledBox({ 0.048f, by, bz + 0.0058f }, { 0.028f, 0.002f, 0.001f }, 0.0f, 0.0f, -15.0f, { 40, 40, 40, 220 });
        DrawPFAngledBox({ 0.048f, by, bz + 0.0058f }, { 0.002f, 0.018f, 0.001f }, 0.0f, 0.0f, -15.0f, { 40, 40, 40, 220 });
        DrawPFAngledBox({ 0.048f, by, bz + 0.0062f }, { 0.005f, 0.005f, 0.001f }, 0.0f, 0.0f, -15.0f, { 255, 45, 35, 255 });
    }

    // ------------------------------------------------------------------------
    // 5. ULTRA-LEAN OPTIC HOUSINGS & CRISP PRECISION RETICLES
    // ------------------------------------------------------------------------
    OpticType effOptic = att.optic;
    if (stats.id == WPN_INTERVENTION && effOptic == OPTIC_IRON) effOptic = OPTIC_PMII_SCOPE;
    float baseY  = sock.opticMountPos.y;
    float sightY = GetOpticSightHeight(stats.id, effOptic);
    float opticZ = sock.opticMountPos.z;

    if (effOptic == OPTIC_IRON) {
        // All primary & secondary weapons now have bespoke, grounded, authentic iron sight assemblies
        // modeled directly on their barrels/slides. Section 5 acts as a robust fail-safe.
        bool hasBespokeIron = (stats.id == WPN_M4A1 || stats.id == WPN_AK47 ||
                               stats.id == WPN_MP7  || stats.id == WPN_REMINGTON870 ||
                               stats.id == WPN_M60  || stats.id == WPN_G17 ||
                               stats.id == WPN_DEAGLE);
        if (!hasBespokeIron) {
            float rearZ   = stats.isPrimary ? 0.07f : 0.03f;
            float frontZ  = sock.muzzleTipPos.z + 0.06f;
            float barrelY = sock.muzzleTipPos.y;
            // Lean Low-Profile Rear Sight Base + Slim Notch Wings (wide, clear target view)
            DrawPFBox({ 0.0f, baseY + 0.002f, rearZ }, { 0.024f, 0.006f, 0.020f }, cDark);
            DrawPFReceiverPin({ 0.0f, baseY + 0.004f, rearZ }, 0.030f, 0.0035f, cSteel);
            DrawPFAngledBox({ -0.012f, sightY - 0.003f, rearZ }, { 0.0018f, 0.014f, 0.010f }, -14.0f, 0.0f,  18.0f, cDark);
            DrawPFAngledBox({  0.012f, sightY - 0.003f, rearZ }, { 0.0018f, 0.014f, 0.010f }, -14.0f, 0.0f, -18.0f, cDark);
            // Solid barrel collar / riser from barrel up to sight post (NO FLOATING AIR GAP!)
            float postStemH = sightY - barrelY;
            if (postStemH > 0.010f) {
                DrawPFBox({ 0.0f, barrelY + postStemH * 0.45f, frontZ }, { 0.014f, postStemH * 0.90f, 0.018f }, cDark);
            }
            // Slender Front Sight Needle Post + Fine Luminous Tritium Bead at (0, sightY, frontZ)
            DrawPFAngledBox({ 0.0f, sightY - 0.008f, frontZ - 0.003f }, { 0.0035f, 0.015f, 0.010f }, -24.0f, 0.0f, 0.0f, cDark);
            DrawPFBox({ 0.0f, sightY - 0.005f, frontZ }, { 0.0018f, 0.013f, 0.004f }, cDark);
            DrawCubeV({ 0.0f, sightY, frontZ }, { 0.0016f, 0.0016f, 0.0016f }, { 70, 255, 110, 255 });
        }

    } else if (effOptic == OPTIC_ZPOINT) {
        // Ultra-Lean Zeiss Z-Point: Thin Titanium Walls & Wide Viewing Aperture
        DrawPFBox({ 0.0f, baseY + 0.003f, opticZ }, { 0.028f, 0.007f, 0.052f }, cDark); // Low-profile base
        DrawPFReceiverPin({ 0.0f, baseY + 0.003f, opticZ }, 0.034f, 0.0035f, cSteel);
        DrawPFCylinder({ 0.017f, sightY - 0.004f, opticZ }, { 0.022f, sightY - 0.004f, opticZ }, 0.006f, 0.006f, 8, cPolymer);
        // Slender walls (0.0016f thin!) pushed out to x = +/-0.019f for maximum clear sight picture
        DrawPFAngledBox({ -0.019f, sightY - 0.002f, opticZ }, { 0.0016f, 0.018f, 0.050f }, 0.0f, 0.0f, -10.0f, cDark);
        DrawPFAngledBox({  0.019f, sightY - 0.002f, opticZ }, { 0.0016f, 0.018f, 0.050f }, 0.0f, 0.0f,  10.0f, cDark);
        DrawPFAngledBox({ -0.014f, sightY + 0.012f, opticZ }, { 0.0015f, 0.014f, 0.048f }, 0.0f, 0.0f, -38.0f, cDark);
        DrawPFAngledBox({  0.014f, sightY + 0.012f, opticZ }, { 0.0015f, 0.014f, 0.048f }, 0.0f, 0.0f,  38.0f, cDark);
        // Slim top roof strip + wide anti-reflective multi-coated glass
        DrawPFBox({ 0.0f, sightY + 0.017f, opticZ }, { 0.018f, 0.0018f, 0.048f }, { 170, 48, 32, 255 });
        DrawCubeV({ 0.0f, sightY, opticZ }, { 0.034f, 0.026f, 0.001f }, { 75, 205, 255, 22 });
        // Pinpoint 1.5 MOA Red Dot Reticle (razor-sharp micro point, zero clutter)
        DrawCubeV({ 0.0f, sightY, opticZ }, { 0.0012f, 0.0012f, 0.001f }, { 255, 45, 45, 255 });

    } else if (effOptic == OPTIC_COYOTE) {
        // Ultra-Lean Coyote Reflex: Iconic Razor-Thin Titanium Hood & Edge-to-Edge Panoramic Window
        DrawPFBox({ 0.0f, baseY + 0.003f, opticZ }, { 0.030f, 0.006f, 0.058f }, cDark); // Low-profile flush base
        DrawPFReceiverPin({ 0.0f, baseY + 0.003f, opticZ + 0.008f }, 0.036f, 0.0035f, cSteel);
        DrawPFAngledBox({ 0.0f, baseY + 0.005f, opticZ - 0.018f }, { 0.028f, 0.005f, 0.026f }, 16.0f, 0.0f, 0.0f, cPolymer);
        DrawPFBox({ 0.0f, baseY + 0.008f, opticZ + 0.018f }, { 0.007f, 0.004f, 0.008f }, cSteel);
        // Ultra-lean forward-swept hood pillars (0.0014f thin!) spread wide to x = +/-0.022f
        DrawPFAngledBox({ -0.022f, sightY - 0.001f, opticZ - 0.008f }, { 0.0014f, 0.028f, 0.022f }, -24.0f, 0.0f, -10.0f, cDark);
        DrawPFAngledBox({  0.022f, sightY - 0.001f, opticZ - 0.008f }, { 0.0014f, 0.028f, 0.022f }, -24.0f, 0.0f,  10.0f, cDark);
        DrawPFAngledBox({ -0.015f, sightY + 0.015f, opticZ - 0.012f }, { 0.0014f, 0.013f, 0.020f }, -24.0f, 0.0f, -42.0f, cDark);
        DrawPFAngledBox({  0.015f, sightY + 0.015f, opticZ - 0.012f }, { 0.0014f, 0.013f, 0.020f }, -24.0f, 0.0f,  42.0f, cDark);
        DrawPFBox({ 0.0f, sightY + 0.018f, opticZ - 0.014f }, { 0.024f, 0.0014f, 0.018f }, cDark); // Slim upper crossbar
        // Edge-to-edge panoramic crystal lens & Pinpoint 1.5 MOA Red Dot
        DrawCubeV({ 0.0f, sightY, opticZ - 0.008f }, { 0.039f, 0.029f, 0.001f }, { 75, 205, 255, 20 });
        DrawCubeV({ 0.0f, sightY, opticZ - 0.008f }, { 0.0011f, 0.0011f, 0.001f }, { 255, 38, 38, 255 });

    } else if (effOptic == OPTIC_EOTECH552) {
        // Ultra-Lean EOTech 552: Sleek Protective Shroud & Delicate Holographic Reticle
        DrawPFBox({ 0.0f, baseY + 0.003f, opticZ + 0.005f }, { 0.034f, 0.008f, 0.048f }, cDark); // Low-profile riser
        DrawPFCylinder({ -0.018f, baseY + 0.004f, opticZ + 0.004f }, { -0.023f, baseY + 0.004f, opticZ + 0.004f }, 0.0045f, 0.0045f, 8, cSteel);
        DrawPFAngledBox({ 0.0f, baseY + 0.005f, opticZ - 0.032f }, { 0.032f, 0.010f, 0.044f }, -8.0f, 0.0f, 0.0f, cPolymer);
        DrawPFBox({ 0.0f, baseY + 0.009f, opticZ - 0.048f }, { 0.024f, 0.005f, 0.008f }, cSteel);
        // Slender aluminum roll-cage walls (0.0017f thin!) spread out to x = +/-0.023f for vast aperture
        DrawPFAngledBox({ -0.023f, sightY - 0.001f, opticZ + 0.008f }, { 0.0017f, 0.025f, 0.044f }, -5.0f, 0.0f,  -6.0f, cDark);
        DrawPFAngledBox({  0.023f, sightY - 0.001f, opticZ + 0.008f }, { 0.0017f, 0.025f, 0.044f }, -5.0f, 0.0f,   6.0f, cDark);
        DrawPFAngledBox({ -0.016f, sightY + 0.014f, opticZ + 0.008f }, { 0.0016f, 0.015f, 0.044f }, -5.0f, 0.0f, -40.0f, cDark);
        DrawPFAngledBox({  0.016f, sightY + 0.014f, opticZ + 0.008f }, { 0.0016f, 0.015f, 0.044f }, -5.0f, 0.0f,  40.0f, cDark);
        DrawPFBox({ 0.0f, sightY + 0.019f, opticZ + 0.008f }, { 0.024f, 0.0020f, 0.044f }, cDark); // Lean top bar
        // Broad clear holographic window
        DrawCubeV({ 0.0f, sightY, opticZ + 0.008f }, { 0.041f, 0.031f, 0.001f }, { 80, 200, 255, 18 });
        // Delicate Holographic Reticle: Fine 65 MOA outer quadrant ticks + 1 MOA pinpoint center dot
        DrawCubeV({ -0.0050f, sightY, opticZ + 0.008f }, { 0.0022f, 0.0006f, 0.001f }, { 255, 45, 45, 230 });
        DrawCubeV({  0.0050f, sightY, opticZ + 0.008f }, { 0.0022f, 0.0006f, 0.001f }, { 255, 45, 45, 230 });
        DrawCubeV({ 0.0f, sightY - 0.0050f, opticZ + 0.008f }, { 0.0006f, 0.0022f, 0.001f }, { 255, 45, 45, 230 });
        DrawCubeV({ 0.0f, sightY + 0.0050f, opticZ + 0.008f }, { 0.0006f, 0.0022f, 0.001f }, { 255, 45, 45, 230 });
        DrawCubeV({ 0.0f, sightY, opticZ + 0.008f }, { 0.0009f, 0.0009f, 0.001f }, { 255, 45, 45, 255 }); // Pinpoint center dot

    } else if (effOptic == OPTIC_OKP7) {
        // Ultra-Lean OKP-7: Authentic Rimless Design (75% Unobstructed Sight Picture!)
        DrawPFBox({ 0.0f, baseY + 0.002f, opticZ }, { 0.030f, 0.005f, 0.062f }, cDark);
        DrawPFCylinder({ -0.015f, baseY + 0.005f, opticZ + 0.016f }, { -0.015f, baseY + 0.011f, opticZ + 0.016f }, 0.005f, 0.005f, 8, cSteel);
        DrawPFAngledBox({ 0.0f, baseY + 0.005f, opticZ - 0.018f }, { 0.024f, 0.005f, 0.026f }, 18.0f, 0.0f, 0.0f, cDark);
        // Slender bottom-left support arm (0.0016f thin) tucked far to the left (x = -0.022f)
        DrawPFAngledBox({ -0.022f, sightY - 0.002f, opticZ }, { 0.0016f, 0.024f, 0.026f }, -10.0f, 0.0f, -14.0f, cDark);
        DrawPFAngledBox({ -0.014f, sightY + 0.014f, opticZ }, { 0.0014f, 0.018f, 0.022f }, -10.0f, 0.0f, -55.0f, cDark);
        DrawPFAngledBox({  0.004f, sightY + 0.018f, opticZ }, { 0.0012f, 0.018f, 0.022f }, -10.0f, 0.0f,  80.0f, cDark);
        // Completely rimless right & lower glass boundary
        DrawCubeV({ 0.0f, sightY, opticZ }, { 0.038f, 0.030f, 0.001f }, { 60, 220, 150, 20 });
        // Razor-Sharp Precision Green Chevron Reticle (needle-thin legs meeting at pinpoint apex)
        DrawPFAngledBox({ -0.0015f, sightY - 0.0013f, opticZ }, { 0.0034f, 0.00055f, 0.001f }, 0, 0,  35.0f, { 45, 255, 85, 255 });
        DrawPFAngledBox({  0.0015f, sightY - 0.0013f, opticZ }, { 0.0034f, 0.00055f, 0.001f }, 0, 0, -35.0f, { 45, 255, 85, 255 });

    } else if (effOptic == OPTIC_ACOG || effOptic == OPTIC_PMII_SCOPE) {
        float tubeLen = (effOptic == OPTIC_PMII_SCOPE) ? 0.24f : 0.135f;
        // Lean Rail Base + Low-Profile Clamshell Scope Rings
        DrawPFBox({ 0.0f, baseY + 0.003f, opticZ }, { 0.028f, 0.006f, tubeLen * 0.68f }, cDark);
        float tubeR = (effOptic == OPTIC_PMII_SCOPE) ? 0.023f : 0.021f; // Enlarged internal viewing aperture
        for (int r = -1; r <= 1; r += 2) {
            float rz = opticZ + r * (tubeLen * 0.24f);
            DrawPFReceiverPin({ 0.0f, baseY + 0.003f, rz }, 0.036f, 0.0035f, cSteel);
            Vector3 ringCenter = { 0.0f, baseY + 0.014f, rz };
            DrawPFScopeRing(ringCenter, tubeR, cDark, cSteel);
        }

        if (adsAlpha < 0.85f || effOptic == OPTIC_ACOG) {
            // Ultra-Lean Octagonal Hollow Scope Tube (0.0014f thin walls, spacious clear tunnel)
            float r = tubeR;
            float tw = 0.0014f; // Razor-thin housing wall thickness!
            DrawPFBox({ -r, sightY, opticZ }, { tw, r * 1.05f, tubeLen }, cDark);
            DrawPFBox({  r, sightY, opticZ }, { tw, r * 1.05f, tubeLen }, cDark);
            DrawPFBox({ 0.0f, sightY + r, opticZ }, { r * 1.05f, tw, tubeLen }, cDark);
            DrawPFBox({ 0.0f, sightY - r, opticZ }, { r * 1.05f, tw, tubeLen }, cDark);
            float cr = r * 0.76f;
            DrawPFAngledBox({ -cr, sightY + cr, opticZ }, { tw, r * 0.80f, tubeLen }, 0.0f, 0.0f, -45.0f, cDark);
            DrawPFAngledBox({  cr, sightY + cr, opticZ }, { tw, r * 0.80f, tubeLen }, 0.0f, 0.0f,  45.0f, cDark);
            DrawPFAngledBox({ -cr, sightY - cr, opticZ }, { tw, r * 0.80f, tubeLen }, 0.0f, 0.0f,  45.0f, cDark);
            DrawPFAngledBox({  cr, sightY - cr, opticZ }, { tw, r * 0.80f, tubeLen }, 0.0f, 0.0f, -45.0f, cDark);

            // Sleek, minimal front & rear bezel lip rings (0.0011f thin)
            for (int endDir = -1; endDir <= 1; endDir += 2) {
                float bz = opticZ + endDir * (tubeLen * 0.47f);
                float br = r + 0.0014f;
                float bw = 0.0011f;
                DrawPFBox({ -br, sightY, bz }, { bw, br * 1.1f, 0.012f }, cPolymer);
                DrawPFBox({  br, sightY, bz }, { bw, br * 1.1f, 0.012f }, cPolymer);
                DrawPFBox({ 0.0f, sightY + br, bz }, { br * 1.1f, bw, 0.012f }, cPolymer);
                DrawPFBox({ 0.0f, sightY - br, bz }, { br * 1.1f, bw, 0.012f }, cPolymer);
            }

            if (effOptic == OPTIC_ACOG) {
                // Trijicon ACOG TA31: Slim Top Fiber-Optic Tube & Compact Turrets
                DrawPFCylinder({ 0.0f, sightY + r + 0.003f, opticZ + 0.02f },
                               { 0.0f, sightY + r + 0.001f, opticZ - 0.055f },
                               0.0022f, 0.0018f, 6, { 255, 45, 35, 255 });
                DrawPFCylinder({ 0.0f, sightY + r, opticZ + 0.028f }, { 0.0f, sightY + r + 0.006f, opticZ + 0.028f }, 0.005f, 0.005f, 8, cDark);
                DrawPFCylinder({ r, sightY, opticZ + 0.008f }, { r + 0.006f, sightY, opticZ + 0.008f }, 0.005f, 0.005f, 8, cDark);
                // Ultra-Fine ACOG Reticle: Crisp red micro-chevron + delicate BDC stadia lines
                DrawPFAngledBox({ -0.0013f, sightY - 0.0009f, opticZ }, { 0.0028f, 0.00055f, 0.001f }, 0, 0,  36.0f, { 255, 38, 38, 255 });
                DrawPFAngledBox({  0.0013f, sightY - 0.0009f, opticZ }, { 0.0028f, 0.00055f, 0.001f }, 0, 0, -36.0f, { 255, 38, 38, 255 });
                DrawCubeV({ 0.0f, sightY - 0.006f, opticZ }, { 0.00035f, 0.010f, 0.001f }, { 25, 25, 25, 220 }); // Fine vertical drop line
                DrawCubeV({ 0.0f, sightY - 0.004f, opticZ }, { 0.0028f, 0.00035f, 0.001f }, { 25, 25, 25, 220 }); // 200m hash
                DrawCubeV({ 0.0f, sightY - 0.007f, opticZ }, { 0.0042f, 0.00035f, 0.001f }, { 25, 25, 25, 220 }); // 400m hash
                DrawCubeV({ 0.0f, sightY - 0.010f, opticZ }, { 0.0056f, 0.00035f, 0.001f }, { 25, 25, 25, 220 }); // 600m hash
                DrawCubeV({ -0.011f, sightY, opticZ }, { 0.008f, 0.0004f, 0.001f }, { 25, 25, 25, 210 }); // Left framing bar
                DrawCubeV({  0.011f, sightY, opticZ }, { 0.008f, 0.0004f, 0.001f }, { 25, 25, 25, 210 }); // Right framing bar
            } else {
                // Schmidt & Bender PM II: Low-Profile Precision Turrets & Hairline Crosshairs
                DrawPFCylinder({ 0.0f, sightY + r, opticZ }, { 0.0f, sightY + r + 0.010f, opticZ }, 0.0075f, 0.0075f, 8, cDark);
                DrawPFCylinder({  r, sightY, opticZ }, {  r + 0.010f, sightY, opticZ }, 0.0075f, 0.0075f, 8, cDark);
                DrawPFCylinder({ -r, sightY, opticZ }, { -r - 0.008f, sightY, opticZ }, 0.0065f, 0.0065f, 8, cDark);
                // Hairline Crosshairs + Pinpoint 0.8 MOA Center Aiming Pip
                DrawCubeV({ 0.0f, sightY, opticZ }, { 0.034f, 0.00040f, 0.001f }, { 20, 22, 26, 230 });
                DrawCubeV({ 0.0f, sightY, opticZ }, { 0.00040f, 0.034f, 0.001f }, { 20, 22, 26, 230 });
                DrawCubeV({ 0.0f, sightY, opticZ }, { 0.00085f, 0.00085f, 0.001f }, { 255, 38, 38, 255 });
            }
        }
    }

    // ------------------------------------------------------------------------
    // 6. MUZZLE FLASH (When firing & unsuppressed)
    // ------------------------------------------------------------------------
    if (muzzleFlashTimer > 0.0f && att.barrel != BARREL_SUPPRESSOR && att.barrel != BARREL_ARS_SUPPRESSOR) {
        float s = 0.07f + (std::rand() % 40) * 0.0015f;
        DrawCubeV(Vector3Add(flashOrigin, { 0, 0, -0.04f }), { s * 1.8f, s * 0.45f, s * 0.45f }, { 255, 215, 85, 230 });
        DrawCubeV(Vector3Add(flashOrigin, { 0, 0, -0.04f }), { s * 0.45f, s * 1.8f, s * 0.45f }, { 255, 175, 45, 230 });
        DrawCubeV(Vector3Add(flashOrigin, { 0, 0, -0.07f }), { s * 0.55f, s * 0.55f, s * 1.6f }, { 255, 245, 180, 245 });
    }

    // ------------------------------------------------------------------------
    // 7. DYNAMIC BLOCKY FIRST-PERSON ARMS (With Tactical Watch, Cuff & Glove Knuckles)
    // ------------------------------------------------------------------------
    if (drawArms) {
        Color sleeveCol = { 44, 68, 102, 255 };
        Color cuffCol   = { 32, 48, 74, 255 };
        Color gloveCol  = { 28, 30, 34, 255 };
        Color knuckleCol= { 46, 50, 56, 255 };

        // Right Hand + Forearm Sleeve + Tactical Knuckle Guard on Pistol Grip
        DrawPFAngledBox({ 0.052f, -0.098f, 0.185f }, { 0.066f, 0.066f, 0.22f }, 12.0f, -10.0f, 0.0f, sleeveCol);
        DrawPFBox({ 0.025f, -0.082f, 0.105f }, { 0.062f, 0.060f, 0.032f }, cuffCol);
        DrawPFBox({ 0.012f, -0.075f, 0.078f }, { 0.056f, 0.058f, 0.062f }, gloveCol);
        DrawPFBox({ 0.026f, -0.068f, 0.074f }, { 0.036f, 0.048f, 0.022f }, knuckleCol);

        // Left Hand + Forearm + Wristwatch (tracks magazine during reload, or grips underbarrel foregrip!)
        Vector3 lhBase = stats.isPrimary ? leftHandGripPos : Vector3{ -0.018f, -0.075f, 0.048f };
        Vector3 leftHand = Vector3Add(lhBase, Vector3Scale(magOffset, 0.90f));
        DrawPFBox(leftHand, { 0.058f, 0.058f, 0.070f }, gloveCol);
        DrawPFBox(Vector3Add(leftHand, { -0.014f, -0.008f, 0.0f }), { 0.038f, 0.046f, 0.026f }, knuckleCol);
        // Tactical digital wristwatch on left wrist
        DrawPFBox(Vector3Add(leftHand, { -0.016f, -0.010f, 0.042f }), { 0.062f, 0.062f, 0.018f }, cDark);
        DrawPFBox(Vector3Add(leftHand, {  0.012f,  0.018f, 0.042f }), { 0.016f, 0.012f, 0.016f }, { 45, 195, 135, 255 });
        DrawPFAngledBox(Vector3Add(leftHand, { -0.052f, -0.042f, 0.155f }),
                        { 0.066f, 0.066f, 0.24f }, 14.0f, 14.0f, 0.0f, sleeveCol);
    }
}
