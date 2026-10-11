#pragma once
#include "entities.hpp"
#include <cstdio>

// ============================================================================
// PHANTOM FORCES WORLD SIMULATION: MOVEMENT, BALLISTICS, WALLBANGS, AI & MODES
// ============================================================================

struct GameWorld {
    MapId currentMapId = MAP_DESERT_STORM;
    GameModeId currentMode = MODE_FLARE_DOMINATION;
    GameMap map;

    Player player;
    std::vector<BotAgent> bots;
    std::vector<Projectile> projectiles;
    std::vector<ShellCasing> shells;
    std::vector<ImpactDecal> decals;
    std::vector<FragGrenade> grenades;
    std::vector<KillFeedEntry> killFeed;
    std::vector<XpPopup> xpPopups;

    int teamScore[2] = { 0, 0 }; // [0] = Phantoms (Blue), [1] = Ghosts (Orange)
    float matchTimer = 600.0f;   // 10-minute match
    float objectiveTickTimer = 0.0f;
    bool inDeployMenu = true;    // Starts on PF Loadout/Deploy Screen

    SoundBank* audio = nullptr;

    void AddXpEvent(const std::string& label, int pts) {
        player.score += pts;
        xpPopups.push_back({ label, pts, 3.0f });
        if (xpPopups.size() > 6) {
            xpPopups.erase(xpPopups.begin());
        }
    }

    void PushKillFeed(const std::string& killer, int kTeam, const std::string& wpn,
                      const std::string& victim, int vTeam, bool hs, bool wb) {
        killFeed.push_back({ killer, kTeam, wpn, victim, vTeam, hs, wb, 6.5f });
        if (killFeed.size() > 6) {
            killFeed.erase(killFeed.begin());
        }
    }

    void LoadMapAndReset(MapId mId, GameModeId modeId) {
        currentMapId = mId;
        currentMode = modeId;
        map = BuildMap(mId);

        projectiles.clear();
        shells.clear();
        decals.clear();
        grenades.clear();
        killFeed.clear();
        xpPopups.clear();
        teamScore[0] = 0;
        teamScore[1] = 0;
        matchTimer = 600.0f;

        // Spawn 14 Allied Phantoms (+ Player = 15 Team A) and 15 Enemy Ghosts (15 Team B) for 15v15 Blueprint Combat!
        bots.clear();
        const char* phantomNames[14] = {
            "StyLiS_Lito", "Shaylan007", "AxisAngle", "Phantom_Recon",
            "GhostHunter99", "KektorMain", "Rank100_Vet", "Mar_PF",
            "Blue_Vanguard", "Delta_Striker", "CQB_Specialist", "Apex_Phantom",
            "Railgun_Optic", "VaultBreaker"
        };
        const char* ghostNames[15] = {
            "Anti_Sight", "Toben_PF", "DesertSniper", "MetroSweeper",
            "WallbangKing", "SlideCancel", "SuperJumper", "M60_Suppressor",
            "Orange_Reaper", "FoundryBoss", "DocksideRaider", "Atlas_Sniper",
            "MesaOutrider", "MonasteryGuard", "BFG_PreBuyer"
        };

        WeaponId botGuns[] = { WPN_M4A1, WPN_AK47, WPN_MP7, WPN_INTERVENTION, WPN_REMINGTON870, WPN_M60 };

        for (int i = 0; i < 14; ++i) {
            BotAgent b;
            b.id = i + 1;
            b.name = phantomNames[i];
            b.team = 0;
            b.weaponId = botGuns[i % 6];
            b.attachments.optic  = (OpticType)(i % (int)OPTIC_COUNT);
            b.attachments.barrel = (BarrelType)(i % (int)BARREL_COUNT);
            b.attachments.grip   = (UnderbarrelType)(i % (int)GRIP_COUNT);
            b.pos = map.phantomSpawns[i % map.phantomSpawns.size()];
            b.pos.x += ((i % 5) - 2) * 1.4f;
            b.pos.z += ((i / 5) - 1) * 1.4f;
            b.yaw = 0.0f;
            b.lastProgressPos = b.pos;
            b.targetWaypoint = map.FindClosestWaypoint(b.pos);
            b.goalWaypoint = (i * 5 + 3) % (int)map.waypoints.size();
            bots.push_back(b);
        }

        for (int i = 0; i < 15; ++i) {
            BotAgent b;
            b.id = 100 + i;
            b.name = ghostNames[i];
            b.team = 1;
            b.weaponId = botGuns[i % 6];
            b.attachments.optic  = (OpticType)((i + 1) % (int)OPTIC_COUNT);
            b.attachments.barrel = (BarrelType)((i + 2) % (int)BARREL_COUNT);
            b.attachments.grip   = (UnderbarrelType)((i + 1) % (int)GRIP_COUNT);
            b.pos = map.ghostSpawns[i % map.ghostSpawns.size()];
            b.pos.x += ((i % 5) - 2) * 1.4f;
            b.pos.z += ((i / 5) - 1) * 1.4f;
            b.yaw = PI;
            b.lastProgressPos = b.pos;
            b.targetWaypoint = map.FindClosestWaypoint(b.pos);
            b.goalWaypoint = (i * 5 + 1) % (int)map.waypoints.size();
            bots.push_back(b);
        }

        Vector3 pSpawn = map.phantomSpawns[0];
        player.SpawnAt(pSpawn, PI);
    }

    void DeployPlayer() {
        Vector3 sp = map.phantomSpawns[std::rand() % map.phantomSpawns.size()];
        float spawnYaw = std::atan2(-sp.x, -sp.z);
        player.SpawnAt(sp, spawnYaw);
        inDeployMenu = false;
        DisableCursor();
    }

    // Check clear line-of-sight between two world points (ignoring glass)
    bool HasLineOfSight(Vector3 a, Vector3 b) const {
        Vector3 diff = Vector3Subtract(b, a);
        float dist = Vector3Length(diff);
        if (dist < 0.05f) return true;
        Vector3 dir = Vector3Scale(diff, 1.0f / dist);

        for (const auto& blk : map.blocks) {
            if (!blk.collidable || blk.material == MAT_GLASS) continue;
            RayBoxHit hit = IntersectRayAABB(a, dir, blk.GetBounds());
            if (hit.hit && hit.tNear >= 0.0f && hit.tNear < dist) {
                return false;
            }
        }
        return true;
    }

    // Check if a bot can wallbang through thin cover to hit a remembered target position
    bool CanBotWallbangTarget(Vector3 a, Vector3 b, float maxPenetration) const {
        Vector3 diff = Vector3Subtract(b, a);
        float dist = Vector3Length(diff);
        if (dist < 0.1f || dist > 42.0f) return false;
        Vector3 dir = Vector3Scale(diff, 1.0f / dist);

        float totalCost = 0.0f;
        for (const auto& blk : map.blocks) {
            if (!blk.collidable || blk.material == MAT_GLASS) continue;
            RayBoxHit hit = IntersectRayAABB(a, dir, blk.GetBounds());
            if (hit.hit && hit.tNear <= dist && hit.tFar >= 0.0f) {
                float enterT = std::max(0.0f, hit.tNear);
                float exitT  = std::min(dist, std::max(enterT, hit.tFar));
                float rawThick = exitT - enterT;
                totalCost += rawThick * GetMaterialPenetrationCost(blk.material);
                if (totalCost > maxPenetration * 0.88f) return false;
            }
        }
        return (totalCost > 0.01f && totalCost <= maxPenetration * 0.88f);
    }

    // Cast a chest-high horizontal feeler ray for local obstacle avoidance whiskers
    float CastSteeringRay(Vector3 footPos, Vector3 flatDir, float maxDist) const {
        Vector3 orig = { footPos.x, footPos.y + 1.15f, footPos.z };
        float bestT = maxDist;
        for (const auto& blk : map.blocks) {
            if (!blk.collidable || blk.material == MAT_GLASS) continue;
            BoundingBox box = blk.GetBounds();
            if (box.max.y <= footPos.y + 0.55f || box.min.y >= footPos.y + 1.75f) continue;
            RayBoxHit hit = IntersectRayAABB(orig, flatDir, box);
            if (hit.hit && hit.tNear >= 0.0f && hit.tNear < bestT) {
                bestT = hit.tNear;
            }
        }
        return bestT;
    }

