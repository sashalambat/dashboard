#include "Game/World.h"

namespace sbs {
namespace {

float difficultyLead(const DifficultyDef& d) {
    return 0.15f + d.accuracy * 0.35f;
}

Vec2 flankPoint(World& world, const PlayerState& bot, const PlayerState& target) {
    const Vec2 to = (target.pos - bot.pos);
    const Vec2 side{-to.y, to.x};
    const Vec2 cand = target.pos + side.normalized() * world.rng().range(4.0f, 8.0f);
    return world.map().clampWorld(cand);
}

} // namespace

void tickBot(World& world, PlayerState& bot, float dt) {
    if (!bot.active || !bot.isBot) return;
    if (bot.spectating) return;

    const DifficultyDef& diff = difficultyDef(bot.difficulty);
    bot.botThink -= dt;
    PlayerInput in = bot.input;

    if (!bot.alive) {
        in.fire = false;
        in.moveX = 0;
        in.moveY = 0;
        bot.input = in;
        return;
    }

    if (bot.botThink <= 0.0f) {
        bot.botThink = diff.reaction + world.rng().range(0.0f, 0.12f);
        PlayerState* enemy = world.closestEnemy(bot, 48.0f, false);
        bot.botTarget = enemy ? enemy->id : 255;

        Vec2 goal = bot.pos;
        const GameModeId mode = world.mode();
        bool objective = false;
        if (mode == GameModeId::CaptureTheFlag) {
            objective = true;
            if (bot.team == TeamId::Alliance) {
                goal = world.flagB().carrier == bot.id ? world.flagA().home : world.flagB().pos;
            } else {
                goal = world.flagA().carrier == bot.id ? world.flagB().home : world.flagA().pos;
            }
        } else if (mode == GameModeId::Domination) {
            objective = true;
            float best = 1e9f;
            for (const auto& d : world.dom()) {
                if (d.owner == bot.team) continue;
                const float dist = distanceSq(bot.pos, d.pos);
                if (dist < best) {
                    best = dist;
                    goal = d.pos;
                }
            }
        } else if (mode == GameModeId::KingOfTheHill) {
            objective = true;
            goal = world.hill();
        }

        if (enemy && world.rng().nextFloat() < diff.aggression) {
            if (world.rng().nextFloat() < diff.flankChance) {
                goal = flankPoint(world, bot, *enemy);
            } else {
                goal = enemy->pos;
            }
        } else if (!objective) {
            goal = {world.rng().range(3.0f, static_cast<float>(world.map().width()) - 3.0f),
                    world.rng().range(3.0f, static_cast<float>(world.map().height()) - 3.0f)};
        }

        world.map().pathfind(bot.pos, goal, bot.path);
        bot.pathIndex = 1;
        if (bot.health < classDef(bot.cls).maxHealth * 0.28f && world.rng().nextFloat() > diff.aggression) {
            bot.path.clear();
            in.moveX = -dirFromYaw(bot.yaw).x;
            in.moveY = -dirFromYaw(bot.yaw).y;
        }
    }

    if (bot.pathIndex < static_cast<int>(bot.path.size())) {
        const Vec2 wp = bot.path[static_cast<size_t>(bot.pathIndex)];
        const Vec2 d = (wp - bot.pos);
        if (d.length() < 0.45f) {
            bot.pathIndex++;
        } else {
            const Vec2 n = d.normalized();
            in.moveX = n.x;
            in.moveY = n.y;
        }
    } else {
        in.moveX *= 0.8f;
        in.moveY *= 0.8f;
    }

    PlayerState* target = (bot.botTarget != 255) ? world.playerById(bot.botTarget) : nullptr;
    in.fire = false;
    in.reload = false;
    if (target && target->alive && !target->spectating) {
        const bool los = world.map().lineOfSight(bot.pos, target->pos);
        const float desired = angleTo(bot.pos, target->pos);
        const float turn = angleDiff(bot.yaw, desired);
        bot.yaw = wrapAngle(bot.yaw + clampf(turn, -diff.aimSpeed * dt, diff.aimSpeed * dt));
        in.yaw = bot.yaw;
        const float dist = distance(bot.pos, target->pos);
        if (los && std::fabs(turn) < (0.35f + (1.0f - diff.accuracy)) && dist < 28.0f) {
            if (world.rng().nextFloat() < diff.accuracy + difficultyLead(diff) * 0.1f) {
                in.fire = true;
            }
        }
        if (dist < 7.0f) bot.weaponSlot = 1;
        else bot.weaponSlot = 0;
    } else {
        in.yaw = bot.yaw;
    }

    if (bot.weapons[bot.weaponSlot].ammoInMag <= 0) in.reload = true;
    in.sprint = distance(bot.pos, world.hill()) > 10.0f;
    bot.input = in;
}

} // namespace sbs