    // Spawn a hitscan shot with wallbang penetration, instantaneous damage & visual tracer
    void SpawnWeaponFire(
        int ownerId,
        int team,
        Vector3 origin,
        Vector3 aimDir,
        const WeaponStats& stats,
        const WeaponAttachments& att,
        float currentSpread
    ) {
        if (audio) {
            if (att.barrel == BARREL_SUPPRESSOR || att.barrel == BARREL_ARS_SUPPRESSOR) {
                audio->Play(audio->sfxSuppressedShot, 0.06f);
            } else if (stats.id == WPN_INTERVENTION) {
                audio->Play(audio->sfxSniperShot, 0.04f);
            } else if (stats.id == WPN_REMINGTON870) {
                audio->Play(audio->sfxShotgunShot, 0.05f);
            } else if (stats.id == WPN_MP7 || stats.id == WPN_G17) {
                audio->Play(audio->sfxSmgShot, 0.06f);
            } else {
                audio->Play(audio->sfxRifleShot, 0.05f);
            }
        }

        Vector3 right = Vector3Normalize(Vector3CrossProduct(aimDir, { 0.0f, 1.0f, 0.0f }));
        if (Vector3LengthSqr(right) < 0.001f) right = { 1.0f, 0.0f, 0.0f };
        Vector3 up = Vector3Normalize(Vector3CrossProduct(right, aimDir));

        // Spawn 3D Ejected Brass Shell Casing (or 12-gauge shotgun hull)
        if (shells.size() < 90) {
            ShellCasing sc;
            sc.active = true;
            sc.pos = Vector3Add(origin, Vector3Add(Vector3Scale(aimDir, 0.32f), Vector3Scale(right, 0.16f)));
            float rJitter = ((std::rand() % 100) / 100.0f) * 1.4f;
            sc.vel = Vector3Add(Vector3Scale(right, 2.8f + rJitter), { 0.0f, 2.2f + rJitter * 0.5f, 0.0f });
            sc.rotDeg = { 0.0f, 0.0f, 0.0f };
            sc.rotVel = { 540.0f + rJitter * 200.0f, 380.0f, 720.0f };
            sc.isShotgunShell = (stats.id == WPN_REMINGTON870);
            sc.color = sc.isShotgunShell ? Color{ 215, 42, 42, 255 } : Color{ 230, 190, 68, 255 };
            sc.timer = 2.6f;
            shells.push_back(sc);
        }

        for (int p = 0; p < stats.pellets; ++p) {
            float rx = 0.0f;
            float ry = 0.0f;
            if (stats.pellets > 1) {
                // Uniform circular disc distribution for shotgun pellets
                float r = std::sqrt(((float)(std::rand() % 1000) + 1.0f) / 1000.0f) * currentSpread;
                float theta = (((float)(std::rand() % 3600)) / 3600.0f) * 2.0f * PI;
                rx = r * std::cos(theta);
                ry = r * std::sin(theta);
            } else {
                rx = (((float)(std::rand() % 2000) / 1000.0f) - 1.0f) * currentSpread;
                ry = (((float)(std::rand() % 2000) / 1000.0f) - 1.0f) * currentSpread;
            }
            Vector3 dir = Vector3Normalize(Vector3Add(aimDir, Vector3Add(Vector3Scale(right, rx), Vector3Scale(up, ry))));

            // ================================================================
            // INSTANTANEOUS HITSCAN RAYCAST RESOLUTION
            // ================================================================
            Vector3 rayOrig = origin;
            Vector3 rayDir = dir;
            float remainingDist = 350.0f;
            float currentPen = stats.penetration;
            bool hasWallbanged = false;
            float totalDistTraveled = 0.0f;
            Vector3 finalHitPos = Vector3Add(origin, Vector3Scale(dir, remainingDist));

            for (int seg = 0; seg < 4; ++seg) {
                // 1. Closest collidable map block
                float closestBlockT = remainingDist;
                const MapBlock* hitBlock = nullptr;
                RayBoxHit blockHit;

                for (const auto& blk : map.blocks) {
                    if (!blk.collidable) continue;
                    RayBoxHit h = IntersectRayAABB(rayOrig, rayDir, blk.GetBounds());
                    if (h.hit && h.tNear >= 0.0f && h.tNear < closestBlockT) {
                        closestBlockT = h.tNear;
                        hitBlock = &blk;
                        blockHit = h;
                    }
                }

                // 2. Closest character (closer than block)
                float closestCharT = closestBlockT;
                BotAgent* hitBot = nullptr;
                int hitZone = -1; // 2=head, 1=torso, 0=limbs
                bool hitPlayer = false;

                for (auto& b : bots) {
                    if (!b.alive || b.team == team) continue;
                    RayBoxHit hHead = IntersectRayAABB(rayOrig, rayDir, b.GetHeadBox());
                    if (hHead.hit && hHead.tNear >= 0.0f && hHead.tNear < closestCharT) {
                        closestCharT = hHead.tNear;
                        hitBot = &b;
                        hitZone = 2;
                        hitPlayer = false;
                    } else {
                        RayBoxHit hTorso = IntersectRayAABB(rayOrig, rayDir, b.GetTorsoBox());
                        if (hTorso.hit && hTorso.tNear >= 0.0f && hTorso.tNear < closestCharT) {
                            closestCharT = hTorso.tNear;
                            hitBot = &b;
                            hitZone = 1;
                            hitPlayer = false;
                        } else {
                            RayBoxHit hLimbs = IntersectRayAABB(rayOrig, rayDir, b.GetLimbsBox());
                            if (hLimbs.hit && hLimbs.tNear >= 0.0f && hLimbs.tNear < closestCharT) {
                                closestCharT = hLimbs.tNear;
                                hitBot = &b;
                                hitZone = 0;
                                hitPlayer = false;
                            }
                        }
                    }
                }

                if (team == 1 && player.alive && !inDeployMenu) {
                    BoundingBox pBox = {
                        { player.pos.x - 0.38f, player.pos.y, player.pos.z - 0.38f },
                        { player.pos.x + 0.38f, player.pos.y + player.eyeHeightCurrent + 0.18f, player.pos.z + 0.38f }
                    };
                    RayBoxHit hPlayer = IntersectRayAABB(rayOrig, rayDir, pBox);
                    if (hPlayer.hit && hPlayer.tNear >= 0.0f && hPlayer.tNear < closestCharT) {
                        closestCharT = hPlayer.tNear;
                        hitPlayer = true;
                        hitBot = nullptr;
                        hitZone = 1;
                    }
                }

                // If character was hit first:
                if (closestCharT < closestBlockT) {
                    totalDistTraveled += closestCharT;
                    finalHitPos = Vector3Add(rayOrig, Vector3Scale(rayDir, closestCharT));
                    float wallbangDmgScale = hasWallbanged ? 0.78f : 1.0f;

                    if (hitBot != nullptr) {
                        float dmg = CalculatePFDamage(stats, totalDistTraveled, hitZone) * wallbangDmgScale;
                        hitBot->health -= dmg;

                        // Immediate threat reflex
                        hitBot->AlertToThreat(origin, { 0, 0, 0 }, 6.0f);
                        for (auto& sq : bots) {
                            if (sq.alive && sq.team == hitBot->team && sq.id != hitBot->id) {
                                if (Vector3Distance(sq.pos, hitBot->pos) < 18.0f) {
                                    sq.AlertToThreat(origin, { 0, 0, 0 }, 4.5f);
                                }
                            }
                        }

                        bool killed = (hitBot->health <= 0.0f);
                        if (ownerId == 0) {
                            player.hitmarkerTimer = 0.18f;
                            player.hitmarkerHeadshot = (hitZone == 2);
                            player.hitmarkerKill = killed;
                            if (audio) {
                                audio->Play((hitZone == 2 || killed) ? audio->sfxHeadshot : audio->sfxHitmarker, 0.02f);
                            }
                        }

                        if (killed) {
                            hitBot->alive = false;
                            hitBot->deaths++;
                            hitBot->respawnTimer = 4.0f;
                            teamScore[team]++;

                            std::string killerName = "YOU";
                            if (ownerId == 0) {
                                player.kills++;
                                AddXpEvent("[+100 Enemy Killed]", 100);
                                if (hitZone == 2) AddXpEvent("[+25 Headshot Bonus]", 25);
                                if (hasWallbanged) AddXpEvent("[+25 Wallbang Bonus]", 25);
                                if (totalDistTraveled >= 45.0f) AddXpEvent("[+50 Longshot Bonus]", 50);
                            } else {
                                for (auto& ally : bots) {
                                    if (ally.id == ownerId) {
                                        ally.kills++;
                                        killerName = ally.name;
                                        break;
                                    }
                                }
                                if (hitBot->spottedTimer > 0.0f) {
                                    AddXpEvent("[+25 Spot Assist]", 25);
                                }
                            }
                            PushKillFeed(killerName, team, stats.name, hitBot->name, hitBot->team, (hitZone == 2), hasWallbanged);
                        }
                    } else if (hitPlayer) {
                        float dmg = CalculatePFDamage(stats, totalDistTraveled, 1) * 0.55f * wallbangDmgScale;
                        player.health -= dmg;
                        player.timeSinceDamage = 0.0f;
                        player.lastDamageFromPos = origin;
                        player.damageIndicatorTimer = 1.2f;
                        player.camRecoilSpring.Shove({ 1.4f, 0.8f, 0.0f });

                        if (player.health <= 0.0f) {
                            player.alive = false;
                            player.deaths++;
                            teamScore[1]++;
                            std::string killerName = "Ghost_Operative";
                            for (auto& b : bots) {
                                if (b.id == ownerId) {
                                    b.kills++;
                                    killerName = b.name;
                                    break;
                                }
                            }
                            PushKillFeed(killerName, 1, stats.name, "YOU", 0, false, hasWallbanged);
                            inDeployMenu = true;
                            EnableCursor();
                        }
                    }
                    break;
                }

                // If block was hit:
                if (hitBlock != nullptr) {
                    totalDistTraveled += closestBlockT;
                    finalHitPos = blockHit.point;

                    float enterT = std::max(0.0f, blockHit.tNear);
                    float exitT  = std::max(enterT, blockHit.tFar);
                    float rawThickness = exitT - enterT;
                    float effectiveCost = rawThickness * GetMaterialPenetrationCost(hitBlock->material);

                    if (blockHit.tNear >= 0.0f && decals.size() < 160) {
                        decals.push_back({ blockHit.point, blockHit.normal, { 28, 30, 34, 230 }, 7.0f, false });
                    }

                    if (currentPen >= effectiveCost && effectiveCost > 0.001f) {
                        currentPen -= effectiveCost;
                        hasWallbanged = true;
                        Vector3 exitPoint = Vector3Add(rayOrig, Vector3Scale(rayDir, exitT));
                        if (decals.size() < 160) {
                            decals.push_back({ exitPoint, Vector3Negate(blockHit.normal), { 255, 165, 40, 230 }, 7.0f, true });
                        }
                        rayOrig = Vector3Add(exitPoint, Vector3Scale(rayDir, 0.02f));
                        remainingDist -= exitT;
                        if (remainingDist <= 0.0f) break;
                    } else {
                        break; // Wall stops bullet
                    }
                } else {
                    totalDistTraveled += remainingDist;
                    finalHitPos = Vector3Add(rayOrig, Vector3Scale(rayDir, remainingDist));
                    break;
                }
            }

            // Spawn Visual Hitscan Tracer
            Projectile proj;
            proj.active = true;
            proj.ownerId = ownerId;
            proj.team = team;
            proj.weaponId = stats.id;
            proj.attachments = att;
            proj.startPos = origin;
            proj.hitPos = finalHitPos;
            proj.pos = origin;
            proj.prevPos = origin;
            proj.maxDistance = Vector3Distance(origin, finalHitPos);
            proj.vel = Vector3Scale(dir, 650.0f); // High-speed visual streak
            proj.penetrationLeft = currentPen;
            proj.distanceTraveled = 0.0f;
            proj.lifeTime = 0.16f;
            proj.hasWallbanged = hasWallbanged;
            if (ownerId == 0) {
                proj.tracerColor = Color{ 255, 228, 80, 255 }; // Player: bright golden-yellow tracer
            } else if (team == 0) {
                proj.tracerColor = Color{ 85, 215, 255, 255 }; // Allied Phantom: bright cyan tracer
            } else {
                proj.tracerColor = Color{ 255, 130, 45, 255 }; // Enemy Ghost: bright orange-red tracer
            }
            projectiles.push_back(proj);
        }
    }

    // Update First-Person Player Input, PF Movement Tech, Viewmodel Springs & Combat
    void UpdatePlayer(float dt) {
        if (!player.alive) return;

        // 1. Mouse Look + Sniper Steady Breath (Hold Shift while ADS)
        Vector2 mouseDelta = GetMouseDelta();
        float sens = 0.0022f;
        if (player.isAiming) {
            sens *= (player.currentFov / player.baseHipFov) * 0.85f;
        }
        player.yaw   -= mouseDelta.x * sens;
        player.pitch -= mouseDelta.y * sens;
        player.pitch = std::clamp(player.pitch, -1.48f, 1.48f);

        player.swaySpring.Shove({ -mouseDelta.x * 0.0028f, mouseDelta.y * 0.0028f, -mouseDelta.x * 0.0045f });

        // 2. Weapon Switching (1 = Primary, 2 = Secondary, V = FireMode, T = Canted Sight, H = Inspect)
        if (IsKeyPressed(KEY_ONE) && player.activeSlot != 0) {
            player.activeSlot = 0;
            player.isReloading = false;
            player.inspectTimer = 0.0f;
            player.gunPosSpring.Shove({ 0.0f, -0.8f, 0.4f });
            player.gunRotSpring.Shove({ -18.0f, 0.0f, 12.0f });
        }
        if (IsKeyPressed(KEY_TWO) && player.activeSlot != 1) {
            player.activeSlot = 1;
            player.isReloading = false;
            player.inspectTimer = 0.0f;
            player.gunPosSpring.Shove({ 0.0f, -0.8f, 0.4f });
            player.gunRotSpring.Shove({ -18.0f, 0.0f, 12.0f });
        }

        WeaponStats activeStats = player.GetActiveStats();
        const WeaponAttachments& activeAtt = player.GetActiveAttachments();

        if (IsKeyPressed(KEY_V) && activeStats.defaultFireMode != FIRE_BOLT_OR_PUMP) {
            FireMode& fm = player.currentFireMode[player.activeSlot];
            if (fm == FIRE_AUTO) fm = FIRE_BURST;
            else if (fm == FIRE_BURST) fm = FIRE_SEMI;
            else fm = FIRE_AUTO;
            if (audio) audio->Play(audio->sfxReloadClick, 0.05f);
        }

        if (IsKeyPressed(KEY_T) && activeAtt.other == OTHER_CANTED_DELTA) {
            player.useCantedSight = !player.useCantedSight;
            player.gunRotSpring.Shove({ 0.0f, 0.0f, player.useCantedSight ? -12.0f : 12.0f });
            if (audio) audio->Play(audio->sfxReloadClick, 0.08f);
        }

        // Weapon Inspect ('H' key)
        if (IsKeyPressed(KEY_H) && !player.isReloading && !player.isAiming) {
            player.inspectTimer = player.inspectDuration;
            player.gunPosSpring.Shove({ 0.0f, 0.25f, 0.0f });
        }
        if (player.inspectTimer > 0.0f) {
            player.inspectTimer = std::max(0.0f, player.inspectTimer - dt);
        }

        // 3. Movement Input Vector
        Vector3 fwd = player.GetFlatForward();
        Vector3 rgt = player.GetRight();
        Vector3 wishDir = { 0, 0, 0 };
        if (IsKeyDown(KEY_W)) wishDir = Vector3Add(wishDir, fwd);
        if (IsKeyDown(KEY_S)) wishDir = Vector3Subtract(wishDir, fwd);
        if (IsKeyDown(KEY_D)) wishDir = Vector3Add(wishDir, rgt);
        if (IsKeyDown(KEY_A)) wishDir = Vector3Subtract(wishDir, rgt);
        bool hasMoveInput = Vector3LengthSqr(wishDir) > 0.01f;
        if (hasMoveInput) wishDir = Vector3Normalize(wishDir);

        // 4. Vaulting Over Obstacles (Active Interpolation)
        if (player.isVaulting) {
            player.vaultTimer += dt * 3.8f;
            if (player.vaultTimer >= 1.0f) {
                player.pos = player.vaultTarget;
                player.isVaulting = false;
                player.vel = Vector3Scale(fwd, 7.5f);
            } else {
                player.pos = Vector3Lerp(player.vaultStart, player.vaultTarget, player.vaultTimer);
                player.pos.y += std::sin(player.vaultTimer * PI) * 0.45f;
            }
        } else {
            bool shiftHeld = IsKeyDown(KEY_LEFT_SHIFT);
            player.sprinting = shiftHeld && IsKeyDown(KEY_W) && !player.isAiming &&
                               player.posture == POSTURE_STAND && player.diveRecoveryTimer <= 0.0f;

            // A) SLIDING: Press 'C' or 'Left Control' while sprinting on ground
            bool crouchPressed = IsKeyPressed(KEY_C) || IsKeyPressed(KEY_LEFT_CONTROL);
            if (crouchPressed) {
                if (player.grounded && (player.sprinting || Vector2Length({ player.vel.x, player.vel.z }) > 8.5f) &&
                    player.posture != POSTURE_SLIDE) {
                    player.posture = POSTURE_SLIDE;
                    player.slideTimer = 0.78f;
                    player.slideDir = hasMoveInput ? wishDir : fwd;
                    float slideSpeed = 15.5f * activeStats.walkSpeedMult;
                    player.vel.x = player.slideDir.x * slideSpeed;
                    player.vel.z = player.slideDir.z * slideSpeed;
                    player.sprinting = false;
                    player.gunRotSpring.Shove({ 4.0f, 0.0f, -14.0f });
                } else if (player.posture == POSTURE_STAND) {
                    player.posture = POSTURE_CROUCH;
                } else if (player.posture == POSTURE_CROUCH || player.posture == POSTURE_PRONE) {
                    player.posture = POSTURE_STAND;
                }
            }

            // B) DOLPHIN DIVE & MID-AIR SUPERJUMP: Press 'Z' or 'X'
            bool divePressed = IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_X);
            if (divePressed && player.diveRecoveryTimer <= 0.0f) {
                if (!player.grounded && !player.usedAirDive) {
                    player.usedAirDive = true;
                    player.posture = POSTURE_DIVE;
                    Vector3 boostDir = hasMoveInput ? wishDir : fwd;
                    player.vel.x = boostDir.x * 13.5f;
                    player.vel.z = boostDir.z * 13.5f;
                    player.vel.y = 8.4f;
                    player.diveRecoveryTimer = 0.55f;
                    player.gunRotSpring.Shove({ -22.0f, 0.0f, 10.0f });
                } else if (player.grounded && (player.sprinting || hasMoveInput)) {
                    player.posture = POSTURE_DIVE;
                    player.usedAirDive = true;
                    Vector3 boostDir = hasMoveInput ? wishDir : fwd;
                    player.vel.x = boostDir.x * 14.2f;
                    player.vel.z = boostDir.z * 14.2f;
                    player.vel.y = 6.2f;
                    player.grounded = false;
                    player.diveRecoveryTimer = 0.55f;
                    player.gunRotSpring.Shove({ -18.0f, 0.0f, 8.0f });
                } else {
                    player.posture = (player.posture == POSTURE_PRONE) ? POSTURE_STAND : POSTURE_PRONE;
                }
            }

            // C) JUMPING, SLIDE-JUMPING (EMP-Slide), & VAULTING ('Space')
            if (IsKeyPressed(KEY_SPACE)) {
                Vector3 vaultTarget;
                if (CheckVaultOpportunity(player.pos, fwd, map, vaultTarget)) {
                    player.isVaulting = true;
                    player.vaultTimer = 0.0f;
                    player.vaultStart = player.pos;
                    player.vaultTarget = vaultTarget;
                    player.posture = POSTURE_STAND;
                    player.gunRotSpring.Shove({ 14.0f, -8.0f, -10.0f });
                } else if (player.grounded) {
                    if (player.posture == POSTURE_SLIDE) {
                        player.vel.y = 8.2f;
                        player.vel.x *= 1.12f;
                        player.vel.z *= 1.12f;
                        player.posture = POSTURE_STAND;
                        player.grounded = false;
                    } else if (player.posture == POSTURE_CROUCH || player.posture == POSTURE_PRONE) {
                        player.posture = POSTURE_STAND;
                    } else {
                        player.vel.y = 7.8f;
                        player.grounded = false;
                        player.gunPosSpring.Shove({ 0.0f, -0.35f, 0.0f });
                    }
                }
            }

            if (player.diveRecoveryTimer > 0.0f) {
                player.diveRecoveryTimer -= dt;
            }
            if (player.posture == POSTURE_SLIDE) {
                player.slideTimer -= dt;
                float drag = 1.0f - 2.2f * dt;
                player.vel.x *= drag;
                player.vel.z *= drag;
                if (player.slideTimer <= 0.0f || shiftHeld) {
                    player.posture = shiftHeld ? POSTURE_STAND : POSTURE_CROUCH;
                }
            } else if (player.grounded) {
                if (player.posture == POSTURE_DIVE) {
                    player.posture = POSTURE_PRONE;
                }
                player.usedAirDive = false;

                float baseSpeed = 8.2f * activeStats.walkSpeedMult;
                if (player.sprinting) baseSpeed *= 1.48f;
                else if (player.posture == POSTURE_CROUCH) baseSpeed *= 0.58f;
                else if (player.posture == POSTURE_PRONE)  baseSpeed *= 0.32f;
                if (player.isAiming) baseSpeed *= 0.72f;

                Vector3 targetVel = Vector3Scale(wishDir, baseSpeed);
                player.vel.x = SmoothDampFloat(player.vel.x, targetVel.x, 14.0f, dt);
                player.vel.z = SmoothDampFloat(player.vel.z, targetVel.z, 14.0f, dt);
            } else {
                if (hasMoveInput) {
                    player.vel.x += wishDir.x * 12.0f * dt;
                    player.vel.z += wishDir.z * 12.0f * dt;
                }
            }

            player.vel.y -= 21.0f * dt;

            bool wasGrounded = player.grounded;
            float fallSpeed = player.vel.y;
            player.pos = Vector3Add(player.pos, Vector3Scale(player.vel, dt));

            float charHeight = (player.posture == POSTURE_PRONE || player.posture == POSTURE_DIVE) ? 0.65f
                             : (player.posture == POSTURE_CROUCH || player.posture == POSTURE_SLIDE) ? 1.15f
                             : 1.75f;
            ResolveCharacterCollision(player.pos, player.vel, 0.36f, charHeight, player.grounded, map);

            if (!wasGrounded && player.grounded && fallSpeed < -3.5f) {
                player.gunPosSpring.Shove({ 0.0f, std::max(-1.2f, fallSpeed * 0.07f), 0.0f });
                player.gunRotSpring.Shove({ std::min(18.0f, -fallSpeed * 0.9f), 0.0f, 0.0f });
            }
        }

        float targetEye = 1.62f;
        if (player.posture == POSTURE_CROUCH || player.posture == POSTURE_SLIDE) targetEye = 1.05f;
        else if (player.posture == POSTURE_PRONE || player.posture == POSTURE_DIVE) targetEye = 0.45f;
        player.eyeHeightCurrent = SmoothDampFloat(player.eyeHeightCurrent, targetEye, 14.0f, dt);

        // 5. Aim Down Sights (ADS) & FOV Interpolation
        player.isAiming = IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && !player.isReloading && player.meleeSwingTimer <= 0.0f;
        if (player.isAiming) player.inspectTimer = 0.0f;
        float targetAds = player.isAiming ? 1.0f : 0.0f;
        player.adsAlpha = SmoothDampFloat(player.adsAlpha, targetAds, activeStats.adsSpeed, dt);

        float targetFov = player.isAiming
            ? (player.useCantedSight ? 55.0f : activeStats.adsFov)
            : (player.sprinting ? player.baseHipFov + 8.0f : player.baseHipFov);
        player.currentFov = SmoothDampFloat(player.currentFov, targetFov, 14.0f, dt);

        player.holdingBreath = player.isAiming && IsKeyDown(KEY_LEFT_SHIFT) && player.steadyBreathStamina > 0.2f;
        if (player.holdingBreath) {
            player.steadyBreathStamina = std::max(0.0f, player.steadyBreathStamina - dt);
        } else {
            player.steadyBreathStamina = std::min(5.0f, player.steadyBreathStamina + dt * 0.7f);
        }

        // 6. Reloading ('R')
        int& curMag = player.magAmmo[player.activeSlot];
        int& curRes = player.reserveAmmo[player.activeSlot];
        if ((IsKeyPressed(KEY_R) || (curMag == 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) &&
            !player.isReloading && curMag < activeStats.magSize && curRes > 0) {
            player.isReloading = true;
            player.inspectTimer = 0.0f;
            player.reloadTimer = 0.0f;
            player.reloadDuration = (curMag > 0) ? activeStats.reloadTactical : activeStats.reloadEmpty;
            if (audio) audio->Play(audio->sfxReloadClick, 0.04f);
            player.gunRotSpring.Shove({ 14.0f, -12.0f, -15.0f });
        }

        if (player.isReloading) {
            player.reloadTimer += dt;
            if (player.reloadTimer >= player.reloadDuration) {
                int needed = activeStats.magSize - curMag;
                int take = std::min(needed, curRes);
                curMag += take;
                curRes -= take;
                player.isReloading = false;
                if (audio) audio->Play(audio->sfxBoltCycle, 0.03f);
                player.gunRotSpring.Shove({ -10.0f, 6.0f, 8.0f });
            }
        }

        // 7. Firing Weapons (Auto / Burst / Semi / Bolt-Action)
        if (player.fireCooldown > 0.0f) player.fireCooldown -= dt;
        if (player.boltAnimTimer > 0.0f) player.boltAnimTimer = std::max(0.0f, player.boltAnimTimer - dt * 3.8f);
        if (player.muzzleFlashTimer > 0.0f) player.muzzleFlashTimer = std::max(0.0f, player.muzzleFlashTimer - dt);
        if (player.radarPingTimer > 0.0f) player.radarPingTimer = std::max(0.0f, player.radarPingTimer - dt);

        FireMode fm = player.currentFireMode[player.activeSlot];
        bool triggerFire = false;
        if (!player.isReloading && player.meleeSwingTimer <= 0.0f && player.fireCooldown <= 0.0f) {
            if (fm == FIRE_AUTO) {
                triggerFire = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
            } else if (fm == FIRE_BURST) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && player.burstShotsQueued == 0) {
                    player.burstShotsQueued = 3;
                }
                if (player.burstShotsQueued > 0) {
                    triggerFire = true;
                }
            } else {
                triggerFire = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
            }
        }

        player.timeSinceLastShot += dt;
        if (player.timeSinceLastShot > 0.30f) {
            player.continuousShots = 0;
        }

        if (triggerFire && curMag > 0) {
            curMag--;
            if (player.burstShotsQueued > 0) player.burstShotsQueued--;
            player.continuousShots++;
            player.timeSinceLastShot = 0.0f;
            player.fireCooldown = 60.0f / activeStats.rpm;
            player.boltAnimTimer = 1.0f;
            player.muzzleFlashTimer = 0.045f;
            player.inspectTimer = 0.0f;
            player.sprinting = false;
            if (activeAtt.barrel != BARREL_SUPPRESSOR &&
                activeAtt.barrel != BARREL_ARS_SUPPRESSOR) {
                player.radarPingTimer = 2.6f;
            }

            bool isMoving = (Vector3LengthSqr({ player.vel.x, 0.0f, player.vel.z }) > 0.5f);
            bool isAirborne = !player.grounded;
            float spread = CalculateDynamicSpread(
                activeStats,
                player.adsAlpha,
                player.posture,
                isMoving,
                player.sprinting,
                isAirborne,
                player.continuousShots
            );

            Vector3 patternKick = GetWeaponRecoilPattern(
                activeStats.id,
                player.continuousShots - 1,
                activeAtt,
                activeStats
            );
            player.camRecoilSpring.Shove(patternKick);
            player.gunPosSpring.Shove({
                patternKick.y * 0.008f,
                activeStats.modelKick.y,
                activeStats.modelKick.z * (1.0f - 0.35f * player.adsAlpha)
            });
            player.gunRotSpring.Shove({
                patternKick.x * 4.2f * (1.0f - 0.30f * player.adsAlpha),
                patternKick.y * 3.2f,
                patternKick.z * 2.2f
            });

            Vector3 eye = player.GetEyePos();
            Vector3 aimDir = player.GetForward();
            SpawnWeaponFire(0, 0, eye, aimDir, activeStats, activeAtt, spread);
        }

        // 8. Quick Melee Knife ('F')
        if (player.meleeSwingTimer > 0.0f) player.meleeSwingTimer -= dt;
        if (IsKeyPressed(KEY_F) && player.meleeSwingTimer <= 0.0f) {
            player.meleeSwingTimer = 0.42f;
            player.isReloading = false;
            player.inspectTimer = 0.0f;
            player.gunPosSpring.Shove({ -0.4f, 0.2f, -1.6f });
            player.gunRotSpring.Shove({ -25.0f, 35.0f, -20.0f });
            if (audio) audio->Play(audio->sfxBoltCycle, 0.15f);

            Vector3 eye = player.GetEyePos();
            Vector3 fwdDir = player.GetForward();
            for (auto& b : bots) {
                if (!b.alive || b.team == 0) continue;
                Vector3 toBot = Vector3Subtract(b.GetEyePos(), eye);
                float dist = Vector3Length(toBot);
                if (dist <= 2.7f && Vector3DotProduct(Vector3Normalize(toBot), fwdDir) > 0.55f) {
                    b.health = 0.0f;
                    b.alive = false;
                    b.deaths++;
                    b.respawnTimer = 4.0f;
                    player.kills++;
                    teamScore[0]++;
                    player.hitmarkerTimer = 0.25f;
                    player.hitmarkerHeadshot = true;
                    player.hitmarkerKill = true;
                    if (audio) audio->Play(audio->sfxHeadshot, 0.0f);
                    AddXpEvent("[+100 Enemy Killed]", 100);
                    AddXpEvent("[+50 Knife Takedown]", 50);
                    PushKillFeed("YOU", 0, "KNIFE", b.name, 1, false, false);
                    break;
                }
            }
        }

        // 9. M67 Frag Grenade ('G')
        if (player.grenadeThrowCooldown > 0.0f) player.grenadeThrowCooldown -= dt;
        if (IsKeyPressed(KEY_G) && player.grenadesLeft > 0 && player.grenadeThrowCooldown <= 0.0f) {
            player.grenadesLeft--;
            player.grenadeThrowCooldown = 1.1f;
            FragGrenade g;
            g.active = true;
            g.ownerId = 0;
            g.team = 0;
            g.pos = Vector3Add(player.GetEyePos(), Vector3Scale(player.GetForward(), 0.5f));
            g.vel = Vector3Add(Vector3Scale(player.GetForward(), 19.5f), { 0.0f, 4.2f, 0.0f });
            g.fuseTimer = 3.0f;
            grenades.push_back(g);
            player.gunRotSpring.Shove({ 20.0f, -15.0f, 0.0f });
        }

        // 10. Active 3D Enemy Spotting ('E' key - strictly player-triggered, never passive!)
        if (player.spotCooldown > 0.0f) player.spotCooldown -= dt;
        if (IsKeyPressed(KEY_E) && player.spotCooldown <= 0.0f) {
            player.spotCooldown = 1.0f;
            // Active PF spotting hand/weapon gesture impulse
            player.gunPosSpring.Shove({ -0.18f, 0.22f, -0.25f });
            player.gunRotSpring.Shove({ 10.0f, -14.0f, 8.0f });

            Vector3 eye = player.GetEyePos();
            Vector3 look = player.GetForward();
            int spottedCount = 0;
            for (auto& b : bots) {
                if (!b.alive || b.team == 0) continue;
                Vector3 toBot = Vector3Subtract(b.GetEyePos(), eye);
                float dist = Vector3Length(toBot);
                if (dist < 110.0f && Vector3DotProduct(Vector3Normalize(toBot), look) > 0.92f) {
                    if (HasLineOfSight(eye, b.GetEyePos())) {
                        if (b.spottedTimer <= 0.0f) {
                            AddXpEvent("[+25 Spotted Enemy]", 25);
                        }
                        b.spottedTimer = 8.0f;
                        spottedCount++;
                        // Command allied Phantom bots to engage the enemy you actively spotted!
                        for (auto& ally : bots) {
                            if (ally.alive && ally.team == 0 && Vector3Distance(ally.pos, b.pos) < 55.0f) {
                                ally.AlertToThreat(b.GetEyePos(), b.vel, 6.0f);
                            }
                        }
                    }
                }
            }
            if (spottedCount > 0 && audio) {
                audio->Play(audio->sfxReloadClick, 0.1f);
            }
        }

        // 11. Health Auto-Regeneration
        player.timeSinceDamage += dt;
        if (player.timeSinceDamage >= 5.5f && player.health < 100.0f) {
            player.health = std::min(100.0f, player.health + 18.0f * dt);
        }
        if (player.hitmarkerTimer > 0.0f) player.hitmarkerTimer -= dt;
        if (player.damageIndicatorTimer > 0.0f) player.damageIndicatorTimer -= dt;

        // 12. Update Procedural Springs
        float horizSpeed = Vector2Length({ player.vel.x, player.vel.z });
        if (player.grounded && horizSpeed > 0.5f && player.posture != POSTURE_SLIDE) {
            player.walkCycleTime += dt * horizSpeed * 1.15f;
        }

        float recMult = (activeAtt.other == OTHER_GREEN_LASER) ? 1.18f : 1.0f;
        player.camRecoilSpring.Update(dt * recMult);
        player.gunPosSpring.Update(dt * recMult);
        player.gunRotSpring.Update(dt * recMult);
        player.swaySpring.Update(dt);

        player.pitch += player.camRecoilSpring.position.x * dt * 0.65f;
        player.yaw   += player.camRecoilSpring.position.y * dt * 0.45f;
        player.pitch = std::clamp(player.pitch, -1.48f, 1.48f);
    }

    // Update Projectiles, Shell Casings, Wallbanging & Hitbox Damage
    void UpdateProjectilesAndGrenades(float dt) {
        // 0. Update 3D Spinning Brass Shell Casings
        for (auto& s : shells) {
            if (!s.active) continue;
            s.timer -= dt;
            if (s.timer <= 0.0f) {
                s.active = false;
                continue;
            }
            s.vel.y -= 16.0f * dt;
            s.pos = Vector3Add(s.pos, Vector3Scale(s.vel, dt));
            s.rotDeg = Vector3Add(s.rotDeg, Vector3Scale(s.rotVel, dt));
            if (s.pos.y <= 0.02f) {
                s.pos.y = 0.02f;
                s.vel.y = -s.vel.y * 0.35f;
                s.vel.x *= 0.65f;
                s.vel.z *= 0.65f;
                s.rotVel = Vector3Scale(s.rotVel, 0.5f);
            }
        }
        shells.erase(
            std::remove_if(shells.begin(), shells.end(), [](const ShellCasing& s) { return !s.active; }),
            shells.end()
        );

        // 1. Hitscan Visual Tracer Simulation (Instant damage already applied on frame 0)
        for (auto& p : projectiles) {
            if (!p.active) continue;
            p.lifeTime -= dt;
            if (p.lifeTime <= 0.0f) {
                p.active = false;
                continue;
            }

            p.prevPos = p.pos;
            p.distanceTraveled += 650.0f * dt;
            if (p.distanceTraveled >= p.maxDistance) {
                p.pos = p.hitPos;
                p.active = false;
            } else {
                p.pos = Vector3Add(p.startPos, Vector3Scale(Vector3Normalize(p.vel), p.distanceTraveled));
            }
        }

        projectiles.erase(
            std::remove_if(projectiles.begin(), projectiles.end(), [](const Projectile& p) { return !p.active; }),
            projectiles.end()
        );

        // 2. Update Decals
        for (auto& d : decals) d.timer -= dt;
        decals.erase(
            std::remove_if(decals.begin(), decals.end(), [](const ImpactDecal& d) { return d.timer <= 0.0f; }),
            decals.end()
        );

        // 3. Update M67 Frag Grenades
        for (auto& g : grenades) {
            if (!g.active) continue;
            if (g.explosionFlashTimer > 0.0f) {
                g.explosionFlashTimer -= dt;
                if (g.explosionFlashTimer <= 0.0f) g.active = false;
                continue;
            }

            g.fuseTimer -= dt;
            g.vel.y -= 18.0f * dt;
            Vector3 nextPos = Vector3Add(g.pos, Vector3Scale(g.vel, dt));

            if (nextPos.y <= 0.15f) {
                nextPos.y = 0.15f;
                g.vel.y = -g.vel.y * 0.45f;
                g.vel.x *= 0.75f;
                g.vel.z *= 0.75f;
            }
            for (const auto& blk : map.blocks) {
                if (!blk.collidable) continue;
                BoundingBox box = blk.GetBounds();
                if (CheckCollisionBoxSphere(box, nextPos, 0.16f)) {
                    g.vel = Vector3Scale(g.vel, -0.45f);
                    nextPos = g.pos;
                    break;
                }
            }
            g.pos = nextPos;

            if (g.fuseTimer <= 0.0f) {
                g.explosionFlashTimer = 0.35f;
                if (audio) audio->Play(audio->sfxExplosion, 0.05f);

                const float blastRadius = 10.5f;
                for (auto& b : bots) {
                    if (!b.alive || b.team == g.team) continue;
                    float dist = Vector3Distance(g.pos, b.pos);
                    if (dist < blastRadius) {
                        float dmg = 135.0f * (1.0f - dist / blastRadius);
                        b.health -= dmg;
                        if (b.health <= 0.0f) {
                            b.alive = false;
                            b.deaths++;
                            b.respawnTimer = 4.0f;
                            player.kills++;
                            teamScore[0]++;
                            AddXpEvent("[+100 Enemy Killed]", 100);
                            AddXpEvent("[+35 Frag Bonus]", 35);
                            PushKillFeed("YOU", 0, "M67 FRAG", b.name, b.team, false, false);
                        }
                    }
                }
            }
        }
        grenades.erase(
            std::remove_if(grenades.begin(), grenades.end(), [](const FragGrenade& g) { return !g.active; }),
            grenades.end()
        );
    }

    // Update Phantoms (Allies) & Ghosts (Enemies) AI Bots:
    // - Multi-Sensory Awareness (FOV Cone, Footsteps, Gunfire Radar, Damage Reflex, Team Spotting, Target Memory)
    // - A* Navigation Graph Routing across Multi-Level Maps (Escalators, Doorways, Rooftop Stairs)
    // - 3-Ray Whisker Obstacle Avoidance, Auto-Vaulting over Windows/Sandbags, Grenade Evasion & Separation
    void UpdateBots(float dt) {
        int numWaypoints = (int)map.waypoints.size();
        for (size_t botIdx = 0; botIdx < bots.size(); ++botIdx) {
            auto& b = bots[botIdx];
            if (!b.alive) {
                b.respawnTimer -= dt;
                if (b.respawnTimer <= 0.0f) {
                    b.alive = true;
                    b.health = 100.0f;
                    const auto& spawns = (b.team == 0) ? map.phantomSpawns : map.ghostSpawns;
                    b.pos = spawns[std::rand() % spawns.size()];
                    b.pos.x += ((std::rand() % 60) - 30) * 0.08f;
                    b.pos.z += ((std::rand() % 60) - 30) * 0.08f;
                    b.vel = { 0, 0, 0 };
                    b.crouching = false;
                    b.hasTargetMemory = false;
                    b.memoryTimer = 0.0f;
                    b.losLostTimer = 99.0f;
                    b.combatAlertTimer = 0.0f;
                    b.targetLockTimer = 0.0f;
                    b.lastProgressPos = b.pos;
                    b.stuckTimer = 0.0f;
                    b.targetWaypoint = map.FindClosestWaypoint(b.pos);
                    b.goalWaypoint = std::rand() % std::max(1, numWaypoints);
                }
                continue;
            }

            if (b.spottedTimer > 0.0f) b.spottedTimer -= dt;
            if (b.radarPingTimer > 0.0f) b.radarPingTimer -= dt;
            if (b.muzzleFlashTimer > 0.0f) b.muzzleFlashTimer -= dt;
            if (b.fireCooldown > 0.0f) b.fireCooldown -= dt;
            if (b.vaultCooldown > 0.0f) b.vaultCooldown -= dt;
            if (b.combatAlertTimer > 0.0f) b.combatAlertTimer -= dt;
            b.scanPhase += dt * 2.2f;

            b.timeSinceLastShot += dt;
            if (b.timeSinceLastShot > 0.35f) {
                b.continuousShots = 0;
            }

            if (b.memoryTimer > 0.0f) {
                b.memoryTimer -= dt;
                if (b.memoryTimer <= 0.0f) {
                    b.hasTargetMemory = false;
                }
            }

            WeaponStats ws = GetModifiedWeaponStats(b.weaponId, b.attachments);
            Vector3 eye = b.GetEyePos();
            Vector3 botFwd = b.GetFlatForward();

            // ================================================================
            // 1. MULTI-SENSORY THREAT & SITUATION AWARENESS
            // ================================================================
            bool hasTarget = false;
            Vector3 targetEye = { 0, 0, 0 };
            Vector3 targetVel = { 0, 0, 0 };
            float bestTargetScore = 1e9f;
            float bestTargetDist = 88.0f;

            auto evaluateCandidate = [&](Vector3 candEye, Vector3 candVel, bool candSpotted, bool candFiring) {
                Vector3 toCand = Vector3Subtract(candEye, eye);
                float dist = Vector3Length(toCand);
                if (dist > 88.0f || dist < 0.1f) return;

                Vector3 flatDir = Vector3Normalize({ toCand.x, 0.0f, toCand.z });
                float fovDot = Vector3DotProduct(botFwd, flatDir);

                // Vision Cone + Peripheral/Close Footsteps + Alerted 360 Vision
                bool withinVisionCone = (fovDot > -0.22f) || (dist < 14.0f) || (b.combatAlertTimer > 0.0f) || candSpotted;
                if (withinVisionCone && HasLineOfSight(eye, candEye)) {
                    // Score by distance, favoring targets near current crosshair or spotted threats
                    float score = dist - fovDot * 10.0f - (candSpotted ? 12.0f : 0.0f);
                    if (score < bestTargetScore) {
                        bestTargetScore = score;
                        bestTargetDist = dist;
                        targetEye = candEye;
                        targetVel = candVel;
                        hasTarget = true;
                    }
                } else {
                    // Non-LOS Audio / Gunfire Radar / Footstep / Team 3D Spot Awareness!
                    bool heardGunfire   = candFiring && (dist < 58.0f);
                    bool heardFootsteps = (dist < 13.5f) && (Vector2Length({ candVel.x, candVel.z }) > 3.5f);
                    bool teamSpotted    = candSpotted && (dist < 75.0f);
                    if (heardGunfire || heardFootsteps || teamSpotted) {
                        if (!b.hasTargetMemory || dist < Vector3Distance(eye, b.lastKnownTargetPos) + 6.0f) {
                            b.AlertToThreat(candEye, candVel, teamSpotted ? 5.0f : 4.2f);
                        }
                    }
                }
            };

            // Enemy bots check the Player (Team 0)
            if (b.team == 1 && player.alive && !inDeployMenu) {
                evaluateCandidate(player.GetEyePos(), player.vel, false, (player.radarPingTimer > 0.0f));
            }

            // All bots check opposing team bots
            for (size_t j = 0; j < bots.size(); ++j) {
                const auto& other = bots[j];
                if (!other.alive || other.team == b.team) continue;
                evaluateCandidate(other.GetEyePos(), other.vel, (other.spottedTimer > 0.0f), (other.radarPingTimer > 0.0f));
            }

            // Update Target Memory & Internal Squad Callouts (never sets HUD spottedTimer passively!)
            bool suppressiveWallbang = false;
            if (hasTarget) {
                b.AlertToThreat(targetEye, targetVel, 5.2f);
                b.losLostTimer = 0.0f;
                b.targetLockTimer += dt;

                // Internal AI radio callout to nearby squadmates (does NOT trigger passive HUD 3D spotting!)
                if (b.targetLockTimer >= 0.45f) {
                    for (auto& squadmate : bots) {
                        if (squadmate.alive && squadmate.team == b.team && squadmate.id != b.id) {
                            if (Vector3Distance(squadmate.pos, b.pos) < 16.0f) {
                                squadmate.AlertToThreat(targetEye, targetVel, 3.8f);
                            }
                        }
                    }
                }
            } else {
                b.targetLockTimer = 0.0f;
                b.losLostTimer += dt;
                if (b.hasTargetMemory && b.losLostTimer < 0.65f) {
                    // Extrapolate enemy movement slightly behind cover
                    b.lastKnownTargetPos = Vector3Add(b.lastKnownTargetPos, Vector3Scale(b.lastKnownTargetVel, dt * 0.6f));
                }
                // Check if we can wallbang-suppress an enemy who just ducked behind thin wood/sheet-metal cover!
                if (b.hasTargetMemory && b.losLostTimer < 1.35f) {
                    float memDist = Vector3Distance(eye, b.lastKnownTargetPos);
                    if (memDist < 40.0f && CanBotWallbangTarget(eye, b.lastKnownTargetPos, ws.penetration)) {
                        suppressiveWallbang = true;
                        bestTargetDist = memDist;
                    }
                }
            }

            // ================================================================
            // 2. STRATEGIC GOAL SELECTION & A* GRAPH PATHFINDING
            // ================================================================
            bool hasStrategicPos = false;
            Vector3 strategicPos = { 0, 0, 0 };

            if (currentMode == MODE_KING_OF_THE_HILL) {
                // Route to the Hill if uncaptured, contested, or if close/medium-range class
                if (map.hillPoint.ownerTeam != b.team || b.weaponId != WPN_INTERVENTION) {
                    strategicPos = map.hillPoint.pos;
                    hasStrategicPos = true;
                }
            } else if (currentMode == MODE_FLARE_DOMINATION && !map.flarePoints.empty()) {
                // Dynamically score all 3 Flares (A, B, C) by ownership, contest state, and distance
                int bestFlare = -1;
                float bestFlareScore = 1e9f;
                for (int f = 0; f < (int)map.flarePoints.size(); ++f) {
                    const auto& fp = map.flarePoints[f];
                    if (fp.ownerTeam == b.team && std::fabs(fp.captureProgress) > 0.90f) continue; // Already secure
                    float d = Vector3Distance(b.pos, fp.pos);
                    // Spread squadmates naturally across flags using bot ID bias
                    float squadSpreadBias = ((b.id + f) % 3 == 0) ? -10.0f : 0.0f;
                    float urgencyBias = (fp.ownerTeam == -1) ? -8.0f : -4.0f;
                    float score = d + squadSpreadBias + urgencyBias;
                    if (score < bestFlareScore) {
                        bestFlareScore = score;
                        bestFlare = f;
                    }
                }
                if (bestFlare >= 0) {
                    strategicPos = map.flarePoints[bestFlare].pos;
                    hasStrategicPos = true;
                }
            }

            // If no objective overrides, or if hunting a recent threat in TDM / nearby combat, pursue Last Known Position
            if (b.hasTargetMemory && b.weaponId != WPN_INTERVENTION) {
                float distToThreat = Vector3Distance(b.pos, b.lastKnownTargetPos);
                if (!hasStrategicPos || distToThreat < 26.0f) {
                    strategicPos = b.lastKnownTargetPos;
                    hasStrategicPos = true;
                }
            }

            if (numWaypoints > 0) {
                if (hasStrategicPos) {
                    b.goalWaypoint = map.FindClosestWaypoint(strategicPos);
                }

                b.targetWaypoint = std::clamp(b.targetWaypoint, 0, numWaypoints - 1);
                b.goalWaypoint   = std::clamp(b.goalWaypoint,   0, numWaypoints - 1);

                Vector3 curWp = map.waypoints[b.targetWaypoint];
                float horizToWp = Vector2Distance({ b.pos.x, b.pos.z }, { curWp.x, curWp.z });
                float vertToWp  = std::fabs(b.pos.y - curWp.y);

                // Re-anchor to closest waypoint if current targetWaypoint is on a different floor or too far
                if (horizToWp > 34.0f || (vertToWp > 3.6f && horizToWp > 18.0f)) {
                    b.targetWaypoint = map.FindClosestWaypoint(b.pos);
                    curWp = map.waypoints[b.targetWaypoint];
                    horizToWp = Vector2Distance({ b.pos.x, b.pos.z }, { curWp.x, curWp.z });
                    vertToWp  = std::fabs(b.pos.y - curWp.y);
                }

                // Reached current step waypoint -> advance along A* shortest path toward goalWaypoint
                if (horizToWp < 2.6f && vertToWp < 2.0f) {
                    if (b.targetWaypoint == b.goalWaypoint) {
                        // Pick a new tactical patrol/vantage waypoint
                        if (b.weaponId == WPN_INTERVENTION) {
                            // Snipers prefer elevated rooftop/overwatch waypoints
                            int bestHighWp = std::rand() % numWaypoints;
                            for (int tryIdx = 0; tryIdx < 6; ++tryIdx) {
                                int cand = std::rand() % numWaypoints;
                                if (map.waypoints[cand].y > map.waypoints[bestHighWp].y) {
                                    bestHighWp = cand;
                                }
                            }
                            b.goalWaypoint = bestHighWp;
                        } else {
                            b.goalWaypoint = (b.goalWaypoint + 3 + (std::rand() % std::max(1, numWaypoints - 1))) % numWaypoints;
                        }
                    }
                    b.targetWaypoint = map.FindNextWaypointOnPath(b.targetWaypoint, b.goalWaypoint);
                }
            }

            // Determine immediate navigation target point
            Vector3 navTarget = (numWaypoints > 0) ? map.waypoints[b.targetWaypoint] : b.pos;
            if (hasStrategicPos && Vector3Distance(b.pos, strategicPos) < 14.0f &&
                std::fabs(b.pos.y - strategicPos.y) < 1.2f &&
                map.CanTraverseSegment(b.pos, strategicPos)) {
                navTarget = strategicPos;
            }

            // Stuck detection & automatic unstick recovery
            b.stuckTimer += dt;
            if (b.stuckTimer >= 0.55f) {
                float moved = Vector2Distance({ b.pos.x, b.pos.z }, { b.lastProgressPos.x, b.lastProgressPos.z });
                b.lastProgressPos = b.pos;
                b.stuckTimer = 0.0f;
                if (moved < 0.32f && (!hasTarget || b.weaponId != WPN_INTERVENTION)) {
                    b.strafeDir = -b.strafeDir;
                    int closestWp = map.FindClosestWaypoint(b.pos);
                    if (!map.navNeighbors[closestWp].empty()) {
                        b.targetWaypoint = map.navNeighbors[closestWp][std::rand() % map.navNeighbors[closestWp].size()];
                    }
                    if (b.grounded) {
                        b.vel.y = 6.2f; // Hop over small snag
                        b.grounded = false;
                    }
                }
            }

            Vector3 toNav = { navTarget.x - b.pos.x, 0.0f, navTarget.z - b.pos.z };
            Vector3 moveDir = (Vector3LengthSqr(toNav) > 0.04f) ? Vector3Normalize(toNav) : Vector3{ 0, 0, 0 };

            // ================================================================
            // 3. COMBAT AIMING, VELOCITY LEAD & WEAPON-ROLE TACTICAL MOVEMENT
            // ================================================================
            if (hasTarget || suppressiveWallbang) {
                Vector3 rawAimTarget = hasTarget ? targetEye : b.lastKnownTargetPos;
                Vector3 rawAimVel    = hasTarget ? targetVel : Vector3{ 0, 0, 0 };

                float flightTime = bestTargetDist / std::max(50.0f, ws.muzzleVelocity);
                Vector3 predictedTarget = Vector3Add(rawAimTarget, Vector3Scale(rawAimVel, flightTime * 0.85f));

                Vector3 toTarget = Vector3Subtract(predictedTarget, eye);
                float horizDist = std::sqrt(toTarget.x * toTarget.x + toTarget.z * toTarget.z);
                float desiredYaw = std::atan2(-toTarget.x, -toTarget.z);
                float desiredPitch = std::atan2(toTarget.y, std::max(0.1f, horizDist));
                desiredPitch += 0.5f * 9.81f * flightTime / std::max(50.0f, ws.muzzleVelocity);

                // Smooth angular tracking
                b.yaw = SmoothTurnAngle(b.yaw, desiredYaw, 11.0f, dt);
                b.pitch = SmoothDampFloat(b.pitch, std::clamp(desiredPitch, -0.95f, 0.95f), 14.0f, dt);

                b.strafeTimer -= dt;
                if (b.strafeTimer <= 0.0f) {
                    b.strafeTimer = 0.65f + (std::rand() % 9) * 0.1f;
                    b.strafeDir = ((std::rand() % 2) == 0) ? 1.0f : -1.0f;
                    if (b.weaponId == WPN_INTERVENTION && bestTargetDist > 22.0f) {
                        b.crouching = true; // Snipers crouch for steady long-range shots
                    } else if (b.health < 42.0f) {
                        b.crouching = true; // Low-health evasive crouch
                    } else {
                        b.crouching = ((std::rand() % 3) == 0);
                    }
                }

                Vector3 right = { std::cos(b.yaw), 0.0f, -std::sin(b.yaw) };
                Vector3 flatToEnemy = Vector3Normalize({ toTarget.x, 0.0f, toTarget.z });

                // Weapon-specific combat movement
                if (b.weaponId == WPN_REMINGTON870 || b.weaponId == WPN_MP7) {
                    // CQC Aggressive Push + Strafe Weave
                    float pushWeight = (bestTargetDist > 9.0f) ? 0.72f : 0.15f;
                    moveDir = Vector3Normalize(Vector3Add(
                        Vector3Scale(flatToEnemy, pushWeight),
                        Vector3Scale(right, b.strafeDir * 0.75f)
                    ));
                } else if (b.weaponId == WPN_INTERVENTION) {
                    // Sniper spacing: backpedal if enemy rushes close, hold steady at long range
                    float backWeight = (bestTargetDist < 18.0f) ? -0.65f : 0.0f;
                    moveDir = Vector3Add(
                        Vector3Scale(flatToEnemy, backWeight),
                        Vector3Scale(right, b.strafeDir * 0.45f)
                    );
                } else {
                    // Assault Rifle / LMG tactical strafe + advance/retreat based on health & range
                    float rangeBias = (b.health < 38.0f) ? -0.45f : ((bestTargetDist > 28.0f) ? 0.35f : 0.0f);
                    moveDir = Vector3Normalize(Vector3Add(
                        Vector3Scale(flatToEnemy, rangeBias),
                        Vector3Scale(right, b.strafeDir * 0.80f)
                    ));
                }

                b.reactionTimer -= dt;
                float aimError = std::fabs(WrapAngleRad(desiredYaw - b.yaw));
                if (b.reactionTimer <= 0.0f && b.fireCooldown <= 0.0f && aimError < 0.26f) {
                    b.fireCooldown = (60.0f / ws.rpm) * (suppressiveWallbang ? 1.65f : 1.28f);
                    b.muzzleFlashTimer = 0.05f;
                    if (b.attachments.barrel != BARREL_SUPPRESSOR &&
                        b.attachments.barrel != BARREL_ARS_SUPPRESSOR) {
                        b.radarPingTimer = 2.5f;
                    }
                    b.continuousShots++;
                    b.timeSinceLastShot = 0.0f;
                    Vector3 aimDir = Vector3Normalize(toTarget);
                    bool isMoving = (Vector3LengthSqr({ b.vel.x, 0.0f, b.vel.z }) > 0.4f);
                    float botSpread = CalculateDynamicSpread(
                        ws,
                        1.0f, // Bots aim down sights when tracking target
                        b.crouching ? 1 : 0,
                        isMoving,
                        false,
                        !b.grounded,
                        b.continuousShots
                    );
                    SpawnWeaponFire(b.id, b.team, eye, aimDir, ws, b.attachments, botSpread);
                }
            } else {
                // Keep reaction timer warm when already combat-alerted or tracking target memory!
                b.reactionTimer = (b.hasTargetMemory || b.combatAlertTimer > 0.0f) ? 0.11f : 0.24f;
                b.crouching = false;

                if (b.hasTargetMemory && Vector3Distance(b.pos, b.lastKnownTargetPos) < 36.0f) {
                    // Pre-aim toward Last Known Threat Position + corner-checking scan
                    Vector3 toMem = Vector3Subtract(b.lastKnownTargetPos, eye);
                    float memHoriz = std::sqrt(toMem.x * toMem.x + toMem.z * toMem.z);
                    float desiredYaw = std::atan2(-toMem.x, -toMem.z) + std::sin(b.scanPhase) * 0.16f;
                    float desiredPitch = std::atan2(toMem.y, std::max(0.1f, memHoriz));
                    b.yaw = SmoothTurnAngle(b.yaw, desiredYaw, 8.5f, dt);
                    b.pitch = SmoothDampFloat(b.pitch, std::clamp(desiredPitch, -0.6f, 0.6f), 10.0f, dt);
                } else if (Vector3LengthSqr(moveDir) > 0.01f) {
                    // Smoothly look in direction of patrol with subtle situational awareness scan
                    float moveYaw = std::atan2(-moveDir.x, -moveDir.z) + std::sin(b.scanPhase * 0.7f) * 0.12f;
                    float desiredPitch = std::clamp((navTarget.y - b.pos.y) * 0.08f, -0.35f, 0.35f);
                    b.yaw = SmoothTurnAngle(b.yaw, moveYaw, 7.5f, dt);
                    b.pitch = SmoothDampFloat(b.pitch, desiredPitch, 8.0f, dt);
                }
            }

            // ================================================================
            // 4. LOCAL OBSTACLE AVOIDANCE WHISKERS, AUTO-VAULTING & SEPARATION
            // ================================================================
            // A) Live Frag Grenade Evasion (Highest Priority Hazard Override!)
            for (const auto& g : grenades) {
                if (!g.active || g.explosionFlashTimer > 0.0f) continue;
                Vector3 awayFromGrenade = { b.pos.x - g.pos.x, 0.0f, b.pos.z - g.pos.z };
                float gDist = Vector3Length(awayFromGrenade);
                if (gDist < 11.5f && gDist > 0.05f) {
                    moveDir = Vector3Normalize(awayFromGrenade);
                    b.crouching = false;
                    break;
                }
            }

            // B) 3-Ray Whisker Obstacle Avoidance & Auto-Vaulting over Low Cover/Windows
            if (Vector3LengthSqr(moveDir) > 0.01f) {
                Vector3 vaultTarget = { 0, 0, 0 };
                if (b.grounded && b.vaultCooldown <= 0.0f && CheckVaultOpportunity(b.pos, moveDir, map, vaultTarget)) {
                    // Vault/hop cleanly over waist-high window sills, sandbags & crates
                    b.vel.y = 6.9f;
                    b.pos.y += 0.12f;
                    b.grounded = false;
                    b.vaultCooldown = 0.75f;
                } else {
                    const float feelerDist = 2.4f;
                    float angleL =  38.0f * DEG2RAD;
                    float angleR = -38.0f * DEG2RAD;
                    Vector3 dirL = {
                        moveDir.x * std::cos(angleL) - moveDir.z * std::sin(angleL),
                        0.0f,
                        moveDir.x * std::sin(angleL) + moveDir.z * std::cos(angleL)
                    };
                    Vector3 dirR = {
                        moveDir.x * std::cos(angleR) - moveDir.z * std::sin(angleR),
                        0.0f,
                        moveDir.x * std::sin(angleR) + moveDir.z * std::cos(angleR)
                    };

                    float clearC = CastSteeringRay(b.pos, moveDir, feelerDist);
                    float clearL = CastSteeringRay(b.pos, dirL,    feelerDist);
                    float clearR = CastSteeringRay(b.pos, dirR,    feelerDist);

                    if (clearC < feelerDist * 0.85f || clearL < feelerDist * 0.65f || clearR < feelerDist * 0.65f) {
                        if (clearL > clearR + 0.15f) {
                            moveDir = Vector3Normalize(Vector3Lerp(moveDir, dirL, 0.72f));
                        } else if (clearR > clearL + 0.15f) {
                            moveDir = Vector3Normalize(Vector3Lerp(moveDir, dirR, 0.72f));
                        } else if (clearC < 1.0f) {
                            moveDir = (b.strafeDir > 0.0f) ? dirL : dirR;
                        }
                    }
                }
            }

            // C) Bot-to-Bot Flocking Separation (prevents bots from stacking or jamming doorways)
            Vector3 sepPush = { 0, 0, 0 };
            for (size_t j = 0; j < bots.size(); ++j) {
                if (j == botIdx || !bots[j].alive) continue;
                if (std::fabs(b.pos.y - bots[j].pos.y) > 1.5f) continue;
                Vector3 diff = { b.pos.x - bots[j].pos.x, 0.0f, b.pos.z - bots[j].pos.z };
                float d = Vector3Length(diff);
                if (d < 1.15f && d > 0.01f) {
                    sepPush = Vector3Add(sepPush, Vector3Scale(diff, (1.15f - d) / d));
                }
            }
            if (Vector3LengthSqr(sepPush) > 0.001f) {
                moveDir = Vector3Normalize(Vector3Add(moveDir, Vector3Scale(sepPush, 0.55f)));
            }

            float botSpeed = b.crouching ? 4.2f : 6.8f;
            b.vel.x = moveDir.x * botSpeed;
            b.vel.z = moveDir.z * botSpeed;
            b.vel.y -= 21.0f * dt;
            b.pos = Vector3Add(b.pos, Vector3Scale(b.vel, dt));
            ResolveCharacterCollision(b.pos, b.vel, 0.36f, 1.70f, b.grounded, map);

            if (Vector2Length({ b.vel.x, b.vel.z }) > 0.5f) {
                b.walkPhase += dt * 9.0f;
            }
        }
    }

    // Update Flare Domination & King of the Hill Objectives
    void UpdateObjectives(float dt) {
        matchTimer = std::max(0.0f, matchTimer - dt);

        auto updatePoint = [&](ObjectivePoint& pt) {
            int countTeam[2] = { 0, 0 };
            if (player.alive && !inDeployMenu && Vector3Distance(player.pos, pt.pos) <= pt.radius) {
                countTeam[0]++;
            }
            for (const auto& b : bots) {
                if (b.alive && Vector3Distance(b.pos, pt.pos) <= pt.radius) {
                    countTeam[b.team]++;
                }
            }

            if (countTeam[0] > countTeam[1]) {
                int prevOwner = pt.ownerTeam;
                pt.captureProgress = std::max(-1.0f, pt.captureProgress - 0.35f * dt * (countTeam[0] - countTeam[1]));
                if (pt.captureProgress <= -0.99f && prevOwner != 0) {
                    pt.ownerTeam = 0;
                    if (audio) audio->Play(audio->sfxObjective, 0.0f);
                    if (player.alive && Vector3Distance(player.pos, pt.pos) <= pt.radius + 1.0f) {
                        AddXpEvent(std::string("[+150 Captured ") + pt.label + "]", 150);
                    }
                }
            } else if (countTeam[1] > countTeam[0]) {
                pt.captureProgress = std::min(1.0f, pt.captureProgress + 0.35f * dt * (countTeam[1] - countTeam[0]));
                if (pt.captureProgress >= 0.99f) {
                    pt.ownerTeam = 1;
                }
            }
        };

        if (currentMode == MODE_FLARE_DOMINATION) {
            for (auto& fp : map.flarePoints) updatePoint(fp);
        } else if (currentMode == MODE_KING_OF_THE_HILL) {
            updatePoint(map.hillPoint);
        }

        objectiveTickTimer += dt;
        if (objectiveTickTimer >= 2.5f) {
            objectiveTickTimer = 0.0f;
            if (currentMode == MODE_FLARE_DOMINATION) {
                for (const auto& fp : map.flarePoints) {
                    if (fp.ownerTeam >= 0) teamScore[fp.ownerTeam]++;
                }
            } else if (currentMode == MODE_KING_OF_THE_HILL) {
                if (map.hillPoint.ownerTeam >= 0) teamScore[map.hillPoint.ownerTeam] += 2;
            }
        }

        for (auto& kf : killFeed) kf.timer -= dt;
        killFeed.erase(
            std::remove_if(killFeed.begin(), killFeed.end(), [](const KillFeedEntry& k) { return k.timer <= 0.0f; }),
            killFeed.end()
        );
        for (auto& xp : xpPopups) xp.timer -= dt;
        xpPopups.erase(
            std::remove_if(xpPopups.begin(), xpPopups.end(), [](const XpPopup& x) { return x.timer <= 0.0f; }),
            xpPopups.end()
        );
    }

    void Update(float dt) {
        if (!inDeployMenu) {
            UpdatePlayer(dt);
        }
        UpdateBots(dt);
        UpdateProjectilesAndGrenades(dt);
        UpdateObjectives(dt);
    }
};
